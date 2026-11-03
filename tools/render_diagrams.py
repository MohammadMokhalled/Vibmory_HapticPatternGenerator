"""Render maintained Graphviz sources to PNG and SVG; Graphviz is optional for builds."""

from pathlib import Path
import argparse
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, default=ROOT / "docs/diagrams",
                        help="Use out/diagrams to validate without replacing tracked images.")
    args = parser.parse_args()
    dot = shutil.which("dot")
    if dot is None:
        raise SystemExit("Graphviz dot was not found. Install Graphviz and put dot on PATH.")
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    for source in sorted((ROOT / "docs/diagrams").glob("*.dot")):
        for format_name in ("png", "svg"):
            destination = output / f"{source.stem}.{format_name}"
            subprocess.run([dot, f"-T{format_name}", str(source), "-o", str(destination)], check=True)
            if format_name == "svg":
                text = destination.read_text(encoding="utf-8")
                destination.write_text(text, encoding="utf-8", newline="\n")
            print(destination.relative_to(ROOT) if destination.is_relative_to(ROOT) else destination)


if __name__ == "__main__":
    main()
