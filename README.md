# TopScreen

**Live screen-capture effects for OBS Studio.**

TopScreen is an OBS *filter plugin* that applies live effects over any screen
capture — a styled cursor, click effects, automatic zoom/pan, a background with
rounded corners, a key overlay, spotlight and highlighter — **without recording
anything and without post-editing**. Everything is automatic or triggered by
hotkeys, and the result is consumed directly by OBS.

> It is not a recorder. It produces no file. It has no timeline. It exists to be
> used **together with OBS**, as a filter on an OBS source itself.

---

## How it works (quick view)

1. In OBS, add a **Display Capture** and pick the screen.
2. On the source, under **Filters**, add **TopScreen FX**.
3. Adjust it in the filter's **Properties** and/or the **TopScreen dock**.
4. Configure hotkeys in **Settings → Hotkeys → TopScreen**.

The filter runs entirely on the GPU, inside the OBS process — no IPC, no encode,
no frame copy.

📖 **Full user manual: [`docs/MANUAL.md`](docs/MANUAL.md).**

---

## Project status

Validated in real use: it loads and runs on **OBS 30.0.2** (Linux, X11) — effects,
presets, click sound, key overlay and dock.

| Phase | Scope | Status |
|------|--------|--------|
| F0 | Plugin skeleton + passthrough filter | ✅ |
| F1 | Background, corners, padding, shadow, presets + dock | ✅ |
| F2 | Cursor (size/smoothing/sway/idle/loop) | ✅ |
| F3 | L/R clicks + sound + hotkeys + presets | ✅ |
| F4 | Auto + manual Zoom/Pan (spring) + pan + motion blur | ✅ |
| F5 | Key overlay + spotlight + highlighter | 🟡 (webcam ⬜) |
| F6 | Own capture source + own audio | ⬜ |
| F7 | Wayland parity (portal + evdev) + CI | 🟡 (evdev ✅; portal ⬜) |

The core has **automated tests** and the OBS layer is checked against the real
OBS headers. See [`docs/TESTS.md`](docs/TESTS.md).

### Quick tests

```bash
./tests/run.sh            # Linux: 126 core tests
pwsh -File tests\run.ps1  # Windows (or PowerShell on any OS)
```

---

## Requirements

- **OBS Studio** 30.0+ (validated on 30.0.2)
- **Windows 10+** or **Linux** (X11 and Wayland)
- CMake 3.20+, C++20 compiler
- Qt6 (optional, for the dock)

## Installation (simple)

**Linux**
```bash
./scripts/install-linux.sh            # per-user plugin (no sudo)
./scripts/install-linux.sh --system   # system OBS (uses sudo)
```

**Windows**
```powershell
# 1) Download topscreen-<version>-windows.zip (GitHub release / CI artifact)
# 2) Extract the WHOLE zip and open the "topscreen" folder
# 3) Double-click install.bat  (it sits next to bin/ and data/)
install.bat                  # per-user plugin (no admin, no compiler)
# Uninstall the same way (same folder):
uninstall.bat
```
Portable OBS: **drag the portable folder onto `install.bat`** (same for `uninstall.bat`).
Drag a `topscreen.dll` onto it to install a specific build. Running
`scripts\install-windows.bat` from a source clone works too — it searches for a
compiled DLL and explains how to build if there is none.
<details>
<summary>Compiling from source instead (developers)</summary>

```powershell
winget install Kitware.CMake
winget install Microsoft.VisualStudio.2022.BuildTools   # then use a Developer PowerShell
pwsh -File scripts\install-windows.ps1 -ObsDevDir C:\obs-dev
pwsh -File scripts\install-windows.ps1 -ObsDir D:\OBS  # portable
pwsh -File scripts\install-windows.ps1 -Dll .\topscreen.dll
```
</details>

Full step by step in [`docs/BUILD.md`](docs/BUILD.md).

## Removal

```bash
./scripts/uninstall-linux.sh            # Linux (system + user)
```
```powershell
pwsh -File scripts\uninstall-windows.ps1   # Windows (user plugin)
```

---

## Documentation

Full index: [`docs/README.md`](docs/README.md).

| Document | Contents |
|-----------|----------|
| [`docs/MANUAL.md`](docs/MANUAL.md) | **User manual** (install, use, hotkeys, recipes, FAQ) |
| [`docs/CONFIGURATION.md`](docs/CONFIGURATION.md) | Reference for every option, preset and hotkey |
| [`docs/EFFECTS.md`](docs/EFFECTS.md) | Effect catalog (with status) |
| [`docs/INPUT-AND-PLATFORMS.md`](docs/INPUT-AND-PLATFORMS.md) | Input per OS and X11/Wayland parity |
| [`docs/BUILD.md`](docs/BUILD.md) | Build and install (Windows/Linux) |
| [`docs/VERIFICATION.md`](docs/VERIFICATION.md) | Validation script on a real OBS |
| [`docs/TESTS.md`](docs/TESTS.md) | Automated tests: how to run and what they cover |
| [`docs/PLAN.md`](docs/PLAN.md) · [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) · [`docs/ROADMAP.md`](docs/ROADMAP.md) · [`docs/TASKS.md`](docs/TASKS.md) | Plan, architecture, roadmap and tasks |

---

## License

GPL-2.0-or-later. See [`COPYING`](COPYING) (full text) and [`LICENSE`](LICENSE).
Plugins that link `libobs` must be GPLv2-compatible (see `docs/PLAN.md` §2).
