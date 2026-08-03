#!/usr/bin/env python3
"""
Adds the two concept cluster screens from `vesc_dashboard_advanced.html`
(800x480 live mock-ups) to Super_VESC_Display.guiguider:

  * dashboard_Lamborghini — hex frame accents, shift-light strip, centre arc
                            gauge with tick ring + redline, wedge side cards,
                            bidirectional regen/power flow bar, bottom bar.
  * dashboard_Supermoto   — telltale rail, big tach ring with a max-hold marker,
                            segmented battery gauge, temp cards with bars,
                            symmetric power-flow bar, trip/odo/mode strip.

Both are named `dashboard_<Suffix>`, so scripts/gen_dashboard_themes.py picks
them up at build time and registers each as a generic theme — they appear in
the Settings "Dashboard theme" dropdown with no hand-written C. Their readout
widgets therefore use the naming convention from
Super_VESC_Display/custom/theme_generic.h (Speed_text, Battery_proc_text,
batt_seg_NN, ...) and their frozen sample text matches the formats
theme_generic.c writes, so the editor mock-up reads like the running screen.

Unlike tools/build_amber_dashboard.py (which CLONES dashboard_Classic and
recolours it) these layouts share nothing with the existing screens, so every
widget is built from scratch here.

GUI Guider keeps two parallel representations of every screen:
  * FrontJson[]          — editor-canvas model: `list`, left/top/width/height,
                           un-prefixed names. list[0] is the TOPMOST layer.
  * Application.screen[] — code-gen model: `widgets`, pos/size, names prefixed
                           with the screen name. Array order == creation order,
                           i.e. the exact REVERSE of FrontJson.list.
Both copies of a widget share the same `id`. This script builds one back-to-
front widget list per screen and emits both representations from it.

HTML -> LVGL substitutions (the mock uses browser features LVGL v8 has not):
  radial-gradient bg   -> vertical bg gradient
  carbon-weave texture -> dropped (would need a tiled image asset)
  clip-path wedges     -> plain rectangles with a coloured left/right border
  SVG arc + dasharray  -> lv_meter arc indicators (track / value / redline)
  SVG tick ring        -> lv_meter scale ticks (major tick labels hidden by
                          painting them in the background colour)
  ➯ ⚠ ☭ ⚙ telltales    -> lv_led lamps + 3-letter captions; the montserrat
                          fonts GUI Guider exports here carry NO symbol glyphs
  italic mono numerals -> Antonio_Regular (the project's condensed cluster face)

Fonts: everything reuses sizes the project already generates, except one new
size — Antonio_Regular @100 for the two hero speed readouts. Add it to JOBS in
tools/regen_cockpit_fonts.py and to COCKPIT_FONT_NAMES in
components/vesc_ui/CMakeLists.txt (done in the same change) or the firmware
ships GUI Guider's full-charset export instead of the digit subset.

Idempotent: a second run drops any previous copy of both screens first.

    cd Super_VESC_Display
    python3 tools/build_concept_dashboards.py
"""

from __future__ import annotations

import copy
import json
import os
import sys
from typing import Any

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT = os.path.join(ROOT, "Super_VESC_Display.guiguider")
BACKUP = PROJECT + ".concept.bak"

LAMBO = "dashboard_Lamborghini"
MOTO = "dashboard_Supermoto"
# Screen names this script has ever emitted — all are dropped before rebuilding
# so a rename never leaves an orphan screen behind in the project.
OWNED = {LAMBO, MOTO, "dashboard_Lambo", "dashboard_Moto"}

MONO = "montserratMedium"      # small labels / units / mixed-case text
NUM = "Antonio_Regular"        # condensed numerals (the cluster face)


# ---------------------------------------------------------------------------
# style builders — key sets copied verbatim from the `reference` screen so the
# editor gets exactly the properties it expects per widget type
# ---------------------------------------------------------------------------
def _label_state(state: str, disable: bool, color: str, font: int, family: str,
                 align: str, ls: int, opa: int) -> dict:
    return {
        "part": "LV_PART_MAIN", "state": state, "disable": disable,
        "border_side": ["LV_BORDER_SIDE_FULL"], "border_width": 0,
        "border_opa": 255, "border_color": "#000000", "radius": 0,
        "text_color": color, "text_opa": opa, "font": font,
        "text_align": align, "font_family": family,
        "letter_space": ls, "line_space": 0,
        "bg_color": "#000000", "bg_grad_color": "#000000", "bg_opa": 0,
        "bg_grad_dir": "LV_GRAD_DIR_NONE", "bg_dither_mode": "LV_DITHER_NONE",
        "bg_main_stop": 0, "bg_grad_stop": 255,
        "padding_top": 0, "padding_right": 0, "padding_bottom": 0,
        "padding_left": 0,
        "bg_img_src": "", "bg_img_opa": 255, "bg_img_recolor": "#ffffff",
        "bg_img_recolor_opa": 0,
        "shadow_color": "#000000", "shadow_opa": 255, "shadow_width": 0,
        "shadow_spread": 0, "shadow_ofs_x": 0, "shadow_ofs_y": 0,
    }


def label(name, left, top, width, height, text, *, color="#FFFFFF",
          font=16, family=MONO, align="LV_TEXT_ALIGN_LEFT", ls=0, opa=255,
          clickable=False):
    """Labels need is_static / custom_attribute / attribute_type and a
    two-entry style array (DEFAULT + DISABLED) or GUI Guider renders nothing.
    `clickable` adds LV_OBJ_FLAG_CLICKABLE — labels are not hit-testable by
    default, so a nav label without it silently swallows nothing."""
    return {
        "name": name, "type": "label",
        "left": left, "top": top, "width": width, "height": height,
        "visible": True, "isLock": False,
        "flag": ["LV_OBJ_FLAG_CLICKABLE"] if clickable else [],
        "default_style": False,
        "text": text, "long_mode": "LV_LABEL_LONG_WRAP", "is_static": False,
        "custom_attribute": [], "attribute_type": "",
        "style": [
            _label_state("LV_STATE_DEFAULT", False, color, font, family, align, ls, opa),
            _label_state("LV_STATE_DISABLED", True, color, font, family, align, ls, opa),
        ],
    }


def cont(name, left, top, width, height, *, bg="#111318", radius=0,
         border_color="#000000", border_width=0,
         border_side=("LV_BORDER_SIDE_FULL",), bg_opa=255,
         bg_grad=None, bg_grad_dir="LV_GRAD_DIR_NONE"):
    return {
        "name": name, "type": "cont",
        "left": left, "top": top, "width": width, "height": height,
        "visible": True, "isLock": False, "flag": [],
        "default_style": False, "scrollbar_mode": "OFF", "child": [],
        "style": [{
            "part": "LV_PART_MAIN", "state": "LV_STATE_DEFAULT", "disable": False,
            "border_side": list(border_side), "border_width": border_width,
            "border_color": border_color, "border_opa": 255, "radius": radius,
            "bg_color": bg, "bg_grad_color": bg_grad or bg,
            "bg_grad_dir": bg_grad_dir, "bg_dither_mode": "LV_DITHER_NONE",
            "bg_main_stop": 0, "bg_grad_stop": 255, "bg_opa": bg_opa,
            "padding_top": 0, "padding_bottom": 0, "padding_left": 0,
            "padding_right": 0,
            "bg_img_src": "", "bg_img_opa": 255, "bg_img_recolor": "#ffffff",
            "bg_img_recolor_opa": 0,
            "shadow_color": "#000000", "shadow_opa": 255, "shadow_width": 0,
            "shadow_spread": 0, "shadow_ofs_x": 0, "shadow_ofs_y": 0,
        }],
    }


def bar(name, left, top, width, height, *, value, track, indicator,
        range_s=0, range_e=100, mode="LV_BAR_MODE_NORMAL", radius=0,
        track_opa=255):
    return {
        "name": name, "type": "bar",
        "left": left, "top": top, "width": width, "height": height,
        "visible": True, "isLock": False, "flag": [],
        "default_style": False, "animtime": 1000,
        "bar_value": value, "bar_start_value": 0,
        "range_s": range_s, "range_e": range_e, "anim": False, "mode": mode,
        "attribute_type": "freemaster", "custom_attribute": [],
        "style": [
            {"part": "LV_PART_MAIN", "state": "LV_STATE_DEFAULT", "disable": False,
             "bg_color": track, "bg_grad_color": track,
             "bg_grad_dir": "LV_GRAD_DIR_NONE", "bg_dither_mode": "LV_DITHER_NONE",
             "bg_main_stop": 0, "bg_grad_stop": 255, "bg_opa": track_opa,
             "radius": radius,
             "shadow_color": "#000000", "shadow_opa": 255, "shadow_width": 0,
             "shadow_spread": 0, "shadow_ofs_x": 0, "shadow_ofs_y": 0,
             "bg_img_src": "", "bg_img_opa": 255, "bg_img_recolor": "#ffffff",
             "bg_img_recolor_opa": 0},
            {"part": "LV_PART_INDICATOR", "state": "LV_STATE_DEFAULT", "disable": False,
             "bg_color": indicator, "bg_grad_color": indicator,
             "bg_grad_dir": "LV_GRAD_DIR_NONE", "bg_dither_mode": "LV_DITHER_NONE",
             "bg_main_stop": 0, "bg_grad_stop": 255, "bg_opa": 255,
             "radius": radius,
             "bg_img_src": "", "bg_img_opa": 255, "bg_img_recolor": "#ffffff",
             "bg_img_recolor_opa": 0},
        ],
    }


def led(name, left, top, size, color, bright=255):
    return {
        "name": name, "type": "led",
        "left": left, "top": top, "width": size, "height": size,
        "visible": True, "isLock": False, "flag": [],
        "default_style": False,
        "led_set_bright": bright, "led_color": color,
    }


def line(name, left, top, width, height, points, *, color, lw=2, rounded=False):
    return {
        "name": name, "type": "line",
        "left": left, "top": top, "width": width, "height": height,
        "visible": True, "isLock": False, "flag": [],
        "is_move_animation": False, "desX": 0, "desY": 0,
        "path_type": "linear", "duration": 1000,
        "default_style": False, "scrollbar_mode": "OFF",
        "line_points": [{"p1": x, "p2": y} for x, y in points],
        "style": [{
            "part": "LV_PART_MAIN", "state": "LV_STATE_DEFAULT", "disable": False,
            "line_color": color, "line_width": lw, "line_opa": 255,
            "line_rounded": rounded,
        }],
    }


def meter(name, left, top, size, *, tick_cnt, tick_len, tick_color,
          major_nth, major_len, major_color, min_value, max_value,
          arcs, angle_range=270, rotation=135, tick_width=1, major_width=2,
          label_color="#000000", label_gap=4, pad=0):
    """Tick ring + arc indicators.

    LVGL derives the scale radius from the widget: R = size/2 - pad, and ticks
    are always drawn INWARD from R ([R-len, R]). An arc indicator sits at
    R + r_mod, so a positive `pad` is what buys room for arcs OUTSIDE the tick
    ring (the Lamborghini gauge) while pad=0 keeps them inside it (Supermoto).

    LVGL also draws a numeric label at every major tick with no way to switch
    them off, so `label_color` is set to the screen background to hide them."""
    return {
        "name": name, "type": "meter",
        "left": left, "top": top, "width": size, "height": size,
        "visible": True, "isLock": False, "flag": [],
        "default_style": False,
        "attribute_type": "freemaster", "custom_attribute": [],
        "scales": [{
            "tick_cnt": tick_cnt, "tick_width": tick_width, "tick_len": tick_len,
            "tick_color": tick_color, "label_gap": label_gap,
            "major_tick_enable": True, "major_tick_nth": major_nth,
            "major_tick_width": major_width, "major_tick_len": major_len,
            "major_tick_color": major_color,
            "range_enable": True, "min_value": min_value, "max_value": max_value,
            "angle_range": angle_range, "rotation": rotation,
            "needles": [],
            "arcs": [{"width": w, "color": c, "r_mod": r,
                      "start_value": s, "end_value": e} for w, c, r, s, e in arcs],
            "scale_lines": [],
        }],
        "style": [
            {"part": "LV_PART_MAIN", "state": "LV_STATE_DEFAULT", "object": "main",
             "disable": False,
             "bg_color": "#000000", "bg_grad_color": "#000000",
             "bg_grad_dir": "LV_GRAD_DIR_NONE", "bg_opa": 0,
             "bg_main_stop": 0, "bg_grad_stop": 255, "radius": size // 2,
             "border_side": ["LV_BORDER_SIDE_FULL"], "border_width": 0,
             "border_color": "#000000", "border_opa": 255,
             "padding_top": pad, "padding_bottom": pad, "padding_left": pad,
             "padding_right": pad,
             "shadow_color": "#000000", "shadow_opa": 255, "shadow_width": 0,
             "shadow_spread": 0, "shadow_ofs_x": 0, "shadow_ofs_y": 0,
             "bg_img_src": "", "bg_img_opa": 255, "bg_img_recolor": "#ffffff",
             "bg_img_recolor_opa": 0},
            {"part": "LV_PART_MAIN", "state": "LV_STATE_FOCUSED", "object": "main",
             "disable": True,
             "bg_color": "#000000", "bg_grad_color": "#000000",
             "bg_grad_dir": "LV_GRAD_DIR_NONE", "bg_opa": 0,
             "bg_main_stop": 0, "bg_grad_stop": 255, "radius": size // 2,
             "border_side": ["LV_BORDER_SIDE_FULL"], "border_width": 0,
             "border_color": "#000000", "border_opa": 255,
             "padding_top": pad, "padding_bottom": pad, "padding_left": pad,
             "padding_right": pad,
             "shadow_color": "#000000", "shadow_opa": 255, "shadow_width": 0,
             "shadow_spread": 0, "shadow_ofs_x": 0, "shadow_ofs_y": 0,
             "bg_img_src": "", "bg_img_opa": 255, "bg_img_recolor": "#ffffff",
             "bg_img_recolor_opa": 0},
            {"part": "LV_PART_TICKS", "state": "LV_STATE_DEFAULT", "desc": "digit",
             "object": "main", "disable": False,
             "font": 11, "font_family": MONO, "text_color": label_color},
            {"part": "LV_PART_TICKS", "state": "LV_STATE_FOCUSED", "desc": "digit",
             "object": "main", "disable": True,
             "font": 11, "font_family": MONO, "text_color": label_color},
            {"part": "LV_PART_INDICATOR", "state": "LV_STATE_DEFAULT",
             "object": "main", "disable": False,
             "bg_color": "#000000", "bg_opa": 0},
        ],
    }


# ---------------------------------------------------------------------------
# navigation chrome
#
# Both screens carry the two entry points dashboard_Classic has: a VESC label
# that opens the on-device VESC Tool menu (custom_code) and a SETTINGS label
# that loads the settings screen. The SETTINGS action is COPIED from
# dashboard_Classic at build time rather than hand-written: GUI Guider's
# load_screen action embeds a `_target` snapshot of the destination screen and
# references it by its real id, so copying is the only way to get an action the
# editor resolves. See attach_nav().
# ---------------------------------------------------------------------------
NAV_VESC = "status_vesc"
NAV_SETTINGS = "Settings_text"

VESC_EVENT = {
    "widget": {
        "clicked": {
            "condition": [],
            "action": [{
                "dst": "custom_code",
                "bindCondi": "",
                "actionList": {"custom_code": {
                    "inc_c": '#include "custom.h"',
                    "code_c": "run_vesc_tool_menu();",
                    "inc_py": "",
                    "code_py": "",
                }},
            }],
        }
    }
}


def attach_nav(widgets: list[dict], settings_event: dict) -> None:
    for w in widgets:
        if w["name"] == NAV_VESC:
            w["event"] = copy.deepcopy(VESC_EVENT)
        elif w["name"] == NAV_SETTINGS:
            w["event"] = copy.deepcopy(settings_event)


# ---------------------------------------------------------------------------
# screen 1 — Lamborghini
# ---------------------------------------------------------------------------
L_BG_TOP, L_BG_BOT = "#14161B", "#050506"
L_PANEL = "#111318"
L_AMBER, L_RED, L_GREEN, L_BLUE = "#F5A623", "#C8102E", "#4ADE80", "#60A5FA"
L_LBL, L_LBL2, L_DIM, L_GRID = "#6A6F78", "#8B909A", "#3A3F48", "#1F2228"
L_OFF, L_TRACK, L_TICK, L_TICKMAJ = "#1A1C21", "#1B1E24", "#282C33", "#4A4F58"
L_WHITE, L_BARTRACK, L_FLOW, L_BOTTOM = "#FFFFFF", "#24272E", "#15171C", "#0A0B0D"

# sample telemetry, frozen from the mock-up
L_SPEED, L_SPEED_MAX = 28, 50
# Shift-light strip. The mock draws 22 LEDs, but theme_generic binds at most 12
# `speed_seg_NN` cells — 12 wider cells that actually track speed beat 22 frozen
# ones. Zones keep the mock's green -> amber -> red ramp.
L_SHIFT_N, L_SHIFT_LIT = 12, 7


def build_lambo() -> list[dict]:
    """Back-to-front widget list (Application/creation order)."""
    w: list[dict] = []

    # --- hex frame accents (SVG polygons in the mock) ----------------------
    w.append(line("frame_left", 0, 26, 132, 430,
                  [(1, 1), (96, 1), (129, 49), (129, 428), (1, 428)],
                  color=L_GRID, lw=2))
    w.append(line("frame_right", 668, 26, 132, 430,
                  [(131, 1), (36, 1), (3, 49), (3, 428), (131, 428)],
                  color=L_GRID, lw=2))
    w.append(line("accent_left", 132, 27, 70, 6,
                  [(0, 3), (68, 3)], color=L_RED, lw=2))
    w.append(line("accent_right", 598, 27, 70, 6,
                  [(2, 3), (70, 3)], color=L_RED, lw=2))

    # --- shift-light strip (speed_seg_00 = leftmost, lights first) ---------
    for i in range(L_SHIFT_N):
        if i >= L_SHIFT_LIT:
            c = L_OFF
        elif i < 6:
            c = L_GREEN
        elif i < 9:
            c = L_AMBER
        else:
            c = L_RED
        w.append(cont(f"speed_seg_{i:02d}", 91 + i * 52, 7, 48, 5, bg=c))

    # --- left wedge stack --------------------------------------------------
    LEFT = ("LV_BORDER_SIDE_LEFT",)
    w.append(cont("card_batt", 10, 74, 112, 78, bg=L_PANEL,
                  border_side=LEFT, border_width=3, border_color=L_AMBER))
    w.append(cont("card_volt", 10, 163, 112, 64, bg=L_PANEL,
                  border_side=LEFT, border_width=3, border_color=L_DIM))
    w.append(cont("card_amp", 10, 238, 112, 64, bg=L_PANEL,
                  border_side=LEFT, border_width=3, border_color=L_DIM))

    w.append(label("batt_lbl", 23, 84, 90, 14, "BATT",
                   color=L_LBL, font=11, ls=2))
    w.append(label("Battery_proc_text", 23, 98, 90, 38, "87",
                   color=L_AMBER, font=32, family=NUM))
    w.append(bar("batt_bar", 23, 138, 88, 3, value=87,
                 track=L_BARTRACK, indicator=L_AMBER))
    w.append(label("volt_lbl", 23, 173, 90, 14, "VOLTS",
                   color=L_LBL, font=11, ls=2))
    w.append(label("Voltage_text", 23, 187, 90, 36, "42.1",
                   color=L_WHITE, font=32, family=NUM))
    w.append(label("amp_lbl", 23, 248, 90, 14, "AMPS",
                   color=L_LBL, font=11, ls=2))
    w.append(label("Current_text", 23, 262, 95, 36, "31.4 A",
                   color=L_WHITE, font=32, family=NUM))

    # --- right wedge stack -------------------------------------------------
    RIGHT = ("LV_BORDER_SIDE_RIGHT",)
    w.append(cont("card_tmot", 678, 74, 112, 66, bg=L_PANEL,
                  border_side=RIGHT, border_width=3, border_color="#E8613C"))
    w.append(cont("card_tfet", 678, 151, 112, 66, bg=L_PANEL,
                  border_side=RIGHT, border_width=3, border_color=L_DIM))
    w.append(cont("card_range", 678, 228, 112, 66, bg=L_PANEL,
                  border_side=RIGHT, border_width=3, border_color=L_DIM))

    # Value + unit are separate labels: theme_generic writes bare numbers
    # ("%d" for temps, "%.1f" for range) into the *_text widgets.
    w.append(label("tmot_lbl", 689, 84, 90, 14, "MOTOR",
                   color=L_LBL, font=11, ls=2, align="LV_TEXT_ALIGN_RIGHT"))
    w.append(label("temp_mot_text", 679, 98, 84, 36, "62",
                   color="#E8613C", font=32, family=NUM,
                   align="LV_TEXT_ALIGN_RIGHT"))
    w.append(label("temp_mot_unit", 765, 98, 20, 36, "°",
                   color="#E8613C", font=32, family=NUM))
    w.append(label("tfet_lbl", 689, 161, 90, 14, "FET",
                   color=L_LBL, font=11, ls=2, align="LV_TEXT_ALIGN_RIGHT"))
    w.append(label("temp_esc_text", 679, 175, 84, 36, "48",
                   color=L_WHITE, font=32, family=NUM,
                   align="LV_TEXT_ALIGN_RIGHT"))
    w.append(label("temp_esc_unit", 765, 175, 20, 36, "°",
                   color=L_WHITE, font=32, family=NUM))
    w.append(label("range_lbl", 689, 238, 90, 14, "RANGE",
                   color=L_LBL, font=11, ls=2, align="LV_TEXT_ALIGN_RIGHT"))
    w.append(label("Range_text", 668, 252, 84, 36, "34.0",
                   color=L_WHITE, font=32, family=NUM,
                   align="LV_TEXT_ALIGN_RIGHT"))
    w.append(label("range_unit", 754, 266, 26, 16, "km",
                   color=L_LBL, font=12))

    # --- nav: one more wedge at the foot of each stack ---------------------
    w.append(cont("card_vesc", 10, 318, 112, 46, bg=L_PANEL,
                  border_side=LEFT, border_width=3, border_color=L_DIM))
    w.append(label(NAV_VESC, 10, 331, 112, 22, "VESC",
                   color=L_AMBER, font=16, ls=1,
                   align="LV_TEXT_ALIGN_CENTER", clickable=True))
    w.append(cont("card_settings", 678, 318, 112, 46, bg=L_PANEL,
                  border_side=RIGHT, border_width=3, border_color=L_DIM))
    w.append(label(NAV_SETTINGS, 678, 331, 112, 22, "SETTINGS",
                   color=L_AMBER, font=16, align="LV_TEXT_ALIGN_CENTER",
                   clickable=True))

    # --- centre gauge ------------------------------------------------------
    # Mock: centre (400,246); arc r=152 w=13; tick ring r=128..141, i.e. the
    # arc runs OUTSIDE the ticks. Box 332 with padding 25 -> scale radius 141,
    # ticks land on 128..141 exactly and the arcs go out to r_mod +11 => 152.
    w.append(meter("speed_meter", 234, 80, 332, pad=25,
                   tick_cnt=41, tick_len=7, tick_color=L_TICK,
                   major_nth=4, major_len=13, major_color=L_TICKMAJ,
                   min_value=0, max_value=L_SPEED_MAX,
                   label_color=L_BG_BOT,
                   arcs=[(13, L_TRACK, 11, 0, L_SPEED_MAX),
                         (13, L_RED, 11, 45, L_SPEED_MAX),
                         (13, L_AMBER, 11, 0, L_SPEED)]))
    # Antonio@100: line height 145, digit band sits 28..116 below the box top,
    # so the box top is (wanted digit centre - 72) and the height must be 145.
    w.append(label("Speed_text", 250, 161, 300, 145, "28",
                   color=L_WHITE, font=100, family=NUM,
                   align="LV_TEXT_ALIGN_CENTER"))
    w.append(label("speed_unit", 250, 288, 300, 18, "KM/H",
                   color=L_LBL, font=12, ls=5, align="LV_TEXT_ALIGN_CENTER"))
    # right-aligned value + left-aligned unit straddling x=400 so the pair
    # stays visually centred whatever "%.1f" kW resolves to
    w.append(label("power_value", 284, 336, 112, 18, "1.3",
                   color=L_LBL2, font=12, ls=2, align="LV_TEXT_ALIGN_RIGHT"))
    w.append(label("power_unit", 400, 336, 60, 18, "kW",
                   color=L_LBL2, font=12, ls=2))

    # --- power flow --------------------------------------------------------
    w.append(label("flow_regen_lbl", 190, 396, 120, 14, "REGEN",
                   color=L_LBL, font=11, ls=2))
    w.append(label("flow_power_lbl", 490, 396, 120, 14, "POWER",
                   color=L_AMBER, font=11, ls=2, align="LV_TEXT_ALIGN_RIGHT"))
    w.append(bar("power_bar", 190, 415, 420, 9, value=26,
                 range_s=-100, range_e=100, mode="LV_BAR_MODE_SYMMETRICAL",
                 track=L_FLOW, indicator=L_AMBER))
    w.append(cont("flow_center", 400, 413, 1, 13, bg=L_TICKMAJ))

    # --- bottom bar --------------------------------------------------------
    w.append(cont("bottom_bar", 0, 436, 800, 44, bg=L_BOTTOM,
                  border_side=("LV_BORDER_SIDE_TOP",), border_width=1,
                  border_color=L_GRID))
    # "status_bt" is the BLE telltale caption — theme_generic recolours it.
    for nm, x, col, bright, cap in (
            ("tt_cruise_lbl", 22, L_GREEN, 255, "CRZ"),
            ("tt_fault_lbl", 84, L_AMBER, 40, "FLT"),
            ("status_bt", 146, L_BLUE, 255, "BT"),
    ):
        w.append(led(f"led_{nm}", x, 451, 14, col, bright))
        w.append(label(nm, x + 18, 452, 36, 13, cap,
                       color=col if bright == 255 else L_DIM, font=11, ls=1))

    w.append(label("trip_lbl", 380, 444, 60, 13, "TRIP",
                   color=L_LBL, font=11, ls=2))
    w.append(label("TRIP_text", 380, 457, 34, 18, "4.2",
                   color=L_WHITE, font=12))
    w.append(label("trip_unit", 416, 457, 30, 18, "KM",
                   color=L_LBL, font=12))
    w.append(label("odo_lbl", 476, 444, 60, 13, "ODO",
                   color=L_LBL, font=11, ls=2))
    w.append(label("odo_text", 476, 457, 48, 18, "00312",
                   color=L_WHITE, font=12))
    w.append(label("odo_unit", 526, 457, 30, 18, "KM",
                   color=L_LBL, font=12))
    w.append(label("mode_text", 578, 450, 100, 18, "MODE 1",
                   color=L_AMBER, font=12, ls=2))
    w.append(label("cur_time_label", 688, 447, 90, 22, "14:32",
                   color=L_WHITE, font=16, align="LV_TEXT_ALIGN_RIGHT"))
    return w


# ---------------------------------------------------------------------------
# screen 2 — Supermoto
# ---------------------------------------------------------------------------
M_BG, M_PANEL, M_BORDER = "#0A0B0C", "#111315", "#1E2124"
M_GREEN, M_AMBER, M_RED, M_BLUE = "#4ADE80", "#F5A623", "#EF4444", "#38BDF8"
M_LBL, M_LBL2, M_DIM = "#6A6F78", "#8B909A", "#5F646C"
M_FACE, M_RING, M_TICK, M_TICKMAJ = "#0D0F11", "#17191C", "#23262B", "#4A4F58"
M_WHITE, M_OFF = "#FFFFFF", "#1E2124"

M_SPEED, M_SPEED_MAX = 28, 50
# 14 battery cells (theme_generic's batt_seg cap), laid out so the leftmost is
# batt_seg_13: paint_v_bar lights the HIGHEST indices first, which turns into a
# left-to-right fill for a horizontal row.
M_CELLS = 14
M_CELL_PCT = 87


def build_moto() -> list[dict]:
    w: list[dict] = []

    # --- top telltale rail -------------------------------------------------
    w.append(cont("rail", 0, 0, 800, 38, bg=M_PANEL,
                  border_side=("LV_BORDER_SIDE_BOTTOM",), border_width=2,
                  border_color=M_BORDER))
    for nm, x, col, bright, cap in (
            ("tt_cruise_lbl", 16, M_GREEN, 255, "CRZ"),
            ("status_bt", 74, M_BLUE, 255, "BT"),
            ("tt_fault_lbl", 132, M_RED, 40, "FLT"),
            ("tt_cfg_lbl", 190, M_AMBER, 40, "CFG"),
    ):
        w.append(led(f"led_{nm}", x, 12, 14, col, bright))
        w.append(label(nm, x + 18, 13, 36, 13, cap,
                       color=col if bright == 255 else "#4A5158", font=11, ls=1))
    w.append(label("rail_info", 280, 12, 300, 16, "VESC / CAN 500K / ID 2",
                   color=M_LBL, font=12, ls=1, align="LV_TEXT_ALIGN_CENTER"))
    w.append(label("cur_time_label", 690, 9, 94, 22, "14:32",
                   color=M_WHITE, font=16, align="LV_TEXT_ALIGN_RIGHT"))

    # --- tach --------------------------------------------------------------
    # Mock: centre (181,219); ring r=132 w=20; tick ring r=142..152 (OUTSIDE
    # the ring); face r=108. Box 308 with padding 2 -> scale radius 152, so the
    # ticks land on 142..152 and the ring sits at r_mod -20 => 132.
    # Value arc LAST: theme_generic binds the highest-numbered arc as the live
    # one, and drawing it last also keeps it over the track.
    w.append(meter("speed_meter", 27, 65, 308, pad=2,
                   tick_cnt=51, tick_len=10, tick_color=M_TICK,
                   major_nth=5, major_len=12, major_color=M_TICKMAJ,
                   min_value=0, max_value=M_SPEED_MAX,
                   label_color=M_BG,
                   arcs=[(20, M_RING, -20, 0, M_SPEED_MAX),
                         (3, M_RED, -4, 43, M_SPEED_MAX),
                         (20, M_GREEN, -20, 0, M_SPEED)]))
    w.append(cont("tach_face", 73, 111, 216, 216, bg=M_FACE, radius=108,
                  border_width=1, border_color=M_BORDER))
    w.append(label("tach_max", 31, 152, 300, 14, "MAX 46",
                   color=M_DIM, font=11, ls=2, align="LV_TEXT_ALIGN_CENTER"))
    # digit band = box top + 28..116 (Antonio@100); centred on the face (y=219)
    w.append(label("Speed_text", 31, 147, 300, 145, "28",
                   color=M_WHITE, font=100, family=NUM,
                   align="LV_TEXT_ALIGN_CENTER"))
    w.append(label("speed_unit", 31, 268, 300, 22, "KM/H",
                   color=M_AMBER, font=16, ls=4, align="LV_TEXT_ALIGN_CENTER"))

    # --- battery hero ------------------------------------------------------
    w.append(cont("batt_card", 364, 54, 420, 112, bg=M_PANEL, radius=5,
                  border_width=2, border_color=M_BORDER))
    w.append(label("batt_lbl", 379, 68, 160, 14, "BATTERY",
                   color=M_LBL2, font=11, ls=2))
    w.append(label("Battery_proc_text", 604, 53, 130, 58, "87",
                   color=M_GREEN, font=40, family=NUM,
                   align="LV_TEXT_ALIGN_RIGHT"))
    w.append(label("batt_pct_unit", 738, 68, 32, 26, "%",
                   color=M_GREEN, font=20))
    # position p (0 = leftmost) carries index 13-p, so the bar fills left->right
    lit = (M_CELL_PCT * M_CELLS + 50) // 100
    for p in range(M_CELLS):
        idx = M_CELLS - 1 - p
        w.append(cont(f"batt_seg_{idx:02d}", 379 + round(p * 28.08), 102, 25, 22,
                      bg=M_GREEN if idx >= M_CELLS - lit else M_OFF, radius=1))
    w.append(label("Voltage_text", 379, 134, 58, 16, "42.1",
                   color=M_LBL2, font=12))
    w.append(label("volt_unit", 411, 134, 20, 16, "V",
                   color=M_LBL2, font=12))
    w.append(label("Current_text", 519, 134, 110, 16, "31.4 A",
                   color=M_LBL2, font=12, align="LV_TEXT_ALIGN_CENTER"))
    w.append(label("power_value", 659, 134, 76, 16, "1.3",
                   color=M_LBL2, font=12, align="LV_TEXT_ALIGN_RIGHT"))
    w.append(label("power_unit", 739, 134, 30, 16, "kW",
                   color=M_LBL2, font=12))

    # --- temps -------------------------------------------------------------
    w.append(cont("temp_m_card", 364, 178, 204, 88, bg=M_PANEL, radius=5,
                  border_width=2, border_color=M_BORDER))
    w.append(cont("temp_f_card", 580, 178, 204, 88, bg=M_PANEL, radius=5,
                  border_width=2, border_color=M_BORDER))
    w.append(label("temp_m_lbl", 377, 189, 150, 14, "MOTOR",
                   color=M_LBL, font=11, ls=2))
    w.append(label("temp_mot_text", 377, 204, 44, 40, "62",
                   color=M_AMBER, font=32, family=NUM))
    w.append(label("temp_mot_unit", 407, 204, 24, 40, "°",
                   color=M_AMBER, font=32, family=NUM))
    w.append(bar("temp_mot_bar", 377, 248, 178, 5, value=62,
                 track=M_BORDER, indicator=M_AMBER))
    w.append(label("temp_f_lbl", 593, 189, 150, 14, "FET",
                   color=M_LBL, font=11, ls=2))
    w.append(label("temp_esc_text", 593, 204, 44, 40, "48",
                   color=M_GREEN, font=32, family=NUM))
    w.append(label("temp_esc_unit", 623, 204, 24, 40, "°",
                   color=M_GREEN, font=32, family=NUM))
    w.append(bar("temp_esc_bar", 593, 248, 178, 5, value=48,
                 track=M_BORDER, indicator=M_GREEN))

    # --- power flow --------------------------------------------------------
    w.append(cont("flow_card", 364, 278, 420, 76, bg=M_PANEL, radius=5,
                  border_width=2, border_color=M_BORDER))
    w.append(label("flow_regen_lbl", 377, 289, 120, 14, "REGEN",
                   color=M_LBL, font=11, ls=2))
    w.append(label("flow_drive_lbl", 651, 289, 120, 14, "DRIVE",
                   color=M_LBL, font=11, ls=2, align="LV_TEXT_ALIGN_RIGHT"))
    w.append(cont("flow_track", 376, 310, 396, 20, bg=M_BG,
                  border_width=1, border_color=M_BORDER))
    w.append(bar("power_bar", 378, 312, 392, 16, value=28,
                 range_s=-100, range_e=100, mode="LV_BAR_MODE_SYMMETRICAL",
                 track=M_BG, indicator=M_AMBER, track_opa=0))
    w.append(cont("flow_center", 573, 312, 2, 16, bg="#3A3F48"))

    # --- nav: two cards under the tach, on the trip-strip baseline ---------
    for key, x, text in (("vesc", 28, "VESC"), ("settings", 194, "SETTINGS")):
        w.append(cont(f"card_{key}", x, 366, 150, 66, bg=M_PANEL, radius=5,
                      border_width=2, border_color=M_BORDER))
        w.append(label(NAV_VESC if key == "vesc" else NAV_SETTINGS,
                       x, 388, 150, 24, text, color=M_GREEN, font=20, ls=1,
                       align="LV_TEXT_ALIGN_CENTER", clickable=True))

    # --- trip / odo / mode -------------------------------------------------
    for key, x, border, col, cap, value, unit in (
            ("trip", 364, M_BORDER, M_WHITE, "TRIP", ("TRIP_text", "4.2", 48), "KM"),
            ("odo", 508, M_BORDER, M_WHITE, "ODO", ("odo_text", "00312", 60), "KM"),
            ("mode", 652, M_GREEN, M_GREEN, "DRIVE", ("mode_text", "MODE 1", 110), None),
    ):
        vname, vtext, vwidth = value
        w.append(cont(f"{key}_card", x, 366, 132, 66, bg=M_PANEL, radius=5,
                      border_width=2, border_color=border))
        w.append(label(f"{key}_lbl", x + 12, 377, 110, 14, cap,
                       color=col if key == "mode" else M_LBL, font=11, ls=2))
        w.append(label(vname, x + 12, 394, vwidth, 30, vtext,
                       color=col, font=22, family=NUM))
        if unit:
            w.append(label(f"{key}_unit", x + 14 + vwidth, 402, 30, 18, unit,
                           color=M_LBL, font=12))
    return w


# ---------------------------------------------------------------------------
# screen assembly
# ---------------------------------------------------------------------------
def screen_style(bg: str, grad: str | None = None) -> list[dict]:
    return [{
        "part": "LV_PART_MAIN", "state": "LV_STATE_DEFAULT", "disable": False,
        "bg_color": bg,
        "bg_grad_dir": "LV_GRAD_DIR_VER" if grad else "LV_GRAD_DIR_NONE",
        "bg_dither_mode": "LV_DITHER_NONE",
        "bg_main_stop": 0, "bg_grad_stop": 255,
        "bg_grad_color": grad or bg, "bg_opa": 255,
        "bg_img_src": "", "bg_img_opa": 0, "bg_img_recolor": "#000000",
        "bg_img_recolor_opa": 0,
    }]


def to_app_widget(w: dict, screen: str) -> dict:
    """FrontJson widget -> Application widget: prefixed name, pos/size instead
    of left/top/width/height, no editor-only `isLock`."""
    a = {"name": f"{screen}_{w['name']}", "id": w["id"], "type": w["type"],
         "visible": w["visible"], "flag": list(w.get("flag", [])),
         "pos": [w["left"], w["top"]], "size": [w["width"], w["height"]]}
    skip = {"name", "id", "type", "visible", "flag",
            "left", "top", "width", "height", "isLock"}
    for k, v in w.items():
        if k not in skip:
            a[k] = copy.deepcopy(v)
    return a


def make_screen(name: str, widgets: list[dict], screen_id: str,
                style: list[dict]) -> tuple[dict, dict]:
    """`widgets` is back-to-front. Application keeps that order (== creation
    order); FrontJson gets the reverse (list[0] is the topmost layer)."""
    common = {
        "name": name, "id": screen_id, "type": "scr", "version": 190,
        "scrollbar_mode": "OFF",
        "customer_code": {"code_c": "", "inc_c": "", "code_py": "",
                          "inc_py": "", "type": "screen"},
        "width": 800, "height": 480, "visible": True, "flag": [],
    }
    front = dict(common)
    front["list"] = [copy.deepcopy(w) for w in reversed(widgets)]
    front["event"] = {"widget": {}}
    front["style"] = copy.deepcopy(style)

    app = dict(common)
    app["event"] = {"widget": {}}
    app["style"] = copy.deepcopy(style)
    app["size"] = [800, 480]
    app["widgets"] = [to_app_widget(w, name) for w in widgets]
    return front, app


# ---------------------------------------------------------------------------
# id allocation
# ---------------------------------------------------------------------------
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


def assign_ids(widgets: list[dict], prefix: str, used: set[str]) -> None:
    n = 0
    for w in widgets:
        while True:
            n += 1
            v = f"{prefix}{n:06d}"
            if v not in used:
                break
        used.add(v)
        w["id"] = v


# ---------------------------------------------------------------------------
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

    # idempotent re-run
    app_screens[:] = [s for s in app_screens if s.get("name") not in OWNED]
    front_screens[:] = [s for s in front_screens if s.get("name") not in OWNED]

    used: set[str] = set()
    collect_ids(proj, used)
    print(f"[i] {len(used)} existing ids collected")

    # The settings nav action, lifted from dashboard_Classic. Its embedded
    # `_target` carries the settings screen's real ids — assign_ids() only
    # touches the top-level widget ids, so those stay intact.
    settings_event = None
    src = next((s for s in front_screens if s["name"] == "dashboard_Classic"), None)
    if src:
        w = next((x for x in src["list"] if x["name"] == NAV_SETTINGS), None)
        if w and w.get("event"):
            settings_event = w["event"]
    if settings_event is None:
        print(f"[err] no '{NAV_SETTINGS}' load_screen event on dashboard_Classic "
              f"to copy — the new screens would have no way back to settings",
              file=sys.stderr)
        return 1

    for name, prefix, builder, style in (
        (LAMBO, "lb", build_lambo, screen_style(L_BG_TOP, L_BG_BOT)),
        (MOTO, "mt", build_moto, screen_style(M_BG)),
    ):
        widgets = builder()
        attach_nav(widgets, settings_event)
        assign_ids(widgets, prefix, used)
        scr_id = f"{prefix}scr001"
        while scr_id in used:
            scr_id += "x"
        used.add(scr_id)
        front, app = make_screen(name, widgets, scr_id, style)
        front_screens.append(front)
        app_screens.append(app)
        print(f"[ok] '{name}': {len(widgets)} widgets, id={scr_id}")

    with open(PROJECT, "w", encoding="utf-8") as f:
        json.dump(proj, f, ensure_ascii=False, indent="\t")
    print(f"[ok] {PROJECT}")
    print()
    print("next, in GUI Guider (it caches the project — close WITHOUT saving first):")
    print("  1. File -> Open Project -> Super_VESC_Display.guiguider")
    print(f"  2. '{LAMBO}' / '{MOTO}' appear in the screen list")
    print("  3. Generate Code -> generated/setup_scr_dashboard_{Lambo,Moto}.c")
    print("  4. python3 tools/regen_cockpit_fonts.py   # re-subset Antonio (incl. @100)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
