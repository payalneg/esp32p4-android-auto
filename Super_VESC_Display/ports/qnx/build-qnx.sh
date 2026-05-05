#!/bin/bash

QNX_ENV_SCRIPT=$HOME/qnx710/qnxsdp-env.sh
if [ -r $QNX_ENV_SCRIPT ];then
    . $QNX_ENV_SCRIPT
else
    echo "$QNX_ENV_SCRIPT not exists."
    exit 1
fi

if ! [ -r ports/qnx/Makefile ] || ! [ -e ports/qnx/Makefile.lvgl ];then
    echo "qnx/Makefile or qnx/Makefile.lvgl not exists."
    exit 1
fi

PLATFORM=aarch64le
BUILD_PROFILE=release
ARTIFACT=lvgl_demo

export PLATFORM BUILD_PROFILE ARTIFACT

QNX_GG_BINARY=build/$PLATFORM-$BUILD_PROFILE/$ARTIFACT

echo Start building...

make -f ports/qnx/Makefile clean
make -f ports/qnx/Makefile.lvgl
make -f ports/qnx/Makefile

if [ -e $QNX_GG_BINARY ];then
    echo
    echo "Building $QNX_GG_BINARY succeed."
else
    echo
    echo "Building $QNX_GG_BINARY fail."
fi
