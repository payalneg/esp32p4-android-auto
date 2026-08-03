@echo off
setlocal

set IDF_PATH=D:\Espressif\frameworks\esp-idf-v5.5.4
set IDF_TOOLS_PATH=D:\Espressif

:: Setup python from the virtual env
set PATH=D:\Espressif\python_env\idf5.5_py3.11_env\Scripts;%PATH%

:: Run export.bat to set up tool paths
call "%IDF_PATH%\export.bat"

:: Navigate to project
cd /d "D:\git\esp32p4-android-auto"

:: Build waveshare
echo ==== Building WAVESHARE ====
call idf.py -B build_waveshare -D SDKCONFIG=build_waveshare/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.defaults.waveshare" build
if errorlevel 1 (
    echo WAVESHARE BUILD FAILED
    exit /b 1
)

:: Build jc4880
echo ==== Building JC4880 ====
call idf.py -B build_jc4880 -D SDKCONFIG=build_jc4880/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.defaults.jc4880" build
if errorlevel 1 (
    echo JC4880 BUILD FAILED
    exit /b 1
)

:: Stage firmware into Flutter assets
echo ==== Staging firmware assets ====
set DEST=flutter-application\assets\firmware
if not exist "%DEST%" mkdir "%DEST%"

copy /Y "build_waveshare\esp32p4_android_auto.bin" "%DEST%\esp32p4_android_auto-waveshare.bin"
copy /Y "build_jc4880\esp32p4_android_auto.bin" "%DEST%\esp32p4_android_auto-jc4880.bin"

:: Copy version.txt
for /f "tokens=*" %%a in (version.txt) do (
    echo %%a> "%DEST%\version.txt"
    goto :done_ver
)
:done_ver

echo ==== Building Flutter APK ====
cd flutter-application
call D:\flutter\bin\flutter.bat pub get
call D:\flutter\bin\flutter.bat build apk --release
if errorlevel 1 (
    echo FLUTTER BUILD FAILED
    exit /b 1
)

echo ==== BUILD COMPLETE ====
echo APK: flutter-application\build\app\outputs\flutter-apk\app-release.apk
