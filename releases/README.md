# TopScreen releases

Ready-to-use packages, versioned in this folder (no build required).
Each version keeps its files; newer versions are added alongside. The newest
archives are also attached to
[GitHub Releases](https://github.com/ticosanjr/topscreen/releases).

| Version | Linux | Windows | Highlights |
|---------|-------|---------|------------|
| **0.2.22** | [`topscreen-0.2.22-linux.tar.gz`](topscreen-0.2.22-linux.tar.gz) | [`topscreen-0.2.22-windows.zip`](topscreen-0.2.22-windows.zip) \* | **Hotkeys:** if OBS cannot load the locale, the list falls back to the literal `TopScreen_*` ID instead of showing `Hotkey.*`; **Window shadow:** separate horizontal offset (`bg_shadow_offset_x`) in addition to the existing vertical offset |
| **0.2.21** | [`topscreen-0.2.21-linux.tar.gz`](topscreen-0.2.21-linux.tar.gz) | [`topscreen-0.2.21-windows.zip`](topscreen-0.2.21-windows.zip) \* | **Dock target selector** (dock buttons and hotkeys follow the chosen filter), **debounced profile auto-save** (~400 ms, flushed on switch/save/export/import/destroy), registry fallback tests. **Performance:** key overlay without per-frame heap allocations, cached X11/Xinerama display, O(1) ASCII glyph lookup, batched text glyph draws |
| **0.2.20** | [`topscreen-0.2.20-linux.tar.gz`](topscreen-0.2.20-linux.tar.gz) | [`topscreen-0.2.20-windows.zip`](topscreen-0.2.20-windows.zip) \* | **Correctness:** Smoothing/Sway 0.7/0.3 are no longer reset, profile import validates size/depth, sound files are not re-read on every edit, no global `SIGCHLD` change, UTF-8 paths on Windows. **Performance:** key overlay no longer allocates per frame. **Tests/CI:** export/import round-trip, migration/property regressions, ASan+UBSan |
| **0.2.19** | [`topscreen-0.2.19-linux.tar.gz`](topscreen-0.2.19-linux.tar.gz) | [`topscreen-0.2.19-windows.zip`](topscreen-0.2.19-windows.zip) \* | **Export/import profiles** (all 10 slots, dock buttons + Properties fallback), hotkey labels renamed to `TopScreen_<Action>` (visible as typed in OBS), previous 0.2.18 features included |
| **0.2.18** | [`topscreen-0.2.18-linux.tar.gz`](topscreen-0.2.18-linux.tar.gz) | [`topscreen-0.2.18-windows.zip`](topscreen-0.2.18-windows.zip) \* | **Version visible in Properties and the dock**; **cursor list grouped by theme** (all Bibata together, clear names); every hotkey labeled `TopScreen.<Action>` so it is easy to find; new highlighter mode **Zoom on mouse press** (hold 400 ms, no dragging); highlighter zoom no longer cancelled by unrelated hotkeys |
| **0.2.17** | [`topscreen-0.2.17-linux.tar.gz`](topscreen-0.2.17-linux.tar.gz) | [`topscreen-0.2.17-windows.zip`](topscreen-0.2.17-windows.zip) \* | **10 new photographic wallpapers** (34 total: Silk, Pastel, Helix, Fiber, Prism, Twilight, Ink, Spark, Cobalt, Dusk), 1920x1080, provenance in `data/wallpapers/SOURCES.txt` |
| **0.2.16** | [`topscreen-0.2.16-linux.tar.gz`](topscreen-0.2.16-linux.tar.gz) | [`topscreen-0.2.16-windows.zip`](topscreen-0.2.16-windows.zip) \* | **11 link-hand cursors** (the same themes' own pointer, at last 37 styles) and **3 real mouse-click recordings** (CC0, `sounds/click-mouse-*.wav`) |
| **0.2.15** | [`topscreen-0.2.15-linux.tar.gz`](topscreen-0.2.15-linux.tar.gz) | [`topscreen-0.2.15-windows.zip`](topscreen-0.2.15-windows.zip) \* | **Line + zoom** drag mode (draws the stroke *and* zooms, like Box + zoom), line zoom selection = the stroke's bounding box, so circling an area zooms into it; single set of mode predicates shared by the renderer and the settings UI, pinned by tests |
| **0.2.14** | [`topscreen-0.2.14-linux.tar.gz`](topscreen-0.2.14-linux.tar.gz) | [`topscreen-0.2.14-windows.zip`](topscreen-0.2.14-windows.zip) \* | **line mode drawn by the main shader pass** (freehand trail as polyline segments — no separate effect/texture/draw call), line decimation unit-tested for the shader slots, dead highlighter overlay removed |
| **0.2.13** | [`topscreen-0.2.13-linux.tar.gz`](topscreen-0.2.13-linux.tar.gz) | [`topscreen-0.2.13-windows.zip`](topscreen-0.2.13-windows.zip) \* | working global hotkeys (frontend registration), Box+zoom drag mode, conditional highlighter UI, hold-to-zoom release glide |
| **0.2.12** | [`topscreen-0.2.12-linux.tar.gz`](topscreen-0.2.12-linux.tar.gz) | [`topscreen-0.2.12-windows.zip`](topscreen-0.2.12-windows.zip) \* | box never paints in line mode (dedicated overlay verified end-to-end); hold-to-zoom waits for the delay, stays while held, glides back on release from the current zoom; **Box + zoom** mode (marks *and* zooms); conditional highlighter UI |
| 0.2.11 | [`topscreen-0.2.11-linux.tar.gz`](topscreen-0.2.11-linux.tar.gz) | [`topscreen-0.2.11-windows.zip`](topscreen-0.2.11-windows.zip) \* | line mode never paints the box, hold-to-zoom that starts on hold and glides back on release, zoom in/out/punch directions, settings reordered with conditional groups |
| 0.2.10 | [`topscreen-0.2.10-linux.tar.gz`](topscreen-0.2.10-linux.tar.gz) | [`topscreen-0.2.10-windows.zip`](topscreen-0.2.10-windows.zip) \* | line mode no longer paints the box, first-frame line overlay guard, zoom centre = selection centre |
| 0.2.9 | [`topscreen-0.2.9-linux.tar.gz`](topscreen-0.2.9-linux.tar.gz) | [`topscreen-0.2.9-windows.zip`](topscreen-0.2.9-windows.zip) \* | line drawing fixed (1:1 overlay), zoom gesture directions (in/out/punch in/punch out) starting from the current zoom, clearer/reordered settings |
| 0.2.8 | [`topscreen-0.2.8-linux.tar.gz`](topscreen-0.2.8-linux.tar.gz) | [`topscreen-0.2.8-windows.zip`](topscreen-0.2.8-windows.zip) \* | 6 new realistic mouse clicks (14 total), 10 new cursors (26 total), highlighter **line** mode, **zoom to the selection** gesture, manual monitor origin |
| 0.2.7 | [`topscreen-0.2.7-linux.tar.gz`](topscreen-0.2.7-linux.tar.gz) | [`topscreen-0.2.7-windows.zip`](topscreen-0.2.7-windows.zip) \* | multi-monitor input fix (cursor/clicks/zoom on stacked or side-by-side screens) + manual monitor origin |
| 0.2.6 | [`topscreen-0.2.6-linux.tar.gz`](topscreen-0.2.6-linux.tar.gz) | [`topscreen-0.2.6-windows.zip`](topscreen-0.2.6-windows.zip) | first build with the Windows Qt dock; 10 profiles |

## Install (Windows, no compiler, no admin)

1. Download `topscreen-<version>-windows.zip`.
2. **Extract the whole zip** and open the `topscreen` folder.
3. Double-click **`install.bat`** — it sits next to `bin` and `data`, so nothing
   has to be moved.
   - Drag your **OBS portable** folder onto it to install there instead.
   - Remove later with `uninstall.bat` in the same folder.
4. Restart OBS → add a **Display Capture** → **Filters → + → "TopScreen FX"**.

CI-built Windows packages include the Qt dock: **View → Docks → TopScreen**.
The 0.2.18–0.2.22 packages were cross-built locally and do **not** include the
dock; the full filter still works normally.

> \* **0.2.7 – 0.2.10, 0.2.18, 0.2.19, 0.2.20, 0.2.21 and 0.2.22 Windows**: built by cross-compiling from Linux
> (`scripts/build-windows-cross.sh`) because the GitHub Actions minutes were
> blocked. It is the full filter (Properties, hotkeys, presets, profiles,
> multi-screen fix) but **without the Qt dock**. A CI-built package would include
> the complete dock. If the dock matters to you, use the 0.2.6 package until a
> CI-built Windows package is available.

## Install (Linux, no sudo)

```bash
tar -xzf topscreen-<version>-linux.tar.gz -C ~/.config/obs-studio/plugins/
# optional (keystroke overlay): allow reading /dev/input
sudo cp data/udev/99-topscreen.rules /etc/udev/rules.d/ 2>/dev/null || true
sudo udevadm control --reload-rules && sudo udevadm trigger
```

Then restart OBS and add the filter as above.

## Adding a new version

```bash
git pull
VERSION=$(sed -n 's/^project(TopScreen VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)

./scripts/build-install.sh          # builds build/topscreen.so (with the Qt dock)
./scripts/build-windows-cross.sh    # builds build/topscreen.dll (without the dock)
./scripts/package.sh                # writes both archives into releases/
./tests/run-all.sh

git add -A
git commit -m "release: $VERSION"
git push origin main

# Publish without Actions (the tag would otherwise trigger the Release workflow).
gh workflow disable "Release"
git tag -a "v$VERSION" -m "TopScreen $VERSION"
git push origin "v$VERSION"
gh release create "v$VERSION" --verify-tag \
  --title "TopScreen $VERSION" \
  --notes-file /tmp/topscreen-release-notes.md \
  "releases/topscreen-$VERSION-linux.tar.gz" \
  "releases/topscreen-$VERSION-windows.zip"
gh workflow enable "Release"
```

`scripts/package.sh` writes to this folder by default (`--out DIR` to override).
Version numbers come from `CMakeLists.txt` (`project(TopScreen VERSION ...)`).
The GitHub Release and the archives here carry the same bytes.

## Notes

- These archives are the canonical, versioned download. The newest archives are
  mirrored in the GitHub Release so users can download without cloning.
- Every package fully replaces `data/` on install, so stale files from older
  versions never linger.
- Contents:

```
topscreen-<version>-linux.tar.gz
└── topscreen/
    ├── bin/64bit/topscreen.so
    └── data/…

topscreen-<version>-windows.zip
└── topscreen/
    ├── install.bat
    ├── uninstall.bat
    ├── bin/64bit/topscreen.dll
    └── data/…
```
