"""Поиск по объектам карты: адреса домов и именованные POI.

Индекс — плоский TSV, отсортированный по нормализованному ключу:
    norm \t display \t kind \t lat \t lon
Формат сознательно тривиальный: на ESP32-P4 тот же файл лежит на SD-карте
и ищется бинарным поиском по префиксу / линейным сканом.
"""
import unicodedata

import config

MAX_RESULTS = 20


# Буквы, которые NFKD не раскладывает (не диакритика, а отдельные символы)
_SPECIAL = str.maketrans({"ł": "l", "ø": "o", "đ": "d", "ß": "ss", "æ": "ae"})


def normalize(s: str) -> str:
    """Нижний регистр + удаление диакритики: 'Floriańska' -> 'florianska'."""
    s = unicodedata.normalize("NFKD", s.lower()).translate(_SPECIAL)
    return "".join(c for c in s if not unicodedata.combining(c))


class SearchIndex:
    def __init__(self, path=None):
        self.path = path or config.SEARCH_INDEX
        self._entries: list[tuple[str, str, str, float, float]] | None = None

    def _load(self):
        if self._entries is None:
            self._entries = []
            with open(self.path, encoding="utf-8") as f:
                for line in f:
                    norm, display, kind, lat, lon = line.rstrip("\n").split("\t")
                    self._entries.append((norm, display, kind, float(lat), float(lon)))

    def search(self, query: str, limit: int = MAX_RESULTS):
        """Возвращает [(display, kind, lat, lon)]: сперва совпадения по префиксу."""
        q = normalize(query.strip())
        if not q:
            return []
        self._load()
        prefix, contains = [], []
        for norm, display, kind, lat, lon in self._entries:
            if norm.startswith(q):
                prefix.append((display, kind, lat, lon))
            elif q in norm:
                contains.append((display, kind, lat, lon))
            if len(prefix) >= limit:
                break
        return (prefix + contains)[:limit]
