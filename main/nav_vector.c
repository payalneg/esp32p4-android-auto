#include "nav_vector.h"

#include <math.h>
#include <string.h>

/* The tile's ground. Not the compositor's "no tile yet" slate: this is paper
 * with the parks, water and buildings printed on it. */
#define COLOUR_RGB(r, g, b) \
    ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

#define LAND        COLOUR_RGB(243, 242, 235)
#define WATER       COLOUR_RGB(166, 212, 232)
#define WATER_EDGE  COLOUR_RGB(135, 190, 214)
#define GREEN       COLOUR_RGB(220, 233, 210)
#define BUILDING    COLOUR_RGB(231, 223, 210)
#define BUILD_EDGE  COLOUR_RGB(211, 199, 183)
#define RAIL        COLOUR_RGB(120, 120, 120)
#define TRAM        COLOUR_RGB(150, 150, 150)

typedef struct {
    const uint8_t *base;
    const uint8_t *end;
    const uint8_t *strings;
    const uint8_t *layers;
    uint16_t string_count;
    uint8_t  z;
} nvt_t;

typedef struct {
    const uint8_t *p;
    const uint8_t *end;
} reader_t;

typedef struct {
    const uint8_t *features;   /* first feature of the layer */
    uint16_t       count;
    uint8_t        id;
} layer_t;

typedef struct {
    uint8_t  geom;             /* 0 line, 1 polygon, 2 point */
    uint8_t  cls;
    uint8_t  flags;
    uint8_t  name_id;
    uint8_t  rank;
    uint8_t  rings;
    uint16_t ring_start[NAV_VECTOR_MAX_RINGS + 1];
    uint16_t npoints;
} feature_t;

static inline uint16_t rd_u16(const uint8_t *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static inline uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool rd_u8(reader_t *r, uint8_t *out)
{
    if (r->p >= r->end) return false;
    *out = *r->p++;
    return true;
}

static bool rd_u16r(reader_t *r, uint16_t *out)
{
    if ((size_t)(r->end - r->p) < 2) return false;
    *out = rd_u16(r->p);
    r->p += 2;
    return true;
}

static bool rd_varint(reader_t *r, uint32_t *out)
{
    uint32_t v = 0;
    int shift = 0;
    for (int i = 0; i < 5; i++) {
        if (r->p >= r->end) return false;
        const uint8_t b = *r->p++;
        v |= (uint32_t)(b & 0x7F) << shift;
        if ((b & 0x80) == 0) {
            *out = v;
            return true;
        }
        shift += 7;
    }
    return false;                 /* more than 32 bits: not this format */
}

static inline int32_t zigzag(uint32_t v)
{
    return (int32_t)(v >> 1) ^ -(int32_t)(v & 1);
}

static bool nvt_begin(const uint8_t *data, size_t len, nvt_t *t)
{
    if (!data || !t || len < 16) return false;
    if (data[0] != NAV_VECTOR_MAGIC0 || data[1] != NAV_VECTOR_MAGIC1 ||
        data[2] != NAV_VECTOR_MAGIC2 || data[3] != NAV_VECTOR_MAGIC3) {
        return false;
    }
    if (data[4] != NAV_VECTOR_VERSION) return false;
    if (rd_u16(data + 6) != NAV_VECTOR_EXTENT) return false;
    const uint8_t z = data[8];
    const uint8_t layer_count = data[9];
    const uint16_t string_count = rd_u16(data + 10);
    if (z > 22 || layer_count == 0 || string_count > NAV_VECTOR_MAX_STRINGS) {
        return false;
    }

    const uint8_t *p = data + 16;
    const uint8_t *end = data + len;
    for (uint16_t i = 0; i < string_count; i++) {
        if (p >= end) return false;
        const uint8_t n = *p++;
        if ((size_t)(end - p) < n) return false;
        p += n;
    }

    t->base = data;
    t->end = end;
    t->strings = data + 16;
    t->layers = p;
    t->string_count = string_count;
    t->z = z;
    return true;
}

static const char *name_at(const nvt_t *t, uint8_t id)
{
    if (id == 0xFF || id >= t->string_count) return NULL;
    const uint8_t *p = t->strings;
    for (uint8_t i = 0; i < id; i++) {
        if (p >= t->end) return NULL;
        p += 1 + *p;
    }
    if (p >= t->end) return NULL;
    p++;                          /* skip the length byte */
    /* The format does not NUL-terminate. The caller copies with a bound, so
     * returning the pointer into the tile is enough here. */
    return (const char *)p;
}

static bool layer_first(const nvt_t *t, layer_t *l)
{
    if (t->layers + 3 > t->end) return false;
    l->id = t->layers[0];
    l->count = rd_u16(t->layers + 1);
    l->features = t->layers + 3;
    return true;
}

static bool skip_feature(reader_t *r)
{
    uint8_t hdr[6];
    for (int i = 0; i < 6; i++) if (!rd_u8(r, &hdr[i])) return false;
    const uint8_t rings = hdr[5];
    if (rings == 0 || rings > NAV_VECTOR_MAX_RINGS) return false;
    for (uint8_t ring = 0; ring < rings; ring++) {
        uint16_t n = 0;
        if (!rd_u16r(r, &n) || n == 0) return false;
        for (uint16_t i = 0; i < n; i++) {
            uint32_t v;
            if (!rd_varint(r, &v)) return false;
            if (!rd_varint(r, &v)) return false;
        }
    }
    return true;
}

static bool layer_next(const nvt_t *t, layer_t *l)
{
    reader_t r = { l->features, t->end };
    for (uint16_t i = 0; i < l->count; i++) {
        if (!skip_feature(&r)) return false;
    }
    if (r.p + 3 > t->end) return false;
    l->id = r.p[0];
    l->count = rd_u16(r.p + 1);
    l->features = r.p + 3;
    return true;
}

static bool read_feature(reader_t *r, feature_t *f, int32_t *pts, int max_pts)
{
    uint8_t hdr[6];
    for (int i = 0; i < 6; i++) if (!rd_u8(r, &hdr[i])) return false;
    f->geom = hdr[0];
    f->cls = hdr[1];
    f->flags = hdr[2];
    f->name_id = hdr[3];
    f->rank = hdr[4];
    f->rings = hdr[5];
    if (f->geom > 2 || f->rings == 0 || f->rings > NAV_VECTOR_MAX_RINGS) {
        return false;
    }

    int32_t x = 0, y = 0;
    f->npoints = 0;
    for (uint8_t ring = 0; ring < f->rings; ring++) {
        uint16_t n = 0;
        if (!rd_u16r(r, &n) || n == 0) return false;
        f->ring_start[ring] = f->npoints;
        if (f->npoints + n > max_pts) return false;
        for (uint16_t i = 0; i < n; i++) {
            uint32_t vx, vy;
            if (!rd_varint(r, &vx) || !rd_varint(r, &vy)) return false;
            x += zigzag(vx);
            y += zigzag(vy);
            if (x < -NAV_VECTOR_COORD_LIMIT || x > NAV_VECTOR_COORD_LIMIT ||
                y < -NAV_VECTOR_COORD_LIMIT || y > NAV_VECTOR_COORD_LIMIT) {
                return false;
            }
            pts[f->npoints * 2] = x;
            pts[f->npoints * 2 + 1] = y;
            f->npoints++;
        }
    }
    f->ring_start[f->rings] = f->npoints;

    if (f->geom == 2 && (f->rings != 1 || f->npoints < 1)) return false;
    if (f->geom == 0 && f->npoints < 2) return false;
    if (f->geom == 1 && f->npoints < 3) return false;
    return true;
}

/* ---- drawing ---- */

#define PX_PER_UNIT (256.0f / (float)NAV_VECTOR_EXTENT)

static inline void put(uint16_t *dst, int x, int y, uint16_t c)
{
    if ((unsigned)x < NAV_VECTOR_PX && (unsigned)y < NAV_VECTOR_PX) {
        dst[(size_t)y * NAV_VECTOR_PX + x] = c;
    }
}

/* Liang-Barsky against the tile grown by `margin`, so the ends of a segment
 * are not lost before its thickness is drawn. */
static bool clip_segment(float *x0, float *y0, float *x1, float *y1,
                         float margin)
{
    const float lo = -margin, hi = (float)(NAV_VECTOR_PX - 1) + margin;
    const float dx = *x1 - *x0, dy = *y1 - *y0;
    const float p[4] = { -dx, dx, -dy, dy };
    const float q[4] = { *x0 - lo, hi - *x0, *y0 - lo, hi - *y0 };
    float t0 = 0.0f, t1 = 1.0f;
    for (int i = 0; i < 4; i++) {
        if (p[i] == 0.0f) {
            if (q[i] < 0.0f) return false;
            continue;
        }
        const float r = q[i] / p[i];
        if (p[i] < 0.0f) {
            if (r > t1) return false;
            if (r > t0) t0 = r;
        } else {
            if (r < t0) return false;
            if (r < t1) t1 = r;
        }
    }
    const float ox = *x0, oy = *y0;
    *x0 = ox + t0 * dx;
    *y0 = oy + t0 * dy;
    *x1 = ox + t1 * dx;
    *y1 = oy + t1 * dy;
    return true;
}

static void stamp(uint16_t *dst, int cx, int cy, int half, uint16_t c)
{
    for (int y = cy - half; y <= cy + half; y++) {
        for (int x = cx - half; x <= cx + half; x++) put(dst, x, y, c);
    }
}

static void __attribute__((optimize("O2")))
draw_segment(uint16_t *dst, float x0, float y0, float x1, float y1,
             int width, uint16_t c)
{
    if (width < 1) width = 1;
    const int half = (width - 1) / 2;
    if (!clip_segment(&x0, &y0, &x1, &y1, (float)(half + 2))) return;

    const float dx = x1 - x0, dy = y1 - y0;
    const int steps = (int)ceilf(fmaxf(fabsf(dx), fabsf(dy)));
    if (steps <= 0) {
        stamp(dst, (int)lroundf(x0), (int)lroundf(y0), half, c);
        return;
    }
    const float sx = dx / (float)steps, sy = dy / (float)steps;
    if (fabsf(dx) >= fabsf(dy)) {
        /* Walk x and paint a vertical run: consecutive writes land in one
         * cache line, which matters on PSRAM. */
        float x = x0, y = y0;
        for (int i = 0; i <= steps; i++, x += sx, y += sy) {
            const int cx = (int)lroundf(x), cy = (int)lroundf(y);
            for (int o = -half; o <= half; o++) put(dst, cx, cy + o, c);
        }
    } else {
        float x = x0, y = y0;
        for (int i = 0; i <= steps; i++, x += sx, y += sy) {
            const int cx = (int)lroundf(x), cy = (int)lroundf(y);
            for (int o = -half; o <= half; o++) put(dst, cx + o, cy, c);
        }
    }
    if (half > 0) {
        stamp(dst, (int)lroundf(x0), (int)lroundf(y0), half, c);
        stamp(dst, (int)lroundf(x1), (int)lroundf(y1), half, c);
    }
}

/* A line is a chain, not a loop: the last vertex is not joined back to the
 * first, or every road would have a phantom segment across the tile. */
static void draw_polyline(uint16_t *dst, const int32_t *pts, uint16_t start,
                          uint16_t end, int width, uint16_t c)
{
    for (uint16_t i = start; i + 1 < end; i++) {
        draw_segment(dst, pts[i * 2] * PX_PER_UNIT,
                     pts[i * 2 + 1] * PX_PER_UNIT,
                     pts[(i + 1) * 2] * PX_PER_UNIT,
                     pts[(i + 1) * 2 + 1] * PX_PER_UNIT, width, c);
    }
}

/* A polygon edge does close, whether or not the generator repeated the first
 * point on the ring. */
static void draw_ring(uint16_t *dst, const int32_t *pts, uint16_t start,
                      uint16_t end, int width, uint16_t c)
{
    if (end <= start + 1) return;
    draw_polyline(dst, pts, start, end, width, c);
    draw_segment(dst, pts[(end - 1) * 2] * PX_PER_UNIT,
                 pts[(end - 1) * 2 + 1] * PX_PER_UNIT,
                 pts[start * 2] * PX_PER_UNIT,
                 pts[start * 2 + 1] * PX_PER_UNIT, width, c);
}

static void __attribute__((optimize("O2")))
fill_polygon(uint16_t *dst, const int32_t *pts, const feature_t *f, uint16_t c)
{
    float ymin = 1e9f, ymax = -1e9f;
    for (uint16_t i = 0; i < f->npoints; i++) {
        const float y = pts[i * 2 + 1] * PX_PER_UNIT;
        if (y < ymin) ymin = y;
        if (y > ymax) ymax = y;
    }
    int y0 = (int)floorf(ymin);
    int y1 = (int)ceilf(ymax);
    if (y0 < 0) y0 = 0;
    if (y1 > NAV_VECTOR_PX - 1) y1 = NAV_VECTOR_PX - 1;

    float xs[256];
    for (int y = y0; y <= y1; y++) {
        const float sy = (float)y + 0.5f;
        int nx = 0;
        for (uint8_t ring = 0; ring < f->rings; ring++) {
            const uint16_t a = f->ring_start[ring];
            const uint16_t b = f->ring_start[ring + 1];
            if (b <= a + 1) continue;
            for (uint16_t i = a; i < b; i++) {
                const uint16_t j = (i + 1 < b) ? (uint16_t)(i + 1) : a;
                const float ax = pts[i * 2] * PX_PER_UNIT;
                const float ay = pts[i * 2 + 1] * PX_PER_UNIT;
                const float bx = pts[j * 2] * PX_PER_UNIT;
                const float by = pts[j * 2 + 1] * PX_PER_UNIT;
                if ((ay <= sy && sy < by) || (by <= sy && sy < ay)) {
                    const float x = ax + (sy - ay) * (bx - ax) / (by - ay);
                    if (nx < (int)(sizeof xs / sizeof xs[0])) xs[nx++] = x;
                }
            }
        }
        if (nx < 2) continue;
        /* Insertion sort: a scanline crosses a handful of edges, and this
         * avoids a function-pointer call per pair in the tight loop. */
        for (int i = 1; i < nx; i++) {
            const float v = xs[i];
            int j = i - 1;
            while (j >= 0 && xs[j] > v) {
                xs[j + 1] = xs[j];
                j--;
            }
            xs[j + 1] = v;
        }
        uint16_t *row = dst + (size_t)y * NAV_VECTOR_PX;
        for (int i = 0; i + 1 < nx; i += 2) {
            int xa = (int)ceilf(xs[i] - 0.5f);
            int xb = (int)floorf(xs[i + 1] - 0.5f);
            if (xa < 0) xa = 0;
            if (xb > NAV_VECTOR_PX - 1) xb = NAV_VECTOR_PX - 1;
            for (int x = xa; x <= xb; x++) row[x] = c;
        }
    }
}

/* Road classes, in the order nav_vector.h documents them. */
typedef struct {
    uint16_t fill;
    uint16_t casing;
    uint8_t  width;
    bool     has_casing;
} road_style_t;

static const road_style_t k_roads[7] = {
    { COLOUR_RGB(232, 200, 138), COLOUR_RGB(185, 155, 99),  7, true  },
    { COLOUR_RGB(242, 217, 160), COLOUR_RGB(195, 174, 132), 7, true  },
    { COLOUR_RGB(255, 240, 194), COLOUR_RGB(201, 188, 153), 5, true  },
    { COLOUR_RGB(255, 255, 255), COLOUR_RGB(201, 201, 192), 3, true  },
    { COLOUR_RGB(255, 255, 255), COLOUR_RGB(213, 213, 204), 1, true  },
    { COLOUR_RGB(183, 183, 174), 0,                         1, false },
    { COLOUR_RGB(46, 158, 142),  COLOUR_RGB(232, 245, 242), 3, true  },
};

static void add_label(const nvt_t *t, const feature_t *f, const int32_t *pts,
                      nav_vector_label_t *labels, uint8_t *nlabels,
                      uint8_t max_labels)
{
    if (!labels || !nlabels || max_labels == 0) return;
    int x = (int)lroundf(pts[0] * PX_PER_UNIT);
    int y = (int)lroundf(pts[1] * PX_PER_UNIT);
    if (x < 0) x = 0;
    if (x >= NAV_VECTOR_PX) x = NAV_VECTOR_PX - 1;
    if (y < 0) y = 0;
    if (y >= NAV_VECTOR_PX) y = NAV_VECTOR_PX - 1;

    const char *name = name_at(t, f->name_id);
    if (!name) return;

    int slot = -1;
    if (*nlabels < max_labels) {
        slot = (*nlabels)++;
    } else {
        /* Full: only a more important name may displace the worst one. */
        uint8_t worst = 0;
        for (uint8_t i = 1; i < *nlabels; i++) {
            if (labels[i].rank > labels[worst].rank) worst = i;
        }
        if (labels[worst].rank <= f->rank) return;
        slot = worst;
    }

    nav_vector_label_t *l = &labels[slot];
    l->kind = f->cls;
    l->rank = f->rank;
    l->x = (uint16_t)x;
    l->y = (uint16_t)y;
    /* name_at only proves the string starts inside the tile; the length byte
     * is the byte before it, which is how the table is laid out. */
    const uint8_t *p = (const uint8_t *)name;
    const uint8_t n = p[-1];
    size_t copy = n < NAV_VECTOR_LABEL_NAME - 1 ? n : NAV_VECTOR_LABEL_NAME - 1;
    memcpy(l->name, name, copy);
    l->name[copy] = '\0';
}

/* Reads every feature of one layer into `pts`. `draw` decides what to do;
 * keeping iteration in one place means a layer can be walked twice (roads
 * need casing, then fill) without a second code path. */
static bool walk_layer(const nvt_t *t, const layer_t *l,
                       bool (*draw)(const nvt_t *, const layer_t *,
                                    const feature_t *, const int32_t *,
                                    int pass, void *ctx),
                       int pass, void *ctx)
{
    reader_t r = { l->features, t->end };
    int32_t pts[NAV_VECTOR_MAX_POINTS * 2];
    for (uint16_t i = 0; i < l->count; i++) {
        feature_t f;
        if (!read_feature(&r, &f, pts, NAV_VECTOR_MAX_POINTS)) return false;
        if (draw && !draw(t, l, &f, pts, pass, ctx)) return false;
    }
    return true;
}

typedef struct {
    uint16_t *dst;
    nav_vector_label_t *labels;
    uint8_t *nlabels;
    uint8_t max_labels;
} draw_ctx_t;

static bool draw_feature(const nvt_t *t, const layer_t *l, const feature_t *f,
                         const int32_t *pts, int pass, void *vctx)
{
    draw_ctx_t *ctx = vctx;
    uint16_t *dst = ctx->dst;

    if (l->id == NAV_V_LAYER_LABEL) {
        if (f->geom == 2 && pass == 0) {
            add_label(t, f, pts, ctx->labels, ctx->nlabels, ctx->max_labels);
        }
        return true;
    }

    if (l->id == NAV_V_LAYER_ROAD) {
        const int cls = f->cls < 7 ? f->cls : 5;
        const road_style_t *s = &k_roads[cls];
        if (pass == 0) {
            if (!s->has_casing) return true;
            for (uint8_t ring = 0; ring < f->rings; ring++) {
                draw_polyline(dst, pts, f->ring_start[ring],
                              f->ring_start[ring + 1], s->width + 2, s->casing);
            }
        } else {
            for (uint8_t ring = 0; ring < f->rings; ring++) {
                draw_polyline(dst, pts, f->ring_start[ring],
                              f->ring_start[ring + 1], s->width, s->fill);
            }
        }
        return true;
    }

    if (l->id == NAV_V_LAYER_BUILDING) {
        if (f->geom == 1) {
            fill_polygon(dst, pts, f, BUILDING);
            for (uint8_t ring = 0; ring < f->rings; ring++) {
                draw_ring(dst, pts, f->ring_start[ring],
                          f->ring_start[ring + 1], 1, BUILD_EDGE);
            }
        }
        return true;
    }

    if (l->id == NAV_V_LAYER_WATER) {
        if (f->geom == 1) {
            fill_polygon(dst, pts, f, WATER);
            for (uint8_t ring = 0; ring < f->rings; ring++) {
                draw_ring(dst, pts, f->ring_start[ring],
                          f->ring_start[ring + 1], 1, WATER_EDGE);
            }
        } else if (f->geom == 0) {
            for (uint8_t ring = 0; ring < f->rings; ring++) {
                draw_polyline(dst, pts, f->ring_start[ring],
                              f->ring_start[ring + 1], 4, WATER);
            }
        }
        return true;
    }

    if (l->id == NAV_V_LAYER_GREEN) {
        if (f->geom == 1) fill_polygon(dst, pts, f, GREEN);
        return true;
    }

    if (l->id == NAV_V_LAYER_WATERWAY) {
        if (f->geom == 0) {
            const int w = f->cls == 0 ? 4 : 1;
            for (uint8_t ring = 0; ring < f->rings; ring++) {
                draw_polyline(dst, pts, f->ring_start[ring],
                              f->ring_start[ring + 1], w, WATER);
            }
        }
        return true;
    }

    if (l->id == NAV_V_LAYER_RAIL) {
        if (f->geom == 0) {
            const uint16_t c = f->cls == 0 ? RAIL : TRAM;
            for (uint8_t ring = 0; ring < f->rings; ring++) {
                draw_polyline(dst, pts, f->ring_start[ring],
                              f->ring_start[ring + 1], 1, c);
            }
        }
        return true;
    }
    return true;   /* unknown layer: skip, a newer generator may add one */
}

static bool draw_all(const nvt_t *t, draw_ctx_t *ctx)
{
    layer_t l;
    if (!layer_first(t, &l)) return false;
    const uint8_t layer_count = t->base[9];
    for (uint8_t li = 0; li < layer_count; li++) {
        if (l.id == NAV_V_LAYER_ROAD) {
            /* Casing first over the whole layer, then the fill: drawing them
             * per feature lets a later road's casing cut across an earlier
             * road's fill, which looks like every junction is a bridge. */
            if (!walk_layer(t, &l, draw_feature, 0, ctx)) return false;
            if (!walk_layer(t, &l, draw_feature, 1, ctx)) return false;
        } else {
            if (!walk_layer(t, &l, draw_feature, 0, ctx)) return false;
        }
        if (li + 1 < layer_count && !layer_next(t, &l)) return false;
    }
    return true;
}

static int __attribute__((optimize("O2")))
rasterize_o2(const uint8_t *data, size_t len, uint16_t *dst,
             nav_vector_label_t *labels, uint8_t *nlabels, uint8_t max_labels)
{
    nvt_t t;
    if (!nvt_begin(data, len, &t)) return -1;

    if (nlabels) *nlabels = 0;
    for (int i = 0; i < NAV_VECTOR_PX * NAV_VECTOR_PX; i++) dst[i] = LAND;

    draw_ctx_t ctx = { dst, labels, nlabels, max_labels };
    if (!draw_all(&t, &ctx)) return -1;
    return 0;
}

bool nav_vector_rasterize(const uint8_t *data, size_t len, uint16_t *dst,
                          nav_vector_label_t *labels, uint8_t *nlabels,
                          uint8_t max_labels)
{
    if (!dst) return false;
    return rasterize_o2(data, len, dst, labels, nlabels, max_labels) == 0;
}

bool nav_vector_validate(const uint8_t *data, size_t len)
{
    nvt_t t;
    if (!nvt_begin(data, len, &t)) return false;
    layer_t l;
    if (!layer_first(&t, &l)) return false;
    const uint8_t layer_count = data[9];
    for (uint8_t li = 0; li < layer_count; li++) {
        reader_t r = { l.features, t.end };
        int32_t pts[NAV_VECTOR_MAX_POINTS * 2];
        for (uint16_t i = 0; i < l.count; i++) {
            feature_t f;
            if (!read_feature(&r, &f, pts, NAV_VECTOR_MAX_POINTS)) return false;
            if (f.name_id != 0xFF && f.name_id >= t.string_count) return false;
        }
        if (li + 1 < layer_count && !layer_next(&t, &l)) return false;
    }
    return true;
}

int nav_vector_label_count(const uint8_t *data, size_t len)
{
    nvt_t t;
    if (!nvt_begin(data, len, &t)) return 0;
    layer_t l;
    if (!layer_first(&t, &l)) return 0;
    const uint8_t layer_count = data[9];
    int count = 0;
    for (uint8_t li = 0; li < layer_count; li++) {
        if (l.id == NAV_V_LAYER_LABEL) {
            reader_t r = { l.features, t.end };
            int32_t pts[NAV_VECTOR_MAX_POINTS * 2];
            for (uint16_t i = 0; i < l.count; i++) {
                feature_t f;
                if (!read_feature(&r, &f, pts, NAV_VECTOR_MAX_POINTS)) return count;
                if (f.geom == 2) count++;
            }
        }
        if (li + 1 < layer_count && !layer_next(&t, &l)) return count;
    }
    return count;
}
