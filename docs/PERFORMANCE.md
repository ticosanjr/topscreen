# Performance notes

## Text overlay

The key overlay used to issue **five draws** per frame when the shadow was enabled
(four shadow offsets plus the main text). The shadow quads and the main quads are
now written into the same vertex buffer, with black/white vertex colors; the
shader combines them in **one draw**. The integration test
`integration_text_batches_glyph_draws` pins this behavior.

## Wallpaper blur / diffusion

The wallpaper shader used to apply a **stretched 3×3 kernel** (9 taps). At large
radii the taps were so far apart that individual source pixels were still
visible, which read as a grid of repeated pixels instead of frosted glass.

The diffusion selector now maps to real kernels:

| Mode | Taps | Kernel |
|------|------|--------|
| Soft | 9 | 3×3, fast path |
| Gaussian | 25 | 5×5 Gaussian (σ = 1 grid unit) |
| Creamy | 25 | 5×5 Gaussian, wider spread |
| Frosted glass | 25 | 5×5 Gaussian, wider spread + sub-pixel grid jitter |

Soft is the low-cost option; the 25-tap modes trade texture samples for a smooth
result. The shader still uses no loops or uniform arrays, so it stays compatible
with the D3D11 and OpenGL backends.

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
