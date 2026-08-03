@echo off
setlocal enabledelayedexpansion

set HOST=%~1
if "%HOST%"=="" set HOST=192.168.4.1

if not defined PORT set PORT=80
if not defined BIN set BIN=build\esp32p4_android_auto.bin

if not exist "%BIN%" (
    echo ota_push: %BIN% not found - run idf.py build first ^>&2
    exit /b 1
)

for %%I in ("%BIN%") do set SIZE=%%~zI
set URL=http://%HOST%:%PORT%/ota

echo ota_push: uploading %BIN% (%SIZE% bytes) -^> %URL%

curl --fail --show-error -# --no-buffer --max-time 600 -H "Content-Type: application/octet-stream" --data-binary "@%BIN%" "%URL%"
if %errorlevel% neq 0 (
    echo ota_push: upload failed ^>&2
    exit /b 1
)
echo.
echo ota_push: device is rebooting
exit /b 0
