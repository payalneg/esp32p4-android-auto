# Vector map tiles on the head unit

The navigator used to be a picture the phone rendered and pushed over BLE.
It now has a second source: an NVT1 package on the panel's microSD card,
rasterized by the P4 into the same RGB565 tile cache the PNG path used. The
phone keeps GPS, routing and manoeuvres; it stops sending pixels.

This document is the format and the integration order. The code is:

| What | Where |
| --- | --- |
| Format reader, rasterizer, label extraction | `main/nav_vector.c`, `main/nav_vector.h` |
| Tile cache integration | `main/nav_tiles.c`, `main/nav_tile_store.c` |
| Compositor + upright labels | `main/nav_map.c`, `main/nav_screen.c` |
| Packer + demo Kraków + preview | `scripts/mapgen/navtiles.py`, `scripts/mapgen/nvtile_format.py` |
| Wire change | `NAV_ST_STATE` bit 1, `main/ble_nav.c` / `lib/ble/nav_stream.dart` |

## The tile format: NVT1

Little-endian, 4096 units per side, drawn into 256×256 pixels (16 units per
pixel). A tile is a few kilobytes, not a 15–30 KB PNG.

```
header (16 bytes)
  u8  magic[4]      "NVT1"
  u8  version       1
  u8  flags         bit0: labels present
  u16 extent        4096
  u8  z
  u8  layer_count
  u16 string_count
  u32 reserved

string table (string_count entries)
  u8  len, len bytes of UTF-8 (not NUL-terminated)

layers, in draw order
  u8  layer_id
  u16 feature_count
  feature:
    u8  geom        0 line, 1 polygon, 2 point
    u8  cls         meaning by layer
    u8  flags       bit0 bridge, bit1 tunnel
    u8  name_id     0xFF = unnamed
    u8  rank        0 = most important (labels); 0 elsewhere
    u8  ring_count  1 for line/point, 1..4 for polygon
    rings:
      u16 point_count
      point_count × (zigzag varint dx, zigzag varint dy)
```

Points are deltas in tile units from the previous point; the first is from
0,0. A twenty-vertex road is about forty bytes. Coordinates may fall outside
`[0, 4096)` — a feature that crosses a tile edge is stored whole and clipped
while it is drawn. That is cheaper and more accurate than clipping every
ring in the packer, and it is why a tile can contain a river whose visible
piece is a corner.

### Layers and classes

| id | layer | class | draw |
| --- | --- | --- | --- |
| 2 | green | 0 park/forest/grass | filled polygon |
| 1 | water | 0 water area | filled + edge |
| 7 | waterway | 0 river, 1 stream | 4 px / 1 px line |
| 3 | building | 0 building | filled + 1 px edge |
| 4 | road | 0 motorway/trunk, 1 primary, 2 secondary/tertiary, 3 residential, 4 service, 5 path, 6 cycle | casing then fill |
| 5 | rail | 0 rail, 1 tram | 1 px |
| 6 | label | 0 place, 1 water, 2 park, 3 street, 4 landmark | not rasterized |

The file order is the draw order. The rasterizer hard-codes the ids but
skips an unknown layer, so a later packer can add one without breaking an
older firmware.

### Why not Mapbox Vector Tile

MVT is the obvious choice for a phone. Here the consumer is a 20 KB C file
with no protobuf, no allocation on the render path, and a 256-point budget
per feature. NVT1 is the smallest thing that satisfies that: fixed layers,
delta-varint coordinates, names in a flat table, no properties. A converter
from MVT is possible later; nothing in the firmware assumes this is the only
source.

## The card layout

```
/sdcard/navtiles/
  vector.idx          marker + zoom range + bbox (32 bytes)
  11/1135/1374.vtile
  ...
  18/145588/88833.vtile
```

The firmware also accepts the earlier `/sdcard/vtiles/...` root, so an
existing mapgen package works unchanged. A `.vtile` is preferred over a
`.png` for the same z/x/y; a card that still has raster tiles keeps working.

`vector.idx`:

```
u8  magic[4] "NVTI"
u8  version  1
u8  zmin
u8  zmax
u8  reserved
i32 north_e7, west_e7, south_e7, east_e7
u32 tile_count
u32 bytes
```

The firmware only needs `zmin`/`zmax`: the prefetch sweep uses them to stop
asking the card for z−6 when the package starts at z11.

## Firmware pipeline

```
BLE VIEW/ROUTE/GUIDE ──► nav_map view + route (unchanged)
                              │
nav_tile_store_read ──────────┤ prefers .vtile over .png
        │                     │
        ▼                     │
nav_tiles_put(fmt=VECTOR)     │
  nav_vector_rasterize ───────┤ RGB565 256×256 + up to 4 labels
        │                     │
        ▼                     │
  LRU slot in PSRAM ──────────┘
        │
        ▼
nav_map_render
  ├─ blit / rotate tiles (existing north-up and heading-up paths)
  ├─ route line + rider marker (existing)
  └─ collect labels, project with the same camera, publish a seqlocked list
        │
        ▼
nav_screen tick ──► LVGL labels over the image, horizontal by construction
```

The vector path is deliberately *inside* the existing cache. That gives it
the LRU, the PSRAM budget, the fallback layers and the "evicted quietly if
the card has it" rule for free. The compositor never learns whether a slot
came from PNG or NVT1.

`nav_vector_rasterize` fills its own 256×256 ground colour first, so a
vector tile is opaque and cannot show the slate "no tile yet" colour through
parks or water. Features are clipped by the scanline/segment loops, not by
the packer.

### Labels

The label layer is not drawn into RGB565. The rasterizer copies at most four
names per tile into the slot (tile-local pixel anchor, kind, rank), and
`nav_map_render` projects them with the same camera as the ground:

```
screen_x = w/2 + dx·cos(h) + dy·sin(h)
screen_y = h/2 − dx·sin(h) + dy·cos(h)
```

It de-duplicates by name (detail wins over a fallback tile), sorts by rank,
and greedily rejects overlapping boxes. `nav_screen.c` owns sixteen ordinary
LVGL labels above the image and below the HUD, so they stay horizontal at any
heading and no text is rasterized into the rotating frame.

## Integration order (what was done, and what to do on hardware)

1. **Format + packer.** `navtiles.py --demo` produces a 596-tile Kraków
   package in about 95 KiB (z11–z18). The same script takes `--pbf` for a
   real Geofabrik extract when pyosmium is installed.
2. **Reader + rasterizer.** `nav_vector.c` is pure C (no IDF, no allocator).
   `navvec <z> <x> <y>` rasterizes one tile from the card straight into the
   panel buffer and prints the time; that is the number to compare with the
   bench (6–14 ms at z17, 17–24 ms at z16, geometry in flash).
3. **Cache + prefetch.** `nav_tile_store_read` prefers `.vtile`;
   `nav_tiles_put` rasterizes into a slot; `prefetch_pass` asks for the
   visible detail tiles first, then the fallback levels the package actually
   contains. Budget is still two tiles per idle pass, so a screenful fills in
   about a second without freezing the worker.
4. **Compositor.** No change to the tile blit or rotation. `nav_map_render`
   now also collects labels; `nav_screen` draws them.
5. **Phone.** `NAV_ST_STATE` bit 1 says "ground is on the card". The app
   stops the tile queue and keeps VIEW, ROUTE and GUIDE. Older apps read the
   byte as a boolean and are unaffected.
6. **Measure.** On hardware, with no 2× antialiasing:
   - `navstat`: `vector ok`, `vector last ms`, `labels`, `compose us`,
     `msync us`, `last ms`, `sd cache ... vector=yes`.
   - `navvec z x y` for a cold tile: file read + rasterize, no cache.
   - cold start: time from VIEW to the first full frame; count `tiles
     stored` and `sd loaded`.
   - moving edges: watch `last_wanted - have`; a persistent gap means the
     prefetch budget or the package coverage is wrong.
   - rotation: `navheading 0`, `90`, `180`, `270`, then `auto`; labels must
     stay upright and not swim.
   - frame rate: `frames shown` over a fixed ride; expected ~8 fps at the
     current 120 ms frame period, limited by compose + msync, not by the
     card if prefetch keeps up.
7. **Then** evaluate 2× supersampling. The bench says 42–94 ms per tile; at
   two tiles per pass that is 84–188 ms, which would halve the frame rate
   while filling. A plausible middle ground is 2× only for the detail level
   and only for the tile under the rider, or a separate lower-resolution
   pass. Do not enable it globally before the no-AA numbers are known.

## Limits of this prototype

- The demo geometry is a hand-placed, recognisable Kraków (Rynek, Wawel,
  Vistula, Planty, Kazimierza, Kleparz), not an OSM extract. It exists so the
  firmware path can be tested offline. `--pbf` is the real-data path.
- Labels are a flat name list, rejected on overlap. There is no curved text,
  no street-name-along-the-road, no priority by zoom beyond the packer's
  `min_zoom` and rank.
- No 2× antialiasing yet; the rasterizer is single-sample. The bench numbers
  are the reason it is a separate step.
- One tile format, no compression. A z18 city-centre tile is tens of KB; a
  whole voivodeship at z18 would not fit a card, which is why the packer is
  meant for a city-sized region.
