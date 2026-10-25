"""Create a reviewable draft from successful jobs and matching, verified artifacts."""

from pathlib import Path, PurePosixPath
import argparse
import json
import re
import stat
import subprocess
import zipfile

from prepare_distribution import digest, safe_target, verify

REQUIRED_JOBS = {"Core / Linux / sanitizers", "Windows desktop and portable executable"}


def validate_build(repository: str, run: dict, jobs: list[dict], metadata: dict, tag: str) -> None:
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repository):
        raise ValueError("Invalid repository name.")
    if (run.get("repository", {}).get("full_name", "").lower() != repository.lower()
            or run.get("head_repository", {}).get("full_name", "").lower() != repository.lower()
            or run.get("path") != ".github/workflows/build.yml"
            or run.get("event") not in {"push", "workflow_dispatch"}):
        raise ValueError("Release artifacts must come from this repository's trusted build workflow.")
    if run.get("status") not in {"in_progress", "completed"} or (run.get("status") == "completed" and run.get("conclusion") != "success"):
        raise ValueError("The build did not succeed.")
    successful = {job["name"] for job in jobs if job.get("conclusion") == "success"}
    if not REQUIRED_JOBS.issubset(successful):
        raise ValueError("Required core and Windows jobs have not both succeeded.")
    if not re.fullmatch(r"[0-9a-f]{40}", run.get("head_sha", "")) or metadata.get("sourceRevision") != run["head_sha"]:
        raise ValueError("Artifact/source commit mismatch.")
    version = metadata.get("version", "")
    if not re.fullmatch(r"\d+\.\d+\.\d+", version) or len(tag) > 80 or not re.fullmatch(r"v" + re.escape(version) + r"(?:-[0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*)?", tag):
        raise ValueError("Release tag must match the application's CMake version.")


def validate_existing_release(release: dict | None, marker: str) -> None:
    if release is not None and (not release.get("draft") or marker not in release.get("body", "")):
        raise ValueError("Refusing to change a published release or another draft.")


def gh_json(path: str) -> dict | list:
    result = subprocess.run(["gh", "api", path], capture_output=True, text=True, encoding="utf-8")
    if result.returncode:
        raise RuntimeError(result.stderr)
    return json.loads(result.stdout)


def find_release(repository: str, tag: str) -> dict | None:
    # The tag-specific REST endpoint returns published releases. The list
    # endpoint also includes drafts for this authenticated workflow owner.
    for page in range(1, 101):
        releases = gh_json(f"repos/{repository}/releases?per_page=100&page={page}")
        for release in releases:
            if release["tag_name"] == tag:
                return release
        if len(releases) < 100:
            return None
    raise ValueError("Release listing exceeds the verification limit.")


def extract_runtime(archive: Path, output: Path) -> Path:
    with zipfile.ZipFile(archive) as source:
        entries = source.infolist()
        if len(entries) > 20000 or sum(item.file_size for item in entries) > 1024 * 1024 * 1024:
            raise ValueError("Unexpected runtime archive size.")
        for entry in entries:
            parts = PurePosixPath(entry.filename).parts
            if not parts or parts[0] != "Vibmory" or stat.S_ISLNK(entry.external_attr >> 16):
                raise ValueError("Unexpected runtime archive root or symlink.")
            safe_target(output, entry.filename)
        source.extractall(output)
    return output / "Vibmory"


def check_asset(path: Path) -> None:
    expected = path.with_name(path.name + ".sha256").read_text(encoding="utf-8-sig").strip().lower()
    if not re.fullmatch(r"[0-9a-f]{64}", expected) or digest(path) != expected:
        raise ValueError(f"Artifact checksum mismatch: {path.name}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", required=True)
    parser.add_argument("--run-id", type=int, required=True)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--work", type=Path, default=Path("artifacts/release"))
    args = parser.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", args.repository):
        raise ValueError("Invalid repository name.")
    work = args.work.resolve()
    assets, sources = work / "assets", work / "sources"
    if work.exists() and any(work.iterdir()):
        raise ValueError("Release work directory must be empty.")
    work.mkdir(parents=True, exist_ok=True)
    for name, directory in (("Vibmory-windows-x64", assets), ("Vibmory-dependency-sources", sources)):
        subprocess.run(["gh", "run", "download", str(args.run_id), "--repo", args.repository, "--name", name, "--dir", str(directory)], check=True)
    runtime = assets / "Vibmory-windows-x64.zip"
    installer = assets / "Vibmory-windows-x64-setup.exe"
    check_asset(runtime)
    check_asset(installer)
    app = extract_runtime(runtime, work / "runtime")
    metadata = json.loads((app / "application.json").read_text(encoding="utf-8"))
    run = gh_json(f"repos/{args.repository}/actions/runs/{args.run_id}")
    jobs = gh_json(f"repos/{args.repository}/actions/runs/{args.run_id}/jobs?per_page=100")["jobs"]
    validate_build(args.repository, run, jobs, metadata, args.tag)
    reference = gh_json(f"repos/{args.repository}/git/ref/tags/{args.tag}")["object"]
    for _ in range(8):
        if reference["type"] != "tag":
            break
        reference = gh_json(f"repos/{args.repository}/git/tags/{reference['sha']}")["object"]
    if reference["type"] != "commit" or reference["sha"] != run["head_sha"]:
        raise ValueError("Release tag points to a different source commit.")
    verify(app, sources)
    provenance = json.loads((sources / "PROVENANCE.json").read_text(encoding="utf-8"))
    if provenance["application"] != metadata:
        raise ValueError("Dependency-source provenance does not match the application.")
    qa = json.loads((assets / "installer-qa.json").read_text(encoding="utf-8-sig"))
    if not all(qa.get(key) is True for key in ("installed", "upgraded", "launchedWithoutQt", "busyInstallRejected", "busyUninstallRejected", "userProjectPreserved", "uninstalled")):
        raise ValueError("Installer acceptance checks are missing or failed.")
    signing = [json.loads((assets / name).read_text(encoding="utf-8-sig"))["signed"] for name in ("application-signing.json", "installer-signing.json")]
    if signing[0] != signing[1] or not all(isinstance(value, bool) for value in signing):
        raise ValueError("Application/installer signing reports are inconsistent.")
    dependency_zip = assets / "Vibmory-dependency-sources.zip"
    with zipfile.ZipFile(dependency_zip, "w", compression=zipfile.ZIP_STORED) as archive:
        for path in sorted(sources.rglob("*")):
            if path.is_file():
                archive.write(path, path.relative_to(sources).as_posix())
    dependency_zip.with_name(dependency_zip.name + ".sha256").write_text(digest(dependency_zip) + "\n", encoding="ascii")
    primary = [runtime, installer, dependency_zip]
    checksums = assets / "SHA256SUMS.txt"
    checksums.write_text("".join(f"{digest(path)}  {path.name}\n" for path in primary), encoding="ascii")
    marker = f"<!-- vibmory-build:{args.run_id}:{run['head_sha']} -->"
    existing = find_release(args.repository, args.tag)
    validate_existing_release(existing, marker)
    signature = "Authenticode signed and verified" if signing[0] else "Unsigned test build; no signing certificate is configured"
    body = (f"{marker}\n\nVibmory {metadata['version']} for Windows x64. This is a draft for review and device testing.\n\n"
            f"Built from [{run['head_sha'][:12]}](https://github.com/{args.repository}/commit/{run['head_sha']}) in "
            f"[verified build {args.run_id}](https://github.com/{args.repository}/actions/runs/{args.run_id}). "
            "Linux sanitizer/core checks and Windows core/UI, package, install, upgrade, busy-file and data-preserving uninstall checks passed.\n\n"
            "- Portable ZIP: extract and launch `Vibmory/Vibmory.exe`. Qt is bundled.\n"
            "- Setup EXE: installs for the current user without administrator rights; Windows 10 version 1809 or newer, x64.\n"
            "- Dependency source ZIP: matching Qt/FFmpeg sources and upstream software-renderer sources, licenses, provenance and build/replacement directions.\n"
            f"- Signing: {signature}.\n\n"
            "Vibmory's own code uses MIT. This software uses Qt under LGPLv3 and libraries from FFmpeg under LGPLv2.1-or-later. "
            "The source ZIP and bundled notices provide their source access and terms. Actual actuator routing/latency remains a device check.\n\n"
            "Verify downloads against `SHA256SUMS.txt` or the individual checksum files.\n")
    notes = work / "release-notes.md"
    notes.write_text(body, encoding="utf-8", newline="\n")
    upload = primary + [path.with_name(path.name + ".sha256") for path in primary] + [checksums]
    if existing:
        subprocess.run(["gh", "release", "upload", args.tag, "--repo", args.repository, "--clobber", *map(str, upload)], check=True)
        subprocess.run(["gh", "release", "edit", args.tag, "--repo", args.repository, "--notes-file", str(notes)], check=True)
    else:
        command = ["gh", "release", "create", args.tag, "--repo", args.repository, "--verify-tag", "--target", run["head_sha"],
                   "--draft", "--title", f"Vibmory {args.tag.removeprefix('v')}", "--notes-file", str(notes)]
        if "-" in args.tag:
            command.append("--prerelease")
        subprocess.run([*command, *map(str, upload)], check=True)
    release = find_release(args.repository, args.tag)
    if release is None or not release["draft"]:
        raise ValueError("Release must remain a draft.")
    print(release["html_url"])


if __name__ == "__main__":
    main()
