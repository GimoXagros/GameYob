#!/usr/bin/env python3
"""Convert the supplied 32px RGBA logo to the DS banner's 16-index BMP.

DS banners have one binary transparent palette index, so partially transparent
source pixels cannot be represented exactly. Alpha < 128 becomes transparent;
all other pixels are composited onto black before 15-color quantization.
"""

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "platform/ds/icon.png"
OUTPUT = ROOT / "platform/ds/icon.bmp"
BANNER_OUTPUT = ROOT / "platform/ds/icon_banner.png"


def main():
    source = Image.open(SOURCE).convert("RGBA")
    if source.size != (32, 32):
        raise ValueError("DS banner source must be exactly 32x32")

    rgba = list(source.getdata())
    composite = Image.new("RGB", source.size)
    composite.putdata([
        tuple((channel * alpha + 127) // 255 for channel in (red, green, blue))
        if alpha >= 128 else (0, 0, 0)
        for red, green, blue, alpha in rgba
    ])
    quantized = composite.quantize(colors=15, method=Image.Quantize.MEDIANCUT,
                                   dither=Image.Dither.NONE)
    palette = quantized.getpalette()[:45]
    indexed = Image.new("P", source.size)
    # ndstool reserves banner palette index 0 for magenta transparency.
    indexed.putpalette([255, 0, 255] + palette + [0] * (768 - 3 - len(palette)))
    indexed.putdata([
        0 if pixel[3] < 128 else color + 1
        for pixel, color in zip(rgba, quantized.getdata())
    ])
    indexed.save(OUTPUT, format="BMP")
    colors = indexed.getpalette()
    banner = Image.new("RGBA", source.size)
    banner.putdata([
        (255, 0, 255, 0) if index == 0 else
        (colors[index * 3], colors[index * 3 + 1], colors[index * 3 + 2], 255)
        for index in indexed.getdata()
    ])
    banner.save(BANNER_OUTPUT, format="PNG")
    print(f"Wrote {OUTPUT} from {SOURCE} (alpha threshold 128, 15 visible colors)")
    print(f"Wrote {BANNER_OUTPUT} as 16-color RGBA PNG for ndstool")


if __name__ == "__main__":
    main()
