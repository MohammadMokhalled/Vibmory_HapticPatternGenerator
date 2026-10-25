"""Validate catalog completeness and optionally compare against Qt's extractor."""

from collections import Counter
from pathlib import Path
import argparse
import json
import re
import shutil
import subprocess
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "translations/vibmory_de.ts"


def messages(path: Path) -> dict[tuple[str, str], ET.Element]:
    root = ET.parse(path).getroot()
    if root.get("language") != "de_DE" or root.get("sourcelanguage") != "en_US":
        raise ValueError("Expected German catalog with English source language.")
    result = {}
    for context in root.findall("context"):
        for message in context.findall("message"):
            translation = message.find("translation")
            if translation is not None and translation.get("type") in {"obsolete", "vanished"}:
                continue
            key = (context.findtext("name", ""), message.findtext("source", ""))
            if key in result:
                raise ValueError(f"Duplicate translation: {key}")
            result[key] = message
    return result


def source_messages() -> set[tuple[str, str]]:
    # The Qt extractor is authoritative in desktop CI. This small scanner lets
    # Qt-free CI catch new ordinary tr() literals without installing a Qt SDK.
    result = set()
    for source in (ROOT / "src/ui").glob("*.cpp"):
        text = source.read_text(encoding="utf-8")
        constructor = re.search(r"\b(\w+)::\1\(", text)
        for match in re.finditer(r'\btr\s*\(\s*((?:"(?:[^"\\]|\\.)*"\s*)+)', text):
            if constructor is None:
                raise ValueError(f"Cannot infer translation context in {source.name}")
            literal = "".join(json.loads(s) for s in re.findall(r'"(?:[^"\\]|\\.)*"', match[1]))
            result.add(("vibmory::" + constructor[1], literal))
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lupdate", help="Path/name of Qt's lupdate executable")
    args = parser.parse_args()
    catalog = messages(CATALOG)
    if set(catalog) != source_messages():
        raise ValueError(f"Catalog/source mismatch: {set(catalog) ^ source_messages()}")
    for key, message in catalog.items():
        translation = message.find("translation")
        if translation is None or translation.get("type") == "unfinished" or not translation.text:
            raise ValueError(f"Unfinished translation: {key}")
        for pattern in (r"%L?\d+|%n", r"(?<!&)&(?!&)", r"\*\.[A-Za-z0-9]+"):
            if Counter(re.findall(pattern, key[1])) != Counter(re.findall(pattern, translation.text)):
                raise ValueError(f"Placeholder, accelerator, or file filter mismatch: {key}")
    if args.lupdate:
        with tempfile.TemporaryDirectory(prefix="vibmory-translations-") as directory:
            extracted = Path(directory) / CATALOG.name
            shutil.copyfile(CATALOG, extracted)
            result = subprocess.run([args.lupdate, str(ROOT / "src/ui"), "-I", str(ROOT / "src"), "-no-obsolete", "-locations", "none", "-ts", str(extracted)],
                                    capture_output=True, text=True, encoding="utf-8")
            if result.returncode:
                raise ValueError(result.stdout + result.stderr)
            actual = messages(extracted)
            if set(actual) != set(catalog):
                difference = sorted(set(actual) ^ set(catalog))
                raise ValueError(f"Qt extractor/catalog mismatch ({len(difference)} keys): {difference[:12]}\n{result.stdout}\n{result.stderr}")
    print(f"Validated {len(catalog)} German UI messages" + (" against Qt lupdate." if args.lupdate else "."))


if __name__ == "__main__":
    main()
