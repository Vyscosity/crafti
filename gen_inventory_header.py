import os
import sys

from PIL import Image


def rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def main() -> int:
    if len(sys.argv) != 4:
        print("Usage: python3 gen_inventory_header.py <input_png> <output_header> <symbol_name>")
        return 2

    input_png = sys.argv[1]
    output_header = sys.argv[2]
    symbol_name = sys.argv[3]

    img = Image.open(input_png).convert("RGBA")
    w, h = img.size
    pixels = list(img.getdata())

    has_alpha = any(a < 250 for (_, _, _, a) in pixels)
    values = [0 if a < 128 else rgb565(r, g, b) for (r, g, b, a) in pixels]

    src_name = os.path.basename(input_png)
    with open(output_header, "w", encoding="ascii") as out:
        out.write(f"//Generated from {src_name} (output format: ngl)\n")
        out.write(f"static uint16_t {symbol_name}_data[] = {{\n")
        for y in range(h):
            row = values[y * w : (y + 1) * w]
            out.write(", ".join(f"0x{v:x}" for v in row))
            out.write(", \n")
        out.write("};\n")
        out.write(f"static TEXTURE {symbol_name}{{\n")
        out.write(f".width = {w},\n")
        out.write(f".height = {h},\n")
        out.write(f".has_transparency = {'true' if has_alpha else 'false'},\n")
        out.write(".transparent_color = 0,\n")
        out.write(f".bitmap = {symbol_name}_data }};\n")

    print(f"generated {output_header} from {input_png} ({w}x{h})")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
