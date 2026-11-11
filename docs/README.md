# Documentation — TopScreen

Start with the **user manual**.

## For users

| Document | Contents |
|-----------|----------|
| [`MANUAL.md`](MANUAL.md) | **User manual**: install, use each feature, hotkeys, recipes, FAQ |
| [`CONFIGURATION.md`](CONFIGURATION.md) | Reference for every option, preset and hotkey |
| [`CURSORS.md`](CURSORS.md) | All 79 visible cursor styles and their stable IDs |
| [`EFFECTS.md`](EFFECTS.md) | Effect catalog and what is implemented |
| [`INPUT-AND-PLATFORMS.md`](INPUT-AND-PLATFORMS.md) | Cursor/keys per OS; permission on Linux; Wayland parity |
| [`STREAMDECK.md`](STREAMDECK.md) | Stream Deck / OBS WebSocket action names |
| [`BUILD.md`](BUILD.md) | Build and install (Linux/Windows) |
| [`VERIFICATION.md`](VERIFICATION.md) | Script to validate on a real OBS |
| [`TESTS.md`](TESTS.md) | Automated tests: how to run and what they cover |
| [`PERFORMANCE.md`](PERFORMANCE.md) | Text/blur/audio optimizations and the OBS benchmark |

## For developers

| Document | Contents |
|-----------|----------|
| [`PLAN.md`](PLAN.md) | Goal, decisions, scope, risks |
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | Architecture, modules, pipeline |
| [`ROADMAP.md`](ROADMAP.md) | Phases and acceptance criteria |
| [`TASKS.md`](TASKS.md) | Task breakdown and status |
| [`../CONTRIBUTING.md`](../CONTRIBUTING.md) | How to contribute |

## Getting started

1. Install: `./scripts/install-linux.sh` (Linux) or
   `pwsh -File scripts\install-windows.ps1` (Windows).
2. In OBS: **Display Capture** → **Filters** → **+** → **TopScreen FX**.
3. Adjust in Properties and create hotkeys in **Settings → Hotkeys**.
4. Full script in [`MANUAL.md`](MANUAL.md).
