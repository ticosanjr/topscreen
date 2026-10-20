#!/usr/bin/env python3
"""
Generates the TopScreen cursor atlas.

- Extracts real cursors from installed XCursor themes (Bibata, Breeze, Adwaita,
  GoogleDot) reading the XCursor format. Each themed arrow also ships the
  theme's own `pointer` (hover/link) hand.
- Draws "macOS (classic)" and "Windows (classic)" procedurally.
- Appends the user-supplied 128x128 PNGs from Resources/cursors.
- Packs everything into data/cursors/atlas.rgba (TSAT) + atlas.json (hotspots).

TSAT format (little-endian):
  0  : magic 'TSAT'
  4  : u32 version = 2
  8  : u32 cell (icon side, px)
  12 : u32 count
  16 : u32 columns
  20 : pixels RGBA8 (columns x ceil(count/columns) cells, row-major)

Version 1 (no columns field, one vertical column) is still accepted by the
runtime parser.

Usage: python3 tools/make-cursor-atlas.py
"""
import json
import math
import os
import struct
import sys

from PIL import Image

CELL = 128
COLUMNS = 16
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(ROOT, "data", "cursors")
CUSTOM_MANIFEST = os.path.join(ROOT, "Resources", "cursors", "manifest.json")

ICONS = "/usr/share/icons"

# name, source (xcursor path)  -- order = CursorStyle id in the code
SOURCES = [
    ("macos", "__procedural_mac__"),
    ("windows", "__procedural_win__"),
    ("breeze", f"{ICONS}/breeze_cursors/cursors/left_ptr"),
    ("bibata", f"{ICONS}/Bibata-Modern-Classic/cursors/left_ptr"),
    ("adwaita", f"{ICONS}/Adwaita/cursors/left_ptr"),
    ("googledot", f"{ICONS}/GoogleDot-White/cursors/left_ptr"),
    ("pointer", f"{ICONS}/Adwaita/cursors/pointer"),
    ("dmz", f"{ICONS}/DMZ-White/cursors/left_ptr"),
    ("yaru", f"{ICONS}/Yaru/cursors/left_ptr"),
    ("xcursorpro", f"{ICONS}/XCursor-Pro-Light/cursors/left_ptr"),
    ("bibataice", f"{ICONS}/Bibata-Modern-Ice/cursors/left_ptr"),
    ("bibataoriginal", f"{ICONS}/Bibata-Original-Classic/cursors/left_ptr"),
    ("neon", "__procedural_neon__"),
    ("dot", "__procedural_dot__"),
    ("cross", "__procedural_cross__"),
    ("onyx", "__procedural_onyx__"),
    # Hands, text and extra styles (real themes when installed).
    ("handgrab", f"{ICONS}/Adwaita/cursors/grab"),
    ("handgrabbing", f"{ICONS}/Adwaita/cursors/grabbing"),
    ("ibeam", f"{ICONS}/Adwaita/cursors/xterm"),
    ("dmzblack", f"{ICONS}/DMZ-Black/cursors/left_ptr"),
    ("googledotblack", f"{ICONS}/GoogleDot-Black/cursors/left_ptr"),
    ("breezesnow", f"{ICONS}/Breeze_Snow/cursors/left_ptr"),
    ("bibataiceorig", f"{ICONS}/Bibata-Original-Ice/cursors/left_ptr"),
    ("yarugrab", f"{ICONS}/Yaru/cursors/grab"),
    ("yaruibeam", f"{ICONS}/Yaru/cursors/xterm"),
    ("breezeibeam", f"{ICONS}/breeze_cursors/cursors/xterm"),
    # Hover / link hands: the REAL `pointer` cursor of the very same themes
    # (nothing is drawn here, the theme files are used as shipped).
    # GoogleDot-White/Black are excluded: that theme ships no hand at all
    # (its "pointer" is a dot, like every other cursor in it).
    ("breezehover", f"{ICONS}/breeze_cursors/cursors/pointer"),
    ("bibatahover", f"{ICONS}/Bibata-Modern-Classic/cursors/pointer"),
    ("adwaitahover", f"{ICONS}/Adwaita/cursors/pointer"),
    ("dmzhover", f"{ICONS}/DMZ-White/cursors/hand2"),
    ("yaruhover", f"{ICONS}/Yaru/cursors/pointer"),
    ("xcursorprohover", f"{ICONS}/XCursor-Pro-Light/cursors/pointer"),
    ("bibataicehover", f"{ICONS}/Bibata-Modern-Ice/cursors/pointer"),
    ("bibataoriginalhover", f"{ICONS}/Bibata-Original-Classic/cursors/pointer"),
    ("dmzblackhover", f"{ICONS}/DMZ-Black/cursors/hand2"),
    ("breezesnowhover", f"{ICONS}/Breeze_Snow/cursors/pointer"),
    ("bibataiceorighover", f"{ICONS}/Bibata-Original-Ice/cursors/pointer"),
]


# ---------------------------------------------------------------------------
# XCursor
# ---------------------------------------------------------------------------
def parse_xcursor(path):
    data = open(path, "rb").read()
    magic, hsize, version, ntoc = struct.unpack_from("<4sIII", data, 0)
    if magic != b"Xcur":
        raise ValueError(f"not an XCursor: {path}")
    images = []
    for i in range(ntoc):
        ctype, subtype, pos = struct.unpack_from("<III", data, hsize + i * 12)
        if ctype != 0xFFFD0002:  # image
            continue
        hsize2, itype, isub, iver, w, h, xh, yh, delay = struct.unpack_from("<IIIIIIIII", data, pos)
        pix = struct.unpack_from("<%dI" % (w * h), data, pos + hsize2)
        images.append({"w": w, "h": h, "xhot": xh, "yhot": yh, "pix": pix})
    if not images:
        raise ValueError(f"no images: {path}")
    return images


def best_image(images):
    """Largest image with side <= 128 (sharp and light)."""
    cand = [im for im in images if max(im["w"], im["h"]) <= 128] or images
    return max(cand, key=lambda im: max(im["w"], im["h"]))


def argb_to_rgba(pix, w, h):
    out = []
    for p in pix:
        a = (p >> 24) & 0xFF
        r = (p >> 16) & 0xFF
        g = (p >> 8) & 0xFF
        b = p & 0xFF
        out.append((r, g, b, a))
    return out


# ---------------------------------------------------------------------------
# Procedural rasterization (polygons with anti-aliasing)
# ---------------------------------------------------------------------------
def point_in_poly(x, y, poly):
    inside = False
    n = len(poly)
    j = n - 1
    for i in range(n):
        xi, yi = poly[i]
        xj, yj = poly[j]
        if (yi > y) != (yj > y):
            xint = (xj - xi) * (y - yi) / (yj - yi) + xi
            if x < xint:
                inside = not inside
        j = i
    return inside


def dist_seg(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    l2 = dx * dx + dy * dy
    if l2 == 0:
        return math.hypot(px - ax, py - ay)
    t = max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / l2))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


def sd_poly(x, y, poly):
    d = min(dist_seg(x, y, *poly[i], *poly[(i + 1) % len(poly)]) for i in range(len(poly)))
    return -d if point_in_poly(x, y, poly) else d


def rasterize(poly, fill, outline, outline_w, grid=24):
    """Rasterizes a polygon (coordinates in `grid`) to CELL x CELL RGBA."""
    scale = CELL / grid
    src = [(px * scale, py * scale) for (px, py) in poly]
    ow = outline_w * scale
    ss = 3  # supersampling
    out = bytearray(CELL * CELL * 4)
    for y in range(CELL):
        for x in range(CELL):
            ar = ag = ab = aa = 0
            for sy in range(ss):
                for sx in range(ss):
                    fx = x + (sx + 0.5) / ss
                    fy = y + (sy + 0.5) / ss
                    d = sd_poly(fx, fy, src)
                    if d <= 0.0:
                        r, g, b, a = fill
                    elif d <= ow:
                        r, g, b, a = outline
                    else:
                        r = g = b = a = 0
                    ar += r * a
                    ag += g * a
                    ab += b * a
                    aa += a
            n = ss * ss
            a = aa / n
            idx = (y * CELL + x) * 4
            if a > 0:
                out[idx + 0] = min(255, int(ar / aa))
                out[idx + 1] = min(255, int(ag / aa))
                out[idx + 2] = min(255, int(ab / aa))
                out[idx + 3] = min(255, int(a))
            else:
                out[idx + 3] = 0
    return out


MAC_POLY = [(1.2, 1.0), (1.2, 18.0), (5.3, 14.2), (8.4, 21.5), (11.2, 20.3),
            (8.1, 13.2), (13.2, 13.2)]
WIN_POLY = [(0.8, 0.6), (0.8, 17.5), (4.8, 13.7), (7.9, 19.9), (10.3, 18.9),
            (7.2, 12.6), (12.0, 12.6)]


def procedural(kind):
    if kind == "mac":
        # macOS: black fill, thin white outline
        return rasterize(MAC_POLY, (0, 0, 0, 255), (255, 255, 255, 235), 0.9)
    if kind == "win":
        # Windows: white fill, black outline
        return rasterize(WIN_POLY, (255, 255, 255, 255), (0, 0, 0, 255), 1.0)
    if kind == "neon":
        # Neon: glowing cyan arrow for dark scenes
        return rasterize(MAC_POLY, (0, 229, 255, 255), (8, 16, 38, 255), 1.0)
    if kind == "dot":
        # Dot: coral disc, centered hotspot (touch-style)
        poly = [(12 + 5 * math.cos(2 * math.pi * i / 16),
                 12 + 5 * math.sin(2 * math.pi * i / 16)) for i in range(16)]
        return rasterize(poly, (255, 92, 138, 255), (255, 255, 255, 255), 0.7)
    if kind == "cross":
        # Cross: precision crosshair, centered hotspot
        poly = [(11, 3), (13, 3), (13, 11), (21, 11), (21, 13), (13, 13),
                (13, 21), (11, 21), (11, 13), (3, 13), (3, 11), (11, 11)]
        return rasterize(poly, (255, 255, 255, 255), (20, 20, 30, 255), 0.8)
    else:
        # Onyx: navy arrow with a gold outline (elegant dark theme)
        return rasterize(WIN_POLY, (26, 26, 46, 255), (255, 209, 102, 255), 0.9)


# ---------------------------------------------------------------------------
# Atlas composition
# ---------------------------------------------------------------------------
def resample_to_cell(rgba, w, h):
    """Resizes (bilinear) to CELL x CELL, centered."""
    out = bytearray(CELL * CELL * 4)
    sc = min(CELL / w, CELL / h)
    dw, dh = max(1, int(w * sc)), max(1, int(h * sc))
    ox, oy = (CELL - dw) // 2, (CELL - dh) // 2
    for y in range(dh):
        for x in range(dw):
            sx = min(w - 1, int(x / sc))
            sy = min(h - 1, int(y / sc))
            si = (sy * w + sx) * 4
            di = ((oy + y) * CELL + (ox + x)) * 4
            out[di:di + 4] = rgba[si:si + 4]
    return out, ox, oy, sc


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    cells = []
    meta = []

    for name, src in SOURCES:
        if src == "__procedural_mac__":
            cells.append(procedural("mac"))
            meta.append({"name": name, "hotspot": [round(1.2 / 24, 4), round(1.0 / 24, 4)]})
            print(f"[atlas] {name}: procedural mac")
            continue
        if src == "__procedural_win__":
            cells.append(procedural("win"))
            meta.append({"name": name, "hotspot": [round(0.8 / 24, 4), round(0.6 / 24, 4)]})
            print(f"[atlas] {name}: procedural windows")
            continue
        if src == "__procedural_neon__":
            cells.append(procedural("neon"))
            meta.append({"name": name, "hotspot": [round(1.2 / 24, 4), round(1.0 / 24, 4)]})
            print(f"[atlas] {name}: procedural neon")
            continue
        if src == "__procedural_dot__":
            cells.append(procedural("dot"))
            meta.append({"name": name, "hotspot": [0.5, 0.5]})
            print(f"[atlas] {name}: procedural dot")
            continue
        if src == "__procedural_cross__":
            cells.append(procedural("cross"))
            meta.append({"name": name, "hotspot": [0.5, 0.5]})
            print(f"[atlas] {name}: procedural cross")
            continue
        if src == "__procedural_onyx__":
            cells.append(procedural("onyx"))
            meta.append({"name": name, "hotspot": [round(0.8 / 24, 4), round(0.6 / 24, 4)]})
            print(f"[atlas] {name}: procedural onyx")
            continue

        if not os.path.exists(src):
            raise SystemExit(f"cursor not found: {src}")

        im = best_image(parse_xcursor(src))
        rgba = bytearray()
        for (r, g, b, a) in argb_to_rgba(im["pix"], im["w"], im["h"]):
            rgba += bytes((r, g, b, a))

        cell, ox, oy, sc = resample_to_cell(rgba, im["w"], im["h"])
        hx = (ox + im["xhot"] * sc) / CELL
        hy = (oy + im["yhot"] * sc) / CELL
        cells.append(cell)
        meta.append({"name": name, "hotspot": [round(hx, 4), round(hy, 4)]})
        print(f"[atlas] {name}: {im['w']}x{im['h']} hotspot=({hx:.3f},{hy:.3f}) from {src}")

    # User-supplied PNGs (already 128x128, straight RGBA). The manifest order
    # is the CursorStyle order; IDs start at 37 (after the 37 built-ins).
    with open(CUSTOM_MANIFEST, "r", encoding="utf-8") as f:
        custom = json.load(f)["cursors"]
    for expected, entry in enumerate(custom, start=37):
        if entry["id"] != expected:
            raise SystemExit(f"custom cursor id {entry['id']} != {expected}")
        path = os.path.join(ROOT, "Resources", entry["file"])
        if not os.path.exists(path):
            raise SystemExit(f"cursor not found: {path}")
        im = Image.open(path).convert("RGBA")
        if im.size != (CELL, CELL):
            im = im.resize((CELL, CELL), Image.Resampling.LANCZOS)
        rgba = bytearray(im.tobytes())
        alpha = im.getchannel("A")
        hx = hy = 0.05
        for y in range(CELL):
            for x in range(CELL):
                if alpha.getpixel((x, y)) > 32:
                    hx = (x + 0.5) / CELL
                    hy = (y + 0.5) / CELL
                    break
            else:
                continue
            break
        cells.append(rgba)
        meta.append({"name": entry["slug"], "hotspot": [round(hx, 4), round(hy, 4)]})
        print(f"[atlas] {entry['slug']}: PNG hotspot=({hx:.3f},{hy:.3f})")

    count = len(cells)
    rows = (count + COLUMNS - 1) // COLUMNS
    width = CELL * COLUMNS
    height = CELL * rows
    blob = bytearray(width * height * 4)
    for idx, cell in enumerate(cells):
        row, col = divmod(idx, COLUMNS)
        for y in range(CELL):
            src_off = y * CELL * 4
            dst_off = ((row * CELL + y) * width + col * CELL) * 4
            blob[dst_off:dst_off + CELL * 4] = cell[src_off:src_off + CELL * 4]

    with open(os.path.join(OUT_DIR, "atlas.rgba"), "wb") as f:
        f.write(b"TSAT")
        f.write(struct.pack("<IIII", 2, CELL, count, COLUMNS))
        f.write(blob)

    with open(os.path.join(OUT_DIR, "atlas.json"), "w") as f:
        json.dump({"cell": CELL, "count": count, "columns": COLUMNS, "cursors": meta}, f, indent=2)
        f.write("\n")

    print(f"[atlas] OK: {count} cursors, grid={COLUMNS}x{rows}, cell={CELL} -> data/cursors/")


if __name__ == "__main__":
    sys.exit(main())
