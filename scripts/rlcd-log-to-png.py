#!/usr/bin/env python3
from __future__ import annotations

import argparse
import binascii
import re
import struct
import sys
import zlib
from pathlib import Path


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
BEGIN_RE = re.compile(r"CALENDAR_RLCD_FRAME_BEGIN .*width=(\d+) height=(\d+) bytes=(\d+)")
HEX_RE = re.compile(r"CALENDAR_RLCD_FRAME_HEX offset=(\d+) data=([0-9a-fA-F]+)")


def _png_chunk(kind: bytes, payload: bytes) -> bytes:
    crc = binascii.crc32(kind)
    crc = binascii.crc32(payload, crc) & 0xffffffff
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", crc)


def _write_rgba_png(path: Path, width: int, height: int, pixels: list[tuple[int, int, int, int]]) -> None:
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        for x in range(width):
            raw.extend(pixels[y * width + x])

    path.write_bytes(
        PNG_SIGNATURE
        + _png_chunk(b"IHDR", ihdr)
        + _png_chunk(b"IDAT", zlib.compress(bytes(raw)))
        + _png_chunk(b"IEND", b"")
    )


def _extract_frame(log_path: Path, width: int | None, height: int | None) -> tuple[int, int, bytes]:
    log = log_path.read_text(errors="replace")
    expected_len = None
    begin = BEGIN_RE.search(log)
    if begin:
        log_width, log_height, log_len = (int(value) for value in begin.groups())
        width = width if width is not None else log_width
        height = height if height is not None else log_height
        expected_len = log_len

    if width is None or height is None:
        raise ValueError("width and height are required when the log has no frame header")

    chunks: dict[int, bytes] = {}
    for match in HEX_RE.finditer(log):
        chunks[int(match.group(1))] = bytes.fromhex(match.group(2))
    if not chunks:
        raise ValueError("no CALENDAR_RLCD_FRAME_HEX records found")

    if expected_len is None:
        expected_len = (width * height) // 8
    frame = bytearray([0] * expected_len)
    for offset, data in sorted(chunks.items()):
        end = offset + len(data)
        if end > expected_len:
            raise ValueError(f"frame chunk at offset {offset} exceeds expected length {expected_len}")
        frame[offset:end] = data
    return width, height, bytes(frame)


def _is_white(frame: bytes, width: int, height: int, x: int, y: int) -> bool:
    inv_y = height - 1 - y
    byte_x = x >> 1
    block_y = inv_y >> 2
    blocks_per_column = height >> 2
    index = byte_x * blocks_per_column + block_y
    local_x = x & 1
    local_y = inv_y & 3
    bit = 7 - ((local_y << 1) | local_x)
    return (frame[index] & (1 << bit)) != 0


def convert_log_to_png(log_path: Path, png_path: Path, width: int | None = None, height: int | None = None) -> None:
    width, height, frame = _extract_frame(log_path, width, height)
    expected_len = (width * height) // 8
    if len(frame) != expected_len:
        raise ValueError(f"unexpected frame length {len(frame)} for {width}x{height}; expected {expected_len}")

    pixels = [
        (255, 255, 255, 255) if _is_white(frame, width, height, x, y) else (0, 0, 0, 255)
        for y in range(height)
        for x in range(width)
    ]
    _write_rgba_png(png_path, width, height, pixels)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Convert ESP32 calendar RLCD frame hex logs to a PNG.")
    parser.add_argument("log", type=Path)
    parser.add_argument("png", type=Path)
    parser.add_argument("--width", type=int)
    parser.add_argument("--height", type=int)
    args = parser.parse_args(argv)

    try:
        convert_log_to_png(args.log, args.png, args.width, args.height)
    except Exception as exc:
        print(f"RLCD log conversion failed: {exc}", file=sys.stderr)
        return 1

    print(f"Wrote RLCD frame PNG: {args.png}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
