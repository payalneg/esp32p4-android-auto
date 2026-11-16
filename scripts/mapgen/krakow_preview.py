"""Render the phone's Krakow map prototype in Python and Pillow."""

import argparse
import json
import math
from functools import cached_property, lru_cache
from pathlib import Path
from shutil import copytree
from tempfile import TemporaryDirectory
from time import perf_counter

from PIL import Image, ImageChops, ImageDraw, ImageFont

import config
from map_engine import MapEngine, TILE_CACHE_SIZE
import tile_math

BUCKET_ZOOM = 13
GEOMETRY_ZOOM = 19
BACKGROUND = '#f3f2eb'
WATER = '#a6d4e8'
GREEN, WATER_AREA, RIVER, PATH, ROAD, MAIN_ROAD, BIKE_LANE, BIKE_PATH = range(8)
BUILDING, BRIDGE, BIKE_BRIDGE = range(8, 11)
DRAW_ORDER = (GREEN, WATER_AREA, RIVER, BUILDING, PATH, ROAD, MAIN_ROAD,
              BRIDGE, BIKE_LANE, BIKE_PATH, BIKE_BRIDGE)
AREA_STYLE = {GREEN: ('#dce9d2', None), WATER_AREA: (WATER, '#87bed6'),
              BUILDING: ('#e7dfd2', '#d9cfc0')}
LINE_STYLE = {
    RIVER: (WATER, None, 4),
    PATH: ('#a5ad9f', None, 0.9),
    ROAD: ('#fffdf9', '#c9c9bf', 3.2),
    MAIN_ROAD: ('#f4dfab', '#bcab85', 5.0),
    BRIDGE: ('#fff0cb', '#657b83', 5.4),
    BIKE_LANE: ('#4e9d9d', '#f4fbf8', 2.3),
    BIKE_PATH: ('#167f77', '#f4fbf8', 2.5),
    BIKE_BRIDGE: ('#167f77', '#275d63', 3.0),
}
GOOGLE_AREA_STYLE = {GREEN: ('#b8e4cd', None), WATER_AREA: ('#82cbdf', None),
                     BUILDING: ('#ebe7dd', None)}
GOOGLE_LINE_STYLE = {
    RIVER: ('#82cbdf', None, 4),
    PATH: ('#bccbd0', None, 1.0),
    ROAD: ('#f7f9fa', '#d4dde1', 2.0),
    MAIN_ROAD: ('#e4ecef', '#c5d0d6', 7.0),
    BRIDGE: ('#e2eaed', '#a6b7c1', 9.0),
    BIKE_LANE: ('#b0d1c3', None, 1.3),
    BIKE_PATH: ('#84b9a8', None, 1.4),
    BIKE_BRIDGE: ('#b6cbd0', '#8ca9b5', 2.8),
}
LANDMARK_NAMES = {
    'Zamek Królewski na Wawelu': 'Wawel',
    'Zamek Wawel': 'Wawel',
    'Muzeum Sztuki i Techniki Japońskiej Manggha': 'Muzeum Manggha',
}


@lru_cache(maxsize=8)
def label_font(size, bold):
    for name in (('segoeuib.ttf', 'DejaVuSans-Bold.ttf') if bold else
                 ('segoeui.ttf', 'DejaVuSans.ttf')):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default()


def feature_kind(tags, closed):
    if closed:
        if tags.get("natural") == "water" or tags.get("waterway") == "riverbank":
            return WATER_AREA
        if (tags.get("leisure") == "park" or tags.get("landuse") == "forest"
                or tags.get("natural") == "wood"):
            return GREEN
    if tags.get("waterway") == "river":
        return RIVER
    highway = tags.get("highway")
    if highway is None or tags.get("area") == "yes":
        return None
    if highway == "cycleway":
        return BIKE_PATH
    if highway in {"path", "footway", "pedestrian"}:
        return BIKE_PATH if tags.get("bicycle") == "designated" else PATH
    if any(tags.get(key) in {"lane", "track", "opposite_lane", "opposite_track"}
           for key in ("cycleway", "cycleway:left", "cycleway:right", "cycleway:both")):
        return BIKE_LANE
    if highway in {"motorway", "trunk", "primary", "secondary", "tertiary"}:
        return MAIN_ROAD
    if highway in {"residential", "living_street", "unclassified", "service"}:
        return ROAD
    return None


def build_geometry(pbf, directory):
    """Compile the Krakow preview's ways, area rings and map labels."""
    import osmium

    xs, ys = tile_math.bbox_tiles(config.BBOX, BUCKET_ZOOM)
    buckets = {}
    labels = []
    feature_id = 0
    units = config.TILE_SIZE << (GEOMETRY_ZOOM - BUCKET_ZOOM)
    north, west, south, east = config.BBOX

    def project(nodes):
        return [tile_math.deg2tile_f(n.lat, n.lon, BUCKET_ZOOM) for n in nodes]

    def add_feature(kind, coords, holes=()):
        nonlocal feature_id
        px, py = zip(*coords)
        x_lo, x_hi = max(int(min(px)), xs.start), min(int(max(px)), xs.stop - 1)
        y_lo, y_hi = max(int(min(py)), ys.start), min(int(max(py)), ys.stop - 1)
        if x_lo > x_hi or y_lo > y_hi:
            return
        feature_id += 1
        row = [feature_id, kind]
        for ring in (coords, *holes):
            if len(row) > 2:
                row.extend((None, None))
            row.extend(round(c * units) for xy in ring for c in xy)
        for x in range(x_lo, x_hi + 1):
            for y in range(y_lo, y_hi + 1):
                buckets.setdefault((x, y), []).append(row)

    def in_preview(nodes):
        return (
            len(nodes) >= 2 and all(node.location.valid() for node in nodes)
            and not (all(n.lat > north for n in nodes) or all(n.lat < south for n in nodes)
                     or all(n.lon < west for n in nodes) or all(n.lon > east for n in nodes))
        )

    def add_label(kind, name, x, y, length=0):
        if name:
            labels.append((kind, name, round(x * units), round(y * units), length))

    class Collector(osmium.SimpleHandler):
        def way(self, way):
            kind = feature_kind(way.tags, way.is_closed())
            if kind in (None, GREEN, WATER_AREA) or not in_preview(way.nodes):
                return
            tags = way.tags
            if kind != RIVER and tags.get('bridge') not in (None, 'no', 'false'):
                kind = BIKE_BRIDGE if kind in (BIKE_PATH, BIKE_LANE) else BRIDGE
            coords = project(way.nodes)
            add_feature(kind, coords)
            name = tags.get('name')
            if name and kind in (MAIN_ROAD, BRIDGE, BIKE_BRIDGE, RIVER):
                length = sum(math.dist(a, b) for a, b in zip(coords, coords[1:]))
                x, y = coords[len(coords) // 2]
                label_kind = 'river' if kind == RIVER else 'bridge' if kind in (BRIDGE, BIKE_BRIDGE) else 'street'
                add_label(label_kind, name, x, y, length)

        def area(self, area):
            tags = area.tags
            kind = feature_kind(tags, True)
            landmark = tags.get('name') and (tags.get('historic') in ('castle', 'monument')
                                             or tags.get('tourism') in ('museum', 'attraction'))
            if kind not in (GREEN, WATER_AREA):
                kind = BUILDING if tags.get('building') not in (None, 'no') else None
                if kind is None and not landmark:
                    return
            for outer in area.outer_rings():
                if not in_preview(outer):
                    continue
                coords = project(outer)
                if kind is not None:
                    holes = [project(inner) for inner in area.inner_rings(outer)
                             if all(node.location.valid() for node in inner)]
                    add_feature(kind, coords, holes)
                if landmark:
                    x, y = zip(*coords)
                    add_label('landmark', tags.get('name'), (min(x) + max(x)) / 2, (min(y) + max(y)) / 2)

    Collector().apply_file(str(pbf), locations=True, idx="flex_mem")
    if not buckets:
        raise ValueError("the PBF contains no Krakow geometry")
    directory.mkdir(parents=True, exist_ok=True)
    for (x, y), features in buckets.items():
        (directory / f"{x}-{y}.json").write_text(
            json.dumps(features, separators=(",", ":")), encoding="utf-8")
    (directory / 'labels.json').write_text(json.dumps(labels, ensure_ascii=False), encoding='utf-8')
    return len(buckets)


class KrakowPreview(MapEngine):
    ZOOM_LIMITS = (11, GEOMETRY_ZOOM)

    def __init__(self, geometry, style='cycling'):
        super().__init__()
        self.geometry = geometry
        self.style = style
        self.center_lat, self.center_lon = (50.0479, 19.9384)
        self.zoom = 17
        self.coverage = tile_math.bbox_tiles(config.BBOX, BUCKET_ZOOM)

    @cached_property
    def labels(self):
        path = self.geometry / 'labels.json'
        return json.loads(path.read_text(encoding='utf-8')) if path.is_file() else []

    @lru_cache(maxsize=24)
    def _bucket(self, x, y):
        path = self.geometry / f"{x}-{y}.json"
        return json.loads(path.read_text(encoding="utf-8")) if path.is_file() else ()

    @lru_cache(maxsize=12)
    def _line_bearings(self, x, y):
        bearings = {}
        for _, kind, *coords in self._bucket(x, y):
            if kind not in (MAIN_ROAD, BRIDGE, BIKE_BRIDGE, RIVER):
                continue
            points = list(zip(coords[::2], coords[1::2]))
            index = len(points) // 2
            a, b = points[max(0, index - 2)], points[min(len(points) - 1, index + 2)]
            dx, dy = b[0] - a[0], b[1] - a[1]
            score = dx * dx + dy * dy
            key = points[index]
            if score > bearings.get(key, (0, 0))[0]:
                degrees = math.degrees(math.atan2(dy, dx))
                bearings[key] = (score, (degrees + 90) % 180 - 90)
        return bearings

    def _get_tile(self, z, x, y):
        key = (z, x, y)
        if key not in self._cache:
            self._cache[key] = self.render_tile(*key)
            if len(self._cache) > TILE_CACHE_SIZE:
                self._cache.popitem(last=False)
        self._cache.move_to_end(key)
        return self._cache[key]

    def render_tile(self, z, tx, ty):
        pad, size = 8, config.TILE_SIZE
        scale = 2 ** (BUCKET_ZOOM - z)
        bounds = [(math.floor(t * scale), math.ceil((t + 1) * scale) - 1)
                  for t in (tx, ty)]
        if any(lo >= coverage.stop or hi < coverage.start
               for (lo, hi), coverage in zip(bounds, self.coverage)):
            return self._placeholder
        shift = 1 << max(0, z - BUCKET_ZOOM)
        x_range, y_range = (
            range(lo - (t % shift == 0), hi + 1 + ((t + 1) % shift == 0))
            for (lo, hi), t in zip(bounds, (tx, ty))
        )
        layers = {kind: {} for kind in DRAW_ORDER}
        for x in x_range:
            for y in y_range:
                for identifier, kind, *coords in self._bucket(x, y):
                    layers[kind].setdefault(identifier, coords)

        ss = 2
        google = self.style == 'google'
        image = Image.new('RGB', ((size + 2 * pad) * ss,) * 2,
                          '#f2f3ee' if google else BACKGROUND)
        draw = ImageDraw.Draw(image)
        areas = GOOGLE_AREA_STYLE if google else AREA_STYLE
        lines = GOOGLE_LINE_STYLE if google else LINE_STYLE
        divisor = (1 << (GEOMETRY_ZOOM - z)) / ss
        offset_x, offset_y = (pad - tx * size) * ss, (pad - ty * size) * ss
        line_scale = 0.53 if z < 16 else min(1.6, 1 + (z - 16) * 0.2)
        for kind, features in layers.items():
            if (z < 15 and kind == PATH) or (z < 16 and kind == BUILDING):
                continue
            area_style = areas.get(kind)
            if area_style is None:
                color, casing, width = lines[kind]
                width = max(1, round(width * ss * line_scale))
            shapes = []
            for coords in features.values():
                rings = [[]]
                for x, y in zip(coords[::2], coords[1::2]):
                    if x is None:
                        rings.append([])
                    else:
                        rings[-1].append((round(x / divisor + offset_x), round(y / divisor + offset_y)))
                points = rings[0]
                xs, ys = zip(*points)
                if (min(xs) > image.width or min(ys) > image.height or max(xs) < 0 or max(ys) < 0):
                    continue
                if google and z == 16 and kind == BUILDING and (max(xs) - min(xs)) * (max(ys) - min(ys)) < 180:
                    continue
                if area_style:
                    fill, border = area_style
                    if len(rings) == 1:
                        draw.polygon(points, fill=fill, outline=border)
                    else:
                        mask = Image.new('L', image.size)
                        masked = ImageDraw.Draw(mask)
                        masked.polygon(points, fill=255)
                        for hole in rings[1:]:
                            masked.polygon(hole, fill=0)
                        image.paste(fill, (0, 0, *image.size), mask)
                else:
                    shapes.append(points)
            if area_style is None:
                if casing:
                    for points in shapes:
                        draw.line(points, fill=casing, width=width + 2 * ss, joint='curve')
                for points in shapes:
                    draw.line(points, fill=color, width=width, joint='curve')
        image = image.resize((size + 2 * pad,) * 2, Image.Resampling.BOX)
        return image.crop((pad, pad, size + pad, size + pad))

    def render(self, width, height):
        frame = super().render(width, height)
        cx, cy = tile_math.deg2tile_f(self.center_lat, self.center_lon, self.zoom)
        self._draw_labels(frame, cx * config.TILE_SIZE - width / 2,
                          cy * config.TILE_SIZE - height / 2)
        return frame

    def _draw_labels(self, frame, left, top):
        if self.zoom < 15:
            return
        scale = 1 << (GEOMETRY_ZOOM - self.zoom)
        priority = {'landmark': 0, 'bridge': 1, 'river': 2, 'street': 3}
        colors = {'landmark': '#594965', 'bridge': '#415761',
                  'river': '#31768f', 'street': '#566164'}
        google = self.style == 'google'
        if google:
            colors = {'landmark': '#785da5', 'bridge': '#43515d',
                      'river': '#307b91', 'street': '#49545e'}
        visible = [(kind, name, wx / scale - left, wy / scale - top, length, wx, wy)
                   for kind, name, wx, wy, length in self.labels
                   if 20 < wx / scale - left < frame.width - 20
                   and 50 < wy / scale - top < frame.height - 20]
        draw = ImageDraw.Draw(frame)
        placed = []
        names = set()
        landmarks = []
        for kind, name, x, y, _, wx, wy in sorted(
            visible, key=lambda row: (priority[row[0]], row[1] not in LANDMARK_NAMES, -row[4])
        ):
            if kind == 'landmark':
                name = LANDMARK_NAMES.get(name, name)
                if self.zoom <= 16 and (len(landmarks) >= 3 or any(
                        math.dist((x, y), anchor) < 145 for anchor in landmarks)):
                    continue
            # A named street can be composed of dozens of OSM ways.
            if name in names:
                continue
            font = label_font(14 if kind == 'landmark' else 12, kind != 'street')
            if kind == 'landmark' and len(name) > 24 and ' ' in name:
                split = min((i for i, c in enumerate(name) if c == ' '), key=lambda i: abs(i - len(name) / 2))
                name = '\n'.join((name[:split], name[split + 1:]))
            multiline = '\n' in name
            anchor = 'mm' if not multiline else 'ma'
            box = draw.multiline_textbbox((x, y), name, font=font, anchor=anchor, spacing=1, stroke_width=1)
            stamp = None
            icon = None
            if google and kind in ('street', 'bridge', 'river'):
                shift = GEOMETRY_ZOOM - BUCKET_ZOOM + 8
                _, bearing = self._line_bearings(wx >> shift, wy >> shift).get((wx, wy), (0, 0))
                w, h = math.ceil(box[2] - box[0]) + 8, math.ceil(box[3] - box[1]) + 8
                stamp = Image.new('RGBA', (w, h))
                ImageDraw.Draw(stamp).text((4 + x - box[0], 4 + y - box[1]), name,
                                           fill=colors[kind], font=font, anchor=anchor,
                                           stroke_width=1, stroke_fill='#f9fbfa')
                stamp = stamp.rotate(-bearing, resample=Image.Resampling.BICUBIC, expand=True)
                px, py = round(x - stamp.width / 2), round(y - stamp.height / 2)
                box = (px, py, px + stamp.width, py + stamp.height)
            elif google and kind == 'landmark':
                icon = (box[0] - 9, y)
                box = (box[0] - 16, box[1], box[2], box[3])
            l, t, r, b = (box[0] - 5, box[1] - 3, box[2] + 5, box[3] + 3)
            if l < 8 or r > frame.width - 8 or b > frame.height - 8:
                continue
            if any(l < pr and r > pl and t < pb and b > pt for pl, pt, pr, pb in placed):
                continue
            if stamp is not None:
                frame.paste(stamp, (px, py), stamp)
            else:
                draw.multiline_text((x, y), name, fill=colors[kind], font=font,
                                    anchor=anchor, align='center', spacing=1,
                                    stroke_width=2, stroke_fill='#f7f5ee')
                if icon is not None:
                    ix, iy = icon
                    draw.ellipse((ix - 6, iy - 6, ix + 6, iy + 6),
                                 fill=colors[kind], outline='white', width=2)
                    draw.ellipse((ix - 1, iy - 1, ix + 1, iy + 1), fill='white')
            placed.append((l, t, r, b))
            names.add(name.replace('\n', ' '))
            if kind == 'landmark':
                landmarks.append((x, y))

    def sample(self):
        x, y = tile_math.deg2tile(self.center_lat, self.center_lon, self.zoom)
        size = config.TILE_SIZE
        frame = Image.new("RGB", (3 * size, 2 * size))
        for row in range(2):
            for col in range(3):
                frame.paste(self._get_tile(self.zoom, x + col - 1, y + row),
                            (col * size, row * size))
        return frame


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--pbf", type=Path, help="build geometry from an OSM PBF")
    source.add_argument("--geometry", type=Path, help="read the app's JSON buckets")
    parser.add_argument("--save-geometry", type=Path, help="keep the PBF build for future runs")
    parser.add_argument("--png", type=Path, help="save a six-tile sample by the Vistula")
    parser.add_argument("--reference", type=Path, help="compare with a matching Dart PNG sample")
    parser.add_argument("--gui", action="store_true", help="open the 800x480 map viewer")
    parser.add_argument('--style', choices=('cycling', 'google'), default='cycling')
    args = parser.parse_args()
    if args.save_geometry and (not args.pbf or args.save_geometry.exists()):
        parser.error("--save-geometry requires --pbf and a new destination directory")

    with TemporaryDirectory(prefix="krakow-python-") as staging:
        geometry = args.geometry or Path(staging)
        if args.pbf:
            print(f"Reading OSM PBF ({args.pbf.stat().st_size / 1e6:.0f} MB)", flush=True)
            started = perf_counter()
            count = build_geometry(args.pbf, geometry)
            print(f"Built {count} geometry buckets in {perf_counter() - started:.1f} s")
            if args.save_geometry:
                copytree(geometry, args.save_geometry)
        elif not any(geometry.glob("*-*.json")):
            parser.error("no geometry buckets found")
        preview = KrakowPreview(geometry, args.style)
        if args.png or args.reference:
            started = perf_counter()
            sample = preview.sample()
            print(f"Rendered {sample.width}x{sample.height} in {(perf_counter() - started) * 1000:.0f} ms")
            if args.png:
                args.png.parent.mkdir(parents=True, exist_ok=True)
                sample.save(args.png)
            if args.reference:
                with Image.open(args.reference) as png:
                    if png.size != sample.size:
                        parser.error("reference image must match the sample dimensions")
                    r, g, b = ImageChops.difference(sample, png.convert("RGB")).split()
                    n = ImageChops.lighter(ImageChops.lighter(r, g), b).histogram()[0]
                print(f"Exact pixel match: {n / (sample.width * sample.height):.1%}")
        if args.gui or not (args.png or args.reference):
            from ui import MapUI
            preview.center_lat, preview.center_lon, preview.zoom = 50.0524, 19.9368, 16
            MapUI(preview).run()


if __name__ == "__main__":
    main()
