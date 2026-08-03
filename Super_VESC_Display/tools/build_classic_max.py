#!/usr/bin/env python3
"""
Replaces the `dashboard_Amber` screen with `dashboard_Classic_Max` — a clone of
`dashboard_Classic` whose music tile is swapped for a max-power / max-speed
readout and a RESET button.

What it does
------------
1. Drops `dashboard_Amber` from both FrontJson[] and Application.screen[].
2. Clones `dashboard_Classic` (all 96 widgets, both representations, fresh
   paired ids) as `dashboard_Classic_Max`.
3. On the clone: removes `music_info` (tileview) + `song_title_label`, and adds
   into the space they freed —
       max_power_label / max_power_text   "P MAX"  "-.- KW"
       max_speed_label / max_speed_text   "V MAX"  "-- KM/H"
       max_reset_btn                      "RESET"
   sitting on the same y=370 baseline as the existing RANGE / POWER-MAX row so
   the strip reads as one line across the screen.

Events are KEPT (unlike tools/build_amber_dashboard.py, which stripped them):
the clone is a full dashboard theme, so SETTINGS / STATISTICS / the VESC menu /
the brightness slider must keep working. Widget `event` blocks are lifted out
before the id remap and put back verbatim afterwards, so the embedded `_target`
snapshot of the settings screen keeps pointing at the real settings screen id
instead of being rewritten to a dangling clone id.

Because the screen is named `dashboard_<Suffix>`, scripts/gen_dashboard_themes.py
auto-registers it as a generic theme (custom/theme_generic.c) — its widgets
already follow that naming convention since the convention was derived from
dashboard_Classic. `max_power_text` / `max_speed_text` are new convention
fields added for this screen.

Deleting the amber screen also required removing its hand-written theme module
(custom/theme_dashboard_amber.{c,h}) and its registration in custom.c — that
part is a source edit, not done here.

Idempotent: a second run drops any previous `dashboard_Classic_Max` first. Note
that once `dashboard_Amber` is gone a re-run simply finds nothing to remove.

    cd Super_VESC_Display
    python3 tools/build_classic_max.py
"""

from __future__ import annotations

import copy
import json
import os
import sys
from typing import Any

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from build_concept_dashboards import (  # noqa: E402  (same tools/ dir)
    collect_ids, label, to_app_widget,
)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT = os.path.join(ROOT, "Super_VESC_Display.guiguider")
BACKUP = PROJECT + ".classicmax.bak"

SRC = "dashboard_Classic"
NEW = "dashboard_Classic_Max"
DROP_SCREENS = {"dashboard_Amber", "dashboard_amber"}

# Music chrome the new readout replaces.
DROP_WIDGETS = {"music_info", "song_title_label"}

# Cruise-control overlays. On dashboard_Classic these are authored visible and
# hidden at runtime by the hand-written cockpit chrome (custom.c). The clone
# runs on theme_generic, which has no cruise hooks, so they would sit there as
# a stray "--" and an empty image — author them hidden instead.
HIDE_WIDGETS = {"cruise_control_img", "Speed_cc_text"}

# dashboard_Classic palette.
C_LABEL = "#8A9499"
C_VALUE = "#E8EDEE"
C_ACCENT = "#B6FF2E"
C_PANEL = "#161B1E"
C_GRID = "#1F2629"

MONO = "montserratMedium"


def gen_id(used: set[str]) -> str:
    n = 1
    while True:
        v = f"cm{n:06d}"
        if v not in used:
            used.add(v)
            return v
        n += 1


def remap_ids(node: Any, id_map: dict[str, str]) -> None:
    if isinstance(node, dict):
        v = node.get("id")
        if isinstance(v, str) and v in id_map:
            node["id"] = id_map[v]
        for vv in node.values():
            remap_ids(vv, id_map)
    elif isinstance(node, list):
        for x in node:
            remap_ids(x, id_map)


def prefix_app_names(node: Any) -> None:
    """Application-side names carry the screen prefix: dashboard_Classic_foo ->
    dashboard_Classic_Max_foo. Run while the screen root is still called
    dashboard_Classic (it does not match the 'dashboard_Classic_' prefix, so
    only the widgets are touched)."""
    if isinstance(node, dict):
        nm = node.get("name")
        if isinstance(nm, str) and nm.startswith(SRC + "_"):
            node["name"] = NEW + "_" + nm[len(SRC) + 1:]
        for vv in node.values():
            prefix_app_names(vv)
    elif isinstance(node, list):
        for x in node:
            prefix_app_names(x)


def lift_events(screen: dict, items_key: str) -> dict[int, dict]:
    """Pop every widget `event` (keyed by index) so the id remap cannot reach
    the embedded `_target` screen snapshot inside a load_screen action."""
    out: dict[int, dict] = {}
    for i, w in enumerate(screen.get(items_key, [])):
        ev = w.pop("event", None)
        if ev is not None:
            out[i] = ev
    return out


def restore_events(screen: dict, items_key: str, saved: dict[int, dict]) -> None:
    for i, ev in saved.items():
        screen[items_key][i]["event"] = ev


def button(name, left, top, width, height, text, *, tmpl: dict,
           bg, text_color, border_color, font=16, event=None) -> dict:
    """A btn cloned from the settings screen's exit_button (the project's only
    button, so its style key set is the one GUI Guider actually writes) and
    recoloured to the cockpit palette."""
    b = copy.deepcopy(tmpl)
    b.pop("id", None)
    b.pop("event", None)
    b["name"] = name
    b["left"], b["top"], b["width"], b["height"] = left, top, width, height
    b["text"] = text
    for st in b.get("style", []):
        pressed = st.get("state") == "LV_STATE_PRESSED"
        st["bg_color"] = border_color if pressed else bg
        st["bg_grad_color"] = st["bg_color"]
        st["border_width"] = 1
        st["border_color"] = border_color
        st["radius"] = 4
        if "font" in st:
            st["font"] = font
        if "text_color" in st:
            st["text_color"] = text_color
    if event:
        b["event"] = event
    return b


RESET_EVENT = {
    "widget": {
        "clicked": {
            "condition": [],
            "action": [{
                "dst": "custom_code",
                "bindCondi": "",
                "actionList": {"custom_code": {
                    "inc_c": '#include "dashboard_theme.h"',
                    "code_c": "dashboard_max_reset();",
                    "inc_py": "",
                    "code_py": "",
                }},
            }],
        }
    }
}


def max_block(btn_tmpl: dict) -> list[dict]:
    """Back-to-front. y=377 is the baseline the neighbouring RANGE (left column)
    and POWER MAX (right column) labels already sit on, so the three read as one
    strip; x 183..617 is the centre column between battery_sep and power_sep.
    The button stops at y=400 — bottom_top_sep / bottom_bg start there."""
    return [
        label("max_power_label", 183, 381, 60, 22, "P MAX",
              color=C_LABEL, font=18, family=MONO),
        label("max_power_text", 247, 377, 96, 30, "-.- KW",
              color=C_VALUE, font=22, family=MONO),
        label("max_speed_label", 351, 381, 60, 22, "V MAX",
              color=C_LABEL, font=18, family=MONO),
        label("max_speed_text", 415, 377, 104, 30, "-- KM/H",
              color=C_VALUE, font=22, family=MONO),
        button("max_reset_btn", 527, 372, 90, 28, "RESET", tmpl=btn_tmpl,
               bg=C_PANEL, text_color=C_ACCENT, border_color=C_GRID,
               font=16, event=RESET_EVENT),
    ]


def main() -> int:
    if not os.path.isfile(PROJECT):
        print(f"[err] {PROJECT} not found", file=sys.stderr)
        return 1

    with open(PROJECT, "r", encoding="utf-8") as f:
        proj = json.load(f)

    if not os.path.exists(BACKUP):
        with open(BACKUP, "w", encoding="utf-8") as f:
            json.dump(proj, f, ensure_ascii=False, indent="\t")
        print(f"[i] backup -> {BACKUP}")

    app_screens = proj["Application"]["screen"]
    front_screens = proj["FrontJson"]

    gone = {s["name"] for s in front_screens if s["name"] in DROP_SCREENS}
    dead = DROP_SCREENS | {NEW}
    app_screens[:] = [s for s in app_screens if s.get("name") not in dead]
    front_screens[:] = [s for s in front_screens if s.get("name") not in dead]
    print(f"[i] removed: {', '.join(sorted(gone)) or '(nothing)'}")

    src_app = next((s for s in app_screens if s.get("name") == SRC), None)
    src_front = next((s for s in front_screens if s.get("name") == SRC), None)
    if src_app is None or src_front is None:
        print(f"[err] source screen '{SRC}' missing", file=sys.stderr)
        return 1

    used: set[str] = set()
    collect_ids(proj, used)
    print(f"[i] {len(used)} existing ids collected")

    new_app = copy.deepcopy(src_app)
    new_front = copy.deepcopy(src_front)
    ev_app = lift_events(new_app, "widgets")
    ev_front = lift_events(new_front, "list")
    new_app["event"] = copy.deepcopy(src_app.get("event", {"widget": {}}))
    new_front["event"] = copy.deepcopy(src_front.get("event", {"widget": {}}))

    # app and front share ids, so one map keeps the two copies paired.
    old_ids: set[str] = set()
    collect_ids(new_app, old_ids)
    collect_ids(new_front, old_ids)
    id_map = {old: gen_id(used) for old in sorted(old_ids)}
    remap_ids(new_app, id_map)
    remap_ids(new_front, id_map)
    print(f"[i] remapped {len(id_map)} ids ({SRC} -> {NEW})")

    prefix_app_names(new_app)
    for s in (new_app, new_front):
        s["name"] = NEW
        if "layerName" in s:
            s["layerName"] = NEW

    restore_events(new_app, "widgets", ev_app)
    restore_events(new_front, "list", ev_front)

    # --- swap the music chrome for the max readout -------------------------
    def unprefixed(w: dict) -> str:
        n = w["name"]
        return n[len(NEW) + 1:] if n.startswith(NEW + "_") else n

    before = len(new_front["list"])
    new_app["widgets"] = [w for w in new_app["widgets"] if unprefixed(w) not in DROP_WIDGETS]
    new_front["list"] = [w for w in new_front["list"] if w["name"] not in DROP_WIDGETS]
    print(f"[i] dropped {before - len(new_front['list'])} music widgets: "
          f"{', '.join(sorted(DROP_WIDGETS))}")

    for w in new_app["widgets"] + new_front["list"]:
        if unprefixed(w) in HIDE_WIDGETS:
            flags = w.setdefault("flag", [])
            if "LV_OBJ_FLAG_HIDDEN" not in flags:
                flags.append("LV_OBJ_FLAG_HIDDEN")
    print(f"[i] hid {', '.join(sorted(HIDE_WIDGETS))}")

    btn_tmpl = next(w for s in front_screens if s["name"] == "settings"
                    for w in s["list"] if w["type"] == "btn")
    added = max_block(btn_tmpl)
    for w in added:
        w["id"] = gen_id(used)
    # Application order == creation order, so appending puts them on top;
    # FrontJson.list is the reverse, so they go in front (index 0) reversed.
    new_app["widgets"].extend(to_app_widget(w, NEW) for w in added)
    new_front["list"][:0] = [copy.deepcopy(w) for w in reversed(added)]
    print(f"[i] added {len(added)} widgets: {', '.join(w['name'] for w in added)}")

    app_screens.append(new_app)
    front_screens.append(new_front)

    with open(PROJECT, "w", encoding="utf-8") as f:
        json.dump(proj, f, ensure_ascii=False, indent="\t")

    print(f"[ok] '{NEW}': {len(new_app['widgets'])} app / {len(new_front['list'])} front widgets")
    print(f"[ok] {PROJECT}")
    print()
    print("next, in GUI Guider (close WITHOUT saving first — it caches the project):")
    print("  1. File -> Open Project -> Super_VESC_Display.guiguider")
    print(f"  2. '{NEW}' in the screen list; 'dashboard_Amber' is gone")
    print("  3. Generate Code")
    print("  4. python3 tools/regen_cockpit_fonts.py")
    return 0


if __name__ == "__main__":
    sys.exit(main())
