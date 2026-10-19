# Tests

## Three verification layers

1. **Core tests** (`./tests/run.sh`) — pure logic, runs anywhere.
2. **Check against the OBS headers** (`./tools/check-obs-headers.sh`) —
   compiles every plugin source (34 files) against the **real** OBS headers (via
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

**CI:** `.github/workflows/tests.yml` (core, Linux+Windows) and
`check-obs-headers.yml` (Linux, with X11) on every push/PR.

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
| `test_json.cpp` | `core/json.*` | object/array/nested, numbers, escapes (`\uXXXX`), errors |
| `test_preset.cpp` | `core/preset.*` | parse + apply, missing fields kept, errors, **real files** `data/presets/*.json` |
| `test_shader_params.cpp` | `data/effects/topscreen.effect` | presence of the uniforms the compositor looks up by name + `Draw` technique (catches silent divergence) |
| `test_preset_all_keys.cpp` | `core/preset.*` | all config keys are mapped (guarantees a complete preset) |
| `test_locale_keys.cpp` | `data/locale/*.ini` | every key used in `obs_module_text()` exists in `en-US.ini`; no orphan keys |
| `test_profiles.cpp` | `core/profiles.*` | distinct A–J seeds (colors + zoom), labels, index wrap |
| `test_wallpapers.cpp` | `src/ts-wallpapers.*` | label/path/stem resolution, case-insensitivity, all 24 built-ins, invalid input |

| `test_key_overlay.cpp` | `core/key_overlay.*` | key queue, TTL, repeat, modifier filter, limit, combo formatting |
| `test_wav.cpp` | `core/wav.*` | PCM 8/16/24/32 and float, stereo, errors, and **real files** `data/sounds/*.wav` |
| `test_font_atlas.cpp` | `core/font_atlas.*` | atlas generated from the **real TTF**, per-glyph metrics, alpha, invalid cases |
| `test_evdev.cpp` | `core/evdev.*` | `input_event` decoding, labels, modifiers, partial records |
| `test_highlighter.cpp` | `core/highlighter.*` | drag, normalized rectangle, hold/fade, small drag, disabled |
| `test_docs.cpp` | `docs/` × code | every key in `keys.hpp` is documented; hotkeys from the code are in the reference |
| `test_cursor_atlas.cpp` | `core/cursor_atlas.*` + `data/cursors/` | TSAT format, hotspots and **real atlas** (16 styles = `CursorStyle::Count`) |
| `test_image.cpp` | `core/image.*` + `data/wallpapers/` | image decode (reads a **real wallpaper** 1920x1080) and invalid cases |
| `test_mapping.cpp` | `core/mapping.*` | content area (padding/aspect), forward zoom and **consistency with the shader** (proves the cursor lands on the right pixel at any zoom) |

## Current numbers

```
127 tests | 2433 checks | 0 failures   (core)
22 tests | 290 checks   | 0 failures   (integration with fake libobs)
```

## What the integration layer covers

`tests/integration/` runs the real `ts-filter.cpp`, `ts-settings.cpp`,
`ts-properties.cpp`, `ts-presets.cpp`, input, render and hotkey code against the
fake libobs:

| File | Coverage |
|------|----------|
| `test_integration.cpp` | filter lifecycle (create/tick/render/destroy), preset apply, background toggle, all 18 hotkeys, native-cursor shutdown on the parent, profiles (save/switch/seeds/list/auto-save/restart) |
| `test_integration.cpp` new: `integration_registers_all_hotkeys` covers every hotkey id/name/callback |
| `test_uniforms.cpp` | every uniform declared by `topscreen.effect` is uploaded on a plain frame (D3D11 aborts the draw otherwise) |
| `test_settings.cpp` | defaults match `Config`, legacy source falls back, store/load roundtrip, migrations, properties tree, input without a backend, click sound error handling |

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
