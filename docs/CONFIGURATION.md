# Configuration reference

Values persisted by OBS (per source/scene) and used in the preset files.
The usage manual is in [`MANUAL.md`](MANUAL.md).

## Colors

OBS format: integer **`0xAABBGGRR`** (low byte = red).
E.g.: `0xFF3A2E5C` = A=FF, B=3A, G=2E, R=5C.

## All options

| Group (UI) | Option (UI) | Key | Type | Range | Default |
|------------|------------|-------|------|-------|--------|
| — | Effects enabled | `effects_enabled` | bool | — | `true` |
| Background | Enable background | `bg_enabled` | bool | — | `true` |
| | Style | `bg_mode` | int | 0 solid · 1 vertical · 2 diagonal | `2` |
| | Color A | `bg_color_a` | color | — | `0xFF20242E` |
| | Color B | `bg_color_b` | color | — | `0xFF3A2E5C` |
| | Wallpaper | `bg_image` | string | "" (none), `__transparent__` (show the scene below), or `wallpapers/*.jpg` (42) | `""` |
| | Image from PC | `bg_image_file` | string | file path (**clears** the Wallpaper) | `""` |
| | Wallpaper blur (px) | `bg_image_blur` | double | 0–60 | `0` |
| | Blur type | `bg_image_blur_type` | int | 0 none · 1 soft · 2 gaussian · 3 creamy | `2` |
| | Horizontal spacing (px) | `bg_padding_x` | double | 0–400 | `64` |
| | Vertical spacing (px) | `bg_padding_y` | double | 0–400 | `64` |
| | Keep screen aspect ratio | `bg_keep_aspect` | bool | — | `true` |
| | Rounded corners (px) | `bg_corner_radius` | double | 0–200 | `18` |
| | Window shadow | `bg_shadow` | bool | — | `true` |
| | Shadow opacity | `bg_shadow_alpha` | double | 0–1 | `0.35` |
| | Shadow blur (px) | `bg_shadow_blur` | double | 0–200 | `48` |
| | Shadow horizontal offset (px) | `bg_shadow_offset_x` | double | -100–100 | `0` |
| | Shadow vertical offset (px) | `bg_shadow_offset` | double | -100–100 | `12` |
| Cursor | Show cursor | `cursor_enabled` | bool | — | `true` |
| | Style (grouped by theme) | `cursor_style` | int | **Bibata** 3 arrow · 27 link hand · 10 Ice · 32 Ice link · 11 Original · 33 Original link · 22 Ice Original · 36 Ice Original link; **Breeze** 2 · 26 link · 21 Snow · 35 Snow link · 25 I-beam; **Adwaita** 4 · 28 link · 6 hand · 16 grab open · 17 grab closed · 18 I-beam; **Yaru** 8 · 30 link · 23 grab · 24 I-beam; **DMZ** 7 white · 29 white link · 19 black · 34 black link; **Google Dot** 5 white · 20 black; **XCursor Pro** 9 · 31 link; **macOS** 0; **Windows** 1; **Effects** 12 Neon · 15 Onyx · 13 Dot · 14 Crosshair; **Custom** 37–105 (see [`CURSORS.md`](CURSORS.md)) | `0` |
| | Size | `cursor_size` | double | 0.5–4 | `1.4` |
| | Cursor shadow | `cursor_shadow` | bool | — | `true` |
| | Smoothing | `cursor_smooth` | double | 0–1 | `0.7` |
| | Curve tilt | `cursor_sway` | double | 0–1 | `0.3` |
| | Hide when idle | `cursor_hide_idle` | bool | — | `true` |
| | Hide delay (ms) | `cursor_idle_delay_ms` | double | 0–5000 | `1200` |
| | Return to start when idle | `cursor_loop` | bool | — | `false` |
| | Return time (ms) | `cursor_loop_ms` | double | 1000–20000 | `4000` |
| Zoom | Mode | `zoom_mode` | int | 0 off, 1 auto, 2 manual | `1` |
| | Intensity | `zoom_amount` | double | 1–4 | `1.6` |
| | Speed (ms) | `zoom_duration_ms` | double | 100–2000 | `450` |
| | Idle time (ms) | `zoom_idle_timeout_ms` | double | 200–10000 | `1500` |
| | Pan intensity | `zoom_pan_amount` | double | 0–1 | `1.0` |
| | Centering | `zoom_center_amount` | double | 0–1 | `1.0` |
| Left click | Effect | `click_left_effect` | int | 0 ripple, 1 orb, 2 ring, 3 force, 4 glow, 5 echo, 6 flare | `0` |
| | Size | `click_left_size` | double | 0.1–4 | `1.0` |
| | Color | `click_left_color` | color | — | `0xFFFFFFFF` |
| | Duration (ms) | `click_left_duration_ms` | double | 100–2000 | `650` |
| | Curve | `click_left_easing` | int | 0 linear, 1 in, 2 out, 3 in-out | `2` |
| Right click | Effect | `click_right_effect` | int | same | `1` |
| | Size | `click_right_size` | double | 0.1–4 | `1.2` |
| | Color | `click_right_color` | color | — | `0xFF7C5CFF` |
| | Duration (ms) | `click_right_duration_ms` | double | 100–2000 | `700` |
| | Curve | `click_right_easing` | int | same | `2` |
| Click sound | Enable sound | `click_sound_enabled` | bool | — | `true` |
| | Left click sound | `click_sound_file_left` | string | bundled `sounds/*.wav` (41; the three `click-mouse-*.wav` are real mouse recordings) | `sounds/click-soft.wav` |
| | Right click sound | `click_sound_file_right` | string | bundled `sounds/*.wav` | `sounds/click-pop.wav` |
| | Legacy single file | `click_sound_file` | string | migrates to both sides | — |
| | Volume | `click_sound_volume` | double | 0–1 (honored by paplay) | `0.6` |
| | On left click | `click_sound_left` | bool | — | `true` |
| | On right click | `click_sound_right` | bool | — | `false` |
| Movement | Motion blur | `motion_blur_enabled` | bool | — | `true` |
| | Intensity | `motion_blur_amount` | double | 0–1 | `0.35` |
| Spotlight | Enable spotlight | `spotlight_enabled` | bool | — | `false` |
| | Radius | `spotlight_radius` | double | 0.05–1 | `0.30` |
| | Dimming | `spotlight_dim` | double | 0–1 | `0.60` |
| Keys | Show typed keys | `keys_enabled` | bool | — | `true` |
| | Modifiers only | `keys_modifiers_only` | bool | — | `false` |
| | Show special keys as icons | `keys_show_special` | bool | — | `true` (off = ignore Space/Enter/arrows/modifiers) |
| | On-screen time (ms) | `keys_ttl_ms` | double | 500–8000 | `2000` |
| | Max keys | `keys_max` | int | 1–12 | `6` |
| | Position X | `keys_pos_x` | double | 0–1 | `0.5` |
| | Position Y | `keys_pos_y` | double | 0–1 | `0.93` |
| | Text size | `keys_size` | double | 0.4–4 | `1.0` |
| | Font | `keys_font` | string | `fonts/*.ttf` or path | `fonts/DejaVuSans-Bold.ttf` |
| | Text color | `keys_color` | color | — | `0xFFFFFFFF` |
| | Text shadow | `keys_shadow` | bool | — | `true` |
| | Background band | `keys_band` | bool | — | `true` |
| | Band color | `keys_band_color` | color | — | `0x99000000` |
| Highlighter | Enable drag actions | `highlight_enabled` | bool | — | `true` |
| | Enable hotkey highlight (toggle zoom) | `highlight_hotkey_enabled` | bool | — | `true` |
| Look | Drag action: box / line / zoom / press-zoom / box+zoom / line+zoom | `highlight_mode` | int | 0 box, 1 line, 2 zoom, 3 box+zoom, 4 line+zoom, 5 zoom on mouse press | `0` |
| Look | Color | `highlight_color` | color | — | `0x6600E0FF` |
| Look | Opacity (0 = invisible) | `highlight_opacity` | double | 0–1 | `0.35` |
| Look | Line thickness (fraction of screen) | `highlight_thickness` | double | 0.001–0.05 | `0.004` |
| Timing | Keep on screen (ms) | `highlight_hold_ms` | double | 0–5000 | `900` |
| Timing | Fade out (ms) | `highlight_fade_ms` | double | 0–2000 | `300` |
| Zoom | Zoom direction (in / out / punch) | `highlight_zoom_dir` | int | 0 in, 1 out, 2 punch in, 3 punch out | `0` |
| Zoom | Hold-to-zoom delay (ms) | `highlight_zoom_delay_ms` | double | 0–5000 | `1200` |
| Zoom | Zoom on mouse press: hold delay (ms) | `highlight_press_delay_ms` | double | 0–3000 | `400` |
| Zoom | Maximum zoom | `highlight_zoom_max` | double | 1.2–8 | `2.5` |
| Zoom | Zoom-in time (ms) | `highlight_zoom_in_ms` | double | 50–2000 | `250` |
| Zoom | Return time after release (ms) | `highlight_zoom_out_ms` | double | 100–8000 | `1200` |
| Zoom | Hold at target after release (ms) | `highlight_zoom_hold_ms` | double | 0–5000 | `600` |
| Monitor | Manual origin | `monitor_manual` | bool | — | `false` |
| | Origin X (px) | `monitor_x` | int | −32768…32767 | `0` |
| | Origin Y (px) | `monitor_y` | int | −32768…32767 | `0` |
| Profiles | Current profile | `profile_current` | int | 0–9 (A–J) | `0` |
| | Profile slots | `profile_slot_0`, `profile_slot_1`, … `profile_slot_5` … `profile_slot_9` (profiles A–J) | object | Full config (nested obs_data) | — |
| | Export path (Properties fallback) | `profile_export_path` | string | Any `.json` path | `""` |
| | Import path (Properties fallback) | `profile_import_path` | string | Any `.json` path | `""` |

> Profiles: switching profile applies the whole slot; subsequent edits are written
> to the active slot (auto-save, debounced by ~0.4 s) and the chosen profile is
> re-applied when the source loads.

> Highlighter: the hotkey toggle is independent of the drag modes and of the
> **Enable drag actions** switch. It uses the same Zoom settings above, so the
> Zoom group stays visible whenever the hotkey is enabled, even in Box/Line
> mode. With **Zoom direction = out** at 1× there is nothing to zoom out from;
> use **in** (the default) for the first press.

### Profiles backup

`Export profiles` writes a `.json` with the 10 slots and the active index:

```json
{
  "topscreen_profiles": 1,
  "plugin": "TopScreen",
  "version": "0.3.0",
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
| `TopScreen_ToggleHighlight` | `TopScreen_ToggleHighlight — Toggle highlight zoom` |

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
    "bg_mode": 2,
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
