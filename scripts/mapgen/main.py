"""Оффлайн-карта Кракова (векторная, с прицелом на порт ESP32-P4).

    python main.py --download-pbf   скачать выгрузку OSM региона (Geofabrik)
    python main.py --build          собрать векторные тайлы из выгрузки
    python main.py                  открыть векторную карту
    python main.py --raster         открыть растровую карту (старый режим)
    python main.py --download       скачать растровые тайлы OSM
"""
import argparse
import sys

import config


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--download-pbf", action="store_true",
                        help="скачать выгрузку OSM региона")
    parser.add_argument("--build", action="store_true",
                        help="собрать векторные тайлы из выгрузки")
    parser.add_argument("--raster", action="store_true",
                        help="растровый режим вместо векторного")
    parser.add_argument("--download", action="store_true",
                        help="скачать растровые тайлы области")
    args = parser.parse_args()

    if args.download_pbf:
        # requests (тянет корневые сертификаты из certifi), а не urllib:
        # у python.org-сборок под macOS нет системного CA-бандла и
        # urlretrieve падает с CERTIFICATE_VERIFY_FAILED.
        import requests
        config.DATA_DIR.mkdir(exist_ok=True)
        print(f"Скачиваю {config.PBF_URL} ...")
        with requests.get(config.PBF_URL, stream=True, timeout=60) as resp:
            resp.raise_for_status()
            total = int(resp.headers.get("content-length", 0))
            done = 0
            with open(config.PBF_PATH, "wb") as f:
                for chunk in resp.iter_content(1 << 20):
                    f.write(chunk)
                    done += len(chunk)
                    if total:
                        print(f"\r  {done / total:.0%} ({done >> 20} МБ)",
                              end="", flush=True)
            print()
        print(f"Готово: {config.PBF_PATH}")
        return

    if args.build:
        from builder import build
        build()
        return

    if args.download:
        from downloader import download_all
        download_all()
        return

    if args.raster:
        if not config.TILES_DIR.exists():
            sys.exit("Растровых тайлов нет. Сначала: python main.py --download")
        from ui import MapUI
        MapUI().run()
        return

    if not config.VTILES_DIR.exists():
        sys.exit("Векторных тайлов нет. Сначала: python main.py --download-pbf "
                 "и python main.py --build")
    from ui import MapUI
    from vector_engine import VectorMapEngine
    MapUI(VectorMapEngine()).run()


if __name__ == "__main__":
    main()
