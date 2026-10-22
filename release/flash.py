#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ESP32-P4 / ESP32-S3 head unit flasher.

Sits next to the merged images release/esp32p4_android_auto-<board>-<ver>-merged.bin.
Asks which board to flash, finds the serial port, writes the image with esptool.

Run:
  Mac/Linux : double-click flash.command   (or: python3 flash.py)
  Windows   : double-click flash.bat        (or: py flash.py)

Images are self-contained (bootloader + partition table + otadata + app) and are
written in one shot at offset 0x0. esptool is installed automatically (pip) if
it is missing.
"""
import glob, os, re, shutil, subprocess, sys

BAUD = "460800"          # safe on almost any USB-serial; can be raised to 921600
HERE = os.path.dirname(os.path.abspath(__file__))

# Friendly board names keyed by the slug in the filename.
BOARDS = {
    "waveshare": "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3  (32 MB)",
    "jc4880":    "Guition JC4880P443C                      (16 MB)",
    "s3touch4":  "Waveshare ESP32-S3-Touch-LCD-4           (16 MB)",
}
CHIPS = {"waveshare": "esp32p4", "jc4880": "esp32p4", "s3touch4": "esp32s3"}


def die(msg):
    print("\n  ERROR: " + msg)
    pause()
    sys.exit(1)


def pause():
    try:
        input("\nPress Enter to exit...")
    except (EOFError, KeyboardInterrupt):
        pass


def find_firmwares():
    """[(board, version, path), ...] from *-merged.bin next to this script."""
    pat = re.compile(r"esp32p4_android_auto-(.+?)-([\d.]+)-merged\.bin$")
    out = []
    for p in sorted(glob.glob(os.path.join(HERE, "esp32p4_android_auto-*-merged.bin"))):
        m = pat.search(os.path.basename(p))
        if m:
            out.append((m.group(1), m.group(2), p))
    return out


def esptool_cmd():
    """Command prefix to launch esptool (module / binary / auto-install)."""
    try:
        import esptool  # noqa: F401
        return [sys.executable, "-m", "esptool"]
    except ImportError:
        pass
    for exe in ("esptool", "esptool.py"):
        path = shutil.which(exe)
        if path:
            return [path]
    # esptool missing -- try to install it
    print("esptool not found. Installing via pip...")
    for args in (["--user", "esptool"], ["esptool"]):
        try:
            subprocess.check_call([sys.executable, "-m", "pip", "install"] + args)
            return [sys.executable, "-m", "esptool"]
        except subprocess.CalledProcessError:
            continue
    die("could not install esptool. Install it manually:  pip3 install esptool")


def list_ports():
    try:
        from serial.tools import list_ports
        return [p.device for p in list_ports.comports()]
    except Exception:
        return []


def choose(prompt, items, render):
    """Menu. Returns the chosen item. If there is only one, picks it."""
    if len(items) == 1:
        print(f"{prompt}: {render(items[0])}")
        return items[0]
    print(prompt + ":")
    for i, it in enumerate(items, 1):
        print(f"  {i}) {render(it)}")
    while True:
        try:
            s = input("Choice [1]: ").strip() or "1"
        except (EOFError, KeyboardInterrupt):
            die("cancelled")
        if s.isdigit() and 1 <= int(s) <= len(items):
            return items[int(s) - 1]
        print("  Enter a number from the list.")


def main():
    print("=" * 60)
    print("  ESP32-P4 / ESP32-S3 head unit flasher")
    print("=" * 60)

    fws = find_firmwares()
    if not fws:
        die("no esp32p4_android_auto-<board>-<version>-merged.bin files next to me")

    # --- choose board ---
    board, ver, fw = choose(
        "\nSelect board", fws,
        lambda f: f"{BOARDS.get(f[0], f[0])}   v{f[1]}",
    )
    chip = CHIPS.get(board)
    if chip is None:
        die(f"unsupported board: {board}")
    size_mb = os.path.getsize(fw) / 1024 / 1024
    print(f"\nImage: {os.path.basename(fw)}  ({size_mb:.1f} MB)")

    # --- choose port ---
    port = None
    ports = list_ports()
    if ports:
        choice = choose("\nSelect port (or 0 = auto-detect)",
                        ["0"] + ports, lambda p: "auto-detect" if p == "0" else p)
        port = None if choice == "0" else choice
    else:
        print("\nNo ports listed -- esptool will auto-detect the port.")

    # --- flash ---
    cmd = esptool_cmd() + ["--chip", chip]
    if port:
        cmd += ["--port", port]
    cmd += ["--baud", BAUD, "write-flash", "--flash-size", "keep", "0x0", fw]

    print("\n" + "-" * 60)
    print("Running:", " ".join(cmd))
    print("-" * 60 + "\n")

    try:
        rc = subprocess.call(cmd)
    except KeyboardInterrupt:
        die("interrupted by user")

    if rc == 0:
        print("\nDONE. Board flashed and rebooting.")
    else:
        print(f"\nFAILED. esptool exited with code {rc}.")
        print("   Tips: check the cable/port; hold BOOT while plugging in;")
        print("   if it fails at speed, lower BAUD at the top of flash.py.")
    pause()


if __name__ == "__main__":
    main()
