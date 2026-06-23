#!/usr/bin/env python3
"""Generate a small LVGL 1bpp Simplified Chinese font for the calendar UI."""

from __future__ import annotations

from pathlib import Path
import re
import subprocess


ROOT = Path(__file__).resolve().parents[1]
FONT_SOURCE = ROOT / "third_party/lvgl/scripts/built_in_font/SimSun.woff"
SOURCE_PATHS = [
    ROOT / "src/app/calendar_model.c",
    ROOT / "src/app/calendar_ui.c",
]
OUTPUT_C = ROOT / "src/app/calendar_font_zh.c"
OUTPUT_H = ROOT / "src/app/calendar_font_zh.h"
FONT_SIZE = 16
FONT_BPP = 1
FONT_NAME = "calendar_font_zh_16"
FALLBACK_FONT_NAME = "lv_font_montserrat_16"
REQUIRED_ASCII_CHARS = (
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    " :%-./+"
)


def extract_chars() -> list[str]:
    chars: set[str] = set(REQUIRED_ASCII_CHARS)
    for path in SOURCE_PATHS:
        source = path.read_text(encoding="utf-8")
        for literal in re.findall(r'"((?:[^"\\]|\\.)*)"', source):
            for char in literal:
                if ord(char) > 127 or (char in REQUIRED_ASCII_CHARS):
                    chars.add(char)
    return sorted(chars, key=ord)


def repo_relative(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def build_lv_font_conv_command(chars: list[str]) -> list[str]:
    symbols = "".join(chars)
    return [
        "lv_font_conv",
        "--no-compress",
        "--no-prefilter",
        "--bpp",
        str(FONT_BPP),
        "--size",
        str(FONT_SIZE),
        "--font",
        repo_relative(FONT_SOURCE),
        "--symbols",
        symbols,
        "--format",
        "lvgl",
        "-o",
        repo_relative(OUTPUT_C),
        "--lv-include",
        "calendar_font_zh.h",
        "--lv-font-name",
        FONT_NAME,
        "--lv-fallback",
        FALLBACK_FONT_NAME,
        "--force-fast-kern-format",
    ]


def write_header() -> None:
    OUTPUT_H.write_text(
        f"""#pragma once

#include "lvgl.h"

LV_FONT_DECLARE({FONT_NAME});
""",
        encoding="utf-8",
    )


def generate_font(chars: list[str]) -> None:
    write_header()
    subprocess.run(build_lv_font_conv_command(chars), check=True, cwd=ROOT)


def main() -> int:
    if not FONT_SOURCE.exists():
        raise RuntimeError(f"Simplified Chinese font not found: {FONT_SOURCE}")
    chars = extract_chars()
    generate_font(chars)
    print(f"generated {OUTPUT_C.relative_to(ROOT)} with {len(chars)} glyphs from {FONT_SOURCE}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
