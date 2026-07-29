#!/usr/bin/env python3
"""Render a small visual smoke test from the generated 8x8 Hangul asset."""

from pathlib import Path
import argparse

from PIL import Image, ImageDraw


def glyph_index(codepoint: int) -> int | None:
    if 0x1100 <= codepoint <= 0x11FF:
        return codepoint - 0x1100
    if 0x3130 <= codepoint <= 0x318F:
        return 0x100 + codepoint - 0x3130
    if 0xAC00 <= codepoint <= 0xD7A3:
        return 0x160 + codepoint - 0xAC00
    return None


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("font", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    phrases = ("한글 파일명 테스트", "치트 목록 표시 확인", "가나다라마바사아자차카타파하")
    scale = 4
    image = Image.new("RGB", (32 * 8 * scale, len(phrases) * 10 * scale), "black")
    draw = ImageDraw.Draw(image)
    font = args.font.read_bytes()

    for line, phrase in enumerate(phrases):
        for column, character in enumerate(phrase):
            index = glyph_index(ord(character))
            if index is None:
                continue
            bitmap = font[index * 8:index * 8 + 8]
            for y, row in enumerate(bitmap):
                for x in range(8):
                    if row & (1 << (7 - x)):
                        left = (column * 8 + x) * scale
                        top = (line * 10 + y) * scale
                        draw.rectangle(
                            (left, top, left + scale - 1, top + scale - 1),
                            fill="white",
                        )

    args.output.parent.mkdir(parents=True, exist_ok=True)
    image.save(args.output)


if __name__ == "__main__":
    main()
