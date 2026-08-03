@echo off
setlocal enabledelayedexpansion
set PYTHONUTF8=1
set PYTHONIOENCODING=utf-8

set ROOT=%~dp0..
cd /d "%ROOT%"

set BOARDS=waveshare jc4880

where idf.py >nul 2>nul
if %errorlevel% neq 0 (
    if defined IDF_PATH (
        call "%IDF_PATH%\export.bat"
    ) else (
        echo release: idf.py not found - run export.bat first >&2
        exit /b 1
    )
)
where flutter >nul 2>nul
if %errorlevel% neq 0 (
    echo release: flutter not found in PATH >&2
    exit /b 1
)

set NEW_FW=%1
set NEW_APP=%2

echo ==^> Bumping versions
python -c "import sys, re; fw_new = sys.argv[1] if len(sys.argv)>1 else ''; app_new = sys.argv[2] if len(sys.argv)>2 else ''; f = open('version.txt').read().strip(); fw = fw_new if fw_new else f.rsplit('.',1)[0]+'.'+str(int(f.rsplit('.',1)[1])+1); open('version.txt','w').write(fw+'\n'); p = open('flutter-application/pubspec.yaml').read(); app_ver = re.search(r'^version:\s*([0-9\.]+)\+([0-9]+)', p, re.M); a_base = app_ver.group(1); a_bld = int(app_ver.group(2)); app = app_new if app_new else a_base.rsplit('.',1)[0]+'.'+str(int(a_base.rsplit('.',1)[1])+1); open('flutter-application/pubspec.yaml','w').write(re.sub(r'^version:.*', f'version: {app}+{a_bld+1}', p, flags=re.M)); print(f'FW: {f} -^> {fw}'); print(f'APP: {a_base}+{a_bld} -^> {app}+{a_bld+1}'); open('.release_tmp.txt','w').write(f'{fw}\n{app}')" %NEW_FW% %NEW_APP%

if exist .release_tmp.txt (
    for /f "tokens=*" %%i in (.release_tmp.txt) do (
        if not defined FW_VER ( set FW_VER=%%i ) else ( set APP_VER=%%i )
    )
    del .release_tmp.txt
)

for %%B in (%BOARDS%) do (
    echo ==^> build %%B: reconfigure + build
    call scripts\build_board.bat %%B reconfigure
    if !errorlevel! neq 0 exit /b !errorlevel!
    call scripts\build_board.bat %%B build
    if !errorlevel! neq 0 exit /b !errorlevel!
)

echo ==^> staging firmwares into the Flutter app
call scripts\stage_firmware_asset.bat
if !errorlevel! neq 0 exit /b !errorlevel!

echo ==^> flutter build apk
call scripts\build_app.bat
if !errorlevel! neq 0 exit /b !errorlevel!

set APK=flutter-application\build\app\outputs\flutter-apk\app-release.apk
if not exist "%APK%" (
    echo release: built APK not found >&2
    exit /b 1
)

echo ==^> staging release artifacts
if not exist release mkdir release
del /q release\esp32p4_android_auto-*.bin 2>nul
del /q release\aa_bridge-*.apk 2>nul

for %%B in (%BOARDS%) do (
    copy /Y "build_%%B\esp32p4_android_auto.bin" "release\esp32p4_android_auto-%%B-%FW_VER%.bin" >nul
    
    for /f "delims=" %%I in ('python -c "import json,sys; fa=json.load(open(sys.argv[1]))['flash_files']; bdir=sys.argv[2]; print(' '.join('{} {}/{}'.format(a,bdir,f) for a,f in sorted(fa.items(),key=lambda kv:int(kv[0],16))))" "build_%%B\flasher_args.json" "build_%%B"') do set PAIRS=%%I
    
    python -m esptool --chip esp32p4 merge_bin -o "release\esp32p4_android_auto-%%B-%FW_VER%-merged.bin" !PAIRS! >nul
)
copy /Y "%APK%" "release\aa_bridge-%APP_VER%.apk" >nul

echo ==^> done.
for %%B in (%BOARDS%) do (
    echo     firmware : release\esp32p4_android_auto-%%B-%FW_VER%.bin
    echo     flasher  : release\esp32p4_android_auto-%%B-%FW_VER%-merged.bin
)
echo     apk      : release\aa_bridge-%APP_VER%.apk
echo     USB flash: release\flash.bat
echo.
echo Review, then commit:
echo     git add -A version.txt flutter-application/pubspec.yaml release/
echo     git commit -m "release: fw %FW_VER% + app %APP_VER%"
