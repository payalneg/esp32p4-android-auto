#!/usr/bin/env python3
"""Host side of the ESP32-P4 Wi-Fi debug bridge (main/debug_wifi.c).

Talks to the head unit's TCP debug port (default 3333) over its own Wi-Fi:
  * tails the log — live, preceded by the last kilo-bytes of the boot log;
  * runs any bridge command (`tasks`, `lvtimers`, `uimode`, `navstat`, ...);
  * grabs a screenshot of the live LVGL screen and saves it;
  * injects touch (tap / swipe / down-move-up).

The command language is the same one scripts/uart_debug.py speaks, so anything
you know from the serial bridge works here. The screenshot wire format differs
— over TCP the payload is raw instead of base64 lines:

    SCR-BEGIN w=800 h=480 fmt=jpeg len=12345 crc32=0x1a2b3c4d
    <len raw bytes>
    SCR-END

Logging is muted on the device for the duration of a capture, so nothing can
splice into the payload; the CRC32 is advisory.

Host and port default to the SoftAP address and port the firmware logs on boot
("dbg_wifi: debug bridge on 192.168.4.1:3333"), and can be overridden with
-H/--host and -P/--port or $AA_DEBUG_HOST / $AA_DEBUG_PORT.

Requires Pillow (`pip install pillow`) for screenshots.

Examples:
    python scripts/wifi_debug.py logs
    python scripts/wifi_debug.py logs --grep ERROR --save boot.log
    python scripts/wifi_debug.py screenshot scr.png
    python scripts/wifi_debug.py tap 400 240
    python scripts/wifi_debug.py swipe 700 240 100 240 300
    python scripts/wifi_debug.py cmd navstat
"""

from __future__ import annotations

import argparse
import io
import os
import re
import socket
import sys
import time
import zlib

DEFAULT_HOST = os.environ.get("AA_DEBUG_HOST", "192.168.4.1")
DEFAULT_PORT = int(os.environ.get("AA_DEBUG_PORT", "3333"))

HDR_RE = re.compile(
    r"SCR-BEGIN\s+w=(\d+)\s+h=(\d+)\s+fmt=(\w+)\s+len=(\d+)\s+"
    r"crc32=0x([0-9a-fA-F]+)")


def _die(msg: str) -> "NoReturn":  # type: ignore[name-defined]
    print(f"error: {msg}", file=sys.stderr)
    sys.exit(1)


class LinkLost(Exception):
    """The device went away (reboot, Wi-Fi drop, closed socket)."""


class Link:
    """Buffered reader over the debug socket: text lines and raw payloads come
    out of the same stream, so both have to share one buffer."""

    def __init__(self, host: str, port: int, connect_timeout: float = 5.0):
        try:
            self.sock = socket.create_connection((host, port), timeout=connect_timeout)
        except OSError as e:
            raise LinkLost(f"cannot connect to {host}:{port}: {e}") from e
        self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.buf = bytearray()

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass

    def _fill(self, deadline: float) -> bool:
        while True:
            remain = deadline - time.time()
            if remain <= 0:
                return False
            self.sock.settimeout(min(remain, 1.0))
            try:
                chunk = self.sock.recv(4096)
            except socket.timeout:
                continue
            except OSError as e:
                raise LinkLost(f"connection lost: {e}") from e
            if not chunk:
                raise LinkLost("device closed the connection")
            self.buf.extend(chunk)
            return True

    def readline(self, deadline: float) -> bytes | None:
        while True:
            nl = self.buf.find(b"\n")
            if nl >= 0:
                line = bytes(self.buf[:nl])
                del self.buf[:nl + 1]
                return line
            if not self._fill(deadline):
                return None

    def read_exact(self, n: int, deadline: float) -> bytes | None:
        while len(self.buf) < n:
            if not self._fill(deadline):
                return None
        out = bytes(self.buf[:n])
        del self.buf[:n]
        return out

    def send(self, line: str):
        self.sock.sendall(line.encode("ascii") + b"\n")


# --------------------------------------------------------------------------
# log tail
# --------------------------------------------------------------------------

def do_logs(host: str, port: int, grep: str | None, save: str | None,
            quiet: bool, reconnect: bool = True):
    pattern = re.compile(grep, re.IGNORECASE) if grep else None
    out = open(save, "ab") if save else None
    text = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace",
                            newline="")
    link = Link(host, port)
    print("# connected — tailing the device log (Ctrl+C to stop)", file=sys.stderr)
    try:
        while True:
            try:
                line = link.readline(time.time() + 5.0)
            except LinkLost as e:
                # A USB flash (or any reset) reboots the board, and the Wi-Fi
                # link dies with it — keep watching across that instead of
                # making the operator reconnect by hand.
                print(f"# {e}", file=sys.stderr)
                link.close()
                if not reconnect:
                    return
                while True:
                    time.sleep(1.0)
                    try:
                        link = Link(host, port)
                        print("# reconnected", file=sys.stderr)
                        break
                    except LinkLost:
                        continue
                continue
            if line is None:
                # Nothing for 5 s: the device is simply quiet. Keep waiting —
                # a crash or a hang would otherwise look like "log stopped".
                continue
            if out:
                out.write(line + b"\n")
                out.flush()
            if quiet:
                continue
            if pattern and not pattern.search(line.decode("utf-8", "replace")):
                continue
            text.write(line.decode("utf-8", "replace") + "\n")
            text.flush()
    except KeyboardInterrupt:
        pass
    finally:
        text.flush()
        if out:
            out.close()
        link.close()


# --------------------------------------------------------------------------
# commands
# --------------------------------------------------------------------------

def do_cmd(link: Link, line: str, timeout: float, echo: bool = True) -> str:
    """Run one command; print (and return) its output up to the device's
    end-of-command marker. Log lines that arrive meanwhile are shown too, as
    they are on the serial console."""
    link.send(line)
    deadline = time.time() + timeout
    idle = time.time()
    chunks: list[str] = []
    while True:
        resp = link.readline(min(idle + 2.0, deadline))
        if resp is None:
            if time.time() >= deadline or time.time() - idle >= 2.0:
                print(f"warning: no completion marker for '{line}' "
                      f"(sent anyway)", file=sys.stderr)
                break
            continue
        text = resp.decode("utf-8", "replace").rstrip("\r")
        if text == "# done":
            break
        if text.startswith("# err"):
            print(f"warning: device said: {text}", file=sys.stderr)
            break
        if text.startswith("# "):
            continue        # banner / bridge chatter
        idle = time.time()
        chunks.append(text)
        if echo:
            print(text)
    return "\n".join(chunks)


# --------------------------------------------------------------------------
# screenshot
# --------------------------------------------------------------------------

def do_screenshot(link: Link, outfile: str, fmt: str | None, quality: int | None,
                  swap_rb: bool, timeout: float):
    cmd = "screenshot"
    if fmt:
        cmd += f" --fmt {fmt}"
    if quality is not None:
        cmd += f" --quality {quality}"
    link.send(cmd)

    deadline = time.time() + timeout
    header = None
    while True:
        line = link.readline(deadline)
        if line is None:
            _die("timed out waiting for the screenshot frame")
        text = line.decode("latin-1").strip()
        if text.startswith("SCR-ERR"):
            _die(f"device reported: {text}")
        if text.startswith("SCR-BEGIN"):
            m = HDR_RE.search(text)
            if not m:
                _die(f"malformed header: {text}")
            header = {
                "w": int(m.group(1)), "h": int(m.group(2)), "fmt": m.group(3),
                "len": int(m.group(4)), "crc32": int(m.group(5), 16),
            }
            break

    payload = link.read_exact(header["len"], deadline)
    if payload is None:
        _die(f"timed out inside the frame ({header['len']} bytes expected)")

    # The device terminates with "\nSCR-END\n"; swallow up to the marker.
    for _ in range(3):
        line = link.readline(deadline)
        if line is None:
            _die("timed out waiting for SCR-END")
        if line.strip() == b"SCR-END":
            break

    crc = zlib.crc32(payload) & 0xFFFFFFFF
    if crc != header["crc32"]:
        print(f"warning: CRC32 mismatch (got 0x{crc:08x}, "
              f"header 0x{header['crc32']:08x}) — saving anyway", file=sys.stderr)

    img = decode_image(payload, header, swap_rb)
    img.save(outfile)
    print(f"saved {outfile}  ({header['w']}x{header['h']}, fmt={header['fmt']}, "
          f"{len(payload)} payload bytes)")


def decode_image(payload: bytes, header: dict, swap_rb: bool):
    from PIL import Image

    w, h, fmt = header["w"], header["h"], header["fmt"]
    if fmt == "jpeg":
        img = Image.open(io.BytesIO(payload)).convert("RGB")
        if swap_rb:
            r, g, b = img.split()
            img = Image.merge("RGB", (b, g, r))
        return img

    if fmt == "rgb565":
        out = bytearray(w * h * 3)
        j = 0
        for i in range(0, len(payload) - 1, 2):
            v = payload[i] | (payload[i + 1] << 8)
            r = ((v >> 11) & 0x1F) * 255 // 31
            g = ((v >> 5) & 0x3F) * 255 // 63
            b = (v & 0x1F) * 255 // 31
            if swap_rb:
                r, b = b, r
            out[j] = r
            out[j + 1] = g
            out[j + 2] = b
            j += 3
        return Image.frombytes("RGB", (w, h), bytes(out))

    _die(f"unknown image format from device: {fmt}")


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-H", "--host", default=DEFAULT_HOST,
                    help=f"device address (default {DEFAULT_HOST}, $AA_DEBUG_HOST)")
    ap.add_argument("-P", "--port", type=int, default=DEFAULT_PORT,
                    help=f"debug port (default {DEFAULT_PORT}, $AA_DEBUG_PORT)")
    ap.add_argument("--swap-rb", action="store_true",
                    help="swap red/blue channels in screenshots")

    sub = ap.add_subparsers(dest="cmd", required=True)

    lg = sub.add_parser("logs", help="tail the device log")
    lg.add_argument("--grep", default=None, help="only show lines matching this regex")
    lg.add_argument("--save", default=None, help="append the raw stream to this file")
    lg.add_argument("-q", "--quiet", action="store_true",
                    help="do not print (useful with --save)")

    s = sub.add_parser("screenshot", help="capture the screen to an image file")
    s.add_argument("outfile")
    s.add_argument("--fmt", choices=["jpeg", "rgb565"], default=None,
                   help="image format (device default: jpeg)")
    s.add_argument("--quality", type=int, default=None, help="JPEG quality 1-100")
    s.add_argument("--timeout", type=float, default=30.0)

    t = sub.add_parser("tap", help="tap at x y")
    t.add_argument("x", type=int)
    t.add_argument("y", type=int)

    sw = sub.add_parser("swipe", help="swipe x1 y1 x2 y2 [ms]")
    for a in ("x1", "y1", "x2", "y2"):
        sw.add_argument(a, type=int)
    sw.add_argument("ms", type=int, nargs="?", default=300)

    d = sub.add_parser("touchdown", help="press at x y (held)")
    d.add_argument("x", type=int)
    d.add_argument("y", type=int)
    m = sub.add_parser("touchmove", help="move held press to x y")
    m.add_argument("x", type=int)
    m.add_argument("y", type=int)
    sub.add_parser("touchup", help="release the held press")

    c = sub.add_parser("cmd", help="run a bridge command and print its output "
                                   "(e.g. 'tasks', 'navstat')")
    c.add_argument("line", nargs="+", help="command and its arguments")
    c.add_argument("--timeout", type=float, default=15.0)

    args = ap.parse_args()

    if args.cmd == "logs":
        try:
            do_logs(args.host, args.port, args.grep, args.save, args.quiet)
        except LinkLost as e:
            _die(f"{e}\n"
                 "       (is the unit on the same Wi-Fi, and the firmware built "
                 "with CONFIG_DEBUG_WIFI_BRIDGE=y?)")
        return

    try:
        link = Link(args.host, args.port)
    except LinkLost as e:
        _die(f"{e}\n"
             "       (is the unit on the same Wi-Fi, and the firmware built "
             "with CONFIG_DEBUG_WIFI_BRIDGE=y?)")

    try:
        if args.cmd == "screenshot":
            do_screenshot(link, args.outfile, args.fmt, args.quality,
                          args.swap_rb, args.timeout)
        elif args.cmd == "tap":
            do_cmd(link, f"tap {args.x} {args.y}", 10.0, echo=False)
        elif args.cmd == "swipe":
            do_cmd(link, f"swipe {args.x1} {args.y1} {args.x2} {args.y2} "
                         f"{args.ms}", 10.0, echo=False)
        elif args.cmd == "touchdown":
            do_cmd(link, f"touchdown {args.x} {args.y}", 10.0, echo=False)
        elif args.cmd == "touchmove":
            do_cmd(link, f"touchmove {args.x} {args.y}", 10.0, echo=False)
        elif args.cmd == "touchup":
            do_cmd(link, "touchup", 10.0, echo=False)
        elif args.cmd == "cmd":
            do_cmd(link, " ".join(args.line), args.timeout)
    except LinkLost as e:
        _die(str(e))
    finally:
        link.close()


if __name__ == "__main__":
    main()
