
@set QNX_ENV_SCRIPT=%HOMEPATH%\qnx710\qnxsdp-env.bat
@if exist %QNX_ENV_SCRIPT% (call %QNX_ENV_SCRIPT%) else (echo. && echo %QNX_ENV_SCRIPT% not exists && exit /B 1)

@echo off

@REM Build architecture/variant string, possible values: x86, armv7le, etc...
@set PLATFORM=aarch64le

@REM Build profile, possible values: release, debug, profile, coverage
@set BUILD_PROFILE=release

@set ARTIFACT=lvgl_demo
@set QNX_GG_BINARY=%CD%\build\%PLATFORM%-%BUILD_PROFILE%\%ARTIFACT%

@echo.
@echo Start building...

make -f ports\qnx\Makefile clean
make -f ports\qnx\Makefile.lvgl
make -f ports\qnx\Makefile

@if exist %QNX_GG_BINARY% (echo. && echo Building %QNX_GG_BINARY% succeed.) else (echo. && echo Building %QNX_GG_BINARY% fail.)
