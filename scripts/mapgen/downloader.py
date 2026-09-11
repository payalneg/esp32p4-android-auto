"""Скачивание OSM-тайлов области в tiles/{z}/{x}/{y}.jpg.

Однопоточно, с паузой между запросами — по политике tile.openstreetmap.org.
PNG конвертируется в JPEG: этот же формат будет читать аппаратный
JPEG-декодер ESP32-P4 с SD-карты.
"""
import io
import time

import requests
from PIL import Image

import config
import tile_math

REQUEST_PAUSE = 0.1  # секунд между запросами
JPEG_QUALITY = 85


def tile_path(z: int, x: int, y: int):
    return config.TILES_DIR / str(z) / str(x) / f"{y}.jpg"


def download_all() -> None:
    session = requests.Session()
    session.headers["User-Agent"] = config.USER_AGENT

    grand_total = 0
    for z in range(config.ZOOM_MIN, config.ZOOM_MAX + 1):
        xs, ys = tile_math.bbox_tiles(config.BBOX, z)
        total = len(xs) * len(ys)
        done = skipped = 0
        for x in xs:
            for y in ys:
                path = tile_path(z, x, y)
                if path.exists():
                    skipped += 1
                    done += 1
                    continue
                url = config.TILE_URL.format(z=z, x=x, y=y)
                resp = session.get(url, timeout=30)
                resp.raise_for_status()
                img = Image.open(io.BytesIO(resp.content)).convert("RGB")
                path.parent.mkdir(parents=True, exist_ok=True)
                img.save(path, "JPEG", quality=JPEG_QUALITY)
                done += 1
                if done % 25 == 0:
                    print(f"z={z}: {done}/{total}", flush=True)
                time.sleep(REQUEST_PAUSE)
        grand_total += total
        print(f"z={z}: готово {done}/{total} (пропущено из кэша: {skipped})", flush=True)
    print(f"Скачивание завершено, всего тайлов: {grand_total}")


if __name__ == "__main__":
    download_all()
