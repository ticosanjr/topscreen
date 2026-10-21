# Tests

## Three verification layers

1. **Core tests** (`./tests/run.sh`) — pure logic, runs anywhere.
2. **Check against the OBS headers** (`./tools/check-obs-headers.sh`) —
   compiles every plugin source (37 files) against the **real** OBS headers (via
   `-fsyntax-only`, without libobs installed). It catches API/signature/type
   errors that the core tests do not. The dock (`src/ui`) is checked against real
   Qt6 when available, or the stub in `tools/qt-stub`. Runs in CI
   (`.github/workflows/check-obs-headers.yml`).
3. **Integration with a fake libobs** (`./tests/run-integration.sh`) — links the
   real plugin sources against a minimal `libobs` implementation
   (`tests/harness/fake_libobs.*`) and drives the filter end to end: create,
   tick, render, hotkeys, presets, profiles and settings. Runs in
   `./tests/run-all.sh` along with layers 1 and 2.

## How to run

**Layer 1 — core (no cmake/libobs):**

```bash
./tests/run.sh
```

**Layer 2 — OBS headers (downloads the headers automatically):**

```bash
./tools/check-obs-headers.sh
# X11 present? the X11 backend is checked too; otherwise it is skipped.
```

**Layer 3 — integration with the fake libobs:**

```bash
./tests/run-integration.sh
```

**Everything at once:**

```bash
./tests/run-all.sh
```

**Via CMake/CTest:**

```bash
cmake -S . -B build -DTOPSCREEN_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

**CI:** `.github/workflows/tests.yml` (core on Linux+Windows + **ASan/UBSan**)
and `check-obs-headers.yml` (Linux, with X11; OBS header check + integration,
including **ASan/UBSan**) on every push/PR.

## What is tested

The tests cover the **pure core** (`src/core/`), which does not depend on OBS.
That is where all the math and the state logic live — what actually must be right.

| File | Target | Coverage |
|---------|------|-----------|
| `test_color.cpp` | `core/color.hpp` | ABGR unpack/pack roundtrip, interpolation and clipping |
| `test_easing.cpp` | `core/easing.*` | extremes, domain clamp, monotonicity, symmetry |
| `test_spring.cpp` | `core/spring.*` | convergence, no overshoot, reset, stability with large dt |
| `test_geometry.cpp` | `core/geometry.*` | rounded-rectangle SDF, AA mask, zoom transform, pixel→content, focus clamp |
| `test_coord.cpp` | `core/coord.*` | screen↔content mapping, monitor bounds, invalid cases |
| `test_cursor.cpp` | `core/cursor.*` | initial position, target pursuit, idle/fade, loop, disabled |
| `test_click_fx.cpp` | `core/click_fx.*` | spawn, progress, expiry, distinct L/R, slot recycling, saturation |
| `test_zoom.cpp` | `core/zoom.*` | manual step and limits, auto with recent focus, return on idle, off mode, focus clamp |
| `test_json.cpp` | `core/json.*` | object/array/nested, numbers, escapes (`\uXXXX`), errors, **depth and size limits** (user-provided profile files) |
| `test_file.cpp` | `core/file.hpp` | reading a file with a **UTF-8 name** (accents) through the native path helper |
| `test_preset.cpp` | `core/preset.*` | parse + apply, missing fields kept, errors, **real files** `data/presets/*.json` |
| `test_shader_params.cpp` | `data/effects/topscreen.effect` | presence of the uniforms the compositor looks up by name + `Draw` technique (catches silent divergence); **line slots**: every consecutive `line_p` pair has its own segment and `line_count`/`line_width` are declared (the CPU decimates to those 16 points) |
| `test_preset_all_keys.cpp` | `core/preset.*` | all config keys are mapped (guarantees a complete preset) |
| `test_locale_keys.cpp` | `data/locale/*.ini` | every key used in `obs_module_text()` exists in `en-US.ini`; no orphan keys |
| `test_profiles.cpp` | `core/profiles.*` | distinct A–J seeds (colors + zoom), labels, index wrap |
| `test_profiles_io.cpp` | `core/profiles_io.*` | backup format validation: full bundle, marker/plugin, missing `current` defaults to 0, missing/empty slots accepted, slot object type, malformed JSON, extra keys accepted |
| `test_wallpapers.cpp` | `src/ts-wallpapers.*` | label/path/stem resolution, case-insensitivity, all 42 built-ins, transparent sentinel, invalid input |
| `test_key_overlay.cpp` | `core/key_overlay.*` | key queue, TTL, repeat, modifier filter, special-key ignore mode, limit, combo formatting, inline `KeyLabel` truncation at the renderer's 63-byte limit |
| `test_wav.cpp` | `core/wav.*` | PCM 8/16/24/32 and float, stereo, errors, and **real files** `data/sounds/*.wav`; the three real mouse recordings must be short (<150 ms) with the transient in the first 25 ms |
| `test_sound_catalog.cpp` | `core/sound_catalog.hpp` + `data/sounds/` | 41 bundled entries, unique labels/paths, no forbidden source words, every WAV parses as mono 44.1 kHz |
| `test_font_atlas.cpp` | `core/font_atlas.*` | atlas generated from the **real TTF**, per-glyph metrics, alpha, invalid cases, ASCII fast index equivalent to the ordered lookup |
| `test_evdev.cpp` | `core/evdev.*` | `input_event` decoding, labels, shifted punctuation (`!@#$%^&*()` and symbols), modifiers, partial records |
| `test_highlighter.cpp` | `core/highlighter.*` | drag, normalized rectangle, hold/fade, small drag, disabled |
| `test_highlight_modes.cpp` | `core/highlighter.*`, `core/zoom.*` | trail dedupe/cap, line decimation into the shader slots (first/last kept, monotonic), a pinned predicate matrix for all 6 drag modes (box/line/zoom/press-zoom/box+zoom/line+zoom), zoom request timing, press-only zoom (delay, ignore drag, quick click does nothing), hold-to-release glide, stroke bounding box as the zoom selection for the line modes, box+zoom and line+zoom draw *and* zoom |
| `test_docs.cpp` | `docs/` × code | every key in `keys.hpp` is documented; hotkeys from the code are in the reference; `CMakeLists.txt` and `buildspec.json` versions agree |
| `test_cursor_catalog.cpp` | `core/cursor_catalog.hpp` + `data/locale/en-US.ini` | one entry per `CursorStyle` (37 built-in + 69 custom), families contiguous and ordered, unique label keys present in the locale, no forbidden source words, style clamping |
| `test_hotkeys.cpp` | `hotkeys/ts-hotkey-actions.hpp` + locale | every id starts with `TopScreen_`, no duplicates, every displayed description starts with the literal id, raw `Hotkey.*` fallback becomes the literal id |
| `test_cursor_atlas.cpp` | `core/cursor_atlas.*` + `data/cursors/` | TSAT v1/v2, row-major grid, hotspots and **real atlas** (106 styles = `CursorStyle::Count`); built-in link hands plus every custom cell must be populated |
| `test_image.cpp` | `core/image.*` + `data/wallpapers/` | image decode (reads a **real wallpaper** 1920x1080), invalid cases and the **decompression-bomb pixel cap** |
| `test_monitor.cpp` (integration: manual origin wins over the env) | `core/monitor.*` + `core/coord.*` | monitor pick (hint/size/ambiguity) and global→content mapping with the capture **below/right/left/above**, including the old broken case |
| `test_mapping.cpp` | `core/mapping.*` | content area (padding/aspect), forward zoom and **consistency with the shader** (proves the cursor lands on the right pixel at any zoom) |

## Current numbers

```
201 tests | 5162 checks | 0 failures   (core)
40 tests | 503 checks   | 0 failures   (integration with fake libobs)
```

## What the integration layer covers

`tests/integration/` runs the real `ts-filter.cpp`, `ts-settings.cpp`,
`ts-properties.cpp`, `ts-presets.cpp`, input, render and hotkey code against the
fake libobs:

| File | Coverage |
|------|----------|
| `test_integration.cpp` | filter lifecycle (create/tick/render/destroy), preset apply, background toggle, all 19 hotkeys, native-cursor shutdown/restore on the parent, profiles (save/switch/seeds/list/auto-save/restart/export-import round-trip, **click-sound reload on switch**, **partial import fills missing fields/slots**), **independent hotkey highlight toggle (manual zoom/drag ownership)**, **Properties refresh after profile/preset changes**, **dock target selection (hotkeys follow it)** and **debounced auto-save with flush** |
| `test_integration.cpp` hotkeys | `integration_registers_all_hotkeys` covers every hotkey id/name/callback; `integration_hotkeys_shared_between_filters` proves one process-wide set shared across filters; `integration_registry_target_selection` proves selection routing/fallback |
| `test_uniforms.cpp` | every uniform declared by `topscreen.effect` is uploaded on a plain frame (D3D11 aborts the draw otherwise) |
| `test_text_render.cpp` | key-overlay text batches all glyphs + shadow quads into one vertex buffer (**1 draw total**, cache hit without rebuild) and falls back to per-glyph sprites if vertex-buffer creation fails |
| `test_settings.cpp` | defaults match `Config`, legacy source falls back, store/load roundtrip, partial-slot fallback, migrations, properties tree/zoom-group visibility with the hotkey, input without a backend, click sound error handling |

## What is NOT covered (and why)

These items depend on a running `libobs`/GPU/OS and do not run here:

| Item | Why not tested here | How to verify |
|------|--------------------------|----------------|
| Shader `topscreen.effect` | needs an OpenGL/D3D context from OBS | load the filter in OBS and check visually |
| `obs_source_info` integration | needs a running OBS | `obs --verbose` and check the TopScreen log |
| Input backends | need real WinAPI/X11 | test cursor/clicks in OBS |
| Qt dock | **compiles** (stub/real Qt), but the runtime needs OBS | open the dock in OBS |
| Click sound | plays via system audio (the OBS channel comes in F6) | test click with sound in OBS |

> The plugin's **compilation** against the OBS headers is checked (layer 2);
> what is missing is the **execution** inside OBS.

The strategy is: **push complexity into the testable core** and keep the OBS
layer thin.

## Adding tests

1. Create `tests/test_<subject>.cpp`.
2. Use the macros from `tests/test_framework.hpp` (`TEST`, `CHECK`, `CHECK_NEAR`,
   `CHECK_EQ`, `REQUIRE`).
3. Run `./tests/run.sh`. CMake picks up the file automatically (`GLOB`).
