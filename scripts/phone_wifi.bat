@echo off
:: Windows wrapper for scripts\phone_wifi.py - debug the phone over Wi-Fi
:: (ADB over TCP: pair / connect / discover / status / logs).
::
:: Usage:
::   scripts\phone_wifi.bat discover
::   scripts\phone_wifi.bat status
::   scripts\phone_wifi.bat logs --grep usbflash
setlocal
set PYTHONUTF8=1
set PYTHONIOENCODING=utf-8
python -u "%~dp0phone_wifi.py" %*
exit /b %errorlevel%
