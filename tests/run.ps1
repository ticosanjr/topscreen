#Requires -Version 5.1
<#
.SYNOPSIS
  Runs the TopScreen core tests (does not need OBS).

.DESCRIPTION
  Builds only the tests (TOPSCREEN_BUILD_PLUGIN=OFF) and runs them via ctest.
  Works on Windows (MSVC) and also on Linux/macOS with PowerShell.

.EXAMPLE
  pwsh -File tests\run.ps1
#>
$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Root 'build-tests'

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Host 'cmake not found.' -ForegroundColor Red
    Write-Host 'Install: winget install Kitware.CMake'
    Write-Host 'And the compiler: winget install Microsoft.VisualStudio.2022.BuildTools'
    exit 1
}

Write-Host '== Configuring (tests only, no OBS) ==' -ForegroundColor Cyan
cmake -S $Root -B $Build -DTOPSCREEN_BUILD_PLUGIN=OFF -DTOPSCREEN_BUILD_TESTS=ON
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host '== Building ==' -ForegroundColor Cyan
cmake --build $Build --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host '== Running tests ==' -ForegroundColor Cyan
ctest --test-dir $Build -C Release --output-on-failure
exit $LASTEXITCODE
