# Architecture

## Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                                                                     │
│  Display Capture                   ┌────── TopScreen Engine ──────┐ │
│  (choose screen) ─► TopScreen FX ─►│  Input      (cursor/clicks)  │ │
│                                    │  Dynamics   (spring/zoom/pan)│ │
│  TopScreen Capture (X11/XShm) ────►│  Compositor (GPU shaders)    │ │
│  (effects embedded in the source)  │  Settings   (props/presets)  │ │
│                                    │  Hotkeys    (OBS API)        │ │
│                                    └────────────────┬─────────────┘ │
│                                                     ▼               │
│                    OBS scene (processed source)                     │
└─────────────────────────────────────────────────────────────────────┘
```

The filter receives the already-rendered frame from the parent source
(`obs_source_process_filter_*`). The experimental TopScreen Capture source owns
the texture instead: it draws the captured frame through the same shared engine
(`src/ts-engine.hpp`) and composition effect, so no child filter is created.
Nothing leaves the OBS process.

## Modules

The code is split into a **pure core** (`src/core/`, with no OBS dependency and
covered by tests) and the **OBS layer** (thin, it only wires the core to libobs).

### Pure core (`src/core/`) — testable

| Module | File | Responsibility |
|--------|---------|------------------|
| Types/color | `types.hpp`, `color.hpp` | Stable enums, ABGR↔RGBA, interpolation |
| Easing | `easing.{hpp,cpp}` | Animation curves |
| Spring | `spring.{hpp,cpp}` | Critically damped spring (with stable substeps) |
| Geometry | `geometry.{hpp,cpp}` | Rounded-corner SDF, mask, zoom transform, focus clamp |
| Coordinates | `coord.{hpp,cpp}` | Screen↔content mapping, monitor |
| Cursor | `cursor.{hpp,cpp}` | Exact/A→B glide, rotation offset, idle fade |
| Clicks | `click_fx.{hpp,cpp}` | Queue of L/R click animations |
| Zoom | `zoom.{hpp,cpp}` | Auto/manual, pan, idle timeout |
| JSON | `json.{hpp,cpp}` | Dependency-free JSON parser |
| Preset | `preset.{hpp,cpp}` | Preset parse + apply |
| Config | `config.hpp`, `config_limits.hpp`, `keys.hpp` | `Config`, shared UI/sanitize ranges and key names |

### OBS layer

| Module | File | Responsibility |
|--------|---------|------------------|
| Plugin | `src/plugin-main.cpp` | `OBS_DECLARE_MODULE`, source registration, dock |
| Filter | `src/ts-filter.cpp` | `obs_source_info` for the filter (thin wrapper over the shared engine) |
| Parent cursor | `src/ts-parent-cursor.{hpp,cpp}` | Process-wide native-cursor lease shared by filters |
| Monitor origin | `src/ts-monitor-origin.{hpp,cpp}` | Global desktop offset of the captured monitor |
| Settings | `src/ts-settings.{hpp,cpp}` | obs_data ↔ `core::Config` mapping |
| Presets | `src/ts-presets.{hpp,cpp}` | Loads `data/presets/*.json` |
| Compositor | `src/render/ts-compositor.{hpp,cpp}` | Shader effect + parameter upload |
| Frame | `src/render/ts-frame.hpp` | Per-frame state (cursor/zoom/clicks) |
| Input | `src/input/ts-input.*` + `ts-input-{win,linux,none}.cpp` | Cursor/button/key state (one backend per platform) |
| Active window | `src/input/ts-active-window.*` | Foreground window rect for Smart Zoom (Windows/X11) |
| Hotkeys | `src/hotkeys/ts-hotkeys.{hpp,cpp}` | Hotkey registration + callbacks |
| Text | `src/render/ts-text.{hpp,cpp}` + `src/core/font_atlas.*` | Key overlay (font atlas + band) |
| Profiles | `src/core/profiles.{hpp,cpp}` + `ts-profile-store.*` + `ts-profile-bundle.*` + `ts-scene-profiles.*` | A–J seeds, slot store, export/import bundle, scene mappings |
| Audio | `src/audio/ts-click-sound.*` + `ts-click-audio.*` + `ts-click-source.*` | Click WAV decode, process-wide mix queue and OBS audio source |
| Engine | `src/ts-engine.{hpp,cpp}` + `ts-engine-{profiles,actions,pointer,render}.cpp` + `ts-engine-internal.hpp` | Shared effect host: settings, profiles, hotkeys, input models, compositor and per-frame state (filter and capture source) |
| Capture | `src/capture/ts-capture.*` + `ts-capture-x11.cpp` + `ts-capture-source.cpp` | Native monitor capture source (X11/XShm) that draws its texture through the shared engine |
| Properties | `src/ts-properties.cpp` + `src/properties/*` + `ts-option-schema.hpp` | OBS properties tree, split by capture/pointer/focus/overlays/profiles; one option registry feeds tooltips/tests |
| Webcam | `src/webcam/ts-webcam-filter.*` + `ts-webcam-source.*` + `data/effects/webcam.effect` | Independent webcam bubble filter and source action |
| UI | `src/ui/ts-dock.{hpp,cpp}` | Qt dock (optional) |

## Render pipeline

Pass order (cheap → expensive when possible):

```
OBS frame (GPU texture)
 1. Background      → color/gradient/image + padding
 2. Rounded corners → SDF mask with anti-aliasing
 3. Shadow / inset  → mip pyramid (cheap blur)
 4. Zoom / Pan      → UV transform (spring)  ← affects 1..3 together
 5. Cursor          → scalable sprite (+shadow, +rotation)
 6. Click FX        → ripple/orb/ring (L and R independent)
 8. Keystroke HUD   → SDF text
 9. Spotlight / Dim → optional single pass
```

In the current implementation (F1), passes 1–4 are in the shader
`data/effects/topscreen.effect`. The others come in the following phases.
The capture source draws the same effect directly (`Compositor::render_texture`,
one `gs_draw_sprite` per pass) instead of going through the filter API.

### Why a single `.effect` in F1

- Fewer state transitions on the GPU (less per-frame overhead).
- Easy to maintain and portable (OBS compiles the same file for HLSL/D3D11 and GLSL/OpenGL).
- When the cost justifies it, the passes are split into separate `.effect` files.

## Input data flow

```
[OS] cursor/buttons/keys
   │
   ▼
ts-input (poll per tick)  ──►  ts_InputState {x,y,left,right,edges}
   │
   ├─► Cursor   (position → sprite)
   ├─► Click FX (edge → new animation)
   ├─► Auto-zoom(edge/position → focus)
   └─► Keys     (events → HUD)
```

Global coordinates → canvas coordinates are resolved in the input module (F2),
taking into account monitor position/scale and DPI.

## Persistence

- Filter/source settings: `obs_data` (JSON) managed by OBS per source/scene.
  TopScreen Capture stores the same effect keys plus its `capture_*` keys.
- Profile slots store only the fields that differ from their A-J seed; a default
  slot is `{}`, and older full slots are compacted once on load.
- TopScreen presets: files in `data/presets/*.json`, applicable by hotkey.
- Hotkeys: managed by OBS (Hotkeys tab) and, on Wayland, mirrored in the portal.

## Graphics composition

- Graphics backend: `gs_*` from `libobs` (D3D11 on Windows, OpenGL on Linux).
- Shaders: `.effect` (HLSL-like syntax, compiled for the active backend).
- No custom draw calls outside `obs_source_process_filter_*`; the filter uses
  `OBS_NO_DIRECT_RENDERING` to ensure the `image` is the parent's texture.
