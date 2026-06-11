#!/usr/bin/env python3
"""Host side of the ESP32-P4 UART debug bridge (main/debug_uart_bridge.c).

Talks to the esp_console REPL on the device's UART0 console to:
  * grab a screenshot of the live LVGL screen and save it as a PNG, and
  * inject synthetic touch events (tap / swipe / low-level down/move/up).

The port is opened in NO-RESET mode by default: DTR/RTS are held so the
ESP32 auto-reset circuit on the USB-UART bridge does not fire, i.e. connecting
does NOT reboot the device and the current screen stays intact (same idea as
`esptool --before no_reset`). Pass --allow-reset to open normally.

Wire protocol for screenshots (logging is muted on the device during this):
    SCR-BEGIN w=800 h=480 fmt=jpeg len=<N> crc32=0x<hex> chunks=<K>
    SCR-DATA 0000 <base64 of 768-byte chunk>
    ...
    SCR-END
Any line not starting with "SCR-" is ignored, so stray log/prompt lines are
tolerated; the payload is verified by chunk count, total length and (advisory)
CRC32.

Requires: pyserial (`pip install pyserial`), Pillow. numpy is used if present
to speed up raw RGB565 decode.

Examples:
    python3 scripts/uart_debug.py -p /dev/tty.usbserial-XXXX screenshot scr.png
    python3 scripts/uart_debug.py -p /dev/tty.usbserial-XXXX screenshot scr.png --fmt rgb565
    python3 scripts/uart_debug.py -p /dev/tty.usbserial-XXXX tap 400 240
    python3 scripts/uart_debug.py -p /dev/tty.usbserial-XXXX swipe 700 240 100 240 300
"""

from __future__ import annotations

import argparse
import base64
import io
import os
import re
import sys
import time
import zlib

HDR_RE = re.compile(
    r"SCR-BEGIN\s+w=(\d+)\s+h=(\d+)\s+fmt=(\w+)\s+len=(\d+)\s+"
    r"crc32=0x([0-9a-fA-F]+)\s+chunks=(\d+)")


def _die(msg: str) -> "NoReturn":  # type: ignore[name-defined]
    print(f"error: {msg}", file=sys.stderr)
    sys.exit(1)


def _import_serial():
    try:
        import serial  # noqa: F401
        return serial
    except ImportError:
        _die("pyserial not installed — run: pip install pyserial")


def open_port(port: str, baud: int, allow_reset: bool):
    serial = _import_serial()
    ser = serial.Serial()
    ser.port = port
    ser.baudrate = baud
    ser.timeout = 0.2
    # No hardware/software handshake; we manage the modem lines ourselves.
    ser.dsrdtr = False
    ser.rtscts = False
    ser.xonxoff = False
    if not allow_reset:
        # Configure the initial line state BEFORE open so the port comes up
        # with DTR/RTS deasserted and the EN/IO0 reset pulse never happens.
        ser.dtr = False
        ser.rts = False
    try:
        ser.open()
    except serial.SerialException as e:  # type: ignore[attr-defined]
        _die(f"cannot open {port}: {e}\n"
             "       (is `idf.py monitor` still attached? only one process "
             "can own the port)")
    if not allow_reset:
        # Re-assert in case the OS toggled lines during open.
        ser.dtr = False
        ser.rts = False
    return ser


class LineReader:
    """Reads newline-terminated text lines from a serial port with an overall
    deadline. Bytes are decoded latin-1 (base64/ASCII payloads survive)."""

    def __init__(self, ser):
        self.ser = ser
        self.buf = bytearray()

    def readline(self, deadline: float):
        while True:
            nl = self.buf.find(b"\n")
            if nl >= 0:
                line = self.buf[:nl]
                del self.buf[:nl + 1]
                return line.decode("latin-1").rstrip("\r")
            if time.time() > deadline:
                return None
            chunk = self.ser.read(4096)
            if chunk:
                self.buf.extend(chunk)


def send_cmd(ser, line: str):
    ser.reset_input_buffer()
    # Leading newline flushes any half-typed prompt state on the device.
    ser.write(b"\n" + line.encode("ascii") + b"\n")
    ser.flush()


# --------------------------------------------------------------------------
# screenshot
# --------------------------------------------------------------------------

def do_screenshot(ser, outfile, fmt, quality, swap_rb, timeout):
    cmd = "screenshot"
    if fmt:
        cmd += f" --fmt {fmt}"
    if quality is not None:
        cmd += f" --quality {quality}"
    send_cmd(ser, cmd)

    reader = LineReader(ser)
    deadline = time.time() + timeout
    header = None
    chunks: dict[int, str] = {}

    while True:
        line = reader.readline(deadline)
        if line is None:
            _die("timed out waiting for screenshot frame")
        line = line.strip()
        if not line.startswith("SCR-"):
            continue  # ignore prompt / log / echo noise
        if line.startswith("SCR-ERR"):
            _die(f"device reported: {line}")
        if line.startswith("SCR-BEGIN"):
            m = HDR_RE.search(line)
            if not m:
                _die(f"malformed header: {line}")
            header = {
                "w": int(m.group(1)), "h": int(m.group(2)),
                "fmt": m.group(3), "len": int(m.group(4)),
                "crc32": int(m.group(5), 16), "chunks": int(m.group(6)),
            }
            chunks = {}
            continue
        if line.startswith("SCR-DATA"):
            if header is None:
                continue
            parts = line.split(None, 2)
            if len(parts) == 3:
                chunks[int(parts[1])] = parts[2]
            continue
        if line.startswith("SCR-END"):
            if header is not None:
                break

    n = header["chunks"]
    missing = [i for i in range(n) if i not in chunks]
    if missing:
        _die(f"incomplete frame: missing {len(missing)}/{n} chunks "
             f"(first: {missing[:5]})")
    payload = base64.b64decode("".join(chunks[i] for i in range(n)))

    if len(payload) != header["len"]:
        _die(f"length mismatch: got {len(payload)}, header said {header['len']}")
    crc = zlib.crc32(payload) & 0xFFFFFFFF
    if crc != header["crc32"]:
        print(f"warning: CRC32 mismatch (got 0x{crc:08x}, "
              f"header 0x{header['crc32']:08x}) — saving anyway",
              file=sys.stderr)

    img = decode_image(payload, header, swap_rb)
    img.save(outfile)
    print(f"saved {outfile}  ({header['w']}x{header['h']}, fmt={header['fmt']}, "
          f"{len(payload)} payload bytes)")


def decode_image(payload, header, swap_rb):
    from PIL import Image
    w, h, fmt = header["w"], header["h"], header["fmt"]

    if fmt == "jpeg":
        img = Image.open(io.BytesIO(payload)).convert("RGB")
        if swap_rb:
            r, g, b = img.split()
            img = Image.merge("RGB", (b, g, r))
        return img

    if fmt == "rgb565":
        return rgb565_to_image(payload, w, h, swap_rb)

    _die(f"unknown image format from device: {fmt}")


def rgb565_to_image(payload, w, h, swap_rb):
    from PIL import Image
    try:
        import numpy as np
        px = np.frombuffer(payload, dtype="<u2").astype(np.uint32)
        r = ((px >> 11) & 0x1F) * 255 // 31
        g = ((px >> 5) & 0x3F) * 255 // 63
        b = (px & 0x1F) * 255 // 31
        if swap_rb:
            r, b = b, r
        rgb = np.stack([r, g, b], axis=-1).astype(np.uint8).reshape((h, w, 3))
        return Image.fromarray(rgb, "RGB")
    except ImportError:
        out = bytearray(w * h * 3)
        j = 0
        for i in range(0, len(payload), 2):
            v = payload[i] | (payload[i + 1] << 8)
            r = ((v >> 11) & 0x1F) * 255 // 31
            g = ((v >> 5) & 0x3F) * 255 // 63
            b = (v & 0x1F) * 255 // 31
            if swap_rb:
                r, b = b, r
            out[j] = r; out[j + 1] = g; out[j + 2] = b
            j += 3
        return Image.frombytes("RGB", (w, h), bytes(out))


# --------------------------------------------------------------------------
# touch
# --------------------------------------------------------------------------

def do_touch(ser, line: str):
    send_cmd(ser, line)
    reader = LineReader(ser)
    deadline = time.time() + 8
    while True:
        resp = reader.readline(deadline)
        if resp is None:
            print(f"warning: no ack for '{line}' (sent anyway)", file=sys.stderr)
            return
        resp = resp.strip()
        if resp.startswith("OK") or resp.startswith("usage"):
            print(resp)
            return


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-p", "--port", default=os.environ.get("AA_PORT"),
                    help="serial port (or set $AA_PORT)")
    ap.add_argument("-b", "--baud", type=int, default=115200)
    ap.add_argument("--allow-reset", action="store_true",
                    help="open normally (lets the device reboot on connect)")
    ap.add_argument("--swap-rb", action="store_true",
                    help="swap red/blue channels (use if screenshot colors look wrong)")

    sub = ap.add_subparsers(dest="cmd", required=True)

    s = sub.add_parser("screenshot", help="capture the screen to an image file")
    s.add_argument("outfile")
    s.add_argument("--fmt", choices=["jpeg", "rgb565"], default=None,
                   help="image format (device default: jpeg)")
    s.add_argument("--quality", type=int, default=None, help="JPEG quality 1-100")
    s.add_argument("--timeout", type=float, default=120.0)

    t = sub.add_parser("tap", help="tap at x y")
    t.add_argument("x", type=int); t.add_argument("y", type=int)

    sw = sub.add_parser("swipe", help="swipe x1 y1 x2 y2 [ms]")
    for a in ("x1", "y1", "x2", "y2"):
        sw.add_argument(a, type=int)
    sw.add_argument("ms", type=int, nargs="?", default=300)

    d = sub.add_parser("touchdown", help="press at x y (held)")
    d.add_argument("x", type=int); d.add_argument("y", type=int)
    m = sub.add_parser("touchmove", help="move held press to x y")
    m.add_argument("x", type=int); m.add_argument("y", type=int)
    sub.add_parser("touchup", help="release the held press")

    args = ap.parse_args()
    if not args.port:
        _die("no port given (use -p/--port or $AA_PORT)")

    ser = open_port(args.port, args.baud, args.allow_reset)
    try:
        if args.cmd == "screenshot":
            do_screenshot(ser, args.outfile, args.fmt, args.quality,
                          args.swap_rb, args.timeout)
        elif args.cmd == "tap":
            do_touch(ser, f"tap {args.x} {args.y}")
        elif args.cmd == "swipe":
            do_touch(ser, f"swipe {args.x1} {args.y1} {args.x2} {args.y2} {args.ms}")
        elif args.cmd == "touchdown":
            do_touch(ser, f"touchdown {args.x} {args.y}")
        elif args.cmd == "touchmove":
            do_touch(ser, f"touchmove {args.x} {args.y}")
        elif args.cmd == "touchup":
            do_touch(ser, "touchup")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
