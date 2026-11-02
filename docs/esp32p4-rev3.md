# Waveshare 4.3: ESP32-P4 rev3.x / v3.2

The chip revision printed by esptool is the silicon revision, not the board
model or PCB revision. ESP32-P4 v1.x and v3.x require separate firmware builds.
Do not bypass a revision mismatch with `--force` or edit the image header.

| Chip reported by esptool | Build profile | Build/config directory |
| --- | --- | --- |
| ESP32-P4 v1.x | `waveshare` | `build_waveshare` |
| ESP32-P4 v3.x (including v3.2) | `waveshare-rev3` | `build_waveshare-rev3` |

Use ESP-IDF **v5.5.6** for this release (minimum v5.5.3 with v3.x support).
The new profile disables the ROM watchdog implementation to avoid
the v3.2 watchdog workaround mismatch in older IDF releases. It uses 200 MHz
PSRAM with the startup memory test enabled and DIO / 80 MHz external flash.
Starting with 1.3.35, the BSP leaves the DSI PHY clock source unset so IDF
selects its revision-aware default (XTAL on v3.x, PLL_F20M on v1.x).
Version 1.3.34 incorrectly used the deprecated `MIPI_DSI_PHY_CLK_SRC_DEFAULT`
alias, which still means PLL_F20M and triggers `abort()` in the v3.x HAL.
The reported 1.3.34 crash at PC `0x4010abe3` resolves to
`_mipi_dsi_ll_set_phy_pllref_clock_source`; flash and PSRAM initialization
had completed before this display startup failure.

## Build and flash on Windows

In the ESP-IDF terminal:

```bat
scripts\build_board.bat waveshare-rev3 build
scripts\build_board.bat waveshare-rev3 -p COM15 -b 115200 flash monitor
```

Use your actual COM port. Before flashing, verify the esptool version in that
same terminal. P4 v3.2 needs at least 4.12.dev2 on the IDF 5.5 branch or
5.3.dev3 on the IDF 6.x branch. If IDF still installs an older tool, update the
IDF installation and its dependencies. Do not force a pip upgrade past IDF's
constraints in its managed Python environment.

Windows builds need long-path support enabled. If the component manager fails
while unpacking or copying deeply nested `esp-serial-flasher/test/...` files,
enable Win32 long paths in Windows policy (`LongPathsEnabled=1`) and restart
the build from a new process. This is a host build issue, separate from the
board's serial flashing errors.

On Linux/macOS use `scripts/build_board.sh waveshare-rev3 build` and the same
profile for `flash monitor`. The separate directory avoids inheriting an
existing v1.x sdkconfig: defaults do not override an already saved sdkconfig.

## Single-file recovery image

Release scripts build both Waveshare profiles and produce separate files:

```text
esp32p4_android_auto-waveshare-<version>-merged.bin       # v1.x only
esp32p4_android_auto-waveshare-rev3-<version>-merged.bin  # v3.x only
```

Write a merged image at **0x0**. An application-only `.bin` is not a recovery
image. Earlier 1.3.33 Waveshare images target v1.x and cannot run on v3.x.

`release/flash.py` requires stable esptool 4.12+ or 5.3+ and checks the connected P4
revision against the merged bootloader header before writing. Run this helper
with a standalone Python environment if the IDF environment still pins an older tool:

```powershell
py -m venv build_flash_tools
.\build_flash_tools\Scripts\python -m pip install "esptool>=5.3"
.\build_flash_tools\Scripts\python release\flash.py
```

The default transfer rate is 115200. Use the USB-UART connector. If automatic
reset fails, hold BOOT, press and release RESET, then release BOOT. Capture the
115200-baud boot log after reset if the display remains black.

The companion app bundles both profiles. A v3.x build advertises
`waveshare-rev3` over BLE and HTTP so the app selects the matching OTA image.
The picker distinguishes P4 v1.x and v3.x and blocks a mismatch with a detected
device. Unknown explicit profiles are rejected; if no model is reported,
the user must select a profile manually. The first recovery flash still needs
USB. Do not rename the v1.x image to `waveshare-rev3`.

References:

- [Espressif v3.x migration guide](https://documentation.espressif.com/esp32-p4-chip-revision-v3.x_user_guide_en.pdf)
- [Espressif v3.2 compatibility advisory AR2026-004](https://documentation.espressif.com/AR2026-004_Compatibility_Advisory_for_ESP32-P4_Chip_Revision_v3.2_EN.html)
