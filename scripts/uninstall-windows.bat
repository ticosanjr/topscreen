@echo off
setlocal EnableExtensions
title TopScreen - uninstaller

rem ============================================================
rem  TopScreen - uninstaller (no admin for the user install)
rem
rem  How to use:
rem    1. Double-click this file  -> removes the user install
rem    2. Or drag your OBS portable folder onto this file
rem       -> removes it from the portable instead
rem ============================================================

set "PORTABLE=%~1"

echo.
echo  ==============================================
echo   TopScreen uninstaller
echo  ==============================================
echo.

if not "%PORTABLE%"=="" goto portable
set "TARGET=%ProgramData%\obs-studio\plugins\topscreen"
echo  [..] removing the user install
goto remove

:portable
if not exist "%PORTABLE%\obs-plugins" (
    echo  [X] "%PORTABLE%" does not look like an OBS portable
    echo      folder (no "obs-plugins" inside).
    echo.
    pause
    exit /b 1
)
echo  [..] removing from the portable OBS: %PORTABLE%
if exist "%PORTABLE%\obs-plugins\64bit\topscreen.dll" (
    del /F /Q "%PORTABLE%\obs-plugins\64bit\topscreen.dll" >nul 2>&1
    echo  removed: obs-plugins\64bit\topscreen.dll
) else (
    echo  not present: obs-plugins\64bit\topscreen.dll
)
if exist "%PORTABLE%\data\obs-plugins\topscreen" (
    rmdir /S /Q "%PORTABLE%\data\obs-plugins\topscreen" >nul 2>&1
    echo  removed: data\obs-plugins\topscreen
) else (
    echo  not present: data\obs-plugins\topscreen
)
goto done

:remove
if exist "%TARGET%" (
    rmdir /S /Q "%TARGET%" >nul 2>&1
    echo  removed: %TARGET%
) else (
    echo  not present: %TARGET%
)

:done
echo.
echo  Done. Restart OBS.
echo.
pause
exit /b 0
