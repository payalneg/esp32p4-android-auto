#!/usr/bin/env python3
"""Debug the Android phone over Wi-Fi: ADB over TCP, no USB cable.

Why this exists: the phone's USB port is busy exactly when its log is most
interesting - the head unit is a USB *device* and the phone is the host while
flashing it (see docs/usb-flashing.md), and a phone on the bike/car is inside
its mount with the cable doing something else. Wireless debugging keeps
`logcat`, `shell` and `dumpsys` available through all of that.

Two ways to get there, depending on the phone:

  Android 11+ - the phone shows a pairing code
      Settings -> Developer options -> Wireless debugging -> On
      "Pair device with pairing code" -> IP:PORT + 6-digit code
      python scripts/phone_wifi.py pair 192.168.1.55:37219      # code is asked
      python scripts/phone_wifi.py discover --connect           # auto-connect
      # or connect to the "IP address & port" shown on that screen - the port
      # is random and changes every time wireless debugging is toggled:
      python scripts/phone_wifi.py connect 192.168.1.55:37223

  Older Android, or a phone already plugged in once over USB
      adb tcpip 5555            # one time, with the cable
      python scripts/phone_wifi.py connect 192.168.1.55   # 5555 assumed

Then, with the cable gone:

      python scripts/phone_wifi.py discover        # mDNS, no IP needed
      python scripts/phone_wifi.py status          # model, Android, battery temp
      python scripts/phone_wifi.py logs            # AA/devbridge lines + file
      python scripts/phone_wifi.py logs --all      # everything
      python scripts/phone_wifi.py logs --grep usbflash,gearhead

`logs` saves the whole stream to `logs/phone-<timestamp>.log` and prints only
lines matching the App-Profile/Auto pattern (same one as scripts/capture_phone.sh)
unless `--all` is given. Ctrl+C stops and keeps the file.

The phone and this machine must be on the same network - the head unit's SoftAP
(192.168.4.0/24) counts, and so does a Tailscale network: the 100.x address of
the phone works as the ADB host too, which is how this works from anywhere.

ADB is looked up in $ADB, then PATH, then the SDK this repo already carries
(build/android-sdk/platform-tools/adb[.exe]).
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import threading
import time

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOG_DIR = os.path.join(REPO, "logs")

# The tags worth reading for Android Auto / this project, plus the phone-side
# phase lines the USB flasher writes (`[usbflash]`, see usb_flasher.dart).
DEFAULT_FILTER = (r"aap|aagh|wireless.?helper|hurev|gearhead|gal|carproject|"
                  r"galserv|projection|androidauto|usbflash")


def die(msg: str) -> "NoReturn":  # type: ignore[name-defined]
    print(f"error: {msg}", file=sys.stderr)
    sys.exit(1)


def find_adb() -> str:
    for cand in (os.environ.get("ADB"), shutil.which("adb"),
                 os.path.join(REPO, "build", "android-sdk", "platform-tools",
                              "adb.exe"),
                 os.path.join(REPO, "build", "android-sdk", "platform-tools",
                              "adb")):
        if cand and os.path.exists(cand):
            return cand
    die("adb not found - set $ADB, put it on PATH, or run "
        "scripts/install_app.sh once to fetch the SDK")


def adb(args: list[str], timeout: float = 20.0) -> subprocess.CompletedProcess:
    exe = find_adb()
    return subprocess.run([exe] + args, capture_output=True, text=True,
                          timeout=timeout)


def devices() -> list[dict]:
    """`adb devices -l` as dicts: serial, state, and the extra key=value pairs."""
    out = adb(["devices", "-l"])
    if out.returncode != 0:
        die(f"adb devices failed: {out.stderr.strip()}")
    rows = []
    for line in out.stdout.splitlines()[1:]:
        line = line.strip()
        if not line:
            continue
        parts = line.split()
        row = {"serial": parts[0], "state": parts[1] if len(parts) > 1 else "?"}
        for kv in parts[2:]:
            if ":" in kv:
                k, v = kv.split(":", 1)
                row[k] = v
        row["wireless"] = (":" in row["serial"]
                           or "_adb-tls-connect" in row["serial"])
        rows.append(row)
    return rows


def pick_serial(explicit: str | None) -> str:
    if explicit:
        return explicit
    rows = [r for r in devices() if r["state"] == "device"]
    if not rows:
        die("no authorised adb device. Connect the phone over USB once, or "
            "pair wireless debugging first:\n"
            "       phone: Settings -> Developer options -> Wireless debugging\n"
            "       python scripts/phone_wifi.py pair <ip:port> --code <code>")
    wireless = [r for r in rows if r["wireless"]]
    if wireless:
        return wireless[0]["serial"]
    if len(rows) > 1:
        print(f"# several devices, using {rows[0]['serial']} "
              f"(pass --serial to choose)", file=sys.stderr)
    return rows[0]["serial"]


# --------------------------------------------------------------------------
# discovery / connection
# --------------------------------------------------------------------------

MDNS_LINE = re.compile(r"(\S+)\s+(\S+):(\d+)\s*$")


def discover(connect: bool) -> list[str]:
    """Wireless-debugging endpoints adb's mDNS daemon can see."""
    out = adb(["mdns", "services"])
    if out.returncode != 0:
        # Older adb without mdns support is not fatal - just say so.
        print(f"# adb mdns unavailable: {out.stderr.strip() or out.stdout.strip()}",
              file=sys.stderr)
        return []

    found: list[str] = []
    for line in out.stdout.splitlines()[1:]:
        low = line.lower()
        if "_adb-tls-connect._tcp" not in low and "_adb-tls-pairing._tcp" not in low:
            continue
        m = MDNS_LINE.search(line)
        if not m:
            continue
        addr = f"{m.group(2)}:{m.group(3)}"
        found.append(addr)
        kind = "pairing" if "pairing" in low else "connect"
        print(f"# {kind:7s} {addr}", file=sys.stderr)
        if connect and kind == "connect":
            res = adb(["connect", addr])
            print((res.stdout or res.stderr).strip(), file=sys.stderr)
    if not found:
        print("# nothing advertised. On the phone: Developer options -> "
              "Wireless debugging -> On (the screen lists IP:port there).",
              file=sys.stderr)
    return found


def connect(addr: str) -> None:
    if ":" not in addr:
        addr += ":5555"
    res = adb(["connect", addr])
    text = (res.stdout + res.stderr).strip()
    print(text)
    if "connected" not in text or "cannot" in text or "failed" in text:
        die("not connected - same network? wireless debugging on? "
            "for Android 11+ pair first (see --help)")


# --------------------------------------------------------------------------
# commands
# --------------------------------------------------------------------------

def do_status(serial: str) -> None:
    def prop(name: str) -> str:
        r = adb(["-s", serial, "shell", "getprop", name])
        return (r.stdout or "").strip() or "?"

    transport = ("wi-fi" if ":" in serial or "_adb-tls-connect" in serial
                 else "usb")
    print(f"device      {serial}  ({transport})")
    print(f"model       {prop('ro.product.model')}")
    print(f"android     {prop('ro.build.version.release')} "
          f"(SDK {prop('ro.build.version.sdk')})")

    bat = adb(["-s", serial, "shell", "dumpsys", "battery"]).stdout or ""
    m = re.search(r"^\s*temperature:\s*(\d+)", bat, re.M)
    level = re.search(r"^\s*level:\s*(\d+)", bat, re.M)
    status = re.search(r"^\s*status:\s*(\d+)", bat, re.M)
    if m:
        print(f"battery     {m.group(1)} (0.1  C)"
              + (f", {level.group(1)}%" if level else "")
              + (f", status {status.group(1)}" if status else ""))
    th = adb(["-s", serial, "shell", "dumpsys", "thermalservice"]).stdout or ""
    m = re.search(r"Thermal Status:\s*(\d+)", th)
    if m:
        names = {"0": "none", "1": "light", "2": "moderate", "3": "severe",
                 "4": "critical", "5": "emergency", "6": "shutdown"}
        print(f"thermal     {m.group(1)} "
              f"({names.get(m.group(1), '?')})")


def do_logs(serial: str, show_all: bool, grep: str | None, seconds: float | None,
            save: str | None) -> None:
    os.makedirs(LOG_DIR, exist_ok=True)
    path = save or os.path.join(
        LOG_DIR, time.strftime("phone-%Y%m%d-%H%M%S.log"))
    # A comma is accepted as alternation so the filter survives being typed
    # into a Windows shell: PowerShell 5.1 strips the quotes and cmd then eats
    # an unquoted `|` before the script ever sees it.
    expr = (grep or DEFAULT_FILTER).replace(",", "|")
    pattern = re.compile(expr, re.IGNORECASE)

    adb(["-s", serial, "logcat", "-c"])
    print(f"# logcat from {serial} -> {path}", file=sys.stderr)
    print("# Ctrl+C to stop" + (" (nothing else)" if show_all else
                                " (only matching lines on screen)"),
          file=sys.stderr)

    exe = find_adb()
    proc = subprocess.Popen([exe, "-s", serial, "logcat", "-v", "time"],
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    timer = None
    if seconds:
        def stop() -> None:
            print(f"\n# {seconds:.0f}s elapsed - stopping", file=sys.stderr)
            proc.terminate()
        timer = threading.Timer(seconds, stop)
        timer.daemon = True
        timer.start()

    saved = 0
    try:
        with open(path, "a", encoding="utf-8", errors="replace") as f:
            assert proc.stdout is not None
            for raw in proc.stdout:
                line = raw.decode("utf-8", "replace").rstrip("\r\n")
                f.write(line + "\n")
                saved += 1
                if show_all or pattern.search(line):
                    print(line, flush=True)
    except KeyboardInterrupt:
        pass
    finally:
        if timer:
            timer.cancel()
        try:
            proc.terminate()
        except OSError:
            pass
    print(f"# {saved} lines -> {path}", file=sys.stderr)


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------

def main() -> None:
    # Logcat lines are not ASCII; a cp1251 console would raise on the first
    # emoji instead of printing the line. Replace what cannot be encoded.
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(encoding="utf-8", errors="replace")
        except (AttributeError, ValueError):
            pass

    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--serial", "-s", default=None,
                    help="adb serial to use (default: the wireless one, else the first)")
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("pair", help="pair with Android 11+ wireless debugging")
    p.add_argument("addr", help="IP:port from the 'Pair device' dialog")
    p.add_argument("--code", default=None,
                   help="6-digit pairing code (asked for when omitted)")

    c = sub.add_parser("connect", help="connect ADB over TCP")
    c.add_argument("addr", help="IP[:port] (port defaults to 5555)")

    d = sub.add_parser("discover", help="find wireless-debugging phones via mDNS")
    d.add_argument("--connect", action="store_true", help="also connect to them")

    sub.add_parser("status", help="model, Android version, battery temperature")

    lg = sub.add_parser("logs", help="stream logcat to logs/phone-<ts>.log")
    lg.add_argument("--all", action="store_true", help="show every line on screen")
    lg.add_argument("--grep", default=None,
                    help=f"screen filter regex (default: {DEFAULT_FILTER})")
    lg.add_argument("--duration", type=float, default=None,
                    help="stop after this many seconds")
    lg.add_argument("--save", default=None, help="file to write (default: logs/...)")

    args = ap.parse_args()

    if args.cmd == "discover":
        discover(connect=args.connect)
        return
    if args.cmd == "pair":
        code = args.code
        if not code:
            # The dialog shows IP:port and a 6-digit code; the code is easier to
            # retype here than to keep the dialog in view while typing a command.
            code = input("pairing code (6 digits from the phone): ").strip()
        if not code.isdigit() or len(code) != 6:
            die("a pairing code is 6 digits")
        res = adb(["pair", args.addr, code])
        text = (res.stdout + res.stderr).strip()
        print(text)
        if "Successfully paired" not in text:
            die("pairing failed - the code expires quickly; reopen the dialog "
                "and retry, and make sure the phone is on the same network")
        return
    if args.cmd == "connect":
        connect(args.addr)
        return

    serial = pick_serial(args.serial)
    if args.cmd == "status":
        do_status(serial)
    elif args.cmd == "logs":
        do_logs(serial, args.all, args.grep, args.duration, args.save)


if __name__ == "__main__":
    main()
