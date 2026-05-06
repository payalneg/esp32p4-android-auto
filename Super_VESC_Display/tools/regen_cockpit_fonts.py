#!/usr/bin/env python3
"""
Regenerates the large Antonio_Regular fonts in generated/guider_fonts/,
keeping only the glyphs Cockpit actually needs. GUI Guider exports the full
ASCII range by default, which inflates Antonio@220 to ~10 MB and the
bitmap_index field overflows the LVGL bitfield, breaking the build.

Run AFTER Generate Code in GUI Guider, before building the simulator:
    cd Super_VESC_Display
    python3 tools/regen_cockpit_fonts.py

Requirements:
    - node/npm (`npx lv_font_conv` is fetched on demand)
    - import/font/Antonio-Regular.ttf
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TTF = os.path.join(ROOT, "import", "font", "Antonio-Regular.ttf")
OUT_DIR = os.path.join(ROOT, "generated", "guider_fonts")

# (file_basename_without_.c, size_px, lv_font_conv_range_spec)
JOBS = [
    # speed_value — digits and minus (in case of negative speed / reverse).
    ("lv_font_Antonio_Regular_220", 220, "0x2D,0x30-0x39"),
    # battery_value / power_value — digits, minus, %, ., V, W, k, °, space.
    # Power = U*I/1000 can be negative under regen.
    ("lv_font_Antonio_Regular_64",   64, "0x20,0x25,0x2D,0x2E,0x30-0x39,0x56,0x57,0x6B,0xB0"),
    # bottom-strip values — digits, minus, ., /, A-Z, °, ·, space.
    ("lv_font_Antonio_Regular_32",   32, "0x20,0x2D-0x39,0x41-0x5A,0xB0,0xB7"),
]

def find_npx() -> str | None:
    """GUI Guider invokes make with a stripped PATH (no homebrew), so
    shutil.which('npx') returns None even if node is installed. We probe
    standard install locations manually."""
    if (p := shutil.which("npx")):
        return p
    candidates = [
        "/opt/homebrew/bin/npx",          # macOS arm64 Homebrew
        "/usr/local/bin/npx",             # macOS x86_64 Homebrew / others
        "/opt/local/bin/npx",             # MacPorts
        "/usr/bin/npx",                   # Linux distros
        os.path.expanduser("~/.nvm/versions/node/current/bin/npx"),
    ]
    # nvm: pick any installed Node version
    nvm_root = os.path.expanduser("~/.nvm/versions/node")
    if os.path.isdir(nvm_root):
        for v in sorted(os.listdir(nvm_root), reverse=True):
            candidates.append(os.path.join(nvm_root, v, "bin", "npx"))
    for c in candidates:
        if os.path.isfile(c) and os.access(c, os.X_OK):
            return c
    return None


NPX = find_npx()


def run_one(name: str, size: int, ranges: str) -> bool:
    out_path = os.path.join(OUT_DIR, f"{name}.c")
    cmd = [
        NPX, "--yes", "lv_font_conv",
        "--font", TTF,
        "--size", str(size),
        "--bpp", "4",
        "--no-compress",
        "--format", "lvgl",
        "--lv-include", "lvgl.h",
        "--range", ranges,
        "-o", out_path,
    ]
    print(f"[i] generating {name} (size={size}, range={ranges})")
    print("   $", " ".join(cmd))
    # npx spawns child node — that one also needs to be on PATH (homebrew/usr/local).
    env = os.environ.copy()
    npx_dir = os.path.dirname(NPX)
    if npx_dir not in env.get("PATH", "").split(":"):
        env["PATH"] = npx_dir + ":" + env.get("PATH", "")
    res = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, env=env)
    if res.returncode != 0:
        sys.stderr.write(res.stdout)
        sys.stderr.write(res.stderr)
        print(f"[err] {name} failed (exit {res.returncode})", file=sys.stderr)
        return False
    sz = os.path.getsize(out_path)
    print(f"[ok] {name} -> {sz/1024:.1f} KB")
    return True


def main() -> int:
    if NPX is None:
        print("[err] npx not found. Install Node.js (brew install node).", file=sys.stderr)
        return 1
    if not os.path.isfile(TTF):
        print(f"[err] missing {TTF}", file=sys.stderr)
        return 1
    if not os.path.isdir(OUT_DIR):
        print(f"[err] missing {OUT_DIR}. Run Generate Code in GUI Guider first.",
              file=sys.stderr)
        return 1

    ok = True
    for name, size, rng in JOBS:
        ok = run_one(name, size, rng) and ok
    if ok:
        print()
        print("[ok] all fonts regenerated — simulator can now be built")
        print("     cd lvgl-simulator && make")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
