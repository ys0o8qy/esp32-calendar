#!/usr/bin/env python3
"""Generate a small LVGL 1bpp Simplified Chinese font for the calendar UI."""

from __future__ import annotations

from pathlib import Path
import re
import struct
import subprocess
import tempfile
import zlib


ROOT = Path(__file__).resolve().parents[1]
FONT_SOURCE = Path("/System/Library/Fonts/Hiragino Sans GB.ttc")
SOURCE_PATHS = [
    ROOT / "src/app/calendar_model.c",
    ROOT / "src/app/calendar_ui.c",
]
OUTPUT_C = ROOT / "src/app/calendar_font_zh.c"
OUTPUT_H = ROOT / "src/app/calendar_font_zh.h"
FONT_SIZE = 18
FONT_LINE_HEIGHT = 22
FONT_BASELINE = 0


def extract_chars() -> list[str]:
    chars: set[str] = set()
    for path in SOURCE_PATHS:
        source = path.read_text(encoding="utf-8")
        for literal in re.findall(r'"((?:[^"\\]|\\.)*)"', source):
            for char in literal:
                if ord(char) > 127:
                    chars.add(char)
    return sorted(chars, key=ord)


def parse_png_grayscale(path: Path) -> tuple[int, int, list[int]]:
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise RuntimeError(f"not a PNG: {path}")

    pos = 8
    width = height = bit_depth = color_type = None
    idat = bytearray()
    while pos < len(data):
        length = struct.unpack(">I", data[pos:pos + 4])[0]
        pos += 4
        chunk_type = data[pos:pos + 4]
        pos += 4
        chunk_data = data[pos:pos + length]
        pos += length + 4
        if chunk_type == b"IHDR":
            width, height, bit_depth, color_type, _, _, interlace = struct.unpack(">IIBBBBB", chunk_data)
            if bit_depth != 8 or color_type != 0 or interlace != 0:
                raise RuntimeError(f"unsupported PNG format: bit={bit_depth} color={color_type} interlace={interlace}")
        elif chunk_type == b"IDAT":
            idat.extend(chunk_data)
        elif chunk_type == b"IEND":
            break

    assert width is not None and height is not None
    raw = zlib.decompress(bytes(idat))
    stride = width
    rows: list[list[int]] = []
    offset = 0
    previous = [0] * stride

    def paeth(a: int, b: int, c: int) -> int:
        p = a + b - c
        pa = abs(p - a)
        pb = abs(p - b)
        pc = abs(p - c)
        if pa <= pb and pa <= pc:
            return a
        if pb <= pc:
            return b
        return c

    for _ in range(height):
        filter_type = raw[offset]
        offset += 1
        row = list(raw[offset:offset + stride])
        offset += stride
        out = [0] * stride
        for x, value in enumerate(row):
            left = out[x - 1] if x else 0
            up = previous[x]
            up_left = previous[x - 1] if x else 0
            if filter_type == 0:
                out[x] = value
            elif filter_type == 1:
                out[x] = (value + left) & 0xff
            elif filter_type == 2:
                out[x] = (value + up) & 0xff
            elif filter_type == 3:
                out[x] = (value + ((left + up) >> 1)) & 0xff
            elif filter_type == 4:
                out[x] = (value + paeth(left, up, up_left)) & 0xff
            else:
                raise RuntimeError(f"unsupported PNG filter: {filter_type}")
        rows.append(out)
        previous = out

    pixels = [value for row in rows for value in row]
    return width, height, pixels


def render_char(char: str, tmpdir: Path) -> tuple[int, int, list[int]]:
    output = tmpdir / f"u{ord(char):04x}.png"
    subprocess.run(
        [
            "hb-view",
            str(FONT_SOURCE),
            char,
            f"--font-size={FONT_SIZE}",
            "--font-bold=0.035",
            "--margin=0",
            "--background=ffffffff",
            "--foreground=000000ff",
            "-O",
            "png",
            "-o",
            str(output),
        ],
        check=True,
    )
    width, height, gray = parse_png_grayscale(output)
    bitmap = [1 if value < 248 else 0 for value in gray]
    return crop_and_embolden(width, height, bitmap)


def crop_and_embolden(width: int, height: int, bitmap: list[int]) -> tuple[int, int, list[int]]:
    points = [(x, y) for y in range(height) for x in range(width) if bitmap[y * width + x]]
    if not points:
        return 0, 0, []
    min_x = max(min(x for x, _ in points) - 1, 0)
    max_x = min(max(x for x, _ in points) + 1, width - 1)
    min_y = max(min(y for _, y in points) - 1, 0)
    max_y = min(max(y for _, y in points) + 1, height - 1)
    cropped_w = max_x - min_x + 1
    cropped_h = max_y - min_y + 1
    cropped = [0] * (cropped_w * cropped_h)
    for y in range(min_y, max_y + 1):
        for x in range(min_x, max_x + 1):
            if bitmap[y * width + x]:
                cx = x - min_x
                cy = y - min_y
                cropped[cy * cropped_w + cx] = 1
                if cx + 1 < cropped_w:
                    cropped[cy * cropped_w + cx + 1] = 1
    return cropped_w, cropped_h, cropped


def pack_bitmap(width: int, height: int, bitmap: list[int]) -> list[int]:
    out: list[int] = []
    byte = 0
    used = 0
    for value in bitmap:
        byte = (byte << 1) | (1 if value else 0)
        used += 1
        if used == 8:
            out.append(byte)
            byte = 0
            used = 0
    if used:
        out.append(byte << (8 - used))
    return out


def c_array(values: list[int], indent: str = "    ") -> str:
    lines = []
    for i in range(0, len(values), 12):
        chunk = ", ".join(f"0x{value:02x}" for value in values[i:i + 12])
        lines.append(f"{indent}{chunk},")
    return "\n".join(lines)


def write_outputs(chars: list[str], glyphs: list[dict[str, object]]) -> None:
    bitmap: list[int] = []
    records = []
    for char, glyph in zip(chars, glyphs):
        packed = pack_bitmap(glyph["width"], glyph["height"], glyph["bitmap"])  # type: ignore[arg-type]
        records.append(
            {
                "codepoint": ord(char),
                "bitmap_index": len(bitmap),
                "width": glyph["width"],
                "height": glyph["height"],
                "advance": max(int(glyph["width"]) + 1, FONT_SIZE),
            }
        )
        bitmap.extend(packed)

    OUTPUT_H.write_text(
        """#pragma once

#include "lvgl.h"

LV_FONT_DECLARE(calendar_font_zh_18);
""",
        encoding="utf-8",
    )

    record_lines = "\n".join(
        "    {0x%04x, %d, %d, %d, %d}," % (
            record["codepoint"],
            record["bitmap_index"],
            record["width"],
            record["height"],
            record["advance"],
        )
        for record in records
    )

    OUTPUT_C.write_text(
        f"""#include "calendar_font_zh.h"

#include <stddef.h>

typedef struct {{
    uint32_t codepoint;
    uint32_t bitmap_index;
    uint8_t box_w;
    uint8_t box_h;
    uint8_t adv_w;
}} calendar_font_glyph_t;

static const uint8_t calendar_font_zh_bitmap[] = {{
{c_array(bitmap)}
}};

static const calendar_font_glyph_t calendar_font_zh_glyphs[] = {{
{record_lines}
}};

static const calendar_font_glyph_t *find_glyph(uint32_t letter)
{{
    size_t left = 0;
    size_t right = sizeof(calendar_font_zh_glyphs) / sizeof(calendar_font_zh_glyphs[0]);

    while (left < right) {{
        size_t mid = left + ((right - left) / 2);
        if (calendar_font_zh_glyphs[mid].codepoint == letter) {{
            return &calendar_font_zh_glyphs[mid];
        }}
        if (calendar_font_zh_glyphs[mid].codepoint < letter) {{
            left = mid + 1;
        }} else {{
            right = mid;
        }}
    }}

    return NULL;
}}

static bool calendar_font_zh_get_glyph_dsc(
    const lv_font_t *font,
    lv_font_glyph_dsc_t *dsc,
    uint32_t letter,
    uint32_t letter_next)
{{
    (void)letter_next;
    const calendar_font_glyph_t *glyph = find_glyph(letter);
    if (glyph == NULL) {{
        return false;
    }}

    dsc->resolved_font = font;
    dsc->adv_w = glyph->adv_w;
    dsc->box_w = glyph->box_w;
    dsc->box_h = glyph->box_h;
    dsc->ofs_x = 0;
    dsc->ofs_y = 0;
    dsc->bpp = 1;
    dsc->is_placeholder = 0;
    return true;
}}

static const uint8_t *calendar_font_zh_get_glyph_bitmap(const lv_font_t *font, uint32_t letter)
{{
    (void)font;
    const calendar_font_glyph_t *glyph = find_glyph(letter);
    if (glyph == NULL) {{
        return NULL;
    }}
    return &calendar_font_zh_bitmap[glyph->bitmap_index];
}}

const lv_font_t calendar_font_zh_18 = {{
    .get_glyph_dsc = calendar_font_zh_get_glyph_dsc,
    .get_glyph_bitmap = calendar_font_zh_get_glyph_bitmap,
    .line_height = {FONT_LINE_HEIGHT},
    .base_line = {FONT_BASELINE},
    .subpx = LV_FONT_SUBPX_NONE,
    .underline_position = -2,
    .underline_thickness = 1,
    .dsc = NULL,
    .fallback = &lv_font_montserrat_20,
}};
""",
        encoding="utf-8",
    )


def main() -> int:
    if not FONT_SOURCE.exists():
        raise RuntimeError(f"Simplified Chinese font not found: {FONT_SOURCE}")
    chars = extract_chars()
    with tempfile.TemporaryDirectory(prefix="calendar-zh-font-") as tmp:
        tmpdir = Path(tmp)
        glyphs = []
        for char in chars:
            width, height, bitmap = render_char(char, tmpdir)
            glyphs.append({"width": width, "height": height, "bitmap": bitmap})
    write_outputs(chars, glyphs)
    print(f"generated {OUTPUT_C.relative_to(ROOT)} with {len(chars)} glyphs from {FONT_SOURCE}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
