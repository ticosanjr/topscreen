# Performance notes

## Text overlay

The key overlay used to issue **five draws** per frame when the shadow was enabled
(four shadow offsets plus the main text). The shadow quads and the main quads are
now written into the same vertex buffer, with black/white vertex colors; the
shader combines them in **one draw**. The integration test
`integration_text_batches_glyph_draws` pins this behavior.

## Wallpaper blur

The background blur shader went from **13 taps** (center + two rings) to
**9 taps** (center + cross + diagonals), a 30% reduction in texture samples.
The radius still varies by blur type.

## Click audio (Linux)

The old path spawned a new `paplay`/`pw-play`/`aplay` process for every click.
When `paplay` is available, TopScreen now keeps one raw PCM stream open and
writes the click samples to it. The one-shot spawn remains only as a fallback.
Windows continues to use the asynchronous `PlaySoundW` path.

## Cursor atlas

The 106-cursor atlas is a 2048×896 RGBA texture (~7.3 MB). That is small
compared with a 4K frame buffer, so the release does **not** use BC3/BC7 or a
destructive 64 px downsample; those would add format/quality risk for little
gain. The grid layout already avoids the 8192 px texture-height limit.

## OBS benchmark

Environment: Linux, OBS Studio 30.0.2.1, **llvmpipe software rendering** (no GPU
available in the test VM), Xvfb at 1920×1080 and 3840×2160, 60 FPS. Scene:
screen capture + TopScreen with a wallpaper, blur 30, cursor and effects on.
CPU time is the OBS process user+system time sampled for 15 seconds after a
6-second startup.

| Resolution | 0.2.23 CPU seconds (2 runs) | 0.2.24 CPU seconds (2 runs) |
|---|---:|---:|
| 1920×1080 | 136.19 / 129.48 | 125.90 / 134.94 |
| 3840×2160 | 143.92 / 146.11 | 138.84 / 131.22 |

At 1080p the two runs average ~1.8% less CPU time in 0.2.24; an additional
20-second sample showed ~3%. At 4K the samples ranged from ~7% better to ~1%
worse, i.e. within software-rendering noise. OBS reported 60/60 FPS in both
resolutions.

This is a CPU/software-rendering comparison, not a GPU benchmark. The text draw
count and the blur tap reduction are deterministic and pinned by tests; the
wall-clock numbers should be re-measured on real GPU hardware before making
larger performance claims.
