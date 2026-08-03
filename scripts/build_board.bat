@echo off
setlocal enabledelayedexpansion
set PYTHONUTF8=1
set PYTHONIOENCODING=utf-8

:: Usage:
::   scripts\build_board.bat                       (builds all boards)
::   scripts\build_board.bat waveshare             (builds waveshare)
::   scripts\build_board.bat jc4880                (builds jc4880)

set ROOT=%~dp0..
cd /d "%ROOT%"

:: Make idf.py available if not in PATH
where idf.py >nul 2>nul
if %errorlevel% neq 0 (
    if defined IDF_PATH (
        call "%IDF_PATH%\export.bat"
    ) else (
        echo build_board: idf.py not found - run export.bat first >&2
        exit /b 1
    )
)

set BOARD=%1
if "%BOARD%"=="" set BOARD=all

if "%BOARD%"=="all" (
    call :run_board waveshare
    if !errorlevel! neq 0 exit /b !errorlevel!
    
    call :run_board jc4880
    if !errorlevel! neq 0 exit /b !errorlevel!
    
    exit /b 0
)

call :run_board %BOARD%
exit /b %errorlevel%

:run_board
set B=%1
if not exist "sdkconfig.defaults.%B%" (
    echo build_board: sdkconfig.defaults.%B% not found >&2
    exit /b 1
)

echo ==^> build_board: %B% -^> idf.py build
call idf.py -B "build_%B%" -D SDKCONFIG="build_%B%/sdkconfig" -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.defaults.%B%" build
exit /b %errorlevel%
