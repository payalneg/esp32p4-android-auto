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
import glob, os, re, shutil, struct, subprocess, sys

BAUD = "115200"          # conservative default for recovery over USB-UART
HERE = os.path.dirname(os.path.abspath(__file__))

# Friendly board names keyed by the slug in the filename.
BOARDS = {
    "waveshare": "Waveshare ESP32-P4 4.3  rev1.x (32 MB)",
    "waveshare-rev3": "Waveshare ESP32-P4 4.3  rev3.x / v3.2 (32 MB)",
    "jc4880":    "Guition JC4880P443C                      (16 MB)",
    "s3touch4":  "Waveshare ESP32-S3-Touch-LCD-4           (16 MB)",
}
CHIPS = {"waveshare": "esp32p4", "waveshare-rev3": "esp32p4",
         "jc4880": "esp32p4", "s3touch4": "esp32s3"}


def require_modern_esptool(cmd):
    """Require the P4 v3.2 fix in stable 4.12+ or 5.3+; return major version."""
    result = subprocess.run(cmd + ["version"], capture_output=True, text=True)
    version = result.stdout.strip().splitlines()[-1] if result.stdout.strip() else ""
    match = re.fullmatch(r"v?(\d+)\.(\d+)(?:\.(\d+))?", version)
    numbers = tuple(map(int, match.groups(default="0"))) if match else (0, 0, 0)
    supported = (numbers[0] == 4 and numbers >= (4, 12, 0)) or numbers >= (5, 3, 0)
    if result.returncode or not match or not supported:
        die("stable esptool 4.12+ or 5.3+ required for ESP32-P4 v3.2. "
            f"Detected: {version or 'unknown'}. In a standalone Python environment run: "
            'python -m pip install --upgrade "esptool>=5.3"')
    return numbers[0]


def p4_image_revision_range(path):
    """Read the merged bootloader's extended revision range, at offset 0x2000."""
    with open(path, "rb") as image:
        image.seek(0x2000)
        header = image.read(24)
    if len(header) != 24 or header[0] != 0xE9 or struct.unpack_from("<H", header, 12)[0] != 0x12:
        raise ValueError("not a merged ESP32-P4 image with bootloader at 0x2000")
    return struct.unpack_from("<HH", header, 15)


def check_p4_revision(cmd, fw, tool_major=5):
    """Read silicon revision before write-flash can erase anything. Fail closed."""
    minimum, maximum = p4_image_revision_range(fw)
    result = subprocess.run(cmd + ["chip_id" if tool_major == 4 else "chip-id"],
                            capture_output=True, text=True)
    print(result.stdout)
    match = re.search(r"revision v(\d+)\.(\d+)", result.stdout)
    if result.returncode or not match:
        die("could not read P4 chip revision. " + result.stderr.strip())
    major, minor = map(int, match.groups())
    revision = major * 100 + minor
    if revision < minimum or (maximum not in (0, 65535) and revision > maximum):
        die(f"chip v{major}.{minor} is incompatible with this image "
            f"(v{minimum // 100}.{minimum % 100:02d}.."
            f"v{maximum // 100}.{maximum % 100:02d}). "
            "Select an image for both your board and silicon revision. "
            "For Waveshare v3.x use waveshare-rev3. Do not use --force.")


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
    for args in (["--user", "esptool>=5.3"], ["esptool>=5.3"]):
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

    # --- validate tools and chip before any erase/write ---
    prefix = esptool_cmd()
    tool_major = require_modern_esptool(prefix)
    cmd = prefix + ["--chip", chip]
    if port:
        cmd += ["--port", port]
    cmd += ["--baud", BAUD]
    if chip == "esp32p4":
        try:
            check_p4_revision(cmd, fw, tool_major)
        except (OSError, ValueError) as exc:
            die(str(exc))
    cmd += (["write_flash", "--flash_size"] if tool_major == 4 else
            ["write-flash", "--flash-size"]) + ["keep", "0x0", fw]

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
