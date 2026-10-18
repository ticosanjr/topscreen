@echo off
setlocal EnableExtensions
title TopScreen - uninstaller

rem ============================================================
rem  TopScreen - uninstaller
rem
rem  How to use:
rem    1. Double-click        -> removes the per-user install
rem    2. Drag an OBS portable folder onto it -> removes it there
rem ============================================================

set "ARG=%~1"
set "REMOVED=0"

echo.
echo  ==============================================
echo   TopScreen uninstaller
echo  ==============================================
echo  running from: %~dp0
echo.

if not "%ARG%"=="" goto portable

echo  [..] removing the per-user install
if exist "%ProgramData%\obs-studio\plugins\topscreen" (
    rmdir /S /Q "%ProgramData%\obs-studio\plugins\topscreen" >nul 2>&1
    if exist "%ProgramData%\obs-studio\plugins\topscreen" (
        echo  [X] could not remove it. Close OBS and run as administrator.
    ) else (
        echo  [ok] removed: %%ProgramData%%\obs-studio\plugins\topscreen
        set "REMOVED=1"
    )
) else (
    echo  [..] not present: %%ProgramData%%\obs-studio\plugins\topscreen
)
goto done

:portable
if not exist "%ARG%\obs-plugins" (
    echo  [X] "%ARG%" does not look like an OBS portable
    echo      folder ^(no "obs-plugins" inside^).
    echo.
    echo      Double-click this file with no arguments to remove the
    echo      per-user install instead.
    echo.
    pause
    exit /b 1
)

echo  [..] removing from the portable OBS: %ARG%
if exist "%ARG%\obs-plugins\64bit\topscreen.dll" (
    del /F /Q "%ARG%\obs-plugins\64bit\topscreen.dll" >nul 2>&1
    echo  [ok] removed: obs-plugins\64bit\topscreen.dll
    set "REMOVED=1"
) else (
    echo  [..] not present: obs-plugins\64bit\topscreen.dll
)
if exist "%ARG%\data\obs-plugins\topscreen" (
    rmdir /S /Q "%ARG%\data\obs-plugins\topscreen" >nul 2>&1
    echo  [ok] removed: data\obs-plugins\topscreen
    set "REMOVED=1"
) else (
    echo  [..] not present: data\obs-plugins\topscreen
)

:done
echo.
if "%REMOVED%"=="0" (
    echo  Nothing was removed. Possible reasons:
    echo    - TopScreen was installed somewhere else ^(a portable OBS,
    echo      or a different plugin folder^);
    echo    - a previous install already removed it.
    echo.
    echo  To remove a portable install, drag the OBS portable folder
    echo  onto this file.
) else (
    echo  Done. Restart OBS.
)
echo.
pause
exit /b 0
