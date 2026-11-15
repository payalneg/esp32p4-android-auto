"""Pack a small area of Kraków (or any PBF extract) into NVT1 vector tiles.

Two sources:

    python navtiles.py demo --out build/navtiles --preview build/krakow.png
    python navtiles.py pbf --pbf data/malopolskie-latest.osm.pbf \
        --bbox 50.075,19.90,50.045,19.98 --zmin 11 --zmax 18 --out build/navtiles

`demo` needs no network and no PBF: it lays out the Old Town, Wawel, the
Vistula, Planty and the surrounding streets from real landmarks, so the
firmware path can be exercised end to end on a bench. `pbf` is the real
thing and needs pyosmium (`pip install osmium`).

The output directory is what goes on the card as `/sdcard/navtiles`:

    navtiles/vector.idx
    navtiles/11/1136/1375.vtile
    ...

Copy the whole `navtiles` folder to the card. The firmware prefers a
`.vtile` beside a `.png`, so a card that also has raster tiles keeps working.
"""
from __future__ import annotations

import argparse
import math
import random
import shutil
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path

import nvtile_format as nvt

# Windows consoles default to a legacy code page, and the help text and some
# names are UTF-8. Reconfiguring keeps `--help` and the tile log readable.
try:
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
except Exception:
    pass

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:  # preview only
    Image = None

M_PER_DEG_LAT = 111320.0
LAT0, LON0 = 50.0617, 19.9373
M_LON = 1.0 / (M_PER_DEG_LAT * math.cos(math.radians(LAT0)))


# --------------------------------------------------------------------------
# geometry in lon/lat
# --------------------------------------------------------------------------

@dataclass
class GeoFeature:
    layer: int
    cls: int
    rings: list[list[tuple[float, float]]]
    name: str = ""
    rank: int = 0
    min_zoom: int = 10
    flags: int = 0
    geom: int = nvt.GEOM_LINE

    def bbox(self):
        xs = [p[0] for ring in self.rings for p in ring]
        ys = [p[1] for ring in self.rings for p in ring]
        return min(xs), min(ys), max(xs), max(ys)


class Features:
    def __init__(self):
        self.items: list[GeoFeature] = []

    def add(self, layer, cls, rings, *, name="", rank=0, min_zoom=10,
            flags=0, geom=nvt.GEOM_LINE):
        # Drop degenerate rings here rather than in the packer: an empty ring
        # would make the C reader reject the whole tile.
        rings = [r for r in rings if len(r) >= (3 if geom == nvt.GEOM_POLYGON
                                                else 2 if geom == nvt.GEOM_LINE
                                                else 1)]
        if not rings:
            return
        self.items.append(GeoFeature(layer, cls, rings, name, rank, min_zoom,
                                     flags, geom))

    def point(self, layer, cls, lon, lat, *, name="", rank=0, min_zoom=10):
        self.items.append(GeoFeature(layer, cls, [[(lon, lat)]], name, rank,
                                     min_zoom, 0, nvt.GEOM_POINT))


def ll_xy(e: float, n: float) -> tuple[float, float]:
    # n is north-positive; latitude grows north, so add.
    return LON0 + e * M_LON, LAT0 + n / M_PER_DEG_LAT


def rect(e: float, n: float, w: float, h: float):
    return [[ll_xy(e - w / 2, n - h / 2), ll_xy(e + w / 2, n - h / 2),
             ll_xy(e + w / 2, n + h / 2), ll_xy(e - w / 2, n + h / 2)]]


def line(points):
    return [[ll_xy(e, n) for e, n in points]]


def band(points, width: float):
    """A river-like ribbon: offset the centre line both ways."""
    left, right = [], []
    half = width / 2
    for i, (e, n) in enumerate(points):
        if i == 0:
            de, dn = points[1][0] - e, points[1][1] - n
        elif i == len(points) - 1:
            de, dn = e - points[i - 1][0], n - points[i - 1][1]
        else:
            de = points[i + 1][0] - points[i - 1][0]
            dn = points[i + 1][1] - points[i - 1][1]
        length = math.hypot(de, dn) or 1.0
        # normal = (-dn, de)
        ne, nn = -dn / length, de / length
        left.append(ll_xy(e + ne * half, n + nn * half))
        right.append(ll_xy(e - ne * half, n - nn * half))
    return [left + list(reversed(right))]


def ring(outer_e0, outer_n0, outer_e1, outer_n1,
         inner_e0, inner_n0, inner_e1, inner_n1):
    outer = [ll_xy(outer_e0, outer_n0), ll_xy(outer_e1, outer_n0),
             ll_xy(outer_e1, outer_n1), ll_xy(outer_e0, outer_n1)]
    inner = [ll_xy(inner_e0, inner_n0), ll_xy(inner_e1, inner_n0),
             ll_xy(inner_e1, inner_n1), ll_xy(inner_e0, inner_n1)]
    return [outer, inner]


# --------------------------------------------------------------------------
# a recognisable, offline Kraków
# --------------------------------------------------------------------------

def build_demo() -> Features:
    f = Features()
    rng = random.Random(20261115)

    # ---- ground ---------------------------------------------------------
    # The Vistula, curving past Wawel and under the bridges.
    river = [(-1500, -760), (-900, -840), (-400, -900), (100, -890),
             (650, -810), (1250, -690), (1800, -610)]
    f.add(nvt.LAYER_WATER, 0, band(river, 110), min_zoom=10,
          geom=nvt.GEOM_POLYGON)
    f.add(nvt.LAYER_WATERWAY, 0, line([(-1500, -760), (-900, -840),
                                       (-400, -900), (100, -890),
                                       (650, -810), (1250, -690)]),
          min_zoom=10)

    # Planty: the green belt where the city walls used to be.
    f.add(nvt.LAYER_GREEN, 0,
          ring(-470, -390, 470, 470, -390, -310, 390, 390), min_zoom=10,
          geom=nvt.GEOM_POLYGON)

    # Błonia, west of the Old Town.
    f.add(nvt.LAYER_GREEN, 0, rect(-1250, 190, 500, 380), min_zoom=10,
          geom=nvt.GEOM_POLYGON)
    # A park below Wawel and one in Kazimierz.
    f.add(nvt.LAYER_GREEN, 0, rect(230, -760, 260, 180), min_zoom=11,
          geom=nvt.GEOM_POLYGON)
    f.add(nvt.LAYER_GREEN, 0, rect(700, -470, 180, 120), min_zoom=13,
          geom=nvt.GEOM_POLYGON)

    # ---- Old Town blocks ------------------------------------------------
    # A grid of tenement blocks with real street gaps, around the Rynek.
    for i in range(-5, 6):
        for j in range(-4, 5):
            e, n = i * 84.0, j * 84.0
            if abs(e) > 390 or n < -300 or n > 390:
                continue
            if abs(e) < 115 and abs(n) < 115:
                continue          # the Rynek itself
            w = rng.uniform(52, 68)
            h = rng.uniform(50, 66)
            f.add(nvt.LAYER_BUILDING, 0, rect(e, n, w, h), min_zoom=15,
                  geom=nvt.GEOM_POLYGON)
            # A courtyard, which also gives the block a second outline.
            if w > 60 and h > 58:
                f.add(nvt.LAYER_GREEN, 0, rect(e, n, w * 0.32, h * 0.32),
                      min_zoom=16, geom=nvt.GEOM_POLYGON)

    # Sukiennice in the middle of the Rynek, and the Town Hall tower.
    f.add(nvt.LAYER_BUILDING, 0, rect(0, 0, 50, 90), min_zoom=15,
          geom=nvt.GEOM_POLYGON)
    f.add(nvt.LAYER_BUILDING, 0, rect(-70, 18, 12, 12), min_zoom=16,
          geom=nvt.GEOM_POLYGON)

    # ---- Wawel ----------------------------------------------------------
    f.add(nvt.LAYER_GREEN, 0, rect(70, -760, 360, 220), min_zoom=11,
          geom=nvt.GEOM_POLYGON)
    for e, n, w, h in ((40, -790, 150, 55), (150, -770, 70, 45),
                       (75, -700, 130, 40), (-10, -735, 45, 70)):
        f.add(nvt.LAYER_BUILDING, 0, rect(e, n, w, h), min_zoom=14,
              geom=nvt.GEOM_POLYGON)
    f.point(nvt.LAYER_LABEL, nvt.LABEL_LANDMARK, *ll_xy(90, -760),
            name="Wawel", rank=0, min_zoom=11)

    # ---- Kazimierz ------------------------------------------------------
    for i in range(6):
        for j in range(4):
            e = 500 + i * 78
            n = -760 + j * 78
            f.add(nvt.LAYER_BUILDING, 0, rect(e, n, 52, 50), min_zoom=15,
                  geom=nvt.GEOM_POLYGON)

    # ---- Kleparz / station ---------------------------------------------
    for i in range(-3, 4):
        for j in range(2):
            e = i * 95
            n = 540 + j * 105
            f.add(nvt.LAYER_BUILDING, 0, rect(e, n, 62, 66), min_zoom=15,
                  geom=nvt.GEOM_POLYGON)
    f.add(nvt.LAYER_BUILDING, 0, rect(130, 830, 190, 55), min_zoom=12,
          geom=nvt.GEOM_POLYGON)

    # ---- streets --------------------------------------------------------
    named = [
        # (name, class, points, rank)
        ("Basztowa", nvt.ROAD_PRIMARY,
         [(-470, 430), (-250, 430), (0, 430), (250, 430), (470, 430)], 1),
        ("Westerplatte", nvt.ROAD_SECONDARY,
         [(430, 430), (430, 250), (430, 0), (430, -250)], 2),
        ("Grodzka", nvt.ROAD_SECONDARY,
         [(0, 0), (0, -180), (0, -380), (10, -560), (20, -690)], 1),
        ("Floriańska", nvt.ROAD_SECONDARY,
         [(0, 0), (0, 140), (0, 280), (0, 430)], 1),
        ("Szewska", nvt.ROAD_RESIDENTIAL, [(-130, 0), (0, 0), (130, 0)], 3),
        ("Sławkowska", nvt.ROAD_RESIDENTIAL,
         [(-130, 84), (0, 84), (130, 84)], 3),
        ("św. Jana", nvt.ROAD_RESIDENTIAL, [(-130, -84), (0, -84), (130, -84)], 3),
        ("Starowiślna", nvt.ROAD_SECONDARY,
         [(480, 430), (480, 100), (480, -200), (480, -470)], 2),
        ("Dietla", nvt.ROAD_PRIMARY,
         [(-520, -470), (-200, -470), (200, -470), (600, -470), (900, -470)], 1),
        ("Aleje Trzech Wieszczów", nvt.ROAD_PRIMARY,
         [(-520, -700), (-520, -350), (-520, 0), (-520, 300), (-520, 560)], 1),
        ("Jana Pawła II", nvt.ROAD_PRIMARY,
         [(900, -880), (900, -500), (900, -100), (900, 300), (900, 560)], 1),
        ("Kalwaryjska", nvt.ROAD_RESIDENTIAL,
         [(520, -650), (680, -650), (900, -650)], 3),
        ("Most Dębnicki", nvt.ROAD_SECONDARY,
         [(-570, -980), (-540, -880), (-510, -760)], 2),
        ("Most Grunwaldzki", nvt.ROAD_SECONDARY,
         [(0, -980), (0, -900), (0, -820)], 1),
        ("Bulwary Wiślane", nvt.ROAD_PATH,
         [(-500, -820), (-200, -860), (100, -850), (400, -790), (700, -740)], 5),
        ("Planty", nvt.ROAD_PATH,
         [(-430, -350), (430, -350), (430, 430), (-430, 430), (-430, -350)], 5),
    ]
    for name, cls, pts, rank in named:
        f.add(nvt.LAYER_ROAD, cls, line(pts), name=name, min_zoom=11,
              flags=nvt.FLAG_BRIDGE if name.startswith("Most") else 0)
        # A street label at the middle of the stretch. Long roads get one per
        # package; the renderer de-duplicates them by name on screen.
        mid = pts[len(pts) // 2]
        f.point(nvt.LAYER_LABEL, nvt.LABEL_STREET, *ll_xy(*mid),
                name=name, rank=rank, min_zoom=15)

    # Residential grid: the gaps between the blocks are the streets.
    for i in range(-6, 7):
        e = i * 84 + 42
        f.add(nvt.LAYER_ROAD, nvt.ROAD_RESIDENTIAL,
              line([(e, -330), (e, 410)]), min_zoom=14)
    for j in range(-5, 6):
        n = j * 84 + 42
        f.add(nvt.LAYER_ROAD, nvt.ROAD_RESIDENTIAL,
              line([(-420, n), (420, n)]), min_zoom=14)

    # The cycle path along the river.
    f.add(nvt.LAYER_ROAD, nvt.ROAD_CYCLE,
          line([(-700, -810), (-300, -850), (100, -840), (500, -770),
                (800, -720)]), min_zoom=15)

    # ---- rail -----------------------------------------------------------
    f.add(nvt.LAYER_RAIL, 0, line([(-1500, 820), (-700, 815), (0, 810),
                                   (700, 815), (1500, 825)]), min_zoom=11)

    # ---- labels ---------------------------------------------------------
    f.point(nvt.LAYER_LABEL, nvt.LABEL_PLACE, *ll_xy(0, 0),
            name="Rynek Główny", rank=0, min_zoom=10)
    f.point(nvt.LAYER_LABEL, nvt.LABEL_PARK, *ll_xy(-330, 350),
            name="Planty", rank=1, min_zoom=11)
    f.point(nvt.LAYER_LABEL, nvt.LABEL_WATER, *ll_xy(0, -900),
            name="Wisła", rank=0, min_zoom=10)
    f.point(nvt.LAYER_LABEL, nvt.LABEL_PLACE, *ll_xy(-250, 250),
            name="Stare Miasto", rank=1, min_zoom=11)
    f.point(nvt.LAYER_LABEL, nvt.LABEL_PLACE, *ll_xy(650, -600),
            name="Kazimierz", rank=1, min_zoom=11)
    f.point(nvt.LAYER_LABEL, nvt.LABEL_PLACE, *ll_xy(0, 650),
            name="Kleparz", rank=2, min_zoom=12)
    f.point(nvt.LAYER_LABEL, nvt.LABEL_PARK, *ll_xy(-1150, 200),
            name="Błonia", rank=2, min_zoom=12)
    f.point(nvt.LAYER_LABEL, nvt.LABEL_LANDMARK, *ll_xy(0, 520),
            name="Barbakan", rank=2, min_zoom=13)
    f.point(nvt.LAYER_LABEL, nvt.LABEL_LANDMARK, *ll_xy(55, 60),
            name="Kościół Mariacki", rank=2, min_zoom=14)

    return f


# --------------------------------------------------------------------------
# projection, simplification and tiling
# --------------------------------------------------------------------------

def merc(lon: float, lat: float) -> tuple[float, float]:
    lat = max(-85.0511, min(85.0511, lat))
    x = (lon + 180.0) / 360.0
    y = (1.0 - math.asinh(math.tan(math.radians(lat))) / math.pi) / 2.0
    return x, y


def world_units(lon: float, lat: float, z: int) -> tuple[float, float]:
    x, y = merc(lon, lat)
    n = (1 << z) * nvt.EXTENT
    return x * n, y * n


def _perp_dist(p, a, b) -> float:
    ax, ay = a
    bx, by = b
    px, py = p
    dx, dy = bx - ax, by - ay
    den = math.hypot(dx, dy)
    if den == 0:
        return math.hypot(px - ax, py - ay)
    return abs(dx * (ay - py) - dy * (ax - px)) / den


def simplify(points: list[tuple[float, float]], tolerance: float):
    """Ramer-Douglas-Peucker, iterative so a long river cannot recurse deep."""
    if len(points) <= 2 or tolerance <= 0:
        return points
    keep = [False] * len(points)
    keep[0] = keep[-1] = True
    stack = [(0, len(points) - 1)]
    while stack:
        a, b = stack.pop()
        if b <= a + 1:
            continue
        worst, index = -1.0, -1
        for i in range(a + 1, b):
            d = _perp_dist(points[i], points[a], points[b])
            if d > worst:
                worst, index = d, i
        if worst > tolerance:
            keep[index] = True
            stack.append((a, index))
            stack.append((index, b))
    return [p for p, k in zip(points, keep) if k]


def tolerance(z: int) -> float:
    # A quarter of a pixel at the top of the useful range; coarser below,
    # where the same geometry is redrawn at a fraction of its size.
    if z <= 11:
        return 48.0
    if z <= 13:
        return 24.0
    if z <= 14:
        return 12.0
    if z <= 16:
        return 6.0
    return 2.0


def fit_rings(rings: list[list[tuple[float, float]]], limit: int = 256,
              min_len: int = 2):
    """Keep a feature inside the firmware's 256-point budget.

    A river crossing several tiles at z18 can otherwise be stored whole with
    thousands of vertices in every tile it touches. The C reader rejects a
    feature that does not fit, which would take the rest of the tile down
    with it; simplify harder rather than lose the tile.
    """
    total = sum(len(r) for r in rings)
    if total <= limit:
        return rings
    tol = 2.0
    while total > limit and tol < 1e6:
        rings = [simplify(r, tol) for r in rings]
        total = sum(len(r) for r in rings)
        tol *= 2.0
    if total <= limit:
        return rings

    # Pathological input: sample each ring proportionally. A road at this
    # point is a chain of a few dozen points, which is all a 256 px tile can
    # show anyway.
    out = []
    for r in rings:
        share = max(min_len, int(limit * len(r) / total))
        share = min(share, len(r))
        if share < len(r):
            step = (len(r) - 1) / (share - 1) if share > 1 else 1
            r = [r[round(i * step)] for i in range(share)]
        out.append(r)
    if sum(len(r) for r in out) > limit:
        return []
    return out


MIN_ZOOM = {
    (nvt.LAYER_WATER, 0): 10,
    (nvt.LAYER_WATERWAY, 0): 10,
    (nvt.LAYER_GREEN, 0): 10,
    (nvt.LAYER_BUILDING, 0): 15,
    (nvt.LAYER_ROAD, nvt.ROAD_MOTORWAY): 10,
    (nvt.LAYER_ROAD, nvt.ROAD_PRIMARY): 11,
    (nvt.LAYER_ROAD, nvt.ROAD_SECONDARY): 12,
    (nvt.LAYER_ROAD, nvt.ROAD_RESIDENTIAL): 14,
    (nvt.LAYER_ROAD, nvt.ROAD_SERVICE): 15,
    (nvt.LAYER_ROAD, nvt.ROAD_PATH): 15,
    (nvt.LAYER_ROAD, nvt.ROAD_CYCLE): 15,
    (nvt.LAYER_RAIL, 0): 11,
    (nvt.LAYER_RAIL, 1): 13,
    (nvt.LAYER_LABEL, nvt.LABEL_PLACE): 10,
    (nvt.LAYER_LABEL, nvt.LABEL_WATER): 10,
    (nvt.LAYER_LABEL, nvt.LABEL_PARK): 11,
    (nvt.LAYER_LABEL, nvt.LABEL_STREET): 15,
    (nvt.LAYER_LABEL, nvt.LABEL_LANDMARK): 12,
}

# A tile that needs more than this is a data problem, not a map: the firmware
# copies the whole thing into 192 KB and then draws it in one frame.
CAPS = {
    nvt.LAYER_GREEN: 80,
    nvt.LAYER_WATER: 60,
    nvt.LAYER_WATERWAY: 40,
    nvt.LAYER_BUILDING: 1200,
    nvt.LAYER_ROAD: 900,
    nvt.LAYER_RAIL: 30,
    nvt.LAYER_LABEL: 4,
}


def _bbox_area(ring):
    xs = [p[0] for p in ring]
    ys = [p[1] for p in ring]
    return max(0.0, (max(xs) - min(xs)) * (max(ys) - min(ys)))


def build_tiles(features: Features, bbox, zmin: int, zmax: int,
                out: Path, *, verbose=True) -> tuple[int, int]:
    """Cut every feature into every tile that its bbox touches.

    Returns (tile_count, bytes_written).
    """
    north, west, south, east = bbox
    out.mkdir(parents=True, exist_ok=True)
    total_tiles = 0
    total_bytes = 0

    for z in range(zmin, zmax + 1):
        tiles: dict[tuple[int, int], dict[int, list[nvt.Feature]]] = \
            defaultdict(lambda: defaultdict(list))

        for gf in features.items:
            if z < gf.min_zoom:
                continue
            min_z = MIN_ZOOM.get((gf.layer, gf.cls), gf.min_zoom)
            if z < min_z:
                continue

            if gf.geom == nvt.GEOM_POINT:
                lon, lat = gf.rings[0][0]
                wx, wy = world_units(lon, lat, z)
                tx, ty = int(wx // nvt.EXTENT), int(wy // nvt.EXTENT)
                local = [(round(wx - tx * nvt.EXTENT),
                          round(wy - ty * nvt.EXTENT))]
                tiles[(tx, ty)][gf.layer].append(nvt.Feature(
                    nvt.GEOM_POINT, gf.cls, [local], gf.name, gf.rank,
                    gf.flags))
                continue

            wrings = []
            for r in gf.rings:
                pts = [world_units(lon, lat, z) for lon, lat in r]
                if gf.geom == nvt.GEOM_POLYGON:
                    pts = simplify(pts, tolerance(z))
                else:
                    pts = simplify(pts, tolerance(z))
                if gf.geom == nvt.GEOM_POLYGON and len(pts) < 3:
                    pts = []
                if gf.geom == nvt.GEOM_LINE and len(pts) < 2:
                    pts = []
                wrings.append(pts)
            if not wrings or any(not r for r in wrings):
                continue

            wrings = fit_rings(
                wrings, min_len=3 if gf.geom == nvt.GEOM_POLYGON else 2)
            if not wrings:
                continue
            if gf.geom == nvt.GEOM_POLYGON and any(len(r) < 3 for r in wrings):
                continue
            if gf.geom == nvt.GEOM_LINE and any(len(r) < 2 for r in wrings):
                continue

            if gf.geom == nvt.GEOM_POLYGON:
                area = _bbox_area(wrings[0])
                # Below a pixel and a half the shape is a smudge; dropping it
                # is both faster and quieter than drawing a dot.
                min_px = 3.0 if z >= 16 else 2.0
                if area < (min_px * nvt.EXTENT / 256) ** 2:
                    continue

            xs = [p[0] for r in wrings for p in r]
            ys = [p[1] for r in wrings for p in r]
            pad = 96 if gf.geom == nvt.GEOM_POLYGON else 64
            tx0 = int((min(xs) - pad) // nvt.EXTENT)
            tx1 = int((max(xs) + pad) // nvt.EXTENT)
            ty0 = int((min(ys) - pad) // nvt.EXTENT)
            ty1 = int((max(ys) + pad) // nvt.EXTENT)
            for ty in range(ty0, ty1 + 1):
                for tx in range(tx0, tx1 + 1):
                    local = [[(round(x - tx * nvt.EXTENT),
                               round(y - ty * nvt.EXTENT)) for x, y in r]
                             for r in wrings]
                    # A line with one vertex in a tile still crosses it; keep
                    # it. A polygon whose bbox misses has already been skipped.
                    tiles[(tx, ty)][gf.layer].append(nvt.Feature(
                        gf.geom, gf.cls, local, gf.name, gf.rank, gf.flags))

        # Cap and order. Buildings and roads dominate the file, and a rider
        # cannot tell the difference between 900 and 1200 tenements at 400 m.
        for (tx, ty), layers in tiles.items():
            for layer_id, items in list(layers.items()):
                cap = CAPS.get(layer_id, 1000)
                if layer_id == nvt.LAYER_LABEL:
                    # One label per name per tile, best rank first.
                    best = {}
                    for it in items:
                        if not it.name:
                            continue
                        old = best.get(it.name)
                        if old is None or it.rank < old.rank:
                            best[it.name] = it
                    items = sorted(best.values(), key=lambda x: x.rank)[:cap]
                elif len(items) > cap:
                    if layer_id == nvt.LAYER_BUILDING:
                        items.sort(
                            key=lambda x: _bbox_area(x.rings[0]), reverse=True)
                    elif layer_id == nvt.LAYER_ROAD:
                        items.sort(key=lambda x: (x.cls, -len(x.rings[0])))
                    else:
                        items.sort(
                            key=lambda x: _bbox_area(x.rings[0]), reverse=True)
                    items = items[:cap]
                layers[layer_id] = items
            # Draw order is the file order; nvtile_format enforces it.

        zdir = out / str(z)
        for (tx, ty), layers in sorted(tiles.items()):
            if tx < 0 or ty < 0 or tx >= (1 << z) or ty >= (1 << z):
                continue
            data = nvt.pack_tile(layers, z)
            path = zdir / str(tx) / f"{ty}.vtile"
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
            total_tiles += 1
            total_bytes += len(data)
        if verbose:
            print(f"z={z}: {len(tiles)} tiles", flush=True)

    nvt.write_index(out / "vector.idx", zmin=zmin, zmax=zmax,
                    north=north, west=west, south=south, east=east,
                    tiles=total_tiles, nbytes=total_bytes)
    (out / "README.txt").write_text(
        "NVT1 vector tiles for the head-unit navigator.\n"
        f"bbox: {north:.6f},{west:.6f},{south:.6f},{east:.6f}\n"
        f"zooms: {zmin}..{zmax}\n"
        f"tiles: {total_tiles}, bytes: {total_bytes}\n"
        "Copy this directory to /sdcard/navtiles on the panel.\n",
        encoding="utf-8")
    return total_tiles, total_bytes


# --------------------------------------------------------------------------
# PBF source (optional)
# --------------------------------------------------------------------------

def load_pbf(pbf: Path, bbox, zmin: int, zmax: int) -> Features:
    try:
        import osmium  # noqa: F401
    except ImportError:
        raise SystemExit("pyosmium is not installed: pip install osmium")
    import config
    import builder

    config.BBOX = bbox
    collector = builder.Collector()
    collector.apply_file(str(pbf), locations=True)

    # VTL2 layer -> NVT layer. The geometry enum is also different (1/2/3
    # versus 0/1/2), which is the one place a silent conversion would be
    # invisible: a polygon would be read as a line.
    layer_map = {
        1: nvt.LAYER_WATER,     # water: polygons or lines
        2: nvt.LAYER_GREEN,
        3: nvt.LAYER_ROAD,
        4: nvt.LAYER_RAIL,
        5: nvt.LAYER_BUILDING,
        6: nvt.LAYER_LABEL,
    }
    geom_map = {1: nvt.GEOM_LINE, 2: nvt.GEOM_POLYGON, 3: nvt.GEOM_POINT}
    out = Features()
    for layer, cls, name, mcoords, geom in collector.features:
        nlayer = layer_map.get(layer)
        ngeom = geom_map.get(geom)
        if nlayer is None or ngeom is None:
            continue
        if layer == 1 and geom == 1:
            nlayer = nvt.LAYER_WATERWAY
        ncls = cls
        if nlayer == nvt.LAYER_LABEL:
            # VTL2's label classes are place sizes (city/town/suburb/...).
            # NVT1's are kinds, and a town is not water; collapse them to
            # "place" and keep the size as the rank.
            ncls = nvt.LABEL_PLACE
        rings = [[]]
        for mx, my in mcoords:
            # Collector already has Web Mercator fractions; invert to lon/lat
            # only to let the common tiler do the projection once more. That
            # is exact to float precision and keeps one code path.
            lon = mx * 360.0 - 180.0
            n = math.pi * (1.0 - 2.0 * my)
            lat = math.degrees(math.atan(math.sinh(n)))
            rings[0].append((lon, lat))
        rank = cls if nlayer == nvt.LAYER_LABEL else 0
        gf = GeoFeature(nlayer, ncls, rings, name or "", rank,
                        MIN_ZOOM.get((nlayer, ncls), 10), 0, ngeom)
        out.items.append(gf)
        if nlayer == nvt.LAYER_ROAD and name and ngeom == nvt.GEOM_LINE:
            mid = rings[0][len(rings[0]) // 2]
            out.point(nvt.LAYER_LABEL, nvt.LABEL_STREET, mid[0], mid[1],
                      name=name, rank=3 if cls <= 2 else 5, min_zoom=15)
    return out


# --------------------------------------------------------------------------
# preview (developer aid)
# --------------------------------------------------------------------------

STYLE = {
    nvt.LAYER_GREEN: ((220, 233, 210), None, 0),
    nvt.LAYER_WATER: ((166, 212, 232), (135, 190, 214), 1),
    nvt.LAYER_BUILDING: ((231, 223, 210), (211, 199, 183), 1),
    nvt.LAYER_WATERWAY: ((166, 212, 232), None, 4),
    nvt.LAYER_RAIL: ((120, 120, 120), None, 1),
}
ROAD_STYLE = {
    0: ((232, 200, 138), (185, 155, 99), 7),
    1: ((242, 217, 160), (195, 174, 132), 7),
    2: ((255, 240, 194), (201, 188, 153), 5),
    3: ((255, 255, 255), (201, 201, 192), 3),
    4: ((255, 255, 255), (213, 213, 204), 1),
    5: ((183, 183, 174), None, 1),
    6: ((46, 158, 142), (232, 245, 242), 3),
}


def _font(size):
    for name in ("segoeui.ttf", "arial.ttf", "DejaVuSans.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default()


def render_preview(root: Path, lat: float, lon: float, z: int,
                   path: Path, width=800, height=480, heading=0.0):
    if Image is None:
        raise SystemExit("Pillow is not installed")
    cx, cy = world_units(lon, lat, z)
    k = 256.0 / nvt.EXTENT
    x0 = cx - (width / 2) / k
    y0 = cy - (height / 2) / k
    tx0 = int(x0 // nvt.EXTENT)
    tx1 = int((x0 + width / k) // nvt.EXTENT)
    ty0 = int(y0 // nvt.EXTENT)
    ty1 = int((y0 + height / k) // nvt.EXTENT)
    canvas = Image.new("RGB", (width, height), (243, 242, 235))
    draw = ImageDraw.Draw(canvas)
    labels = []
    for ty in range(ty0, ty1 + 1):
        for tx in range(tx0, tx1 + 1):
            p = root / str(z) / str(tx) / f"{ty}.vtile"
            if not p.exists():
                continue
            _, layers = nvt.parse_tile(p.read_bytes())
            # Render one tile in its own 256x256 image. A feature that crosses
            # the tile edge is stored whole, and drawing it straight onto the
            # 800x480 canvas would let its off-screen part fill the canvas.
            tile = Image.new("RGB", (256, 256), (243, 242, 235))
            tdraw = ImageDraw.Draw(tile)
            k = 256.0 / nvt.EXTENT

            def pts2(ring):
                return [(x * k, y * k) for x, y in ring]

            for layer_id in nvt.DRAW_ORDER:
                for feat in layers.get(layer_id, ()):
                    if layer_id == nvt.LAYER_LABEL:
                        if feat.geom == nvt.GEOM_POINT:
                            x, y = feat.rings[0][0]
                            labels.append((feat.rank, feat.cls,
                                           (tx * 4096 + x) * k
                                           - x0 * k,
                                           (ty * 4096 + y) * k
                                           - y0 * k,
                                           feat.name))
                        continue
                    if layer_id == nvt.LAYER_ROAD:
                        fill, casing, w = ROAD_STYLE.get(feat.cls,
                                                         ROAD_STYLE[3])
                        for ring in feat.rings:
                            pp = pts2(ring)
                            if len(pp) < 2:
                                continue
                            if casing and w > 1:
                                tdraw.line(pp, fill=casing, width=w + 2,
                                           joint="curve")
                            tdraw.line(pp, fill=fill, width=w, joint="curve")
                        continue
                    fill, edge, w = STYLE.get(layer_id,
                                              ((200, 200, 200), None, 1))
                    if feat.geom == nvt.GEOM_POLYGON:
                        pp = pts2(feat.rings[0])
                        tdraw.polygon(pp, fill=fill)
                        if edge:
                            tdraw.line(pp + [pp[0]], fill=edge, width=1)
                        # Holes are painted as land, which is what the C
                        # rasterizer's even-odd rule does.
                        for hole in feat.rings[1:]:
                            tdraw.polygon(pts2(hole), fill=(243, 242, 235))
                    elif feat.geom == nvt.GEOM_LINE:
                        for ring in feat.rings:
                            pp = pts2(ring)
                            if len(pp) >= 2:
                                tdraw.line(pp, fill=fill, width=w)

            px = int(round(tx * 4096 * k - x0 * k))
            py = int(round(ty * 4096 * k - y0 * k))
            canvas.paste(tile, (px, py))

    font = _font(16)
    for rank, kind, x, y, name in sorted(labels, key=lambda v: v[0]):
        if not name:
            continue
        tw = draw.textlength(name, font=font)
        draw.rectangle([x - tw / 2 - 3, y - 10, x + tw / 2 + 3, y + 10],
                       fill=(20, 24, 28))
        draw.text((x - tw / 2, y - 9), name, font=font,
                  fill=(255, 230, 190) if kind == nvt.LABEL_WATER
                  else (255, 255, 255))

    if heading:
        canvas = canvas.rotate(heading, resample=Image.BICUBIC)
    path.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(path)
    print(f"preview -> {path}")


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------

def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--demo", action="store_true",
                    help="build the offline Kraków layout")
    ap.add_argument("--pbf", type=Path,
                    help="OSM PBF extract (needs pyosmium)")
    ap.add_argument("--bbox", default="50.075,19.900,50.045,19.985",
                    help="north,west,south,east")
    ap.add_argument("--zmin", type=int, default=11)
    ap.add_argument("--zmax", type=int, default=18)
    ap.add_argument("--out", type=Path, default=Path("build/navtiles"))
    ap.add_argument("--clean", action="store_true")
    ap.add_argument("--preview", type=Path, default=None,
                    help="render a north-up 800x480 PNG at z17")
    ap.add_argument("--preview-z", type=int, default=17)
    ap.add_argument("--preview-heading", type=float, default=0.0)
    args = ap.parse_args(argv)

    if not args.demo and not args.pbf:
        ap.error("choose --demo or --pbf")
    bbox = tuple(float(v) for v in args.bbox.split(","))
    if len(bbox) != 4:
        ap.error("--bbox needs north,west,south,east")
    north, west, south, east = bbox
    if not (north > south and east > west):
        ap.error("bbox is upside down")

    if args.clean and args.out.exists():
        shutil.rmtree(args.out)
    args.out.mkdir(parents=True, exist_ok=True)

    if args.demo:
        features = build_demo()
        # The demo lives around Rynek; the bbox is only for the index unless
        # the caller asked for a wider area, in which case the same geometry
        # is cut where it falls.
        print(f"demo geometry: {len(features.items)} features")
    else:
        if not args.pbf.exists():
            raise SystemExit(f"{args.pbf} not found")
        features = load_pbf(args.pbf, bbox, args.zmin, args.zmax)
        print(f"pbf geometry: {len(features.items)} features")

    tiles, nbytes = build_tiles(features, bbox, args.zmin, args.zmax,
                                args.out)
    print(f"{tiles} tiles, {nbytes / 1024:.0f} KiB -> {args.out}")
    if args.preview:
        center = ((north + south) / 2, (west + east) / 2)
        render_preview(args.out, center[0], center[1], args.preview_z,
                       args.preview, heading=args.preview_heading)
    return 0


if __name__ == "__main__":
    sys.exit(main())
