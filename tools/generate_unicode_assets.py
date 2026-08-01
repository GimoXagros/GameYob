#!/usr/bin/env python3
"""Generate GameYob's compact Unicode bitmap font and CP949 table.

The font file contains sorted fixed-size records:
  4 bytes magic (GYUF), 2 bytes version, 2 bytes reserved,
  4 bytes record count, then records of u16 codepoint + 8 bitmap rows.
"""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


MAGIC = b"GYUF"
VERSION = 1


def read_bdf(path: Path) -> dict[int, tuple[int, int, list[int]]]:
    glyphs: dict[int, tuple[int, int, list[int]]] = {}
    encoding: int | None = None
    width = height = 0
    bitmap: list[int] | None = None

    with path.open("r", encoding="utf-8", errors="strict") as source:
        for raw_line in source:
            line = raw_line.rstrip("\r\n")
            if line.startswith("ENCODING "):
                encoding = int(line.split()[1])
            elif line.startswith("BBX "):
                parts = line.split()
                width, height = int(parts[1]), int(parts[2])
            elif line == "BITMAP":
                bitmap = []
            elif line == "ENDCHAR":
                if (
                    encoding is not None
                    and 0 <= encoding <= 0xFFFF
                    and bitmap is not None
                ):
                    glyphs[encoding] = (width, height, bitmap)
                encoding = None
                bitmap = None
            elif bitmap is not None:
                bitmap.append(int(line, 16))
    return glyphs


def source_pixel(row: int, width: int, x: int) -> bool:
    stored_bits = ((width + 7) // 8) * 8
    return bool(row & (1 << (stored_bits - 1 - x)))


def axis_groups(size: int) -> list[list[int]]:
    if size <= 8:
        return [[x] for x in range(size)] + [[] for _ in range(8 - size)]
    # Preserve the outside shape and merge evenly spaced source pixels.
    return [
        list(range((out * size) // 8, ((out + 1) * size) // 8))
        for out in range(8)
    ]


def to_8x8(glyph: tuple[int, int, list[int]]) -> bytes:
    width, height, rows = glyph
    if width <= 0 or height <= 0 or len(rows) < height:
        raise ValueError("invalid BDF glyph dimensions")

    x_groups = axis_groups(width)
    y_groups = axis_groups(height)
    result = bytearray(8)
    for out_y, source_ys in enumerate(y_groups):
        row_value = 0
        for out_x, source_xs in enumerate(x_groups):
            if any(
                source_pixel(rows[source_y], width, source_x)
                for source_y in source_ys
                for source_x in source_xs
            ):
                row_value |= 1 << (7 - out_x)
        result[out_y] = row_value
    return bytes(result)


def generate_font(galmuri7: Path, galmuri9: Path) -> bytes:
    seven = read_bdf(galmuri7)
    nine = read_bdf(galmuri9)
    merged = dict(nine)
    merged.update(seven)

    records = []
    for codepoint in sorted(merged):
        if codepoint < 0x80 or codepoint in (0xFEFF,):
            continue
        glyph = merged[codepoint]
        if glyph[0] <= 0 or glyph[1] <= 0 or len(glyph[2]) < glyph[1]:
            continue
        records.append(struct.pack("<H", codepoint) + to_8x8(glyph))

    header = MAGIC + struct.pack("<HHI", VERSION, 0, len(records))
    return header + b"".join(records)


def generate_cp949_table() -> bytes:
    output = bytearray()
    for lead in range(0x81, 0xFF):
        for trail in range(0x100):
            try:
                text = bytes((lead, trail)).decode("cp949")
            except UnicodeDecodeError:
                codepoint = 0
            else:
                codepoint = ord(text) if len(text) == 1 and ord(text) <= 0xFFFF else 0
            output.extend(struct.pack("<H", codepoint))
    return bytes(output)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--galmuri-dir", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()

    args.output_dir.mkdir(parents=True, exist_ok=True)
    font = generate_font(
        args.galmuri_dir / "Galmuri7.bdf",
        args.galmuri_dir / "Galmuri9.bdf",
    )
    cp949 = generate_cp949_table()
    (args.output_dir / "unicode_font.bin").write_bytes(font)
    (args.output_dir / "cp949_table.bin").write_bytes(cp949)
    print(f"unicode_font.bin: {len(font)} bytes")
    print(f"unicode glyphs: {(len(font) - 12) // 10}")
    print(f"cp949_table.bin: {len(cp949)} bytes")


if __name__ == "__main__":
    main()
