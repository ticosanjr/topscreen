#!/usr/bin/env python3
"""
Generates the TopScreen built-in wallpapers (macOS style: soft gradients).

Output: data/wallpapers/*.jpg (1920x1080) for the generated gradient set only.

The 18 Unsplash photographic wallpapers (silk, pastel, helix, fiber, prism,
twilight, ink, spark, cobalt, dusk, violet-wave, wood-arch, color-melt,
ivory-dunes, glacier, pink-ribbon, graphite-fold, amber-ribbon) are fetched by
tools/fetch-unsplash-wallpapers.py - see data/wallpapers/SOURCES.txt; this tool
never overwrites or removes them.

Usage: python3 tools/make-wallpapers.py
"""
import os
import random
import sys

from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "data", "wallpapers")
SIZE = (1920, 1080)

# name -> palette (3-5 colors)  -- inspired by the macOS wallpapers
PALETTES = {
    "aurora":    ["#0b2a4a", "#0f6a6a", "#2fd0a8", "#7b4bd8", "#0b2a4a"],
    "sunset":    ["#2b1055", "#7b2ff7", "#f107a3", "#ff8a3d", "#2b1055"],
    "monterey":  ["#0a1a3f", "#2b4bd8", "#8a2be2", "#ff5c8a", "#0a1a3f"],
    "ventura":   ["#20243e", "#ff6a3d", "#ff2e63", "#4b6cff", "#20243e"],
    "sonoma":    ["#1b2a41", "#2f6f8f", "#f0a35e", "#e05252", "#1b2a41"],
    "sequoia":   ["#241b4d", "#6a3fd8", "#c86bff", "#3fa9ff", "#241b4d"],
    "graphite":  ["#141416", "#2c2c30", "#4a4a52", "#6e6e78", "#141416"],
    "ocean":     ["#031b33", "#04507a", "#0f8fb0", "#22d3ee", "#031b33"],
    "forest":    ["#071a12", "#12563a", "#2f9e5a", "#a3e635", "#071a12"],
    "lavender":  ["#1e1b3a", "#5b4bd8", "#a78bfa", "#f4c2ff", "#1e1b3a"],
    "ember":     ["#1a0a0a", "#7a1f1f", "#e0452b", "#ffb703", "#1a0a0a"],
    "midnight":  ["#05060f", "#101a3a", "#2b3fa8", "#6a4bd8", "#05060f"],
    "neon":      ["#0d0221", "#ff006e", "#8338ec", "#3a86ff", "#00f5d4"],
    "cyberpunk": ["#0a0f1e", "#f9f002", "#ff2a6d", "#05d9e8", "#0a0f1e"],
    "tropical":  ["#04395e", "#0ead69", "#f5e960", "#ff5d8f", "#04395e"],
    "coral":     ["#2b0b3a", "#ff5c8a", "#ff9f1c", "#ffd166", "#2b0b3a"],
    "galaxy":    ["#0b032d", "#843b62", "#f67e7d", "#6a2c70", "#0b032d"],
    "rainbow":   ["#2d00f7", "#c200fb", "#0075f2", "#ff0a54", "#00e5ff"],
    "porcelain": ["#e8e4dc", "#cfc8bb", "#f5f2ec", "#a8b5c4", "#e8e4dc"],
    "sand":      ["#e6d5b8", "#d4a373", "#f4ead5", "#b08968", "#e6d5b8"],
    "sage":      ["#dfe5d8", "#9caf88", "#5f7161", "#e9f0e4", "#dfe5d8"],
    "onyx":      ["#08080c", "#1c1c24", "#3a3a4a", "#6a5cff", "#08080c"],
    "rose":      ["#2b0f1e", "#c86b8a", "#f4a7c3", "#f9e0e8", "#2b0f1e"],
    "azure":     ["#dbe9f6", "#7fb3e0", "#2f6fb0", "#f2f7fd", "#dbe9f6"],
}


def hex2rgb(h):
    h = h.lstrip("#")
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def mesh(colors, seed):
    """Mesh gradient: colored points on a small grid, smooth upscale."""
    random.seed(seed)
    sw, sh = 24, 14
    small = Image.new("RGB", (sw, sh), colors[0])
    px = small.load()
    pts = [(random.randrange(sw), random.randrange(sh)) for _ in range(len(colors) * 3)]
    for i, (x, y) in enumerate(pts):
        px[x, y] = colors[i % len(colors)]
    return small.resize(SIZE, Image.BICUBIC)


def radial_mask(size, cx, cy, r, blur):
    m = Image.new("L", size, 0)
    ImageDraw.Draw(m).ellipse([cx - r, cy - r, cx + r, cy + r], fill=255)
    return m.filter(ImageFilter.GaussianBlur(blur))


def vignette(size, strength=0.35):
    w, h = size
    m = Image.new("L", size, 255)
    d = ImageDraw.Draw(m)
    steps = 40
    for i in range(steps):
        t = i / steps
        v = int(255 * (1 - strength * t))
        pad = int(min(w, h) * 0.6 * t)
        d.rectangle([-pad + pad, -pad + pad, w + pad, h + pad], outline=v)
    return m.filter(ImageFilter.GaussianBlur(120))


def build(name, palette, seed):
    colors = [hex2rgb(c) for c in palette]
    img = mesh(colors, seed)

    # soft light (highlight) at a random point
    random.seed(seed + 1)
    cx, cy = random.randrange(200, SIZE[0] - 200), random.randrange(100, SIZE[1] - 100)
    r = random.randrange(500, 900)
    hm = radial_mask(SIZE, cx, cy, r, r * 0.6).point(lambda v: int(v * 0.22))
    img = Image.composite(Image.new("RGB", SIZE, (255, 255, 255)), img, hm)

    # vignette
    vm = vignette(SIZE, 0.30)
    dark = Image.new("RGB", SIZE, tuple(int(c * 0.55) for c in colors[0]))
    img = Image.composite(img, dark, vm)

    img = img.filter(ImageFilter.GaussianBlur(0.6))
    return img


def main():
    os.makedirs(OUT, exist_ok=True)
    for i, (name, palette) in enumerate(PALETTES.items()):
        img = build(name, palette, seed=100 + i)
        path = os.path.join(OUT, f"{name}.jpg")
        img.save(path, "JPEG", quality=92, optimize=True)
        print(f"[wallpaper] {name}: {os.path.getsize(path)//1024} KB")
    print(f"[wallpaper] OK: {len(PALETTES)} in data/wallpapers/")


if __name__ == "__main__":
    sys.exit(main())
