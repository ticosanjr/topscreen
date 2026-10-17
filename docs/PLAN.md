# TopScreen — Plan

> Master document. It gathers the goal, decisions, scope and criteria.

## 1. Goal

Produce a **live video capture object** from screens, with visual effects, that
is consumed **exclusively by OBS** as a source/scene. It does not record, does
not edit, does not export. All effects are **automatic or by hotkey**.

### Non-negotiable constraints

- **OBS only**: delivered as a native OBS source.
- **Live**: no post-production effects (no timeline, no cutting, no manual keyframe).
- **Light**: runs on modest hardware; GPU, zero-copy, no encode.
- **Portable**: distributed as a plugin file for portable OBS.
- **Windows + Linux** with parity (X11 and Wayland).
- **Hotkeys** easy to configure, in OBS's format.

## 2. Architecture decision

| Option | Verdict |
|-------|----------|
| **A. Native OBS plugin (libobs)** | ✅ **Chosen** |
| B. Standalone app + GPU bridge | Rejected (2 parts, more risk) |
| C. Standalone app + NDI | Rejected (heavy, external runtime, non-portable) |
| D. Standalone app + Spout2 | Rejected (Windows only) |

**Why A**: it becomes a native source (no IPC), processes on the GPU inside the
OBS process, ships as a `.dll`/`.so`, gets the native Properties and Hotkeys UI,
and `libobs` abstracts D3D11/OpenGL — the same code on Windows and Linux.

**Initial form**: a **filter** (`OBS_SOURCE_TYPE_FILTER`) applied over an OBS
Display Capture — it reuses OBS's tested capture and delivers value fast. The
**own capture source** comes in F6.

**License**: GPL-2.0-or-later (plugins that link `libobs`). Open project.

## 3. Product decisions

| Topic | Decision |
|------|---------|
| Configuration UI | Native OBS Properties **+ Qt dock** (presets/toggles) |
| Input on Wayland | **Tiers 2+3**: hotkeys via the `GlobalShortcuts` portal + clicks/keys via opt-in `evdev` |
| License | GPL-2.0-or-later |
| Linux target | X11 and Wayland as first-class citizens |
| Preview | The OBS canvas (the dock is control, not preview) |

## 4. Effect scope

**Only** live-viable effects get in. The full list and the rationale for each one
is in [`EFFECTS.md`](EFFECTS.md).

**In**: background/corners/shadow/padding, cursor (style/size/shadow/sway/
idle/loop), L/R click (orb/ripple/ring/force + color/size/duration/curve),
click sound, automatic zoom (click/cursor) and manual, light pan, motion blur,
key overlay (SDF), spotlight, webcam bubble, presets by hotkey.

**Out**: timeline, cuts, speed ramps, manual keyframes, export/
transcode, audio normalization, real text highlight via accessibility
(replaced by the drag highlighter).

## 5. "Automatic vs hotkey" model

Each feature has a **mode**:

- `off` — disabled
- `auto` — reacts on its own (e.g. zoom follows the click)
- `hotkey` — only triggers by hotkey
- `always` — always on (e.g. background)

This directly answers the requirement "either automatic or configurable by hotkey".

## 6. Quality goals

- **GPU**: 0.5–1.5 ms/frame at 1080p60 for the default set of effects.
- **Idle CPU**: < 1%.
- **No per-frame allocations** on the render path.
- **No frame readback** to the CPU.

## 7. Risks

| Risk | Mitigation |
|-------|-----------|
| Global hotkeys on Wayland | `GlobalShortcuts` portal (Tier 2) + WebSocket fallback |
| Passive input on Wayland | opt-in `evdev` (Tier 3) + graceful degradation |
| wlroots without a hotkey portal | Fallback to use with OBS focused / WebSocket |
| OBS ABI across versions | CI against 2 OBS versions |
| DPI/multi-monitor | Global→canvas mapping centralized in the input module |
| `evdev` permission | Document udev `uaccess` as the recommended path |
