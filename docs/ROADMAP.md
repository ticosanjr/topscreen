# Roadmap

Each phase delivers something usable and has a verifiable acceptance criterion.

> **Verification:** the core logic is covered by automated tests
> (`docs/TESTS.md`). The shader and the OBS integration still need a pass on a
> real OBS — marked with 🟡 even when implemented.

| Phase | Delivery | Acceptance criterion | Status |
|------|---------|--------------------|--------|
| **F0** | Plugin skeleton: module loads, passthrough filter, Win/Linux CMake | Loads in portable OBS without crashing; the filter does not change the image | ✅ |
| **F1** | Background (color/gradient), rounded corners, padding, shadow, properties, presets + initial dock | "Nice" look live, < 1 ms GPU on the default preset | ✅ |
| **F2** | Cursor: style, size, shadow, smoothing, sway, idle/loop | Native cursor hidden; own cursor follows smoothly | ✅ |
| **F3** | L/R clicks (effects + color/size/duration/curve), sound, hotkeys, presets | Hotkeys in the OBS tab; L and R distinct; presets by hotkey | ✅ |
| **F4** | Zoom/Pan: spring, auto-zoom (click/cursor), manual zoom, light pan | Adjustable curve/duration/speed/idle; no "jump" | ✅ |
| **F5** | Key overlay, spotlight, drag highlighter, webcam bubble | Sharp text at any scale; clean masked camera bubble | ✅ |
| **F6** | Own capture source + own audio | Choose the screen in the TopScreen UI; audio through the source channel | 🟡 (Linux/X11 source; Windows DXGI + Wayland PipeWire pending) |
| **F7** | Wayland parity: `GlobalShortcuts` portal + evdev, CI 2 OBS versions, polish | Input matrix green on X11 and Wayland | ⬜ |

### Detail of what is implemented vs verified

| Phase | Implemented | Verified by tests | Verified in OBS |
|------|--------------|-----------------------|-------------------|
| F0 | yes | — | yes |
| F1 | yes | yes (core) | yes |
| F2 | yes | yes (core) | yes |
| F3 | yes (sound via the TopScreen Click Sound OBS source) | yes (core) | yes |
| F4 | yes | yes (core) | yes |
| F5 | yes (text, spotlight, highlighter, webcam source/filter, device frames) | yes (core + integration) | yes (smoke) |

## Cross-cutting goals

- 0.5–1.5 ms of GPU/frame @1080p60 on the default preset.
- < 1% idle CPU.
- No allocations on the hot render path.
- No frame readback.

## Quality milestones

- **M1 (after F3)**: plugin usable end to end for "nice" live capture.
- **M2 (after F5)**: feature parity with the live effects list.
- **M3 (after F7)**: Windows/X11/Wayland platform parity.
