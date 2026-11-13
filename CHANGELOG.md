# Changelog

User-visible changes. Dates are in ISO form. The released packages are listed in
[`releases/README.md`](releases/README.md).

## Unreleased

- **TopScreen Capture:** native cursor compositing (X11/XFixes) with an
  in-memory override so hiding the cursor for the custom overlay is never
  persisted in the scene.
- Prefer real monitor geometry for the manual origin and `TOPSCREEN_MONITOR_ORIGIN`;
  add an XRandR fallback when Xinerama reports one combined screen.
- Capture at most once per tick, reusing the texture when OBS renders preview
  and program in the same frame.
- Pinned highlights without a zoom now freeze the view too.
- Hotkey highlight, manual zoom and spotlight/magnifier exit a pinned Reading.
- Pointer mapping centralized in `core::map_pointer()` with tests for monitor
  offsets, inverse zoom and edge clamping.
- TopScreen Capture can output the click-sound bus on its own audio channel
  (off by default, so the dedicated click source remains the safe default).

## 1.2.6

- **Reading highlight:** dim outside a dragged selection and fit the zoom;
  keep it pinned until the next left click (which is swallowed).
- **Keep until the next click** option for Box, Line, Zoom, Box+Zoom and
  Line+Zoom.
- Display/source coordinate split, inverse zoom and real monitor geometry;
  fixes cursor/selection drift mostly seen with zoom active.
- Separate spotlight mask and zoom focus.
- TopScreen Capture reports its monitor index (identical monitors).

## 1.2.5

- Experimental **TopScreen Capture** source on Linux/X11 (XShm) with monitor
  selection and an attached effects filter.
- Scene profiles by UUID, startup apply and profile-backup export/import.
- Smart Zoom foreground-window toggle and stronger X11/Windows detection.
- Webcam bubble source action, draggable bubble and shader device frames.
- Stream Deck/OBS WebSocket action guide.

## 1.2.4

- Smart Zoom, cursor smoothing, motion blur, blurred-screen/bezel backgrounds,
  webcam bubble filter, click-audio source and scene-profile actions.
