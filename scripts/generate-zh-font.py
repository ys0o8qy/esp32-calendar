#!/usr/bin/env python3
"""Generate the LVGL 1bpp Fusion Pixel Font subset for the calendar UI."""

from __future__ import annotations

from pathlib import Path
import re
import subprocess


ROOT = Path(__file__).resolve().parents[1]
FONT_SOURCE = ROOT / "assets/fonts/fusion-pixel-10px-monospaced-zh_hans.ttf"
SOURCE_PATHS = [
    ROOT / "src/app/calendar_model.c",
    ROOT / "src/app/calendar_ui.c",
    ROOT / "src/platform/esp32/calendar_platform.c",
]
OUTPUT_C = ROOT / "src/app/calendar_font_zh.c"
OUTPUT_H = ROOT / "src/app/calendar_font_zh.h"
FONT_SIZE = 10
FONT_BPP = 1
FONT_NAME = "calendar_font_zh_16"
LARGE_FONT_VARIANTS = [
    {
        "output": ROOT / "src/app/calendar_font_fusion_48.c",
        "size": 48,
        "name": "calendar_font_fusion_48",
        "symbols": "0123456789:",
    },
    {
        "output": ROOT / "src/app/calendar_font_fusion_28.c",
        "size": 28,
        "name": "calendar_font_fusion_28",
        "symbols": "0123456789-°C",
    },
]
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


def build_lv_font_conv_command(
    chars: list[str],
    *,
    output: Path = OUTPUT_C,
    size: int = FONT_SIZE,
    font_name: str = FONT_NAME,
) -> list[str]:
    symbols = "".join(chars)
    return [
        "lv_font_conv",
        "--no-compress",
        "--no-prefilter",
        "--bpp",
        str(FONT_BPP),
        "--size",
        str(size),
        "--font",
        repo_relative(FONT_SOURCE),
        "--symbols",
        symbols,
        "--format",
        "lvgl",
        "-o",
        repo_relative(output),
        "--lv-include",
        OUTPUT_H.name,
        "--lv-font-name",
        font_name,
        "--force-fast-kern-format",
    ]


def write_header() -> None:
    OUTPUT_H.write_text(
        f"""#pragma once

#include "lvgl.h"

LV_FONT_DECLARE({FONT_NAME});
LV_FONT_DECLARE(calendar_font_fusion_48);
LV_FONT_DECLARE(calendar_font_fusion_28);
""",
        encoding="utf-8",
    )


def generate_font(chars: list[str]) -> None:
    write_header()
    subprocess.run(build_lv_font_conv_command(chars), check=True, cwd=ROOT)
    for variant in LARGE_FONT_VARIANTS:
        subprocess.run(
            build_lv_font_conv_command(
                list(variant["symbols"]),
                output=variant["output"],
                size=variant["size"],
                font_name=variant["name"],
            ),
            check=True,
            cwd=ROOT,
        )


def main() -> int:
    if not FONT_SOURCE.exists():
        raise RuntimeError(f"font not found: {FONT_SOURCE}")
    chars = extract_chars()
    generate_font(chars)
    print(
        f"generated {OUTPUT_C.relative_to(ROOT)} with {len(chars)} glyphs "
        f"from {FONT_SOURCE}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
