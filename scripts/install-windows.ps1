#Requires -Version 5.1
<#
.SYNOPSIS
  Compila e instala o TopScreen no OBS (Windows).

.DESCRIPTION
  Sem argumentos, instala como plugin de USUARIO (nao precisa de admin):
      %ProgramData%\obs-studio\plugins\topscreen\bin\64bit\topscreen.dll
      %ProgramData%\obs-studio\plugins\topscreen\data\...

  Com -ObsDir, instala num OBS PORTABLE:
      <ObsDir>\obs-plugins\64bit\topscreen.dll
      <ObsDir>\data\obs-plugins\topscreen\...

  Se voce ja tem um topscreen.dll compilado, passe -Dll para so instalar.

.PARAMETER ObsDir
  Pasta do OBS portable (contem bin\64bit, obs-plugins, data).

.PARAMETER Dll
  Caminho de um topscreen.dll pronto (pula o build).

.PARAMETER ObsDevDir
  Pasta de desenvolvimento do OBS (com libobs) usada no build (CMAKE_PREFIX_PATH).

.PARAMETER SkipBuild
  Nao compila; usa o dll existente em build\Release\topscreen.dll.

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
# 1. Descobrir o DLL
# ---------------------------------------------------------------------------
$built = Join-Path $Build 'Release\topscreen.dll'
if (-not $Dll -and -not $SkipBuild) {
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        Fail "cmake nao encontrado. Instale: winget install Kitware.CMake`nOu passe -Dll caminho\para\topscreen.dll para so instalar."
    }

    Info '== Configurando o build =='
    $cmakeArgs = @('-S', $Root, '-B', $Build, '-DTOPSCREEN_BUILD_PLUGIN=ON', '-DCMAKE_BUILD_TYPE=Release')
    if ($ObsDevDir) { $cmakeArgs += "-DCMAKE_PREFIX_PATH=$ObsDevDir" }
    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        Warn 'Falha ao configurar (provavelmente falta o libobs dev).'
        Warn "Rode 'pwsh tests\run.ps1' para compilar so os testes, ou passe -ObsDevDir <pasta do OBS dev>."
        Fail 'Build abortado.'
    }

    Info '== Compilando =='
    & cmake --build $Build --config Release
    if ($LASTEXITCODE -ne 0) { Fail 'Build falhou.' }
}

if (-not $Dll) {
    foreach ($cand in @((Join-Path $Build 'Release\topscreen.dll'), (Join-Path $Build 'topscreen.dll'))) {
        if (Test-Path $cand) { $Dll = $cand; break }
    }
}
if (-not $Dll -or -not (Test-Path $Dll)) {
    Fail "topscreen.dll nao encontrado. Compile (sem -SkipBuild) ou passe -Dll <caminho>."
}
Ok "DLL: $Dll"

# ---------------------------------------------------------------------------
# 2. Instalar
# ---------------------------------------------------------------------------
$Data = Join-Path $Root 'data'

if ($ObsDir) {
    if (-not (Test-Path $ObsDir)) { Fail "ObsDir nao existe: $ObsDir" }
    $binDir = Join-Path $ObsDir 'obs-plugins\64bit'
    $dataDir = Join-Path $ObsDir 'data\obs-plugins\topscreen'
    Info "== Instalando no OBS portable: $ObsDir =="
}
else {
    $base = Join-Path $env:ProgramData 'obs-studio\plugins\topscreen'
    $binDir = Join-Path $base 'bin\64bit'
    $dataDir = Join-Path $base 'data'
    Info "== Instalando como plugin de usuario: $base =="
}

New-Item -ItemType Directory -Force -Path $binDir, $dataDir | Out-Null
Copy-Item -Force $Dll (Join-Path $binDir 'topscreen.dll')
Copy-Item -Recurse -Force (Join-Path $Data '*') $dataDir

Ok 'Instalado!'
Write-Host ''
Write-Host 'Para ver o log do TopScreen:'
Write-Host '  & "$env:ProgramFiles\obs-studio\bin\64bit\obs64.exe" --verbose'
Write-Host '  # ou abra %APPDATA%\obs-studio\logs (arquivo mais recente)'
