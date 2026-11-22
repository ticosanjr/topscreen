# Tests

## Three verification layers

1. **Core tests** (`./tests/run.sh`) — pure logic, runs anywhere.
2. **Check against the OBS headers** (`./tools/check-obs-headers.sh`) —
   compiles every plugin source (68 files) against the **real** OBS headers (via
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
| `test_cursor.cpp` | `core/cursor.*` | initial position, target pursuit, idle/fade, disabled, exact/A→B glide, rotation offset, primary→secondary link style |
| `test_click_fx.cpp` | `core/click_fx.*` | spawn, progress, expiry, distinct L/R, slot recycling, saturation |
| `test_zoom.cpp` | `core/zoom.*` | manual step and limits, auto with recent focus, return on idle, off mode, focus clamp, **Smart region fit/focus**, **locked spot and freeze** |
| `test_webcam.cpp` | `core/webcam.*` | mask/geometry anchors and offsets, sanitizing, **default source item layout** |
| `test_highlighter.cpp` | `core/highlighter.*` | drag box/line, hold/fade, opacity, **Reading lock**, **keep-until-click for marks and zoom** |
| `test_mapping.cpp` | `core/mapping.*` | padding/keep-aspect, cursor content output, **display ↔ source zoom inverse** |
| `test_json.cpp` | `core/json.*` | object/array/nested, numbers, escapes (`\uXXXX`), errors, **depth and size limits** (user-provided profile files) |
| `test_file.cpp` | `core/file.hpp` | reading a file with a **UTF-8 name** (accents) through the native path helper and enforcing a caller-provided size cap |
| `test_preset.cpp` | `core/preset.*` | parse + apply, missing fields kept, errors, **legacy `bg_mode` 1/2/3 migration** and `bg_type` precedence, **real files** `data/presets/*.json` use the modern background key |
| `test_config_limits.cpp` | `core/config_limits.hpp` | every UI/sanitize range is ordered with a positive step and every `Config` default sits inside its matching range |
| `test_config_sanitize.cpp` | `core/config.*` | NaN/infinity fallback, out-of-range clamping (cursor size/rotation, click effects and magnifier), angle wrapping, spotlight/magnifier exclusivity, valid values untouched, preset apply sanitizes imported data |
| `test_shader_params.cpp` | `data/effects/topscreen.effect` | presence of the uniforms the compositor looks up by name + `Draw` technique (catches silent divergence); **line slots**: every consecutive `line_p` pair has its own segment and `line_count`/`line_width` are declared (the CPU decimates to those 32 points) |
| `test_preset_all_keys.cpp` | `core/preset.*` | all config keys are mapped (guarantees a complete preset) |
| `test_locale_keys.cpp` | `data/locale/*.ini` | every key used in `obs_module_text()` exists in `en-US.ini`; no orphan keys; labels are non-empty, not raw keys and have no stray whitespace; `pt-BR.ini` mirrors the exact en-US key set |
| `test_profiles.cpp` | `core/profiles.*` | distinct A–J seeds (colors + zoom), labels, index wrap |
| `test_profiles_io.cpp` | `core/profiles_io.*` | backup format validation: full bundle, marker/plugin, missing `current` defaults to 0, missing/empty slots accepted, slot object type, malformed JSON, extra keys accepted |
| `test_wallpapers.cpp` | `src/ts-wallpapers.*` | label/path/stem resolution, case-insensitivity, all 18 built-ins, transparent sentinel, invalid input |
| `test_key_overlay.cpp` | `core/key_overlay.*` | key queue, TTL, repeat, modifier filter, special-key names, literal Space, Ctrl+key combos, limit, inline `KeyLabel` truncation at the renderer's 63-byte limit |
| `test_wav.cpp` | `core/wav.*` | PCM 8/16/24/32 and float, stereo, errors, and **real files** `data/sounds/*.wav`; the three real mouse recordings must be short (<150 ms) with the transient in the first 25 ms |
| `test_sound_catalog.cpp` | `core/sound_catalog.hpp` + `data/sounds/` | 26 bundled entries, unique labels/paths, no forbidden source words, every WAV parses as mono 44.1 kHz |
| `test_font_atlas.cpp` | `core/font_atlas.*` | atlas generated from the **real TTF**, per-glyph metrics, alpha, invalid cases, ASCII fast index equivalent to the ordered lookup |
| `test_evdev.cpp` | `core/evdev.*` | `input_event` decoding, labels, shifted punctuation (`!@#$%^&*()` and symbols), modifiers, partial records |
| `test_highlighter.cpp` | `core/highlighter.*` | drag, normalized rectangle, hold/fade, small drag, disabled |
| `test_spotlight.cpp` | `core/spotlight.*` | initial state, zoom in and out, easing, mid-transition reversal, cursor focus, clamp |
| `test_magnifier.cpp` | `core/magnifier.*` | activation/deactivation transitions, local zoom, follow cursor and fixed center |
| `test_highlight_modes.cpp` | `core/highlighter.*`, `core/zoom.*` | trail dedupe/cap, line decimation into the shader slots (first/last kept, monotonic), a pinned predicate matrix for the 5 drag modes (box/line/zoom/box+zoom/line+zoom), zoom request timing, hold-to-release glide, stroke bounding box as the zoom selection for the line modes, box+zoom and line+zoom draw *and* zoom |
| `test_docs.cpp` | `docs/` × code | every key in `keys.hpp` is documented; hotkeys from the code are in the reference; `CMakeLists.txt` and `buildspec.json` versions agree |
| `test_cursor_catalog.cpp` | `core/cursor_catalog.hpp` + locale | one entry per active `CursorStyle` (29 built-in + 50 custom), retired IDs excluded from the menu and unique, families contiguous and ordered, unique label keys present in the locale, no forbidden source words, style clamping and active retired fallbacks |
| `test_hotkeys.cpp` | `hotkeys/ts-hotkey-actions.hpp` + locale | every id starts with `TopScreen_`, no duplicates, every displayed description starts with the literal id, raw `Hotkey.*` fallback becomes the literal id |
| `test_cursor_atlas.cpp` | `core/cursor_atlas.*` + `data/cursors/` | TSAT v1/v2, row-major grid, hotspots and **real atlas** (106 cells = `CursorStyle::Count`); built-in link hands plus every active custom cell must be populated, and all retired cells transparent |
| `test_image.cpp` | `core/image.*` + `data/wallpapers/` | image decode (reads a **real wallpaper** 1920x1080), invalid cases and the **decompression-bomb pixel cap** |
| `test_image_filter.cpp` | `core/image_filter.*` | bilinear resize, max-size cap, identity no-blur path, blurred edges and alpha preservation |
| `test_monitor.cpp` (integration: manual origin wins over the env) | `core/monitor.*` + `core/coord.*` | monitor pick (hint/size/ambiguity) and global→content mapping with the capture **below/right/left/above**, including the old broken case |
| `test_mapping.cpp` | `core/mapping.*` | content area (padding/aspect), forward zoom and **consistency with the shader** (proves the cursor lands on the right pixel at any zoom) |

## Current numbers

```
263 tests | 10787 checks | 0 failures   (core)
84 tests | 1029 checks   | 0 failures   (integration with fake libobs, LeakSanitizer clean)
```

## What the integration layer covers

`tests/integration/` runs the real `ts-filter.cpp`, `ts-settings.cpp`,
`ts-properties.cpp`, `ts-presets.cpp`, input, render and hotkey code against the
fake libobs:

| File | Coverage |
|------|----------|
| `test_integration.cpp` | filter lifecycle (create/tick/render/destroy), preset apply, background toggle, all 26 hotkeys, native-cursor shutdown/restore on the parent (including effects/cursor-off and shared refcount), profiles (save/switch/seeds/list/auto-save/restart/export-import round-trip, **click-sound reload on switch**, **partial import fills missing fields/slots**), **independent hotkey highlight toggle (manual zoom/drag ownership)**, **profile names/duplicate/revert**, **user look save/apply/delete**, **temporary keystroke pause**, **shortcuts-only key mode**, **click sound preview**, **scene-switch toggle**, **temporary preset**, **lossless seed revision**, **sparse profile slots + one-time compaction**, **spotlight hotkey**, **magnifier hotkey + Spotlight/Magnifier exclusion**, **Properties refresh after profile/preset changes**, **dock target selection (hotkeys follow it)**, **debounced auto-save with flush**, **scene → profile assignment applied on scene change, on filter creation and by scene UUID (with lazy migration of legacy name entries and name fallback)**, **empty scene lists never erase assignments**, and **scene map export/import round-trip** |
| `test_integration.cpp` hotkeys | `integration_registers_all_hotkeys` covers every hotkey id/name/callback; `integration_hotkeys_shared_between_filters` proves one process-wide set shared across filters; `integration_registry_target_selection` proves selection routing/fallback |
| `test_uniforms.cpp` | every uniform declared by `topscreen.effect` is uploaded on a plain frame (D3D11 aborts the draw otherwise) |
| `test_click_audio.cpp` | click WAV is decoded, queued and mixed by the `topscreen_click_audio` source; silence when idle, PCM when a click is queued |
| `test_webcam_filter.cpp` | webcam bubble filter uploads its mask/border/shadow uniforms and exposes the mask properties |
| `test_capture_source.cpp` | `topscreen_capture` lifecycle, embedded effect properties, monitor/cursor/audio settings and **native-cursor runtime override** (the real X11 frame pull/cursor compositing is verified in OBS) |
| `test_text_render.cpp` | key-overlay text batches all glyphs + shadow quads into one vertex buffer (**1 draw total**, cache hit without rebuild) and falls back to per-glyph sprites if vertex-buffer creation fails |
| `test_option_schema.cpp` | the shared option registry is unique, every config key is registered, every tooltip exists in both locales, and every tooltip used by the Properties builders is registered |
| `test_presets.cpp` | user looks: save one JSON file, discover it on reload, apply it, delete it, and refuse to delete bundled looks |
| `test_settings.cpp` | defaults match `Config`, legacy source falls back, store/load roundtrip, partial-slot fallback, background type migration/visibility, migrations, properties tree/zoom-group visibility with the hotkey, **dependent-control visibility callbacks** (cursor idle, spotlight corner, keystroke band), input without a backend, click sound error handling |

## What is NOT covered (and why)

These items depend on a running `libobs`/GPU/OS and do not run here:

| Item | Why not tested here | How to verify |
|------|--------------------------|----------------|
| Shader `topscreen.effect` | needs an OpenGL/D3D context from OBS | load the filter in OBS and check visually |
| `topscreen_capture` (X11) | needs a real X server and OBS graphics context | add the source in OBS and check the monitor is captured; see `VERIFICATION.md` |
| `obs_source_info` integration | needs a running OBS | `obs --verbose` and check the TopScreen log |
| Input backends | need real WinAPI/X11 | test cursor/clicks in OBS |
| Qt dock | **compiles** (stub/real Qt), but the runtime needs OBS | open the dock in OBS |
| Click sound | mixes through the TopScreen Click Sound OBS source | test click with sound in OBS |

> The plugin's **compilation** against the OBS headers is checked (layer 2);
> what is missing is the **execution** inside OBS.

The strategy is: **push complexity into the testable core** and keep the OBS
layer thin.

## Adding tests

1. Create `tests/test_<subject>.cpp`.
2. Use the macros from `tests/test_framework.hpp` (`TEST`, `CHECK`, `CHECK_NEAR`,
   `CHECK_EQ`, `REQUIRE`).
3. Run `./tests/run.sh`. CMake picks up the file automatically (`GLOB`).
