"""Build a per-user NSIS installer from the already verified deployment tree."""

from pathlib import Path, PurePosixPath
import argparse
import json
import re
import shutil
import subprocess
import zipfile

from prepare_distribution import ROOT, fetch, safe_target, write_json


def nsis_string(value: str) -> str:
    if any(c in value for c in "\r\n\0"):
        raise ValueError("NSIS paths cannot contain control characters.")
    return value.replace("$", "$$").replace('"', '$\\"')


def uninstall_manifest(app: Path) -> str:
    files, directories = [], []
    for path in app.rglob("*"):
        if path.is_symlink() or not path.resolve().is_relative_to(app.resolve()):
            raise ValueError("Installer input must contain only files within its deployment tree.")
        relative = path.relative_to(app)
        if relative.name.lower() == "uninstall.exe":
            raise ValueError("Build the installer from a clean stage, not an existing installation.")
        (directories if path.is_dir() else files).append(relative)
    lines = [f'Delete "$INSTDIR\\{nsis_string(str(path))}"' for path in sorted(files)]
    # Deleting nonempty directories must not signal a failed uninstall; user
    # documents can deliberately remain in them. Check only owned file errors.
    lines += ['IfErrors failed']
    lines += [f'RMDir "$INSTDIR\\{nsis_string(str(path))}"' for path in sorted(directories, key=lambda p: (len(p.parts), str(p)), reverse=True)]
    lines += ['ClearErrors']
    return "\n".join(lines) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--work", type=Path, default=ROOT / "out/installer")
    parser.add_argument("--offline", action="store_true")
    args = parser.parse_args()
    app, output, work = args.app_dir.resolve(), args.output.resolve(), args.work.resolve()
    metadata = json.loads((app / "application.json").read_text(encoding="utf-8"))
    version = metadata["version"]
    if not re.fullmatch(r"\d+\.\d+\.\d+", version) or not (app / "runtime-inventory.json").is_file():
        raise ValueError("A versioned, inventoried deployment is required.")
    tool = json.loads((ROOT / "packaging/build_tools.json").read_text(encoding="utf-8"))["nsis"]
    archive = fetch(tool, ROOT / "out/build-tools", args.offline)
    compiler_root = work / tool["root"]
    with zipfile.ZipFile(archive) as package:
        for entry in package.infolist():
            path = PurePosixPath(entry.filename)
            if not path.parts or path.parts[0] != tool["root"]:
                raise ValueError("Unexpected NSIS archive root.")
            target = safe_target(work, entry.filename)
            if entry.is_dir():
                target.mkdir(parents=True, exist_ok=True)
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                with package.open(entry) as source, target.open("wb") as destination:
                    shutil.copyfileobj(source, destination)
    shutil.copyfile(compiler_root / "COPYING", app / "licenses/vendor/nsis-COPYING.txt")
    manifest_path = work / "uninstall-files.nsh"
    manifest_path.write_text(uninstall_manifest(app), encoding="utf-8", newline="\n")
    output.parent.mkdir(parents=True, exist_ok=True)
    defines = {"APP_DIR": str(app), "APP_VERSION": version, "OUTPUT_FILE": str(output), "UNINSTALL_FILES": str(manifest_path)}
    script = work / "build.nsi"
    script.write_text("\n".join(f'!define {key} "{nsis_string(value)}"' for key, value in defines.items()) +
                      f'\n!include "{nsis_string(str(ROOT / "packaging/windows/installer.nsi"))}"\n', encoding="utf-8", newline="\n")
    subprocess.run([str(compiler_root / "makensis.exe"), "/V3", "/WX", "/INPUTCHARSET", "UTF8", str(script)], check=True)
    if not output.is_file():
        raise ValueError("NSIS did not produce the installer.")
    write_json(output.with_suffix(".build.json"), {"application": metadata, "installerCompiler": tool, "scope": "current user", "compressor": "zlib"})


if __name__ == "__main__":
    main()
