@echo off
setlocal enabledelayedexpansion

set ROOT=%~dp0..
set VER=%ROOT%\version.txt
set DEST=%ROOT%\flutter-application\assets\firmware
set BOARDS=waveshare jc4880

if not exist "%DEST%" mkdir "%DEST%"

for %%B in (%BOARDS%) do (
    set BIN=%ROOT%\build_%%B\esp32p4_android_auto.bin
    if not exist "!BIN!" (
        echo stage_firmware_asset: !BIN! not found - run scripts\build_board.bat %%B first ^>&2
        exit /b 1
    )
    copy /Y "!BIN!" "%DEST%\esp32p4_android_auto-%%B.bin" >nul
    echo stage_firmware_asset: bundled %%B
)

:: Get first line of version.txt
set /p VERSION=<"%VER%"
:: Trim whitespace if any (usually not needed for set /p, but safe)
echo %VERSION%> "%DEST%\version.txt"
echo stage_firmware_asset: version %VERSION%

:: Fetch helper firmware (esp32c3-ble-helper)
set HELPER_REPO=payalneg/esp32c3-ble-helper
set API_URL=https://api.github.com/repos/%HELPER_REPO%/releases/latest

echo Fetching latest helper firmware...
powershell -NoProfile -Command "$ErrorActionPreference = 'Stop'; try { $rel = Invoke-RestMethod -Uri '%API_URL%' -Headers @{'Accept'='application/vnd.github+json'}; $asset = $rel.assets | Where-Object { $_.name -match '^esp32c3_ble_helper-([0-9][0-9.]*)\.bin$' } | Select-Object -First 1; if ($asset) { $ver = $asset.name -replace '^esp32c3_ble_helper-([0-9][0-9.]*)\.bin$','$1'; Invoke-WebRequest -Uri $asset.browser_download_url -OutFile '%DEST%\esp32c3_ble_helper.bin'; [IO.File]::WriteAllText('%DEST%\esp32c3_ble_helper_version.txt', $ver); Write-Host \"stage_firmware_asset: helper $ver downloaded\" } else { Write-Error 'No asset found' } } catch { exit 1 }"

if %errorlevel% neq 0 (
    echo stage_firmware_asset: helper fetch failed.
    if not exist "%DEST%\esp32c3_ble_helper.bin" (
        echo stage_firmware_asset: no helper staged - app will offer download ^>&2
        type nul > "%DEST%\esp32c3_ble_helper.bin"
        type nul > "%DEST%\esp32c3_ble_helper_version.txt"
    ) else (
        echo stage_firmware_asset: keeping existing helper firmware ^>&2
    )
)
exit /b 0
