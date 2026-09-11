"""Настройки карты Кракова."""
from pathlib import Path

# Границы области: север, запад, юг, восток (Краков с запасом)
BBOX = (50.13, 19.79, 49.97, 20.22)

ZOOM_MIN = 11
ZOOM_MAX = 16

TILE_URL = "https://tile.openstreetmap.org/{z}/{x}/{y}.png"
TILE_SIZE = 256

# Политика OSM требует осмысленный User-Agent
USER_AGENT = "KrakowMapPrototype/0.1 (educational project; alexey.nenashev@soteranalytics.com)"

TILES_DIR = Path(__file__).parent / "tiles"

# Рыночная площадь (Rynek Główny)
START_CENTER = (50.0619, 19.9368)
START_ZOOM = 13

# Разрешение целевого дисплея ESP32-P4; окно эмулирует экран устройства
WINDOW_SIZE = (800, 480)

# --- векторная карта ---
DATA_DIR = Path(__file__).parent / "data"
PBF_URL = "https://download.geofabrik.de/europe/poland/malopolskie-latest.osm.pbf"
PBF_PATH = DATA_DIR / "malopolskie-latest.osm.pbf"
VTILES_DIR = Path(__file__).parent / "vtiles"
SEARCH_INDEX = VTILES_DIR / "search_index.tsv"
ROUTE_GRAPH = VTILES_DIR / "route_graph.bin"

VZOOM_MIN = 8        # минимальный зум просмотра и данных
VZOOM_MAX_DATA = 14  # тайлы с данными строятся до этого зума
VZOOM_MAX_VIEW = 18  # дальше — оверзум: данные z14 масштабируются
