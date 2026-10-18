@echo off
setlocal EnableExtensions
title TopScreen - installer

rem ============================================================
rem  TopScreen - one-click installer (no compiler, no admin)
rem
rem  How to use:
rem    1. Double-click this file  -> installs for your user
rem    2. Or drag your OBS portable folder onto this file
rem       (the folder that contains "obs-plugins") -> installs there
rem
rem  Expects this layout (as in the release zip):
rem    install.bat
rem    bin\64bit\topscreen.dll
rem    data\...
rem ============================================================

set "ROOT=%~dp0"
set "DLL="
set "DATA=%ROOT%data"
set "PORTABLE=%~1"

echo.
echo  ==============================================
echo   TopScreen installer
echo  ==============================================
echo.

rem ---- locate the plugin binary shipped next to this file ----
for %%F in ("%ROOT%bin\64bit\topscreen.dll" "%ROOT%topscreen.dll" "%ROOT%..\build\Release\topscreen.dll" "%ROOT%..\build\topscreen.dll") do (
    if not defined DLL if exist "%%~fF" set "DLL=%%~fF"
)
if not defined DLL (
    echo  [X] topscreen.dll was not found.
    echo.
    echo      Extract the whole release zip and run install.bat
    echo      from inside the extracted folder.
    echo.
    pause
    exit /b 1
)
if not exist "%DATA%\effects\topscreen.effect" (
    echo  [X] the "data" folder was not found next to this file.
    echo      Extract the whole release zip and try again.
    echo.
    pause
    exit /b 1
)
echo  [ok] plugin : %DLL%
echo  [ok] data   : %DATA%

rem ---- choose the destination ----
if not "%PORTABLE%"=="" goto portable
set "BINDIR=%ProgramData%\obs-studio\plugins\topscreen\bin\64bit"
set "DATADIR=%ProgramData%\obs-studio\plugins\topscreen\data"
echo  [..] installing for your user (no admin needed)
goto copy

:portable
if not exist "%PORTABLE%\obs-plugins" (
    echo  [X] "%PORTABLE%" does not look like an OBS portable
    echo      folder (no "obs-plugins" inside).
    echo.
    pause
    exit /b 1
)
set "BINDIR=%PORTABLE%\obs-plugins\64bit"
set "DATADIR=%PORTABLE%\data\obs-plugins\topscreen"
echo  [..] installing into the portable OBS: %PORTABLE%

:copy
rem ---- OBS must be closed (the DLL is locked while running) ----
tasklist /FI "IMAGENAME eq obs64.exe" 2>nul | find /I "obs64.exe" >nul
if not errorlevel 1 (
    echo.
    echo  [warn] OBS is running.
    echo         Close OBS, then press any key to continue...
    pause >nul
)

rem ---- copy (full sync: no stale files from older versions) ----
if not exist "%BINDIR%" mkdir "%BINDIR%" >nul 2>&1
if exist "%DATADIR%" rmdir /S /Q "%DATADIR%" >nul 2>&1
if not exist "%DATADIR%" mkdir "%DATADIR%" >nul 2>&1
if not exist "%DATADIR%" (
    echo  [X] could not create "%DATADIR%".
    echo      Try right-clicking this file and "Run as administrator".
    echo.
    pause
    exit /b 1
)

copy /Y "%DLL%" "%BINDIR%\topscreen.dll" >nul
if errorlevel 1 (
    echo  [X] failed to copy the plugin into "%BINDIR%".
    echo      Close OBS and try again, or run as administrator.
    echo.
    pause
    exit /b 1
)

xcopy /E /I /Y /Q "%DATA%" "%DATADIR%" >nul
if errorlevel 1 (
    echo  [X] failed to copy the data files.
    echo.
    pause
    exit /b 1
)

rem ---- verify ----
set "MISSING=0"
for %%F in ("%BINDIR%\topscreen.dll" "%DATADIR%\locale\en-US.ini" "%DATADIR%\presets\demo.json" "%DATADIR%\effects\topscreen.effect" "%DATADIR%\cursors\atlas.rgba" "%DATADIR%\sounds\click-soft.wav") do (
    if not exist "%%~fF" (
        echo  [X] missing after install: %%~fF
        set "MISSING=1"
    )
)
if "%MISSING%"=="1" (
    echo.
    echo  Verify FAILED.
    pause
    exit /b 1
)

echo.
echo  ==============================================
echo   Installed and verified. Restart OBS.
echo  ==============================================
echo.
echo   In OBS: Display Capture - Filters - + - "TopScreen FX"
echo.
echo   Log: %%APPDATA%%\obs-studio\logs (most recent file)
echo.
pause
exit /b 0
