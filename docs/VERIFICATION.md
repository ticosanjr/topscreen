# Verification on a real OBS

Everything verifiable **without** OBS is covered by:
[`TESTS.md`](TESTS.md) (core + compilation against headers + integration).
This document is the **manual script** to validate what only OBS can prove.

## 1. Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release     # see docs/BUILD.md
cmake --build build -j
# portable: copy build/topscreen.so (or topscreen.dll) and the data/ folder to OBS
obs --verbose 2>&1 | grep -i topscreen
```

Expected in the log: `[TopScreen] loaded v...`, `filter created (presets=3)`,
`sound loaded: sounds/click-soft.wav`, `evdev: using /dev/input/eventN`
(or a permission warning).

## 2. Test script (visual)

Add a **Display Capture** and the **TopScreen FX** filter. Then:

| # | What to test | Expected |
|---|--------------|----------|
| 1 | Passthrough filter (effects off) | image identical to the capture |
| 2 | Background type | solid color uses one color; gradient blends two colors and follows angle/meeting point; wallpaper keeps the existing image/blur controls |
| 2b | Background screen corner radius/margin/shadow | screen "floating" with background, rounded corners and shadow |
| 2c | Wallpaper diffusion | Soft is the fastest; Gaussian/Creamy/Frosted glass have no visible 3×3 sample grid at large radii; Frosted is the smoothest |
| 3 | Cursor | native cursor disappears; own arrow appears (light with a dark outline) |
| 4 | Cursor size/smoothing/sway | changes with the sliders |
| 5 | Left click | ripple/orb effect at the click point |
| 6 | Right click | effect distinct from the left |
| 7 | Click sound | sound on click (if a player is available) |
| 8 | Automatic zoom | on click, zooms and returns after stopping |
| 9 | Manual zoom (hotkeys) | in/out/reset work |
| 10 | Motion blur | light radial blur during zoom |
| 11 | Key overlay | keys appear at the bottom and disappear after the TTL |
| 12 | Spotlight | `TopScreen_ToggleSpotlight` drops/zooms in/out at the cursor; circle/square; border, halo shadow, outside blur; follows the mouse |
| 13 | Highlighter | dragging with the left button marks the region; `TopScreen_ToggleHighlight` zooms at the cursor on the first press and returns on the second; manual zoom/drag cancels the hotkey toggle |
| 14 | Presets (hotkey) | next/previous switch the look |
| 15 | Profiles | switching A–J also reloads the click sounds (test A vs. D with different WAVs) |
| 16 | Profile import | an older/partial `.json` imports with defaults instead of failing |
| 17 | Effects off / cursor off / filter removed | the capture's native cursor comes back; with two TopScreen filters on the same capture it only returns after the last one releases it |
| 18 | Dock | reflects the state and shows the input tier |

## 3. Hotkeys

**Settings → Hotkeys → TopScreen** must list the 20 actions
(`ToggleEffects`, `ToggleBackground`, `ZoomIn`, `ZoomOut`, `ResetZoom`,
`NextPreset`, `PrevPreset`, `ProfileA` … `ProfileJ`, `ProfileSave`,
`ToggleHighlight`).

## 4. Input per platform

| Platform | Expected |
|------------|----------|
| Windows | cursor + clicks + keys (Native tier) |
| Linux X11 | cursor + clicks + keys (Native tier) |
| Linux Wayland | cursor from OBS/PipeWire; keys via evdev (evdev tier); if `evdev` is unavailable, install the udev rule (`data/udev/99-topscreen.rules`) |

To test the overlay without an input backend: set `TOPSCREEN_DEBUG_KEY=Ctrl`.

## 5. If something does not work

- **Filter passes straight through (no effect)**: the `.effect` did not load —
  check the log (`failed to create the effect`). Confirm that
  `data/effects/topscreen.effect` is in the module data directory.
- **Dock missing**: build without Qt6/obs-frontend-api (the configure prints
  `TopScreen: Qt=... frontend=...`).
- **No keys on Wayland**: permission on `/dev/input` (udev rule/input group).
- **Overlay text skewed**: libobs matrix system — report it with a screenshot.

## 6. Release archives

Before attaching a package to GitHub, run:

```bash
./scripts/verify-release.sh
```

It checks the archives, binary version string, architecture, required data,
locale keys, Windows launchers and byte identity with `build/topscreen.so/.dll`.

## 7. Reporting

Include: OBS version, OS/graphics server, log excerpt (`--verbose`),
and which table item failed.
