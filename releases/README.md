# TopScreen releases

Ready-to-use packages, versioned in this folder (no build required).
Each version keeps its files; newer versions are added alongside.

| Version | Linux | Windows | Highlights |
|---------|-------|---------|------------|
| **0.2.7** | [`topscreen-0.2.7-linux.tar.gz`](topscreen-0.2.7-linux.tar.gz) | [`topscreen-0.2.7-windows.zip`](topscreen-0.2.7-windows.zip) \* | multi-monitor input fix (cursor/clicks/zoom on stacked or side-by-side screens) + manual monitor origin |
| 0.2.6 | [`topscreen-0.2.6-linux.tar.gz`](topscreen-0.2.6-linux.tar.gz) | [`topscreen-0.2.6-windows.zip`](topscreen-0.2.6-windows.zip) | first build with the Windows Qt dock; 10 profiles |

## Install (Windows, no compiler, no admin)

1. Download `topscreen-<version>-windows.zip`.
2. **Extract the whole zip** and open the `topscreen` folder.
3. Double-click **`install.bat`** — it sits next to `bin` and `data`, so nothing
   has to be moved.
   - Drag your **OBS portable** folder onto it to install there instead.
   - Remove later with `uninstall.bat` in the same folder.
4. Restart OBS → add a **Display Capture** → **Filters → + → "TopScreen FX"**.

The Windows package includes the Qt dock: **View → Docks → TopScreen**.

> \* **0.2.7 Windows**: built by cross-compiling from Linux
> (`scripts/build-windows-cross.sh`) because the GitHub Actions minutes were
> blocked. It is the full filter (Properties, hotkeys, presets, profiles,
> multi-screen fix) but **without the Qt dock** — the next CI build will
> replace it with the complete package. If the dock matters to you, stay on
> 0.2.6 for now and update once the CI package is published.

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
# Linux (this machine):
./scripts/build-install.sh          # builds build/topscreen.so
./scripts/package.sh                # writes releases/topscreen-<v>-linux.tar.gz

# Windows package: built by the "Release" workflow (Actions → Release → Run
# workflow) or on a Windows machine with `scripts\install-windows.ps1`; copy the
# resulting zip here.
git add releases/ && git commit -m "release: <version>" && git push
```

`scripts/package.sh` writes to this folder by default (`--out DIR` to override).
Version numbers come from `CMakeLists.txt` (`project(TopScreen VERSION ...)`).

## Notes

- These archives are the canonical download. GitHub Releases are no longer
  created for new versions.
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
