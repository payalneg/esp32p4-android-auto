#!/usr/bin/env python3
"""Generate auto-registration for convention-named GUI Guider dashboard screens.

Scans a generated ``gui_guider.h`` for every ``setup_scr_dashboard_<name>``
screen (the ``dashboard_`` prefix + a suffix) and emits a C file that registers
each one as a *generic* dashboard theme (see Super_VESC_Display/custom/
theme_generic.{h,c}). The generic theme binds live VESC data to the screen's
widgets purely by naming convention, so adding a ``dashboard_<something>``
screen in GUI Guider + rebuilding makes it appear in the Settings theme
dropdown with working data — no hand-written C.

Hand-written themes are skipped (see SKIP): the base ``dashboard`` screen is the
cockpit theme (no suffix, so never matched) and ``dashboard_amber`` has its own
ops module. dashboard_theme_register() also de-dupes, so a double registration
would be harmless anyway.

Run from the build system (components/vesc_ui/CMakeLists.txt); regenerates
whenever gui_guider.h changes.
"""

import argparse
import re
import sys

# Screens with a bespoke, hand-written theme — do not auto-register these.
# (The cockpit screen was renamed dashboard -> dashboard_Classic and the amber
# one dashboard_amber -> dashboard_Amber in GUI Guider; both match dashboard_*
# now, so both must be skipped. Old lowercase kept for safety across regens.)
SKIP = {"dashboard_Classic", "dashboard_Amber", "dashboard_amber"}

# (dashboard_widgets_t member, gui_guider.h field suffix). The field name is
# "<screen>_<suffix>"; emitted only when that field exists on the screen.
SCALARS = [
    ("speed_text",      "Speed_text"),
    ("current_text",    "Current_text"),
    ("voltage_text",    "Voltage_text"),
    ("batt_pct_text",   "Battery_proc_text"),
    ("temp_fet_text",   "temp_esc_text"),
    ("temp_motor_text", "temp_mot_text"),
    ("trip_text",       "TRIP_text"),
    ("range_text",      "Range_text"),
    ("odo_text",        "odo_text"),
    ("ah_text",         "Ah_text"),
    ("uptime_text",     "uptime_text"),
    ("mode_text",       "mode_text"),
    ("time_label",      "cur_time_label"),
    ("power_value",     "power_value"),
    ("max_power_text",  "max_power_text"),
    ("max_speed_text",  "max_speed_text"),
    ("power_max_val",   "power_max_val"),
    ("status_bt",       "status_bt"),
    # "ESC NOT CONNECTED" banner + the STATISTICS label it overlaps. Screens
    # cloned from Classic carry both; screens without a banner get one created at
    # runtime instead (see theme_generic.c).
    ("esc_warn_text",     "esc_not_connected_text"),
    ("statistics_button", "statistics_button"),
    # "°C" labels next to the temperatures — only so a dual-head reading
    # ("34/37") can push them aside. Two naming variants in the wild.
    ("temp_fet_unit",   "temp_esc_unit"),
    ("temp_fet_unit",   "col_ctmp_unit"),
    ("temp_motor_unit", "temp_mot_unit"),
    ("temp_motor_unit", "col_mtmp_unit"),
    # Analogue gauges (plain lv_obj_t *; the meter's scale/arc need their own
    # types and are emitted by emit_speed_gauge()).
    ("speed_meter",     "speed_meter"),
    ("batt_bar",        "batt_bar"),
    ("temp_mot_bar",    "temp_mot_bar"),
    ("temp_esc_bar",    "temp_esc_bar"),
    ("power_bar",       "power_bar"),
    # Not a render target: the invisible brightness drag slider, so the
    # "brightness gesture" setting can hide it on this screen too.
    ("brightness_slider", "brightness_slider"),
]
# (member array, field-suffix stem, max count). Counts contiguous _00.._NN.
SEGMENTS = [
    ("batt_seg",  "batt_seg",  14),
    ("power_seg", "power_seg", 14),
    ("speed_seg", "speed_seg", 12),
]


def emit_speed_gauge(out, screen, fields):
    """Wire the lv_meter speed gauge. GUI Guider names a meter's parts
    `<meter>_scale_0`, `<meter>_scale_0_arc_0`, `_arc_1`, ... — different C
    types (lv_meter_scale_t * / lv_meter_indicator_t *), so they can't ride
    along in SCALARS. By convention the LAST arc is the one that tracks speed;
    that is also the one a screen must author last so it draws over the track
    and redline arcs."""
    scale = f"{screen}_speed_meter_scale_0"
    if scale not in fields:
        return
    arcs = sorted(int(m.group(1)) for f in fields
                  if (m := re.fullmatch(re.escape(scale) + r"_arc_(\d+)", f)))
    if not arcs:
        return
    out.append(f"    w->speed_scale = guider_ui.{scale};")
    out.append(f"    w->speed_arc = guider_ui.{scale}_arc_{arcs[-1]};")


def parse(header_text):
    """Return (screens, fields): screen base names and the set of all field ids."""
    screens = re.findall(r"void\s+setup_scr_(dashboard_[A-Za-z0-9_]+)\s*\(", header_text)
    # Any lv_*_t pointer, not just lv_obj_t: a meter contributes
    # lv_meter_scale_t * / lv_meter_indicator_t * members that emit_speed_gauge
    # needs to see. Lookups are by exact name, so extra entries are harmless.
    fields = set(re.findall(r"lv_[A-Za-z0-9_]+_t\s*\*\s*([A-Za-z0-9_]+)\s*;", header_text))
    # De-dup, keep discovery order stable.
    seen, ordered = set(), []
    for s in screens:
        if s not in seen:
            seen.add(s)
            ordered.append(s)
    return ordered, fields


def emit_screen(out, screen, fields):
    suffix = screen[len("dashboard_"):]          # e.g. "BlaBlaBla"
    ident = re.sub(r"[^A-Za-z0-9_]", "_", suffix)  # safe C identifier
    name = suffix.replace("_", " ")                # dropdown label
    out.append(f"/* ---- {screen} ---- */")
    out.append(f"static dashboard_widgets_t s_w_{ident};")
    out.append(f"static lv_obj_t *create_{ident}(void) {{")
    out.append(f"    setup_scr_{screen}(&guider_ui);")
    out.append(f"    dashboard_widgets_t *w = &s_w_{ident};")
    out.append("    *w = (dashboard_widgets_t){0};")
    # Theme root — parent for the runtime-created "ESC NOT CONNECTED" banner.
    out.append(f"    w->screen = guider_ui.{screen};")
    for member, sfx in SCALARS:
        if f"{screen}_{sfx}" in fields:
            out.append(f"    w->{member} = guider_ui.{screen}_{sfx};")
    for member, stem, maxn in SEGMENTS:
        n = 0
        while n < maxn and f"{screen}_{stem}_{n:02d}" in fields:
            n += 1
        for i in range(n):
            out.append(f"    w->{member}[{i}] = guider_ui.{screen}_{stem}_{i:02d};")
        if n:
            out.append(f"    w->{member}_n = {n};")
    emit_speed_gauge(out, screen, fields)
    out.append("    dashboard_generic_set_active(w);")
    out.append(f"    return guider_ui.{screen};")
    out.append("}")
    out.append(f"static const dashboard_theme_t theme_{ident} = {{")
    out.append(f'    .id = "{suffix}",')
    out.append(f'    .name = "{name}",')
    out.append(f"    .create = create_{ident},")
    out.append("    .ops = &dashboard_generic_ops,")
    out.append("};")
    out.append("")
    return ident


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--header", required=True, help="path to generated gui_guider.h")
    ap.add_argument("--out", required=True, help="output .c path")
    args = ap.parse_args()

    try:
        with open(args.header, "r", encoding="utf-8", errors="replace") as f:
            header_text = f.read()
    except OSError as e:
        print(f"gen_dashboard_themes: cannot read {args.header}: {e}", file=sys.stderr)
        return 1

    screens, fields = parse(header_text)
    discovered = [s for s in screens if s not in SKIP]

    out = [
        "/* AUTO-GENERATED by scripts/gen_dashboard_themes.py — DO NOT EDIT.",
        " * Registers a generic, convention-bound theme for every GUI Guider",
        " * screen named dashboard_* (except hand-written themes). Regenerated",
        " * from generated/gui_guider.h on every build. See theme_generic.h. */",
        '#include "gui_guider.h"',
        '#include "dashboard_theme.h"',
        '#include "theme_generic.h"',
        "",
    ]
    idents = [emit_screen(out, s, fields) for s in discovered]

    out.append("void dashboard_themes_auto_register_all(void) {")
    if idents:
        for ident in idents:
            out.append(f"    dashboard_theme_register(&theme_{ident});")
    else:
        out.append("    /* no convention-named dashboard_* screens discovered */")
    out.append("}")
    out.append("")

    text = "\n".join(out)
    try:
        with open(args.out, "w", encoding="utf-8") as f:
            f.write(text)
    except OSError as e:
        print(f"gen_dashboard_themes: cannot write {args.out}: {e}", file=sys.stderr)
        return 1

    print(f"gen_dashboard_themes: {len(discovered)} screen(s) -> {args.out}: "
          + (", ".join(discovered) if discovered else "(none)"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
