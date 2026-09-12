"""Генератор векторных тайлов: OSM PBF -> vtiles/{z}/{x}/{y}.vtile.

Аналог генератора карт Organic Maps в миниатюре:
  1) pyosmium вычитывает выгрузку региона, оставляя фичи внутри bbox;
  2) координаты нормализуются в Web Mercator [0..1];
  3) для каждого зума z8..z14 фичи фильтруются по важности, упрощаются
     под разрешение зума и раскладываются по тайлам.
"""
import math
from collections import defaultdict

import osmium

import config
import graph_format as gf
import profiles as prof
import vtile_format as vt
from search import normalize

# Запас вокруг BBOX, чтобы фичи на краю не обрезались
BBOX_MARGIN = 0.02

ROAD_CLASSES = {
    "motorway": 0, "motorway_link": 0, "trunk": 0, "trunk_link": 0,
    "primary": 1, "primary_link": 1,
    "secondary": 2, "secondary_link": 2, "tertiary": 2, "tertiary_link": 2,
    "residential": 3, "unclassified": 3, "living_street": 3, "pedestrian": 3,
    "service": 4, "track": 4,
    "footway": 5, "path": 5, "cycleway": 5, "steps": 5,
}
GREEN_LEISURE = {"park", "garden", "golf_course", "pitch", "playground",
                 "nature_reserve"}
GREEN_LANDUSE = {"forest", "grass", "meadow", "recreation_ground",
                 "village_green", "cemetery", "orchard", "allotments"}
GREEN_NATURAL = {"wood", "scrub", "heath", "grassland"}
WATER_AREA = lambda t: (t.get("natural") == "water"
                        or t.get("waterway") == "riverbank"
                        or t.get("landuse") in ("reservoir", "basin"))
PLACE_CLASSES = {"city": 0, "town": 1, "suburb": 2, "quarter": 2,
                 "village": 3, "neighbourhood": 3}

# Велосипед: скорость по типу дороги, км/ч (footway — пешком с велосипедом)
# Скорости переехали в profiles.py: граф хранит класс дороги, а во что он
# обходится, решает профиль при запросе маршрута.

# Минимальный зум, на котором фича попадает в тайлы: (layer, cls) -> zoom
MIN_ZOOM = {
    (vt.LAYER_ROADS, 0): 8, (vt.LAYER_ROADS, 1): 9, (vt.LAYER_ROADS, 2): 10,
    (vt.LAYER_ROADS, 3): 13, (vt.LAYER_ROADS, 4): 14, (vt.LAYER_ROADS, 5): 14,
    (vt.LAYER_RAIL, 0): 9, (vt.LAYER_RAIL, 1): 13,
    (vt.LAYER_WATER, 0): 8, (vt.LAYER_WATER, 1): 13,
    (vt.LAYER_LANDUSE, 0): 11,
    (vt.LAYER_BUILDINGS, 0): 14,
    (vt.LAYER_LABELS, 0): 8, (vt.LAYER_LABELS, 1): 9,
    (vt.LAYER_LABELS, 2): 12, (vt.LAYER_LABELS, 3): 13,
}


def _merc(lon: float, lat: float) -> tuple[float, float]:
    """Градусы -> Web Mercator, нормированный в [0..1]."""
    x = (lon + 180.0) / 360.0
    lat = min(max(lat, -85.0511), 85.0511)
    y = (1.0 - math.asinh(math.tan(math.radians(lat))) / math.pi) / 2.0
    return x, y


class Collector(osmium.SimpleHandler):
    """Собирает фичи внутри bbox в нормированных меркаторских координатах."""

    def __init__(self):
        super().__init__()
        north, west, south, east = config.BBOX
        self.lon_min, self.lon_max = west - BBOX_MARGIN, east + BBOX_MARGIN
        self.lat_min, self.lat_max = south - BBOX_MARGIN, north + BBOX_MARGIN
        # (layer, cls, name, [(mx, my)...], geom_type)
        self.features: list[tuple[int, int, str, list, int]] = []
        # поисковый индекс: (norm, display) -> (kind, lat, lon)
        self.search: dict[tuple[str, str], tuple[str, float, float]] = {}
        # велодороги: (speed_kmh, direction, [node_ref...], [(lon, lat)...])
        # direction: 0 — в обе стороны, 1 — по ходу, -1 — против
        self.bike_ways: list[tuple[float, int, list[int], list]] = []

    def _inside(self, coords) -> bool:
        return any(self.lon_min <= lon <= self.lon_max
                   and self.lat_min <= lat <= self.lat_max
                   for lon, lat in coords)

    def _add(self, layer, cls, name, coords, geom):
        if len(coords) >= (3 if geom == vt.GEOM_POLYGON else 2) or geom == vt.GEOM_POINT:
            if self._inside(coords):
                self.features.append(
                    (layer, cls, name, [_merc(lon, lat) for lon, lat in coords], geom))

    def _add_search(self, display: str, kind: str, lon: float, lat: float):
        display = " ".join(display.split())  # без табов и переводов строк
        if display and (self.lon_min <= lon <= self.lon_max
                        and self.lat_min <= lat <= self.lat_max):
            self.search[(normalize(display), display)] = (kind, lat, lon)

    def _extract_search(self, tags, lon: float, lat: float):
        """Адрес и/или именованный POI из набора тегов объекта."""
        street, housenumber = tags.get("addr:street"), tags.get("addr:housenumber")
        if street and housenumber:
            self._add_search(f"{street} {housenumber}", "адрес", lon, lat)
        name = tags.get("name")
        kind = tags.get("shop") or tags.get("amenity") or tags.get("tourism")
        if name and kind:
            self._add_search(name, kind, lon, lat)

    # линейные объекты
    def way(self, w):
        tags = w.tags
        try:
            coords = [(n.lon, n.lat) for n in w.nodes]
        except osmium.InvalidLocationError:
            return
        hw = tags.get("highway")
        if hw in prof.HIGHWAY_CLASS:
            self._collect_bike_way(w, tags, hw, coords)
        if hw in ROAD_CLASSES:
            self._add(vt.LAYER_ROADS, ROAD_CLASSES[hw],
                      tags.get("name") or "", coords, vt.GEOM_LINE)
            return
        rw = tags.get("railway")
        if rw in ("rail", "light_rail", "subway"):
            self._add(vt.LAYER_RAIL, 0, "", coords, vt.GEOM_LINE)
            return
        if rw in ("tram", "narrow_gauge"):
            self._add(vt.LAYER_RAIL, 1, "", coords, vt.GEOM_LINE)
            return
        ww = tags.get("waterway")
        if ww in ("river", "canal"):
            self._add(vt.LAYER_WATER, 0, "", coords, vt.GEOM_LINE)
        elif ww == "stream":
            self._add(vt.LAYER_WATER, 1, "", coords, vt.GEOM_LINE)

    def _collect_bike_way(self, w, tags, hw, coords):
        if not self._inside(coords):
            return
        bicycle = tags.get("bicycle")
        if tags.get("access") in ("no", "private") and bicycle != "yes":
            return
        cls = prof.HIGHWAY_CLASS[hw]
        # Тротуар и переход — отдельные классы: на самокате это полноценные
        # варианты проезда, а не штрафные пешеходные дорожки.
        if hw == "footway":
            fw = tags.get("footway")
            if fw == "sidewalk":
                cls = "sidewalk"
            elif fw == "crossing":
                cls = "crossing"
        elif hw == "path" and tags.get("path") == "crossing":
            cls = "crossing"
        # дорожка, где езда явно разрешена, функционально равна велодорожке
        if cls in ("footway", "path", "pedestrian", "sidewalk") and \
                bicycle in ("yes", "designated"):
            cls = "cycleway"
        # bicycle=no/use_sidepath намеренно НЕ отбрасываем: класс дороги уже
        # сказан, а можно ли по нему ехать — решает профиль.
        # односторонность для велосипеда
        direction = 0
        oneway = tags.get("oneway")
        ob = tags.get("oneway:bicycle")
        contraflow = any(v.startswith("opposite")
                         for k, v in ((t.k, t.v) for t in w.tags)
                         if k.startswith("cycleway"))
        if ob == "no" or contraflow:
            direction = 0
        elif oneway in ("yes", "1", "true") or ob == "yes":
            direction = 1
        elif oneway == "-1":
            direction = -1
        refs = [n.ref for n in w.nodes]
        self.bike_ways.append((cls, direction, refs, coords))

    # площадные объекты (osmium сам собирает мультиполигоны)
    def area(self, a):
        tags = {t.k: t.v for t in a.tags}
        if "addr:housenumber" in tags or "name" in tags:
            try:
                ring = next(iter(a.outer_rings()))
                lons = [n.lon for n in ring]
                lats = [n.lat for n in ring]
                self._extract_search(tags, sum(lons) / len(lons),
                                     sum(lats) / len(lats))
            except (osmium.InvalidLocationError, StopIteration):
                pass
        name = ""
        if "building" in tags:
            layer, cls = vt.LAYER_BUILDINGS, 0
            name = tags.get("addr:housenumber", "")  # номер дома на здании
        elif WATER_AREA(tags):
            layer, cls = vt.LAYER_WATER, 0
        elif (tags.get("leisure") in GREEN_LEISURE
              or tags.get("landuse") in GREEN_LANDUSE
              or tags.get("natural") in GREEN_NATURAL):
            layer, cls = vt.LAYER_LANDUSE, 0
        else:
            return
        try:
            for outer in a.outer_rings():
                coords = [(n.lon, n.lat) for n in outer]
                self._add(layer, cls, name, coords, vt.GEOM_POLYGON)
        except osmium.InvalidLocationError:
            return

    # подписи населённых пунктов + точечные адреса и POI
    def node(self, n):
        tags = n.tags
        place = tags.get("place")
        if place in PLACE_CLASSES and "name" in tags:
            self._add(vt.LAYER_LABELS, PLACE_CLASSES[place], tags["name"],
                      [(n.location.lon, n.location.lat)], vt.GEOM_POINT)
            # И в поиск: населённый пункт — это цель поездки сам по себе.
            # Без этой строки в индексе были все дома на Тынецкой и не было
            # самого Тынца, так что найти посёлок было нельзя — только улицу,
            # названную в его честь.
            self._add_search(tags["name"], place,
                             n.location.lon, n.location.lat)
        if ("addr:housenumber" in tags or "shop" in tags
                or "amenity" in tags or "tourism" in tags):
            self._extract_search(tags, n.location.lon, n.location.lat)


def _haversine(lat1, lon1, lat2, lon2) -> float:
    """Расстояние в метрах."""
    r = 6371000.0
    p1, p2 = math.radians(lat1), math.radians(lat2)
    dp, dl = p2 - p1, math.radians(lon2 - lon1)
    a = math.sin(dp / 2) ** 2 + math.cos(p1) * math.cos(p2) * math.sin(dl / 2) ** 2
    return 2 * r * math.asin(math.sqrt(a))


def build_graph(bike_ways):
    """Дороги -> граф: вершины на перекрёстках и концах, рёбра с геометрией."""
    from collections import Counter
    use = Counter()
    for _, _, refs, _ in bike_ways:
        use.update(refs)
        use[refs[0]] += 1  # концы дорог — всегда вершины
        use[refs[-1]] += 1

    node_index: dict[int, int] = {}
    nodes: list[tuple[float, float]] = []
    edges: list[dict] = []

    def vertex(ref: int, lonlat) -> int:
        if ref not in node_index:
            node_index[ref] = len(nodes)
            nodes.append((lonlat[1], lonlat[0]))
        return node_index[ref]

    for cls, direction, refs, coords in bike_ways:
        if len(refs) < 2:
            continue
        seg_refs, seg_pts = [refs[0]], [coords[0]]
        for i in range(1, len(refs)):
            seg_refs.append(refs[i])
            seg_pts.append(coords[i])
            if use[refs[i]] < 2 and i != len(refs) - 1:
                continue
            # закончился отрезок между вершинами -> ребро
            pts = [(lat, lon) for lon, lat in seg_pts]
            length = sum(_haversine(*a, *b) for a, b in zip(pts, pts[1:]))
            if length > 0:
                src = vertex(seg_refs[0], seg_pts[0])
                dst = vertex(seg_refs[-1], seg_pts[-1])
                if direction == -1:
                    src, dst = dst, src
                    pts.reverse()
                edges.append({
                    "from": src, "to": dst,
                    "len_dm": round(length * 10),
                    "way_class": prof.CLASS_INDEX[cls],
                    "flags": gf.FLAG_BIDIR if direction == 0 else 0,
                    "points": pts,
                })
            seg_refs, seg_pts = [refs[i]], [coords[i]]
    return gf.largest_component(nodes, edges)


def _simplify(points: list[tuple[int, int]], tolerance: int) -> list[tuple[int, int]]:
    """Прореживание: точка остаётся, если ушла от предыдущей на >= tolerance."""
    if tolerance <= 0 or len(points) <= 2:
        return points
    out = [points[0]]
    for p in points[1:-1]:
        if max(abs(p[0] - out[-1][0]), abs(p[1] - out[-1][1])) >= tolerance:
            out.append(p)
    out.append(points[-1])
    return out


def _tolerance(layer: int, zoom: int) -> int:
    # Тайл рисуется в 256px => 1px = EXTENT/256 = 16 юнитов. Держать детали
    # мельче пикселя бессмысленно и дорого для устройства.
    if zoom >= config.VZOOM_MAX_DATA:
        return 4   # z14 оверзумится до z18 (x16) — оставляем запас детализации
    return 16


def build_tiles(features) -> None:
    north, west, south, east = config.BBOX
    total_written = 0
    for zoom in range(config.VZOOM_MIN, config.VZOOM_MAX_DATA + 1):
        scale = (1 << zoom) * vt.EXTENT  # мерк. [0..1] -> глобальные юниты зума
        tiles: dict[tuple[int, int], dict[int, list]] = defaultdict(
            lambda: defaultdict(list))
        # допустимый диапазон тайлов зума (bbox с запасом)
        gx0, gy0 = _merc(west - BBOX_MARGIN, north + BBOX_MARGIN)
        gx1, gy1 = _merc(east + BBOX_MARGIN, south - BBOX_MARGIN)
        txr = range(int(gx0 * (1 << zoom)), int(gx1 * (1 << zoom)) + 1)
        tyr = range(int(gy0 * (1 << zoom)), int(gy1 * (1 << zoom)) + 1)

        for layer, cls, name, mcoords, geom in features:
            if zoom < MIN_ZOOM.get((layer, cls), 99):
                continue
            pts = [(round(mx * scale), round(my * scale)) for mx, my in mcoords]
            tris = None
            if geom != vt.GEOM_POINT:
                tol = _tolerance(layer, zoom)
                pts = _simplify(pts, tol)
                if geom == vt.GEOM_POLYGON:
                    # крупные кольца упрощаем жёстче: ear clipping о(n^2)
                    while len(pts) > 400:
                        tol *= 2
                        pts = _simplify(pts, tol)
                    # мелкие полигоны не видны — выбрасываем
                    # (на обзорных зумах порог жёстче: ~3px; на z14 — 1.5px,
                    # чтобы не тащить каждый сарай)
                    min_size = 48 if zoom <= 12 else (24 if zoom >= 14 else 16)
                    xs = [p[0] for p in pts]
                    ys = [p[1] for p in pts]
                    if max(xs) - min(xs) < min_size and max(ys) - min(ys) < min_size:
                        continue
                    tris = vt.triangulate(pts)
                if len(pts) < (3 if geom == vt.GEOM_POLYGON else 2):
                    continue
            xs = [p[0] for p in pts]
            ys = [p[1] for p in pts]
            pad = 128  # буфер, чтобы обводка на границе тайла не рвалась
            for tx in range(max((min(xs) - pad) // vt.EXTENT, txr.start),
                            min((max(xs) + pad) // vt.EXTENT, txr.stop - 1) + 1):
                for ty in range(max((min(ys) - pad) // vt.EXTENT, tyr.start),
                                min((max(ys) + pad) // vt.EXTENT, tyr.stop - 1) + 1):
                    ox, oy = tx * vt.EXTENT, ty * vt.EXTENT
                    local = [(x - ox, y - oy) for x, y in pts]
                    # имена улиц/домов нужны только на детальных зумах
                    tile_name = name if (zoom >= 13 or layer == vt.LAYER_LABELS) else ""
                    tiles[(tx, ty)][layer].append(
                        {"geom": geom, "cls": cls, "name": tile_name,
                         "points": local, "tris": tris})

        for (tx, ty), layers in tiles.items():
            path = config.VTILES_DIR / str(zoom) / str(tx) / f"{ty}.vtile"
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(vt.pack_tile(layers))
        total_written += len(tiles)
        print(f"z={zoom}: {len(tiles)} тайлов", flush=True)
    print(f"Готово: {total_written} тайлов в {config.VTILES_DIR}")


def build() -> None:
    if not config.PBF_PATH.exists():
        raise SystemExit(f"Нет {config.PBF_PATH} — сначала: python main.py --download-pbf")
    print("Читаю PBF (несколько минут)...", flush=True)
    collector = Collector()
    collector.apply_file(str(config.PBF_PATH), locations=True)
    stats = defaultdict(int)
    for layer, *_ in collector.features:
        stats[layer] += 1
    print("Фич в bbox:", dict(stats), flush=True)
    build_tiles(collector.features)

    config.SEARCH_INDEX.parent.mkdir(parents=True, exist_ok=True)
    with open(config.SEARCH_INDEX, "w", encoding="utf-8") as f:
        for (norm, display), (kind, lat, lon) in sorted(collector.search.items()):
            f.write(f"{norm}\t{display}\t{kind}\t{lat:.6f}\t{lon:.6f}\n")
    print(f"Поисковый индекс: {len(collector.search)} записей "
          f"в {config.SEARCH_INDEX}")

    nodes, edges = build_graph(collector.bike_ways)
    config.ROUTE_GRAPH.write_bytes(gf.pack_graph(nodes, edges))
    print(f"Велограф: {len(nodes)} вершин, {len(edges)} рёбер "
          f"({config.ROUTE_GRAPH.stat().st_size // 1024} КБ)")


if __name__ == "__main__":
    build()
