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
| 2c | Wallpaper diffusion | The wallpaper is prefiltered by a worker when the image, blur amount, or target size changes; the Soft filter uses one shader sample and the old texture remains visible until the new one is ready |
| 3 | Cursor | native cursor disappears; own arrow appears (light with a dark outline) |
| 4 | Cursor size/rotation | changes with the sliders |
| 5 | Left click | ripple/orb effect at the click point |
| 6 | Right click | effect distinct from the left |
| 7 | Click sound | sound on click through the `TopScreen Click Sound` OBS source; **Add click sound to the current scene** creates the source |
| 8 | Automatic zoom | on click, zooms and returns after stopping |
| 9 | Manual zoom (hotkeys) | in/out/reset work |
| 11 | Key overlay | keys appear at the bottom and disappear after the TTL |
| 12 | Spotlight | `TopScreen_ToggleSpotlight` fades/zooms in/out at the cursor; circle/square; border, Dim/Blur outside; follows the mouse |
| 12b | Magnifier | `TopScreen_ToggleMagnifier` zooms only inside the lens; circle/square; border/shadow; follows cursor or fixed center; Spotlight and Magnifier are mutually exclusive |
| 13 | Highlighter | dragging with the left button marks the region; `TopScreen_ToggleHighlight` zooms at the cursor on the first press and returns on the second; manual zoom/drag cancels the hotkey toggle; **Keep until the next click** pins marks/zoom |
| 13b | Reading | drag a rectangle, release and check the outside dims + selection is fitted/zoomed; moving the mouse does not move the scene; the next left click exits without ripple/sound; scene/profile/effects-off also exit |
| 13c | Mouse alignment | with zoom, spotlight and padding active, the cursor overlay, click and highlight must stay under the physical pointer; repeat on a second monitor, identical monitors and a scaled capture |
| 14 | Presets (hotkey) | next/previous switch the look; switching profile discards it; **Save profile** keeps it |
| 15 | Profiles | switching A–J also reloads the click sounds (test A vs. D with different WAVs) |
| 15b | Scene profiles | assign profile B to a scene, switch scenes, and check that B is applied; restart OBS on that scene and check it applies on startup; rename the scene and check the assignment survives |
| 15c | Smart Zoom active window | in Smart mode, focus another window and check the zoom follows it; turn **Focus the foreground window** off and check the zoom only reacts to actions/cursor |
| 15d | Webcam bubble | **Add webcam bubble source** creates a draggable bottom-right bubble; a second click reuses it; configure the camera in the source properties |
| 15e | Device frames | select Browser/MacBook/iPhone and check the frame is drawn around the captured screen (add padding if the base/notch is clipped) |
| 15f | TopScreen Capture (Linux X11) | add **TopScreen Capture**, pick the monitor, confirm the scene shows the screen with effects applied directly by the source (no child filter); reload OBS and confirm there is only one source and no filter; turn effects off and confirm the native cursor returns (**Include native cursor**); enable **Output TopScreen click sounds from this source** and confirm clicks appear on the source's audio meter |
| 15g | Properties groups | Screen, Zoom and highlight, Cursor and clicks, Spotlight and magnifier, Keys and webcam; the master switch is first and the version is last; each group checkbox hides/shows its section |
| 15h | Preset is temporary | apply a preset, switch to another profile and back: the original profile returns; apply a preset and press **Save profile**: it persists after a restart |
| 15i | Color transparency | edit the highlight color and confirm the look does not jump (alpha folded into opacity); edit the key band color and confirm the alpha picker preserves transparency |
| 15j | Keys privacy | a new install starts with profile A and **Keystrokes** off; enabling it shows the warning; **Modifiers only** hides the warning |
| 15k | Profile names/copy/revert | rename A, copy it to another slot, switch profiles and check the names; edit a slider, **Revert changes** and confirm the activation state returns |
| 15l | Keys pause | with Keystrokes on, click **Pause keystrokes** (or press `TopScreen_ToggleKeys`) and confirm the overlay stops without changing the saved setting; press again to resume |
| 15m | Destructive confirmation | in Properties, importing or restoring without ticking the confirmation does nothing; with it ticked the action runs; the dock asks in a dialog |
| 15n | Dock panel | sections Live/Look/Scene/Sources/Backup/Diagnostics; target shows `Source › Filter`; status shows input, monitor origin and keys; **Copy diagnostics** copies the text |
| 15o | User looks | **Save look...** creates a file in the looks folder and adds it to the selector/hotkey cycle; **Delete look** removes it; bundled looks cannot be deleted; **Open looks folder** shows the single-look files |
| 15p | Shortcuts-only keys | enable **Only show shortcuts** and confirm plain letters/Shift+letter and standalone modifiers are hidden while Ctrl/Alt/Win shortcuts appear; **Modifiers only** remains the stricter alternative |
| 15q | Click sound preview | **Test left/right click** in Properties and the dock plays the selected WAV immediately |
| 15r | Scene switching toggle | uncheck **Switch profile when the scene changes**, switch scenes and confirm the current profile is kept; re-enable and confirm assignments still apply |

| 16 | Profile import | an older/partial `.json` imports with defaults instead of failing |
| 17 | Effects off / cursor off / filter removed | the capture's native cursor comes back; with two TopScreen filters on the same capture it only returns after the last one releases it |
| 18 | Dock | reflects the state and shows the input tier |

## 3. Hotkeys

**Settings → Hotkeys → TopScreen** must list the 26 actions
(`ToggleEffects`, `ToggleBackground`, `ZoomIn`, `ZoomOut`, `ResetZoom`,
`NextPreset`, `PrevPreset`, `ProfileA` … `ProfileJ`, `ProfileNext`,
`ProfilePrev`, `ProfileSave`, `ToggleClickSound`, `ToggleCursor`,
`ToggleHighlight`, `ToggleSpotlight`, `ToggleMagnifier`, `ToggleKeys`).

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
