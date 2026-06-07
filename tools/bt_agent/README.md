# Bluetooth Classic Agent (D1 Mini ESP32-WROOM-32)

Standalone ESP-IDF project that runs on a **D1 Mini ESP32** (or any
ESP32-WROOM-32-based board) and acts as the Classic Bluetooth front-end for
the ESP32-P4 Android Auto head unit. It exists to defeat the modern
"Cakewalk" wireless-AA flow that Google rolled out in late 2023, which
silently blocks projection to head units that haven't gone through a real BT
pairing handshake.

## Wiring to the Waveshare ESP32-P4 board

J3 header on the bottom edge:

```
D1 Mini side             ESP32-P4 (J3)
─────────────────────────────────────────
GPIO 1 (TX0)    ────►    GPIO 21  (RX)
GPIO 3 (RX0)    ◄────    GPIO 22  (TX)
GND             ────     GND
5V (or VBUS)    ◄────    5V
```

The agent talks over **UART0** (the standard USB-Serial pins, GPIO 1/3).
The UART2 pads (GPIO 16/17) didn't reliably drive a 3.3 V signal on this
D1 Mini clone — on some clones those pins route to the CH9102 auto-program
circuit — so UART0 is the only path that moves data in both directions.
Trade-off: the on-board CH9102 USB-to-UART bridge shares these lines, so
the USB console and the P4 link both ride GPIO 1/3.

GPIO 21/22 on P4 are free expansion pins, so the USB-C debug console on P4
(GPIO 37/38) stays usable while the agent is wired up.

## Build & flash

The agent's UART link to P4 rides the same GPIO 1 (TX0) / GPIO 3 (RX0)
pins as the on-board CH9102 USB-Serial bridge, so the wires to P4 and the
USB-C cable fight over those lines. Flash in this order:

1. unplug the wires from P4 (GPIO 1 / GPIO 3 / GND / 5V)
2. plug in USB-C, run `idf.py flash` below
3. unplug USB-C, reconnect the wires to P4

Plug the D1 Mini into your laptop via its **USB-C** port. Find the serial
port (usually `/dev/cu.usbserial-XXXX` on macOS), then from this directory:

```
. ~/.espressif/v5.5.3/esp-idf/export.sh   # or wherever IDF lives
idf.py set-target esp32                   # only first time
idf.py -p /dev/cu.usbserial-XXXX flash monitor
```

First build pulls some BT components and is slow (couple of minutes).
Subsequent builds are quick.

## What you should see in the monitor

Boot output:

```
I (NNN) bt_agent: BT classic + SPP up. Discoverable as 'ESP32-P4 AA' (Car Audio)
```

Then on the phone, go to `Settings → Bluetooth → Pair new device` and you
should see **ESP32-P4 AA**. Tap it, accept the pairing prompt. The monitor
should print:

```
I (NNN) bt_agent: SSP confirm req num=... — accepting
I (NNN) bt_agent: auth ok with '<phone-name>'
I (NNN) bt_agent: SPP client connected: handle=...
I (NNN) bt_agent: TX msg=1 payload=N bytes        (WifiStartRequest)
I (NNN) bt_agent: RX msg=2 payload=0 bytes        (WifiInfoRequest)
I (NNN) bt_agent: TX msg=3 payload=N bytes        (WifiInfoResponse)
```

After the WifiInfoResponse, Android Auto on the phone should auto-launch
and try to connect to the P4 head unit at the IP/port we advertised.

## Configuration

`main/main.c` has hard-coded credentials at the top:

- `HU_WIFI_SSID` / `HU_WIFI_PASS` — the LAN credentials to hand to the
  phone. Must match what `main/bench_wifi.h` on the P4 contains.
- `HU_LAN_IP` — IP the P4 currently has (DHCP-assigned). Hard-coded for
  now; future revision will receive it from P4 via UART.
- `HU_LAN_PORT` — TCP port the AAP receiver listens on (5288).

## UART signals to P4 (for future P4-side coordination)

Single line per event, ending in `\n`:

| Line                       | Meaning                                       |
|----------------------------|-----------------------------------------------|
| `BT:BOOT`                  | Agent just powered on                         |
| `BT:READY`                 | SPP server up, waiting for phone              |
| `BT:PAIRED`                | Phone completed Classic BT auth               |
| `BT:CONNECTED`             | Phone opened the SPP/RFCOMM channel           |
| `BT:WIFI_INFO_SENT`        | Sent WifiInfoResponse with creds              |
| `BT:WIFI_CONNECT_STATUS`   | Phone reported WifiConnectStatus              |
| `BT:DISCONNECTED`          | SPP channel closed                            |

P4 doesn't need to read these for the BT pairing experiment to work, but
they're useful for the eventual Mode A coordination layer.
