"""NVT1 — the binary vector tile the head unit itself draws.

The C side is `main/nav_vector.c`; this module is its mirror, used by the
packer and by the tests. Keep the two in lockstep. The format is documented
at the top of `main/nav_vector.h`; the short version:

    header, string table, layers in draw order, features with delta-varint
    rings in a 4096-unit tile.

It is deliberately not Mapbox Vector Tile: no protobuf, no zigzag packed
commands, no per-feature properties. The firmware has a single small
rasterizer, and a tile has to be readable by a 20 KB C file with no
allocator.
"""
from __future__ import annotations

import struct
from dataclasses import dataclass, field

MAGIC = b"NVT1"
VERSION = 1
EXTENT = 4096

GEOM_LINE = 0
GEOM_POLYGON = 1
GEOM_POINT = 2

FLAG_BRIDGE = 1 << 0
FLAG_TUNNEL = 1 << 1

LAYER_WATER = 1
LAYER_GREEN = 2
LAYER_BUILDING = 3
LAYER_ROAD = 4
LAYER_RAIL = 5
LAYER_LABEL = 6
LAYER_WATERWAY = 7

DRAW_ORDER = (
    LAYER_GREEN,
    LAYER_WATER,
    LAYER_WATERWAY,
    LAYER_BUILDING,
    LAYER_ROAD,
    LAYER_RAIL,
    LAYER_LABEL,
)

LABEL_PLACE = 0
LABEL_WATER = 1
LABEL_PARK = 2
LABEL_STREET = 3
LABEL_LANDMARK = 4

ROAD_MOTORWAY = 0
ROAD_PRIMARY = 1
ROAD_SECONDARY = 2
ROAD_RESIDENTIAL = 3
ROAD_SERVICE = 4
ROAD_PATH = 5
ROAD_CYCLE = 6


@dataclass
class Feature:
    """One feature, in tile coordinates, ready to be packed."""
    geom: int
    cls: int
    rings: list[list[tuple[int, int]]]
    name: str = ""
    rank: int = 0
    flags: int = 0


def zigzag(value: int) -> int:
    return (value << 1) ^ (value >> 31) if value >= 0 else ((-value) << 1) - 1


def unzigzag(value: int) -> int:
    return (value >> 1) ^ -(value & 1)


def _varint(value: int) -> bytes:
    out = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        if value:
            out.append(byte | 0x80)
        else:
            out.append(byte)
            return bytes(out)


def pack_tile(layers: dict[int, list[Feature]], z: int = 0) -> bytes:
    """Serialize one tile. Layers not listed in DRAW_ORDER are refused."""
    # String table: names in first-use order, at most 255 entries. A road
    # without a name costs nothing; a label without one is pointless.
    strings: list[str] = []
    ids: dict[str, int] = {}
    for layer_id in DRAW_ORDER:
        for f in layers.get(layer_id, ()):
            if f.name and f.name not in ids:
                if len(strings) >= 255:
                    raise ValueError("too many strings in one tile")
                ids[f.name] = len(strings)
                strings.append(f.name)

    used = [lid for lid in DRAW_ORDER if layers.get(lid)]
    out = bytearray()
    out += struct.pack(
        "<4sBBHBBHI",
        MAGIC,
        VERSION,
        (1 if LAYER_LABEL in used else 0),
        EXTENT,
        z & 0xFF,
        len(used),
        len(strings),
        0,
    )
    for s in strings:
        raw = s.encode("utf-8")
        if len(raw) > 255:
            raw = raw[:255]
            # Do not leave a dangling UTF-8 lead byte.
            while raw and (raw[-1] & 0xC0) == 0x80:
                raw = raw[:-1]
            if raw and raw[-1] >= 0xC0:
                raw = raw[:-1]
        out.append(len(raw))
        out += raw

    for layer_id in used:
        features = layers[layer_id]
        out += struct.pack("<BH", layer_id, len(features))
        for f in features:
            name_id = ids.get(f.name, 0xFF) if f.name else 0xFF
            rings = f.rings or [[]]
            if len(rings) > 4:
                raise ValueError(f"feature has {len(rings)} rings, max 4")
            out += struct.pack(
                "<BBBBBB", f.geom, f.cls, f.flags, name_id, f.rank, len(rings)
            )
            for ring in rings:
                out += struct.pack("<H", len(ring))
                px = py = 0
                for x, y in ring:
                    out += _varint(zigzag(x - px))
                    out += _varint(zigzag(y - py))
                    px, py = x, y
    return bytes(out)


def _read_varint(data: bytes, pos: int) -> tuple[int, int]:
    value = 0
    shift = 0
    while True:
        if pos >= len(data):
            raise ValueError("truncated varint")
        b = data[pos]
        pos += 1
        value |= (b & 0x7F) << shift
        if not (b & 0x80):
            return value, pos
        shift += 7
        if shift > 28:
            raise ValueError("varint too long")


def parse_tile(data: bytes) -> tuple[int, dict[int, list[Feature]]]:
    """Inverse of [pack_tile], for tests and the preview renderer."""
    if len(data) < 16:
        raise ValueError("tile shorter than its header")
    magic, version, flags, extent, z, layer_count, string_count, _ = \
        struct.unpack_from("<4sBBHBBHI", data, 0)
    if magic != MAGIC:
        raise ValueError("not an NVT1 tile")
    if version != VERSION:
        raise ValueError(f"unsupported NVT1 version {version}")
    if extent != EXTENT:
        raise ValueError(f"extent {extent}, expected {EXTENT}")
    pos = 16
    strings: list[str] = []
    for _ in range(string_count):
        n = data[pos]
        pos += 1
        strings.append(data[pos:pos + n].decode("utf-8", "replace"))
        pos += n

    layers: dict[int, list[Feature]] = {}
    for _ in range(layer_count):
        layer_id, count = struct.unpack_from("<BH", data, pos)
        pos += 3
        features: list[Feature] = []
        for _ in range(count):
            geom, cls, fflags, name_id, rank, rings_n = \
                struct.unpack_from("<BBBBBB", data, pos)
            pos += 6
            rings: list[list[tuple[int, int]]] = []
            for _ in range(rings_n):
                (npoints,) = struct.unpack_from("<H", data, pos)
                pos += 2
                pts: list[tuple[int, int]] = []
                x = y = 0
                for _ in range(npoints):
                    vx, pos = _read_varint(data, pos)
                    vy, pos = _read_varint(data, pos)
                    x += unzigzag(vx)
                    y += unzigzag(vy)
                    pts.append((x, y))
                rings.append(pts)
            features.append(Feature(
                geom=geom,
                cls=cls,
                rings=rings,
                name=strings[name_id] if name_id < len(strings) else "",
                rank=rank,
                flags=fflags,
            ))
        layers[layer_id] = features
    if pos != len(data):
        raise ValueError(f"{len(data) - pos} trailing bytes")
    return z, layers


def write_index(path, *, zmin: int, zmax: int, north: float, west: float,
                south: float, east: float, tiles: int, nbytes: int) -> None:
    """`vector.idx` — the marker and coverage the firmware checks for.

    The head unit only needs the zoom range to stop asking the card for levels
    the package does not contain; the bbox is there for the diagnostics and
    for whoever copies the folder.
    """
    def e7(v: float) -> int:
        return int(round(v * 1e7))

    path.write_bytes(struct.pack(
        "<4sBBBBiiiiII",
        b"NVTI",
        1,
        zmin & 0xFF,
        zmax & 0xFF,
        0,
        e7(north), e7(west), e7(south), e7(east),
        tiles,
        nbytes,
    ))


def read_index(path) -> dict:
    data = path.read_bytes()
    if len(data) < 32 or data[:4] != b"NVTI":
        raise ValueError("not a vector index")
    magic, version, zmin, zmax, _r, n, w, s, e, tiles, nbytes = \
        struct.unpack_from("<4sBBBBiiiiII", data, 0)
    if version != 1:
        raise ValueError(f"unsupported index version {version}")
    return {
        "zmin": zmin, "zmax": zmax,
        "north": n / 1e7, "west": w / 1e7,
        "south": s / 1e7, "east": e / 1e7,
        "tiles": tiles, "bytes": nbytes,
    }
