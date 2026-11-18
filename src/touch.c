/* Port/eame kit: the on-screen controller's layout, fingers and bits
 * (porteame/touch.h). The same rules as Port/eame's Yu-Gi-Oh! Forbidden
 * Memories (touch_pad.c) and EarthBound (touch_controls.cpp) controllers,
 * which this replaces; only the console's shoulders and labels differ. */
#include "touch_internal.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

PeTouch *pe_touch_create(PeTouchStyle style)
{
    PeTouch *touch = calloc(1, sizeof(*touch));
    int b;
    if (!touch) return NULL;
    touch->style = style;
    touch->mode = PE_TOUCH_AUTO;
    for (b = 0; b < PE_TOUCH_BUTTONS; b++) touch->present[b] = 1;
    if (style == PE_TOUCH_SNES) touch->present[PE_TOUCH_L2] = touch->present[PE_TOUCH_R2] = 0;
    /* Until the game maps them, each button sends its own bit. */
    for (b = 0; b < PE_TOUCH_MENU; b++) touch->bits[b] = 1u << b;
    return touch;
}

void pe_touch_destroy(PeTouch *touch)
{
    if (!touch) return;
    pe_touch_forget_pictures(touch);
    free(touch);
}

PeTouchStyle pe_touch_style(const PeTouch *touch) { return touch->style; }

void pe_touch_map(PeTouch *touch, PeTouchButton button, uint32_t bits)
{
    if (button < PE_TOUCH_MENU) touch->bits[button] = bits;
}

uint32_t pe_touch_bits(const PeTouch *touch, PeTouchButton button)
{
    return button < PE_TOUCH_MENU ? touch->bits[button] : 0;
}

int pe_touch_shown(const PeTouch *touch)
{
    return touch->laid_out && touch->layout.width > 0 && touch->layout.height > 0 && !touch->blocked &&
           (touch->mode == PE_TOUCH_SHOW || (touch->mode == PE_TOUCH_AUTO && touch->touched));
}

static void place(PeTouch *touch, PeTouchButton button, float x, float y, float w, float h)
{
    touch->rects[button].x = x;
    touch->rects[button].y = y;
    touch->rects[button].w = w;
    touch->rects[button].h = h;
}

/* A button is about a thumb's width: 13% of the screen's short side and,
 * where the density is known, 48 to 80 points of it (the smallest target a
 * finger hits, and no bigger, so a tablet's buttons stay at the edges), but
 * never above 16% of the short side. On a phone the D-pad and the face
 * buttons keep to the bars beside the picture (each about 2.9 buttons wide
 * as drawn), down to Apple's 44-point target; where the picture fills the
 * screen they go over it. Everything keeps clear of the safe area's insets
 * (two thirds of them: the rounded corners leave room). */
static void lay_out(PeTouch *touch)
{
    const PeTouchLayout *l = &touch->layout;
    float shorter = (float)(l->width < l->height ? l->width : l->height), s = shorter * 0.13f, m;
    float left, right, pill_w, pill_h, gap, y0, menu_y, low, bottom;
    if (l->density > 0) {
        float least = 48 * l->density, most = 80 * l->density;
        if (s < least) s = least;
        if (s > most) s = most;
        if (s > shorter * 0.16f) s = shorter * 0.16f;
    }
    m = s * 0.35f;
    left = l->inset_left * 2.0f / 3 > m ? l->inset_left * 2.0f / 3 : m;
    right = l->inset_right * 2.0f / 3 > m ? l->inset_right * 2.0f / 3 : m;
    if (l->picture_right > l->picture_left) {
        float bar_left = l->picture_left - left, bar_right = l->width - l->picture_right - right;
        float fit = (bar_left < bar_right ? bar_left : bar_right) / 2.9f;
        if (fit < s && fit >= 44 * (l->density > 1 ? l->density : 1)) {
            s = fit;
            m = s * 0.35f;
        }
    }
    touch->unit = s;
    pill_w = s * 1.2f;
    pill_h = s * 0.55f;
    gap = s / 6;
    y0 = l->top + m / 2;
    /* The middles one button and a half (the cross's arm as drawn: 1.3) in. */
    touch->dpad_x = left + s * 1.3f;
    touch->face_x = l->width - right - s * 1.4f;
    if (touch->style == PE_TOUCH_PS1) {
        place(touch, PE_TOUCH_L2, left, y0, pill_w, pill_h);
        place(touch, PE_TOUCH_L1, left + pill_w + gap, y0, pill_w, pill_h);
        place(touch, PE_TOUCH_R1, l->width - right - 2 * pill_w - gap, y0, pill_w, pill_h);
        place(touch, PE_TOUCH_R2, l->width - right - pill_w, y0, pill_w, pill_h);
    } else {
        place(touch, PE_TOUCH_L1, left, y0, pill_w, pill_h);
        place(touch, PE_TOUCH_R1, l->width - right - pill_w, y0, pill_w, pill_h);
        place(touch, PE_TOUCH_L2, 0, 0, 0, 0);
        place(touch, PE_TOUCH_R2, 0, 0, 0, 0);
    }
    menu_y = y0 + pill_h + gap;
    place(touch, PE_TOUCH_MENU, left, menu_y, pill_w * 1.25f, pill_h * 0.9f);
    touch->dpad_y = touch->face_y = l->top + (l->height - l->top) * 0.56f;
    if (touch->dpad_y - s * 1.5f < menu_y + pill_h + gap) touch->dpad_y = touch->face_y = menu_y + pill_h + gap + s * 1.5f;
    place(touch, PE_TOUCH_UP, touch->dpad_x - s / 2, touch->dpad_y - s * 1.5f, s, s);
    place(touch, PE_TOUCH_DOWN, touch->dpad_x - s / 2, touch->dpad_y + s / 2, s, s);
    place(touch, PE_TOUCH_LEFT, touch->dpad_x - s * 1.5f, touch->dpad_y - s / 2, s, s);
    place(touch, PE_TOUCH_RIGHT, touch->dpad_x + s / 2, touch->dpad_y - s / 2, s, s);
    place(touch, PE_TOUCH_NORTH, touch->face_x - s / 2, touch->face_y - s * 1.5f, s, s);
    place(touch, PE_TOUCH_SOUTH, touch->face_x - s / 2, touch->face_y + s / 2, s, s);
    place(touch, PE_TOUCH_WEST, touch->face_x - s * 1.5f, touch->face_y - s / 2, s, s);
    place(touch, PE_TOUCH_EAST, touch->face_x + s / 2, touch->face_y - s / 2, s, s);
    low = l->inset_bottom / 2.0f > m / 2 ? l->inset_bottom / 2.0f : m / 2;
    bottom = l->height - low;
    place(touch, PE_TOUCH_SELECT, touch->dpad_x - s * 0.75f, bottom - pill_h, s * 1.5f, pill_h);
    place(touch, PE_TOUCH_START, touch->face_x - s * 0.75f, bottom - pill_h, s * 1.5f, pill_h);
}

static int refresh(PeTouch *touch);
static void release_all(PeTouch *touch)
{
    memset(touch->fingers, 0, sizeof(touch->fingers));
    refresh(touch);
}

int pe_touch_layout(PeTouch *touch, const PeTouchLayout *layout)
{
    PeTouchRect before[PE_TOUCH_BUTTONS];
    float before_unit = touch->unit;
    if (touch->laid_out && !memcmp(&touch->layout, layout, sizeof(*layout))) return 0;
    memcpy(before, touch->rects, sizeof(before));
    touch->layout = *layout;
    touch->laid_out = 1;
    lay_out(touch);
    if (before_unit == touch->unit && !memcmp(before, touch->rects, sizeof(before))) return 0;
    touch->generation++;
    /* Fingers held over the old places would press the wrong buttons. */
    release_all(touch);
    return 1;
}

/* A button's rectangle, a little larger to aim at; the short ones (pills) as
 * tall as a button to tap. */
static int in_button(const PeTouch *touch, PeTouchButton button, float x, float y)
{
    PeTouchRect r = touch->rects[button];
    float grow = touch->unit * 0.15f;
    if (!touch->present[button] || r.w <= 0) return 0;
    if (r.h < touch->unit) {
        r.y -= (touch->unit - r.h) / 2;
        r.h = touch->unit;
    }
    return x >= r.x - grow && y >= r.y - grow && x < r.x + r.w + grow && y < r.y + r.h + grow;
}

static float distance(float x, float y, float cx, float cy) { return hypotf(x - cx, y - cy); }

static PeZone zone_at(const PeTouch *touch, float x, float y, PeTouchButton *button)
{
    int b;
    float reach = touch->unit * 2; /* two buttons from the middle */
    if (distance(x, y, touch->dpad_x, touch->dpad_y) <= reach) return PE_ZONE_DPAD;
    if (distance(x, y, touch->face_x, touch->face_y) <= reach) return PE_ZONE_FACE;
    for (b = PE_TOUCH_L1; b < PE_TOUCH_BUTTONS; b++) {
        if (in_button(touch, (PeTouchButton)b, x, y)) {
            *button = (PeTouchButton)b;
            return PE_ZONE_BUTTON;
        }
    }
    return PE_ZONE_NONE;
}

/* The face button a finger presses: the nearest, unless it slid well away. */
static int face_button(const PeTouch *touch, const PeFingerState *finger)
{
    int b, nearest = -1;
    float best = 0;
    if (distance(finger->x, finger->y, touch->face_x, touch->face_y) > touch->unit * 3) return -1;
    for (b = PE_TOUCH_NORTH; b <= PE_TOUCH_WEST; b++) {
        const PeTouchRect *r = &touch->rects[b];
        float d = distance(finger->x, finger->y, r->x + r->w / 2, r->y + r->h / 2);
        if (nearest < 0 || d < best) {
            nearest = b;
            best = d;
        }
    }
    return nearest;
}

/* What a finger of its zone presses. The D-pad goes by the angle from its
 * middle, so a diagonal between two arrows presses both (67.5 degrees each
 * way); its middle presses nothing. */
static uint32_t press_at(const PeTouch *touch, const PeFingerState *finger)
{
    switch (finger->zone) {
    case PE_ZONE_DPAD: {
        float dx = finger->x - touch->dpad_x, dy = finger->y - touch->dpad_y, length = hypotf(dx, dy), c, s;
        uint32_t out = 0;
        if (length < touch->unit * 0.3f) return 0;
        c = dx / length;
        s = dy / length;
        if (c > 0.383f) out |= touch->bits[PE_TOUCH_RIGHT];
        if (c < -0.383f) out |= touch->bits[PE_TOUCH_LEFT];
        if (s > 0.383f) out |= touch->bits[PE_TOUCH_DOWN];
        if (s < -0.383f) out |= touch->bits[PE_TOUCH_UP];
        return out;
    }
    case PE_ZONE_FACE: {
        int b = face_button(touch, finger);
        return b < 0 ? 0 : touch->bits[b];
    }
    case PE_ZONE_BUTTON:
        return finger->button < PE_TOUCH_MENU && in_button(touch, finger->button, finger->x, finger->y)
                   ? touch->bits[finger->button] : 0;
    default:
        return 0;
    }
}

/* Which on-screen arrows a D-pad finger holds, for drawing. */
static unsigned dpad_shown(const PeTouch *touch, const PeFingerState *finger)
{
    float dx = finger->x - touch->dpad_x, dy = finger->y - touch->dpad_y, length = hypotf(dx, dy), c, s;
    unsigned out = 0;
    if (length < touch->unit * 0.3f) return 0;
    c = dx / length;
    s = dy / length;
    if (c > 0.383f) out |= 1u << PE_TOUCH_RIGHT;
    if (c < -0.383f) out |= 1u << PE_TOUCH_LEFT;
    if (s > 0.383f) out |= 1u << PE_TOUCH_DOWN;
    if (s < -0.383f) out |= 1u << PE_TOUCH_UP;
    return out;
}

static int refresh(PeTouch *touch)
{
    uint32_t now = 0;
    unsigned shown = 0;
    int i;
    for (i = 0; i < PE_FINGERS; i++) {
        const PeFingerState *f = &touch->fingers[i];
        if (!f->used) continue;
        now |= f->bits;
        if (f->zone == PE_ZONE_DPAD) {
            shown |= dpad_shown(touch, f);
        } else if (f->zone == PE_ZONE_FACE) {
            int b = face_button(touch, f);
            if (b >= 0) shown |= 1u << b;
        } else if (f->zone == PE_ZONE_BUTTON && in_button(touch, f->button, f->x, f->y)) {
            shown |= 1u << f->button;
        }
    }
    touch->presses |= now & ~touch->held;
    touch->tapped |= now & ~touch->held;
    if (now == touch->held && shown == touch->shown_held) return 0;
    touch->held = now;
    touch->shown_held = shown;
    return 1;
}

int pe_touch_set_mode(PeTouch *touch, PeTouchMode mode)
{
    int was = pe_touch_shown(touch);
    if (mode == touch->mode) return 0;
    touch->mode = mode;
    if (!pe_touch_shown(touch)) release_all(touch);
    return was != pe_touch_shown(touch);
}

int pe_touch_block(PeTouch *touch, int blocked)
{
    int was = pe_touch_shown(touch);
    blocked = !!blocked;
    if (blocked == touch->blocked) return 0;
    touch->blocked = blocked;
    if (blocked) release_all(touch);
    return was != pe_touch_shown(touch);
}

int pe_touch_other_input(PeTouch *touch)
{
    if (touch->mode != PE_TOUCH_AUTO || !touch->touched) return 0;
    touch->touched = 0;
    release_all(touch);
    return 1;
}

int pe_touch_covers(const PeTouch *touch, float x, float y)
{
    PeTouchButton button = PE_TOUCH_UP;
    return pe_touch_shown(touch) && zone_at(touch, x, y, &button) != PE_ZONE_NONE;
}

unsigned pe_touch_finger(PeTouch *touch, PeFinger kind, uint64_t id, float x, float y)
{
    PeFingerState *finger = NULL;
    int i;
    for (i = 0; i < PE_FINGERS; i++)
        if (touch->fingers[i].used && touch->fingers[i].id == id) finger = &touch->fingers[i];
    if (kind == PE_FINGER_DOWN) {
        PeTouchButton button = PE_TOUCH_UP;
        PeZone zone;
        if (!touch->laid_out || touch->mode == PE_TOUCH_HIDE || touch->blocked) return 0;
        if (!pe_touch_shown(touch)) {
            touch->touched = 1; /* the first touch shows the controller; it presses nothing */
            return PE_TOUCH_TAKEN | PE_TOUCH_CHANGED;
        }
        zone = zone_at(touch, x, y, &button);
        if (zone == PE_ZONE_NONE || finger) return 0;
        for (i = 0; i < PE_FINGERS && !finger; i++)
            if (!touch->fingers[i].used) finger = &touch->fingers[i];
        if (!finger) return PE_TOUCH_TAKEN;
        finger->used = 1;
        finger->id = id;
        finger->zone = zone;
        finger->button = button;
        finger->x = x;
        finger->y = y;
        finger->bits = press_at(touch, finger);
        if (zone == PE_ZONE_BUTTON && button == PE_TOUCH_MENU) touch->menu_taps++;
        return PE_TOUCH_TAKEN | (refresh(touch) ? PE_TOUCH_CHANGED : 0);
    }
    if (!finger) return 0;
    if (kind == PE_FINGER_MOVE) {
        finger->x = x;
        finger->y = y;
        finger->bits = press_at(touch, finger);
    } else {
        memset(finger, 0, sizeof(*finger));
    }
    return PE_TOUCH_TAKEN | (refresh(touch) ? PE_TOUCH_CHANGED : 0);
}

uint32_t pe_touch_buttons(PeTouch *touch)
{
    uint32_t out = touch->held | touch->tapped;
    touch->tapped = 0;
    return out;
}

uint32_t pe_touch_take_presses(PeTouch *touch)
{
    uint32_t out = touch->presses;
    touch->presses = 0;
    return out;
}

int pe_touch_take_menu(PeTouch *touch)
{
    int taps = touch->menu_taps;
    touch->menu_taps = 0;
    return taps != 0;
}

int pe_touch_rect(const PeTouch *touch, PeTouchButton button, PeTouchRect *rect)
{
    if (!pe_touch_shown(touch) || button >= PE_TOUCH_BUTTONS || !touch->present[button]) return 0;
    *rect = touch->rects[button];
    return 1;
}

int pe_touch_held(const PeTouch *touch, PeTouchButton button)
{
    return button < PE_TOUCH_BUTTONS && ((touch->shown_held >> button) & 1u);
}

int pe_touch_free_span(const PeTouch *touch, float *left, float *right)
{
    int b;
    float grow = touch->unit * 0.15f, middle = touch->layout.width / 2.0f;
    if (!pe_touch_shown(touch)) return 0;
    *left = touch->dpad_x + 2 * touch->unit; /* the D-pad's and the face buttons' reach (zone_at) */
    *right = touch->face_x - 2 * touch->unit;
    for (b = 0; b < PE_TOUCH_BUTTONS; b++) {
        const PeTouchRect *r = &touch->rects[b];
        if (!touch->present[b] || r->w <= 0) continue;
        if (r->x + r->w / 2 < middle) {
            if (r->x + r->w + grow > *left) *left = r->x + r->w + grow;
        } else if (r->x - grow < *right) {
            *right = r->x - grow;
        }
    }
    return 1;
}

unsigned pe_touch_generation(const PeTouch *touch) { return touch->generation; }
