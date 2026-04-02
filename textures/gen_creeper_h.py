#!/usr/bin/env python3
"""Emit textures/creeper.h from creeper.png (64x32 RGB565 for nGL).

Used when ConvertImg is not installed. Run from textures/: python3 gen_creeper_h.py
"""
from __future__ import annotations

import os
import sys

try:
    from PIL import Image
except ImportError:
    sys.stderr.write("Install Pillow: pip install pillow\n")
    sys.exit(1)

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PNG_PATH = os.path.join(SCRIPT_DIR, "creeper.png")
OUT_PATH = os.path.join(SCRIPT_DIR, "creeper.h")


def rgb888_to_rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def main() -> None:
    if len(sys.argv) >= 2:
        path = sys.argv[1]
    else:
        path = PNG_PATH
    if not os.path.isfile(path):
        sys.stderr.write(f"Missing {path}\n")
        sys.exit(1)

    im = Image.open(path).convert("RGBA")
    w, h = im.size
    if w != 64 or h != 32:
        sys.stderr.write(f"Expected 64x32 creeper skin, got {w}x{h}\n")
        sys.exit(1)

    pixels: list[int] = []
    for y in range(h):
        for x in range(w):
            r, g, b, a = im.getpixel((x, y))
            if a < 128:
                pixels.append(0x0000)
                continue
            # Match chicken.h / nGL: map opaque black to avoid accidental transparency
            if r <= 1 and g <= 1 and b <= 1:
                pixels.append(0x0841)
            else:
                pixels.append(rgb888_to_rgb565(r, g, b))

    out = OUT_PATH if len(sys.argv) < 3 else sys.argv[2]
    lines = [
        "// Generated from textures/creeper.png (64x32 mob skin; UVs match ModelCreeper)",
        "// Regenerate: make -C textures creeper.h   OR   python3 gen_creeper_h.py",
        "static uint16_t creeper_data[] = {",
    ]
    per_line = 12
    for i in range(0, len(pixels), per_line):
        chunk = pixels[i : i + per_line]
        sep = "," if i + per_line < len(pixels) else ""
        lines.append("    " + ", ".join(f"0x{v:04x}" for v in chunk) + sep)
    lines.append("};")
    lines.append("")
    lines.append("static TEXTURE creeper_tex = {")
    lines.append("    64, 32,")
    lines.append("    true, 0,")
    lines.append("    creeper_data")
    lines.append("};")
    lines.append("")

    with open(out, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print(f"Wrote {out} ({len(pixels)} pixels)", file=sys.stderr)


if __name__ == "__main__":
    main()
