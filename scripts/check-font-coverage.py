#!/usr/bin/env python3
"""Verify that UI string literals fit the generated Simplified Chinese font."""

from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[1]
CALENDAR_HOME_SRC = ROOT / "application/edge_agent/components/calendar_home/src"
FONT_PATH = CALENDAR_HOME_SRC / "calendar_font_zh.c"
SOURCE_PATHS = [
    CALENDAR_HOME_SRC / "calendar_ui.c",
    CALENDAR_HOME_SRC / "calendar_model.c",
    CALENDAR_HOME_SRC / "calendar_board_data.c",
    CALENDAR_HOME_SRC / "calendar_home.c",
    ROOT / "sim/main_sdl.c",
]


def parse_covered_codepoints(font_source):
    covered = set(range(32, 128))
    for match in re.finditer(r"\{\s*(0x[0-9a-fA-F]+)\s*,", font_source):
        covered.add(int(match.group(1), 16))
    for match in re.finditer(r"/\*\s*U\+([0-9a-fA-F]+)\s+", font_source):
        covered.add(int(match.group(1), 16))
    return covered


def load_coverage():
    if not FONT_PATH.exists():
        raise RuntimeError(f"generated font file not found: {FONT_PATH}")

    font_source = FONT_PATH.read_text(encoding="utf-8")
    return parse_covered_codepoints(font_source)


def extract_string_literals(path):
    source = path.read_text(encoding="utf-8")
    return re.findall(r'"((?:[^"\\]|\\.)*)"', source)


def main():
    covered = load_coverage()
    missing = {}

    for path in SOURCE_PATHS:
        for literal in extract_string_literals(path):
            for char in literal:
                codepoint = ord(char)
                if codepoint > 127 and codepoint not in covered:
                    missing.setdefault(char, set()).add((path.relative_to(ROOT), literal))

    if missing:
        for char, uses in sorted(missing.items(), key=lambda item: ord(item[0])):
            refs = ", ".join(f"{path}:{literal}" for path, literal in sorted(uses))
            print(f"missing U+{ord(char):04X} {char}: {refs}", file=sys.stderr)
        return 1

    print("generated Simplified Chinese font coverage OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
