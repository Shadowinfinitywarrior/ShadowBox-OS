#!/usr/bin/env python3
import sys, re
import numpy as np
from PIL import Image

FONT_H = "/home/darkdevil404/OS/userland/font8x8.h"
src = open(FONT_H).read()

glyphs = []
for m in re.finditer(r"\{([0-9a-fA-Fx, ]+?)\}", src):
    vals = [int(v, 16) for v in re.findall(r"0x([0-9a-fA-F]+)", m.group(1))]
    if len(vals) == 8:
        glyphs.append(vals)
g = np.array(glyphs, dtype=np.uint8)  # (N,8) bytes are rows
G = ((g[:, :, None] >> np.arange(8)[None, None, :]) & 1).transpose(0, 2, 1)  # (N, col, row)

def decode(ppm):
    img = np.array(Image.open(ppm).convert("RGB")).astype(int)
    H, W = img.shape[:2]
    lum = img[..., 0] * 0.299 + img[..., 1] * 0.587 + img[..., 2] * 0.114
    rows = []
    for r in range(24):
        y = 182 + r * 12
        if y + 8 > H:
            rows.append("")
            continue
        line = ""
        for c in range(60):
            x = 158 + c * 8
            if x + 8 > W:
                break
            cell = lum[y:y+8, x:x+8]
            if not (cell > 100).any():
                line += " "
                continue
            B = (cell > 100).astype(np.int8).T
            match = np.sum(B[None, :, :] == G, axis=(1, 2))
            gi = int(np.argmax(match))
            b = int(match[gi])
            if b < 50:
                line += " "
            elif 32 <= gi < 127:
                line += chr(gi)
            else:
                line += "[%d]" % gi
        rows.append(line)
    return rows

if __name__ == "__main__":
    for f in sys.argv[1:]:
        print("=== %s ===" % f)
        for i, line in enumerate(decode(f)):
            print("%2d: %s" % (i, line))
