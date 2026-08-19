#!/usr/bin/env python3
"""Convert icons/*.png into 32x32 24-bit BMP tiles the OS can load.

The OS (desktop.c / wallpaper_engine.c) parses 24-bit bottom-up BMPs with a
54-byte header. PNG alpha is composited over the desktop tile color 0x34495E so
transparent corners keep the rounded-tile look. Output goes to userland/icons/
so the Makefile packs them into initrd.tar at /icons/<id>.bmp.

Source icons must be named <id>.png (matching the launcher/app IDs).
"""

import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "icons"
DST = ROOT / "userland" / "icons"
SIZE = 32
TILE_BG = (0x34, 0x49, 0x5E)  # 0x34495E


def main() -> int:
    sources = sorted(SRC.glob("*.png"))
    if not sources:
        print("No icons/*.png found.")
        return 1

    DST.mkdir(parents=True, exist_ok=True)
    written = []
    for src in sources:
        img = Image.open(src).convert("RGBA")
        img = img.resize((SIZE, SIZE), Image.LANCZOS)

        bg = Image.new("RGBA", (SIZE, SIZE), TILE_BG + (255,))
        out = Image.alpha_composite(bg, img).convert("RGB")

        dst = DST / f"{src.stem}.bmp"
        out.save(dst, "BMP")
        written.append(dst.name)

    print(f"Wrote {len(written)} icons to userland/icons/")
    return 0


if __name__ == "__main__":
    sys.exit(main())
