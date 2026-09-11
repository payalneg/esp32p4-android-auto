"""Профили маршрутизации: во что обходится каждый класс дороги.

Скорость намеренно НЕ запекается в граф. Граф хранит класс дороги, а
стоимость ребра считается при запросе — поэтому профиль переключается на
лету, без пересборки восемнадцатимегабайтного файла.

Скорость None означает «этим классом не ехать»: ребро просто не
раскрывается в A*.
"""

# Индекс класса пишется в граф одним байтом — порядок менять нельзя,
# только дописывать в конец.
WAY_CLASSES = (
    "cycleway",     # 0  выделенная велодорожка (и footway/path с bicycle=designated)
    "tertiary",     # 1
    "residential",  # 2  включая unclassified и living_street
    "secondary",    # 3
    "primary",      # 4
    "service",      # 5  проезды, дворы, парковки
    "path",         # 6  тропы
    "track",        # 7  грунтовки
    "pedestrian",   # 8  пешеходные улицы
    "footway",      # 9  пешеходные дорожки (парки, скверы)
    "sidewalk",     # 10 тротуар вдоль дороги (footway=sidewalk)
    "crossing",     # 11 пешеходный переход (footway=crossing)
    "steps",        # 12 лестницы
    "link",         # 13 искусственная связка тротуар<->дорога, см. builder
)
CLASS_INDEX = {name: i for i, name in enumerate(WAY_CLASSES)}

# highway=* -> класс. Всё, чего здесь нет, в граф не попадает вообще.
HIGHWAY_CLASS = {
    "cycleway": "cycleway",
    "tertiary": "tertiary", "tertiary_link": "tertiary",
    "residential": "residential", "unclassified": "residential",
    "living_street": "residential",
    "secondary": "secondary", "secondary_link": "secondary",
    "primary": "primary", "primary_link": "primary",
    "service": "service",
    "path": "path", "track": "track",
    "pedestrian": "pedestrian", "footway": "footway", "steps": "steps",
}

# Классы, по которым едут пешком/на тротуаре — нужны для анализа связности.
FOOT_CLASSES = ("pedestrian", "footway", "sidewalk", "crossing", "steps")

# км/ч по классам; None = запрещено
PROFILES: dict[str, dict[str, float | None]] = {
    "Велосипед": {
        "cycleway": 18, "tertiary": 16, "residential": 15, "secondary": 15,
        "primary": 13, "service": 13, "path": 12, "track": 12,
        "pedestrian": 6, "footway": 5, "sidewalk": 5, "crossing": 5,
        "steps": 2, "link": 4,
    },
    # Самокат: по дорогам как велосипед, но тротуар — полноценный вариант,
    # а не штраф. Ровно то, как на нём ездят в городе.
    "Самокат": {
        "cycleway": 20, "tertiary": 18, "residential": 18, "secondary": 16,
        "primary": 14, "service": 14, "path": 12, "track": 10,
        "pedestrian": 12, "footway": 12, "sidewalk": 15, "crossing": 8,
        "steps": None, "link": 5,
    },
    # Только тротуары и дорожки: проезжая часть запрещена.
    "Только тротуары": {
        "cycleway": 18, "tertiary": None, "residential": None,
        "secondary": None, "primary": None, "service": 10,
        "path": 12, "track": 10,
        "pedestrian": 12, "footway": 12, "sidewalk": 15, "crossing": 8,
        "steps": None, "link": 5,
    },
    # Прогулочный: подальше от больших дорог, лестницы разрешены.
    "Спокойный": {
        "cycleway": 18, "tertiary": 12, "residential": 15, "secondary": 8,
        "primary": 5, "service": 13, "path": 14, "track": 12,
        "pedestrian": 12, "footway": 10, "sidewalk": 12, "crossing": 8,
        "steps": 2, "link": 4,
    },
}

DEFAULT_PROFILE = "Самокат"


def speeds_ms(profile_name: str) -> list[float | None]:
    """Таблица «индекс класса -> м/с» (None = класс запрещён)."""
    table = PROFILES[profile_name]
    return [None if table.get(name) is None else table[name] / 3.6
            for name in WAY_CLASSES]


def max_speed_ms(profile_name: str) -> float:
    """Максимальная скорость профиля — для допустимой эвристики A*."""
    return max(s for s in speeds_ms(profile_name) if s is not None)
