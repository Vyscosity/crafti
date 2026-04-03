#!/usr/bin/env python3
"""Emit textures/furnace.h from furnace.png (RGB565 for nGL).

Regenerate: python3 gen_furnace_h.py   (from textures/)
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
PNG_PATH = os.path.join(SCRIPT_DIR, "furnace.png")
OUT_PATH = os.path.join(SCRIPT_DIR, "furnace.h")


def rgb888_to_rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def main() -> None:
    path = PNG_PATH if len(sys.argv) < 2 else sys.argv[1]
    out = OUT_PATH if len(sys.argv) < 3 else sys.argv[2]
    if not os.path.isfile(path):
        sys.stderr.write(f"Missing {path}\n")
        sys.exit(1)

    im = Image.open(path).convert("RGBA")
    w, h = im.size

    pixels: list[int] = []
    for y in range(h):
        for x in range(w):
            r, g, b, a = im.getpixel((x, y))
            if a < 128:
                pixels.append(0x0000)
                continue
            if r <= 1 and g <= 1 and b <= 1:
                pixels.append(0x0841)
            else:
                pixels.append(rgb888_to_rgb565(r, g, b))

    lines = [
        f"// Generated from {os.path.basename(path)} ({w}x{h} vanilla furnace GUI)",
        "// Regenerate: python3 textures/gen_furnace_h.py",
        "static uint16_t furnace_gui_data[] = {",
    ]
    per_line = 12
    for i in range(0, len(pixels), per_line):
        chunk = pixels[i : i + per_line]
        sep = "," if i + per_line < len(pixels) else ""
        lines.append("    " + ", ".join(f"0x{v:04x}" for v in chunk) + sep)
    lines.append("};")
    lines.append("")
    lines.append("static TEXTURE furnace_gui{")
    lines.append(f"    {w}, {h},")
    lines.append("    true, 0,")
    lines.append("    furnace_gui_data")
    lines.append("};")
    lines.append("")

    with open(out, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print(f"Wrote {out} ({w}x{h}, {len(pixels)} pixels)", file=sys.stderr)


if __name__ == "__main__":
    main()
