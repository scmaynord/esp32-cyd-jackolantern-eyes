#!/usr/bin/env python3
"""
img2cyd.py - turn drawings into an Image the CYD can draw (see Anim.h).

WHAT IT DOES
  Takes one or more pictures (PNG, JPG, or an animated GIF) and writes a .h
  file. Put that .h file in the sketch folder, #include it, and draw it with
      drawImageCentered(layer->canvas, x, y, myImage, frameNumber);

  Several pictures become several FRAMES of one Image, like a flip book.
  Every frame is resized to the same size.

  Transparent parts of a PNG/GIF stay transparent on the CYD, so whatever
  is behind them (the background, another shape) shows through.

NEEDS
  Python 3 and Pillow:   pip install pillow

EXAMPLES
  One picture:
      python img2cyd.py ghost.png --name ghost --width 120
  A mouth drawn in 4 positions, closed -> wide open (becomes frames 0..3):
      python img2cyd.py mouth0.png mouth1.png mouth2.png mouth3.png --name mouth
  Every frame of an animated GIF:
      python img2cyd.py bat.gif --name bat --height 80

MEMORY TIP
  Each pixel costs 2 bytes of flash. A 120x120 image with 4 frames is
  120*120*2*4 = 115,200 bytes. The CYD has about 1.2 MB of room for
  program + pictures with the default settings, so keep pictures small and
  only as big as they appear on screen.
"""

import argparse
import os
import re
import sys

try:
    from PIL import Image, ImageSequence
except ImportError:
    sys.exit("Pillow is not installed. Run:  pip install pillow")

# Magenta is used as the "see-through" marker color. Any real pixel that
# happens to be exactly this color is nudged by one shade so it still shows.
TRANSPARENT_565 = 0xF81F


def to_565(r, g, b):
    """Pack 0-255 red/green/blue into the screen's 16-bit RGB565 format."""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def load_frames(paths):
    """Read every picture (and every frame of a GIF) as RGBA images."""
    frames = []
    for path in paths:
        img = Image.open(path)
        if getattr(img, "is_animated", False):
            for frame in ImageSequence.Iterator(img):
                frames.append(frame.convert("RGBA"))
        else:
            frames.append(img.convert("RGBA"))
    return frames


def target_size(first, width, height):
    """Work out the output size, keeping the shape if only one side is given."""
    w, h = first.size
    if width and height:
        return width, height
    if width:
        return width, max(1, round(h * width / w))
    if height:
        return max(1, round(w * height / h)), height
    return w, h


def main():
    parser = argparse.ArgumentParser(
        description="Convert pictures into a CYD Image header (.h).")
    parser.add_argument("images", nargs="+", help="PNG/JPG/GIF files, in frame order")
    parser.add_argument("--name", help="name to use in code (default: first file name)")
    parser.add_argument("--width", type=int, help="resize to this width (pixels)")
    parser.add_argument("--height", type=int, help="resize to this height (pixels)")
    parser.add_argument("--out", help="output .h file (default: <name>.h)")
    args = parser.parse_args()

    name = args.name or os.path.splitext(os.path.basename(args.images[0]))[0]
    name = re.sub(r"\W", "_", name)                  # must be a valid C name
    if name[0].isdigit():
        name = "img_" + name
    out_path = args.out or name + ".h"

    frames = load_frames(args.images)
    w, h = target_size(frames[0], args.width, args.height)

    pixels = []
    any_transparent = False
    for frame in frames:
        data = frame.resize((w, h), Image.LANCZOS).tobytes()   # R,G,B,A,R,G,B,A...
        for i in range(0, len(data), 4):
            r, g, b, a = data[i], data[i + 1], data[i + 2], data[i + 3]
            if a < 128:                              # mostly see-through
                pixels.append(TRANSPARENT_565)
                any_transparent = True
            else:
                c = to_565(r, g, b)
                if c == TRANSPARENT_565:
                    c -= 1                           # keep real magenta visible
                pixels.append(c)

    guard = name.upper() + "_H"
    lines = [
        f"// Made by img2cyd.py from: {', '.join(os.path.basename(p) for p in args.images)}",
        f"// {w} x {h} pixels, {len(frames)} frame(s), {len(pixels) * 2:,} bytes",
        f"// Use:  drawImageCentered(canvas, x, y, {name}, frame);",
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        '#include "Anim.h"',
        "",
        f"const uint16_t {name}_pixels[] PROGMEM = {{",
    ]
    for i in range(0, len(pixels), 12):
        chunk = ", ".join(f"0x{p:04X}" for p in pixels[i:i + 12])
        lines.append(f"  {chunk},")
    lines += [
        "};",
        "",
        f"const Image {name} = {{ {w}, {h}, {len(frames)}, "
        f"{'true' if any_transparent else 'false'}, 0x{TRANSPARENT_565:04X}, {name}_pixels }};",
        "",
        f"#endif // {guard}",
        "",
    ]

    with open(out_path, "w") as f:
        f.write("\n".join(lines))

    print(f"Wrote {out_path}: {w}x{h}, {len(frames)} frame(s), "
          f"{len(pixels) * 2:,} bytes of flash"
          f"{', with transparency' if any_transparent else ''}.")


if __name__ == "__main__":
    main()
