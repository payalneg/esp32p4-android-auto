# porting LVGL to QNX

Steps to build and run LVGL application for QNX

## Build
First, source QNX toolchain environment script `qnxsdp-env.bat` or `qnxsdp-env.sh`.
Then, run commands:
``` shell
cd <exported codes folder>
make -f ports/qnx/Makefile.lvgl
make -f ports/qnx/Makefile
```
liblvgl.so and lvgl_demo are generated under folder build/

## Run on board
1. ensure screen is not running
```shell
slay screen
```

2. enable mounse + keyboard and start screen server
```shell
io-hid -dusb upath=/dev/usb1/io-usb-otg
screen -c /usr/lib/graphics/iMX93/graphics.conf
```
**Note**: Check io-usb-otg path. Default is /dev/usb/io-usb-otg. Use upath= parameter for cusomized path.

**Note**: replace "iMX93" to use graphics.conf for your target board

**Note**: io-hid and devi-hid binaries might not be present on your target by default. You can find the binaries in qnx710 folder.

3. Upload liblvgl.so, lvgl_demo to /tmp (or integrate to your QNX image)

**Note**: when connect to the board using the WinSCP, please select FTP protocol, the user name/passwd is qnxuser/qnxuser

4. Run the demo (in case of binaries are put under /tmp)
```shell
chmod a+x /tmp/lvgl_demo
LD_LIBRARY_PATH=/tmp /tmp/lvgl_demo
```
