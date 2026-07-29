#!/usr/bin/env python3
"""Generate GameYob's compact Hangul font and CP949 lookup assets."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


RANGES = (
    (0x1100, 0x11FF),  # Hangul Jamo
    (0x3130, 0x318F),  # Hangul Compatibility Jamo
    (0xAC00, 0xD7A3),  # Precomposed Hangul syllables
)

MODERN_JAMO_TO_COMPATIBILITY = {
    **dict(zip(range(0x1100, 0x1113),
               (0x3131, 0x3132, 0x3134, 0x3137, 0x3138, 0x3139, 0x3141,
                0x3142, 0x3143, 0x3145, 0x3146, 0x3147, 0x3148, 0x3149,
                0x314A, 0x314B, 0x314C, 0x314D, 0x314E))),
    **dict(zip(range(0x1161, 0x1176), range(0x314F, 0x3164))),
    **dict(zip(range(0x11A8, 0x11C3),
               (0x3131, 0x3132, 0x3133, 0x3134, 0x3135, 0x3136, 0x3137,
                0x3139, 0x313A, 0x313B, 0x313C, 0x313D, 0x313E, 0x313F,
                0x3140, 0x3141, 0x3142, 0x3144, 0x3145, 0x3146, 0x3147,
                0x3148, 0x314A, 0x314B, 0x314C, 0x314D, 0x314E))),
}


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
                if encoding is not None and encoding >= 0 and bitmap is not None:
                    glyphs[encoding] = (width, height, bitmap)
                encoding = None
                bitmap = None
            elif bitmap is not None:
                bitmap.append(int(line, 16))

    return glyphs


def source_pixel(row: int, width: int, x: int) -> bool:
    stored_bits = ((width + 7) // 8) * 8
    return bool(row & (1 << (stored_bits - 1 - x)))


def to_8x8(glyph: tuple[int, int, list[int]]) -> bytes:
    width, height, rows = glyph

    def groups_for_axis(size: int) -> list[list[int]]:
        if size <= 8:
            return [[x] for x in range(size)] + [[] for _ in range(8 - size)]
        if size == 9:
            # Preserve every source pixel by merging the middle two positions.
            return [[0], [1], [2], [3], [4, 5], [6], [7], [8]]
        raise ValueError(f"unsupported glyph axis: {size}")

    x_groups = groups_for_axis(width)
    y_groups = groups_for_axis(height)

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
    output = bytearray()
    historical_jamo_fallbacks: list[int] = []
    fallback_glyph = seven.get(ord("?")) or nine.get(ord("?"))
    if fallback_glyph is None:
        raise RuntimeError("font lacks the required '?' fallback glyph")

    for first, last in RANGES:
        for codepoint in range(first, last + 1):
            glyph = seven.get(codepoint) or nine.get(codepoint)
            if glyph is None:
                compatibility = MODERN_JAMO_TO_COMPATIBILITY.get(codepoint)
                if compatibility is not None:
                    glyph = seven.get(compatibility) or nine.get(compatibility)
                else:
                    # Galmuri covers modern Korean but intentionally omits some
                    # historical/extended Jamo. Keep the table total and visible.
                    historical_jamo_fallbacks.append(codepoint)
                    glyph = fallback_glyph
            output.extend(to_8x8(glyph))

    if historical_jamo_fallbacks:
        print(
            "historical Jamo rendered as '?': "
            f"{len(historical_jamo_fallbacks)} codepoints"
        )
    return bytes(output)


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
    (args.output_dir / "hangul_font.bin").write_bytes(font)
    (args.output_dir / "cp949_table.bin").write_bytes(cp949)
    print(f"hangul_font.bin: {len(font)} bytes")
    print(f"cp949_table.bin: {len(cp949)} bytes")


if __name__ == "__main__":
    main()
