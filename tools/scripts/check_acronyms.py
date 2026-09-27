#!/usr/bin/env python3
"""Check documented acronyms in repository documentation and source comments."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
GLOSSARY = ROOT / "docs" / "GLOSSARY.md"
IGNORE_FILE = ROOT / "tools" / "scripts" / "acronym-ignore.txt"
COMMENT_EXTENSIONS = {".c", ".h", ".proto", ".py"}
MARKDOWN_EXTENSIONS = {".md"}
SKIP_PARTS = {".git", "build", ".venv"}

# Uppercase words with at least two letters, optionally ending in digits.
ACRONYM_RE = re.compile(r"\b[A-Z][A-Z0-9]{1,}\b")
TABLE_ROW_RE = re.compile(r"^\|\s*([^|]+?)\s*\|", re.MULTILINE)


def normalize(term: str) -> str:
    """Normalize glossary terms and discovered words for comparison."""
    return re.sub(r"[^A-Za-z0-9]", "", term).lower()


def load_glossary() -> set[str]:
    glossary_text = GLOSSARY.read_text(encoding="utf-8")
    terms = set()
    for term in TABLE_ROW_RE.findall(glossary_text):
        if term.lower() != "term" and set(term.strip()) != {"-"}:
            clean_term = term.strip(" `")
            terms.add(normalize(clean_term))
            # A row such as "QR code" or "ARM7 / ARM9" defines each
            # acronym used in the compound term as well.
            for component in re.split(r"[/ ]+", clean_term):
                component = component.strip("`.,()")
                if component:
                    terms.add(normalize(component))
                    terms.add(normalize(re.split(r"[=]", component, maxsplit=1)[0]))
                    terms.add(normalize(re.sub(r"\d+$", "", component)))
    return terms


def load_ignore_list() -> set[str]:
    if not IGNORE_FILE.exists():
        return set()
    return {
        normalize(line.strip())
        for line in IGNORE_FILE.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.lstrip().startswith("#")
    }


def source_comments(text: str, suffix: str) -> str:
    """Return comments from C-like and Python source files."""
    if suffix == ".py":
        return "\n".join(line.split("#", 1)[1] for line in text.splitlines() if "#" in line)

    comments: list[str] = []
    block = re.compile(r"/\*.*?\*/", re.DOTALL)
    comments.extend(block.findall(text))
    for line in text.splitlines():
        if "//" in line:
            comments.append(line.split("//", 1)[1])
    return "\n".join(comments)


def files_to_check() -> list[Path]:
    paths = []
    for path in ROOT.rglob("*"):
        if not path.is_file() or any(part in SKIP_PARTS for part in path.parts):
            continue
        if path.suffix in MARKDOWN_EXTENSIONS or path.suffix in COMMENT_EXTENSIONS:
            paths.append(path)
    return paths


def main() -> int:
    glossary = load_glossary()
    ignored = load_ignore_list()
    failures: dict[Path, set[str]] = {}

    for path in files_to_check():
        text = path.read_text(encoding="utf-8", errors="replace")
        content = text if path.suffix in MARKDOWN_EXTENSIONS else source_comments(text, path.suffix)
        missing = {
            word
            for word in ACRONYM_RE.findall(content)
            if normalize(word) not in glossary and normalize(word) not in ignored
        }
        if missing:
            failures[path.relative_to(ROOT)] = missing

    if failures:
        print("Acronyms missing from docs/GLOSSARY.md or tools/scripts/acronym-ignore.txt:")
        for path, words in sorted(failures.items()):
            print(f"  {path}: {', '.join(sorted(words))}")
        return 1

    print("All detected acronyms are defined in the glossary or ignored.")
    return 0


if __name__ == "__main__":
    sys.exit(main())