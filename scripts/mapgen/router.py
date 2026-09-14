"""Маршрутизация: A* по бинарному графу route_graph.bin (формат RGF2).

Алгоритм и структуры сознательно простые — на ESP32-P4 переносится как есть:
граф читается с SD-карты, A* с бинарной кучей, эвристика — гаверсин,
делённый на максимальную скорость профиля, т.е. допустимая.

Стоимость ребра берётся из профиля (profiles.py) в момент запроса, а не из
графа: смена профиля не требует пересборки, а запрещённые классы просто не
раскрываются в поиске.
"""
import heapq
import math

import config
import graph_format as gf
import profiles as prof

SNAP_MAX_M = 500             # дальше этого точку к графу не привязываем
_GRID = 0.002                # ~220 м по широте


def _haversine(a: tuple[float, float], b: tuple[float, float]) -> float:
    r = 6371000.0
    p1, p2 = math.radians(a[0]), math.radians(b[0])
    dp, dl = p2 - p1, math.radians(b[1] - a[1])
    h = math.sin(dp / 2) ** 2 + math.cos(p1) * math.cos(p2) * math.sin(dl / 2) ** 2
    return 2 * r * math.asin(math.sqrt(h))


class Router:
    def __init__(self, path=None, profile: str = prof.DEFAULT_PROFILE):
        self.path = path or config.ROUTE_GRAPH
        self.profile = profile
        self._loaded = False
        self._comp_cache: dict[str, tuple[list[int], int]] = {}

    def set_profile(self, name: str) -> None:
        """Смена профиля. Граф перезагружать не нужно — цена считается позже."""
        if name not in prof.PROFILES:
            raise KeyError(f"нет профиля {name!r}")
        self.profile = name

    def _components(self) -> tuple[list[int], int]:
        """(корень компоненты для каждой вершины, корень крупнейшей).

        Считается по рёбрам, разрешённым текущим профилем, и кэшируется.
        Нужно для привязки: точка, попавшая на островок, отрезанный
        запретами профиля (классика — двор с выходом только по лестнице),
        иначе даёт «маршрут не найден» независимо от того, куда ехать.
        """
        cached = self._comp_cache.get(self.profile)
        if cached is not None:
            return cached
        speeds = prof.speeds_ms(self.profile)
        parent = list(range(len(self.nodes)))

        def find(x: int) -> int:
            while parent[x] != x:
                parent[x] = parent[parent[x]]     # сжатие путей
                x = parent[x]
            return x

        for e in self.edges:
            if speeds[e["way_class"]] is None:
                continue
            a, b = find(e["from"]), find(e["to"])
            if a != b:
                parent[a] = b
        roots = [find(i) for i in range(len(self.nodes))]
        sizes: dict[int, int] = {}
        for e in self.edges:                       # изолированные вершины не в счёт
            if speeds[e["way_class"]] is not None:
                sizes[roots[e["from"]]] = sizes.get(roots[e["from"]], 0) + 1
        biggest = max(sizes, key=sizes.get) if sizes else -1
        self._comp_cache[self.profile] = (roots, biggest)
        return roots, biggest

    def _load(self):
        if self._loaded:
            return
        self.nodes, self.edges = gf.parse_graph(self.path.read_bytes())
        # список смежности: node -> [(сосед, номер ребра, вперёд?)].
        # Времени здесь нет: оно зависит от профиля и считается в route().
        self.adj: list[list[tuple[int, int, bool]]] = [[] for _ in self.nodes]
        for i, e in enumerate(self.edges):
            self.adj[e["from"]].append((e["to"], i, True))
            if e["flags"] & gf.FLAG_BIDIR:
                self.adj[e["to"]].append((e["from"], i, False))
        # сеточный индекс вершин для привязки кликов
        self.grid: dict[tuple[int, int], list[int]] = {}
        for i, (lat, lon) in enumerate(self.nodes):
            self.grid.setdefault((int(lat / _GRID), int(lon / (_GRID * 1.6))),
                                 []).append(i)
        self._loaded = True

    def nearest_node(self, lat: float, lon: float,
                     in_profile: bool = True) -> int | None:
        """Ближайшая вершина графа. При in_profile — только та, из которой
        профиль вообще может уехать (крупнейшая разрешённая компонента)."""
        self._load()
        roots, biggest = self._components() if in_profile else (None, -1)
        cell = (int(lat / _GRID), int(lon / (_GRID * 1.6)))
        best, best_d = None, SNAP_MAX_M
        for ring in range(3):
            for dx in range(-ring, ring + 1):
                for dy in range(-ring, ring + 1):
                    if max(abs(dx), abs(dy)) != ring:
                        continue
                    for i in self.grid.get((cell[0] + dx, cell[1] + dy), ()):
                        if roots is not None and roots[i] != biggest:
                            continue
                        d = _haversine((lat, lon), self.nodes[i])
                        if d < best_d:
                            best, best_d = i, d
            if best is not None:
                return best
        return best

    def route(self, start: tuple[float, float], goal: tuple[float, float]):
        """A*. Возвращает {'points', 'length_m', 'time_s'} или None."""
        self._load()
        s = self.nearest_node(*start)
        g = self.nearest_node(*goal)
        if s is None or g is None or s == g:
            return None
        goal_pos = self.nodes[g]
        speeds = prof.speeds_ms(self.profile)
        vmax = prof.max_speed_ms(self.profile)
        dist = {s: 0.0}
        came: dict[int, tuple[int, int, bool]] = {}  # node -> (prev, edge, fwd)
        pq = [(_haversine(self.nodes[s], goal_pos) / vmax, s)]
        visited = set()
        while pq:
            _, u = heapq.heappop(pq)
            if u == g:
                break
            if u in visited:
                continue
            visited.add(u)
            for v, ei, fwd in self.adj[u]:
                speed = speeds[self.edges[ei]["way_class"]]
                if speed is None:          # класс запрещён профилем
                    continue
                nd = dist[u] + self.edges[ei]["len_dm"] / 10.0 / speed
                if nd < dist.get(v, math.inf):
                    dist[v] = nd
                    came[v] = (u, ei, fwd)
                    h = _haversine(self.nodes[v], goal_pos) / vmax
                    heapq.heappush(pq, (nd + h, v))
        if g not in came:
            return None
        # восстановление пути с геометрией рёбер
        points: list[tuple[float, float]] = []
        length_dm = 0
        legs: list[tuple[int, int]] = []   # (класс дороги, длина в дм)
        node = g
        while node != s:
            prev, ei, fwd = came[node]
            edge = self.edges[ei]
            pts = edge["points"]
            length_dm += edge["len_dm"]
            legs.append((edge["way_class"], edge["len_dm"]))
            seg = pts if fwd else list(reversed(pts))
            # последняя точка seg == первая точка накопленного пути
            points = seg[:-1] + points if points else seg
            node = prev
        legs.reverse()
        return {"points": points, "length_m": length_dm / 10.0,
                "time_s": dist[g], "legs": legs, "profile": self.profile}
