#!/bin/sh

toolchain=$1
if [ -z "$toolchain" ];then
    toolchain=/opt/fsl-imx-xwayland/6.6-scarthgap/sysroots/x86_64-pokysdk-linux/usr/share/cmake/armv8a-poky-linux-toolchain.cmake
fi
toolchain_path=$(echo $toolchain |sed -E 's,^(.*)/sysroots/.*,\1,')
toolchain_arch=$(basename $toolchain |sed -e 's,-toolchain.cmake$,,')
if [ ! -r $toolchain -o ! -r "$toolchain_path/environment-setup-$toolchain_arch" ];then
    echo "ERROR: Yocto Toolchain not installed?"
    exit 1
fi

if [ -n "$BASH_SOURCE" ]; then
    ROOTDIR="`readlink -f $BASH_SOURCE | xargs dirname`"
elif [ -n "$ZSH_NAME" ]; then
    ROOTDIR="`readlink -f $0 | xargs dirname`"
else
    ROOTDIR="`readlink -f $PWD | xargs dirname`"
fi

BUILDDIR=$ROOTDIR/../../build
rm -fr $BUILDDIR
mkdir $BUILDDIR
. "$toolchain_path/environment-setup-$toolchain_arch"
cd $ROOTDIR/lv_drivers/wayland/
rm -fr CMakeCache.txt  CMakeFiles  cmake_install.cmake Makefile
cmake .
make
cd $BUILDDIR
cmake -G 'Ninja' .. -DCMAKE_TOOLCHAIN_FILE=$toolchain -Wno-dev -DLV_CONF_BUILD_DISABLE_EXAMPLES=1 -DLV_CONF_BUILD_DISABLE_DEMOS=1
ninja
if [ -e gui_guider ];then
    echo "Binary locates at $(readlink -f gui_guider)"
    ls -lh gui_guider
fi

