# TopScreen releases

Ready-to-use packages, **no build required**. The newest **three** versions are
kept here and in [GitHub Releases](https://github.com/ticosanjr/topscreen/releases);
older tags remain in git, but their downloadable archives are removed.

The archives in this folder are local build outputs (ignored by git). The
GitHub Release attachments are the canonical downloadable copies.

| Version | Linux | Windows | Highlights |
|---------|-------|---------|------------|
| **1.3.0** | [`topscreen-1.3.0-linux.tar.gz`](https://github.com/ticosanjr/topscreen/releases/download/v1.3.0/topscreen-1.3.0-linux.tar.gz) | [`topscreen-1.3.0-windows.zip`](https://github.com/ticosanjr/topscreen/releases/download/v1.3.0/topscreen-1.3.0-windows.zip) \* | **Embedded capture effects:** TopScreen Capture applies the effects itself (no child filter) and migrates 1.2.5 scenes; X11 native-cursor compositing with a non-persisted override; click-sound audio on the source; Reading/zoom/pointer accuracy fixes |
| **1.2.6** | [`topscreen-1.2.6-linux.tar.gz`](https://github.com/ticosanjr/topscreen/releases/download/v1.2.6/topscreen-1.2.6-linux.tar.gz) | [`topscreen-1.2.6-windows.zip`](https://github.com/ticosanjr/topscreen/releases/download/v1.2.6/topscreen-1.2.6-windows.zip) \* | **Reading highlight + mouse accuracy:** Reading mode dims outside the selection and pins the fitted zoom until the next click; keep-until-click option for every other highlight/zoom mode; display/source coordinate split and zoom-aware pointer mapping; real monitor geometry, spotlight focus split and TopScreen Capture monitor index |
| **1.2.5** | [`topscreen-1.2.5-linux.tar.gz`](https://github.com/ticosanjr/topscreen/releases/download/v1.2.5/topscreen-1.2.5-linux.tar.gz) | [`topscreen-1.2.5-windows.zip`](https://github.com/ticosanjr/topscreen/releases/download/v1.2.5/topscreen-1.2.5-windows.zip) \* | **Native capture + scene profiles:** experimental **TopScreen Capture** source on Linux/X11 (XShm, monitor picker, attached effects filter), scene profiles by UUID with startup apply and profile-backup export/import, Smart Zoom foreground-window toggle, webcam bubble source action and device frames, Stream Deck/OBS WebSocket guide |

> \* **Windows packages 0.2.18–1.3.0** are built by the `Release` workflow and include the Qt dock.
> Local fallback cross-builds (if used) do not include it. They are the full filter (Properties, hotkeys,
> presets, profiles, multi-screen fix). The Linux package includes the dock.

## Install (Windows, no compiler, no admin)

1. Download `topscreen-<version>-windows.zip`.
2. **Extract the whole zip** and open the `topscreen` folder.
3. Double-click **`install.bat`** — it sits next to `bin` and `data`, so nothing
   has to be moved.
   - Drag your **OBS portable** folder onto it to install there instead.
   - Remove later with `uninstall.bat` in the same folder.
4. Restart OBS → add a **Display Capture** → **Filters → + → "TopScreen FX"**.

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

./scripts/build-install.sh          # build/topscreen.so (with the Qt dock)
./scripts/build-windows-cross.sh    # build/topscreen.dll (without the dock)
./scripts/package.sh                # writes archives and prunes to the newest 3
./scripts/verify-release.sh
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

`scripts/package.sh` writes to this folder by default (`--out DIR` to override)
and automatically calls `scripts/prune-releases.sh`, which keeps only the newest
three versions. Run it manually with:

```bash
./scripts/prune-releases.sh --dry-run
./scripts/prune-releases.sh
```

GitHub Release entries older than the newest three should also be removed:

```bash
gh release list --limit 100
# delete old releases but keep their tags:
gh release delete <old-tag> --yes
```

## Notes

- The **GitHub Release** is the canonical downloadable copy. The local
  `releases/` folder keeps the newest three archives for offline use.
- Release archives are **ignored by git**; only this index is tracked.
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
