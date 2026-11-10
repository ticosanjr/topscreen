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
| Motion smoothing | auto | ✅ | exact, A→B glide or continuous smooth |
| Secondary cursor over links | auto | ✅ | Windows/X11; primary/secondary style pair |
| Hide when idle | auto | ✅ | with delay |

## Clicks

| Feature | Mode | Status | Notes |
|---------|------|--------|-------------|
| Left button effect | auto | ✅ | ripple / orb / ring / force / glow / echo / flare / quiet circle / water ripple + color/size/duration/curve |
| Right button effect | auto | ✅ | independent of the left |
| Click sound | auto | ✅ | 26 bundled sounds, one per button, through the `TopScreen Click Sound` OBS audio source |

## Framing

| Feature | Mode | Status | Notes |
|---------|------|--------|-------------|
| Background | always | ✅ | solid color, two-color gradient or blurred screen |
| Wallpaper background | always | ✅ | 18 built-in wallpapers, an image from your PC, or **Transparent** (show the scene below) |
| Text overlay | auto | ✅ | one draw for text + shadow/outline via vertex colors |
| Rounded corners | always | ✅ | |
| Spacing (padding X and Y) | always | ✅ | separate axes |
| Window shadow | always | ✅ | opacity, blur and offset |
| Inner border / device bezel | always | ✅ | configurable color and width |

## Zoom / Pan

| Feature | Mode | Status | Notes |
|---------|------|--------|-------------|
| Automatic zoom (click/cursor) | auto | ✅ | returns to normal when idle |
| Smart zoom | auto | ✅ | action-driven with configurable hold and return |
| Motion blur | auto | ✅ | optional radial blur while zooming |
| Manual zoom in/out/reset | hotkey | ✅ | |
| Light pan following the mouse | auto | ✅ | |
| Zoom into a selected region | hotkey | ⬜ | |

## Movement and attention

| Feature | Mode | Status | Notes |
|---------|------|--------|-------------|
| Spotlight following the cursor | hotkey | ✅ | round/square, fade/zoom, border and Dim/Blur outside |
| Magnifier lens | hotkey | ✅ | round/square, local-only zoom, border/shadow, follow or fixed center |

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
| Webcam bubble | always | ✅ | separate filter: circle/rounded mask, border and shadow |
| Countdown / hotkey HUD | hotkey | ⬜ | |

## Out of scope (post-production)

Timeline, cuts, speed ramps, manual keyframes, transitions between takes,
MP4/GIF export, recording audio normalization.
