#!/usr/bin/env python3
"""
Patches generated/setup_scr_dashboard.c after Generate Code in GUI Guider:

  - For every `ui->dashboard_X = lv_textarea_create(...)` line, inserts
    immediately afterwards:
        lv_obj_set_style_opa(ui->dashboard_X, LV_OPA_TRANSP, LV_PART_CURSOR);
        lv_obj_clear_flag(ui->dashboard_X,
                          LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

  This hides the blue textarea cursor and disables dragging widgets with the
  pointer (Cockpit is a static dashboard, none of these accept text input).

Idempotent: a second run does not add anything.

Triggered from the simulator Makefile (regen_fonts target). Can also be run
by hand: python3 tools/patch_cockpit_dashboard.py
"""

from __future__ import annotations

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TARGET = os.path.join(ROOT, "generated", "setup_scr_dashboard.c")

# `<indent>ui->dashboard_<name> = lv_textarea_create(...)`
RE_CREATE = re.compile(
    r"^(?P<indent>[ \t]*)ui->dashboard_(?P<name>[A-Za-z0-9_]+)"
    r"\s*=\s*lv_textarea_create\([^)]*\);\s*$"
)

# Marker used to recognise blocks we have already patched.
MARK = "/* cockpit-patch: hide cursor + lock input */"
# Older marker — kept so files patched by a previous version of this script
# are not double-patched.
LEGACY_MARKS = (
    "/* cockpit-patched: hide cursor + lock input */",
)


def patch(text: str) -> tuple[str, int]:
    out: list[str] = []
    n = 0
    skip = 0
    lines = text.splitlines(keepends=True)
    for i, line in enumerate(lines):
        out.append(line)
        if skip:
            skip -= 1
            continue
        m = RE_CREATE.match(line)
        if not m:
            continue
        # If any known marker already appears within the next ~3 lines, skip.
        lookahead = "".join(lines[i + 1: i + 4])
        if MARK in lookahead or any(lm in lookahead for lm in LEGACY_MARKS):
            continue
        indent = m.group("indent")
        name = m.group("name")
        target = f"ui->dashboard_{name}"
        out.append(f"{indent}{MARK}\n")
        out.append(f"{indent}lv_obj_set_style_opa({target}, LV_OPA_TRANSP, LV_PART_CURSOR);\n")
        out.append(f"{indent}lv_obj_clear_flag({target}, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);\n")
        n += 1
    return "".join(out), n


def main() -> int:
    if not os.path.isfile(TARGET):
        print(f"[err] missing {TARGET} — run Generate Code in GUI Guider first",
              file=sys.stderr)
        return 1
    with open(TARGET, "r", encoding="utf-8") as f:
        src = f.read()
    new_src, n_patched = patch(src)
    if new_src == src:
        print("[i] patch already applied or no textareas to patch — no changes")
        return 0
    with open(TARGET, "w", encoding="utf-8") as f:
        f.write(new_src)
    print(f"[ok] patched {n_patched} textareas on dashboard "
          f"({os.path.basename(TARGET)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
