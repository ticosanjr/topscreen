# Performance notes

## Text overlay

The key overlay used to issue **five draws** per frame when the shadow was enabled
(four shadow offsets plus the main text). The shadow quads and the main quads are
now written into the same vertex buffer, with black/white vertex colors; the
shader combines them in **one draw**. The integration test
`integration_text_batches_glyph_draws` pins this behavior.

## Wallpaper blur / diffusion

The wallpaper is decoded once and kept in memory (capped at 4096 px on the long
edge). When the image, blur amount, diffusion mode or target size changes, a
worker thread builds a prefiltered texture while the previous texture stays on
screen. Large radii are downscaled first, then filtered; Gaussian/Creamy/Frosted
use two box passes for a smoother gradient. The shader now takes **one filtered
sample** per pixel instead of the old 9/25 taps, so the cost never blocks normal
playback.

The diffusion selector still changes the effective spread:

| Mode | Prefilter |
|------|-----------|
| Soft | narrower box kernel |
| Gaussian | balanced kernel |
| Creamy | wider kernel |
| Frosted glass | widest kernel, with a softer upscale |

All modes use the same one-sample shader path, including the magnifier lens and
the full-frame spotlight blur. The worker and CPU image cache are shared by all TopScreen filters; only the
final texture upload happens on the render thread. The
spotlight blur itself still uses nine scene taps, but its shadow is applied once
after the blur instead of once per tap.

## Background gradient

The gradient direction (`cos`/`sin` of the angle) is now computed once when the
angle changes and uploaded as a `float2` (`bg_gradient_dir`) every frame. The
shader no longer recomputes trigonometry for every covered pixel; the visual
result is identical. Caching only the CPU math keeps the per-frame uniform upload
required by D3D11 intact.

## Dock refresh

The dock polls the filter registry every 500 ms. It now reads a small revision
counter first and only allocates/copies the target snapshot when a filter is
added or removed. The selector is still rebuilt when needed, but the idle path
does not allocate a vector of strings on every tick.

## Click audio (Linux)

The old path spawned a new `paplay`/`pw-play`/`aplay` process for every click.
When `paplay` is available, TopScreen now keeps one raw PCM stream open and
writes the click samples to it. The one-shot spawn remains only as a fallback.
Windows continues to use the asynchronous `PlaySoundW` path.

## Cursor follow modes

`Exact` and `Spring` keep the previous cost (one spring update per frame).
`GlideSampled` updates the destination at a configurable interval (default
90 ms) and `GlideAB` uses one eased segment per accepted target. Both reuse the
same cursor state and add no GPU work.

## Water ripple and magnifier

The water-ripple click changes the sampling UV for the captured image; it adds
no texture sample and reuses the four existing click slots. The magnifier adds
one extra screen sample only inside the lens mask (pixels outside keep the
normal path). Neither effect adds a render target or a second pass.

## Cursor atlas

The 106-cell cursor atlas (79 visible styles + 27 reserved/transparent IDs) is a 2048×896 RGBA texture (~7.3 MB). That is small
compared with a 4K frame buffer, so the release does **not** use BC3/BC7 or a
destructive 64 px downsample; those would add format/quality risk for little
gain. The grid layout already avoids the 8192 px texture-height limit.

## OBS benchmark (historical)

The numbers below are from the old per-pixel wallpaper blur and are kept for
context only. They are not a current 0.6.x benchmark.

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

## Current 1.1.2 smoke observation

At 1920×1080 with llvmpipe software rendering, wallpaper blur 30, the async
wallpaper prefilter and the magnifier lens active, OBS reported 60/60 FPS and
roughly 45% CPU in its status bar. This is a smoke observation, not a controlled
GPU benchmark or a comparison against 0.6.2.
