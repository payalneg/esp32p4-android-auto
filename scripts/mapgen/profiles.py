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

# Группы линий. Проезжая часть, велодорожка и тротуар вдоль неё — это
# параллельные линии OSM, связанные друг с другом каждые 30-50 метров въездом
# во двор или переходом. По одной только длине прыжок между ними почти
# бесплатен — отсюда маршруты, которые пилят между дорогой и тротуаром ради
# десятка метров. За выход из группы A* берёт GROUP_SWITCH_PENALTY_S, так что
# «туда и обратно» стоит 20 секунд и окупается только реальным выигрышем.
#
# "connector" — не группа: переход (и искусственная связка link) прозрачен и
# переносит предыдущую группу дальше. Поэтому дорога -> переход -> тротуар
# платит смену один раз, а тротуар -> переход -> тротуар не платит вовсе.
LINE_GROUPS = ("road", "cycle", "foot", "other", "connector")
GROUP_INDEX = {name: i for i, name in enumerate(LINE_GROUPS)}
GROUP_CONNECTOR = GROUP_INDEX["connector"]

# Группа каждого класса — порядок как в WAY_CLASSES.
CLASS_GROUP = (
    "cycle",      # 0  cycleway
    "road",       # 1  tertiary
    "road",       # 2  residential
    "road",       # 3  secondary
    "road",       # 4  primary
    "road",       # 5  service
    "other",      # 6  path
    "other",      # 7  track
    "foot",       # 8  pedestrian
    "foot",       # 9  footway
    "foot",       # 10 sidewalk
    "connector",  # 11 crossing
    "foot",       # 12 steps
    "connector",  # 13 link
)
CLASS_GROUP_INDEX = tuple(GROUP_INDEX[g] for g in CLASS_GROUP)

# Секунды за выход из одной группы линий в другую. Замерено на 200 маршрутах
# по Кракову: при 0 с маршрут меняет линию 2.4 раза на километр, из них 0.7 —
# вылазки короче 80 метров; при 10 с — 1.0 и 0.14, а время растёт на 2.5 %.
GROUP_SWITCH_PENALTY_S = 10.0

# Секунды за выход на переход — бордюр и взгляд по сторонам, которые шесть
# метров геометрии (2.7 секунды езды) сами по себе не оплачивают.
CROSSING_PENALTY_S = 10.0

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
