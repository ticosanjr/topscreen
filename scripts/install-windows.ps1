#Requires -Version 5.1
<#
.SYNOPSIS
  Builds and installs TopScreen into OBS (Windows).

.DESCRIPTION
  With no arguments, installs as a USER plugin (no admin needed):
      %ProgramData%\obs-studio\plugins\topscreen\bin\64bit\topscreen.dll
      %ProgramData%\obs-studio\plugins\topscreen\data\...

  With -ObsDir, installs into an OBS PORTABLE:
      <ObsDir>\obs-plugins\64bit\topscreen.dll
      <ObsDir>\data\obs-plugins\topscreen\...

  If you already have a compiled topscreen.dll, pass -Dll to install only.

.PARAMETER ObsDir
  OBS portable folder (contains bin\64bit, obs-plugins, data).

.PARAMETER Dll
  Path to a ready topscreen.dll (skips the build).

.PARAMETER ObsDevDir
  OBS development folder (with libobs) used in the build (CMAKE_PREFIX_PATH).

.PARAMETER SkipBuild
  Does not build; uses the existing dll in build\Release\topscreen.dll.

.EXAMPLE
  pwsh -File scripts\install-windows.ps1
  pwsh -File scripts\install-windows.ps1 -ObsDir 'D:\OBS'
  pwsh -File scripts\install-windows.ps1 -Dll .\topscreen.dll
#>
param(
    [string]$ObsDir,
    [string]$Dll,
    [string]$ObsDevDir,
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Root 'build'

function Info($m) { Write-Host $m -ForegroundColor Cyan }
function Ok($m) { Write-Host $m -ForegroundColor Green }
function Warn($m) { Write-Host $m -ForegroundColor Yellow }
function Fail($m) { Write-Host $m -ForegroundColor Red; exit 1 }

# ---------------------------------------------------------------------------
# 1. Find the DLL
# ---------------------------------------------------------------------------
$built = Join-Path $Build 'Release\topscreen.dll'
if (-not $Dll -and -not $SkipBuild) {
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        Fail "cmake not found. Install: winget install Kitware.CMake`nOr pass -Dll path\to\topscreen.dll to install only."
    }

    Info '== Configuring the build =='
    $cmakeArgs = @('-S', $Root, '-B', $Build, '-DTOPSCREEN_BUILD_PLUGIN=ON', '-DCMAKE_BUILD_TYPE=Release')
    if ($ObsDevDir) { $cmakeArgs += "-DCMAKE_PREFIX_PATH=$ObsDevDir" }
    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        Warn 'Failed to configure (probably the libobs dev is missing).'
        Warn "Run 'pwsh tests\run.ps1' to build only the tests, or pass -ObsDevDir <OBS dev folder>."
        Fail 'Build aborted.'
    }

    Info '== Building =='
    & cmake --build $Build --config Release
    if ($LASTEXITCODE -ne 0) { Fail 'Build failed.' }
}

if (-not $Dll) {
    foreach ($cand in @((Join-Path $Build 'Release\topscreen.dll'), (Join-Path $Build 'topscreen.dll'))) {
        if (Test-Path $cand) { $Dll = $cand; break }
    }
}
if (-not $Dll -or -not (Test-Path $Dll)) {
    Fail "topscreen.dll not found. Build (without -SkipBuild) or pass -Dll <path>."
}
Ok "DLL: $Dll"

# ---------------------------------------------------------------------------
# 2. Install
# ---------------------------------------------------------------------------
$Data = Join-Path $Root 'data'

if ($ObsDir) {
    if (-not (Test-Path $ObsDir)) { Fail "ObsDir does not exist: $ObsDir" }
    $binDir = Join-Path $ObsDir 'obs-plugins\64bit'
    $dataDir = Join-Path $ObsDir 'data\obs-plugins\topscreen'
    Info "== Installing into the portable OBS: $ObsDir =="
}
else {
    $base = Join-Path $env:ProgramData 'obs-studio\plugins\topscreen'
    $binDir = Join-Path $base 'bin\64bit'
    $dataDir = Join-Path $base 'data'
    Info "== Installing as a user plugin: $base =="
}

New-Item -ItemType Directory -Force -Path $binDir, $dataDir | Out-Null
Copy-Item -Force $Dll (Join-Path $binDir 'topscreen.dll')
Copy-Item -Recurse -Force (Join-Path $Data '*') $dataDir

Ok 'Installed!'
Write-Host ''
Write-Host 'To see the TopScreen log:'
Write-Host '  & "$env:ProgramFiles\obs-studio\bin\64bit\obs64.exe" --verbose'
Write-Host '  # or open %APPDATA%\obs-studio\logs (most recent file)'
