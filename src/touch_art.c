/* Port/eame kit: the on-screen controller's pictures (pe_touch_sprites).
 *
 * Every control is drawn at the screen's own resolution: dark glass with a
 * light rim, a soft shadow under it and thin white symbols, in one colour
 * (monochrome). Held, a control lights up and its symbol turns fully white.
 * Each picture is drawn once for its size and kept, so a frame only blends
 * them; no image file and nothing of the game is used. Signed distances
 * (negative inside) give anti-aliased edges at any size.
 *
 * The look of Port/eame's Yu-Gi-Oh! Forbidden Memories (touch_pad_art_ios.c)
 * and EarthBound (touch_controls.cpp) controllers, with the labels in one
 * small stroke font instead of each game's own. */
#include "touch_internal.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* --- shapes -------------------------------------------------------------- */

static float clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
static float coverage(float d) { return clampf(0.5f - d, 0.0f, 1.0f); }
static float maxf(float a, float b) { return a > b ? a : b; }
static float minf(float a, float b) { return a < b ? a : b; }

static float rounded_box(float px, float py, float cx, float cy, float hw, float hh, float r)
{
    float qx = fabsf(px - cx) - hw + r, qy = fabsf(py - cy) - hh + r;
    float ox = maxf(qx, 0), oy = maxf(qy, 0);
    return sqrtf(ox * ox + oy * oy) + minf(maxf(qx, qy), 0) - r;
}

static float segment(float px, float py, float ax, float ay, float bx, float by)
{
    float vx = bx - ax, vy = by - ay, wx = px - ax, wy = py - ay;
    float t = clampf((wx * vx + wy * vy) / (vx * vx + vy * vy + 1e-9f), 0.0f, 1.0f);
    float dx = wx - vx * t, dy = wy - vy * t;
    return sqrtf(dx * dx + dy * dy);
}

/* A smooth union: the D-pad's inner corners are rounded. */
static float smooth_min(float a, float b, float k)
{
    float h = clampf(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
    return b + (a - b) * h - k * h * (1.0f - h);
}

typedef struct { float cx, cy, hw, hh, r; } Box;
static float box_shape(const void *data, float x, float y)
{
    const Box *b = data;
    return rounded_box(x, y, b->cx, b->cy, b->hw, b->hh, b->r);
}

typedef struct { float cx, cy, r; } Circle;
static float circle_shape(const void *data, float x, float y)
{
    const Circle *c = data;
    return hypotf(x - c->cx, y - c->cy) - c->r;
}

typedef struct { float cx, cy, arm, half, r, k, unit; unsigned held; } Cross;
static float cross_shape(const void *data, float x, float y)
{
    const Cross *c = data;
    return smooth_min(rounded_box(x, y, c->cx, c->cy, c->arm, c->half, c->r),
                      rounded_box(x, y, c->cx, c->cy, c->half, c->arm, c->r), c->k);
}

/* --- the canvas: premultiplied RGBA floats --------------------------------- */

typedef struct {
    int w, h;
    float *rgba;
} Canvas;

/* Porter-Duff "over" of a grey `level` (0 black, 1 white) at alpha `a`. */
static void over(Canvas *c, int i, int j, float level, float a)
{
    float *p, keep;
    if (a <= 0.0f || i < 0 || j < 0 || i >= c->w || j >= c->h) return;
    p = &c->rgba[((size_t)j * (size_t)c->w + (size_t)i) * 4];
    keep = 1.0f - a;
    p[0] = level * a + p[0] * keep;
    p[1] = level * a + p[1] * keep;
    p[2] = level * a + p[2] * keep;
    p[3] = a + p[3] * keep;
}

typedef float (*ShapeFn)(const void *shape, float x, float y);
typedef float (*LitFn)(const void *shape, float x, float y); /* 0-1: how lit the glass is there */

/* The glass of a control: a soft shadow below it, a dark tint (so it reads
 * over a bright picture), a white veil lighter towards the top, a gloss over
 * the top and a rim of light, brighter along the top edge. */
static void glass(Canvas *c, ShapeFn shape, const void *data, LitFn lit, float base_lit, float unit)
{
    int i, j;
    float rim = maxf(1.5f, unit * 0.022f), blur = unit * 0.06f, drop = unit * 0.035f;
    for (j = 0; j < c->h; j++) {
        float ny = (float)j / (float)(c->h > 1 ? c->h - 1 : 1);
        for (i = 0; i < c->w; i++) {
            float x = (float)i + 0.5f, y = (float)j + 0.5f, d = shape(data, x, y), inside = coverage(d);
            float shadow = shape(data, x, y - drop), light, band;
            if (shadow > -blur && inside < 1.0f)
                over(c, i, j, 0.0f, 0.30f * (shadow < 0 ? 1.0f : expf(-shadow / blur)) * (1.0f - inside));
            if (inside <= 0.0f) continue;
            light = clampf(base_lit + (lit ? lit(data, x, y) : 0.0f), 0.0f, 1.0f);
            over(c, i, j, 0.06f, 0.30f * inside);
            over(c, i, j, 1.0f, (0.13f + 0.10f * (1.0f - ny) + 0.22f * light) * inside);
            if (ny < 0.45f) over(c, i, j, 1.0f, 0.10f * (1.0f - ny / 0.45f) * inside);
            band = coverage(fabsf(d + rim * 0.5f) - rim * 0.5f);
            over(c, i, j, 1.0f, band * (0.28f + 0.42f * (1.0f - ny) + 0.25f * light));
        }
    }
}

/* Held arrows light their arm of the cross. */
static float cross_lit(const void *data, float x, float y)
{
    const Cross *c = data;
    float dx = x - c->cx, dy = y - c->cy, edge = c->unit * 0.45f, soft = c->unit * 0.12f, l = 0.0f;
    if (c->held & 1) l += clampf((-dy - edge) / soft, 0, 1);
    if (c->held & 2) l += clampf((dy - edge) / soft, 0, 1);
    if (c->held & 4) l += clampf((-dx - edge) / soft, 0, 1);
    if (c->held & 8) l += clampf((dx - edge) / soft, 0, 1);
    return l;
}

/* --- strokes -------------------------------------------------------------- */

#define MAX_SEGMENTS 128
typedef struct {
    int n;
    float s[MAX_SEGMENTS][4];
} Lines;

static void add_line(Lines *l, float ax, float ay, float bx, float by)
{
    if (l->n >= MAX_SEGMENTS) return;
    l->s[l->n][0] = ax;
    l->s[l->n][1] = ay;
    l->s[l->n][2] = bx;
    l->s[l->n][3] = by;
    l->n++;
}

static float lines_field(const void *data, float x, float y)
{
    const Lines *l = data;
    float best = 1e9f;
    int k;
    for (k = 0; k < l->n; k++) best = minf(best, segment(x, y, l->s[k][0], l->s[k][1], l->s[k][2], l->s[k][3]));
    return best;
}
static float ring_field(const void *data, float x, float y) { return fabsf(circle_shape(data, x, y)); }
static float box_outline_field(const void *data, float x, float y) { return fabsf(box_shape(data, x, y)); }

/* A white stroke `width` wide along a distance field. */
static void stroke(Canvas *c, ShapeFn field, const void *data, float width, float alpha)
{
    int i, j;
    for (j = 0; j < c->h; j++)
        for (i = 0; i < c->w; i++) {
            float cover = coverage(field(data, (float)i + 0.5f, (float)j + 0.5f) - width * 0.5f);
            if (cover > 0.0f) over(c, i, j, 1.0f, cover * alpha);
        }
}

/* A chevron pointing (dx, dy) at cx, cy. */
static void chevron(Lines *l, float cx, float cy, float dx, float dy, float size)
{
    float tx = cx + dx * size * 0.5f, ty = cy + dy * size * 0.5f, bx = cx - dx * size * 0.5f, by = cy - dy * size * 0.5f;
    float px = -dy * size, py = dx * size;
    add_line(l, bx + px, by + py, tx, ty);
    add_line(l, bx - px, by - py, tx, ty);
}

/* --- the stroke font -------------------------------------------------------- */
/* Each letter is polylines in a box one unit tall and `width` wide (y down):
 * x, y pairs, PEN lifts the pen, END ends the letter. */
#define PEN -1.0f
#define END -2.0f
typedef struct {
    char c;
    float width;
    const float *points;
} Glyph;

static const float g_A[] = {0, 1, .30f, 0, .60f, 1, PEN, .11f, .64f, .49f, .64f, END};
static const float g_B[] = {0, .48f, .40f, .48f, .52f, .37f, .52f, .11f, .40f, 0, 0, 0, 0, 1, .43f, 1, .56f, .88f,
                            .56f, .60f, .43f, .48f, END};
static const float g_C[] = {.54f, .13f, .42f, 0, .14f, 0, 0, .14f, 0, .86f, .14f, 1, .42f, 1, .54f, .87f, END};
static const float g_E[] = {.48f, 0, 0, 0, 0, 1, .48f, 1, PEN, 0, .5f, .40f, .5f, END};
static const float g_L[] = {0, 0, 0, 1, .46f, 1, END};
static const float g_M[] = {0, 1, 0, 0, .33f, .62f, .66f, 0, .66f, 1, END};
static const float g_N[] = {0, 1, 0, 0, .56f, 1, .56f, 0, END};
static const float g_R[] = {0, 1, 0, 0, .40f, 0, .52f, .12f, .52f, .36f, .40f, .48f, 0, .48f, PEN, .26f, .48f, .54f, 1,
                            END};
static const float g_S[] = {.52f, .12f, .41f, 0, .13f, 0, .01f, .12f, .01f, .36f, .12f, .47f, .43f, .53f, .54f, .64f,
                            .54f, .88f, .42f, 1, .12f, 1, 0, .88f, END};
static const float g_T[] = {0, 0, .56f, 0, PEN, .28f, 0, .28f, 1, END};
static const float g_U[] = {0, 0, 0, .80f, .17f, 1, .39f, 1, .56f, .80f, .56f, 0, END};
static const float g_X[] = {0, 0, .58f, 1, PEN, .58f, 0, 0, 1, END};
static const float g_Y[] = {0, 0, .29f, .5f, .58f, 0, PEN, .29f, .5f, .29f, 1, END};
static const float g_Z[] = {0, 0, .54f, 0, 0, 1, .54f, 1, END};
static const float g_1[] = {.04f, .20f, .26f, 0, .26f, 1, END};
static const float g_2[] = {.02f, .16f, .15f, 0, .39f, 0, .52f, .14f, .52f, .34f, 0, 1, .54f, 1, END};

static const Glyph font[] = {
    {'A', .60f, g_A}, {'B', .56f, g_B}, {'C', .54f, g_C}, {'E', .48f, g_E}, {'L', .46f, g_L}, {'M', .66f, g_M},
    {'N', .56f, g_N}, {'R', .54f, g_R}, {'S', .54f, g_S}, {'T', .56f, g_T}, {'U', .56f, g_U}, {'X', .58f, g_X},
    {'Y', .58f, g_Y}, {'Z', .54f, g_Z}, {'1', .30f, g_1}, {'2', .54f, g_2},
};
#define LETTER_GAP 0.22f

static const Glyph *glyph(char c)
{
    size_t k;
    for (k = 0; k < sizeof(font) / sizeof(font[0]); k++)
        if (font[k].c == c) return &font[k];
    return NULL;
}

static float text_width(const char *text)
{
    float w = 0;
    int any = 0;
    for (; *text; text++) {
        const Glyph *g = glyph(*text);
        if (g) {
            w += g->width + LETTER_GAP;
            any = 1;
        }
    }
    return any ? w - LETTER_GAP : 0;
}

/* Text `size` pixels tall, its box's left edge at x and its middle at y. */
static void add_text(Lines *out, const char *text, float x, float y, float size)
{
    float top = y - size * 0.5f;
    for (; *text; text++) {
        const Glyph *g = glyph(*text);
        const float *p;
        int pen = 0;
        float lx = 0, ly = 0;
        if (!g) continue;
        for (p = g->points; *p != END;) {
            if (*p == PEN) {
                pen = 0;
                p++;
                continue;
            }
            if (pen) add_line(out, x + lx * size, top + ly * size, x + p[0] * size, top + p[1] * size);
            lx = p[0];
            ly = p[1];
            pen = 1;
            p += 2;
        }
        x += (g->width + LETTER_GAP) * size;
    }
}

/* --- the pictures ------------------------------------------------------------ */

static const char *pill_label(const PeTouch *touch, PeTouchButton b)
{
    int ps1 = touch->style == PE_TOUCH_PS1;
    switch (b) {
    case PE_TOUCH_L1: return ps1 ? "L1" : "L";
    case PE_TOUCH_L2: return "L2";
    case PE_TOUCH_R1: return ps1 ? "R1" : "R";
    case PE_TOUCH_R2: return "R2";
    case PE_TOUCH_SELECT: return "SELECT";
    case PE_TOUCH_START: return "START";
    case PE_TOUCH_MENU: return "MENU";
    default: return "";
    }
}

static int begin(PePicture *picture, Canvas *canvas, float x, float y, float w, float h, int pad)
{
    picture->x = (int)x - pad;
    picture->y = (int)y - pad;
    picture->w = (int)w + 2 * pad;
    picture->h = (int)h + 2 * pad;
    canvas->w = picture->w;
    canvas->h = picture->h;
    canvas->rgba = calloc((size_t)canvas->w * (size_t)canvas->h * 4, sizeof(float));
    return canvas->rgba != NULL;
}

static void finish(PePicture *picture, Canvas *canvas)
{
    size_t k, n = (size_t)canvas->w * (size_t)canvas->h * 4;
    picture->rgba = malloc(n);
    if (picture->rgba)
        for (k = 0; k < n; k++) picture->rgba[k] = (uint8_t)lroundf(clampf(canvas->rgba[k], 0.0f, 1.0f) * 255.0f);
    free(canvas->rgba);
    canvas->rgba = NULL;
}

static void make_face(PeTouch *touch, PePicture *picture, PeTouchButton b, int held)
{
    const PeTouchRect r = touch->rects[b];
    Canvas c;
    Circle shape;
    float alpha = held ? 1.0f : 0.9f, width;
    if (!begin(picture, &c, r.x, r.y, r.w, r.h, (int)(touch->unit / 8))) return;
    shape.cx = c.w * 0.5f;
    shape.cy = c.h * 0.5f;
    shape.r = minf(r.w, r.h) * 0.46f;
    glass(&c, circle_shape, &shape, NULL, held ? 1.0f : 0.0f, touch->unit);
    width = maxf(1.5f, shape.r * 0.085f);
    if (touch->style == PE_TOUCH_PS1) {
        float R = shape.r;
        if (b == PE_TOUCH_EAST) {
            Circle ring = {0, 0, 0};
            ring.cx = shape.cx;
            ring.cy = shape.cy;
            ring.r = R * 0.33f;
            stroke(&c, ring_field, &ring, width, alpha);
        } else if (b == PE_TOUCH_WEST) {
            Box square = {0, 0, 0, 0, 0};
            square.cx = shape.cx;
            square.cy = shape.cy;
            square.hw = square.hh = R * 0.30f;
            square.r = R * 0.05f;
            stroke(&c, box_outline_field, &square, width, alpha);
        } else {
            static Lines l;
            l.n = 0;
            if (b == PE_TOUCH_SOUTH) {
                float k = R * 0.27f;
                add_line(&l, shape.cx - k, shape.cy - k, shape.cx + k, shape.cy + k);
                add_line(&l, shape.cx - k, shape.cy + k, shape.cx + k, shape.cy - k);
            } else { /* the triangle, its middle a little low so it looks centred */
                float T = R * 0.40f, cy = shape.cy + T * 0.12f;
                float ax = shape.cx, ay = cy - T, bx = shape.cx - T * 0.866f, by = cy + T * 0.5f, dx = shape.cx + T * 0.866f;
                add_line(&l, ax, ay, bx, by);
                add_line(&l, bx, by, dx, by);
                add_line(&l, dx, by, ax, ay);
            }
            stroke(&c, lines_field, &l, width, alpha);
        }
    } else {
        static Lines l;
        const char *text = b == PE_TOUCH_NORTH ? "X" : b == PE_TOUCH_EAST ? "A" : b == PE_TOUCH_SOUTH ? "B" : "Y";
        float size = shape.r * 0.62f;
        l.n = 0;
        add_text(&l, text, shape.cx - text_width(text) * size / 2, shape.cy, size);
        stroke(&c, lines_field, &l, width, alpha);
    }
    finish(picture, &c);
}

static void make_pill(PeTouch *touch, PePicture *picture, PeTouchButton b, int held)
{
    const PeTouchRect r = touch->rects[b];
    const char *text = pill_label(touch, b);
    static Lines label;
    Canvas c;
    Box shape;
    float size, play, total, left, alpha = held ? 1.0f : 0.9f;
    if (!begin(picture, &c, r.x, r.y, r.w, r.h, (int)(touch->unit / 8))) return;
    shape.cx = c.w * 0.5f;
    shape.cy = c.h * 0.5f;
    shape.hw = r.w * 0.5f;
    shape.hh = r.h * 0.5f;
    shape.r = r.h * 0.5f;
    glass(&c, box_shape, &shape, NULL, held ? 1.0f : 0.0f, touch->unit);
    size = r.h * (strlen(text) > 2 ? 0.36f : 0.44f);
    play = b == PE_TOUCH_START ? size * 0.75f : 0.0f; /* START's play triangle after the word */
    total = text_width(text) * size + (play > 0 ? play + size * 0.35f : 0.0f);
    left = shape.cx - total / 2;
    label.n = 0;
    add_text(&label, text, left, shape.cy, size);
    stroke(&c, lines_field, &label, maxf(1.5f, size * 0.13f), alpha);
    if (play > 0) {
        float px = left + text_width(text) * size + size * 0.35f, py = shape.cy;
        int i, j;
        for (j = 0; j < c.h; j++)
            for (i = 0; i < c.w; i++) {
                float fx = (float)i + 0.5f - px, fy = (float)j + 0.5f - py;
                float e1 = -fx, e2 = fx * 0.5f + fy * 0.866f - play * 0.433f, e3 = fx * 0.5f - fy * 0.866f - play * 0.433f;
                over(&c, i, j, 1.0f, coverage(maxf(e1, maxf(e2, e3))) * alpha);
            }
    }
    finish(picture, &c);
}

static void make_dpad(PeTouch *touch, PePicture *picture, unsigned arrows)
{
    const PeTouchRect left = touch->rects[PE_TOUCH_LEFT], up = touch->rects[PE_TOUCH_UP];
    static Lines chevrons;
    float s = touch->unit;
    Canvas c;
    Cross shape;
    if (!begin(picture, &c, left.x, up.y, s * 3, s * 3, (int)(s / 8))) return;
    shape.cx = touch->dpad_x - (float)picture->x;
    shape.cy = touch->dpad_y - (float)picture->y;
    shape.arm = s * 1.30f;
    shape.half = s * 0.42f;
    shape.r = s * 0.20f;
    shape.k = s * 0.14f;
    shape.unit = s;
    shape.held = arrows;
    glass(&c, cross_shape, &shape, cross_lit, 0.0f, s);
    chevrons.n = 0;
    chevron(&chevrons, shape.cx, shape.cy - s, 0, -1, s * 0.13f);
    chevron(&chevrons, shape.cx, shape.cy + s, 0, 1, s * 0.13f);
    chevron(&chevrons, shape.cx - s, shape.cy, -1, 0, s * 0.13f);
    chevron(&chevrons, shape.cx + s, shape.cy, 1, 0, s * 0.13f);
    stroke(&c, lines_field, &chevrons, s * 0.05f, 0.9f);
    finish(picture, &c);
}

void pe_touch_forget_pictures(PeTouch *touch)
{
    int k;
    for (k = 0; k < PE_TOUCH_SLOTS; k++) free(touch->pictures[k].rgba);
    memset(touch->pictures, 0, sizeof(touch->pictures));
}

static void emit(PeTouch *touch, PeTouchSprite *out, int *count, int max, int slot)
{
    const PePicture *p = &touch->pictures[slot];
    if (!p->rgba || *count >= max) return;
    out[*count].slot = slot;
    out[*count].generation = touch->generation;
    out[*count].x = p->x;
    out[*count].y = p->y;
    out[*count].w = p->w;
    out[*count].h = p->h;
    out[*count].rgba = p->rgba;
    (*count)++;
}

int pe_touch_sprites(PeTouch *touch, PeTouchSprite *out, int max)
{
    int count = 0, b;
    unsigned arrows;
    if (!pe_touch_shown(touch) || touch->unit <= 0) return 0;
    /* Pictures are drawn for the layout they were made for. */
    if (touch->pictures_generation != touch->generation) {
        pe_touch_forget_pictures(touch);
        touch->pictures_generation = touch->generation;
    }
    arrows = (pe_touch_held(touch, PE_TOUCH_UP) ? 1u : 0u) | (pe_touch_held(touch, PE_TOUCH_DOWN) ? 2u : 0u) |
             (pe_touch_held(touch, PE_TOUCH_LEFT) ? 4u : 0u) | (pe_touch_held(touch, PE_TOUCH_RIGHT) ? 8u : 0u);
    if (!touch->pictures[arrows].rgba) make_dpad(touch, &touch->pictures[arrows], arrows);
    emit(touch, out, &count, max, (int)arrows);
    for (b = PE_TOUCH_NORTH; b < PE_TOUCH_BUTTONS; b++) {
        int held = pe_touch_held(touch, (PeTouchButton)b), slot = 16 + 2 * b + held;
        if (!touch->present[b]) continue;
        if (!touch->pictures[slot].rgba) {
            if (b <= PE_TOUCH_WEST) make_face(touch, &touch->pictures[slot], (PeTouchButton)b, held);
            else make_pill(touch, &touch->pictures[slot], (PeTouchButton)b, held);
        }
        emit(touch, out, &count, max, slot);
    }
    return count;
}
