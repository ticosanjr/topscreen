#!/usr/bin/env python3
"""
Downloads the Unsplash photographic wallpapers used by TopScreen.

All source photos are regular Unsplash content under the Unsplash License
(free to use and redistribute; attribution is a courtesy, not a requirement).
Each file is fetched at 2400 px, centre-cropped to 1920x1080 and saved as a
progressive JPEG. The URLs are also listed in data/wallpapers/SOURCES.txt.

Usage: python3 tools/fetch-unsplash-wallpapers.py
"""
import os
from io import BytesIO
from urllib.request import Request, urlopen

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "data", "wallpapers")
SIZE = (1920, 1080)

PHOTOS = [
    ("violet-wave", "photo-1758551051834-61f10a361b73"),
    ("wood-arch", "photo-1773761542060-62301d65af43"),
    ("color-melt", "photo-1636690636968-4568d7e94fe7"),
    ("ivory-dunes", "photo-1749371930388-50c782b0acea"),
    ("glacier", "photo-1786616015639-88ea964c1bd7"),
    ("pink-ribbon", "photo-1779393438516-c08555a3c6f4"),
    ("graphite-fold", "photo-1752679813117-49fdab167868"),
    ("amber-ribbon", "photo-1779393442957-c644aed6c984"),
]


def fetch(photo_id):
    url = f"https://images.unsplash.com/{photo_id}?q=90&w=2400&auto=format&fit=crop"
    req = Request(url, headers={"User-Agent": "TopScreen wallpaper fetcher"})
    with urlopen(req, timeout=60) as response:
        return Image.open(BytesIO(response.read())).convert("RGB")


def crop_16_9(image):
    width, height = image.size
    scale = max(SIZE[0] / width, SIZE[1] / height)
    resized = image.resize((round(width * scale), round(height * scale)), Image.Resampling.LANCZOS)
    left = (resized.width - SIZE[0]) // 2
    top = (resized.height - SIZE[1]) // 2
    return resized.crop((left, top, left + SIZE[0], top + SIZE[1]))


def main():
    os.makedirs(OUT, exist_ok=True)
    for name, photo_id in PHOTOS:
        image = crop_16_9(fetch(photo_id))
        path = os.path.join(OUT, f"{name}.jpg")
        image.save(path, "JPEG", quality=90, optimize=True, progressive=True)
        print(f"[wallpaper] {name}: {os.path.getsize(path) // 1024} KB <- {photo_id}")
    print(f"[wallpaper] OK: {len(PHOTOS)} Unsplash photos in data/wallpapers/")


if __name__ == "__main__":
    main()
