"""Навигация по построенному маршруту: манёвры и ведение по линии.

Роутер отдаёт только полилинию. Навигатор превращает её в список манёвров
(«через 20 м направо»), а затем ведёт по ней позицию: привязывает точку к
маршруту, считает остаток пути и времени и ловит сход.

Имён улиц в графе нет (`builder._collect_bike_way` их не сохраняет), поэтому
манёвр выводится только из геометрии: в каждой вершине берётся азимут входа и
выхода по окну ±TURN_WINDOW_M. Окно короткое намеренно — плавный поворот
улицы размазывает отклонение по десяткам метров и порога не достигает, а
настоящий перекрёсток укладывает его в эти же 15 м целиком.
"""
import math

TURN_WINDOW_M = 15.0    # плечо для замера азимута до и после вершины
MIN_TURN_DEG = 30.0     # меньше — считаем, что едем прямо
MERGE_M = 25.0          # манёвры ближе этого склеиваются в один
BIKE_SPEED_MS = 18 / 3.6

# порог (град) -> тип манёвра; знак: + вправо, - влево
_CLASSES = ((160, "uturn"), (120, "sharp"), (65, "turn"), (MIN_TURN_DEG, "slight"))


def _hav(a, b) -> float:
    r = 6371000.0
    p1, p2 = math.radians(a[0]), math.radians(b[0])
    dp, dl = p2 - p1, math.radians(b[1] - a[1])
    h = math.sin(dp / 2) ** 2 + math.cos(p1) * math.cos(p2) * math.sin(dl / 2) ** 2
    return 2 * r * math.asin(math.sqrt(h))


def _bearing(a, b) -> float:
    p1, p2 = math.radians(a[0]), math.radians(b[0])
    dl = math.radians(b[1] - a[1])
    y = math.sin(dl) * math.cos(p2)
    x = math.cos(p1) * math.sin(p2) - math.sin(p1) * math.cos(p2) * math.cos(dl)
    return math.degrees(math.atan2(y, x)) % 360


def _delta(bearing_in: float, bearing_out: float) -> float:
    """Знаковый доворот: >0 вправо, <0 влево, диапазон (-180, 180]."""
    return (bearing_out - bearing_in + 180) % 360 - 180


def cumulative(points) -> list[float]:
    """Накопленная длина маршрута в метрах для каждой вершины."""
    out = [0.0]
    for a, b in zip(points, points[1:]):
        out.append(out[-1] + _hav(a, b))
    return out


def _at_distance(points, cum, target: float):
    """Точка на маршруте на расстоянии target от начала (с интерполяцией)."""
    if target <= 0:
        return points[0]
    if target >= cum[-1]:
        return points[-1]
    lo, hi = 0, len(cum) - 1
    while hi - lo > 1:                      # cum отсортирован — двоичный поиск
        mid = (lo + hi) // 2
        if cum[mid] <= target:
            lo = mid
        else:
            hi = mid
    seg = cum[hi] - cum[lo]
    t = 0.0 if seg <= 0 else (target - cum[lo]) / seg
    (la1, lo1), (la2, lo2) = points[lo], points[hi]
    return (la1 + (la2 - la1) * t, lo1 + (lo2 - lo1) * t)


def _classify(delta: float) -> str:
    side = "right" if delta > 0 else "left"
    for threshold, name in _CLASSES:
        if abs(delta) >= threshold:
            return "uturn" if name == "uturn" else f"{name}_{side}"
    return "straight"


def maneuvers(points) -> list[dict]:
    """Список манёвров маршрута: [{dist_m, type, angle, point}, ...].

    dist_m — расстояние от начала маршрута до точки манёвра.
    Последним всегда идёт "arrive".
    """
    if len(points) < 3:
        return [{"dist_m": 0.0, "type": "arrive", "angle": 0.0,
                 "point": points[-1] if points else None}]
    cum = cumulative(points)
    total = cum[-1]

    # доворот в каждой вершине, замеренный по окну ±TURN_WINDOW_M
    candidates = []
    for i in range(1, len(points) - 1):
        if cum[i] < TURN_WINDOW_M or total - cum[i] < TURN_WINDOW_M:
            continue
        back = _at_distance(points, cum, cum[i] - TURN_WINDOW_M)
        ahead = _at_distance(points, cum, cum[i] + TURN_WINDOW_M)
        d = _delta(_bearing(back, points[i]), _bearing(points[i], ahead))
        if abs(d) >= MIN_TURN_DEG:
            candidates.append((i, d))

    # склейка: подряд идущие вершины одного поворота — это один манёвр,
    # представителем берём вершину с максимальным доворотом
    out: list[dict] = []
    group: list[tuple[int, float]] = []

    def flush():
        if not group:
            return
        i, d = max(group, key=lambda p: abs(p[1]))
        out.append({"dist_m": cum[i], "type": _classify(d),
                    "angle": d, "point": points[i]})

    for item in candidates:
        if group and cum[item[0]] - cum[group[-1][0]] > MERGE_M:
            flush()
            group = []
        group.append(item)
    flush()

    out.append({"dist_m": total, "type": "arrive", "angle": 0.0,
                "point": points[-1]})
    return out


class Navigator:
    """Ведение по маршруту: привязка позиции, остаток, следующий манёвр.

    Привязка ищется в окне вокруг прошлой позиции, а не по всему маршруту:
    иначе на петле или возврате точка перепрыгивает на другой участок.
    """

    OFF_ROUTE_M = 30.0      # дальше этого от линии — считаем сходом
    BACK_ON_M = 15.0        # ближе этого — вернулись (гистерезис)
    OFF_ROUTE_HITS = 3      # столько замеров подряд, чтобы не дёргаться
    SEARCH_AHEAD_M = 300.0  # окно поиска привязки вперёд от прошлой точки
    SEARCH_BACK_M = 60.0    # и назад

    def __init__(self, route: dict):
        self.points = route["points"]
        self.cum = cumulative(self.points)
        self.maneuvers = maneuvers(self.points)
        self.total_m = self.cum[-1]
        self.along_m = 0.0
        self.offset_m = 0.0
        self.off_route = False
        self._off_hits = 0

    # --- привязка к линии ---

    @staticmethod
    def _project(p, a, b):
        """Проекция p на отрезок ab в локальных метрах: (доля t, отступ м)."""
        k = math.cos(math.radians(p[0]))          # сжатие долготы по широте
        ax, ay = a[1] * k, a[0]
        bx, by = b[1] * k, b[0]
        px, py = p[1] * k, p[0]
        dx, dy = bx - ax, by - ay
        seg2 = dx * dx + dy * dy
        t = 0.0 if seg2 == 0 else max(0.0, min(1.0,
                                               ((px - ax) * dx + (py - ay) * dy) / seg2))
        cx, cy = ax + dx * t, ay + dy * t
        deg = math.hypot(px - cx, py - cy)
        return t, deg * 111320.0

    def _match(self, pos):
        lo_m = self.along_m - self.SEARCH_BACK_M
        hi_m = self.along_m + self.SEARCH_AHEAD_M
        best = None
        for i in range(len(self.points) - 1):
            if self.cum[i + 1] < lo_m or self.cum[i] > hi_m:
                continue
            t, off = self._project(pos, self.points[i], self.points[i + 1])
            if best is None or off < best[1]:
                best = (self.cum[i] + t * (self.cum[i + 1] - self.cum[i]), off)
        # окно пустое или в нём ничего близкого — ищем по всему маршруту.
        # Нужно на телепортах: переподключение фикса, старт из середины,
        # возврат на маршрут в стороне от прошлой привязки.
        if best is None or best[1] > self.OFF_ROUTE_M:
            for i in range(len(self.points) - 1):
                t, off = self._project(pos, self.points[i], self.points[i + 1])
                if best is None or off < best[1]:
                    best = (self.cum[i] + t * (self.cum[i + 1] - self.cum[i]), off)
        return best

    # --- обновление ---

    def update(self, lat: float, lon: float) -> dict:
        self.along_m, self.offset_m = self._match((lat, lon))

        if self.offset_m > self.OFF_ROUTE_M:
            self._off_hits += 1
            if self._off_hits >= self.OFF_ROUTE_HITS:
                self.off_route = True
        elif self.offset_m < self.BACK_ON_M:
            self._off_hits = 0
            self.off_route = False

        nxt = next((m for m in self.maneuvers if m["dist_m"] >= self.along_m - 1),
                   self.maneuvers[-1])
        remaining = max(0.0, self.total_m - self.along_m)
        return {
            "remaining_m": remaining,
            "remaining_s": remaining / BIKE_SPEED_MS,
            "offset_m": self.offset_m,
            "off_route": self.off_route,
            "next": {"type": nxt["type"],
                     "dist_m": max(0.0, nxt["dist_m"] - self.along_m),
                     "point": nxt["point"]},
            "arrived": remaining < 15.0,
        }

    def position_at(self, along_m: float):
        """Точка на маршруте — для симуляции движения без GPS."""
        return _at_distance(self.points, self.cum, along_m)
