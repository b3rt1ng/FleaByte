#!/usr/bin/env python3
"""Renders the dongle screen exactly as the firmware draws it.

Same coordinates, same Adafruit GFX bitmap font, same RGB565 quantisation,
so the result is what the panel shows rather than a mock-up. Colours are the
true ones: the firmware swaps red and blue because the panel is wired BGR.
"""

import re
import sys
from pathlib import Path
from PIL import Image

W, H = 160, 80
SCALE = 8

FONT_SRC = Path.home() / "Arduino/libraries/Adafruit_GFX_Library/glcdfont.c"

BG = (0x00, 0x00, 0x00)
TEXT = (0xE6, 0xFB, 0xF6)
DIM = (0x3E, 0x6B, 0x63)
CYAN = (0x00, 0xE5, 0xD0)
MAGENTA = (0xFF, 0x2D, 0x8A)
LIME = (0xB6, 0xFF, 0x3C)
RED = (0xFF, 0x3B, 0x30)
AMBER = (0xFF, 0xA5, 0x00)

FRAME_INSET, FRAME_ARM, FRAME_THICK = 3, 9, 2
L_LEFT, L_RIGHT = 14, 146


def load_font():
    text = FONT_SRC.read_text()
    body = text[text.index("{") : text.rindex("}")]
    data = [int(b, 16) for b in re.findall(r"0[xX]([0-9a-fA-F]{2})", body)]
    if len(data) < 256 * 5:
        raise SystemExit(f"font data too short: {len(data)} bytes")
    return data


FONT = load_font()


def quant565(c):
    r, g, b = c
    return (r & 0xF8, g & 0xFC, b & 0xF8)


class Screen:
    def __init__(self):
        self.img = Image.new("RGB", (W, H), quant565(BG))
        self.px = self.img.load()

    def point(self, x, y, c):
        if 0 <= x < W and 0 <= y < H:
            self.px[x, y] = quant565(c)

    def hline(self, x, y, n, c):
        for i in range(n):
            self.point(x + i, y, c)

    def vline(self, x, y, n, c):
        for i in range(n):
            self.point(x, y + i, c)

    def rect(self, x, y, w, h, c):
        for j in range(h):
            self.hline(x, y + j, w, c)

    def line(self, x0, y0, x1, y1, c):
        dx, dy = abs(x1 - x0), -abs(y1 - y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1
        err = dx + dy
        while True:
            self.point(x0, y0, c)
            if x0 == x1 and y0 == y1:
                break
            e2 = 2 * err
            if e2 >= dy:
                err += dy
                x0 += sx
            if e2 <= dx:
                err += dx
                y0 += sy

    def char(self, x, y, ch, c, size):
        idx = ord(ch) * 5
        for i in range(5):
            col = FONT[idx + i]
            for j in range(8):
                if col & 1:
                    if size == 1:
                        self.point(x + i, y + j, c)
                    else:
                        self.rect(x + i * size, y + j * size, size, size, c)
                col >>= 1

    def text(self, x, y, s, c, size=1):
        for k, ch in enumerate(s):
            self.char(x + k * 6 * size, y, ch, c, size)

    def corners(self, c):
        l, t = FRAME_INSET, FRAME_INSET
        r, b = W - 1 - FRAME_INSET, H - 1 - FRAME_INSET
        for k in range(FRAME_THICK):
            self.hline(l, t + k, FRAME_ARM, c)
            self.vline(l + k, t, FRAME_ARM, c)
            self.hline(r - FRAME_ARM + 1, t + k, FRAME_ARM, c)
            self.vline(r - k, t, FRAME_ARM, c)
            self.hline(l, b - k, FRAME_ARM, c)
            self.vline(l + k, b - FRAME_ARM + 1, FRAME_ARM, c)
            self.hline(r - FRAME_ARM + 1, b - k, FRAME_ARM, c)
            self.vline(r - k, b - FRAME_ARM + 1, FRAME_ARM, c)

    def dashed(self, y, frm, to, c):
        for x in range(frm, to, 4):
            self.hline(x, y, 2, c)

    def sd_icon(self, x, y, c):
        w, h, bevel = 9, 12, 3
        for j in range(h):
            self.hline(x, y + j, (w - bevel + j) if j < bevel else w, c)
        for i in (2, 4, 6):
            self.vline(x + i, y + h - 4, 3, BG)

    def save(self, path):
        big = self.img.resize((W * SCALE, H * SCALE), Image.NEAREST)
        big.save(path)
        print(f"  {path}  {big.width}x{big.height}")


def status(name, arrangement, ssid, ip, clients, state, colour,
           sd=None, progress=None, marker=None):
    s = Screen()
    s.corners(CYAN)
    s.text(L_LEFT, 4, "//" + name, CYAN)
    s.text(L_RIGHT - len(arrangement) * 6, 4, arrangement, MAGENTA)
    if sd:
        s.sd_icon(96, 2, sd)
    s.hline(L_LEFT, 15, L_RIGHT - L_LEFT, DIM)
    s.text(L_LEFT, 21, "NET", DIM)
    s.text(40, 21, ssid, TEXT)
    s.text(L_LEFT, 32, "IP", DIM)
    s.text(40, 32, ip, DIM)
    s.dashed(44, L_LEFT, L_RIGHT, DIM)
    s.text(L_LEFT, 54, "NODES", DIM)
    s.text(56, 50, str(clients), CYAN, size=2)
    if marker:
        s.rect(84, 55, 5, 5, marker)
    s.text(92, 54, state, colour)
    if progress is not None:
        cells, bar_w = 16, L_RIGHT - L_LEFT
        cw = bar_w // cells
        filled = int(progress * cells)
        for i in range(cells):
            s.rect(L_LEFT + i * cw, 70, cw - 2, 4, AMBER if i < filled else DIM)
    return s


def access(ssid, password):
    s = Screen()
    s.corners(MAGENTA)
    s.text(14, 4, "//ACCESS", CYAN)
    s.hline(8, 15, W - 16, DIM)
    s.text(8, 22, "SSID", DIM)
    s.text(8, 32, ssid, TEXT)
    s.text(8, 48, "KEY", DIM)
    s.text(8, 58, password, LIME)
    return s


if __name__ == "__main__":
    out = Path(sys.argv[1] if len(sys.argv) > 1 else "screens")
    out.mkdir(parents=True, exist_ok=True)

    access("Fleabyte-8218", "flea-A0058218").save(out / "screen-access.png")

    status("Fleabyte", "QWERTY", "Fleabyte-8218", "192.168.4.1", 1,
           "STANDBY", DIM, sd=CYAN, marker=CYAN).save(out / "screen-idle.png")

    status("Fleabyte", "AZERTY", "Fleabyte-8218", "192.168.4.1", 2,
           "EXEC 7/12", AMBER, sd=AMBER, progress=7 / 12,
           marker=AMBER).save(out / "screen-running.png")
