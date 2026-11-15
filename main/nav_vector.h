/* Vector map tiles for the navigator, drawn on the head unit itself.
 *
 * The phone still owns GPS and routing, but instead of forwarding a picture
 * or a PNG per tile it says only where the rider is. The ground underneath
 * comes from a small binary tile on the microSD card, rasterized here into
 * the same RGB565 cache the PNG tiles used to land in. A tile is a few
 * kilobytes of geometry instead of 15-30 KB of PNG, and it carries names, so
 * the panel can draw labels that stay upright while the map turns.
 *
 * The format is deliberately small and C-friendly ("NVT1"):
 *
 *   header, little-endian
 *     u8  magic[4]      'N','V','T','1'
 *     u8  version       1
 *     u8  flags         bit0 = labels present (informational)
 *     u16 extent        tile units per side, always 4096
 *     u8  z             zoom the tile was cut for
 *     u8  layer_count
 *     u16 string_count
 *     u32 reserved
 *   string table, string_count entries:
 *     u8  len, then len bytes of UTF-8 (not NUL-terminated)
 *   layers, in draw order:
 *     u8  layer_id
 *     u16 feature_count
 *     features:
 *       u8  geom        0 line, 1 polygon, 2 point
 *       u8  cls         meaning depends on the layer
 *       u8  flags       bit0 bridge, bit1 tunnel
 *       u8  name_id     0xFF = unnamed
 *       u8  rank        0 = most important; labels only, 0 elsewhere
 *       u8  ring_count  1 for a line/point, 1..4 for a polygon
 *       rings:
 *         u16 point_count
 *         point_count x (zigzag varint dx, zigzag varint dy)
 *
 * Points are deltas in tile units from the previous point (the first from
 * 0,0). Deltas rather than 32-bit absolutes are what keeps a road of twenty
 * vertices under forty bytes. Coordinates may fall outside [0, extent): a
 * feature that crosses the tile edge is kept whole and clipped while it is
 * drawn, which is cheaper and more accurate than clipping every ring in the
 * generator.
 *
 * Layer ids and classes are fixed (see below). A rasterizer that does not
 * know a layer skips it, so a later generator can add one without breaking
 * an older firmware.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NAV_VECTOR_MAGIC0 'N'
#define NAV_VECTOR_MAGIC1 'V'
#define NAV_VECTOR_MAGIC2 'T'
#define NAV_VECTOR_MAGIC3 '1'
#define NAV_VECTOR_VERSION 1

/* Tile units per side. 4096 / 256 px = 16 units per pixel: fine enough for
 * street edges at z18 and still two bytes for most deltas. */
#define NAV_VECTOR_EXTENT 4096
#define NAV_VECTOR_PX     256

/* Layers, in the order they should be drawn. Kept as a fixed numbering
 * because it is the one part of the format the rasterizer hard-codes. */
#define NAV_V_LAYER_GREEN     2   /* cls 0 park/forest/grass */
#define NAV_V_LAYER_WATER     1   /* cls 0 water area */
#define NAV_V_LAYER_WATERWAY  7   /* cls 0 river, 1 stream/canal */
#define NAV_V_LAYER_BUILDING  3   /* cls 0 building */
#define NAV_V_LAYER_ROAD      4   /* cls 0..6, see nav_vector.c */
#define NAV_V_LAYER_RAIL      5   /* cls 0 rail, 1 tram */
#define NAV_V_LAYER_LABEL     6   /* cls is the label kind, rank orders them */

/* Label kinds carried in NAV_V_LAYER_LABEL's cls byte. */
#define NAV_V_LABEL_PLACE     0
#define NAV_V_LABEL_WATER     1
#define NAV_V_LABEL_PARK      2
#define NAV_V_LABEL_STREET    3
#define NAV_V_LABEL_LANDMARK  4

#define NAV_VECTOR_MAX_RINGS  4
#define NAV_VECTOR_MAX_POINTS 256
#define NAV_VECTOR_MAX_STRINGS 255
#define NAV_VECTOR_LABELS_MAX 4
#define NAV_VECTOR_LABEL_NAME 24

/* Local coordinates are allowed well outside the tile, because a feature that
 * crosses it is stored whole. A million units is still only sixty-four tiles
 * away; anything past that is a broken file, not geometry. */
#define NAV_VECTOR_COORD_LIMIT (1 << 20)

typedef struct {
    uint8_t kind;                    /* NAV_V_LABEL_* */
    uint8_t rank;                    /* 0 = most important */
    uint16_t x, y;                   /* tile-local pixels, 0..255 */
    char name[NAV_VECTOR_LABEL_NAME];
} nav_vector_label_t;

/* Check the header, string table and every feature without drawing. Useful
 * on the host and for the `navvec` debug command. */
bool nav_vector_validate(const uint8_t *data, size_t len);

/* Draw one tile into `dst` (256 x 256 RGB565, row-major, no padding) and
 * copy up to `max_labels` names into `labels`. Returns false for a broken or
 * unsupported tile; `dst` is background-filled even then. */
bool nav_vector_rasterize(const uint8_t *data, size_t len, uint16_t *dst,
                          nav_vector_label_t *labels, uint8_t *nlabels,
                          uint8_t max_labels);

/* How many labels the tile carries, without rasterizing it. */
int nav_vector_label_count(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif
