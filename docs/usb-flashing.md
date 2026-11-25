# Flashing the head unit from the Android app over USB

The companion app (`flutter-application/`) can write the ESP32-P4's flash
directly from a phone over a USB-C cable. It is the third update path next to
WiFi OTA and BLE OTA, and the only one that works when the firmware no longer
boots: it talks to the ROM loader in the chip itself, not to the application.

## Requirements

- An Android phone with USB host support (USB-C OTG adapter if the phone's port
  is not already a host port).
- A **data** USB-C cable. Charge-only cables enumerate nothing.
- Enough power for the board. A P4 head unit draws more than a phone port
  usually supplies; a powered USB hub (phone → hub → board) is the reliable
  setup. Brown-outs during the erase are the most common cause of a failed
  flash.
- The board's USB port:

  | Port | Chip | Notes |
  | --- | --- | --- |
  | `USB` | ESP32-P4 USB-Serial/JTAG (VID 0x303A) | CDC-ACM; reset through the emulated DTR/RTS lines |
  | `UART` | CH343/CH343P bridge (VID 0x1A86, PID 0x55D3) | auto-reset through DTR/RTS; link is raised to 921600 Bd |

  Both are accepted. If one port is not recognized by the phone, try the other.

## In the app

1. *Settings → Firmware update* → method **USB**.
2. Choose the source:
   - **Bundled in app** — the image built into the APK. By default it is written
     as a full recovery image: bootloader at 0x2000, partition table at 0x8000,
     app at 0x20000. This resets the NVS (pairing, settings) and boots slot
     `ota_0`. The **Application only** switch writes just the app partition at
     0x20000 instead, keeps NVS/settings, and clears the OTA boot selector so
     the board boots that slot. It still needs the bootloader and partition
     table to already match.
   - **Custom file** — any `.bin`. A merged image is written at 0x0; an
     application-only image at the offset you enter (0x20000 by default for
     this project's P4 partition table, 0x10000 for the ESP-IDF default). An
     app-only write at **0x20000** also clears the OTA selector so the board
     boots that slot; at any other offset the selector is left alone, so make
     sure the board is already set to boot the slot you are writing.
3. Tap the refresh icon, pick the USB device, and press **Flash over USB**.
   Android asks once for permission to use that USB device.
4. Keep the cable and power connected, and keep the app in the foreground.
   The app shows erase → write → verify → reboot. The write is verified with the
   ROM's MD5 command.

## What is checked before anything is erased

- The image header's **chip id** must match the connected chip (GET_SECURITY_INFO
  from the ROM). A P4 image on an S3, or vice versa, is refused.
- For ESP32-P4, the image's **silicon revision range** (bootloader header for a
  merged image, app header for an application image) must include the chip's
  revision: v1.x images refuse a v3.x chip and vice versa. This is the same
  guard as `release/flash.py`, see `docs/esp32p4-rev3.md`.
- Application-only images must be written at a 4 KB-aligned offset.

The check runs before `FLASH_BEGIN`, which is what erases the region.

## If the chip does not answer

The app resets the chip into download mode through DTR/RTS (classic auto-reset
on UART, the USB-JTAG reset sequence on the native port). If the board does not
have that circuit wired, or the port is busy:

1. Hold **BOOT**.
2. Press and release **RESET**.
3. Release **BOOT**.
4. Press *Flash over USB* again.

The error message in that case is "The chip didn't answer…".

## Watching a failed flash: where the two logs are

A USB flash has two sides, and they fail in different places:

- **Phone side** — every phase the flasher enters is printed to logcat, one line
  per phase plus every 10 % of the write:

  ```bash
  adb logcat -s flutter | grep --line-buffered usbflash
  # [usbflash] opening ...
  # [usbflash] connecting chip=ESP32-P4 rev=1.3
  # [usbflash] writing chip=... 40% fw.usb.phase.write name=...
  # [usbflash] error fw.usb.err.sync err=No SYNC reply from the ROM loader
  ```

  While the phone is the USB *host* there is no ADB over USB — use wireless
  debugging instead: `scripts/phone_wifi.py` (see the capture-logs skill),
  `scripts\phone_wifi.bat logs --grep usbflash`.

  `fw.usb.err.permission` / `fw.usb.err.open` mean Android never handed the
  device over (permission dialog, OTG or charge-only cable); `fw.usb.err.sync`
  means the port opened but the ROM never answered; `fw.usb.err.rev` /
  `fw.usb.err.chip` mean the image does not match the silicon (checked before
  anything is erased); a timeout *after* `writing` started is the link dying
  mid-transfer (power, cable, or 921600 too fast for the bridge).
- **Device side** — build the firmware with `CONFIG_DEBUG_WIFI_BRIDGE=y` and tail
  it from a laptop on the head unit's own Wi-Fi:

  ```bash
  python scripts/wifi_debug.py -H 192.168.4.1 logs
  ```

  The device log stops the moment a reset lands the chip in the ROM loader —
  that silence is expected and means the reset worked. If the app reports
  "didn't answer" while the device log keeps running, the reset never reached
  the chip (DTR/RTS not wired, or the app is talking to the wrong port); if the
  log stops and the app still times out, the failure is on the phone side of the
  link (cable/power/baud).

## Why no flasher stub

The ROM loader is used as-is: no stub binary to ship, no RAM download step, and
nothing that can go wrong before the first flash command. The trade-off is no
compressed writes. On a UART bridge the link is switched to 921600 Bd after
connecting (a 4.8 MB image takes about a minute instead of ~7 at 115200); on the
native USB port the baud rate is virtual and the transfer is USB-limited.

## Building the bundled recovery image into the APK

`scripts/stage_firmware_asset.sh` copies each board's application `.bin` and
writes `esp32p4_android_auto-<board>-head.bin` — the first 0x20000 bytes of the
merged image (0xFF padding, bootloader at 0x2000, partition table at 0x8000) from
that build's `flasher_args.json`. `FirmwareUpdater.usbImageFor()` concatenates
the head with the application at flash time, so the APK carries one application
image per board plus a ~128 KB (highly compressible) head, not a second full
copy.

Check the staged result without a phone:

```bash
cd flutter-application
dart run tool/check_usb_image.dart
dart run tool/check_usb_image.dart path/to/third-party.bin
```

The output shows the detected kind (merged/application), flash offset, target
chip and revision range, and exits non-zero for an invalid image.

## CH343 support

The published `usb_serial` 0.6.0 only knows CH340/CH341. The head unit's UART
port is a CH343P, so the plugin is vendored at
`flutter-application/packages/usb_serial` and patched:

- PID `1a86:55d3` added to the probe table;
- the CH343 baud-rate encoding is used (the CH340 921600/307200 special cases do
  not apply);
- the CH340 register handshake is replaced with a short CH343 init whose vendor
  requests may fail without aborting the port, because the chip already comes up
  at 115200 8N1.

Do not switch the app back to the hosted `usb_serial` package without porting
that patch, or the UART port stops being flashable from Android.
