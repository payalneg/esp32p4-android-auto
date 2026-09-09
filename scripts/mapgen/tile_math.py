"""Тайловая математика slippy map (Web Mercator).

Чистые функции без зависимостей — переносятся на C (ESP32-P4) 1:1.
https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
"""
import math


def deg2tile_f(lat: float, lon: float, zoom: int) -> tuple[float, float]:
    """Градусы -> дробные тайловые координаты (x, y) на данном зуме."""
    n = 2 ** zoom
    x = (lon + 180.0) / 360.0 * n
    lat_rad = math.radians(lat)
    y = (1.0 - math.asinh(math.tan(lat_rad)) / math.pi) / 2.0 * n
    return x, y


def deg2tile(lat: float, lon: float, zoom: int) -> tuple[int, int]:
    """Градусы -> индексы тайла, с клампом в допустимый диапазон [0, 2^z - 1]."""
    x, y = deg2tile_f(lat, lon, zoom)
    n = 2 ** zoom
    return (
        min(max(int(x), 0), n - 1),
        min(max(int(y), 0), n - 1),
    )


def tile2deg(x: float, y: float, zoom: int) -> tuple[float, float]:
    """Тайловые координаты (можно дробные) -> градусы (lat, lon)."""
    n = 2 ** zoom
    lon = x / n * 360.0 - 180.0
    lat = math.degrees(math.atan(math.sinh(math.pi * (1.0 - 2.0 * y / n))))
    return lat, lon


def bbox_tiles(bbox: tuple[float, float, float, float], zoom: int) -> tuple[range, range]:
    """Диапазоны индексов тайлов, покрывающих bbox (север, запад, юг, восток)."""
    north, west, south, east = bbox
    x_min, y_min = deg2tile(north, west, zoom)
    x_max, y_max = deg2tile(south, east, zoom)
    return range(x_min, x_max + 1), range(y_min, y_max + 1)
