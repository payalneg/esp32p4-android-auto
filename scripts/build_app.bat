@echo off
:: Windows counterpart of scripts/build_app.sh — build the companion APK.
::
:: Usage:
::   scripts\build_app.bat                  release APK, no embedded LLM key
::   scripts\build_app.bat --with-key       XOR-mask .env's LLM_API_KEY into the
::                                          build (PERSONAL builds only — it is
::                                          recoverable from the APK)
::   scripts\build_app.bat --debug --install
setlocal enabledelayedexpansion

set ROOT=%~dp0..
set APP=%ROOT%\flutter-application
set ENV_FILE=%ROOT%\.env
set MASK=aa-bridge/vesc-display/2026

set MODE=release
set INSTALL=0
set USE_KEY=0

:parse_args
if "%~1"=="" goto run
if "%~1"=="--install" set INSTALL=1
if "%~1"=="--with-key" set USE_KEY=1
if "%~1"=="--no-key" set USE_KEY=0
if "%~1"=="--debug" set MODE=debug
shift
goto parse_args

:run
set DEFINES=
if %USE_KEY%==1 (
    if exist "%ENV_FILE%" (
        for /f "tokens=1,* delims==" %%a in ('findstr "^LLM_API_KEY=" "%ENV_FILE%"') do set KEY=%%b
        if defined KEY (
            set KEY=!KEY:"=!
            set KEY=!KEY: =!
            for /f "delims=" %%I in ('python -c "import base64, os; key = os.environ['KEY'].encode(); mask = os.environ['MASK'].encode(); print(base64.b64encode(bytes(b ^^ mask[i %% len(mask)] for i, b in enumerate(key))).decode())"') do set OBF=%%I
            set DEFINES=--dart-define="LLM_KEY_OBF=!OBF!"
            echo build_app: embedding LLM key from .env
            echo build_app: NOTE - recoverable from the APK; do not share this build
            echo build_app: NOTE - do NOT stage this build into release\ (see release.bat)
        ) else (
            echo build_app: .env has no LLM_API_KEY - building without an embedded key
        )
    ) else (
        echo build_app: .env not found
    )
) else (
    echo build_app: no embedded key (the app asks the user for one)
)

cd /d "%APP%"
call flutter pub get
if %errorlevel% neq 0 exit /b 1
if "%MODE%"=="release" (
    call flutter build apk --release --obfuscate --split-debug-info=build/symbols !DEFINES!
    set APK=%APP%\build\app\outputs\flutter-apk\app-release.apk
) else (
    call flutter build apk --debug !DEFINES!
    set APK=%APP%\build\app\outputs\flutter-apk\app-debug.apk
)
if %errorlevel% neq 0 exit /b 1

echo build_app: !APK!

if %INSTALL%==1 (
    where adb >nul 2>nul
    if !errorlevel! neq 0 (
        echo build_app: adb not found 1>&2
        exit /b 1
    )
    adb install -r "!APK!"
)
exit /b 0
