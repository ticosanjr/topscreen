# Tasks

Executable breakdown per phase. IDs in the format `T<phase>.<n>`.
Status: ✅ done · 🟡 partial · ⬜ pending.

> "Done" = implemented. Where the item is only verifiable in OBS (shader/integration),
> see [`TESTS.md`](TESTS.md) for what still needs validation at runtime.

---

## F0 — Plugin skeleton

| ID | Task | Status |
|----|--------|--------|
| T0.1 | Repository structure, `CMakeLists.txt`, `.gitignore`, `.clang-format` | ✅ |
| T0.2 | `plugin-main.cpp` with `OBS_DECLARE_MODULE` + source registration | ✅ |
| T0.3 | The filter's `obs_source_info` (create/destroy/update/render/defaults/properties) | ✅ |
| T0.4 | Shader effect (`data/effects/topscreen.effect`) | ✅ |
| T0.5 | `en-US.ini` + `pt-BR.ini` locales | ✅ |
| T0.6 | `docs/BUILD.md` with Win/Linux step by step | ✅ |

## F1 — Background, corners, padding, shadow

| ID | Task | Status |
|----|--------|--------|
| T1.1 | `ts-settings` (obs_data ↔ `core::Config`) | ✅ |
| T1.2 | `ts-properties` (groups General/Background/Cursor/Clicks/Sound/Zoom) | ✅ |
| T1.3 | `ts-compositor`: load effect + parameter cache | ✅ |
| T1.4 | Shader: solid color and gradient (angle/meeting point) backgrounds, SDF corners, padding, approx. shadow | ✅ |
| T1.5 | Zoom/pan via parameters in the shader | ✅ |
| T1.6 | Initial Qt dock (presets/toggles) | ✅ |
| T1.7 | Built-in JSON presets + loader (`ts-presets`) | ✅ |
| T1.8 | Dock: apply preset and reflect live state | ✅ |
| T1.9 | Profiles backup: export/import all 10 slots (dock + Properties fallback) | ✅ |
| T1.10 | Dock **Target filter** selector (dock buttons + hotkeys follow it) | ✅ |
| T1.11 | Debounced profile auto-save (~400 ms) with flush on switch/save/export/import/destroy | ✅ |

## F2 — Cursor

| ID | Task | Status |
|----|--------|--------|
| T2.1 | Hide/restore the parent native cursor (effects/cursor-off, destroy, shared across filters) | ✅ |
| T2.2 | Map global coordinates → canvas (DPI/multi-monitor) | ✅ |
| T2.3 | Enable/disable the filter cursor | ✅ |
| T2.4 | Scalable procedural cursor + shadow | ✅ |
| T2.5 | Motion smoothing (spring) | ✅ |
| T2.6 | Sway (tilt on turns) | removed from the UI |
| T2.7 | Hide idle cursor (loop removed) | ✅ |
| T2.8 | Cursor styles grouped by theme (29 built-in + 50 custom) | ✅ |

## F3 — Clicks, sound, hotkeys, presets

| ID | Task | Status |
|----|--------|--------|
| T3.1 | Button edges in the input + event queue | ✅ |
| T3.2 | Hotkey registration (`TopScreen_*`) + callbacks | ✅ |
| T3.3 | Left click effect (ripple/orb/ring/force) | ✅ |
| T3.4 | Independent right click effect | ✅ |
| T3.5 | Click parameters (color/size/duration/curve) in the UI | ✅ |
| T3.6 | Click sound (WAV decode + playback) | ✅ (decoded and mixed through the TopScreen Click Sound OBS source) |
| T3.7 | Presets applicable by hotkey (next/prev) | ✅ |
| T3.8 | Hotkey fallback via OBS WebSocket | ✅ (documented `TriggerHotkeyByName` path in `STREAMDECK.md`) |
| T3.9 | Scene profiles (assign/list/clear/prune, UUID + startup apply) | ✅ |
| T3.10 | Stream Deck / OBS WebSocket action guide + profile/click hotkeys | ✅ |

## F4 — Zoom / Pan

| ID | Task | Status |
|----|--------|--------|
| T4.1 | Spring + easings (`core/spring`, `core/easing`) | ✅ |
| T4.2 | Auto-zoom by click | ✅ |
| T4.3 | Auto-zoom following the cursor with timeout | ✅ |
| T4.4 | Manual zoom in/out/reset by hotkey | ✅ |
| T4.5 | Smart zoom (action-driven hold/return) | ✅ |
| T4.6 | Light pan following the mouse | ✅ |
| T4.7 | Zoom parameters in the UI | ✅ |
| T4.8 | Motion blur (cursor and zoom) | ✅ (radial blur from zoom) |
| T4.9 | Smart Zoom active-window/region focus with opt-out | ✅ (Windows/X11) |

## F5 — Text, spotlight, webcam

| ID | Task | Status |
|----|--------|--------|
| T5.0 | Key overlay model (queue/TTL/combos) | ✅ |
| T5.1 | Font atlas (TTF + stb_truetype) | ✅ |
| T5.2 | Text rendering (glyph sprites, outline) | ✅ |
| T5.3 | Key capture (Windows / X11; evdev in F7) | ✅ |
| T5.4 | Key overlay | ✅ |
| T5.5 | Spotlight/dim following the cursor (round/square, hotkey, drop/zoom, border, halo shadow, outside blur) | ✅ |
| T5.6 | Webcam bubble | ✅ (filter + add-source action, mask/border/shadow, draggable item) |
| T5.7 | Drag highlighter (box, line, zoom to selection, zoom on mouse press) | ✅ |
| T5.8 | Device frames (browser/MacBook/iPhone) | ✅ (shader-drawn) |
| T5.9 | Reading highlight (dim outside + locked zoom) and keep-until-click for other marks | ✅ |

## F6 — Own capture source + audio

| ID | Task | Status |
|----|--------|--------|
| T6.1 | Windows capture (DXGI Desktop Duplication) | ⬜ |
| T6.2 | Linux capture (PipeWire + XShm) | 🟡 (X11/XShm source; PipeWire ⬜) |
| T6.3 | Input source `obs_source_info` | ✅ (Linux/X11 `topscreen_capture`) |
| T6.4 | Screen selection in the UI | ✅ (source monitor list) |
| T6.5 | Own audio output (click sound on the channel) | ✅ (dedicated OBS audio source) |
| T6.6 | Shared effect engine embedded in the capture source | ✅ (filter and source use `ts_engine`; no child filter) |

## F7 — Wayland parity + polish

| ID | Task | Status |
|----|--------|--------|
| T7.1 | `GlobalShortcuts` portal | ⬜ |
| T7.2 | evdev backend (libevdev) | ✅ (direct /dev/input read + tested decoder) |
| T7.3 | `uaccess` udev rule + detection in the dock | ✅ (rule ready + log warning) |
| T7.4 | Graceful degradation + tier report | ✅ (readable tier in the dock + log warnings) |
| T7.5 | CI: Win + Linux build against OBS | ✅ (release workflow builds both; header check covers OBS 30.0.2 and 31.1.1) |
| T7.6 | Portable packaging + release | ✅ (packages in `releases/`, published as GitHub Release `v1.4.0`; Windows package includes the Qt dock) |
| T7.7 | Final i18n + icon + screenshots | ✅ (en-US + pt-BR; icon and screenshots in `assets/`) |
| T7.8 | Automated release archive verification (`scripts/verify-release.sh`) | ✅ |
| T7.9 | Named profiles, duplicate/revert and user looks | ✅ (names in slots/backups; looks in `plugin_config/topscreen/looks`) |
| T7.10 | Single option schema for tooltips/sections | ✅ (`ts-option-schema.hpp` + validation tests) |

## Infrastructure / documentation

| ID | Task | Status |
|----|--------|--------|
| TD.1 | Full GPL-2.0 license text (`COPYING`) | ✅ |
| TD.2 | `CONTRIBUTING.md` and PR template | ✅ |
| TD.3 | Adjust `buildspec.json`/presets for the official CI | ⬜ |
| TD.4 | Core sanity tests | ✅ (258 core + 71 integration + 57 OBS header files + ASan/UBSan) |

---

## Next steps (suggested order)

1. **F6** own capture source + audio through the source channel.
2. **F7** `GlobalShortcuts` portal (Wayland) + tier polish.
3. Runtime coverage of the Qt dock (today only via a real OBS).
