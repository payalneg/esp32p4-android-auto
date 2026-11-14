@echo off
rem Find the Android build tools without changing the user's environment.
where /q flutter
if errorlevel 1 (
    if exist "%~dp0..\build\flutter\bin\flutter.bat" (
        set "PATH=%~dp0..\build\flutter\bin;%PATH%"
    ) else (
        echo %~1: Flutter not found in PATH or build\flutter\bin 1>&2
        exit /b 1
    )
)

if not defined ANDROID_HOME if not defined ANDROID_SDK_ROOT (
    if exist "%~dp0..\build\android-sdk\platform-tools\adb.exe" (
        for %%D in ("%~dp0..\build\android-sdk") do set "ANDROID_HOME=%%~fD"
    )
)

if not defined JAVA_HOME (
    where /q java
    if errorlevel 1 (
        for /d %%D in ("%~dp0..\build\jdk17\*") do (
            if not defined JAVA_HOME if exist "%%D\bin\java.exe" set "JAVA_HOME=%%~fD"
        )
    )
)
exit /b 0
