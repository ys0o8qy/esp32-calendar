#!/usr/bin/env python3
from __future__ import annotations

import argparse
import binascii
import collections
import dataclasses
import struct
import sys
import zlib
from pathlib import Path
from typing import Callable, Iterable


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


@dataclasses.dataclass(frozen=True)
class AnalysisResult:
    ok: bool
    message: str
    width: int
    height: int
    non_background_fraction: float
    edge_fraction: float


def _paeth(a: int, b: int, c: int) -> int:
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def _iter_chunks(data: bytes) -> Iterable[tuple[bytes, bytes]]:
    offset = len(PNG_SIGNATURE)
    while offset + 12 <= len(data):
        length = struct.unpack(">I", data[offset : offset + 4])[0]
        kind = data[offset + 4 : offset + 8]
        payload = data[offset + 8 : offset + 8 + length]
        yield kind, payload
        offset += 12 + length
        if kind == b"IEND":
            return


def read_png_rgba(path: Path) -> tuple[int, int, list[tuple[int, int, int, int]]]:
    data = path.read_bytes()
    if not data.startswith(PNG_SIGNATURE):
        raise ValueError("not a PNG file")

    width = height = bit_depth = color_type = None
    idat_parts: list[bytes] = []
    for kind, payload in _iter_chunks(data):
        if kind == b"IHDR":
            width, height, bit_depth, color_type, compression, filter_method, interlace = struct.unpack(
                ">IIBBBBB", payload
            )
            if compression != 0 or filter_method != 0 or interlace != 0:
                raise ValueError("unsupported PNG compression, filter, or interlace mode")
        elif kind == b"IDAT":
            idat_parts.append(payload)

    if width is None or height is None or bit_depth is None or color_type is None:
        raise ValueError("PNG missing IHDR")
    if bit_depth != 8 or color_type not in (2, 6):
        raise ValueError("only 8-bit RGB/RGBA PNG files are supported")

    channels = 4 if color_type == 6 else 3
    bpp = channels
    stride = width * channels
    raw = zlib.decompress(b"".join(idat_parts))
    expected = (stride + 1) * height
    if len(raw) != expected:
        raise ValueError(f"unexpected PNG payload length: {len(raw)} != {expected}")

    rows: list[bytearray] = []
    pos = 0
    previous = bytearray(stride)
    for _y in range(height):
        filter_type = raw[pos]
        pos += 1
        row = bytearray(raw[pos : pos + stride])
        pos += stride

        if filter_type == 1:
            for i in range(stride):
                row[i] = (row[i] + (row[i - bpp] if i >= bpp else 0)) & 0xff
        elif filter_type == 2:
            for i in range(stride):
                row[i] = (row[i] + previous[i]) & 0xff
        elif filter_type == 3:
            for i in range(stride):
                left = row[i - bpp] if i >= bpp else 0
                up = previous[i]
                row[i] = (row[i] + ((left + up) >> 1)) & 0xff
        elif filter_type == 4:
            for i in range(stride):
                left = row[i - bpp] if i >= bpp else 0
                up = previous[i]
                up_left = previous[i - bpp] if i >= bpp else 0
                row[i] = (row[i] + _paeth(left, up, up_left)) & 0xff
        elif filter_type != 0:
            raise ValueError(f"unsupported PNG filter type: {filter_type}")

        rows.append(row)
        previous = row

    pixels: list[tuple[int, int, int, int]] = []
    for row in rows:
        for x in range(width):
            i = x * channels
            if channels == 4:
                pixels.append((row[i], row[i + 1], row[i + 2], row[i + 3]))
            else:
                pixels.append((row[i], row[i + 1], row[i + 2], 255))
    return width, height, pixels


def _luminance(pixel: tuple[int, int, int, int]) -> int:
    r, g, b, _a = pixel
    return int(0.299 * r + 0.587 * g + 0.114 * b)


def _non_background_fraction(pixels: list[tuple[int, int, int, int]]) -> float:
    colors = collections.Counter((r, g, b) for r, g, b, _a in pixels)
    return 1.0 - (colors.most_common(1)[0][1] / len(pixels))


def _region_non_background_fraction(
    pixels: list[tuple[int, int, int, int]],
    width: int,
    x1: int,
    y1: int,
    x2: int,
    y2: int,
) -> float:
    region = [
        pixels[y * width + x]
        for y in range(y1, y2)
        for x in range(x1, x2)
    ]
    return _non_background_fraction(region)


def _edge_fraction(pixels: list[tuple[int, int, int, int]], width: int, height: int) -> float:
    luminance = [_luminance(pixel) for pixel in pixels]
    edges = 0
    comparisons = 0
    for y in range(height):
        row = y * width
        for x in range(width - 1):
            comparisons += 1
            if abs(luminance[row + x] - luminance[row + x + 1]) > 20:
                edges += 1
    for y in range(height - 1):
        row = y * width
        next_row = row + width
        for x in range(width):
            comparisons += 1
            if abs(luminance[row + x] - luminance[next_row + x]) > 20:
                edges += 1
    return edges / comparisons


def analyze_png(path: Path, expected_width: int = 400, expected_height: int = 300) -> AnalysisResult:
    width, height, pixels = read_png_rgba(path)
    non_bg = _non_background_fraction(pixels)
    edges = _edge_fraction(pixels, width, height)

    if width != expected_width or height != expected_height:
        return AnalysisResult(False, f"unexpected size {width}x{height}", width, height, non_bg, edges)
    if non_bg < 0.003:
        return AnalysisResult(False, "blank or nearly blank render", width, height, non_bg, edges)
    if edges < 0.001:
        return AnalysisResult(False, "render has too little edge/detail structure", width, height, non_bg, edges)

    left_non_bg = _region_non_background_fraction(pixels, width, 0, 0, 185, height)
    right_non_bg = _region_non_background_fraction(pixels, width, 185, 0, width, height)
    if left_non_bg < 0.002 or right_non_bg < 0.002:
        return AnalysisResult(False, "calendar left/right regions look blank", width, height, non_bg, edges)

    return AnalysisResult(True, "render looks structurally valid", width, height, non_bg, edges)


def _png_chunk(kind: bytes, payload: bytes) -> bytes:
    crc = binascii.crc32(kind)
    crc = binascii.crc32(payload, crc) & 0xffffffff
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", crc)


def write_test_png(
    path: Path,
    width: int,
    height: int,
    pixel_fn: Callable[[int, int], tuple[int, int, int, int]],
) -> None:
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        for x in range(width):
            raw.extend(pixel_fn(x, y))

    png = (
        PNG_SIGNATURE
        + _png_chunk(b"IHDR", ihdr)
        + _png_chunk(b"IDAT", zlib.compress(bytes(raw)))
        + _png_chunk(b"IEND", b"")
    )
    path.write_bytes(png)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Check whether the calendar render PNG is structurally valid.")
    parser.add_argument("png", type=Path)
    parser.add_argument("--width", type=int, default=400)
    parser.add_argument("--height", type=int, default=300)
    args = parser.parse_args(argv)

    try:
        result = analyze_png(args.png, args.width, args.height)
    except Exception as exc:
        print(f"render check failed: {exc}", file=sys.stderr)
        return 1

    print(
        f"{result.message}: {result.width}x{result.height}, "
        f"non-bg={result.non_background_fraction:.4f}, edges={result.edge_fraction:.4f}"
    )
    return 0 if result.ok else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
