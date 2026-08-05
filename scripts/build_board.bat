@echo off
:: Windows counterpart of scripts/build_board.sh — build (or flash / monitor /
:: reconfigure / size / ...) the P4 firmware for a head-unit board. Each board
:: gets its own build directory and sdkconfig, with the per-board
:: sdkconfig.defaults.<board> overlay layered on the common one (later wins).
::
:: Usage:
::   scripts\build_board.bat                              (build EVERY board)
::   scripts\build_board.bat all [idf.py args...]         (same, custom args)
::   scripts\build_board.bat waveshare build
::   scripts\build_board.bat jc4880 -p COM7 flash monitor
::   scripts\build_board.bat jc4880 reconfigure
setlocal enabledelayedexpansion
set PYTHONUTF8=1
set PYTHONIOENCODING=utf-8

set ROOT=%~dp0..
cd /d "%ROOT%"

:: Keep in sync with release.bat / build_board.sh.
set BOARDS=waveshare jc4880

:: Make idf.py available if the caller forgot to run export.bat.
where idf.py >nul 2>nul
if %errorlevel% neq 0 (
    if defined IDF_PATH (
        call "%IDF_PATH%\export.bat"
    ) else (
        echo build_board: idf.py not found - run export.bat first 1>&2
        exit /b 1
    )
)

set BOARD=%1
if "%BOARD%"=="" set BOARD=all

:: Everything after the board name is forwarded to idf.py verbatim, so
:: `reconfigure` / `flash` / `-p COM7` behave like in the .sh version. %* keeps
:: the original arguments even after shift, hence the collect loop.
set REST=
shift
:collect
if "%~1"=="" goto collected
set REST=%REST% %1
shift
goto collect
:collected
if not defined REST set REST= build

if /i "%BOARD%"=="all" (
    for %%B in (%BOARDS%) do (
        call :run_board %%B
        if !errorlevel! neq 0 exit /b !errorlevel!
    )
    exit /b 0
)

set VALID=0
for %%B in (%BOARDS%) do if /i "%BOARD%"=="%%B" set VALID=1
if "%VALID%"=="0" (
    echo usage: %~nx0 [all^|waveshare^|jc4880] [idf.py args...] 1>&2
    exit /b 2
)

call :run_board %BOARD%
exit /b %errorlevel%

:run_board
set B=%1
if not exist "sdkconfig.defaults.%B%" (
    echo build_board: sdkconfig.defaults.%B% not found 1>&2
    exit /b 1
)
echo ==^> build_board: %B% -^> idf.py%REST%
call idf.py -B "build_%B%" -D SDKCONFIG="build_%B%/sdkconfig" -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.defaults.%B%"%REST%
exit /b %errorlevel%
