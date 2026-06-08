#!/usr/bin/env python3
"""Polish pass for the `dashboard_amber` screen (run AFTER build_amber_dashboard.py).

Two fixes, applied consistently to all three representations so the GUI Guider
editor canvas, a future "Generate Code", and the already-committed
setup_scr_dashboard_amber.c (what the simulator/firmware actually compile) stay
in sync:

  1. FONT FIT — the bottom telemetry row inherited DSEG7 size 40 from the
     cockpit's Antonio_40. DSEG7 is ~33 px/digit at size 40, so a 4-digit odo
     ("6436") overflows its 100 px cell and LV_LABEL_LONG_WRAP drops the 4th
     digit onto a second line below the panel. DSEG7_24 is ~19.6 px/digit
     (5.1 digits / 100 px) -> fits a 5-digit odo. Also switch those cells to
     LV_LABEL_LONG_CLIP so a 6-digit extreme clips horizontally instead of
     wrapping under the dashboard.

  2. NEON GLOW — amber halo around digits and text via LVGL's shadow_* styles
     (needs LV_DRAW_COMPLEX=1; it is, in all three lv_conf). LVGL has no
     per-glyph text glow, so this is a soft halo around each label's box;
     reads as a backlit-cell neon on the dark background. Segment bars,
     separators, backgrounds and empty labels are excluded.

Idempotent: re-running detects already-applied values and reports no-ops.
Perf note: LV_SHADOW_CACHE_SIZE=0, so the glow on frequently-updated value
labels is recomputed every redraw -> watch fps on-device; widths are kept
modest and tier-scaled. Dial GLOW_TIERS down (or drop VALUE/LABEL tiers) if
the P4 chugs.
"""
import json, os, re, sys

# Glow is OFF by default for now: LVGL's shadow_* glows the widget BOX, not the
# glyphs, so on loose label boxes it reads as a glowing frame. Re-enable with
# --glow once the boxes are tightened to hug the text. Font-fit always applies.
ENABLE_GLOW = "--glow" in sys.argv

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GUI  = os.path.join(ROOT, "Super_VESC_Display.guiguider")
CSRC = os.path.join(ROOT, "generated", "setup_scr_dashboard_amber.c")
SCREEN = "dashboard_amber"
PREFIX = "dashboard_amber_"

GLOW_COLOR = "#FF7A1A"          # amber neon (theme AMBER_ACCENT)
GLOW_COLOR_HEX = 0xFF7A1A

# Bottom telemetry row: shrink DSEG7 40 -> 24 and clip instead of wrap.
FONT_24 = {
    "Voltage_text", "TRIP_text", "odo_text",
    "temp_mot_text", "temp_esc_text", "col_avg_value",
}

# Glow tiers: name -> (shadow_width, shadow_opa). Boxes that loosely bound their
# text (the big hero readouts) get LOW opa so the cell doesn't read as a filled
# block; tight value/label boxes get a brighter halo.
HERO  = {"Speed_text", "Battery_proc_text", "power_value"}
VALUE = {
    "Voltage_text", "TRIP_text", "odo_text", "temp_mot_text", "temp_esc_text",
    "col_avg_value", "uptime_text", "cur_time_label", "Ah_text", "Current_text",
    "Range_text", "power_max_val", "Speed_cc_text", "speed_max",
}
LABEL = {
    "status_vesc", "mode_text", "statistics_button", "Settings_text", "status_bt",
    "battery_label", "power_label", "speed_label", "battery_pct", "power_unit",
    "battery_range_label", "power_max_label", "col_voltage_label", "col_trip_label",
    "col_odo_label", "col_mtmp_label", "col_ctmp_label", "col_avg_label",
    "col_v_label", "col_trip_unit", "col_odo_unit", "col_mtmp_unit",
    "col_ctmp_unit", "col_avg_unit", "speed_min",
}

def glow_for(name):
    if name in HERO:  return (20, 110)
    if name in VALUE: return (12, 160)
    if name in LABEL: return (7, 150)
    return None

GLOW_WIDGETS = HERO | VALUE | LABEL


# ───────────────────────── .guiguider ─────────────────────────
def patch_style_entry(st, name):
    """Mutate the LV_STATE_DEFAULT style entry; return list of change strings."""
    ch = []
    for e in st:
        if e.get("state") != "LV_STATE_DEFAULT":
            continue
        if name in FONT_24 and e.get("font") != 24:
            e["font"] = 24; ch.append("font->24")
        g = glow_for(name) if ENABLE_GLOW else None
        if g:
            w, opa = g
            if e.get("shadow_width") != w:        e["shadow_width"] = w; ch.append(f"sw->{w}")
            if e.get("shadow_color") != GLOW_COLOR: e["shadow_color"] = GLOW_COLOR; ch.append("scol")
            if e.get("shadow_opa") != opa:        e["shadow_opa"] = opa; ch.append(f"sopa->{opa}")
            if e.get("shadow_spread") != 0:       e["shadow_spread"] = 0; ch.append("sspr->0")
    return ch

def walk_front(items, hits):
    for it in items:
        nm = it.get("name")
        if it.get("type") == "label" and nm in (FONT_24 | GLOW_WIDGETS):
            st = it.get("style")
            if isinstance(st, list):
                if patch_style_entry(st, nm): hits.add(nm)
            if nm in FONT_24:
                it["long_mode"] = "LV_LABEL_LONG_CLIP"
        ch = it.get("list") or it.get("children")
        if ch: walk_front(ch, hits)

def walk_app(items, hits):
    for it in items:
        nm = it.get("name")
        base = nm[len(PREFIX):] if nm and nm.startswith(PREFIX) else nm
        if it.get("type") == "label" and base in (FONT_24 | GLOW_WIDGETS):
            st = it.get("style")
            if isinstance(st, list):
                if patch_style_entry(st, base): hits.add(base)
            if base in FONT_24:
                it["long_mode"] = "LV_LABEL_LONG_CLIP"
        ch = it.get("widgets") or it.get("list") or it.get("children")
        if ch: walk_app(ch, hits)

def patch_guiguider():
    with open(GUI) as f:
        d = json.load(f)
    fhits, ahits = set(), set()
    for s in d.get("FrontJson", []):
        if s.get("name") == SCREEN:
            walk_front(s.get("list", []), fhits)
    for s in d.get("Application", {}).get("screen", []):
        if s.get("name") == SCREEN:
            walk_app(s.get("widgets", []), ahits)
    with open(GUI, "w") as f:
        json.dump(d, f, ensure_ascii=False, indent=1)
    print(f"[guiguider] FrontJson touched {len(fhits)} widgets, Application {len(ahits)} "
          f"(glow {'ON' if ENABLE_GLOW else 'OFF'})")
    expect = (FONT_24 | GLOW_WIDGETS) if ENABLE_GLOW else FONT_24
    miss = expect - fhits
    if miss: print(f"  [warn] expected but not touched in FrontJson (may be already-applied no-ops): {sorted(miss)}")
    return fhits, ahits


# ───────────────────────── setup_scr_dashboard_amber.c ─────────────────────────
def patch_c():
    with open(CSRC) as f:
        src = f.read()
    if "lv_font_DSEG7Classic_Regular_24" not in src:
        sys.exit("[c] ERROR: DSEG7_24 not referenced in .c — font not declared, abort")
    changes = 0

    # 1) force DSEG7 -> size 24 (regardless of native 40/32) + long_mode CLIP
    for name in FONT_24:
        w = PREFIX + name
        pat = re.compile(
            rf"(lv_obj_set_style_text_font\(ui->{re.escape(w)}, &lv_font_DSEG7Classic_Regular_)\d+(,)")
        src, n = pat.subn(r"\g<1>24\g<2>", src)
        changes += n
        lm_a = f"lv_label_set_long_mode(ui->{w}, LV_LABEL_LONG_WRAP);"
        lm_b = f"lv_label_set_long_mode(ui->{w}, LV_LABEL_LONG_CLIP);"
        if lm_a in src:
            src = src.replace(lm_a, lm_b); changes += 1

    # 2) glow: expand the single shadow_width(...,0,...) line into a 4-line glow
    for name in (GLOW_WIDGETS if ENABLE_GLOW else ()):
        w = PREFIX + name
        g = glow_for(name)
        if not g:
            continue
        wd, opa = g
        anchor = f"    lv_obj_set_style_shadow_width(ui->{w}, 0, LV_PART_MAIN|LV_STATE_DEFAULT);"
        if anchor not in src:
            # already patched (idempotent) or unexpected layout
            continue
        repl = (
            f"    lv_obj_set_style_shadow_width(ui->{w}, {wd}, LV_PART_MAIN|LV_STATE_DEFAULT);\n"
            f"    lv_obj_set_style_shadow_color(ui->{w}, lv_color_hex(0x{GLOW_COLOR_HEX:06X}), LV_PART_MAIN|LV_STATE_DEFAULT);\n"
            f"    lv_obj_set_style_shadow_opa(ui->{w}, {opa}, LV_PART_MAIN|LV_STATE_DEFAULT);\n"
            f"    lv_obj_set_style_shadow_spread(ui->{w}, 0, LV_PART_MAIN|LV_STATE_DEFAULT);"
        )
        src = src.replace(anchor, repl); changes += 1

    with open(CSRC, "w") as f:
        f.write(src)
    print(f"[c] applied {changes} edits to {os.path.basename(CSRC)}")


if __name__ == "__main__":
    patch_guiguider()
    patch_c()
    # validate JSON round-trips
    with open(GUI) as f:
        json.load(f)
    print("[ok] .guiguider valid JSON; polish pass complete")
