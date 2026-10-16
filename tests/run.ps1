#Requires -Version 5.1
<#
.SYNOPSIS
  Roda os testes do nucleo do TopScreen (nao precisa do OBS).

.DESCRIPTION
  Compila apenas os testes (TOPSCREEN_BUILD_PLUGIN=OFF) e executa via ctest.
  Funciona no Windows (MSVC) e tambem em Linux/macOS com PowerShell.

.EXAMPLE
  pwsh -File tests\run.ps1
#>
$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Root 'build-tests'

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Host 'cmake nao encontrado.' -ForegroundColor Red
    Write-Host 'Instale: winget install Kitware.CMake'
    Write-Host 'E o compilador: winget install Microsoft.VisualStudio.2022.BuildTools'
    exit 1
}

Write-Host '== Configurando (apenas testes, sem OBS) ==' -ForegroundColor Cyan
cmake -S $Root -B $Build -DTOPSCREEN_BUILD_PLUGIN=OFF -DTOPSCREEN_BUILD_TESTS=ON
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host '== Compilando ==' -ForegroundColor Cyan
cmake --build $Build --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host '== Rodando testes ==' -ForegroundColor Cyan
ctest --test-dir $Build -C Release --output-on-failure
exit $LASTEXITCODE
