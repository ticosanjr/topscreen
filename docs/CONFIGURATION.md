# Configuration reference

Values persisted by OBS (per source/scene) and used in the preset files.
The usage manual is in [`MANUAL.md`](MANUAL.md). UI labels include units and
ranges where useful; advanced controls show a longer explanation in their
tooltip.

## Colors

OBS format: integer **`0xAABBGGRR`** (low byte = red).
E.g.: `0xFF3A2E5C` = A=FF, B=3A, G=2E, R=5C.

## All options

| Group (UI) | Option (UI) | Key | Type | Range | Default |
|------------|------------|-------|------|-------|--------|
| — | Effects enabled | `effects_enabled` | bool | — | `true` |
| Background | Enable background | `bg_enabled` | bool | — | `true` |
| | Background type | `bg_type` | int | 0 solid · 1 gradient · 2 wallpaper · 3 local image · 4 transparent | `1` |
| | Legacy style (migrated) | `bg_mode` | int | old 0 solid · 1/2 gradient · 3 wallpaper | — |
| | Color 1 (solid / gradient start) | `bg_color_a` | color | — | `0xFF20242E` |
| | Color 2 (gradient end) | `bg_color_b` | color | gradient only | `0xFF3A2E5C` |
| | Gradient angle (degrees) | `bg_gradient_angle` | double | 0–360° | `45` |
| | Gradient meeting point X | `bg_gradient_center_x` | double | 0–1 | `0.5` |
| | Gradient meeting point Y | `bg_gradient_center_y` | double | 0–1 | `0.5` |
| | Wallpaper preset | `bg_image` | string | "" (none) or `wallpapers/*.jpg` (18); only used by the Wallpaper type | `""` |
| | Local image file | `bg_image_file` | string | file path; only used by the Local image type | `""` |
| | Wallpaper blur radius (px) | `bg_image_blur` | double | 0–60 | `0` |
| | Wallpaper blur style (legacy) | `bg_image_blur_type` | int | Soft is forced (`1`); other values are ignored | `1` |
| | Horizontal screen margin (px) | `bg_padding_x` | double | 0–400 | `64` |
| | Vertical screen margin (px) | `bg_padding_y` | double | 0–400 | `64` |
| | Keep screen aspect ratio | `bg_keep_aspect` | bool | — | `true` |
| | Screen corner radius (px) | `bg_corner_radius` | double | 0–200 | `18` |
| | Window shadow | `bg_shadow` | bool | — | `true` |
| | Shadow opacity (0–1) | `bg_shadow_alpha` | double | 0–1 | `0.35` |
| | Shadow blur (px) | `bg_shadow_blur` | double | 0–200 | `48` |
| | Shadow offset X (px) | `bg_shadow_offset_x` | double | -100–100 | `0` |
| | Shadow offset Y (px) | `bg_shadow_offset` | double | -100–100 | `12` |
| Cursor | Show custom cursor | `cursor_enabled` | bool | — | `true` |
| | Cursor style (grouped by theme) | `cursor_style` | int | **Bibata** 3 · 27 · 10 · 32 · 11 · 33 · 22 · 36; **Breeze** 2 · 26 · 21 · 35; **Adwaita** 4 · 28; **Yaru** 8 · 30; **DMZ** 7 · 29 · 19 · 34; **Google Dot** 5 · 20; **XCursor Pro** 9 · 31; **macOS** 0; **Windows** 1; **Effects** 12 Neon · 15 Onyx · 13 Dot; **Custom** 50 styles (see [`CURSORS.md`](CURSORS.md)) | `0` |
| | Cursor size (1 = default) | `cursor_size` | double | 0.5–4 | `1.4` |
| | Cursor shadow | `cursor_shadow` | bool | — | `true` |
| | Smoothing (legacy, ignored) | `cursor_smooth` | double | — | `0` |
| | Curve tilt (legacy, ignored) | `cursor_sway` | double | — | `0` |
| | Hide cursor when idle | `cursor_hide_idle` | bool | — | `true` |
| | Idle delay before hiding (ms) | `cursor_idle_delay_ms` | double | 0–5000 | `1200` |
| | Return cursor to start (legacy, ignored) | `cursor_loop` | bool | — | `false` |
| | Loop idle time (legacy, ignored) | `cursor_loop_ms` | double | — | `4000` |
| | Pointer correction X (legacy, ignored) | `cursor_offset_x` | double | — | `0` |
| | Pointer correction Y (legacy, ignored) | `cursor_offset_y` | double | — | `0` |
| | Secondary cursor (links) | `cursor_secondary_style` | int | -1 = same as primary; otherwise a cursor style ID | `-1` |
| | Change cursor over links | `cursor_link_detection` | bool | Windows/X11 only | `true` |
| | Cursor movement | `cursor_follow_mode` | int | 0 exact · 1 glide A→B (legacy spring/sampled map to A→B) | `0` |
| | Glide duration (ms) | `cursor_glide_ms` | double | 50–1200 | `180` |
| | Glide sample interval (legacy, ignored) | `cursor_glide_sample_ms` | double | — | `90` |
| Zoom | Zoom mode | `zoom_mode` | int | 0 off · 1 auto; manual zoom is always available through the hotkeys | `1` |
| | Zoom intensity (1 = no zoom) | `zoom_amount` | double | 1–4 | `1.6` |
| | Zoom transition time (ms) | `zoom_duration_ms` | double | 100–2000 | `450` |
| | Return to normal after idle (ms) | `zoom_idle_timeout_ms` | double | 200–10000 | `1500` |
| | Pan intensity (0 = off) | `zoom_pan_amount` | double | 0–1 | `1.0` |
| | Centering (0 = follows mouse, 1 = centered) | `zoom_center_amount` | double | 0–1 | `1.0` |
| Left click | Click effect | `click_left_effect` | int | 0 ripple, 1 orb, 2 ring, 3 force, 4 glow, 5 echo, 6 flare, 7 quiet circle, 8 water ripple, 9 clear water ripple | `0` |
| | Effect size (1 = default) | `click_left_size` | double | 0.1–4 | `1.0` |
| | Effect color | `click_left_color` | color | — | `0xFFFFFFFF` |
| | Effect duration (ms) | `click_left_duration_ms` | double | 100–2000 | `650` |
| | Animation curve (legacy, ignored) | `click_left_easing` | int | In-out cubic is forced | `3` |
| Right click | Click effect | `click_right_effect` | int | same | `1` |
| | Effect size (1 = default) | `click_right_size` | double | 0.1–4 | `1.2` |
| | Effect color | `click_right_color` | color | — | `0xFF7C5CFF` |
| | Effect duration (ms) | `click_right_duration_ms` | double | 100–2000 | `700` |
| | Animation curve (legacy, ignored) | `click_right_easing` | int | In-out cubic is forced | `3` |
| Click sound | Enable click sound | `click_sound_enabled` | bool | — | `true` |
| | Left click sound | `click_sound_file_left` | string | bundled `sounds/*.wav`; `""` = Mute | `sounds/click-soft.wav` |
| | Right click sound | `click_sound_file_right` | string | bundled `sounds/*.wav`; `""` = Mute | `sounds/click-mouse-right.wav` |
| | Legacy single file | `click_sound_file` | string | migrates to both sides | — |
| | Click sound volume (0–1) | `click_sound_volume` | double | 0–1 (honored by paplay) | `0.6` |
| | Play on left click (legacy) | `click_sound_left` | bool | derived from the left file; Mute is an empty path | `true` |
| | Play on right click (legacy) | `click_sound_right` | bool | derived from the right file; Mute is an empty path | `true` |
| Movement | Motion blur (removed) | `motion_blur_enabled` | bool | legacy, ignored | `false` |
| | Motion blur intensity (legacy) | `motion_blur_amount` | double | 0–1, ignored | `0.35` |
| Spotlight | Enable spotlight | `spotlight_enabled` | bool | — | `false` |
| | Spotlight shape | `spotlight_shape` | int | 0 circle, 1 square | `0` |
| | Radius (fraction of screen) | `spotlight_radius` | double | 0.05–1 | `0.30` |
| | Square corner radius (px) | `spotlight_corner_radius` | double | px | `0` |
| | Darken outside (0 = off, 1 = black) | `spotlight_dim` | double | 0–1 | `0.60` |
| | Enable border | `spotlight_border_enabled` | bool | — | `false` |
| | Border color | `spotlight_border_color` | color | — | `0xFFFFFFFF` |
| | Border width (px) | `spotlight_border_width` | double | px | `4` |
| | Halo shadow (removed) | `spotlight_shadow_enabled` | bool | legacy, ignored | `false` |
| | Halo color (legacy) | `spotlight_shadow_color` | color | ignored | `0xFF000000` |
| | Halo opacity (legacy) | `spotlight_shadow_alpha` | double | ignored | `0.55` |
| | Halo blur (legacy) | `spotlight_shadow_blur` | double | ignored | `30` |
| | Halo offset X (legacy) | `spotlight_shadow_offset_x` | double | ignored | `0` |
| | Halo offset Y (legacy) | `spotlight_shadow_offset_y` | double | ignored | `12` |
| | Outside effect (0 dim, 1 blur) | `spotlight_outside_mode` | int | 0–1 | `0` |
| | Enable blur outside (legacy) | `spotlight_blur_enabled` | bool | — | `false` |
| | Outside blur amount (px) | `spotlight_blur_amount` | double | px | `16` |
| | Zoom when active (x) | `spotlight_zoom_amount` | double | 1–4 | `1.35` |
| | Drop/zoom in (ms) | `spotlight_in_ms` | double | ms | `250` |
| | Drop/zoom out (ms) | `spotlight_out_ms` | double | ms | `300` |
| | Vertical drop distance (removed) | `spotlight_drop_px` | double | legacy, ignored | `24` |
| | Show cursor during spotlight | `spotlight_show_cursor` | bool | — | `true` |
| Magnifier | Enable magnifier lens | `magnifier_enabled` | bool | — | `false` |
| | Lens shape | `magnifier_shape` | int | 0 circle · 1 square | `0` |
| | Lens radius | `magnifier_radius` | double | 0.05–1 (fraction of the shorter side) | `0.28` |
| | Lens corner radius (px) | `magnifier_corner_radius` | double | 0–200 | `0` |
| | Magnification (x) | `magnifier_zoom` | double | 1–4 | `2.0` |
| | Follow cursor | `magnifier_follow_cursor` | bool | — | `true` |
| | Fixed center X | `magnifier_center_x` | double | 0–1 | `0.5` |
| | Fixed center Y | `magnifier_center_y` | double | 0–1 | `0.5` |
| | Lens border | `magnifier_border_enabled` | bool | — | `false` |
| | Lens border color | `magnifier_border_color` | color | — | `0xFFFFFFFF` |
| | Lens border width (px) | `magnifier_border_width` | double | 0.5–40 | `4` |
| | Lens shadow | `magnifier_shadow_enabled` | bool | — | `false` |
| | Lens shadow color | `magnifier_shadow_color` | color | — | `0xFF000000` |
| | Lens shadow opacity | `magnifier_shadow_alpha` | double | 0–1 | `0.55` |
| | Lens shadow blur (px) | `magnifier_shadow_blur` | double | 0–200 | `30` |
| | Lens shadow offset X (px) | `magnifier_shadow_offset_x` | double | -100–100 | `0` |
| | Lens shadow offset Y (px) | `magnifier_shadow_offset_y` | double | -100–100 | `12` |
| | Zoom in (ms) | `magnifier_in_ms` | double | 0–2000 | `250` |
| | Zoom out (ms) | `magnifier_out_ms` | double | 0–2000 | `300` |
| | Vertical drop distance (removed) | `magnifier_drop_px` | double | legacy, ignored | `24` |
| | Show cursor during magnifier | `magnifier_show_cursor` | bool | — | `true` |
| Keys | Show typed keys | `keys_enabled` | bool | — | `true` |
| | Modifiers only | `keys_modifiers_only` | bool | — | `false` |
| | Show special key names | `keys_show_special` | bool | — | `false` (off = ignore named keys; Space is always shown) |
| | Show shortcuts | `keys_show_shortcuts` | bool | — | `true` (Ctrl + key as one entry) |
| | On-screen time per key (ms) | `keys_ttl_ms` | double | 500–8000 | `2000` |
| | Maximum keys on screen | `keys_max` | int | 1–12 | `6` |
| | Horizontal position (0 = left, 1 = right) | `keys_pos_x` | double | 0–1 | `0.5` |
| | Vertical position (0 = top, 1 = bottom) | `keys_pos_y` | double | 0–1 | `0.93` |
| | Text size (1 = default) | `keys_size` | double | 0.4–4 | `1.0` |
| | Font | `keys_font` | string | `fonts/*.ttf` or path | `fonts/DejaVuSans-Bold.ttf` |
| | Text color | `keys_color` | color | — | `0xFFFFFFFF` |
| | Text shadow | `keys_shadow` | bool | — | `true` |
| | Show background band | `keys_band` | bool | — | `true` |
| | Band color | `keys_band_color` | color | — | `0x99000000` |
| Highlighter | Enable drag actions | `highlight_enabled` | bool | — | `true` |
| | Enable hotkey highlight (toggle zoom) | `highlight_hotkey_enabled` | bool | — | `true` |
| — | Drag action: box / line / zoom / press-zoom / box+zoom / line+zoom | `highlight_mode` | int | 0 box, 1 line, 2 zoom, 3 box+zoom, 4 line+zoom, 5 zoom on mouse press | `0` |
| Mark appearance | Mark color | `highlight_color` | color | — | `0x6600E0FF` |
| Mark appearance | Mark opacity (0 = invisible, 1 = solid) | `highlight_opacity` | double | 0–1 | `0.35` |
| Mark appearance | Line thickness (fraction of the screen) | `highlight_thickness` | double | 0.001–0.05 | `0.004` |
| Mark timing | Mark stays on screen (ms) | `highlight_hold_ms` | double | 0–5000 | `900` |
| Mark timing | Mark fade-out (ms) | `highlight_fade_ms` | double | 0–2000 | `300` |
| Zoom | Zoom direction (legacy, fixed IN) | `highlight_zoom_dir` | int | forced to `0` (in) | `0` |
| Zoom | Hold-to-zoom delay (ms) | `highlight_zoom_delay_ms` | double | 0–5000 | `1200` |
| Zoom | PressZoom delay (legacy, ignored) | `highlight_press_delay_ms` | double | PressZoom was removed | `400` |
| Zoom | Maximum zoom (x) | `highlight_zoom_max` | double | 1.2–8 | `2.5` |
| Zoom | Zoom-in duration (ms) | `highlight_zoom_in_ms` | double | 50–2000 | `250` |
| Zoom | Return duration (ms) | `highlight_zoom_out_ms` | double | 100–8000 | `1200` |
| Zoom | Hold at target (ms) | `highlight_zoom_hold_ms` | double | 0–5000 | `600` |
| Monitor | Manual origin | `monitor_manual` | bool | — | `false` |
| | Origin X (px) | `monitor_x` | int | −32768…32767 | `0` |
| | Origin Y (px) | `monitor_y` | int | −32768…32767 | `0` |
| Profiles | Current profile | `profile_current` | int | 0–9 (A–J) | `0` |
| | Profile slots | `profile_slot_0`, `profile_slot_1`, … `profile_slot_5` … `profile_slot_9` (profiles A–J) | object | Full config (nested obs_data) | — |
| | Export path (Properties fallback) | `profile_export_path` | string | Any `.json` path | `""` |
| | Import path (Properties fallback) | `profile_import_path` | string | Any `.json` path | `""` |

> Background: `bg_type` selects solid, gradient, wallpaper, local image or
> transparent. Local image and Transparent are background **types**, not entries
> in the wallpaper list. The gradient angle is in degrees and the meeting point
> moves the 50/50 transition. Old sources are migrated from `bg_mode`
> (vertical/diagonal become gradients; an image becomes wallpaper; explicit
> `bg_type` always wins). Sources from 1.1.0 that stored `__local__` or
> `__transparent__` in `bg_image` are converted to the matching type.

> Cursor: the cursor **tip** follows the physical pointer over the captured
> screen (the content area), so it points at the same captured pixel even with
> wallpaper padding. The icon body is drawn in output coordinates and may
> overhang into the padding; the tip never leaves the captured screen. Bitmap
> cursors use a fixed 30° clockwise tilt (no sway). Only Exact and Glide A→B
> remain; legacy spring/sampled values are migrated to Glide A→B.

> Spotlight: the outside effect is Dim or Blur. Only the strength slider for
> the selected mode is shown; the blur style is always Soft. Shadow/drop were
> removed.

> Magnifier: the lens follows the cursor with a 30% bias toward the center near
> the edges. The lens shadow is a real halo behind the lens; the drop animation
> was removed.

> Profiles: switching profile applies the whole slot; subsequent edits are written
> to the active slot (auto-save, debounced by ~0.4 s) and the chosen profile is
> re-applied when the source loads.

> Spotlight and Magnifier are mutually exclusive: enabling one disables the other. Both are full-screen focus effects and share the same transition/hotkey model.

> Highlighter: the hotkey toggle is independent of the drag modes and of the
> **Enable drag actions** switch. It uses the same Zoom settings above, so the
> Zoom group stays visible whenever the hotkey is enabled, even in Box/Line
> mode. The global **Effects enabled** master still applies: with effects off
> the hotkey is a no-op (and releases an active gesture). Highlight zoom is
> always IN and uses in-out cubic easing; delay, zoom, time in, stay in and time
> out are the only zoom controls.

> Imported settings: every numeric value is sanitized when settings, presets or
> profiles are loaded. Non-finite values (NaN/infinity) fall back to the
> built-in default and out-of-range values are clamped to the ranges above, so a
> hand-edited file cannot produce degenerate uniforms or animation state.

### Profiles backup

`Export profiles` writes a `.json` with the 10 slots and the active index:

```json
{
  "topscreen_profiles": 1,
  "plugin": "TopScreen",
  "version": "1.1.2",
  "current": 0,
  "profile_slot_0": { "effects_enabled": true },
  "profile_slot_1": { "effects_enabled": true },
  "...": "profile_slot_2 ... profile_slot_8",
  "profile_slot_9": { "effects_enabled": true }
}
```

`Import profiles` validates the marker and any present slots before changing
anything. Missing `current` defaults to 0; missing or empty slots use that
profile's default seed; missing fields keep that seed's defaults. Import
replaces profiles A–J and activates the stored profile. Unknown keys are
ignored, so newer/older backups and fields added or removed between versions
remain readable. The file contains settings only; custom images and sounds are
referenced by path and are not embedded.

## Hotkeys

Registered IDs (global frontend hotkeys, shown in **Settings → Hotkeys**;
they act on the filter selected in the dock, or the first active one):

| ID | Action |
|----|------|
| `TopScreen_ToggleEffects` | `TopScreen_ToggleEffects — Toggle effects` |
| `TopScreen_ToggleBackground` | `TopScreen_ToggleBackground — Toggle background` |
| `TopScreen_ZoomIn` | `TopScreen_ZoomIn — Zoom in` |
| `TopScreen_ZoomOut` | `TopScreen_ZoomOut — Zoom out` |
| `TopScreen_ResetZoom` | `TopScreen_ResetZoom — Reset zoom` |
| `TopScreen_NextPreset` | `TopScreen_NextPreset — Next preset` |
| `TopScreen_PrevPreset` | `TopScreen_PrevPreset — Previous preset` |
| `TopScreen_ProfileA` | `TopScreen_ProfileA — Activate profile A` |
| `TopScreen_ProfileB` | `TopScreen_ProfileB — Activate profile B` |
| `TopScreen_ProfileC` | `TopScreen_ProfileC — Activate profile C` |
| `TopScreen_ProfileD` | `TopScreen_ProfileD — Activate profile D` |
| `TopScreen_ProfileE` | `TopScreen_ProfileE — Activate profile E` |
| `TopScreen_ProfileF` | `TopScreen_ProfileF — Activate profile F` |
| `TopScreen_ProfileG` | `TopScreen_ProfileG — Activate profile G` |
| `TopScreen_ProfileH` | `TopScreen_ProfileH — Activate profile H` |
| `TopScreen_ProfileI` | `TopScreen_ProfileI — Activate profile I` |
| `TopScreen_ProfileJ` | `TopScreen_ProfileJ — Activate profile J` |
| `TopScreen_ProfileSave` | `TopScreen_ProfileSave — Save to current profile` |
| `TopScreen_ToggleHighlight` | `TopScreen_ToggleHighlight — Toggle hotkey highlight (zoom)` |
| `TopScreen_ToggleSpotlight` | `TopScreen_ToggleSpotlight — Toggle spotlight` |
| `TopScreen_ToggleMagnifier` | `TopScreen_ToggleMagnifier — Toggle magnifier lens` |

## Preset format

Files in `data/presets/*.json`. The `settings` block uses the same keys as the
table above; missing keys keep the current value.

```json
{
  "name": "Demo",
  "description": "Fundo em gradiente, cursor destacado e zoom automatico.",
  "settings": {
    "effects_enabled": true,
    "bg_enabled": true,
    "bg_type": 1,
    "bg_gradient_angle": 45,
    "bg_gradient_center_x": 0.5,
    "bg_gradient_center_y": 0.5,
    "bg_color_a": 4280296494,
    "bg_color_b": 4282003036,
    "bg_padding_x": 64,
    "bg_padding_y": 64,
    "bg_corner_radius": 18,
    "bg_shadow": true,
    "bg_shadow_alpha": 0.35,
    "bg_shadow_blur": 48,
    "bg_shadow_offset_x": 0,
    "bg_shadow_offset": 12,
    "cursor_enabled": true,
    "cursor_size": 1.6,
    "cursor_smooth": 0.8,
    "cursor_sway": 0.4,
    "zoom_mode": 1,
    "zoom_amount": 1.6,
    "click_left_effect": 0,
    "click_right_effect": 1,
    "click_sound_enabled": false,
    "motion_blur_enabled": true,
    "spotlight_enabled": false,
    "keys_enabled": true,
    "keys_ttl_ms": 2000,
    "highlight_enabled": true
  }
}
```

Applying a preset = writing `settings` into the filter's `obs_data`. Switch
preset with the *Next/Previous preset* hotkeys or through the dock.

## Session state (not persisted)

- Effects/background toggle via hotkey (returns to default on restart).
- Manual zoom (in/out/reset) via hotkey.

## Input tiers

| Tier | Meaning |
|------|-------------|
| Native | Windows (WinAPI) or Linux/X11 (cursor + keys) |
| evdev | Linux/Wayland: keys via `/dev/input` |
| Portal | Wayland via `GlobalShortcuts` (planned) |
| Unavailable | No backend; input effects stay inert |

See [`INPUT-AND-PLATFORMS.md`](INPUT-AND-PLATFORMS.md).

| `TopScreen_ToggleCursor` | `TopScreen_ToggleCursor — Toggle cursor visibility` |
