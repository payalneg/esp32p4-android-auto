#!/usr/bin/env python3
"""
Regenerates the large Antonio_Regular fonts in generated/guider_fonts/,
keeping only the glyphs Cockpit actually needs. GUI Guider exports the full
ASCII range (95 glyphs) + 60 icon symbols on every size, which inflates the
big speed font Antonio@200 to ~1.4 MB of flash (and at @220 the bitmap_index
field overflows the LVGL 12-bit bitfield, breaking the build). The dashboard
labels only ever render digits / a few letters, so we subset hard.

The ESP-IDF firmware build runs this automatically (components/vesc_ui/
CMakeLists.txt): it regenerates into the build dir via --out-dir and excludes
the GUI Guider Antonio copies from compilation, so a GUI Guider "Generate Code"
can never reintroduce the bloated fonts into the firmware. With --fallback-from
the build copies the committed subsets when node/npx is absent (CI / clean box).

For the desktop simulator, run it manually after Generate Code (it overwrites
the in-tree generated/guider_fonts/ copies the simulator compiles):
    cd Super_VESC_Display
    python3 tools/regen_cockpit_fonts.py
    cd lvgl-simulator && make

Usage:
    python3 tools/regen_cockpit_fonts.py [--out-dir DIR] [--fallback-from DIR]

Requirements:
    - node/npm (`npx lv_font_conv` is fetched on demand) — except in the
      --fallback-from path, which needs only the committed .c files
    - import/font/Antonio-Regular.ttf
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TTF = os.path.join(ROOT, "import", "font", "Antonio-Regular.ttf")
# DSEG7 Classic — real 7-segment font used by the amber dashboard theme. Only
# the digits/dot/colon/space are kept; the full GUI Guider export is enormous
# (DSEG7_200 alone = 9.7 MB) and overflows the OTA slot.
DSEG_TTF = os.path.join(ROOT, "import", "font", "DSEG7Classic-Regular.ttf")
# DSEG14 Classic — 14-segment alphanumeric ("starburst"). The amber dashboard
# uses it for the WHOLE screen (labels + numbers), so unlike DSEG7 it needs
# letters. Small label sizes get the full charset; the big numeric-only sizes
# (50/64/200) keep the cheap digit subset.
DSEG14_TTF = os.path.join(ROOT, "import", "font", "DSEG14Classic-Regular.ttf")
OUT_DIR = os.path.join(ROOT, "generated", "guider_fonts")

# (file_basename_without_.c, size_px, lv_font_conv_range_spec, ttf)
#
# Char sets are derived from the actual label formatters in
# custom/custom.c (dashboard) and main/aa_overlay.c (AA HUD overlay).
# Keep this list in sync with the sizes referenced by setup_scr_dashboard.c /
# gui_guider.h / aa_overlay.c — a size missing here ships the full-ASCII
# GUI Guider export instead (e.g. Antonio_200 = 1.4 MB).
# DSEG7 only ever renders digits, decimal dot, colon (clock) and space; minus
# kept as one cheap glyph. Same spec at every size the amber theme uses.
_DSEG_RANGE = "0x20,0x2D,0x2E,0x30-0x3A"
# DSEG14 (amber, whole screen). Letter sizes need A-Z + the few lowercase the
# units use (h,k,m for Ah/kW/...), digits, minus, dot, slash, colon, %, degree.
# NOTE: DSEG14 has no middle-dot (U+00B7); the converter rewrites "·" -> "-".
_DSEG14_FULL = "0x20,0x25,0x2D-0x3A,0x41-0x5A,0x68,0x6B,0x6D,0xB0"
# Numeric-only big readouts (speed/battery%/power/cruise) — digits, minus, dot.
_DSEG14_DIG  = "0x20,0x2D,0x2E,0x30-0x3A"

# Fonts at or above this size ship RLE-compressed (lv_font_conv default), the
# rest stay raw (--no-compress). Compression is lossless (bpp kept at 4) and only
# worthwhile on the big glyphs: DSEG7@200 115->15 KB, Antonio@200 57->13 KB; the
# whole cockpit set drops ~160 KB of flash. LVGL decodes a glyph once per label
# redraw (invalidation-based, not per frame), so the ~30% slower decode is cheap
# on the big speedo digits but pointless on the small, frequently-updated labels
# (trip/odo/voltage/units) — hence the threshold. Requires LV_USE_FONT_COMPRESSED=1
# in main/lv_conf.h + Super_VESC_Display/lv_conf.h (set), else compressed glyphs
# render blank.
COMPRESS_MIN_PX = 64

JOBS = [
    # dashboard_Speed_text — snprintf "%02d", clamped 0..999. Digits only;
    # keep minus too since it is one cheap glyph.
    ("lv_font_Antonio_Regular_200", 200, "0x2D,0x30-0x39", TTF),
    # dashboard_Speed_cc_text (cruise target) — "%d" (may be negative) + "--".
    ("lv_font_Antonio_Regular_50",   50, "0x2D,0x30-0x39", TTF),
    # TRIP "%0.1f" / odo "%05d" / temp "%d" (negative) / voltage "%.1f"
    # and the "--.-" / "----" placeholders — digits, dot, minus.
    ("lv_font_Antonio_Regular_40",   40, "0x2D,0x2E,0x30-0x39", TTF),
    # AA overlay unit label "KM/H" / "MPH" / "%" + dashboard "ESC NOT CONNECTED".
    # Be generous (A-Z and common symbols) — cheap at 22 px, future-proof.
    ("lv_font_Antonio_Regular_22",   22, "0x20,0x25,0x2D-0x3A,0x41-0x5A", TTF),
    # AA overlay big digits (speed / battery "%u") — digits, minus, %, ., V, W,
    # k, °, space. Power = U*I/1000 can be negative under regen.
    ("lv_font_Antonio_Regular_64",   64, "0x20,0x25,0x2D,0x2E,0x30-0x39,0x56,0x57,0x6B,0xB0", TTF),
    # bottom-strip values — digits, minus, ., /, A-Z, °, ·, space.
    ("lv_font_Antonio_Regular_32",   32, "0x20,0x2D-0x39,0x41-0x5A,0xB0,0xB7", TTF),

    # ── DSEG7 Classic 7-segment subset for the amber dashboard theme ──
    # Sizes used by setup_scr_dashboard_amber / theme_dashboard_amber.c.
    ("lv_font_DSEG7Classic_Regular_200", 200, _DSEG_RANGE, DSEG_TTF),
    ("lv_font_DSEG7Classic_Regular_64",   64, _DSEG_RANGE, DSEG_TTF),
    ("lv_font_DSEG7Classic_Regular_40",   40, _DSEG_RANGE, DSEG_TTF),
    ("lv_font_DSEG7Classic_Regular_32",   32, _DSEG_RANGE, DSEG_TTF),
    ("lv_font_DSEG7Classic_Regular_24",   24, _DSEG_RANGE, DSEG_TTF),

    # ── DSEG14 Classic 14-segment, the amber dashboard's ONE font ──
    # Letter sizes (labels + units) — full charset.
    ("lv_font_DSEG14Classic_Regular_11",  11, _DSEG14_FULL, DSEG14_TTF),
    ("lv_font_DSEG14Classic_Regular_12",  12, _DSEG14_FULL, DSEG14_TTF),
    ("lv_font_DSEG14Classic_Regular_16",  16, _DSEG14_FULL, DSEG14_TTF),
    ("lv_font_DSEG14Classic_Regular_18",  18, _DSEG14_FULL, DSEG14_TTF),
    ("lv_font_DSEG14Classic_Regular_20",  20, _DSEG14_FULL, DSEG14_TTF),
    ("lv_font_DSEG14Classic_Regular_22",  22, _DSEG14_FULL, DSEG14_TTF),
    ("lv_font_DSEG14Classic_Regular_24",  24, _DSEG14_FULL, DSEG14_TTF),
    ("lv_font_DSEG14Classic_Regular_25",  25, _DSEG14_FULL, DSEG14_TTF),
    ("lv_font_DSEG14Classic_Regular_26",  26, _DSEG14_FULL, DSEG14_TTF),
    # Numeric-only readouts (speed / cruise / battery% / power) — digit subset.
    ("lv_font_DSEG14Classic_Regular_30",  30, _DSEG14_DIG, DSEG14_TTF),
    ("lv_font_DSEG14Classic_Regular_50",  50, _DSEG14_DIG, DSEG14_TTF),
    ("lv_font_DSEG14Classic_Regular_64",  64, _DSEG14_DIG, DSEG14_TTF),
    ("lv_font_DSEG14Classic_Regular_160", 160, _DSEG14_DIG, DSEG14_TTF),
    ("lv_font_DSEG14Classic_Regular_180", 180, _DSEG14_DIG, DSEG14_TTF),
    ("lv_font_DSEG14Classic_Regular_200", 200, _DSEG14_DIG, DSEG14_TTF),
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


def run_one(name: str, size: int, ranges: str, ttf: str, out_dir: str) -> bool:
    out_path = os.path.join(out_dir, f"{name}.c")
    compress = size >= COMPRESS_MIN_PX
    cmd = [
        NPX, "--yes", "lv_font_conv",
        "--font", ttf,
        "--size", str(size),
        "--bpp", "4",
        # big fonts: RLE-compressed (lv_font_conv default); small ones: raw
        *([] if compress else ["--no-compress"]),
        "--format", "lvgl",
        "--lv-include", "lvgl.h",
        "--range", ranges,
        "-o", out_path,
    ]
    print(f"[i] generating {name} (size={size}, range={ranges}, "
          f"compress={'yes' if compress else 'no'})")
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


def copy_fallback(fallback_dir: str, out_dir: str) -> bool:
    """Copy the committed subset .c files instead of regenerating. Used by the
    ESP-IDF build when node/npx is unavailable (clean machine / CI) so the
    firmware still links — the committed files are already subsets."""
    missing = [n for n, *_ in JOBS
               if not os.path.isfile(os.path.join(fallback_dir, f"{n}.c"))]
    if missing:
        print(f"[err] fallback {fallback_dir} is missing: "
              f"{', '.join(n + '.c' for n in missing)}", file=sys.stderr)
        return False
    for name, *_ in JOBS:
        src = os.path.join(fallback_dir, f"{name}.c")
        dst = os.path.join(out_dir, f"{name}.c")
        if os.path.abspath(src) != os.path.abspath(dst):
            shutil.copyfile(src, dst)
    print(f"[warn] node/npx unavailable — copied committed subsets "
          f"from {fallback_dir} (NOT regenerated)", file=sys.stderr)
    return True


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out-dir", default=OUT_DIR,
                    help="where to write the .c files (default: generated/guider_fonts)")
    ap.add_argument("--fallback-from", default=None,
                    help="if node/npx is missing, copy committed subsets from this "
                         "dir instead of failing (used by the ESP-IDF build)")
    args = ap.parse_args()

    out_dir = os.path.abspath(args.out_dir)
    os.makedirs(out_dir, exist_ok=True)

    for _ttf in {j[3] for j in JOBS}:
        if not os.path.isfile(_ttf):
            print(f"[err] missing {_ttf}", file=sys.stderr)
            return 1

    # No toolchain → fall back to the committed subsets if the caller allows it,
    # otherwise fail loudly (manual invocation expects a real regeneration).
    if NPX is None:
        if args.fallback_from:
            return 0 if copy_fallback(args.fallback_from, out_dir) else 1
        print("[err] npx not found. Install Node.js (brew install node).", file=sys.stderr)
        return 1

    ok = True
    for name, size, rng, ttf in JOBS:
        ok = run_one(name, size, rng, ttf, out_dir) and ok
    if not ok:
        # A real tooling error. In a build (fallback-from set) keep going with the
        # committed subsets so the firmware still links; manual runs fail.
        if args.fallback_from:
            print("[warn] regeneration failed — falling back to committed subsets",
                  file=sys.stderr)
            return 0 if copy_fallback(args.fallback_from, out_dir) else 1
        return 1
    print()
    print(f"[ok] all fonts regenerated -> {out_dir}")
    if os.path.abspath(out_dir) == os.path.abspath(OUT_DIR):
        print("     simulator: cd lvgl-simulator && make")
    return 0


if __name__ == "__main__":
    sys.exit(main())
