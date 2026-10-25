"""Assemble pinned dependency notices/sources and inventory the deployed runtime."""

from pathlib import Path, PurePosixPath
import argparse
import ctypes
import hashlib
import json
import os
import posixpath
import re
import shutil
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
MAX_DOWNLOAD_BYTES = 120 * 1024 * 1024


def digest(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def write_json(path: Path, data: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2, sort_keys=True) + "\n", encoding="utf-8", newline="\n")


def safe_target(directory: Path, name: str) -> Path:
    relative = PurePosixPath(name)
    if relative.is_absolute() or ".." in relative.parts or ":" in name or "\\" in name:
        raise ValueError(f"Unsafe distribution path: {name}")
    target = (directory / Path(*relative.parts)).resolve()
    if not target.is_relative_to(directory.resolve()):
        raise ValueError(f"Distribution path escapes output: {name}")
    return target


def fetch(entry: dict, cache: Path, offline: bool) -> Path:
    path = safe_target(cache, entry["file"])
    if not path.exists():
        if offline:
            raise ValueError(f"Missing cached dependency source: {path.name}")
        path.parent.mkdir(parents=True, exist_ok=True)
        temporary = path.with_name(path.name + ".part")
        with urllib.request.urlopen(entry["url"], timeout=30) as response, temporary.open("wb") as output:
            size = 0
            while block := response.read(1024 * 1024):
                size += len(block)
                if size > MAX_DOWNLOAD_BYTES:
                    raise ValueError(f"Dependency download exceeds size limit: {entry['file']}")
                output.write(block)
        if digest(temporary) != entry["sha256"]:
            raise ValueError(f"Dependency checksum mismatch: {entry['file']}")
        temporary.replace(path)
    if digest(path) != entry["sha256"]:
        raise ValueError(f"Cached dependency checksum mismatch: {entry['file']}")
    return path


def collect_notices(archive: Path, output: Path, component: str) -> list[dict]:
    records = []
    with tarfile.open(archive) as source:
        members = {member.name: member for member in source.getmembers()}
        selected = set()
        for name, member in members.items():
            if not member.isfile():
                continue
            parts = PurePosixPath(name).parts
            safe_target(output, name)  # Validate every source path before use.
            filename = parts[-1].lower()
            if ("LICENSES" in parts or re.search(r"license|copying|copyright|notice|authors", filename)
                    or (filename.startswith("readme") and "3rdparty" in parts)):
                selected.add(name)
            if filename == "qt_attribution.json" and "tests" not in parts:
                selected.add(name)
                # Qt's historical metadata permits raw newlines in strings.
                data = json.loads(source.extractfile(member).read(), strict=False)
                for record in ([data] if isinstance(data, dict) else data):
                    records.append({"component": component, "sourceFile": name, **record})
                    if license_file := record.get("LicenseFile"):
                        resolved = posixpath.normpath(posixpath.join(posixpath.dirname(name), license_file))
                        if PurePosixPath(resolved).parts[0] != parts[0]:
                            raise ValueError(f"License reference escapes source root: {resolved}")
                        if resolved not in members or not members[resolved].isfile():
                            raise ValueError(f"Missing upstream license file: {resolved}")
                        selected.add(resolved)
        for name in sorted(selected):
            member = members[name]
            if member.size > 16 * 1024 * 1024:
                raise ValueError(f"Unexpectedly large notice file: {name}")
            relative = PurePosixPath(*PurePosixPath(name).parts[1:]).as_posix()
            destination = safe_target(output / component, relative)
            destination.parent.mkdir(parents=True, exist_ok=True)
            with source.extractfile(member) as content, destination.open("wb") as target:
                shutil.copyfileobj(content, target)
    return records


def component_for(path: Path) -> str:
    name = path.name.lower()
    if name == "vibmory.exe":
        return "vibmory"
    if name.startswith(("avcodec-", "avformat-", "avutil-", "swresample-", "swscale-")):
        return "ffmpeg"
    if name == "opengl32sw.dll":
        return "mesa-llvm"
    if name in {"dxcompiler.dll", "dxil.dll"}:
        return "directx-shader-compiler"
    if name == "d3dcompiler_47.dll":
        return "microsoft-windows-sdk"
    if name.startswith(("msvcp", "vcruntime", "vccorlib", "concrt", "vc_redist")):
        return "microsoft-cpp-runtime"
    if name in {"qt6svg.dll", "qsvg.dll", "qsvgicon.dll"}:
        return "qtsvg"
    if name in {"qt6multimedia.dll", "ffmpegmediaplugin.dll", "windowsmediaplugin.dll"}:
        return "qtmultimedia"
    if name in {"qt6core.dll", "qt6gui.dll", "qt6widgets.dll", "qt6network.dll", "qt6concurrent.dll",
                "qtuiotouchplugin.dll", "qgif.dll", "qico.dll", "qjpeg.dll", "qnetworklistmanager.dll",
                "qwindows.dll", "qmodernwindowsstyle.dll", "qcertonlybackend.dll", "qschannelbackend.dll"}:
        return "qtbase"
    if path.suffix.lower() == ".qm":
        return "qttranslations"
    raise ValueError(f"Unreviewed runtime dependency: {path.as_posix()}")


def file_version(path: Path) -> str | None:
    if os.name != "nt":
        return None
    api = ctypes.WinDLL("version", use_last_error=True)
    api.GetFileVersionInfoSizeW.argtypes = [ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_uint32)]
    api.GetFileVersionInfoSizeW.restype = ctypes.c_uint32
    api.GetFileVersionInfoW.argtypes = [ctypes.c_wchar_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_void_p]
    api.VerQueryValueW.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_uint32)]
    size = api.GetFileVersionInfoSizeW(str(path), None)
    if not size:
        return None
    buffer = ctypes.create_string_buffer(size)
    if not api.GetFileVersionInfoW(str(path), 0, size, buffer):
        raise OSError(f"Cannot read binary version: {path.name}")
    pointer, length = ctypes.c_void_p(), ctypes.c_uint32()
    if not api.VerQueryValueW(buffer, "\\", ctypes.byref(pointer), ctypes.byref(length)) or length.value < 52:
        return None
    values = ctypes.cast(pointer, ctypes.POINTER(ctypes.c_uint32))
    high, low = values[2], values[3]
    return f"{high >> 16}.{high & 65535}.{low >> 16}.{low & 65535}"


def ffmpeg_build(app: Path, expected_version: str) -> dict:
    if os.name != "nt":
        raise ValueError("Runtime verification must run on the supported Windows build host.")
    with os.add_dll_directory(str(app)):
        codec = ctypes.WinDLL(str(app / "avcodec-61.dll"))
        utility = ctypes.WinDLL(str(app / "avutil-59.dll"))
        codec.avcodec_license.restype = ctypes.c_char_p
        codec.avcodec_configuration.restype = ctypes.c_char_p
        utility.av_version_info.restype = ctypes.c_char_p
        result = {"version": utility.av_version_info().decode(), "license": codec.avcodec_license().decode(),
                  "configure": codec.avcodec_configuration().decode()}
    if result["version"] != expected_version or not result["license"].startswith("LGPL version 2.1"):
        raise ValueError(f"FFmpeg binary/source version or license mismatch: {result}")
    if any(option in result["configure"] for option in ("--enable-gpl", "--enable-nonfree", "--enable-version3")):
        raise ValueError("Unexpected FFmpeg license-affecting build flags.")
    return result


def verify(app: Path, sources: Path) -> None:
    inventory = json.loads((app / "runtime-inventory.json").read_text(encoding="utf-8"))
    expected = {item["path"] for item in inventory["files"]}
    actual = {path.relative_to(app).as_posix() for path in app.rglob("*") if path.suffix.lower() in {".dll", ".exe", ".qm"}}
    if expected != actual:
        raise ValueError("Runtime inventory does not match the deployed file set.")
    for item in inventory["files"]:
        path = safe_target(app, item["path"])
        if digest(path) != item["sha256"] or component_for(Path(item["path"])) != item["component"]:
            raise ValueError(f"Runtime inventory mismatch: {item['path']}")
    for entry in inventory["sourceArchives"]:
        if digest(safe_target(sources, entry["file"])) != entry["sha256"]:
            raise ValueError(f"Source archive mismatch: {entry['file']}")
    for entry in inventory["noticeSources"]:
        if digest(safe_target(app / "licenses/vendor", entry["file"])) != entry["sha256"]:
            raise ValueError(f"Vendor notice mismatch: {entry['file']}")
    if inventory["application"] != json.loads((app / "application.json").read_text(encoding="utf-8")):
        raise ValueError("Application metadata/inventory mismatch.")
    for required in ("LICENSE", "THIRD_PARTY_NOTICES.md", "licenses/qtbase/LICENSES/LGPL-3.0-only.txt",
                     "licenses/ffmpeg/COPYING.LGPLv2.1", "licenses/upstream-attributions.json"):
        if not (app / required).is_file():
            raise ValueError(f"Missing distribution notice: {required}")
    print(f"Verified {len(expected)} inventoried runtime files and {len(inventory['sourceArchives'])} source archives.")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app-dir", type=Path, required=True)
    parser.add_argument("--sources-dir", type=Path, required=True)
    parser.add_argument("--cache", type=Path, default=ROOT / "out/distribution-sources")
    parser.add_argument("--qt-root", type=Path)
    parser.add_argument("--offline", action="store_true")
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    app, sources = args.app_dir.resolve(), args.sources_dir.resolve()
    if args.verify:
        verify(app, sources)
        return
    metadata = json.loads((app / "application.json").read_text(encoding="utf-8"))
    manifest = json.loads((ROOT / "packaging/source_archives.json").read_text(encoding="utf-8"))
    notice_manifest = json.loads((ROOT / "packaging/notice_sources.json").read_text(encoding="utf-8"))
    sources.mkdir(parents=True, exist_ok=True)
    attributions = []
    for entry in manifest["archives"]:
        archive = fetch(entry, args.cache, args.offline)
        shutil.copyfile(archive, safe_target(sources, entry["file"]))
        attributions.extend(collect_notices(archive, app / "licenses", entry["id"]))
    for entry in notice_manifest["files"]:
        notice = fetch(entry, args.cache, args.offline)
        destination = safe_target(app / "licenses/vendor", entry["file"])
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(notice, destination)
    write_json(app / "licenses/upstream-attributions.json", {"scope": "Upstream source-module notices, a superset of the deployed features.", "records": attributions})
    if args.qt_root and (args.qt_root / "sbom").is_dir():
        for path in (args.qt_root / "sbom").rglob("*"):
            if path.is_file() and path.name.startswith(("qtbase-", "qtmultimedia-", "qtsvg-", "qttranslations-")):
                target = app / "licenses/qt-sdk-sbom" / path.name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(path, target)
    audio = ffmpeg_build(app, next(e["version"] for e in manifest["archives"] if e["id"] == "ffmpeg"))
    write_json(app / "licenses/ffmpeg-build.json", audio)
    write_json(sources / "PROVENANCE.json", {**manifest, "notices": notice_manifest["files"], "application": metadata, "ffmpegBuild": audio})
    shutil.copyfile(ROOT / "docs/Distribution.md", sources / "BUILD_AND_REPLACE.md")
    files = []
    for path in sorted(app.rglob("*")):
        if path.suffix.lower() not in {".dll", ".exe", ".qm"}:
            continue
        relative = path.relative_to(app)
        version = file_version(path)
        if path.name.startswith("Qt6") and version != manifest["qtVersion"] + ".0":
            raise ValueError(f"Qt binary/source version mismatch: {path.name}: {version}")
        if path.name == "Vibmory.exe" and version != metadata["version"] + ".0":
            raise ValueError("Executable/CMake application version mismatch.")
        files.append({"path": relative.as_posix(), "component": component_for(relative), "sha256": digest(path), "version": version})
    write_json(app / "runtime-inventory.json", {"schemaVersion": 1, "application": metadata,
               "sourceArchives": manifest["archives"], "noticeSources": notice_manifest["files"], "files": files})
    verify(app, sources)


if __name__ == "__main__":
    main()
