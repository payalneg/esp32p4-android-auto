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
  * Fonts are NOT touched — the speed readout in Antonio 200 needs about
    270 px for three digits and still fits. What does not survive a 0.6 squeeze
    is text: "ESC NOT CONNECTED" at font 22 needs ~205 px and its box would
    become 144. So a label keeps whatever width its text actually needs (up to
    the screen edge), growing away from its alignment anchor: a right-aligned
    label grows leftwards, a centred one grows both ways. Labels that end up
    overlapping after that are listed at the end — an overlap is a design
    decision (shorter text, smaller font, a different spot) rather than
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

# Rough advance width of one character as a fraction of the font size. Used
# only to flag labels whose text probably no longer fits; Montserrat/Antonio
# digits and capitals land around 0.5-0.6 em.
CHAR_EM = 0.55


class Geometry:
    """The new geometry of one widget, plus what to do with its children."""

    def __init__(self, x, y, w, h, child_ratio):
        self.x, self.y, self.w, self.h = x, y, w, h
        self.child_ratio = child_ratio


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


def text_width_estimate(text, font_size):
    """Widest line of `text`, in pixels, roughly."""
    if not text or not font_size:
        return 0
    longest = max((len(line) for line in text.split("\n")), default=0)
    return int(longest * font_size * CHAR_EM)


def widen_for_text(g, need, align, screen_w):
    """Give a label back the width its text needs, growing away from whichever
    edge its alignment pins the text to."""
    if need <= g.w:
        return
    want = min(need, screen_w)
    if align == "RIGHT":
        right = g.x + g.w
        g.x = max(0, right - want)
        g.w = min(want, screen_w - g.x)
    elif align == "CENTER":
        centre = g.x + g.w / 2.0
        g.x = max(0, int(round(centre - want / 2.0)))
        g.w = min(want, screen_w - g.x)
    else:
        g.w = min(want, screen_w - g.x)


def font_size_of(widget):
    """Largest font size any style part of this widget asks for."""
    best = 0
    for st in widget.get("style") or []:
        f = st.get("font")
        if isinstance(f, int):
            best = max(best, f)
    return best


def walk_front(screen, ratio, parent_w, geo_map, prefix, boxes):
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
            need = text_width_estimate(w.get("text"), font_size_of(w))
            if need > g.w:
                widen_for_text(g, need, text_align_of(w), NEW_W)
            boxes.append({
                "name": full_name,
                "text": w.get("text") or "",
                "old": (w["left"], w["top"], w["width"], w["height"]),
                "new": (g.x, g.y, g.w, g.h),
            })

        w["left"], w["top"] = g.x, g.y
        w["width"], w["height"] = g.w, g.h
        scale_line_points(w, g.child_ratio)
        walk_front(w, g.child_ratio, g.w, geo_map, prefix, boxes)


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

    proj["FrontJson"] = [s for s in proj["FrontJson"]
                         if s.get("name") not in DROP_SCREENS]
    for screen in proj["FrontJson"]:
        screen["width"], screen["height"] = NEW_W, NEW_H
        boxes = []
        walk_front(screen, ratio, NEW_W, geo_map, screen["name"] + "_", boxes)
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
    return geo_map, screens, boxes_by_screen


POS_RE = re.compile(r"(lv_obj_set_pos\(ui->([A-Za-z0-9_]+),\s*)(-?\d+)(,\s*)(-?\d+)(\))")
SIZE_RE = re.compile(r"(lv_obj_set_size\(ui->([A-Za-z0-9_]+),\s*)(-?\d+)(,\s*)(-?\d+)(\))")
POINTS_RE = re.compile(r"(static\s+lv_point_t\s+([A-Za-z0-9_]+)\[\]\s*=\s*)\{(.*?)\};",
                       re.S)
PAIR_RE = re.compile(r"\{\s*(-?\d+)\s*,\s*(-?\d+)\s*\}")


def rewrite_c(src, geo_map, screens, ratio):
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

    out = POS_RE.sub(fix_pos, src)
    out = SIZE_RE.sub(fix_size, out)
    out = POINTS_RE.sub(fix_points, out)
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


def copy_generated(out_dir, geo_map, screens, ratio):
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
        new_text = rewrite_c(src_text, geo_map, screens, ratio)
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


def print_report(boxes_by_screen):
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
        _, _, boxes = rewrite_project(None)
        print_report(boxes)
        return

    if os.path.exists(out_project) and not args.force:
        sys.exit("%s already exists — re-running would discard hand-tuning.\n"
                 "Pass --force if that is what you want."
                 % os.path.relpath(out_project, REPO_ROOT))

    os.makedirs(args.out_dir, exist_ok=True)
    geo_map, screens, boxes = rewrite_project(out_project)
    with open(SRC_PROJECT, "r", encoding="utf-8") as fh:
        ratio = NEW_W / float(json.load(fh)["projectSettings"]["resolution"]["width"])
    copy_generated(args.out_dir, geo_map, screens, ratio)
    print_report(boxes)


if __name__ == "__main__":
    main()
