@echo off
:: Windows counterpart of scripts/install_app.sh — adb-install a staged release
:: APK. With no argument, picks the newest release\aa_bridge-*.apk.
::
:: Usage:
::   scripts\install_app.bat            newest release\aa_bridge-*.apk
::   scripts\install_app.bat 0.3.6      release\aa_bridge-0.3.6.apk
::   scripts\install_app.bat path\to.apk
setlocal enabledelayedexpansion

set ROOT=%~dp0..
set REL=%ROOT%\release
set PKG=com.aabridge.aa_bridge

where adb >nul 2>nul
if %errorlevel% neq 0 (
    echo install_app: adb not found - install Android platform-tools 1>&2
    exit /b 1
)

set ARG=%1
if "%ARG%"=="" (
    set APK=
    for /f "delims=" %%I in ('python -c "import glob, os; a=sorted(glob.glob(r'%REL%' + '/aa_bridge-*.apk'), key=os.path.getmtime); print(a[-1] if a else '')"') do set APK=%%I
    if "!APK!"=="" (
        echo install_app: no release\aa_bridge-*.apk found - run scripts\release.bat first 1>&2
        exit /b 1
    )
) else (
    if exist "%ARG%" (
        set APK=%ARG%
    ) else (
        set APK=%REL%\aa_bridge-%ARG%.apk
    )
)

if not exist "!APK!" (
    echo install_app: !APK! not found 1>&2
    exit /b 1
)

set N=0
for /f "skip=1 tokens=1,2" %%A in ('adb devices') do (
    if "%%B"=="device" set /a N+=1
)

if !N!==0 (
    echo install_app: no device. Plug in the phone, enable USB debugging, accept the prompt. 1>&2
    adb devices 1>&2
    exit /b 1
)
if !N! gtr 1 (
    if not defined ANDROID_SERIAL (
        echo install_app: !N! devices connected - pin one with set ANDROID_SERIAL=^<serial^> 1>&2
        adb devices 1>&2
        exit /b 1
    )
)

echo install_app: installing !APK!
call adb install -r "!APK!"
if %errorlevel% neq 0 (
    echo.
    echo install_app: install failed. Common causes:
    echo   - signature mismatch ^(APK signed with a different key than what's on the phone^)
    echo   - version downgrade ^(the installed build is newer^)
    echo   Force-replace ^(LOSES app data + BLE pairing^):
    echo       adb uninstall %PKG% ^&^& scripts\install_app.bat %ARG%
    exit /b 1
)
echo install_app: done.
exit /b 0
