# Waveshare ESP32-S3-Touch-LCD-4 (`s3touch4`)

The dashboard-only board: same firmware tree, same VESC dashboard, no Android
Auto. Written down here because the two hardware revisions differ in a way that
looks like a dead display if you guess wrong.

## Module and panel

| | |
|---|---|
| MCU | ESP32-S3-WROOM-1-N16R8 — dual Xtensa @240 MHz, 512 KB SRAM, 16 MB flash, 8 MB **octal** PSRAM |
| Panel | 4", 480×480, ST7701, RGB565 over the 16-bit RGB bus; controller registers over a 3-wire SPI link during init only |
| Touch | GT911, I²C, address 0x5D (0x14 on some units) |
| Radio | on-chip Wi-Fi + BLE 5 — no ESP32-C6, no ESP-Hosted |
| Console | USB-C is the chip's native USB (USB-Serial-JTAG), not a bridge chip |

## Pins

Shared I²C bus: **SDA GPIO15, SCL GPIO7** — touch, the PCF85063A RTC (0x51) and
the board helper all hang off it.

```
RGB   DE 40   VSYNC 39   HSYNC 38   PCLK 41
      B0-B4   5, 45, 48, 47, 21
      G0-G5   14, 13, 12, 11, 10, 9
      R0-R4   46, 3, 8, 18, 17
LCD   3-wire SPI: CS 42, SCL 2, SDA 1     (init only)
SD    1-bit SDMMC: CLK 2, CMD 1, D0 4     (shares CLK/CMD with the LCD link)
CAN   TX 6, RX 0        (TJA1051 on the external header)
RS485 TX 44, RX 43      (not used by this firmware)
```

The SD card and the LCD's control link share GPIO 1 and 2. The ST7701 only
needs that link while it is being initialised, so the card must not be mounted
before `bsp_display_new()` has run — which is the order the firmware boots in.

## Hardware revision — check before flashing

| | V4.0 | V3.0 |
|---|---|---|
| Helper on I²C | CH32V003 at **0x24** | TCA9554 at **0x20** |
| Backlight | PWM, 0–255 (inverted duty) | enable pin only, on/off |
| Also driven | LCD reset, touch reset, system power, buzzer, battery ADC | LCD reset, touch reset, backlight enable |
| Kconfig | `CONFIG_BSP_S3T4_HW_V4` (default) | `CONFIG_BSP_S3T4_HW_V3` |

Tell them apart from the silkscreen, or by which address answers on the bus.
The BSP logs `no IO expander at 0x…` and names the wrong-revision case when it
cannot find the one it was built for. With the wrong revision selected the
panel typically comes up after a warm reset and stays black after a power
cycle, because nothing pulses its reset line.

On V3.0 the brightness slider in Settings degrades to a switch: any value above
zero is full brightness.

Vendor material (schematic, examples, the CH32V003 and RTC components) is at
<https://github.com/waveshareteam/ESP32-S3-Touch-LCD-4> — see CLAUDE.md for the
clone line.

## Memory budget

Internal RAM is the constraint, PSRAM is merely tight:

| | |
|---|---|
| Two 480×480 RGB565 framebuffers | 920 KB PSRAM |
| `.text` + `.rodata` copy (XIP from PSRAM) | ~2.5 MB PSRAM |
| Boot-log ring (`CONFIG_LOG_CAPTURE_SIZE_KB`) | 256 KB PSRAM |
| Notification icon cache, LISP buffers, album art | ~0.6 MB PSRAM |
| LVGL heap (all of it — `LV_MEM_CUSTOM` allocates from PSRAM) | rest |

`CONFIG_SPIRAM_FETCH_INSTRUCTIONS` + `CONFIG_SPIRAM_RODATA` are what keep the
RGB panel from tearing when something writes flash: the LCD's DMA reads its
framebuffer through the cache, and without XIP-from-PSRAM every NVS commit
would disable that cache. It costs the 2.5 MB above, which is why Wi-Fi OTA
falls back to streaming instead of staging the whole image, and why BLE OTA can
run out of PSRAM and ask you to use Wi-Fi instead.

## Bring-up checklist

1. Identify the revision (above) and set it in `menuconfig` if it is V3.0:
   `scripts/build_board.sh s3touch4 menuconfig` → Board Support Package.
2. `scripts/build_board.sh s3touch4 -p /dev/cu.usbmodem* flash monitor`.
3. In the boot log check: `spi_flash` reports 16 MB, `display_init` starts
   printing `render: N fps | avg …ms`, `wifi_manager` brings up the SoftAP,
   `comm_can` starts on TX 6 / RX 0.
4. Watch the panel while something writes flash (open Settings and change a
   value, or run an OTA): no tearing or sideways drift. If it does drift, the
   next lever is `CONFIG_BSP_LCD_RGB_BOUNCE_BUFFER_HEIGHT`, then re-arming
   `flash_shade` (see `main/display_init.c`).
