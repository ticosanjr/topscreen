# Effect catalog

Golden rule: **only what works live gets in**. Post-production effects
(timeline, cuts, manual keyframes) do not exist here.

Status: ✅ implemented · 🟡 partial · ⬜ planned.
Mode: `auto` reacts on its own · `hotkey` only by hotkey · `always` always on.

## Cursor

| Feature | Mode | Status | Notes |
|---------|------|--------|-------------|
| Replace the pointer (turns off the native one) | auto | ✅ | turns off `capture_cursor`/`show_cursor` on the parent source |
| **79 cursor styles** | auto | ✅ | 29 built-ins + 50 custom PNG styles grouped under `Custom`; see [`CURSORS.md`](CURSORS.md) |
| Size | auto | ✅ | 0.5–4× |
| Shadow | auto | ✅ | |
| Motion smoothing | auto | ✅ | critically damped spring |
| Sway (tilts on turns) | auto | ✅ | |
| Hide when idle | auto | ✅ | with delay |
| Loop (returns to start) | off | ✅ | |

## Clicks

| Feature | Mode | Status | Notes |
|---------|------|--------|-------------|
| Left button effect | auto | ✅ | ripple / orb / ring / force / glow / echo / flare + color/size/duration/curve |
| Right button effect | auto | ✅ | independent of the left |
| Click sound | auto | ✅ | 14 sounds (6 realistic mouse clicks + 8 FX), one per button, through the system audio |

## Framing

| Feature | Mode | Status | Notes |
|---------|------|--------|-------------|
| Background | always | ✅ | solid color or two-color gradient with adjustable meeting point/angle |
| Wallpaper background | always | ✅ | 18 built-in wallpapers, an image from your PC, or **Transparent** (show the scene below) |
| Text overlay | auto | ✅ | one draw for text + shadow/outline via vertex colors |
| Rounded corners | always | ✅ | |
| Spacing (padding X and Y) | always | ✅ | separate axes |
| Window shadow | always | ✅ | opacity, blur and offset |
| Inset / inner border | off | ⬜ | |

## Zoom / Pan

| Feature | Mode | Status | Notes |
|---------|------|--------|-------------|
| Automatic zoom (click/cursor) | auto | ✅ | returns to normal when idle |
| Manual zoom in/out/reset | hotkey | ✅ | |
| Light pan following the mouse | auto | ✅ | |
| Zoom into a selected region | hotkey | ⬜ | |

## Movement and attention

| Feature | Mode | Status | Notes |
|---------|------|--------|-------------|
| Motion blur (from zoom) | auto | ✅ | radial blur with 3 samples |
| Motion blur (from cursor) | auto | ⬜ | |
| Spotlight following the cursor | hotkey | ✅ | round/square, drop/zoom, border, halo shadow and outside blur |

## Text and highlight

| Feature | Mode | Status | Notes |
|---------|------|--------|-------------|
| Key overlay | auto/hotkey | ✅ | TTF atlas (font/color/size/shadow) + X/Y position + background band |
| Drag highlighter | auto | ✅ | filled box, freehand **line**, **zoom to the selection** or **zoom on mouse press** (quick in, slow out); dragged region with hold/fade |

> Highlighting the **text that is actually selected** (via accessibility) remains
> out of scope — the substitute is the **drag highlighter**.

## Camera and extras

| Feature | Mode | Status | Notes |
|---------|------|--------|-------------|
| Webcam bubble | always | ⬜ | requires its own source (F6) |
| Countdown / hotkey HUD | hotkey | ⬜ | |

## Out of scope (post-production)

Timeline, cuts, speed ramps, manual keyframes, transitions between takes,
MP4/GIF export, recording audio normalization.
