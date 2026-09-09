"""Бинарный формат графа маршрутизации (.bin, little-endian, C-friendly).

    u32  magic = 0x32464752 ("RGF2")
    u32  node_count
    u32  edge_count
    node_count раз:  i32 lat*1e7, i32 lon*1e7
    edge_count раз:
        u32 from, u32 to          (индексы вершин)
        u32 len_dm                (длина, дециметры)
        u8  way_class             (индекс в profiles.WAY_CLASSES)
        u8  flags                 (bit0 = двунаправленное ребро)
        u16 point_count
        point_count раз: i32 lat*1e7, i32 lon*1e7   (геометрия, включая концы)

Время проезда в графе НЕ хранится: стоимость ребра считает роутер из класса
дороги и выбранного профиля (profiles.py), поэтому профиль переключается без
пересборки графа.

Вершины — перекрёстки и концы дорог; геометрия ребра хранит все
промежуточные точки, чтобы маршрут рисовался по изгибам улицы.
"""
import struct

MAGIC = 0x32464752
SCALE = 1e7

FLAG_BIDIR = 1


def pack_graph(nodes: list[tuple[float, float]], edges: list[dict]) -> bytes:
    out = [struct.pack("<III", MAGIC, len(nodes), len(edges))]
    for lat, lon in nodes:
        out.append(struct.pack("<ii", round(lat * SCALE), round(lon * SCALE)))
    for e in edges:
        pts = e["points"]
        out.append(struct.pack("<IIIBBH", e["from"], e["to"], e["len_dm"],
                               e["way_class"], e["flags"], len(pts)))
        out.append(struct.pack(f"<{2 * len(pts)}i",
                               *[round(c * SCALE) for p in pts for c in p]))
    return b"".join(out)


def largest_component(nodes: list, edges: list) -> tuple[list, list]:
    """Оставляет только крупнейшую компоненту связности (union-find).

    Изолированные обрывки (дворовые дорожки, частные территории) иначе
    ломают маршрутизацию: точка привязывается к островку без выхода.
    """
    parent = list(range(len(nodes)))

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    for e in edges:
        a, b = find(e["from"]), find(e["to"])
        if a != b:
            parent[a] = b

    from collections import Counter
    roots = Counter(find(i) for i in range(len(nodes)))
    main = roots.most_common(1)[0][0]

    remap: dict[int, int] = {}
    new_nodes = []
    for i, node in enumerate(nodes):
        if find(i) == main:
            remap[i] = len(new_nodes)
            new_nodes.append(node)
    new_edges = [{**e, "from": remap[e["from"]], "to": remap[e["to"]]}
                 for e in edges if e["from"] in remap]
    return new_nodes, new_edges


def parse_graph(data: bytes):
    magic, node_count, edge_count = struct.unpack_from("<III", data, 0)
    if magic != MAGIC:
        raise ValueError("не граф маршрутизации: неверная сигнатура")
    pos = 12
    flat = struct.unpack_from(f"<{2 * node_count}i", data, pos)
    pos += 8 * node_count
    nodes = [(flat[i] / SCALE, flat[i + 1] / SCALE)
             for i in range(0, 2 * node_count, 2)]
    edges = []
    for _ in range(edge_count):
        f, t, len_dm, way_class, flags, npts = struct.unpack_from("<IIIBBH", data, pos)
        pos += 16
        pflat = struct.unpack_from(f"<{2 * npts}i", data, pos)
        pos += 8 * npts
        edges.append({
            "from": f, "to": t, "len_dm": len_dm,
            "way_class": way_class, "flags": flags,
            "points": [(pflat[i] / SCALE, pflat[i + 1] / SCALE)
                       for i in range(0, 2 * npts, 2)],
        })
    return nodes, edges
