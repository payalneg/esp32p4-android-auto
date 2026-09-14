"""Движок карты: состояние (центр, зум) и сборка кадра из тайлов.

Не знает про tkinter — только Pillow и tile_math. На ESP32-P4 этот модуль
переписывается на C: Pillow заменяется на LVGL canvas + аппаратный JPEG.
"""
from collections import OrderedDict

from PIL import Image, ImageDraw

import config
import tile_math

TILE_CACHE_SIZE = 64


def _make_placeholder() -> Image.Image:
    """Серая заглушка с сеткой для тайлов, которых нет на диске."""
    img = Image.new("RGB", (config.TILE_SIZE, config.TILE_SIZE), (225, 225, 225))
    draw = ImageDraw.Draw(img)
    edge = config.TILE_SIZE - 1
    draw.rectangle((0, 0, edge, edge), outline=(200, 200, 200))
    return img


class MapEngine:
    ZOOM_LIMITS = (config.ZOOM_MIN, config.ZOOM_MAX)

    def __init__(self):
        self.center_lat, self.center_lon = config.START_CENTER
        self.zoom = config.START_ZOOM
        self._cache: OrderedDict[tuple[int, int, int], Image.Image] = OrderedDict()
        self._placeholder = _make_placeholder()

    # --- тайлы ---

    def _get_tile(self, z: int, x: int, y: int) -> Image.Image:
        key = (z, x, y)
        if key in self._cache:
            self._cache.move_to_end(key)
            return self._cache[key]
        path = config.TILES_DIR / str(z) / str(x) / f"{y}.jpg"
        if path.exists():
            tile = Image.open(path).convert("RGB")
        else:
            tile = self._placeholder
        self._cache[key] = tile
        if len(self._cache) > TILE_CACHE_SIZE:
            self._cache.popitem(last=False)
        return tile

    # --- отрисовка ---

    def render(self, width: int, height: int) -> Image.Image:
        """Собирает кадр width x height вокруг текущего центра."""
        ts = config.TILE_SIZE
        cx, cy = tile_math.deg2tile_f(self.center_lat, self.center_lon, self.zoom)
        # Мировые пиксельные координаты левого верхнего угла кадра
        left = cx * ts - width / 2
        top = cy * ts - height / 2

        frame = Image.new("RGB", (width, height), (225, 225, 225))
        n = 2 ** self.zoom
        tx_min, tx_max = int(left // ts), int((left + width) // ts)
        ty_min, ty_max = int(top // ts), int((top + height) // ts)
        for tx in range(tx_min, tx_max + 1):
            for ty in range(ty_min, ty_max + 1):
                if not (0 <= tx < n and 0 <= ty < n):
                    continue
                tile = self._get_tile(self.zoom, tx, ty)
                frame.paste(tile, (round(tx * ts - left), round(ty * ts - top)))
        return frame

    def geo_to_screen(self, lat: float, lon: float,
                      width: int, height: int) -> tuple[float, float]:
        """Координаты точки на экране относительно текущего кадра."""
        ts = config.TILE_SIZE
        cx, cy = tile_math.deg2tile_f(self.center_lat, self.center_lon, self.zoom)
        px, py = tile_math.deg2tile_f(lat, lon, self.zoom)
        return (px - cx) * ts + width / 2, (py - cy) * ts + height / 2

    def screen_to_geo(self, px: float, py: float,
                      width: int, height: int) -> tuple[float, float]:
        """Точка экрана -> (lat, lon) при текущем центре и зуме."""
        ts = config.TILE_SIZE
        cx, cy = tile_math.deg2tile_f(self.center_lat, self.center_lon, self.zoom)
        return tile_math.tile2deg(cx + (px - width / 2) / ts,
                                  cy + (py - height / 2) / ts, self.zoom)

    # --- навигация ---

    def pan(self, dx_px: float, dy_px: float) -> None:
        """Сдвиг карты на dx/dy пикселей (положительный dx — карта уходит влево)."""
        ts = config.TILE_SIZE
        cx, cy = tile_math.deg2tile_f(self.center_lat, self.center_lon, self.zoom)
        self._set_center(cx + dx_px / ts, cy + dy_px / ts)

    def zoom_at(self, px: float, py: float, delta: int,
                width: int, height: int) -> bool:
        """Зум на delta ступеней к точке курсора (px, py). True, если зум изменился."""
        new_zoom = max(self.ZOOM_LIMITS[0], min(self.zoom + delta, self.ZOOM_LIMITS[1]))
        if new_zoom == self.zoom:
            return False
        ts = config.TILE_SIZE
        # Географическая точка под курсором должна остаться под курсором
        cx, cy = tile_math.deg2tile_f(self.center_lat, self.center_lon, self.zoom)
        fx = cx + (px - width / 2) / ts
        fy = cy + (py - height / 2) / ts
        lat, lon = tile_math.tile2deg(fx, fy, self.zoom)
        self.zoom = new_zoom
        fx2, fy2 = tile_math.deg2tile_f(lat, lon, new_zoom)
        self._set_center(fx2 - (px - width / 2) / ts,
                         fy2 - (py - height / 2) / ts)
        return True

    def _set_center(self, tile_x: float, tile_y: float) -> None:
        """Устанавливает центр по тайловым координатам, не выпуская его за bbox."""
        lat, lon = tile_math.tile2deg(tile_x, tile_y, self.zoom)
        north, west, south, east = config.BBOX
        self.center_lat = min(max(lat, south), north)
        self.center_lon = min(max(lon, west), east)
