"""Диагностика графа маршрутизации: классы, связность, профили.

    python graph_stats.py

Отвечает на два вопроса: сколько в графе тротуаров и можно ли на них
съехать. Второй важнее — тротуары в OSM это отдельные линии вдоль дороги,
и если переходы не размечены, они образуют свой остров, куда роутер
физически не попадёт, сколько ни меняй скорости.
"""
from collections import Counter, defaultdict, deque

import config
import graph_format as gf
import profiles as prof


def load():
    return gf.parse_graph(config.ROUTE_GRAPH.read_bytes())


def components(nodes, edges, allowed: set[int] | None = None):
    """Компоненты связности по рёбрам разрешённых классов."""
    adj = defaultdict(list)
    for e in edges:
        if allowed is not None and e["way_class"] not in allowed:
            continue
        adj[e["from"]].append(e["to"])
        adj[e["to"]].append(e["from"])
    seen, sizes = set(), []
    for start in adj:
        if start in seen:
            continue
        size, q = 0, deque([start])
        seen.add(start)
        while q:
            u = q.popleft()
            size += 1
            for v in adj[u]:
                if v not in seen:
                    seen.add(v)
                    q.append(v)
        sizes.append(size)
    sizes.sort(reverse=True)
    return sizes


def main():
    nodes, edges = load()
    print(f"вершин {len(nodes)}, рёбер {len(edges)}\n")

    by_class = Counter(prof.WAY_CLASSES[e["way_class"]] for e in edges)
    km = defaultdict(float)
    for e in edges:
        km[prof.WAY_CLASSES[e["way_class"]]] += e["len_dm"] / 10000.0
    print("рёбра по классам:")
    for name in prof.WAY_CLASSES:
        if by_class[name]:
            print(f"  {name:<12} {by_class[name]:7}  {km[name]:9.1f} км")

    # где дорога и тротуар делят вершину — это и есть точки съезда
    foot = {prof.CLASS_INDEX[c] for c in prof.FOOT_CLASSES}
    road = set(range(len(prof.WAY_CLASSES))) - foot - {prof.CLASS_INDEX["link"]}
    foot_nodes, road_nodes = set(), set()
    for e in edges:
        target = foot_nodes if e["way_class"] in foot else \
            (road_nodes if e["way_class"] in road else None)
        if target is not None:
            target.update((e["from"], e["to"]))
    shared = foot_nodes & road_nodes
    print(f"\nвершин с пешеходными рёбрами: {len(foot_nodes)}")
    print(f"вершин с дорожными рёбрами:   {len(road_nodes)}")
    print(f"общих (точки съезда):         {len(shared)}"
          f"  = {100 * len(shared) / max(len(foot_nodes), 1):.1f}% пешеходных")

    print("\nсвязность по профилям (размеры компонент, топ-5):")
    for name in prof.PROFILES:
        allowed = {i for i, c in enumerate(prof.WAY_CLASSES)
                   if prof.PROFILES[name].get(c) is not None}
        sizes = components(nodes, edges, allowed)
        total = sum(sizes)
        head = ", ".join(str(x) for x in sizes[:5])
        share = 100 * sizes[0] / total if total else 0
        print(f"  {name:<18} компонент {len(sizes):5}, "
              f"крупнейшая {share:5.1f}% ({head})")


if __name__ == "__main__":
    main()
