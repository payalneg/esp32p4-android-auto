# JK-BMS over BLE — PC probe

Read the state of a **JK-BMS** (Jikong) battery over Bluetooth LE from a
computer, before touching the head-unit firmware. This is the "does the
protocol work on my pack?" tool: it scans, connects, requests a status frame,
verifies the checksum, and prints SOC / remaining Ah / voltage / current /
temperatures / cell extremes.

Tested against a **JK-B2A24S15P** frame layout; the decoder also handles the
32-cell `JK_PB…` variant and the older `JK04` (float) protocol. The
24S/32S layout is auto-detected — see `detect_variant()` in `jkbms.py`.

## Install

Python 3.9+ and [bleak](https://github.com/hbldh/bleak):

```bash
python -m pip install bleak
```

On Windows this uses the native WinRT BLE stack; on Linux BlueZ; on macOS
CoreBluetooth. No pairing or PIN is needed for read-only status frames.

## Use

```bash
# 1. find the pack
python read_bms.py --scan

# 2. read it (address copied from the scan)
python read_bms.py --address AA:BB:CC:DD:EE:FF

# follow live, once a second, with every cell voltage
python read_bms.py --address AA:BB:CC:DD:EE:FF --watch 1 --cells

# model / firmware / serial
python read_bms.py --address AA:BB:CC:DD:EE:FF --device-info

# machine-readable
python read_bms.py --address AA:BB:CC:DD:EE:FF --json
```

Typical output:

```
JK-BMS JK02_24S (JK-B2A24S15P)
  SOC              : 84 %
  Capacity         : 68.49 Ah / 81.00 Ah
  Voltage          : 53.25 V
  Current          : -4.12 A (discharge)
  Power            : -219.4 W
  Temperatures     : MOS 21.0 C, T1 19.0 C, T2 19.1 C
  Cells            : 3.327 .. 3.329 V (delta 2 mV)
  Status           : cycles 0, SOH 100%, charge MOS on, discharge MOS on
```

The scripts must be run from this directory (they import `jkbms.py`).

## If it does not connect

- **Close the JK-BMS phone app first.** The BMS accepts one BLE connection;
  while the phone holds it, every other client is refused or sees "service not
  found".
- The BMS stops advertising after a while when idle. Press the button / wake it
  (or plug in a charger) and scan again.
- Windows: if the device was previously paired at OS level, BLE can still work,
  but "Remove device" in Bluetooth settings is the clean reset.
- `--all-devices` stops the JK/BMS name filter and lists everything in range.

## Protocol summary

Service `0000ffe0-…`, characteristic `0000ffe1-…` (one write-only and one
notify-only instance on some units; the client picks both).

Request (20 bytes):

```
AA 55 90 EB 96 00 00 00 00 00 00 00 00 00 00 00 00 00 00 10   cell info
AA 55 90 EB 97 00 00 00 00 00 00 00 00 00 00 00 00 00 00 11   device info
```

Reply (300 bytes, split over several BLE notifications):

```
55 AA EB 90 <type> <counter> <payload…> <crc>
```

`type` 0x01 settings, 0x02 cell info, 0x03 device info. `crc` is the 8-bit sum
of the first 299 bytes. Key offsets in a 0x02 frame (24-cell layout):

| Offset | Field | Scale |
|---|---|---|
| 6 + 2·i | cell i voltage | 0.001 V |
| 118 | pack voltage (u32) | 0.001 V |
| 126 | current (i32, + = charge) | 0.001 A |
| 130 / 132 | temperature 1 / 2 (i16) | 0.1 °C |
| 134 | MOSFET temperature (i16) | 0.1 °C |
| 141 | SOC | 1 % |
| 142 | remaining capacity (u32) | 0.001 Ah |
| 146 | full charge capacity (u32) | 0.001 Ah |
| 150 | cycle count (u32) | 1 |
| 158 | SOH | 1 % |
| 166 / 167 | charge / discharge MOSFET | 0/1 |

The 32-cell (`JK_PB…`) layout is the same shifted by +32 bytes; its MOSFET
temperature lives at 144 and its error bitmask where 24S puts the MOSFET
temperature.

## Sources / prior art

- [syssi/esphome-jk-bms](https://github.com/syssi/esphome-jk-bms) — the
  reference open-source implementation (MIT). This tool's frame layouts,
  checksum, BLE UUIDs and 24S/32S/JK04 distinctions come from
  `components/jk_bms_ble/jk_bms_ble.cpp`, including the captured sample frames
  used in `test_jkbms.py`.
- [jblance/mpp-solar](https://github.com/jblance/mpp-solar) — earlier
  protocol notes for the JK BMS serial/BLE frames.
- [Louisvdw/dbus-serialbattery](https://github.com/Louisvdw/dbus-serialbattery)
  — independently maintained JK-BMS implementation and field notes.

## Tests

`test_jkbms.py` decodes real captured frames (no hardware required):

```bash
python -m unittest -v
```
