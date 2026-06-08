#!/usr/bin/env python3
"""
Adds a second dashboard screen — `dashboard_amber` — to
Super_VESC_Display.guiguider, visible alongside `dashboard` / `settings` in
GUI Guider's screen tree.

It is the "Cyberpunk Amber" reskin described in
`design_handoff_vesc_dashboard/` (Variant A — Classic Reskin): the exact same
information layout as the existing green `dashboard`, recoloured to the amber
palette from the handoff and frozen with the handoff's sample telemetry so the
screen reads as a finished mock-up in the editor.

How it works
------------
GUI Guider keeps two parallel representations of every screen:
  * FrontJson[]            — the editor-canvas model (`list`, left/top/width/
                             height, un-prefixed widget names). Authoritative
                             for what the editor renders.
  * Application.screen[]   — the code-gen model (`widgets`, pos/size, names
                             prefixed with the screen name).
The two share the SAME widget `id` per widget. We clone the `dashboard` screen
in BOTH, give every cloned widget a fresh globally-unique id (the same new id
in both copies so they stay paired), recolour, bake in sample values, and
append the new screen to both arrays.

Cloning (instead of hand-building) reuses the proven 96-widget layout — fonts,
sizes, z-order and the two-entry label `style` arrays GUI Guider needs to
render — so the new screen is guaranteed to open correctly.

Events are stripped from the clone (it is a static preview, so no navigation /
custom-code wiring) which also drops the embedded `_target` screen copy that
would otherwise tangle id remapping.

Idempotent: a second run removes any previous `dashboard_amber` first.

    cd Super_VESC_Display
    python3 tools/build_amber_dashboard.py
"""

from __future__ import annotations

import copy
import json
import os
import shutil
import string
import sys
from typing import Any

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT = os.path.join(ROOT, "Super_VESC_Display.guiguider")
BACKUP = PROJECT + ".amber.bak"

SRC_SCREEN = "dashboard"
NEW_SCREEN = "dashboard_amber"

# ---------------------------------------------------------------------------
# Amber palette — old (green) colour -> handoff amber colour.
# Values from design_handoff_vesc_dashboard/styles.css, blended over the
# near-black background where the source used translucency.
# ---------------------------------------------------------------------------
COLOR_MAP = {
    "#B6FF2E": "#FFB35C",  # green accent / lit digit   -> --amber-bright
    "#E8EDEE": "#FFE6C2",  # near-white hero value       -> --amber-hot
    "#8A9499": "#B0814F",  # label grey                  -> --label (amber)
    "#4A5358": "#6B4A2C",  # faint label                 -> --label-dim
    "#E70023": "#FF3B2F",  # red warning                 -> --red
    "#161B1E": "#241405",  # segment OFF cell            -> --amber-dim
    "#1F2629": "#2A1A0C",  # dividers / panels           -> --grid-line
    "#0D1113": "#0A0806",  # very-dark panel             -> --bg
    "#07090A": "#0A0806",  # screen background           -> --bg
}
COLOR_KEYS = ("text_color", "bg_color", "bg_grad_color", "border_color")

# Segment-bar "lit" cell — amber gradient (deep -> bright), horizontal.
SEG_ON = {
    "bg_color": "#C8470A",       # --amber-deep
    "bg_grad_color": "#FF7A1A",  # --amber
    "bg_grad_dir": "LV_GRAD_DIR_HOR",
    "bg_opa": 255,
}

# Handoff sample telemetry, baked in as static text (un-prefixed widget name).
VALUES = {
    "uptime_text":       "0:48:12",
    "cur_time_label":    "14:32",
    "Battery_proc_text": "73",
    "Ah_text":           "6.4 Ah",
    "Range_text":        "38 KM",
    "Speed_text":        "42",
    "power_value":       "2.8",
    "Current_text":      "24.5 A",
    "power_max_val":     "4.5 KW",
    "Voltage_text":      "58.4",
    "TRIP_text":         "12.6",
    "odo_text":          "6436",
    "temp_mot_text":     "47",
    "temp_esc_text":     "39",
}

# Dynamic overlays not part of the static handoff design — hidden in the clone.
HIDE = {
    "cruise_control_img", "brightness_slider", "music_info",
    "esc_not_connected_text", "song_title_label", "Speed_cc_text",
}

# Segment bars: cell count and lit-fraction (handoff sample data). Bars fill
# from the bottom, and the highest index sits at the bottom, so the lit cells
# are the high indices: index >= count - round(count * pct).
SEGBARS = {
    "batt_seg_":  (14, 73.0 / 100.0),          # battery 73 %
    "power_seg_": (14, 2.8 / 4.5),             # power 2.8 / 4.5 kW
    # "speed_seg_" is left dim — the handoff speed column has no bar.
}

# ---------------------------------------------------------------------------
# 7-segment font (DSEG7 Classic — the typeface the handoff asks for).
# ImportFonts maps a family name -> ttf path (Windows-style backslash, as GUI
# Guider writes it). The .ttf must already sit in import/font/.
# ---------------------------------------------------------------------------
DSEG7_FAMILY = "DSEG7Classic_Regular"
DSEG7_TTF = "font\\DSEG7Classic-Regular.ttf"

# Only PURE-numeric readouts get the 7-seg font — DSEG7 has digits, ':' and
# '.' but NO letters, so mixed number+unit labels ("38 KM", "6.4 Ah",
# "24.5 A", "4.5 KW") stay on Antonio. In the bottom row the unit is a
# separate widget, so those values are pure-numeric and switch cleanly.
DSEG7_WIDGETS = {
    "Speed_text", "Battery_proc_text", "power_value",
    "Voltage_text", "TRIP_text", "odo_text",
    "temp_mot_text", "temp_esc_text", "col_avg_value",
    "uptime_text", "cur_time_label",
}

# Widget whose stale screen-flip nav (from an earlier revision) gets scrubbed.
NAV_WIDGET = "mode_text"


# ---------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------
def gen_id(used: set[str]) -> str:
    """Deterministic, collision-free 8-char id [a-z0-9] (project id shape)."""
    n = 1
    while True:
        v = "am" + f"{n:06d}"          # am000001, am000002, ...
        if v not in used:
            used.add(v)
            return v
        n += 1


def collect_ids(node: Any, out: set[str]) -> None:
    if isinstance(node, dict):
        v = node.get("id")
        if isinstance(v, str) and v:
            out.add(v)
        for vv in node.values():
            collect_ids(vv, out)
    elif isinstance(node, list):
        for x in node:
            collect_ids(x, out)


def strip_events(screen: dict, items_key: str) -> None:
    """Remove all events (screen-level + per-widget) so the clone is a static
    preview and no embedded `_target` screen copies survive into id remapping."""
    screen["event"] = {"widget": {}}
    for w in screen.get(items_key, []):
        w.pop("event", None)


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


def recolor(node: Any) -> None:
    if isinstance(node, dict):
        for k in COLOR_KEYS:
            val = node.get(k)
            if isinstance(val, str) and val.upper() in COLOR_MAP:
                node[k] = COLOR_MAP[val.upper()]
        for vv in node.values():
            recolor(vv)
    elif isinstance(node, list):
        for x in node:
            recolor(x)


def prefix_app_names(node: Any) -> None:
    """Application-side names are prefixed with the screen name; rewrite
    'dashboard_*' -> 'dashboard_amber_*' everywhere (incl. nested children)."""
    if isinstance(node, dict):
        nm = node.get("name")
        if isinstance(nm, str) and nm.startswith(SRC_SCREEN + "_"):
            node["name"] = NEW_SCREEN + "_" + nm[len(SRC_SCREEN) + 1:]
        for vv in node.values():
            prefix_app_names(vv)
    elif isinstance(node, list):
        for x in node:
            prefix_app_names(x)


def seg_is_lit(key: str) -> bool | None:
    """Returns True/False if `key` is a known segment cell, else None."""
    for pre, (count, pct) in SEGBARS.items():
        if key.startswith(pre):
            try:
                idx = int(key[len(pre):])
            except ValueError:
                return None
            lit_from = count - round(count * pct)
            return idx >= lit_from
    return None


def apply_content(screen: dict, items_key: str, app_side: bool) -> None:
    """Bake sample values, hide overlays, light up segment bars. Keyed by the
    un-prefixed widget name."""
    for w in screen.get(items_key, []):
        name = w.get("name", "")
        key = name[len(NEW_SCREEN) + 1:] if (app_side and name.startswith(NEW_SCREEN + "_")) else name

        if key in HIDE:
            w["visible"] = False
            flags = w.setdefault("flag", [])
            if "LV_OBJ_FLAG_HIDDEN" not in flags:
                flags.append("LV_OBJ_FLAG_HIDDEN")
            continue

        if key in VALUES and w.get("type") == "label":
            w["text"] = VALUES[key]

        if key in DSEG7_WIDGETS:
            for st in w.get("style", []):
                st["font_family"] = DSEG7_FAMILY

        lit = seg_is_lit(key)
        if lit:  # only repaint the lit cells; off cells already amber-dim
            for st in w.get("style", []):
                if st.get("part") == "LV_PART_MAIN":
                    st.update(SEG_ON)


def strip_mode_text_nav(screens: list, items_key: str, screen_name: str, widget_name: str) -> None:
    """Remove any clicked-event + CLICKABLE flag from `widget_name` on
    `screen_name`. Amber is a dashboard THEME (chosen from the Settings
    dropdown), NOT a navigable screen, so no in-UI lv_scr_load is wired to it —
    that would bypass the theme switcher. This undoes a mode_text screen-flip an
    earlier revision of this script added by mistake. Idempotent."""
    scr = next((s for s in screens if s.get("name") == screen_name), None)
    if not scr:
        return
    w = next((x for x in scr.get(items_key, []) if x.get("name") == widget_name), None)
    if not w:
        return
    if w.pop("event", None) is not None:
        print(f"[i] cleared nav event on {screen_name}/{widget_name}")
    flags = w.get("flag")
    if isinstance(flags, list) and "LV_OBJ_FLAG_CLICKABLE" in flags:
        flags.remove("LV_OBJ_FLAG_CLICKABLE")


def clone_screen(src: dict, items_key: str, new_id: str, id_map: dict[str, str],
                 app_side: bool) -> dict:
    new = copy.deepcopy(src)
    strip_events(new, items_key)
    remap_ids(new, id_map)        # screen id + all widget ids -> new ids
    if app_side:
        # Rewrite widget names while the screen is still called 'dashboard'
        # (its root name does not match the 'dashboard_' prefix, so only the
        # widgets are touched), THEN rename the screen itself.
        prefix_app_names(new)     # dashboard_* -> dashboard_amber_*
    new["name"] = NEW_SCREEN
    if "layerName" in new:
        new["layerName"] = NEW_SCREEN
    recolor(new)                  # incl. screen-level bg
    apply_content(new, items_key, app_side)
    return new


# ---------------------------------------------------------------------------
# entry point
# ---------------------------------------------------------------------------
def main() -> int:
    if not os.path.isfile(PROJECT):
        print(f"[err] {PROJECT} not found", file=sys.stderr)
        return 1

    shutil.copy2(PROJECT, BACKUP)
    print(f"[i] backup -> {BACKUP}")

    with open(PROJECT, "r", encoding="utf-8") as f:
        proj = json.load(f)

    app_screens = proj["Application"]["screen"]
    front_screens = proj["FrontJson"]

    # Idempotent: drop any previous dashboard_amber before re-adding.
    app_screens[:] = [s for s in app_screens if s.get("name") != NEW_SCREEN]
    front_screens[:] = [s for s in front_screens if s.get("name") != NEW_SCREEN]

    src_app = next((s for s in app_screens if s.get("name") == SRC_SCREEN), None)
    src_front = next((s for s in front_screens if s.get("name") == SRC_SCREEN), None)
    if src_app is None or src_front is None:
        print(f"[err] source screen '{SRC_SCREEN}' missing in app/front", file=sys.stderr)
        return 1

    used: set[str] = set()
    collect_ids(proj, used)
    print(f"[i] {len(used)} existing ids collected")

    # Build a SHARED old-id -> new-id map from both copies (after a dry strip,
    # so embedded _target ids from events are excluded). app & front share ids,
    # so the same new id lands in both -> widgets stay paired.
    tmp_app = copy.deepcopy(src_app); strip_events(tmp_app, "widgets")
    tmp_front = copy.deepcopy(src_front); strip_events(tmp_front, "list")
    old_ids: set[str] = set()
    collect_ids(tmp_app, old_ids)
    collect_ids(tmp_front, old_ids)
    id_map = {old: gen_id(used) for old in sorted(old_ids)}
    print(f"[i] remapping {len(id_map)} ids ({SRC_SCREEN} -> {NEW_SCREEN})")

    new_screen_id = id_map[src_app["id"]]
    new_app = clone_screen(src_app, "widgets", new_screen_id, id_map, app_side=True)
    new_front = clone_screen(src_front, "list", new_screen_id, id_map, app_side=False)

    app_screens.append(new_app)
    front_screens.append(new_front)

    # Register the DSEG7 7-seg font so GUI Guider generates it on open/export.
    proj.setdefault("ImportFonts", {})[DSEG7_FAMILY] = DSEG7_TTF
    print(f"[i] ImportFonts += {DSEG7_FAMILY} -> {DSEG7_TTF}")

    # Amber is a dashboard THEME (selected via the Settings "Dashboard theme"
    # dropdown → dev_settings + dashboard_theme_set), NOT a navigable screen, so
    # nothing in the UI lv_scr_load's it. Scrub the mode_text screen-flip an
    # earlier revision of this script wired up (green still carries it from a
    # prior run; the amber clone already had its events stripped).
    for arr, key in ((front_screens, "list"), (app_screens, "widgets")):
        strip_mode_text_nav(arr, key, SRC_SCREEN, NAV_WIDGET)

    with open(PROJECT, "w", encoding="utf-8") as f:
        json.dump(proj, f, ensure_ascii=False, indent="\t")

    n_app = len(new_app.get("widgets", []))
    n_front = len(new_front.get("list", []))
    print(f"[ok] added '{NEW_SCREEN}': {n_app} app widgets / {n_front} front widgets, id={new_screen_id}")
    print(f"[ok] {PROJECT}")
    print()
    print("next, in GUI Guider (close & re-open the project if it was open):")
    print(f"  1. File -> Open Project -> {os.path.basename(PROJECT)}")
    print(f"  2. '{NEW_SCREEN}' appears in the screen list — open it to see the amber reskin")
    print(f"  3. Font Manager: confirm '{DSEG7_FAMILY}' is picked up (ttf in import/font/);")
    print( "     it generates the DSEG7 sizes used (24/32/40/64/200) on Generate Code")
    print(f"  4. In the running UI: tap 'MODE' to flip green <-> amber")
    print( "  5. Generate Code -> emits generated/setup_scr_dashboard_amber.c")
    return 0


if __name__ == "__main__":
    sys.exit(main())
