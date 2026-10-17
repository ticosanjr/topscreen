#Requires -Version 5.1
<#
.SYNOPSIS
  Removes TopScreen from OBS (Windows).

.DESCRIPTION
  With no arguments, removes the user plugin (no admin needed):
      %ProgramData%\obs-studio\plugins\topscreen

  With -ObsDir, removes it from an OBS portable instead.

.EXAMPLE
  pwsh -File scripts\uninstall-windows.ps1
  pwsh -File scripts\uninstall-windows.ps1 -ObsDir 'D:\OBS'
#>
param(
    [string]$ObsDir
)

$ErrorActionPreference = 'Stop'

function Info($m) { Write-Host $m -ForegroundColor Cyan }
function Ok($m) { Write-Host $m -ForegroundColor Green }

if ($ObsDir) {
    if (-not (Test-Path $ObsDir)) { Write-Host "ObsDir does not exist: $ObsDir" -ForegroundColor Red; exit 2 }
    $targets = @(
        (Join-Path $ObsDir 'obs-plugins\64bit\topscreen.dll'),
        (Join-Path $ObsDir 'data\obs-plugins\topscreen')
    )
    Info "== removing from the portable OBS: $ObsDir =="
}
else {
    $targets = @((Join-Path $env:ProgramData 'obs-studio\plugins\topscreen'))
    Info '== removing the user plugin (no admin needed) =='
}

foreach ($t in $targets) {
    if (Test-Path $t) {
        Remove-Item -Recurse -Force $t
        Info "removed: $t"
    }
    else {
        Info "not present: $t"
    }
}

Ok 'Done. Restart OBS.'
