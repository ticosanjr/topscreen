@echo off
setlocal EnableExtensions
title TopScreen - installer

rem ============================================================
rem  TopScreen - installer for Windows
rem
rem  How to use:
rem    1. Double-click        -> installs for your user (no admin)
rem    2. Drag an OBS portable folder onto it -> installs there
rem    3. Drag a topscreen.dll onto it       -> installs that dll
rem
rem  Works both from the release zip (install.bat next to bin\ and data\)
rem  and from the source repository (scripts\install-windows.bat), where it
rem  also searches build\, x64\ and out\ for a compiled topscreen.dll.
rem ============================================================

set "HERE=%~dp0"
set "ARG=%~1"
set "DLL="
set "DATA="
set "OBSDIR="

echo.
echo  ==============================================
echo   TopScreen installer
echo  ==============================================
echo.
echo  running from: %HERE%

rem ---------------------------------------------------------------- data
for %%D in ("%HERE%data" "%HERE%..\data" "%HERE%..\..\data") do (
    if not defined DATA if exist "%%~fD\effects\topscreen.effect" set "DATA=%%~fD"
)
if not defined DATA (
    echo.
    echo  [X] the "data" folder was not found next to this file.
    echo.
    echo  Are you inside the release zip? Extract it COMPLETELY and run
    echo  install.bat from the extracted folder, so the layout is:
    echo.
    echo      install.bat
    echo      bin\64bit\topscreen.dll
    echo      data\effects\topscreen.effect
    echo.
    echo  Download the zip from the releases/CI artifacts, or see
    echo  docs\BUILD.md to build from source.
    echo.
    pause
    exit /b 1
)
echo  [ok] data    : %DATA%

rem ------------------------------------------------- argument disambiguation
rem  A .dll argument = that dll; a folder with obs-plugins = portable OBS.
rem  (set + use are split into separate statements on purpose: a variable
rem   assigned inside a block is not visible to %VAR% in the same block.)
if not "%ARG%"=="" (
    echo "%ARG%" | find /I ".dll" >nul
    if not errorlevel 1 if exist "%ARG%" set "DLL=%~f1"
    if errorlevel 1 if exist "%ARG%\obs-plugins" set "OBSDIR=%~f1"
)
if not "%ARG%"=="" if not defined DLL if not defined OBSDIR (
    echo.
    echo  [X] "%ARG%" is not usable here.
    echo.
    echo      I need either:
    echo        - a topscreen.dll file, or
    echo        - an OBS portable folder ^(one that contains "obs-plugins"^).
    echo.
    echo      Tip: double-click this file with no arguments to install
    echo           for your user instead.
    echo.
    pause
    exit /b 1
)
if defined DLL echo  [ok] plugin  : %DLL%
if defined OBSDIR echo  [ok] portable: %OBSDIR%

rem ---------------------------------------------------------------- dll
if not defined DLL (
    echo  [..] looking for topscreen.dll ...
    for %%C in (
        "%HERE%bin\64bit\topscreen.dll"
        "%HERE%topscreen.dll"
        "%HERE%build\Release\topscreen.dll"
        "%HERE%build\RelWithDebInfo\topscreen.dll"
        "%HERE%build\Debug\topscreen.dll"
        "%HERE%build\topscreen.dll"
        "%HERE%..\bin\64bit\topscreen.dll"
        "%HERE%..\topscreen.dll"
        "%HERE%..\build\Release\topscreen.dll"
        "%HERE%..\build\RelWithDebInfo\topscreen.dll"
        "%HERE%..\build\Debug\topscreen.dll"
        "%HERE%..\build\topscreen.dll"
        "%HERE%..\x64\Release\topscreen.dll"
        "%HERE%..\x64\Debug\topscreen.dll"
        "%HERE%..\out\Release\topscreen.dll"
    ) do (
        if not defined DLL (
            if exist "%%~fC" (
                set "DLL=%%~fC"
            ) else (
                echo       - not found: %%~fC
            )
        )
    )
)

rem Last resort: bounded search in the tree (skips .git/build caches via /A-D only).
if not defined DLL (
    echo  [..] searching the folder tree ...
    for /F "delims=" %%C in ('dir /S /B /A-D "%HERE%..\topscreen.dll" 2^>nul') do (
        if not defined DLL set "DLL=%%C"
    )
)
if not defined DLL (
    for /F "delims=" %%C in ('dir /S /B /A-D "%HERE%topscreen.dll" 2^>nul') do (
        if not defined DLL set "DLL=%%C"
    )
)

if not defined DLL goto nodll
echo  [ok] plugin  : %DLL%

rem ---------------------------------------------------------------- target
if not "%OBSDIR%"=="" goto target_portable
set "BINDIR=%ProgramData%\obs-studio\plugins\topscreen\bin\64bit"
set "DATADIR=%ProgramData%\obs-studio\plugins\topscreen\data"
echo  [..] target  : per-user plugin (no admin needed)
goto copy

:target_portable
set "BINDIR=%OBSDIR%\obs-plugins\64bit"
set "DATADIR=%OBSDIR%\data\obs-plugins\topscreen"
echo  [..] target  : portable OBS at %OBSDIR%

:copy
rem ---- OBS must be closed (the DLL is locked while running) ----
tasklist /FI "IMAGENAME eq obs64.exe" 2>nul | find /I "obs64.exe" >nul
if not errorlevel 1 (
    echo.
    echo  [warn] OBS is running. Close it, then press any key to continue...
    pause >nul
)

if not exist "%BINDIR%" mkdir "%BINDIR%" >nul 2>&1
if exist "%DATADIR%" rmdir /S /Q "%DATADIR%" >nul 2>&1
if not exist "%DATADIR%" mkdir "%DATADIR%" >nul 2>&1
if not exist "%DATADIR%" goto perm

copy /Y "%DLL%" "%BINDIR%\topscreen.dll" >nul
if errorlevel 1 goto locked

xcopy /E /I /Y /Q "%DATA%" "%DATADIR%" >nul
if errorlevel 1 (
    echo.
    echo  [X] failed to copy the data files from "%DATA%".
    echo.
    pause
    exit /b 1
)

rem ---------------------------------------------------------------- verify
set "MISSING=0"
for %%F in (
    "%BINDIR%\topscreen.dll"
    "%DATADIR%\locale\en-US.ini"
    "%DATADIR%\presets\demo.json"
    "%DATADIR%\effects\topscreen.effect"
    "%DATADIR%\cursors\atlas.rgba"
    "%DATADIR%\sounds\click-soft.wav"
) do (
    if not exist "%%~fF" (
        echo  [X] missing after install: %%~fF
        set "MISSING=1"
    )
)
if "%MISSING%"=="1" goto failed

echo.
echo  ==============================================
echo   Installed and verified. Restart OBS.
echo  ==============================================
echo.
echo   plugin: %BINDIR%\topscreen.dll
echo   data  : %DATADIR%
echo.
echo   In OBS: Display Capture - Filters - + - "TopScreen FX"
echo   Log   : %%APPDATA%%\obs-studio\logs (most recent file^)
echo.
pause
exit /b 0

rem ---------------------------------------------------------------- errors
:perm
echo.
echo  [X] could not create:
echo      %DATADIR%
echo.
echo  How to fix:
echo    - right-click install.bat and choose "Run as administrator", or
echo    - drag your OBS portable folder onto this file to install there.
echo.
pause
exit /b 1

:locked
echo.
echo  [X] could not write:
echo      %BINDIR%\topscreen.dll
echo.
echo  Why: OBS usually keeps the plugin locked while it is open, and a
echo       previous install may need administrator rights.
echo.
echo  How to fix:
echo    1. close OBS completely ^(check the tray icon^);
echo    2. run this file again;
echo    3. if it still fails, right-click and "Run as administrator".
echo.
pause
exit /b 1

:failed
echo.
echo  [X] the install finished but some files are missing.
echo      Try again, or install manually:
echo        %DLL%   -^>   %BINDIR%\topscreen.dll
echo        %DATA%\*            -^>   %DATADIR%\
echo.
pause
exit /b 1

:nodll
echo.
echo  ==============================================
echo   topscreen.dll was not found
echo  ==============================================
echo.
echo  I looked in "bin", "build", "x64" and "out" next to this file
echo  and in the folder tree. Nothing to install yet.
echo.
echo  Choose ONE of these:
echo.
echo   1^) INSTALL A READY BUILD ^(easiest, no compiler^)
echo      Download topscreen-^<version^>-windows.zip from the GitHub
echo      releases or the "Build plugin" CI artifacts, extract it
echo      COMPLETELY and double-click install.bat inside it.
echo.
echo   2^) BUILD IT YOURSELF ^(needs a toolchain^)
echo      In a "Developer PowerShell":
echo        winget install Kitware.CMake
echo        winget install Microsoft.VisualStudio.2022.BuildTools
echo        pwsh -File scripts\install-windows.ps1 -ObsDevDir C:\obs-dev
echo      ^(you also need the OBS development files - see docs\BUILD.md^)
echo.
echo   3^) INSTALL A DLL YOU ALREADY HAVE
echo      Drag topscreen.dll onto this file, or copy it by hand:
echo        topscreen.dll -^> %%ProgramData%%\obs-studio\plugins\topscreen\bin\64bit\
echo        data\*        -^> %%ProgramData%%\obs-studio\plugins\topscreen\data\
echo.
echo  Full instructions: docs\BUILD.md
echo.
pause
exit /b 1
