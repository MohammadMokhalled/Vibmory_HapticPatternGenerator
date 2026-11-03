"""Check owned source formatting and dependency direction without third-party modules."""

from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
ALLOWED = {
    "domain": {"domain"},
    "application": {"domain", "application"},
    "audio": {"domain", "audio"},
    "persistence": {"domain", "persistence"},
    "platform": {"domain", "application", "audio", "persistence", "platform"},
    "ui": {"domain", "application", "platform", "persistence", "audio", "ui"},
    "app": set(),
}


def main() -> int:
    failures = []
    sources = sorted(p for folder in (ROOT / "src", ROOT / "tests", ROOT / "tools")
                     for p in folder.rglob("*") if p.suffix in {".cpp", ".h"})
    formatter = shutil.which("clang-format")
    if formatter is None:
        failures.append("Install clang-format 21.1.0 (python -m pip install clang-format==21.1.0).")
    else:
        version = subprocess.check_output([formatter, "--version"], text=True)
        if "21.1.0" not in version:
            failures.append(f"Expected clang-format 21.1.0; found {version.strip()}")
        else:
            for source in sources:
                result = subprocess.run([formatter, "--dry-run", "--Werror", str(source)],
                                        capture_output=True, text=True, cwd=ROOT)
                if result.returncode:
                    failures.append(result.stderr.strip())
    for source in sources:
        text = source.read_text(encoding="utf-8")
        relative = source.relative_to(ROOT)
        if any(line.rstrip() != line for line in text.splitlines()) or not text.endswith("\n"):
            failures.append(f"Whitespace/newline violation: {relative}")
        if relative.parts[0] != "src":
            continue
        module = relative.parts[1]
        for included in re.findall(r'^#include\s+["<]([^">]+)[">]', text, re.MULTILINE):
            dependency = included.split("/")[0]
            if module != "app" and dependency in ALLOWED and dependency not in ALLOWED[module]:
                failures.append(f"Forbidden dependency: {relative} -> {included}")
            if module in {"domain", "application", "audio", "persistence"} and dependency.startswith("Q"):
                failures.append(f"Qt must remain outside core modules: {relative} -> {included}")
    for failure in failures:
        print(failure, file=sys.stderr)
    print(f"Checked {len(sources)} owned C++ files; {len(failures)} quality failures.")
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
