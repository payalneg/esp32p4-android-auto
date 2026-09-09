"""Собственный бинарный формат векторного тайла (.vtile).

Формат намеренно примитивный и C-friendly — парсер для ESP32-P4 пишется
за вечер: никаких varint, protobuf и сжатия, только little-endian структуры.

Схема тайла (LE):
    u32  magic = 0x324C5456 ("VTL2")
    u8   layer_count
    повторить layer_count раз:
        u8   layer_id            (LAYER_*)
        u16  feature_count
        повторить feature_count раз:
            u8   geom_type       (GEOM_LINE | GEOM_POLYGON | GEOM_POINT)
            u8   cls             (класс внутри слоя, см. builder)
            u16  name_len        (байт UTF-8; 0 если имени нет)
            ...  name
            u16  num_points
            повторить num_points раз: i32 x, i32 y
            только для GEOM_POLYGON:
                u16  tri_count   (число треугольников заливки)
                повторить tri_count раз: u16 i0, u16 i1, u16 i2
                                 (индексы вершин в списке точек)

Треугольники считаются генератором (ear clipping) — устройство заливает
полигон готовыми треугольниками без геометрии на борту (как в mwm
Organic Maps). Координаты — в локальной сетке тайла: (0,0) — левый
верхний угол, EXTENT — правый нижний. Значения могут выходить за
[0, EXTENT] — фичи не обрезаются геометрически, только по bbox.
"""
import struct

MAGIC = 0x324C5456
EXTENT = 4096

GEOM_LINE = 1
GEOM_POLYGON = 2
GEOM_POINT = 3

LAYER_WATER = 1      # полигоны воды и линии рек; cls: 0=полигон/река, 1=ручей
LAYER_LANDUSE = 2    # зелень (парк/лес/трава); cls: 0
LAYER_ROADS = 3      # cls: 0=motorway/trunk, 1=primary, 2=secondary/tertiary,
                     #      3=residential/unclassified, 4=service/track, 5=path/foot/cycle
LAYER_RAIL = 4       # cls: 0=магистраль, 1=трамвай/прочее
LAYER_BUILDINGS = 5  # cls: 0
LAYER_LABELS = 6     # точки; cls: 0=city, 1=town, 2=suburb, 3=village/neighbourhood

# Порядок отрисовки снизу вверх
DRAW_ORDER = (LAYER_LANDUSE, LAYER_WATER, LAYER_BUILDINGS,
              LAYER_ROADS, LAYER_RAIL, LAYER_LABELS)


def triangulate(pts: list[tuple[int, int]]) -> list[tuple[int, int, int]]:
    """Ear clipping -> список индексных троек. Вызывается в генераторе."""
    n = len(pts)
    if n < 3:
        return []
    if n == 3:
        return [(0, 1, 2)]
    area2 = 0
    for i in range(n):
        x1, y1 = pts[i]
        x2, y2 = pts[(i + 1) % n]
        area2 += x1 * y2 - x2 * y1
    ccw = area2 > 0
    idx = list(range(n))
    tris = []
    guard = 0
    while len(idx) > 3 and guard < 3 * n:
        guard += 1
        m = len(idx)
        for k in range(m):
            i0, i1, i2 = idx[k - 1], idx[k], idx[(k + 1) % m]
            ax, ay = pts[i0]
            bx, by = pts[i1]
            cx, cy = pts[i2]
            cross = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax)
            if (cross > 0) != ccw or cross == 0:
                continue
            ok = True
            for j in idx:
                if j in (i0, i1, i2):
                    continue
                px, py = pts[j]
                d1 = (bx - ax) * (py - ay) - (by - ay) * (px - ax)
                d2 = (cx - bx) * (py - by) - (cy - by) * (px - bx)
                d3 = (ax - cx) * (py - cy) - (ay - cy) * (px - cx)
                if (d1 >= 0 and d2 >= 0 and d3 >= 0) or \
                   (d1 <= 0 and d2 <= 0 and d3 <= 0):
                    ok = False
                    break
            if ok:
                tris.append((i0, i1, i2))
                idx.pop(k)
                break
        else:
            break  # самопересечение — добиваем веером
    if len(idx) == 3:
        tris.append((idx[0], idx[1], idx[2]))
    elif len(idx) > 3:
        for k in range(1, len(idx) - 1):
            tris.append((idx[0], idx[k], idx[k + 1]))
    return tris


def pack_tile(layers: dict[int, list[dict]]) -> bytes:
    """layers: {layer_id: [{"geom", "cls", "name", "points", "tris"}]}

    "tris" — список индексов (i0, i1, i2) для полигонов; может отсутствовать.
    """
    out = [struct.pack("<IB", MAGIC, len(layers))]
    for layer_id, features in sorted(layers.items()):
        out.append(struct.pack("<BH", layer_id, len(features)))
        for f in features:
            name = f.get("name", "").encode("utf-8")[:65535]
            pts = f["points"]
            out.append(struct.pack("<BBH", f["geom"], f["cls"], len(name)))
            out.append(name)
            out.append(struct.pack("<H", len(pts)))
            out.append(struct.pack(f"<{2 * len(pts)}i",
                                   *[c for p in pts for c in p]))
            if f["geom"] == GEOM_POLYGON:
                tris = f.get("tris") or ()
                out.append(struct.pack("<H", len(tris)))
                out.append(struct.pack(f"<{3 * len(tris)}H",
                                       *[i for t in tris for i in t]))
    return b"".join(out)


def parse_tile(data: bytes) -> dict[int, list[dict]]:
    magic, layer_count = struct.unpack_from("<IB", data, 0)
    if magic != MAGIC:
        raise ValueError("не vtile: неверная сигнатура")
    pos = 5
    layers: dict[int, list[dict]] = {}
    for _ in range(layer_count):
        layer_id, feature_count = struct.unpack_from("<BH", data, pos)
        pos += 3
        features = []
        for _ in range(feature_count):
            geom, cls, name_len = struct.unpack_from("<BBH", data, pos)
            pos += 4
            name = data[pos:pos + name_len].decode("utf-8")
            pos += name_len
            (num_points,) = struct.unpack_from("<H", data, pos)
            pos += 2
            flat = struct.unpack_from(f"<{2 * num_points}i", data, pos)
            pos += 8 * num_points
            tris = []
            if geom == GEOM_POLYGON:
                (tri_count,) = struct.unpack_from("<H", data, pos)
                pos += 2
                tflat = struct.unpack_from(f"<{3 * tri_count}H", data, pos)
                pos += 6 * tri_count
                tris = list(zip(tflat[0::3], tflat[1::3], tflat[2::3]))
            features.append({
                "geom": geom, "cls": cls, "name": name,
                "points": list(zip(flat[0::2], flat[1::2])),
                "tris": tris,
            })
        layers[layer_id] = features
    return layers
