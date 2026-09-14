"""Векторный движок: рендер .vtile в битмапы тайлов + подписи.

Наследует навигацию (пан/зум/кадр) от MapEngine, но вместо чтения JPEG
рисует тайл из векторных данных. Выше VZOOM_MAX_DATA работает оверзум:
данные z14 масштабируются — зум становится «бесплатным», как в Organic Maps.

Всё рисование — Pillow; на ESP32-P4 то же самое делается LVGL-канвой
(линии/полигоны) поверх того же формата vtile.
"""
import math
from collections import OrderedDict

from PIL import Image, ImageDraw, ImageFont

import config
import tile_math
import vtile_format as vt
from map_engine import MapEngine

SS = 2  # суперсэмплинг: рисуем 512px, отдаём 256px — дешёвый антиалиасинг

BG = (241, 238, 232)
STYLE = {
    "green": (205, 235, 176),
    "water": (170, 211, 223),
    "building": (217, 208, 201),
    "building_line": (194, 181, 165),
    "rail": (112, 112, 112),
    "tram": (150, 150, 150),
    "label": (60, 60, 60),
    "halo": (255, 255, 255),
}
# Дороги: класс -> (заливка, обводка); рисуются от младших к старшим
ROAD_FILL = {
    0: ((233, 144, 160), (208, 90, 110)),
    1: ((252, 214, 164), (205, 155, 90)),
    2: ((255, 242, 184), (200, 185, 120)),
    3: ((255, 255, 255), (190, 190, 190)),
    4: ((255, 255, 255), (205, 205, 205)),
    5: ((150, 130, 110), None),  # тропинки: тонкая линия без обводки
}
# Базовая ширина дороги в пикселях на z13 (масштабируется 2^(v-13))
# и потолок ширины, чтобы на больших зумах дороги не разбухали
ROAD_WIDTH = {0: 5.0, 1: 4.5, 2: 3.5, 3: 2.5, 4: 1.5, 5: 1.0}
ROAD_WIDTH_MAX = {0: 16.0, 1: 14.0, 2: 11.0, 3: 8.0, 4: 4.0, 5: 1.5}

LABEL_FONT_SIZE = {0: 17, 1: 14, 2: 12, 3: 11}
LABEL_MAX_ZOOM = {0: 15, 1: 16, 2: 17, 3: 17}

STREET_NAME_MIN_ZOOM = 15   # с какого зума подписывать улицы
HOUSE_NUMBER_MIN_ZOOM = 17  # с какого зума показывать номера домов


def _load_font(size: int):
    for name in ("/System/Library/Fonts/Helvetica.ttc", "Arial.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default()


class VectorMapEngine(MapEngine):
    ZOOM_LIMITS = (config.VZOOM_MIN, config.VZOOM_MAX_VIEW)

    def __init__(self):
        super().__init__()
        self._vtile_cache: OrderedDict[tuple, dict | None] = OrderedDict()
        self._fonts = {cls: _load_font(sz) for cls, sz in LABEL_FONT_SIZE.items()}
        self._street_font = _load_font(11 * SS)
        self._house_font = _load_font(9 * SS)
        self._empty = Image.new("RGB", (config.TILE_SIZE, config.TILE_SIZE), BG)

    # --- данные ---

    def _load_vtile(self, z: int, x: int, y: int) -> dict | None:
        """Распарсенный тайл данных с диска, с LRU-кэшем. None — тайла нет."""
        key = (z, x, y)
        if key in self._vtile_cache:
            self._vtile_cache.move_to_end(key)
            return self._vtile_cache[key]
        path = config.VTILES_DIR / str(z) / str(x) / f"{y}.vtile"
        tile = vt.parse_tile(path.read_bytes()) if path.exists() else None
        self._vtile_cache[key] = tile
        if len(self._vtile_cache) > 32:
            self._vtile_cache.popitem(last=False)
        return tile

    def _data_zoom(self) -> int:
        return min(self.zoom, config.VZOOM_MAX_DATA)

    # --- отрисовка тайла (вызывается кадросборкой MapEngine.render) ---

    def _get_tile(self, z: int, x: int, y: int) -> Image.Image:
        key = (z, x, y)
        if key in self._cache:
            self._cache.move_to_end(key)
            return self._cache[key]
        img = self._render_tile(z, x, y)
        self._cache[key] = img
        if len(self._cache) > 128:
            self._cache.popitem(last=False)
        return img

    def _render_tile(self, v: int, tx: int, ty: int) -> Image.Image:
        d = min(v, config.VZOOM_MAX_DATA)
        shift = v - d
        data = self._load_vtile(d, tx >> shift, ty >> shift)
        if not data:
            return self._empty
        size = config.TILE_SIZE * SS
        # локальные юниты исходного тайла -> пиксели этого видового тайла
        span = vt.EXTENT >> shift                      # юнитов на видовой тайл
        off_x = (tx - ((tx >> shift) << shift)) * span
        off_y = (ty - ((ty >> shift) << shift)) * span
        k = size / span

        img = Image.new("RGB", (size, size), BG)
        draw = ImageDraw.Draw(img)

        def xy(points):
            return [((px - off_x) * k, (py - off_y) * k) for px, py in points]

        for layer_id in vt.DRAW_ORDER:
            feats = data.get(layer_id)
            if not feats:
                continue
            if layer_id == vt.LAYER_ROADS:
                self._draw_roads(draw, feats, xy, v)
            elif layer_id == vt.LAYER_LABELS:
                continue  # подписи рисуются поверх кадра, не в тайле
            else:
                self._draw_layer(draw, layer_id, feats, xy, v)
        if v >= HOUSE_NUMBER_MIN_ZOOM:
            self._draw_house_numbers(draw, data.get(vt.LAYER_BUILDINGS, []), xy)
        if v >= STREET_NAME_MIN_ZOOM:
            self._draw_street_names(img, data.get(vt.LAYER_ROADS, []), xy)
        return img.resize((config.TILE_SIZE,) * 2, Image.LANCZOS)

    def _draw_house_numbers(self, draw, feats, xy):
        for f in feats:
            num = f["name"]
            if not num:
                continue
            pts = xy(f["points"])
            cx = sum(p[0] for p in pts) / len(pts)
            cy = sum(p[1] for p in pts) / len(pts)
            # номер рисуем, только если он помещается в здание
            bw = max(p[0] for p in pts) - min(p[0] for p in pts)
            if self._house_font.getlength(num) > bw * 0.9:
                continue
            draw.text((cx, cy), num, font=self._house_font,
                      fill=(120, 110, 100), anchor="mm")

    def _draw_street_names(self, img, feats, xy):
        drawn: set[str] = set()
        size = img.size[0]
        font = self._street_font
        for f in feats:
            name = f["name"]
            if not name or name in drawn:
                continue
            pts = xy(f["points"])
            length = sum(math.dist(a, b) for a, b in zip(pts, pts[1:]))
            text_w = font.getlength(name)
            if length < text_w * 1.3:
                continue  # улица короче своего названия
            # точка и направление в середине полилинии
            target, acc = length / 2, 0.0
            a, b = pts[0], pts[1]
            for a, b in zip(pts, pts[1:]):
                seg = math.dist(a, b)
                if acc + seg >= target:
                    break
                acc += seg
            t = (target - acc) / seg if seg else 0
            mx, my = a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t
            if not (0 <= mx < size and 0 <= my < size):
                continue  # середина в соседнем тайле — там и подпишется
            angle = math.degrees(math.atan2(-(b[1] - a[1]), b[0] - a[0]))
            if angle > 90:
                angle -= 180
            elif angle < -90:
                angle += 180
            self._paste_rotated_text(img, name, mx, my, angle, font)
            drawn.add(name)

    def _paste_rotated_text(self, img, text, x, y, angle, font):
        pad = 3 * SS
        w = int(font.getlength(text)) + 2 * pad
        h = font.size + 2 * pad
        timg = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        td = ImageDraw.Draw(timg)
        for ox in (-SS, 0, SS):
            for oy in (-SS, 0, SS):
                if ox or oy:
                    td.text((pad + ox, pad + oy), text, font=font,
                            fill=STYLE["halo"])
        td.text((pad, pad), text, font=font, fill=(80, 80, 80))
        timg = timg.rotate(angle, expand=True, resample=Image.BICUBIC)
        img.paste(timg, (int(x - timg.width / 2), int(y - timg.height / 2)),
                  timg)

    def _draw_layer(self, draw, layer_id, feats, xy, v):
        for f in feats:
            pts = xy(f["points"])
            if layer_id == vt.LAYER_LANDUSE:
                draw.polygon(pts, fill=STYLE["green"])
            elif layer_id == vt.LAYER_WATER:
                if f["geom"] == vt.GEOM_POLYGON:
                    draw.polygon(pts, fill=STYLE["water"])
                else:
                    w = SS * (3 if f["cls"] == 0 else 1) * max(1, 2 ** (v - 12))
                    draw.line(pts, fill=STYLE["water"], width=int(w), joint="curve")
            elif layer_id == vt.LAYER_BUILDINGS:
                outline = STYLE["building_line"] if v >= 15 else None
                draw.polygon(pts, fill=STYLE["building"], outline=outline)
            elif layer_id == vt.LAYER_RAIL:
                color = STYLE["rail"] if f["cls"] == 0 else STYLE["tram"]
                draw.line(pts, fill=color, width=SS, joint="curve")

    def _draw_roads(self, draw, feats, xy, v):
        by_cls: dict[int, list] = {}
        for f in feats:
            by_cls.setdefault(f["cls"], []).append(xy(f["points"]))
        scale = 2 ** (v - 13)
        # обводки, потом заливки; младшие классы — под старшими
        for pass_fill in (False, True):
            for cls in sorted(by_cls, reverse=True):
                fill, casing = ROAD_FILL[cls]
                w = min(max(ROAD_WIDTH[cls] * scale, 1.0), ROAD_WIDTH_MAX[cls]) * SS
                if not pass_fill:
                    if casing is None or w < 2 * SS:
                        continue
                    for pts in by_cls[cls]:
                        draw.line(pts, fill=casing, width=int(w + 2 * SS),
                                  joint="curve")
                else:
                    for pts in by_cls[cls]:
                        draw.line(pts, fill=fill, width=int(w), joint="curve")

    # --- подписи поверх собранного кадра ---

    def render(self, width: int, height: int) -> Image.Image:
        frame = super().render(width, height)
        self._draw_labels(frame, width, height)
        return frame

    def _draw_labels(self, frame, width, height):
        draw = ImageDraw.Draw(frame)
        v = self.zoom
        d = self._data_zoom()
        shift = v - d
        ts = config.TILE_SIZE
        cx, cy = tile_math.deg2tile_f(self.center_lat, self.center_lon, v)
        # видимые тайлы данных
        dx_min = int((cx - width / 2 / ts)) >> shift
        dx_max = int((cx + width / 2 / ts)) >> shift
        dy_min = int((cy - height / 2 / ts)) >> shift
        dy_max = int((cy + height / 2 / ts)) >> shift
        drawn: set[str] = set()
        for dx in range(dx_min, dx_max + 1):
            for dy in range(dy_min, dy_max + 1):
                data = self._load_vtile(d, dx, dy)
                if not data:
                    continue
                for f in data.get(vt.LAYER_LABELS, []):
                    if v > LABEL_MAX_ZOOM.get(f["cls"], 15) or f["name"] in drawn:
                        continue
                    ux, uy = f["points"][0]
                    if not (0 <= ux < vt.EXTENT and 0 <= uy < vt.EXTENT):
                        continue  # дубликат из соседнего тайла (буфер)
                    fx = (dx + ux / vt.EXTENT) * (1 << shift)  # видовые тайлы
                    fy = (dy + uy / vt.EXTENT) * (1 << shift)
                    sx = (fx - cx) * ts + width / 2
                    sy = (fy - cy) * ts + height / 2
                    if -50 <= sx <= width + 50 and -20 <= sy <= height + 20:
                        self._text_halo(draw, sx, sy, f["name"], f["cls"])
                        drawn.add(f["name"])

    def _text_halo(self, draw, x, y, text, cls):
        font = self._fonts[cls]
        for ox in (-1, 0, 1):
            for oy in (-1, 0, 1):
                if ox or oy:
                    draw.text((x + ox, y + oy), text, font=font,
                              fill=STYLE["halo"], anchor="mm")
        draw.text((x, y), text, font=font, fill=STYLE["label"], anchor="mm")
