#!/usr/bin/env python3
"""
Imports the local click-sound pack into data/sounds/.

The source files live in Resources/sounds (see manifest.json). Each file is
converted to mono 16-bit PCM at 44.1 kHz, trimmed to its audible bounds,
peak-normalised and given a 3 ms fade in/out. The MP3 source is converted by
ffmpeg. The resulting WAVs are what the plugin ships.

Usage: python3 tools/import-sound-pack.py
"""
import json
import os
import struct
import subprocess
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "Resources" / "sounds"
DST = ROOT / "data" / "sounds"
RATE = 44100
THRESHOLD = 200
MARGIN = int(0.003 * RATE)


def convert(source: Path, target: Path):
    subprocess.run(
        [
            "ffmpeg",
            "-hide_banner",
            "-loglevel",
            "error",
            "-y",
            "-i",
            str(source),
            "-ac",
            "1",
            "-ar",
            str(RATE),
            "-c:a",
            "pcm_s16le",
            str(target),
        ],
        check=True,
    )

    with wave.open(str(target), "rb") as f:
        frames = list(struct.unpack("<%dh" % f.getnframes(), f.readframes(f.getnframes())))
    if not frames:
        raise SystemExit(f"empty sound: {target}")

    first = next((i for i, v in enumerate(frames) if abs(v) > THRESHOLD), 0)
    last = len(frames) - 1 - next(
        (i for i, v in enumerate(reversed(frames)) if abs(v) > THRESHOLD), 0
    )
    frames = frames[max(0, first - MARGIN) : min(len(frames) - 1, last + MARGIN) + 1]

    peak = max(abs(v) for v in frames) or 1
    gain = 0.90 * 32767 / peak
    out = [max(-32768, min(32767, int(v * gain))) for v in frames]
    fade = min(MARGIN, len(out) // 2)
    for i in range(fade):
        a = i / fade
        out[i] = int(out[i] * a)
        out[-1 - i] = int(out[-1 - i] * a)

    with wave.open(str(target), "wb") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(RATE)
        f.writeframes(struct.pack("<%dh" % len(out), *out))
    return len(out) / RATE


def main():
    DST.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((SRC / "manifest.json").read_text())["sounds"]
    for entry in manifest:
        source = SRC / entry["file"]
        target = DST / (Path(entry["file"]).stem + ".wav")
        duration = convert(source, target)
        print(f"[sound] {target.name}: {duration:.3f}s <- {source.name}")
    print(f"[sound] OK: {len(manifest)} sounds in data/sounds/")


if __name__ == "__main__":
    main()
