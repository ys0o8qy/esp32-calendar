#!/usr/bin/env python3
"""Generate the LVGL 1bpp Fusion Pixel Font subset for the calendar UI."""

from __future__ import annotations

from pathlib import Path
import re
import subprocess


ROOT = Path(__file__).resolve().parents[1]
FONT_SOURCE = ROOT / "assets/fonts/fusion-pixel-10px-monospaced-zh_hans.ttf"
CALENDAR_HOME_SRC = ROOT / "application/edge_agent/components/calendar_home/src"
SOURCE_PATHS = [
    CALENDAR_HOME_SRC / "calendar_model.c",
    CALENDAR_HOME_SRC / "calendar_ui.c",
    CALENDAR_HOME_SRC / "calendar_board_data.c",
    CALENDAR_HOME_SRC / "calendar_home.c",
    ROOT / "sim/main_sdl.c",
]
OUTPUT_C = CALENDAR_HOME_SRC / "calendar_font_zh.c"
OUTPUT_H = CALENDAR_HOME_SRC / "calendar_font_zh.h"
FONT_SIZE = 10
FONT_BPP = 1
FONT_NAME = "calendar_font_zh_16"
LARGE_FONT_VARIANTS = [
    {
        "output": CALENDAR_HOME_SRC / "calendar_font_fusion_48.c",
        "size": 48,
        "name": "calendar_font_fusion_48",
        "symbols": "0123456789:",
    },
    {
        "output": CALENDAR_HOME_SRC / "calendar_font_fusion_28.c",
        "size": 28,
        "name": "calendar_font_fusion_28",
        "symbols": "0123456789-%°C",
    },
]
NUMERIC_FONT_VARIANTS = [
    {
        "output": CALENDAR_HOME_SRC / "calendar_font_digits_48.c",
        "name": "calendar_font_digits_48",
        "symbols": "0123456789:",
        "width": 22,
        "height": 38,
        "stroke": 6,
        "advance": 25,
        "line_height": 38,
        "base_line": 0,
        "char_widths": {":": 8},
        "char_advances": {":": 10},
    },
    {
        "output": CALENDAR_HOME_SRC / "calendar_font_digits_28.c",
        "name": "calendar_font_digits_28",
        "symbols": "0123456789-%°C",
        "width": 13,
        "height": 22,
        "stroke": 4,
        "advance": 14,
        "line_height": 22,
        "base_line": 0,
        "char_widths": {"°": 9},
        "char_advances": {"°": 9},
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


def draw_rect(glyph: list[list[int]], x: int, y: int, w: int, h: int) -> None:
    height = len(glyph)
    width = len(glyph[0]) if height else 0
    x0 = max(0, x)
    y0 = max(0, y)
    x1 = min(width, x + w)
    y1 = min(height, y + h)
    for row in range(y0, y1):
        for col in range(x0, x1):
            glyph[row][col] = 1


def draw_diagonal(glyph: list[list[int]], thickness: int) -> None:
    height = len(glyph)
    width = len(glyph[0]) if height else 0
    if height == 0 or width == 0:
        return
    for row in range(height):
        col = round(row * (width - 1) / max(1, height - 1))
        draw_rect(glyph, col - thickness // 2, row, thickness, thickness)


def draw_rising_diagonal(glyph: list[list[int]], thickness: int) -> None:
    height = len(glyph)
    width = len(glyph[0]) if height else 0
    if height == 0 or width == 0:
        return
    for row in range(height):
        col = round((height - 1 - row) * (width - 1) / max(1, height - 1))
        draw_rect(glyph, col - thickness // 2, row, thickness, thickness)


def render_digit_glyph(char: str, *, width: int, height: int, stroke: int) -> list[list[int]]:
    glyph = [[0 for _ in range(width)] for _ in range(height)]
    mid_y = height // 2 - stroke // 2
    lower_y = height // 2
    horizontal_x = max(0, stroke // 2)
    horizontal_w = max(1, width - stroke)
    upper_h = max(1, height // 2)
    lower_h = max(1, height - lower_y - stroke // 2)

    segments = {
        "A": (horizontal_x, 0, horizontal_w, stroke),
        "B": (width - stroke, stroke // 2, stroke, upper_h),
        "C": (width - stroke, lower_y, stroke, lower_h),
        "D": (horizontal_x, height - stroke, horizontal_w, stroke),
        "E": (0, lower_y, stroke, lower_h),
        "F": (0, stroke // 2, stroke, upper_h),
        "G": (horizontal_x, mid_y, horizontal_w, stroke),
    }
    segment_map = {
        "0": "ABCDEF",
        "1": "BC",
        "2": "ABGED",
        "3": "ABGCD",
        "4": "FBGC",
        "5": "AFGCD",
        "6": "AFGECD",
        "7": "ABC",
        "8": "ABCDEFG",
        "9": "ABFGCD",
        "-": "G",
        "C": "AFED",
    }

    if char in segment_map:
        for segment in segment_map[char]:
            draw_rect(glyph, *segments[segment])
    elif char == ":":
        dot = max(2, stroke)
        x = (width - dot) // 2
        draw_rect(glyph, x, height // 3 - dot // 2, dot, dot)
        draw_rect(glyph, x, height * 2 // 3 - dot // 2, dot, dot)
    elif char == "%":
        dot = max(3, stroke + 1)
        draw_rect(glyph, 0, 1, dot, dot)
        draw_rect(glyph, width - dot, height - dot - 2, dot, dot)
        draw_rising_diagonal(glyph, max(2, stroke // 2))
    elif char == "°":
        dot = min(width - 2, max(5, stroke + 2))
        x = (width - dot) // 2
        draw_rect(glyph, x, 2, dot, dot)
    else:
        draw_rect(glyph, 0, 0, width, height)

    return glyph


def pack_1bpp_bitmap(glyph: list[list[int]]) -> list[int]:
    packed: list[int] = []
    byte = 0
    bit_count = 0
    for row in glyph:
        for pixel in row:
            byte = (byte << 1) | (1 if pixel else 0)
            bit_count += 1
            if bit_count == 8:
                packed.append(byte)
                byte = 0
                bit_count = 0
    if bit_count:
        packed.append(byte << (8 - bit_count))
    return packed


def format_c_byte_array(values: list[int], *, indent: str = "    ") -> str:
    lines = []
    for index in range(0, len(values), 12):
        chunk = values[index : index + 12]
        lines.append(indent + ", ".join(f"0x{value:02x}" for value in chunk))
    return ",\n".join(lines)


def font_macro_name(font_name: str) -> str:
    return font_name.upper()


def glyph_width_for(variant: dict[str, object], char: str) -> int:
    char_widths = variant.get("char_widths", {})
    if isinstance(char_widths, dict) and char in char_widths:
        return int(char_widths[char])
    return int(variant["width"])


def glyph_advance_for(variant: dict[str, object], char: str) -> int:
    char_advances = variant.get("char_advances", {})
    if isinstance(char_advances, dict) and char in char_advances:
        return int(char_advances[char])
    return int(variant["advance"])


def write_numeric_font_file(variant: dict[str, object]) -> None:
    output = variant["output"]
    if not isinstance(output, Path):
        raise TypeError("numeric font output must be a Path")

    symbols = sorted(str(variant["symbols"]), key=ord)
    glyph_blocks: list[str] = []
    glyph_descriptions = [
        "    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */"
    ]
    glyph_bitmap: list[int] = []
    for char in symbols:
        glyph = render_digit_glyph(
            char,
            width=glyph_width_for(variant, char),
            height=int(variant["height"]),
            stroke=int(variant["stroke"]),
        )
        packed = pack_1bpp_bitmap(glyph)
        glyph_blocks.append(
            f'    /* U+{ord(char):04X} "{char}" */\n{format_c_byte_array(packed)}'
        )
        glyph_descriptions.append(
            "    "
            f"{{.bitmap_index = {len(glyph_bitmap)}, "
            f".adv_w = {glyph_advance_for(variant, char) * 16}, "
            f".box_w = {glyph_width_for(variant, char)}, "
            f".box_h = {int(variant['height'])}, .ofs_x = 0, .ofs_y = 0}}"
        )
        glyph_bitmap.extend(packed)

    range_start = ord(symbols[0])
    range_end = ord(symbols[-1])
    unicode_list = [ord(char) - range_start for char in symbols]
    font_name = str(variant["name"])
    macro_name = font_macro_name(font_name)
    glyph_bitmap_text = ",\n\n".join(glyph_blocks)
    glyph_dsc_text = ",\n".join(glyph_descriptions)
    unicode_list_text = format_c_byte_array(unicode_list)

    output.write_text(
        f"""/*******************************************************************************
 * Generated bold 1bpp numeric font for RLCD high-priority values.
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "calendar_font_zh.h"
#endif

#ifndef {macro_name}
#define {macro_name} 1
#endif

#if {macro_name}

/*-----------------
 *    BITMAPS
 *----------------*/

static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {{
{glyph_bitmap_text}
}};

/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {{
{glyph_dsc_text}
}};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {{
{unicode_list_text}
}};

static const lv_font_fmt_txt_cmap_t cmaps[] =
{{
    {{
        .range_start = {range_start}, .range_length = {range_end - range_start + 1}, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = {len(symbols)}, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }}
}};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
static lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {{
#else
static lv_font_fmt_txt_dsc_t font_dsc = {{
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
}};

/*-----------------
 *  PUBLIC FONT
 *----------------*/

#if LVGL_VERSION_MAJOR >= 8
const lv_font_t {font_name} = {{
#else
lv_font_t {font_name} = {{
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    .line_height = {int(variant["line_height"])},
    .base_line = {int(variant["base_line"])},
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 2,
#endif
    .dsc = &font_dsc,
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
}};

#endif /*#if {macro_name}*/
""",
        encoding="utf-8",
    )


def write_header() -> None:
    OUTPUT_H.write_text(
        f"""#pragma once

#include "lvgl.h"

LV_FONT_DECLARE({FONT_NAME});
LV_FONT_DECLARE(calendar_font_fusion_48);
LV_FONT_DECLARE(calendar_font_fusion_28);
LV_FONT_DECLARE(calendar_font_digits_48);
LV_FONT_DECLARE(calendar_font_digits_28);
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
    for variant in NUMERIC_FONT_VARIANTS:
        write_numeric_font_file(variant)


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
