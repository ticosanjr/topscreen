# Build

## Quick build

```bash
./scripts/build.sh                 # plugin + tests, then run the tests
./scripts/build.sh --tests-only    # tests only (does not need libobs)
./scripts/build.sh --no-test       # compile only
```

## Remove

```bash
./scripts/uninstall-linux.sh            # system + user
./scripts/uninstall-linux.sh --system   # system only (fixes "Duplicate library")
```

```powershell
pwsh -File scripts\uninstall-windows.ps1               # user plugin
pwsh -File scripts\uninstall-windows.ps1 -ObsDir D:\OBS # portable OBS
```

## Install without compiling (release packages)

`scripts/package.sh` packs whatever was built into `dist/`:

| File | Install |
|------|---------|
| `topscreen-<v>-linux.tar.gz` | `tar -xzf topscreen-<v>-linux.tar.gz -C ~/.config/obs-studio/plugins/` |
| `topscreen-<v>-windows.zip` | extract `topscreen\` into `%ProgramData%\obs-studio\plugins\` (portable: `bin\64bit\topscreen.dll` → `obs-plugins\64bit`, `data\*` → `data\obs-plugins\topscreen`) |

CI (`Release` workflow, manual trigger or a `v*` tag) builds on Linux + Windows
and publishes these archives. Every installer fully syncs `data/`, so
stale files from older versions never linger.

## Cutting a release

```bash
git tag v0.2.1 && git push origin v0.2.1
```

The `Release` workflow then builds both packages, runs the core tests on Linux
and publishes a GitHub Release with the two archives.

| Platform | Build environment |
|----------|-------------------|
| Linux | `ubuntu-24.04` + distro `libobs-dev` 30.0.2, `obs-frontend-api` and Qt6 (dock included) |
| Windows | official OBS **sources** of the same version as the runtime (headers) + import libraries generated from `obs.dll` and `obs-frontend-api.dll`, plus Qt6 from obs-deps: the release package includes the **Qt dock** like Linux |

You can also run it by hand (**Actions → Release → Run workflow**) to get the
archives as artifacts without publishing a release.

## Quick install

### Linux

```bash
# dependencies (once)
sudo apt update && sudo apt install -y cmake build-essential libobs-dev
# optional (dock): sudo apt install -y qt6-base-dev

./scripts/install-linux.sh            # per-user plugin (no sudo)
./scripts/install-linux.sh --system   # system OBS (uses sudo)
```

### Windows

No compiler, no admin — download `topscreen-<version>-windows.zip`
(from a GitHub release or the `Build plugin` CI artifacts), **extract the
whole zip**, and double-click **`install.bat`**:

| What you do | Where it installs |
|---|---|
| Double-click `install.bat` | per-user plugin (`%ProgramData%\obs-studio\plugins\topscreen`) |
| Drag your OBS **portable** folder onto `install.bat` | inside the portable (`obs-plugins\64bit` + `data\obs-plugins\topscreen`) |
| Drag a **`topscreen.dll`** onto `install.bat` | per-user plugin, using that file |

To remove: double-click **`uninstall.bat`** (same drag-and-drop rule for portable).

**If you cloned the source instead of the zip**, running
`scripts\install-windows.bat` works too: it searches `bin\`, `build\`,
`x64\`, `out\` and the folder tree for a compiled `topscreen.dll`. If it
finds none, it prints the three ways forward (download the zip, build with
the toolchain, or drop a DLL onto it) instead of failing silently.

The installer warns if OBS is open (the DLL is locked while running), syncs
`data/` fully (no stale files), verifies 6 key files and prints the exact
paths it used. Developers compiling with VS Build Tools: see
[Windows (from source)](#windows-from-source--developers).

### Tests (on both platforms)

```bash
./tests/run.sh                                   # Linux (g++ directly)
cmake -S . -B build-tests -DTOPSCREEN_BUILD_PLUGIN=OFF   # any OS
cmake --build build-tests && ctest --test-dir build-tests --output-on-failure
```

```powershell
pwsh -File tests\run.ps1                         # Windows (or PowerShell on any OS)
```

> **Important:** the OBS log goes to **stderr**. Use `2>&1`:
> `obs --verbose 2>&1 | grep -i topscreen`, or see
> `~/.config/obs-studio/logs/`.

## Dependencies

- CMake >= 3.20
- A compiler with C++20 (MSVC 2019+, GCC 11+, Clang 13+)
- **libobs** (headers/libs) — from the OBS development package, the official
  development build, or a local OBS build.
- **Qt6 Widgets** and **obs-frontend-api** (optional; needed for the dock)
- X11 (`libx11-dev`) on Linux for the X11 input backend
- `linux/input.h` (linux-libc-dev) for the evdev backend (Tier 3)

> This repository does **not** bundle the dependencies. `CMakeLists.txt` looks for
> `libobs` via `find_package(libobs)`; if it does not find it, it falls back to
> `pkg-config libobs`.

---

## Linux

### 1. Get libobs

**Option A — distro packages** (when plugin headers exist):

```bash
# Debian/Ubuntu (example; names vary by version)
sudo apt install cmake build-essential qt6-base-dev libx11-dev libobs-dev
```

**Option B — build OBS from source** (recommended for plugin dev,
guarantees headers, `obs-frontend-api` and CMake configs):

```bash
git clone --recursive https://github.com/obsproject/obs-studio
cd obs-studio
cmake --preset ubuntu-x86_64
cmake --build build_x86_64 -j
cmake --install build_x86_64 --prefix "$HOME/obs-dev"
```

Point TopScreen at that installation:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH="$HOME/obs-dev"
```

### 2. Compile

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### 3. Install / test

Install for the system OBS:

```bash
sudo cmake --install build
# lib:  /usr/local/lib/obs-plugins/topscreen.so
# data: /usr/local/share/obs/obs-plugins/topscreen/
```

**Portable** mode (without installing): put it next to the OBS binary:

```bash
cp build/topscreen.so   <obs-portable>/obs-plugins/
cp -r data/             <obs-portable>/obs-plugins/topscreen/
```

Run OBS with logs and confirm it loaded:

```bash
obs --verbose 2>&1 | grep -i topscreen
```

---

## Windows (from source — developers)

Prefer the one-click path above. To compile yourself:

1. Install **Visual Studio 2022** (Desktop C++) and **CMake**:
   `winget install Kitware.CMake` +
   `winget install Microsoft.VisualStudio.2022.BuildTools`
   (then use a Developer PowerShell).
2. Get the **libobs + obs-frontend-api** (OBS development build or
   compile OBS with `cmake --preset windows-x64`).
3. Configure pointing at OBS/Qt:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_PREFIX_PATH="C:\obs-dev"
cmake --build build --config Release
```

4. Install without manual copying:

```powershell
pwsh -File scripts\install-windows.ps1 -ObsDevDir C:\obs-dev   # per-user
pwsh -File scripts\install-windows.ps1 -ObsDir D:\OBS          # portable
pwsh -File scripts\install-windows.ps1 -Dll .\topscreen.dll   # prebuilt DLL
```

The `.ps1` installer does the same full-sync + verify as `install.bat`.
Uninstall: `pwsh -File scripts\uninstall-windows.ps1` (add `-ObsDir` for portable).

---

## CMake options

| Option | Default | Effect |
|-------|--------|--------|
| `ENABLE_QT` | `ON` | Builds the Qt dock |
| `ENABLE_FRONTEND_API` | `ON` | Uses `obs-frontend-api` (needed for the dock) |
| `TOPSCREEN_BUILD_TESTS` | `ON` | Builds the core tests (they do not depend on OBS) |
| `CMAKE_BUILD_TYPE` | `Release` | Build type on single-config generators |

Automatic defines: `TOPSCREEN_VERSION`, `TOPSCREEN_HAVE_QT`,
`TOPSCREEN_HAVE_FRONTEND`, `TOPSCREEN_HAVE_X11`, `TOPSCREEN_HAVE_EVDEV`.

---

## Tests

The **core** tests do not depend on libobs/cmake and run anywhere:

```bash
./tests/run.sh          # compiles with g++ and runs
```

With CMake:

```bash
cmake -S . -B build -DTOPSCREEN_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

See [`TESTS.md`](TESTS.md) for the coverage and what still needs a real OBS.

To check that the plugin **compiles** against the OBS headers without having it
installed:

```bash
./tools/check-obs-headers.sh
```

---

## Troubleshooting

- **`find_package(libobs)` fails**: set `-DCMAKE_PREFIX_PATH` to the OBS dev
  folder, or make sure there is a `libobs.pc` in `PKG_CONFIG_PATH`.
- **Undefined symbols when loading**: usually an ABI mismatch between the
  libobs used in the build and the running OBS — rebuild against the same version.
- **Dock does not appear**: check whether Qt6 and `obs-frontend-api` were found
  (the configure prints `TopScreen: Qt=... frontend=...`).
- **Input does not work on Linux**: see `docs/INPUT-AND-PLATFORMS.md`
  (X11 enabled via `TOPSCREEN_HAVE_X11`; evdev requires permission).
