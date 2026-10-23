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
1. Get `topscreen-<version>-windows.zip` from the [`releases/`](../releases/README.md) folder and extract the **whole zip**.
2. Open the **`topscreen`** folder and double-click **`install.bat`** (it sits next to `bin` and `data`).
   Drag your portable OBS folder onto it for portable installs, or drag a `topscreen.dll`
   to install a specific build. Remove later with `uninstall.bat` in the same folder.

Cloned the source instead? Running `scripts\install-windows.bat` also works — it
looks for a compiled `topscreen.dll` and, if there is none, tells you exactly how
to proceed. Details in [`BUILD.md`](BUILD.md).

---

## 3. Getting started

1. In OBS, add a **Display Capture** source and pick the screen.
2. Right-click the source → **Filters** → **+** → **TopScreen FX**.
3. Adjust in **Properties** (the tabs/groups are described in §5).
4. Create hotkeys in **Settings → Hotkeys** (search for `TopScreen`, §6).
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
When the effects are enabled **and Cursor → Show custom cursor is on**, TopScreen turns off
the native cursor of the capture and draws its own, smoothed and scalable. That
is why you see **a single cursor**. Turning **Effects enabled** off, turning the
TopScreen cursor off, or removing the filter restores the capture's original
cursor setting.

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
| **Background type** | **Solid color**, **Two-color gradient** or **Wallpaper** |
| **Color 1 (solid / gradient start)** | Background color for solid, or the first gradient color |
| **Color 2 (gradient end)** | Second gradient color (gradient only) |
| **Gradient angle (degrees)** | Direction of the two-color gradient (0–360°) |
| **Gradient meeting point X/Y** | Moves the point where the two colors meet (0 = left/top, 1 = right/bottom) |
| **Wallpaper preset** | **Transparent (use the scene below)**, **No wallpaper**, or one of the **18 images** that ship with the plugin. Choosing one applies it |
| **Custom wallpaper file** | Button to choose a **file from your PC** (jpg/png/bmp/tga/gif/webp). Choosing one **clears** the wallpaper preset (they are mutually exclusive) |
| **Wallpaper blur radius (px)** | How far the background image spreads (0 = sharp) |
| **Wallpaper diffusion** | No blur · Soft (fast) · **Gaussian (balanced)** · Creamy (wide) · Frosted glass (smoothest). The wallpaper is prefiltered once when the image or blur settings change, then sampled once per pixel |
| **Horizontal / vertical screen margin (px)** | Margin between the screen and the edge (separate X and Y) |
| **Keep screen aspect ratio** | Avoids stretching the screen: it is scaled down proportionally (leave it on) |
| **Screen corner radius (px)** | Radius of the captured screen corners |
| **Window shadow** (sub-group) | |
| ↳ **Enable shadow** | Turns the shadow behind the screen on/off |
| ↳ **Shadow opacity (0–1)** | How strong the shadow is (0 = invisible, 1 = strong) |
| ↳ **Shadow blur (px)** | How soft and large the shadow is |
| ↳ **Shadow offset X (px)** | Moves the shadow left/right |
| ↳ **Shadow offset Y (px)** | Moves the shadow up/down (positive = down) |

> The background appears **behind** the screen. **Solid** uses Color 1;
> **Gradient** blends Color 1 → Color 2 through the meeting point and angle;
> **Wallpaper** uses the image controls.
> **Transparent** removes the background/padding and lets the scene below show through; the captured screen, corners, shadow, cursor, clicks and keys stay on top.

### Cursor
| Option | What it does |
|-------|-----------|
| **Show custom cursor** | Draws the own cursor (turns off the native one) |
| **Cursor style** | Primary cursor: one of the 79 cursors (see below) |
| **Secondary cursor (links)** | Cursor used while the native pointer is over a link; "Same as primary" disables the swap |
| **Change cursor over links** | Automatic on **Windows and X11**. Wayland has no portable link-cursor signal, so the primary stays active |
| **Cursor movement** | **Exact**, **Spring smoothing**, **Glide (sampled)** or **Glide (A to B)** |
| **Cursor size (1 = default)** | Cursor scale (1.0 = default) |
| **Pointer correction X/Y (px)** | Fine adjustment if the recorded pointer is a few pixels away from the real one |
| **Smoothing (0 = exact tracking)** | Spring mode: 0 = follows exactly; above that, it glides |
| **Glide duration / sample interval** | A→B duration and how often the sampled glide takes a new destination |
| **Curve tilt (sway)** | The cursor tilts slightly when turning |
| **Cursor shadow** | Shadow to make it stand out from the background |
| **Hide cursor when idle** | Disappears when you stop moving |
| **Idle delay before hiding (ms)** | How long the cursor stays still before hiding |
| **Return cursor to start when idle** | Cursor "returns" to where it started (useful for looping videos) |
| **Idle time before returning (ms)** | How long the cursor stays still before returning |

**Cursor styles available (79)**

The dropdown is **grouped by theme**: every cursor of a theme sits together
(all Bibata first, then all Breeze, and so on), and the label always starts
with the theme name, so it is easy to scan.

| Style | Appearance |
|--------|-----------|
| Bibata — Arrow | Modern rounded arrow (Bibata Modern Classic) |
| Bibata — Link hand | Bibata pointing hand (hover/link) |
| Bibata Ice — Arrow | Bibata with a blue highlight |
| Bibata Ice — Link hand | Bibata Ice pointing hand |
| Bibata Original — Arrow | Bibata in its original shape |
| Bibata Original — Link hand | Bibata Original pointing hand |
| Bibata Ice Original — Arrow | Bibata Ice, original shape |
| Bibata Ice Original — Link hand | Bibata Ice Original pointing hand |
| Breeze — Arrow | Arrow from the Breeze theme (KDE) |
| Breeze — Link hand | Breeze pointing hand |
| Breeze Snow — Arrow | Very light Breeze arrow |
| Breeze Snow — Link hand | Breeze Snow pointing hand |
| Adwaita — Arrow | Arrow from the Adwaita theme (GNOME) |
| Adwaita — Link hand | Adwaita pointing hand |
| Yaru — Arrow | Arrow from the Yaru theme (Ubuntu) |
| Yaru — Link hand | Yaru pointing hand |
| DMZ (white) — Arrow | Classic arrow from the DMZ-White theme |
| DMZ (white) — Link hand | DMZ White pointing hand |
| DMZ (black) — Arrow | DMZ theme, black variant |
| DMZ (black) — Link hand | DMZ Black pointing hand |
| Google Dot (white) — Arrow | Circle with a dot (Google style) |
| Google Dot (black) — Arrow | Google-style dot, black variant |
| XCursor Pro — Arrow | Arrow from the XCursor-Pro theme |
| XCursor Pro — Link hand | XCursor Pro pointing hand |
| macOS — Classic arrow | Black arrow with a white outline |
| Windows — Classic arrow | White arrow with a black outline |
| Effects — Neon | Glowing cyan arrow for dark scenes |
| Effects — Onyx | Elegant dark arrow with a gold outline |
| Effects — Dot | Coral disc, centered (touch-style) |

> The **Link hand** styles are the *same theme's own* pointer cursor, taken from
> the theme package — so the "hovering a link" look matches the arrow you picked.
> Google Dot ships no hand at all (every cursor in that theme is a dot), so it
> has no link variant.

> The 50 custom styles are listed in [`CURSORS.md`](CURSORS.md); removed styles keep their IDs reserved for old profiles.
> The bitmaps are generated by `tools/make-cursor-atlas.py` (reads cursor themes
> installed in `/usr/share/icons` plus `Resources/cursors/`) and live in
> `data/cursors/`.

### Left click / Right click
Each button has its own independent effect.

| Option | What it does |
|-------|-----------|
| **Click effect** | Ripple, Orb, Ring, Force, Glow, Echo (double ring), Flare (spark), **Quiet circle**, **Water ripple** (colored ring with distortion) or **Water ripple (clear)** (distortion only, no color) |
| **Effect size / color / duration / animation curve** | Appearance and timing of the effect |

### Click sound
| Option | What it does |
|-------|-----------|
| **Enable click sound** | Plays a short click |
| **Left / Right click sound** | 26 bundled sounds — real mouse recordings (tight/single/right), Retro, Optical, Muted, Soft, Hard, Tick, Bubble, Thock, plus the remaining custom pack (typewriter, stapler, old camera, etc.) |
| **Click sound volume (0–1)** | Sound volume (applied on paplay; other system players play as-is) |
| **Play on left / right click** | Which buttons trigger it |

> Switching sounds applies immediately (no restart needed).

> The sound plays through the **system audio** (it enters OBS through your Desktop
> Audio capture). It does not yet come out through the source's own audio channel.

### Zoom
| Option | What it does |
|-------|-----------|
| **Zoom mode** | `Off`, `Automatic` (follows clicks/cursor) or `Manual` |
| **Zoom intensity (1 = no zoom)** | How much it magnifies (1.0 = no zoom) |
| **Zoom transition time (ms)** | Animation duration |
| **Return to normal after idle (ms)** | After stopping, returns to normal |
| **Pan intensity (0 = off)** | How much the screen "follows" the mouse |
| **Centering (0 = follows mouse, 1 = centered)** | 0 = follows the mouse; 1 = keeps the mouse in the **center** (the view **anticipates** the edges — the corner appears before you get to it) |

### Movement (motion blur)
| Option | What it does |
|-------|-----------|
| **Enable motion blur** | Radial blur during zoom |
| **Motion blur intensity (0–1)** | Blur strength |

### Spotlight
| Option | What it does |
|-------|-----------|
| **Enable spotlight** | Darkens everything except a shape that follows the cursor |
| **Spotlight shape** | **Circle** or **Square** |
| **Radius (fraction of screen)** | Size of the spotlight (fraction of the shorter screen side) |
| **Square corner radius (px)** | Rounded corners for the square shape |
| **Darken outside (0 = off, 1 = black)** | How much the outside area is darkened |
| **Border** | Enables a colored ring around the spotlight, with color and width |
| **Outer halo shadow** | Soft halo around the spotlight, with halo color, opacity, blur and X/Y offset |
| **Blur outside** | Blurs the whole frame, including the wallpaper, outside the spotlight |
| **Zoom when active (x)** | Extra multiplicative zoom while the spotlight is on |
| **Drop/zoom in / out (ms)** | Durations of the activation/deactivation animation |
| **Vertical drop distance (px)** | Vertical distance the spotlight drops in/out |
| **Show cursor during spotlight** | Keep the recorded cursor visible while the spotlight is active |

The spotlight mask, blur, dim, border, halo and zoom affect the whole frame,
including the wallpaper. Click effects and highlights stay crisp on top. Assign `TopScreen_ToggleSpotlight` in **Settings → Hotkeys** to toggle it
instantly. Activating it drops the shape in, zooms in around the cursor and fades
the border/shadow/dim in; deactivating reverses the animation. The spotlight
mask and zoom focus keep following the mouse while it is active. The global
**Effects enabled** master still applies.

### Magnifier
| Option | What it does |
|-------|-----------|
| **Enable magnifier lens** | Zooms only inside a circle/square; the area outside stays normal |
| **Lens shape** | **Circle** or **Square** |
| **Radius (fraction of screen)** | Size of the lens |
| **Magnification (x)** | Zoom inside the lens |
| **Follow cursor / fixed center** | Lens follows the pointer or stays at a fixed X/Y position |
| **Border / shadow** | Optional ring and soft halo around the lens |
| **Zoom in / out (ms)** | Activation/deactivation transition |
| **Vertical drop distance (px)** | Drop-in/out animation distance |
| **Show cursor during magnifier** | Keep the recorded cursor visible while the lens is active |

The lens magnifies both the wallpaper and the captured screen inside its shape.
Click effects, highlights, the key overlay and the cursor stay crisp on top and
are not magnified. Assign `TopScreen_ToggleMagnifier` in
**Settings → Hotkeys** to toggle it. The magnifier and the spotlight are mutually exclusive: enabling one disables the
other. The global **Effects enabled** master still applies.

### Keys
| Option | What it does |
|-------|-----------|
| **Show typed keys** | Shows what you type on screen |
| **Modifiers only** | Shows only Ctrl/Shift/Alt/Win |
| **Show special keys** | On: Enter, Tab, arrows and similar keys appear by name. Off: those keys are ignored. Space is always shown as a blank space; modifiers are handled by the shortcut option |
| **Show shortcuts** | On: held modifiers are folded into one entry such as `Ctrl + C`; Shift/Ctrl are not shown twice. Off: each modifier and key is shown individually |
| **On-screen time per key (ms)** | How long each key stays |
| **Maximum keys on screen** | How many simultaneous keys to show |

The key overlay is drawn above the cursor so typed text always stays readable.
Every keystroke appears as its own character (repeats included) exactly as typed,
then fades out after the on-screen time. Letters, digits and punctuation such as
`@#!?` are shown as text. Special keys such as `Enter`, `Tab` and `Left` are
shown by name, and Space is a literal blank space. With **Show shortcuts** on,
held modifiers become one entry such as `Ctrl + Shift + C`; with it off, each
modifier is shown once and each normal key follows individually. The Overlays
section lists Highlighter before Keystrokes.
| **Horizontal position (0 = left, 1 = right)** | Where the band sits horizontally |
| **Vertical position (0 = top, 1 = bottom)** | Where the band sits vertically |
| **Text size (1 = default)** | Text scale |
| **Font** | DejaVu Sans Bold / Sans / ExtraLight / Mono Bold / Mono / Serif Bold, or a path to a `.ttf` |
| **Text color** | Letter color |
| **Text shadow** | Dark outline for readability |
| **Show background band / Band color** | Rounded rectangle behind the text and its color |

### Highlighter (drag + hotkey)

| Group | Option | What it does |
|-------|--------|--------------|
| — | **Enable drag actions** | Master switch for what a left-drag does |
| — | **Enable hotkey highlight** | Independent extra: assign `TopScreen_ToggleHighlight`. First press zooms at the current cursor; second press releases/returns. It works with drag actions on or off, and the Zoom group stays visible so those values can be tuned in any drag mode. |
| — | **Drag action** | **Filled box** (classic mark) · **Line** (draw freehand, fades out) · **Zoom to the selection** (no drawing) · **Zoom on mouse press** (hold the button, no drag) · **Box + zoom** (marks *and* zooms) · **Line + zoom** (draws freehand *and* zooms) |
| Mark appearance | **Mark color** | Highlight color (use alpha < 255 for translucent) |
| Mark appearance | **Mark opacity (0 = invisible, 1 = solid)** | Strength ceiling; keeps content readable |
| Mark appearance | **Line thickness (fraction of the screen)** | Stroke width as a fraction of the screen height |
| Mark timing | **Mark stays on screen (ms)** | How long the mark stays after you release |
| Mark timing | **Mark fade-out (ms)** | How long the fade takes |
| Zoom | **Zoom direction** | **Zoom in** (hold to zoom, release to come back) · **Zoom out** (hold to widen) · **Punch in/out** (same, but instant — no hold needed before) |
| Zoom | **Zoom: hold-to-zoom delay** | Hold the button this long (ms) before the zoom kicks in (`0` = behaves like punch) |
| Zoom | **Zoom on press: hold delay** | In *Zoom on mouse press*, how long the button stays down before the zoom starts (default 400 ms) |
| Zoom | **Zoom: maximum** | Zoom ceiling for the gesture |
| Zoom | **Zoom: zoom-in time** | How fast the zoom moves in |
| Zoom | **Zoom: return time** | How slowly it glides back on release |
| Zoom | **Zoom: hold at target** | Extra still time after release before gliding back |

- **Hold-to-zoom in practice:** switch the drag action to **Zoom to the
  selection**, then press the left button and **hold**. After the configured
  delay the view eases into the zoom (fitted to your drag if you drew one,
  otherwise centered where the pointer is) and **stays there while you hold**.
  Let go and it glides back to exactly where it was — even if you were
  already zoomed when you started.
- **Zoom on mouse press in practice:** choose **Zoom on mouse press** and just
  press and hold the left button — no dragging required. After the short hold
  delay (400 ms by default) the view zooms at the point where you pressed and
  stays there while the button is down; release and it glides back. Dragging
  during the hold does **not** change the focus, and a quick click does
  nothing (it stays a normal click).
- Only the options of the selected mode are shown; switching mode hides the
  rest.

**Drag action in practice**
- **Filled box**: marks a region (classic).
- **Line**: draw small shapes/underlines with the mouse; the stroke fades out.
  Great for circling something on screen.
- **Zoom to the selection**: drag over an area — or just press and hold — and
  TopScreen zooms into it, **holds while you keep the button down**, and glides
  back on release.
- **Zoom on mouse press**: pure press-and-hold. No area selection: the zoom is
  centered on the point where you pressed, ignores any drag, and starts after
  the configured short delay. A quick click stays a click.
- **Box + zoom**: draws the box *and* zooms, for when you want to mark the area
  you are inspecting.
- **Line + zoom**: same gesture as *Box + zoom*, but the mark is the freehand
  stroke instead of the box — draw the shape and the view zooms into the
  **area you covered**. Circling something works even though the stroke ends
  where it started, because the selection is the stroke's bounding box.
  - **Zoom direction** picks the motion: *Zoom in* / *Zoom out*, or *Punch in* /
    *Punch out* (no hold: it snaps and returns immediately).
  - The **centre of the dragged area** always ends up in the **centre of the
    screen** (the focus follows the same centering law as the auto-zoom).
  - The gesture always starts from **what is on screen at that moment** and
    returns to it, so selecting while already zoomed works as expected.
  - Press and hold **without** drawing: it zooms where the pointer is and
    nothing is drawn (there is no stroke to show).
  - Max zoom and the three timings (in / hold / out) are configurable; in and
    out are independent.

---

## 6. Hotkeys

In **Settings → Hotkeys**, search for `TopScreen`: all 21 actions are labelled
**`TopScreen_...`** (for example `TopScreen_ProfileA — Activate profile A`), so
they are easy to spot. They are global hotkeys, so they
act on the **target filter selected in the dock** (or the first active one
when nothing is selected):

| Action | Effect |
|------|--------|
| **Enable/disable effects** | Turns everything on/off (master) |
| **Enable/disable background** | Turns only the background/corners on/off |
| **Zoom in / Zoom out / Reset zoom** | Manual zoom |
| **Next preset / Previous preset** | Switch preset |
| **Activate profile A … J** | Applies the profile (all of its settings) |
| **Save to current profile** | Writes the adjustments to the active profile |
| **Toggle highlight** | Independent zoom toggle: first press zooms at the cursor, second press releases/returns (enable it under Highlighter) |
| **Toggle spotlight** | Drops/zooms the spotlight in or out at the cursor |

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

## 8. Profiles (A–J)

The **10 profiles** store the entire configuration and switch everything at once.
The selector is at the **top** of Properties (group **Profiles → Current profile**)
and in the dock as well. The dock also has a **Target filter** selector: all dock
buttons and hotkeys act on the selected TopScreen filter (or the first one when
nothing is selected).

- **Switching profile applies EVERYTHING immediately** (background, cursor, clicks, zoom, keys, click sounds…).
- **Your changes are saved into the active profile automatically** — no need to
  remember to save (the *Save to profile* button and hotkey exist to force it).
  The auto-save is debounced by ~0.4 s, so dragging a slider does not rewrite
  the profile on every frame; switching profiles/saving/exporting flushes it.
- The selected profile is re-applied **when OBS opens**, so the look comes back the same.
- Hotkeys: **Activate profile A … J** and **Save to current profile**.

### Backup / transfer

- **Dock:** `Export profiles...` saves all 10 profiles to a `.json` file;
  `Import profiles...` reads one back. The import asks for confirmation,
  replaces profiles **A–J** and activates the profile that was active in the
  file.
- **Properties (no Qt dock):** group **Profiles → Backup / transfer** — choose
  the file in *Export to file* / *Import from file* and click the corresponding
  button.
- The file stores **settings only** (background, cursor, clicks, zoom, keys,
  highlighter, profiles). Custom images and sounds are referenced by path and
  are **not embedded**.
- Export includes the edits currently on screen (the active profile is saved
  into the file first).
- Older or partial files import safely: a missing `current` starts at A,
  missing slots use their default seed, and missing fields keep that seed's
  defaults instead of becoming zero.

They already ship with examples, so you can just switch and see the difference:

| Profile | Example |
|---|---|
| **A** | default |
| **B** | **green/orange** clicks + **zoom 2.0** |
| **C** | **purple/cyan** clicks + zoom 1.4 and vertical gradient |
| **D** | **yellow/white** clicks and **zoom off** |
| **E** | **blue/orange** clicks + zoom 1.8, solid background |
| **F** | **magenta/lime** clicks + zoom 2.5, vertical gradient |
| **G** | white/pink clicks + gentle zoom 1.25, green highlighter |
| **H** | teal/red clicks + zoom 3.0, click sound off |
| **I** | violet/gold clicks, **neon cursor**, manual zoom |
| **J** | green/black clicks, **wallpaper background**, crosshair cursor |

---

## 9. Recipes

**Product demo (similar to "Screen Studio")**
- Background = Two-color gradient (45°), screen margin 64, corner radius 18, shadow on
- Cursor = size 1.6, smoothing 0.8, sway 0.4
- Zoom = Automatic, intensity 1.6
- Left click = Ripple; Right = Orb

**Tutorial / class**
- Background = Solid color, screen corner radius 12
- Keys = on, on-screen time 2000 ms
- Click sound = on (volume 0.5)
- Zoom = Automatic, intensity 1.4

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
- Higher cost: **shadow** (blur), **motion blur** and the full-frame spotlight
  blur. If the PC is weak: reduce the **Shadow blur**, turn off **Movement**,
  or reduce the spotlight **Blur outside**. Wallpaper diffusion is prefiltered
  once, so it is much cheaper than the old per-pixel kernels.
- High resolutions + a 60 Hz monitor are fine; 120/144 Hz monitors cost more.
- In software rendering (no GPU, e.g. `llvmpipe`), everything works, but slower.

---

## 12. Troubleshooting

**The filter does nothing (passes straight through)**
- Check the log: `obs --verbose 2>&1 | grep -i topscreen`. If
  `failed to create the effect` appears, the file `data/effects/topscreen.effect`
  was not found — reinstall.

**Cursor, clicks or zoom are offset on a second monitor**
- Fixed in **0.2.7**: the capture now contributes its monitor origin, so a
  screen placed **below**, **left** or **above** the primary one maps the mouse
  correctly (previously the vertical axis used the total desktop height).
- It is automatic: the monitor is matched by device name (Windows) or by the
  capture size (X11/Xinerama). The OBS log lists every screen once, e.g.
  `monitor 1: 1920x1080 at (0,1080)`.
- If it is still off (e.g. two identical monitors stacked), open the filter
  **Properties → Monitor (multi-screen)**, tick **Set the capture position
  manually** and type the **Origin X/Y** from that log line (`0,1080` for the
  screen below the primary one; negative values are allowed). Manual values win
  over the automatic detection.
- `TOPSCREEN_MONITOR_ORIGIN=x,y` (environment variable) does the same without
  touching the scene, handy for support.

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
- The build did not have Qt6/obs-frontend-api. Install `qt6-base-dev` (Linux) and
  reinstall, or use a release package.
- Linux packages include the dock. Windows CI builds do too; the locally
  cross-built Windows packages (0.2.18 and newer) do **not** include the dock,
  but the full filter still works.
- On Windows the dock lists under **View → Docks → TopScreen**.

**Hotkeys: I cannot find the TopScreen actions**
- The hotkeys only exist while at least one source has the filter: add
  **TopScreen FX** to a Display Capture first.
- They are **global frontend hotkeys** (filters cannot register per-source
  hotkeys in OBS), so they appear as flat entries in the main list — search
  for `TopScreen` or `Activate profile`.
- With several filters, the hotkey acts on the dock's **Target filter**
  (or the first active one when nothing is selected).
- The log confirms registration: `[TopScreen] hotkeys registered: 20/20`.

**Zoom "jumps" or shakes**
- Increase **Zoom transition time (ms)** and reduce **Zoom intensity**.

---

## 13. FAQ

**Do I need to record?**
No. TopScreen only delivers the image to OBS. OBS is what records/streams.

**Does it let me edit afterwards?**
No — by definition it is **live only**. For later editing use an editor.

**Does it work with "Window Capture" or "Game Capture"?**
Yes, the filter works on any video source. The screen selection is yours, on the source.

**Can I use two TopScreen filters at the same time?**
Yes, each one is independent. The dock's **Target filter** selector chooses
which one the dock buttons and hotkeys control (default: the first one).

**Can I turn off only the background at a specific moment?**
Yes: hotkey **Enable/disable background**.

**The hotkey highlight does nothing**
- Enable **Highlighter → Enable hotkey highlight** and assign
  `TopScreen_ToggleHighlight` in **Settings → Hotkeys**.
- It uses the highlighter Zoom settings. With **Zoom direction = out** at 1×
  there is nothing to zoom out from; use **in** (default) or zoom in once
  manually before pressing.
- The global **Effects enabled** master must be on; with effects off the hotkey
  does nothing.
- Manual zoom and a left-button drag take over the zoom and clear the toggle;
  press the hotkey again to start a new one.

**I turned effects off (or the cursor off) and now there is no cursor**
- TopScreen restores the capture's original native cursor when **Effects
  enabled** is off, when **Cursor → Show custom cursor** is off, and when the filter is
  removed. The "effects off" case was fixed in **0.3.0**; the cursor-off case
  and multiple filters sharing a capture were fixed in **0.3.1**.

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
