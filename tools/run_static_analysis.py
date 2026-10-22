"""Run focused Clang analysis against the real core compilation database."""

from pathlib import Path
import json
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
build = Path(sys.argv[1] if len(sys.argv) > 1 else ROOT / "out/core-preset").resolve()
tool = shutil.which("clang-tidy")
if tool is None:
    raise SystemExit("Install clang-tidy 21.1.0: python -m pip install clang-tidy==21.1.0")
version = subprocess.check_output([tool, "--version"], text=True)
if "21.1.0" not in version:
    raise SystemExit(f"Expected clang-tidy 21.1.0; found {version.strip()}")
database = build / "compile_commands.json"
if not database.is_file():
    raise SystemExit("Configure a Ninja preset with CMAKE_EXPORT_COMPILE_COMMANDS=ON first.")
entries = json.loads(database.read_text(encoding="utf-8"))
sources = set()
for entry in entries:
    source = Path(entry["file"]).resolve()
    relative = source.relative_to(ROOT)
    if relative.parts[:1] == ("src",) and relative.parts[1] in {
        "domain", "application", "audio", "persistence"
    }:
        sources.add(source)
if not sources:
    raise SystemExit("No core sources found; refusing an empty analysis pass.")
failed = False
for source in sorted(sources):
    print(f"Analyzing {source.relative_to(ROOT)}", flush=True)
    failed |= subprocess.run([tool, str(source), "-p", str(build), "--quiet"], cwd=ROOT).returncode != 0
raise SystemExit(failed)
