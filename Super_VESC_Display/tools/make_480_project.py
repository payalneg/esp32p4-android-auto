#!/usr/bin/env python3
"""Bootstrap the 480x480 GUI Guider project from the 800x480 one.

The dashboard is authored once, at 800x480, for the ESP32-P4 head units. The
Waveshare ESP32-S3-Touch-LCD-4 board has a square 480x480 panel, so it needs
its own GUI Guider project — same screens, same widget names, narrower layout.
Doing that by hand is ~330 widgets of arithmetic across four dashboard themes,
so this script does the arithmetic and leaves the judgement to a human.

It writes two things, from one set of rules:

  1. Super_VESC_Display_480/Super_VESC_Display_480.guiguider — open this in
     GUI Guider to nudge the layout by eye, then "Generate Code" as usual.
  2. Super_VESC_Display_480/generated/ — the C the firmware compiles, so the
     board has a working (if not yet pretty) dashboard before anyone opens
     GUI Guider, and so the desktop simulator can show it.

Both come out of the same geometry map, so what GUI Guider shows and what the
firmware draws agree. This is a one-shot bootstrap, not a sync tool: once you
start hand-tuning the 480 project, re-running it would throw that away (it
refuses to overwrite unless --force).

Rules, and why:
  * The screen loses 320 px of width and keeps its height, so x and width
    scale by 480/800 = 0.6 and y/height are left alone.
  * Anything that has to stay round or square — meters, LEDs, images, the
    cruise-control icon — keeps its size and gets its CENTRE scaled instead.
    Squeezing a meter horizontally would turn the speedometer into an oval.
  * A bar or slider that is taller than it is wide is treated the same way:
    it is a vertical gauge, not a panel.
  * Everything else (labels, containers, tileviews, buttons, horizontal bars)
    scales its width.
  * Line point arrays scale their x coordinates.
  * Text is where a 0.6 squeeze actually hurts: "ESC NOT CONNECTED" at font 22
    needs ~205 px and its box would become 144. Two levers, in this order:
      1. Step the caption font down. Montserrat captions at 22-24 px were sized
         for a 800-wide screen; on a 480 one they are simply too big, and the
         project already ships every intermediate size (11…26) so a step costs
         no new font and no flash. Only Montserrat is stepped — Antonio is the
         big readouts, whose digits still fit and whose size IS the design.
      2. If it still does not fit, the label keeps whatever width its text
         needs, growing away from its alignment anchor (a right-aligned label
         grows leftwards, a centred one both ways) and sliding inward when the
         screen edge is what is in the way.
    Labels that end up overlapping after that are listed at the end — an
    overlap is a design decision (shorter text, a different spot) rather than
    arithmetic.

Usage:
    python3 Super_VESC_Display/tools/make_480_project.py [--force]
    python3 Super_VESC_Display/tools/make_480_project.py --report-only
"""

import argparse
import json
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(HERE)                      # Super_VESC_Display/
REPO_ROOT = os.path.dirname(SRC_ROOT)
DEFAULT_OUT = os.path.join(REPO_ROOT, "Super_VESC_Display_480")

SRC_PROJECT = os.path.join(SRC_ROOT, "Super_VESC_Display.guiguider")
SRC_GENERATED = os.path.join(SRC_ROOT, "generated")

NEW_W, NEW_H = 480, 480
NEW_PROJECT_NAME = "Super_VESC_Display_480"

# Font sizes the project actually ships as generated C (generated/guider_fonts).
# Stepping outside this list would not link.
AVAILABLE_FONTS = {
    "montserratMedium": [11, 12, 16, 18, 20, 22, 24, 25, 26],
}

# Families whose size is part of the design rather than a container for text:
# the speedometer's Antonio digits are the one thing on the screen that should
# stay as large as they are.
FONT_KEEP_FAMILIES = {"Antonio_Regular", "DSEG7Classic_Regular",
                      "DSEG14Classic_Regular"}

# How far a caption may shrink before it stops being readable at arm's length
# on a handlebar. Below this the script widens the box instead.
FONT_MIN_PX = 14


# Per-widget decisions the arithmetic cannot make.
#
# Some things do not survive a 480-wide screen no matter how the boxes are
# scaled — a six-item status bar, or a full-width warning banner sitting on top
# of it. Those are laid out here explicitly, in the same 480-space the rest of
# the script produces. This is the place to record a design call; everything
# here is overridable by hand in GUI Guider afterwards.
#
# Keys: x, y, w, h, font (size), family, hidden.
def _classic_bottom_strip():
    """The five-column readout strip along the bottom of the Classic screens.

    At 800 px each column is 160 wide and holds a caption, an Antonio-40 value
    and a unit label to the right of it. Scaled to 96 px the value alone needs
    the whole column, which is why the units ended up underneath the digits.
    Laid out here for the narrow screen instead: a smaller value font, and
    fixed slots for value and unit."""
    cols = [
        ("col_voltage_label", "Voltage_text",  "col_v_label"),
        ("col_trip_label",    "TRIP_text",     "col_trip_unit"),
        ("col_odo_label",     "odo_text",      "col_odo_unit"),
        ("col_mtmp_label",    "temp_mot_text", "col_mtmp_unit"),
        ("col_ctmp_label",    "temp_esc_text", "col_ctmp_unit"),
    ]
    rules = {}
    W = 96
    for i, (cap, val, unit) in enumerate(cols):
        x = i * W
        # "M·TEMP" and "C·TEMP" keep font 18: the middot (U+00B7) is only in
        # the sizes the 800 project already used it at, and a caption stepped
        # to 16 renders it as a tofu box. GUI Guider exports a font with
        # exactly the glyphs the project asks for at that size.
        cap_font = 18 if "·" in cap or cap.endswith("tmp_label") else 16
        rules[cap]  = {"x": x,      "w": W,  "font": cap_font}
        rules[val]  = {"x": x + 2,  "w": 62, "font": 32}   # Antonio
        rules[unit] = {"x": x + 66, "w": 28, "font": 16}
    # The average-speed readout shares the last column with C-TEMP; only one of
    # the two is visible at a time.
    x = 4 * W
    rules["col_avg_label"] = {"x": x,      "w": W,  "font": 11}
    rules["col_avg_value"] = {"x": x + 2,  "w": 62, "font": 32}
    rules["col_avg_unit"]  = {"x": x + 66, "w": 28, "font": 11}
    return rules


# Per-widget decisions the arithmetic cannot make.
#
# Some things do not survive a 480-wide screen no matter how the boxes are
# scaled: a six-item status bar, a full-width warning banner sitting on top of
# it, or a column that has to hold a value and its unit side by side in 108 px.
# Those are laid out here explicitly, in the same 480-space the rest of the
# script produces. This is the place to record a design call; everything here
# can still be moved by hand in GUI Guider afterwards.
#
# Keys: x, y, w, h, font (size), hidden.
SQUARE_OVERRIDES = {
    # Status bar. At 800 it holds VESC | MODE | uptime | STATISTICS | clock |
    # SETTINGS | BT. Six of those fit across 480 at font 16; the clock does
    # not, and the firmware hides it anyway (ENABLE_WALL_CLOCK=0), so it is
    # hidden rather than left to overlap. The red "ESC NOT CONNECTED" banner
    # moves out of the bar and onto the speed column, where there is room.
    "status_bar": {
        "status_vesc":       {"x": 6,   "w": 52,  "font": 16},
        "mode_text":         {"x": 62,  "w": 74,  "font": 16},
        "uptime_text":       {"x": 140, "w": 84,  "font": 16},
        "statistics_button": {"x": 228, "w": 116, "font": 16},
        "Settings_text":     {"x": 350, "w": 100, "font": 16},
        "status_bt":         {"x": 452, "w": 26,  "font": 16},
        "cur_time_label":    {"hidden": True},
        # Antonio 22 carries capitals and digits in the subsetted firmware
        # font, so the warning keeps its typeface — it just stops covering
        # the status bar.
        "esc_not_connected_text": {"x": 110, "y": 40, "w": 260, "h": 26},
    },

    # The two side columns are 108 px wide on the square panel (the separators
    # land at 108 and 372). A value and its unit cannot sit side by side in
    # that, so the RANGE and MAX readouts stack caption over value instead.
    "side_columns": {
        "battery_label":       {"x": 6,   "w": 100, "font": 16},
        "Battery_proc_text":   {"x": 6,   "w": 70,  "font": 50},   # Antonio
        "battery_pct":         {"x": 78,  "y": 96,  "w": 28, "font": 16},
        "Ah_text":             {"x": 6,   "w": 100, "font": 16},
        "battery_range_label": {"x": 4,   "y": 362, "w": 100, "h": 14,
                                "font": 12},
        "Range_text":          {"x": 4,   "y": 376, "w": 100, "h": 26,
                                "font": 18},

        "power_label":         {"x": 374, "w": 102, "font": 16},
        "power_value":         {"x": 374, "w": 70,  "font": 50},   # Antonio
        "power_unit":          {"x": 448, "y": 96,  "w": 28, "font": 16},
        "Current_text":        {"x": 374, "w": 102, "font": 16},
        "power_max_label":     {"x": 376, "y": 362, "w": 100, "h": 14,
                                "font": 12},
        "power_max_val":       {"x": 376, "y": 376, "w": 100, "h": 26,
                                "font": 18},

        # The cruise-control readout and the max-speed caption ride with the
        # speed column in the middle.
        "Speed_cc_text":       {"x": 300, "w": 60,  "font": 40},   # Antonio
        "speed_max":           {"x": 190, "w": 84,  "font": 16},
    },

    "bottom_strip": _classic_bottom_strip(),

    # Classic Max adds a row of session maxima between the gauges and the
    # readout strip. At 800 it is caption + value laid out sideways three
    # times; in the 264 px the square screen leaves between the separators
    # they stack instead, two lines each.
    # Lamborghini/Supermoto survive the squeeze almost intact; these are the
    # spots where a unit label ends up on top of its value or off its panel.
    "lambo_units": {
        "range_unit": {"x": 458, "y": 266, "w": 20, "font": 11},
        "AMPS_text":  {"x": 8,   "w": 92,  "font": 24},
        # Three LED + caption pairs share the bottom-left corner. Keeping each
        # LED's own size (they are round) and scaling only their centres puts
        # the captions under the next LED along, so the pairs are spaced out
        # by hand here.
        "led_tt_cruise_lbl": {"x": 4,   "w": 14},
        "tt_cruise_lbl":     {"x": 20,  "w": 30, "font": 11},
        "led_tt_fault_lbl":  {"x": 54,  "w": 14},
        "tt_fault_lbl":      {"x": 70,  "w": 30, "font": 11},
        "led_status_bt":     {"x": 104, "w": 14},
        "status_bt":         {"x": 120, "w": 26, "font": 11},
        "TRIP_text": {"x": 222, "w": 30, "font": 12},
        "trip_unit": {"x": 254, "w": 24, "font": 11},
        "odo_text":  {"x": 282, "w": 42, "font": 12},
        "odo_unit":  {"x": 326, "w": 24, "font": 11},
    },
    "supermoto_units": {
        "trip_unit": {"x": 258, "w": 24, "font": 11},
        "odo_unit":  {"x": 352, "w": 24, "font": 11},
        "power_unit": {"x": 447, "w": 26, "font": 11},
    },

    "classic_max_row": {
        "max_power_label": {"x": 114, "y": 360, "w": 84, "h": 14, "font": 12},
        "max_power_text":  {"x": 114, "y": 374, "w": 84, "h": 26, "font": 18},
        "max_speed_label": {"x": 202, "y": 360, "w": 84, "h": 14, "font": 12},
        "max_speed_text":  {"x": 202, "y": 374, "w": 84, "h": 26, "font": 18},
        "max_reset_btn":   {"x": 292, "y": 366, "w": 76, "h": 28, "font": 16},
    },
}

# Which screens get which override group.
SQUARE_OVERRIDE_SCREENS = {
    "dashboard_Classic":     ["status_bar", "side_columns", "bottom_strip"],
    "dashboard_Classic_Max": ["status_bar", "side_columns", "bottom_strip",
                              "classic_max_row"],
    "dashboard_Lamborghini": ["lambo_units"],
    "dashboard_Supermoto":   ["supermoto_units"],
}


# Widgets that must not be squeezed on one axis: their size is kept and their
# centre is moved instead.
KEEP_ASPECT = {
    "img", "meter", "led", "animimg", "imgbtn", "spinner", "cpicker",
    "arc", "image3D",
}

# Screens that exist only as a GUI Guider widget palette. Not built by anything
# (and excluded from the firmware because GUI Guider emitted LVGL 9 forms for
# it), so the 480 project drops it rather than carrying a broken copy.
DROP_SCREENS = {"reference"}

# Advance width per character as a fraction of the font size, used to decide how
# much width a label needs. One constant does not do: the captions are all-caps
# Montserrat (~0.72 em per glyph) while the readouts are digits (~0.60), and
# sizing a caption by the digit figure is exactly what makes "RANGE" wrap to
# "RANG/E". Calibrated against the rendered screenshots, erring high — a
# transparent box a few pixels too wide costs nothing, a wrapped caption reads
# as broken. Letter spacing from the style is added on top.
CHAR_EM_DEFAULT = 0.62
CHAR_EM = {
    # Capitals, which is what the captions and the unit suffixes are made of.
    # M and W are nearly twice the width of I — averaging them is what made
    # "KM" and "kW" wrap in boxes the estimate said were big enough.
    **{c: 0.95 for c in "MW"},
    **{c: 0.74 for c in "ABCDGHKNOPQRSUVXYZ"},
    **{c: 0.66 for c in "CEFLT"},
    **{c: 0.35 for c in "IJ"},
    **{c: 0.60 for c in "abcdefghijklmnopqrstuvwxyz0123456789"},
    **{c: 0.85 for c in "mw"},
    **{c: 0.32 for c in " .,:;'`|!"},
    **{c: 0.42 for c in "-()[]/"},
    "%": 0.95,
    "°": 0.45,
}


class Geometry:
    """The new geometry of one widget, plus what to do with its children."""

    def __init__(self, x, y, w, h, child_ratio):
        self.x, self.y, self.w, self.h = x, y, w, h
        self.child_ratio = child_ratio
        self.hidden = False


def transform(kind, x, y, w, h, ratio, parent_w):
    """Map one widget's box from the 800-wide layout into the narrow one."""
    vertical_gauge = kind in ("bar", "slider") and h > w * 1.5
    if kind in KEEP_ASPECT or vertical_gauge:
        new_w, new_h = w, h
        new_x = int(round((x + w / 2.0) * ratio - w / 2.0))
    else:
        new_w, new_h = max(1, int(round(w * ratio))), h
        new_x = int(round(x * ratio))

    # Keep the widget on screen: clamp width first, then slide it back inside.
    if parent_w and new_w > parent_w:
        new_w = parent_w
    if new_x < 0:
        new_x = 0
    if parent_w and new_x + new_w > parent_w:
        new_x = max(0, parent_w - new_w)

    child_ratio = (new_w / float(w)) if w else ratio
    return Geometry(new_x, y, new_w, new_h, child_ratio)


def scale_line_points(widget, ratio):
    """Scale the x of a line widget's points. GUI Guider stores them as a list
    of {"p1": x, "p2": y} objects; the generated C emits the same numbers as
    lv_point_t pairs."""
    pts = widget.get("line_points")
    if not isinstance(pts, list):
        return
    for p in pts:
        if isinstance(p, dict) and "p1" in p:
            p["p1"] = int(round(p["p1"] * ratio))
        elif isinstance(p, (list, tuple)) and len(p) >= 1:
            p[0] = int(round(p[0] * ratio))


def text_align_of(widget):
    for st in widget.get("style") or []:
        a = st.get("text_align")
        if isinstance(a, str) and a.startswith("LV_TEXT_ALIGN_"):
            return a[len("LV_TEXT_ALIGN_"):]
    return "LEFT"


def font_family_of(widget):
    for st in widget.get("style") or []:
        f = st.get("font_family")
        if isinstance(f, str) and f:
            return f
    return None


def letter_space_of(widget):
    for st in widget.get("style") or []:
        v = st.get("letter_space")
        if isinstance(v, int):
            return v
    return 0


def text_width_estimate(text, font_size, letter_space=0):
    """Widest line of `text`, in pixels, roughly."""
    if not text or not font_size:
        return 0
    widest = 0
    for line in text.split("\n"):
        w = sum(CHAR_EM.get(c, CHAR_EM_DEFAULT) * font_size + letter_space
                for c in line)
        widest = max(widest, w)
    return int(widest + 0.5)


def step_font_down(widget, family, size, text, letter_space, avail_w):
    """Pick the largest shipped size of `family` whose text fits `avail_w`.

    Returns the size to use (unchanged when nothing smaller helps or the family
    is one we keep). Only ever steps down, never up."""
    if not family or family in FONT_KEEP_FAMILIES:
        return size
    sizes = AVAILABLE_FONTS.get(family)
    if not sizes or size not in sizes:
        return size
    candidates = sorted((s for s in sizes if s <= size and s >= FONT_MIN_PX),
                        reverse=True)
    for cand in candidates:
        if text_width_estimate(text, cand, letter_space) <= avail_w:
            return cand
    # Nothing fits the box. Step down anyway, but only so far: a caption that
    # has to overflow should overflow by as little as possible without turning
    # into a different size class than its neighbours. The box is widened
    # afterwards to cover the rest.
    floor = max(FONT_MIN_PX, int(round(size * 0.7)))
    for cand in candidates:
        if cand <= floor:
            return cand
    return size


def set_widget_font(widget, new_size):
    """Rewrite the font size in every style part of a widget."""
    for st in widget.get("style") or []:
        if isinstance(st.get("font"), int):
            st["font"] = new_size


def widen_for_text(g, need, align, screen_w):
    # (returns True when the screen is simply too narrow for the text)
    """Give a label back the width its text needs, growing away from whichever
    edge its alignment pins the text to — and sliding the whole box left when
    the screen edge is in the way, because a caption pushed a few pixels off
    its column still reads, while a wrapped one does not."""
    if need <= g.w:
        return
    want = min(need, screen_w)
    if align == "RIGHT":
        right = g.x + g.w
        g.x = right - want
    elif align == "CENTER":
        centre = g.x + g.w / 2.0
        g.x = int(round(centre - want / 2.0))
    if g.x + want > screen_w:
        g.x = screen_w - want
    if g.x < 0:
        g.x = 0
    g.w = min(want, screen_w - g.x)
    return g.w < need


def font_size_of(widget):
    """Largest font size any style part of this widget asks for."""
    best = 0
    for st in widget.get("style") or []:
        f = st.get("font")
        if isinstance(f, int):
            best = max(best, f)
    return best


def walk_front(screen, ratio, parent_w, geo_map, prefix, boxes, fonts):
    """Transform FrontJson-style widgets (left/top/width/height, bare names)."""
    for w in screen.get("list") or []:
        kind = w.get("type")
        if kind is None or "left" not in w:
            continue
        g = transform(kind, w["left"], w["top"], w["width"], w["height"],
                      ratio, parent_w)
        full_name = prefix + w["name"]
        geo_map[full_name] = g

        if kind == "label":
            text = w.get("text") or ""
            size = font_size_of(w)
            ls = letter_space_of(w)
            family = font_family_of(w)

            if text and size and text_width_estimate(text, size, ls) > g.w:
                stepped = step_font_down(w, family, size, text, ls, g.w)
                if stepped != size:
                    set_widget_font(w, stepped)
                    fonts.append((full_name, family, size, stepped, text))
                    size = stepped

            need = text_width_estimate(text, size, ls)
            clipped = False
            if need > g.w:
                clipped = widen_for_text(g, need, text_align_of(w), NEW_W)
            boxes.append({
                "clipped": clipped,
                "need": need,
                "name": full_name,
                "text": w.get("text") or "",
                "old": (w["left"], w["top"], w["width"], w["height"]),
                "new": (g.x, g.y, g.w, g.h),
            })

        w["left"], w["top"] = g.x, g.y
        w["width"], w["height"] = g.w, g.h
        scale_line_points(w, g.child_ratio)
        walk_front(w, g.child_ratio, g.w, geo_map, prefix, boxes, fonts)


def apply_overrides(screen, geo_map, boxes):
    """Apply the hand-written SQUARE_OVERRIDES for this screen, after the
    arithmetic has run, so a design call always wins."""
    groups = SQUARE_OVERRIDE_SCREENS.get(screen["name"], [])
    if not groups:
        return
    rules = {}
    for g in groups:
        rules.update(SQUARE_OVERRIDES.get(g, {}))

    by_name = {b["name"]: b for b in boxes}
    for w in screen.get("list") or []:
        rule = rules.get(w.get("name"))
        if not rule:
            continue
        full = screen["name"] + "_" + w["name"]
        g = geo_map.get(full)

        if rule.get("hidden"):
            w["visible"] = False
            if g:
                g.hidden = True
            by_name.pop(full, None)
            boxes[:] = [b for b in boxes if b["name"] != full]
            continue

        for key, field in (("x", "left"), ("y", "top"),
                           ("w", "width"), ("h", "height")):
            if key in rule:
                w[field] = rule[key]
                if g:
                    setattr(g, {"x": "x", "y": "y", "w": "w", "h": "h"}[key],
                            rule[key])
        if "font" in rule:
            set_widget_font(w, rule["font"])
        if "family" in rule:
            for st in w.get("style") or []:
                if "font_family" in st:
                    st["font_family"] = rule["family"]
        if full in by_name:
            b = by_name[full]
            b["new"] = (w["left"], w["top"], w["width"], w["height"])


def apply_fonts(node, font_by_name):
    for w in node.get("widgets") or []:
        new = font_by_name.get(w.get("name"))
        if new:
            set_widget_font(w, new)
        apply_fonts(w, font_by_name)


def walk_app(node, ratio, parent_w, geo_map):
    """Transform Application.screen-style widgets (pos/size, prefixed names).

    GUI Guider keeps two parallel copies of every screen: the designer tree in
    FrontJson and this one, which is what its code generator reads. They must
    agree, so this pass reuses the geometry computed for FrontJson (the names
    here are already screen-prefixed, the same key) and only falls back to
    computing when a widget somehow isn't in the map."""
    for w in node.get("widgets") or []:
        kind = w.get("type")
        pos, size = w.get("pos"), w.get("size")
        if kind is None or not pos or not size:
            continue
        g = geo_map.get(w.get("name"))
        if g is None:
            g = transform(kind, pos[0], pos[1], size[0], size[1], ratio, parent_w)
        w["pos"] = [g.x, g.y]
        w["size"] = [g.w, g.h]
        scale_line_points(w, g.child_ratio)
        walk_app(w, g.child_ratio, g.w, geo_map)


def rewrite_project(out_project):
    """Rewrite the .guiguider JSON; returns the name -> Geometry map."""
    with open(SRC_PROJECT, "r", encoding="utf-8") as fh:
        proj = json.load(fh)

    src_w = int(proj["projectSettings"]["resolution"]["width"])
    src_h = int(proj["projectSettings"]["resolution"]["height"])
    ratio = NEW_W / float(src_w)
    if src_h != NEW_H:
        print("note: source height %d != %d — vertical layout is copied as-is"
              % (src_h, NEW_H))

    proj["projectName"] = NEW_PROJECT_NAME
    proj["projectSettings"]["resolution"] = {"width": str(NEW_W),
                                             "height": str(NEW_H)}
    proj["Application"]["name"] = NEW_PROJECT_NAME
    proj["Application"]["size"] = [NEW_W, NEW_H]
    if "lvConf" in proj:
        proj["lvConf"]["LV_HOR_RES_MAX"] = "(%d)" % NEW_W
        proj["lvConf"]["LV_VER_RES_MAX"] = "(%d)" % NEW_H

    geo_map = {}
    boxes_by_screen = {}
    font_changes = []

    proj["FrontJson"] = [s for s in proj["FrontJson"]
                         if s.get("name") not in DROP_SCREENS]
    for screen in proj["FrontJson"]:
        screen["width"], screen["height"] = NEW_W, NEW_H
        boxes = []
        walk_front(screen, ratio, NEW_W, geo_map, screen["name"] + "_", boxes,
                   font_changes)
        apply_overrides(screen, geo_map, boxes)
        boxes_by_screen[screen["name"]] = boxes

    proj["Application"]["screen"] = [s for s in proj["Application"]["screen"]
                                     if s.get("name") not in DROP_SCREENS]
    for screen in proj["Application"]["screen"]:
        screen["width"], screen["height"] = NEW_W, NEW_H
        screen["size"] = [NEW_W, NEW_H]
        walk_app(screen, ratio, NEW_W, geo_map)

    if proj["projectSettings"].get("defaultScreen"):
        keep = {s["id"] for s in proj["FrontJson"]}
        if proj["projectSettings"]["defaultScreen"] not in keep:
            proj["projectSettings"]["defaultScreen"] = proj["FrontJson"][0]["id"]

    if out_project:
        with open(out_project, "w", encoding="utf-8") as fh:
            # Compact, like GUI Guider's own saves (it rewrites this file
            # wholesale anyway, and an indented copy is ~15% larger).
            json.dump(proj, fh, ensure_ascii=False)
        print("wrote %s (%d screens, %d widgets)"
              % (os.path.relpath(out_project, REPO_ROOT),
                 len(proj["FrontJson"]), len(geo_map)))

    screens = [s["name"] for s in proj["FrontJson"]]

    # The second (code-generator) tree carries its own copy of every style, so
    # the stepped sizes have to land there too or GUI Guider would regenerate
    # the original ones.
    font_by_name = {name: new for name, _f, _old, new, _t in font_changes}
    # Overrides win over the stepping above.
    for screen in proj["FrontJson"]:
        for grp in SQUARE_OVERRIDE_SCREENS.get(screen["name"], []):
            for wname, rule in SQUARE_OVERRIDES.get(grp, {}).items():
                if "font" in rule:
                    font_by_name[screen["name"] + "_" + wname] = rule["font"]
    for screen in proj["Application"]["screen"]:
        apply_fonts(screen, font_by_name)

    hidden = tuple(
        screen["name"] + "_" + wname
        for screen in proj["FrontJson"]
        for grp in SQUARE_OVERRIDE_SCREENS.get(screen["name"], [])
        for wname, rule in SQUARE_OVERRIDES.get(grp, {}).items()
        if rule.get("hidden"))

    return (geo_map, screens, boxes_by_screen, font_by_name, font_changes,
            hidden)


POS_RE = re.compile(r"(lv_obj_set_pos\(ui->([A-Za-z0-9_]+),\s*)(-?\d+)(,\s*)(-?\d+)(\))")
SIZE_RE = re.compile(r"(lv_obj_set_size\(ui->([A-Za-z0-9_]+),\s*)(-?\d+)(,\s*)(-?\d+)(\))")
POINTS_RE = re.compile(r"(static\s+lv_point_t\s+([A-Za-z0-9_]+)\[\]\s*=\s*)\{(.*?)\};",
                       re.S)
FONT_RE = re.compile(
    r"(lv_obj_set_style_text_font\(ui->([A-Za-z0-9_]+),\s*&lv_font_)"
    r"([A-Za-z_]+?)_(\d+)(,)")
PAIR_RE = re.compile(r"\{\s*(-?\d+)\s*,\s*(-?\d+)\s*\}")


def rewrite_c(src, geo_map, screens, ratio, font_by_name=None,
              hidden=()):
    """Rewrite one generated .c so its geometry matches the new project."""
    def fix_pos(m):
        head, name, x, mid, y, tail = m.groups()
        g = geo_map.get(name)
        if not g:
            return m.group(0)
        return "%s%d%s%d%s" % (head, g.x, mid, g.y, tail)

    def fix_size(m):
        head, name, w, mid, h, tail = m.groups()
        if name in screens:
            return "%s%d%s%d%s" % (head, NEW_W, mid, NEW_H, tail)
        g = geo_map.get(name)
        if not g:
            return m.group(0)
        return "%s%d%s%d%s" % (head, g.w, mid, g.h, tail)

    def fix_points(m):
        head, name, body = m.groups()
        # Points are relative to the line widget, which scales with its parent.
        g = geo_map.get(name)
        r = g.child_ratio if g else ratio
        new_body = PAIR_RE.sub(
            lambda p: "{%d, %s}" % (int(round(int(p.group(1)) * r)), p.group(2)),
            body)
        return "%s{%s};" % (head, new_body)

    def fix_font(m):
        head, name, family, size, tail = m.groups()
        new = (font_by_name or {}).get(name)
        if not new:
            return m.group(0)
        return "%s%s_%d%s" % (head, family, new, tail)

    out = POS_RE.sub(fix_pos, src)
    out = SIZE_RE.sub(fix_size, out)
    out = POINTS_RE.sub(fix_points, out)
    out = FONT_RE.sub(fix_font, out)

    # Widgets an override hides: GUI Guider emits the flag itself for
    # visible:false, but this file was generated from the 800 project, so add
    # it next to the widget's own set_size call.
    for name in hidden:
        marker = "lv_obj_set_size(ui->%s," % name
        i = out.find(marker)
        if i < 0:
            continue
        eol = out.find("\n", i)
        if eol < 0:
            continue
        if "lv_obj_add_flag(ui->%s, LV_OBJ_FLAG_HIDDEN)" % name in out:
            continue
        out = (out[:eol + 1]
               + "    lv_obj_add_flag(ui->%s, LV_OBJ_FLAG_HIDDEN);\n" % name
               + out[eol + 1:])
    return out


# What the 480 project owns. Fonts and images are byte-identical to the 800
# project's (this script changes no font size and no image), so they are taken
# from there by the firmware build and by the simulator instead of being
# duplicated — a second copy of guider_fonts alone is ~1.4 MB of generated C.
# GUI Guider re-creates them inside this project on "Generate Code"; the
# .gitignore written below keeps those copies out of the repository.
SCREEN_FILES = (
    "gui_guider.c", "gui_guider.h",
    "events_init.c", "events_init.h",
    "widgets_init.c", "widgets_init.h",
    "generated.mk",
)

GENERATED_GITIGNORE = """\
# Fonts and images are shared with the 800x480 project (this project changes
# neither), and components/vesc_ui compiles them from there — see
# Super_VESC_Display/tools/make_480_project.py. GUI Guider still writes its own
# copies here whenever it generates code; they are not part of the repository.
guider_fonts/
guider_customer_fonts/
images/
MicroPython/
"""


# Directories the 480 project shares with the 800 one rather than duplicating.
# GUI Guider resolves a project's fonts and images relative to the project
# directory (ImportFonts holds paths like "font\\Montserrat-medium.ttf" under
# import/), so without import/ it silently falls back to a serif face for every
# family it cannot find — the canvas then looks nothing like the device.
# custom/ and lvgl/ are shared because they ARE shared: the firmware compiles
# one copy of custom/ for both geometries.
SHARED_DIRS = ("import", "custom", "lvgl")


def link_shared(out_dir):
    """Point the 480 project at the shared asset/code directories."""
    for name in SHARED_DIRS:
        dst = os.path.join(out_dir, name)
        if os.path.lexists(dst):
            continue
        os.symlink(os.path.join("..", "Super_VESC_Display", name), dst)
        print("linked %s -> ../Super_VESC_Display/%s"
              % (os.path.relpath(dst, REPO_ROOT), name))


def copy_generated(out_dir, geo_map, screens, ratio, font_by_name=None,
                   hidden=()):
    src = SRC_GENERATED
    dst = os.path.join(out_dir, "generated")
    if os.path.isdir(dst):
        shutil.rmtree(dst)
    os.makedirs(dst)

    for fname in os.listdir(src):
        if fname in SCREEN_FILES:
            shutil.copy2(os.path.join(src, fname), os.path.join(dst, fname))
        elif (fname.startswith("setup_scr_") and
              fname[len("setup_scr_"):].rsplit(".", 1)[0] not in DROP_SCREENS):
            shutil.copy2(os.path.join(src, fname), os.path.join(dst, fname))

    with open(os.path.join(dst, ".gitignore"), "w", encoding="utf-8") as fh:
        fh.write(GENERATED_GITIGNORE)

    touched = 0
    for fname in sorted(os.listdir(dst)):
        if not fname.endswith(".c"):
            continue
        path = os.path.join(dst, fname)
        with open(path, "r", encoding="utf-8", errors="surrogateescape") as fh:
            src_text = fh.read()
        new_text = rewrite_c(src_text, geo_map, screens, ratio, font_by_name,
                             hidden)
        if new_text != src_text:
            with open(path, "w", encoding="utf-8",
                      errors="surrogateescape") as fh:
                fh.write(new_text)
            touched += 1

    # gui_guider.h still declares the dropped screen's setup function.
    hdr = os.path.join(dst, "gui_guider.h")
    if os.path.exists(hdr):
        with open(hdr, "r", encoding="utf-8") as fh:
            lines = fh.readlines()
        keep = [ln for ln in lines
                if not any(("setup_scr_%s(" % s) in ln for s in DROP_SCREENS)]
        with open(hdr, "w", encoding="utf-8") as fh:
            fh.writelines(keep)

    print("rewrote geometry in %d generated source file(s) -> %s"
          % (touched, os.path.relpath(dst, REPO_ROOT)))


def overlap_fraction(b1, b2):
    """Shared area as a fraction of the smaller box (0 when disjoint)."""
    x1, y1, w1, h1 = b1
    x2, y2, w2, h2 = b2
    if w1 <= 0 or h1 <= 0 or w2 <= 0 or h2 <= 0:
        return 0.0
    ox = min(x1 + w1, x2 + w2) - max(x1, x2)
    oy = min(y1 + h1, y2 + h2) - max(y1, y2)
    if ox <= 0 or oy <= 0:
        return 0.0
    smaller = min(w1 * h1, w2 * h2)
    return (ox * oy) / float(smaller) if smaller else 0.0


def find_overlaps(boxes, min_fraction=0.25, min_worse=0.10):
    """Label pairs that cover each other MORE than they did at 800x480.

    Boxes overlapping by design is normal here: a value label is a wide
    right-aligned box with its unit label sitting in the gap next to the
    digits, so the two rectangles already intersected in the original layout.
    Only a pair that got materially worse is worth a human's attention."""
    hits = []
    labelled = [b for b in boxes if b["text"].strip()]
    for i in range(len(labelled)):
        b1 = labelled[i]
        for j in range(i + 1, len(labelled)):
            b2 = labelled[j]
            now = overlap_fraction(b1["new"], b2["new"])
            if now < min_fraction:
                continue
            before = overlap_fraction(b1["old"], b2["old"])
            if now - before < min_worse:
                continue
            hits.append((now, before, b1["name"], b1["text"],
                         b2["name"], b2["text"]))
    hits.sort(reverse=True)
    return hits


def print_report(boxes_by_screen, font_changes=()):
    if font_changes:
        print("\n%d caption(s) stepped down a font size to fit:"
              % len(font_changes))
        by_step = {}
        for name, family, old, new, text in font_changes:
            by_step.setdefault((family, old, new), []).append(text)
        for (family, old, new), texts in sorted(by_step.items()):
            sample = ", ".join(sorted(set(t[:14] for t in texts))[:4])
            print("  %-18s %2d -> %2d  x%-3d  %s"
                  % (family, old, new, len(texts), sample))

    # Labels whose text does not fit the screen at all — these will wrap or be
    # cut, which reads as broken, so they come first.
    clipped = [(s, b) for s in sorted(boxes_by_screen)
               for b in boxes_by_screen[s] if b.get("clipped")]
    if clipped:
        print("\n%d label(s) whose text does not fit 480 px — they will wrap:"
              % len(clipped))
        for screen, b in clipped:
            short = b["name"][len(screen) + 1:] \
                if b["name"].startswith(screen) else b["name"]
            print("  %-20s %-26s %3d px of ~%d needed  %r"
                  % (screen, short, b["new"][2], b["need"], b["text"][:20]))
        print("  -> shorten the caption or drop a font size in GUI Guider.")

    total = 0
    for screen in sorted(boxes_by_screen):
        hits = find_overlaps(boxes_by_screen[screen])
        if not hits:
            continue
        total += len(hits)
        print("\n%s — %d label pair(s) that now overlap more than they did:"
              % (screen, len(hits)))
        for now, before, n1, t1, n2, t2 in hits:
            short1 = n1[len(screen) + 1:] if n1.startswith(screen) else n1
            short2 = n2[len(screen) + 1:] if n2.startswith(screen) else n2
            print("  %3d%% (was %2d%%)  %-24s %-14r vs %-24s %r"
                  % (int(now * 100), int(before * 100),
                     short1, t1[:12], short2, t2[:12]))
    if total == 0:
        print("\nno label pair overlaps worse than it did at 800x480")
    else:
        print("\n%d pair(s) to sort out by eye in GUI Guider. Text kept its "
              "width where it needed to, so what is left is placement: move a "
              "widget, shorten a caption, or drop a font size." % total)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out-dir", default=DEFAULT_OUT,
                    help="where the 480 project goes (default: %(default)s)")
    ap.add_argument("--force", action="store_true",
                    help="overwrite an existing project file (throws away "
                         "hand-tuning done in GUI Guider)")
    ap.add_argument("--report-only", action="store_true",
                    help="print the fit report and write nothing")
    args = ap.parse_args()

    if not os.path.exists(SRC_PROJECT):
        sys.exit("source project not found: %s" % SRC_PROJECT)

    out_project = os.path.join(args.out_dir,
                               NEW_PROJECT_NAME + ".guiguider")
    if args.report_only:
        _, _, boxes, _fonts, changes, _hidden = rewrite_project(None)
        print_report(boxes, changes)
        return

    if os.path.exists(out_project) and not args.force:
        sys.exit("%s already exists — re-running would discard hand-tuning.\n"
                 "Pass --force if that is what you want."
                 % os.path.relpath(out_project, REPO_ROOT))

    os.makedirs(args.out_dir, exist_ok=True)
    (geo_map, screens, boxes, font_by_name, changes,
     hidden) = rewrite_project(out_project)
    with open(SRC_PROJECT, "r", encoding="utf-8") as fh:
        ratio = NEW_W / float(json.load(fh)["projectSettings"]["resolution"]["width"])
    copy_generated(args.out_dir, geo_map, screens, ratio, font_by_name, hidden)
    link_shared(args.out_dir)
    print_report(boxes, changes)


if __name__ == "__main__":
    main()
