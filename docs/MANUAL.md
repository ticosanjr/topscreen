# User manual — TopScreen

**TopScreen** is a **live screen-capture effects** plugin for OBS.
It applies a nice cursor, click effects, smooth zoom, a background with rounded
corners, a key overlay and more — **live**, without recording and without editing.

It **does not record** and **does not produce a file**: it delivers the processed
image straight to OBS, as a filter, for you to use in a scene.

---

## 1. Requirements

- **OBS Studio 30.0+** (Windows or Linux)
- A **Display Capture** (Display/Screen Capture) as a source in OBS

> On Linux, the key overlay requires read permission on
> `/dev/input` (see §10).

---

## 2. Installation

**Linux**
```bash
./scripts/install-linux.sh            # per-user plugin (no sudo)
```

**Windows** (no compiler, no admin)
1. Download `topscreen-<version>-windows.zip` (release / CI artifact) and extract the **whole zip**.
2. Double-click **`install.bat`** (drag your portable OBS folder onto it for portable installs; drag a `topscreen.dll` to install a specific build).

Cloned the source instead? Running `scripts\install-windows.bat` also works — it
looks for a compiled `topscreen.dll` and, if there is none, tells you exactly how
to proceed. Details in [`BUILD.md`](BUILD.md).

---

## 3. Getting started

1. In OBS, add a **Display Capture** source and pick the screen.
2. Right-click the source → **Filters** → **+** → **TopScreen FX**.
3. Adjust in **Properties** (the tabs/groups are described in §5).
4. Create hotkeys in **Settings → Hotkeys → TopScreen** (§6).
5. (Optional) Choose a ready **preset** with the *Next preset* hotkey.

> **Tip:** open the **TopScreen** dock (menu **View → Docks → TopScreen**) to
> see the state and switch presets quickly.

---

## 4. Concepts

### Mode of each feature
Some features can be **on**, **off** or **by hotkey**.
The **Effects enabled** (top of Properties) is the "master": it turns everything
off at once (useful as push-to-talk for effects).

### Own cursor
When the effects are enabled, TopScreen **turns off the native cursor of the
capture** and draws its own, smoothed and scalable. That is why you see **a
single cursor**.

### Coordinates
The effects work over the **captured area**. The cursor is mapped from the mouse
to that area (assuming the capture is at the top-left corner of the screen).

---

## 5. Usage reference (by Properties group)

### Effects enabled
Turns **all** filter effects on/off.

### Background
Places the screen "floating" over a background, with corners and shadow.

| Option | What it does |
|-------|-----------|
| **Enable background** | Turns on the background + corners + spacing |
| **Style** | Solid color, vertical or diagonal gradient (used when there is **no** image) |
| **Color A / Color B** | Background colors (B only shows in gradients) |
| **Wallpaper** | One of the **24 images** that ship with the plugin. Choosing one applies it |
| **Image from PC** | Button to choose a **file from your PC** (jpg/png/bmp/tga/gif/webp). Choosing one **clears** the Wallpaper (they are mutually exclusive) |
| **Wallpaper blur (px)** | Blurs the background image (0 = sharp) |
| **Blur type** | No blur · Soft (box) · **Gaussian** · Creamy (very soft) |
| **Horizontal / vertical spacing (px)** | Margin between the screen and the edge (separate X and Y) |
| **Keep screen aspect ratio** | Avoids stretching the screen: it is scaled down proportionally (leave it on) |
| **Rounded corners (px)** | Radius of the screen corners |
| **Window shadow** (sub-group) | |
| ↳ **Enable shadow** | Turns the shadow behind the screen on/off |
| ↳ **Intensity** | How strong the shadow is (0 = invisible, 1 = strong) |
| ↳ **Softness / size** | How soft and large the shadow is |
| ↳ **Downward distance** | Moves the shadow down (emboss effect) |

> The background appears **behind** the screen. Without an image, use the **Style** (color/gradient).

### Cursor
| Option | What it does |
|-------|-----------|
| **Show cursor** | Draws the own cursor (turns off the native one) |
| **Style** | One of the 16 cursors (see below) |
| **Size** | Cursor scale (1.0 = default) |
| **Smoothing** | 0 = **follows the mouse exactly**; above that, it glides ("screen recorder" style) |
| **Curve tilt (sway)** | The cursor tilts slightly when turning |
| **Cursor shadow** | Shadow to make it stand out from the background |
| **Hide when idle / Delay** | Disappears when you stop moving |
| **Return to start when idle / Time** | Cursor "returns" (looping videos) |

**Cursor styles available (16)**

| Style | Appearance |
|--------|-----------|
| macOS (classic) | Black arrow with a white outline |
| Windows (classic) | White arrow with a black outline |
| Breeze (KDE) | Arrow from the Breeze theme |
| Bibata (modern) | Modern rounded arrow (Bibata Modern Classic) |
| Adwaita | Arrow from the Adwaita theme (GNOME) |
| Google Dot | Circle with a dot (Google style) |
| Hand (pointer) | Pointing hand |
| DMZ (classic X11) | Classic arrow from the DMZ-White theme |
| Yaru (Ubuntu) | Arrow from the Yaru theme |
| XCursor Pro | Arrow from the XCursor-Pro theme |
| Bibata Ice (blue) | Bibata with a blue highlight |
| Bibata Original | Bibata in its original form |
| Neon (cyan arrow) | Glowing cyan arrow for dark scenes |
| Dot (touch) | Coral disc, centered (touch-style) |
| Crosshair | Precision cross for pixel work |
| Onyx (navy + gold) | Elegant dark arrow with a gold outline |

> The bitmaps are generated by `tools/make-cursor-atlas.py` (reads cursor themes
> installed in `/usr/share/icons`) and live in `data/cursors/`.

### Left click / Right click
Each button has its own independent effect.

| Option | What it does |
|-------|-----------|
| **Effect** | Ripple, Orb, Ring, Force, Glow, Echo (double ring) or Flare (spark) |
| **Size / Color / Duration / Curve** | Appearance and timing of the effect |

### Click sound
| Option | What it does |
|-------|-----------|
| **Enable sound** | Plays a short click |
| **Left / Right click sound** | 8 bundled sounds: Soft, Hard, Pop, Tick, Wood, Glass, Bubble, Thock |
| **Volume** | Sound volume (applied on paplay; other system players play as-is) |
| **On left / right click** | Which buttons trigger it |

> Switching sounds applies immediately (no restart needed).

> The sound plays through the **system audio** (it enters OBS through your Desktop
> Audio capture). It does not yet come out through the source's own audio channel.

### Zoom
| Option | What it does |
|-------|-----------|
| **Mode** | `Off`, `Automatic` (follows clicks/cursor) or `Manual` |
| **Intensity** | How much it magnifies (1.0 = no zoom) |
| **Speed (ms)** | Animation duration |
| **Idle time (ms)** | After stopping, returns to normal |
| **Pan intensity** | How much the screen "follows" the mouse |
| **Centering** | 0 = follows the mouse; 1 = keeps the mouse in the **center** (the view **anticipates** the edges — the corner appears before you get to it) |

### Movement (motion blur)
| Option | What it does |
|-------|-----------|
| **Motion blur** | Radial blur during zoom |
| **Intensity** | Blur strength |

### Spotlight
| Option | What it does |
|-------|-----------|
| **Enable spotlight** | Darkens everything except a circle that follows the cursor |
| **Radius** | Size of the circle |
| **Dimming** | How much it darkens outside the circle |

### Keys
| Option | What it does |
|-------|-----------|
| **Show typed keys** | Shows what you type on screen |
| **Modifiers only** | Shows only Ctrl/Shift/Alt/Win |
| **On-screen time (ms)** | How long each key stays |
| **Max keys** | How many simultaneous keys to show |

Every keystroke appears as its own letter (repeats included) exactly as typed,
then fades out after the on-screen time. Special keys are shown as icons:
`␣` Space, `⌫` Backspace, `⏎` Enter, `⇥` Tab, arrows, `⇧`/`⌃`/`⌥`/`⊞` for the
modifiers (`Esc` stays as text).
| **Position X / Y** | Where the band sits (0 = left/top, 1 = right/bottom) |
| **Text size** | Text scale |
| **Font** | DejaVu Sans Bold / Sans / ExtraLight / Mono Bold / Mono / Serif Bold, or a path to a `.ttf` |
| **Text color** | Letter color |
| **Text shadow** | Dark outline for readability |
| **Background band / Band color** | Rounded rectangle behind the text and its color |

### Highlighter
| Option | What it does |
|-------|-----------|
| **Enable highlighter** | Highlights the region you drag with the left button |
| **Color** | Highlight color (use alpha < 255 for translucent) |
| **Opacity** | Strength ceiling (0 = invisible, 1 = solid); keeps content readable |
| **On-screen time (ms)** | How long it stays after you release |
| **Fade (ms)** | Fade duration |

---

## 6. Hotkeys

In **Settings → Hotkeys → TopScreen**:

| Action | Effect |
|------|--------|
| **Enable/disable effects** | Turns everything on/off (master) |
| **Enable/disable background** | Turns only the background/corners on/off |
| **Zoom in / Zoom out / Reset zoom** | Manual zoom |
| **Next preset / Previous preset** | Switch preset |
| **Activate profile A / B / C / D** | Applies the profile (all of its settings) |
| **Save to current profile** | Writes the adjustments to the active profile |

**Linux/Wayland:** global OBS hotkeys may not work outside X11;
use the hotkeys with OBS focused, or **obs-websocket** to trigger actions.

---

## 7. Presets

Ready presets (files in `data/presets/`):

| Preset | Look |
|--------|------|
| **Demo** | Gradient, highlighted cursor, automatic zoom |
| **Tutorial** | Solid background, click sound, soft cursor |
| **Presentation** | Dark background, no zoom, no sound |

Switch with the *Next/Previous preset* hotkey (or via the dock). To create your
own, copy a file and adjust it — the format is in [`CONFIGURATION.md`](CONFIGURATION.md).

---

## 8. Profiles (A/B/C/D)

The **4 profiles** store the entire configuration and switch everything at once.
The selector is at the **top** of Properties (group **Profiles → Current profile**)
and in the dock as well.

- **Switching profile applies EVERYTHING immediately** (background, cursor, clicks, zoom, keys…).
- **Your changes are saved into the active profile automatically** — no need to
  remember to save (the *Save to profile* button and hotkey exist to force it).
- The selected profile is re-applied **when OBS opens**, so the look comes back the same.
- Hotkeys: **Activate profile A/B/C/D** and **Save to current profile**.

They already ship with examples, so you can just switch and see the difference:

| Profile | Example |
|---|---|
| **A** | default |
| **B** | **green/orange** clicks + **zoom 2.0** |
| **C** | **purple/cyan** clicks + zoom 1.4 and vertical background |
| **D** | **yellow/white** clicks and **zoom off** |

---

## 9. Recipes

**Product demo (similar to "Screen Studio")**
- Background = Diagonal gradient, Spacing 64, Corners 18, Shadow on
- Cursor = Size 1.6, Smoothing 0.8, sway 0.4
- Zoom = Automatic, Intensity 1.6
- Left click = Ripple; Right = Orb

**Tutorial / class**
- Background = Solid color, Corners 12
- Keys = on, Time 2000 ms
- Click sound = on (volume 0.5)
- Zoom = Automatic, Intensity 1.4

**Presentation / meeting**
- Background = Dark color, Spacing 80
- Zoom = Off
- Spotlight = available by hotkey (when you want to highlight)

**Live coding**
- Keys = on, Max 6
- Zoom = light Automatic (1.3)
- Highlighter = on to highlight code snippets

---

## 10. Linux: permission for the key overlay

The overlay reads keys from `/dev/input` (evdev), which works on X11 **and**
Wayland. It requires permission:

```bash
sudo cp data/udev/99-topscreen.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
# logging back in helps
```

Without this, the plugin warns in the log (`evdev unavailable`) and the overlay
does not appear. To test anyway:

```bash
TOPSCREEN_DEBUG_KEY=Ctrl obs
```

---

## 11. Performance

- The effects run on the **GPU**, in the OBS process (no recording, no copy).
- Higher cost: **shadow** (blur) and **motion blur**. If the PC is weak:
  reduce the **Shadow blur** or turn off **Movement**.
- High resolutions + a 60 Hz monitor are fine; 120/144 Hz monitors cost more.
- In software rendering (no GPU, e.g. `llvmpipe`), everything works, but slower.

---

## 12. Troubleshooting

**The filter does nothing (passes straight through)**
- Check the log: `obs --verbose 2>&1 | grep -i topscreen`. If
  `failed to create the effect` appears, the file `data/effects/topscreen.effect`
  was not found — reinstall.

**Windows: the source disappears when the filter is enabled**
- Fixed in **0.2.4**. The D3D11 device refuses to draw when *any* scalar
  shader parameter was never uploaded (`device_draw (D3D11): Not all shader
  parameters were set`), which left the filter's output empty (the source
  looked transparent while clicks still played). Every uniform is now
  uploaded on every frame and unused textures get a 1x1 dummy binding.
  If you see it on an older build, install the latest release.

**The plugin does not show up / "Duplicate library"**
- You have the plugin installed in two places. Keep only **one**
  (per-user **or** system). See [`BUILD.md`](BUILD.md).

**I see two cursors**
- The capture had the native cursor enabled. TopScreen turns off
  `capture_cursor`/`show_cursor` automatically — if your source uses another
  name, turn off the source's cursor option manually.

**The cursor lags**
- **Smoothing** at `0` makes the cursor follow the mouse **exactly**. Higher
  values add glide (on purpose).
- **Curve tilt** at `0` removes the tilt.

**Zoom cuts off the corners / the cursor disappears at the edge**
- This has been fixed: the focus now **follows** the cursor to the edges
  (the visible window always fits on screen). Reduce **Pan intensity** only if
  you want smoother tracking in the middle of the screen.

**No keys in the overlay (Linux)**
- Apply the udev rule from §10.

**The dock does not appear**
- The build did not have Qt6/obs-frontend-api. Install `qt6-base-dev` and reinstall.

**Zoom "jumps" or shakes**
- Increase **Speed (ms)** and reduce **Intensity**.

---

## 13. FAQ

**Do I need to record?**
No. TopScreen only delivers the image to OBS. OBS is what records/streams.

**Does it let me edit afterwards?**
No — by definition it is **live only**. For later editing use an editor.

**Does it work with "Window Capture" or "Game Capture"?**
Yes, the filter works on any video source. The screen selection is yours, on the source.

**Can I use two TopScreen filters at the same time?**
Yes, but each one is independent; the dock controls the **first** active filter.

**Can I turn off only the background at a specific moment?**
Yes: hotkey **Enable/disable background**.

**Windows: do I need admin?**
No, the default installer uses the per-user plugins folder.

---

## 14. Current limitations

- **Webcam bubble** and **zoom into a region** have not been implemented yet.
- The **click sound** uses the system audio (not the source's audio channel).
- **Global hotkeys on Wayland** depend on the compositor (portal not implemented yet).
- The **own capture source** (choosing the screen inside TopScreen) is
  planned; today you use OBS's Display Capture.

See what is ready in [`TASKS.md`](TASKS.md) and how to verify it in
[`VERIFICATION.md`](VERIFICATION.md).
