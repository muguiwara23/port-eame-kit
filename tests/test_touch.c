/* Port/eame kit: the touch controller's logic, without drawing. */
#include "porteame/touch.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int failures;
#define CHECK(what)                                                                     \
    do {                                                                                \
        if (!(what)) {                                                                  \
            fprintf(stderr, "%s:%d: failed: %s\n", __FILE__, __LINE__, #what);          \
            failures++;                                                                 \
        }                                                                               \
    } while (0)

static void middle(PeTouch *t, PeTouchButton b, float *x, float *y)
{
    PeTouchRect r;
    if (!pe_touch_rect(t, b, &r)) {
        fprintf(stderr, "no rect for %d\n", (int)b);
        exit(1);
    }
    *x = r.x + r.w / 2;
    *y = r.y + r.h / 2;
}

/* An iPhone 12 Pro held sideways: 2532 x 1170 pixels, 3 per point, the
 * notch's inset 47 points each side and the home bar's 21 at the bottom. */
static PeTouchLayout phone(int left, int right)
{
    PeTouchLayout l = {2532, 1170, 0, left, right, 3.0f, 141, 141, 63};
    return l;
}

static void snes(void)
{
    PeTouch *t = pe_touch_create(PE_TOUCH_SNES);
    PeTouchLayout l = phone(597, 1935); /* the 8:7 picture, 1337 wide */
    PeTouchRect r, up, menu;
    float x, y, left, right;
    unsigned answer;
    /* EarthBound's bits: A sends L, Y sends A. */
    pe_touch_map(t, PE_TOUCH_UP, 1u << 11);
    pe_touch_map(t, PE_TOUCH_DOWN, 1u << 10);
    pe_touch_map(t, PE_TOUCH_LEFT, 1u << 9);
    pe_touch_map(t, PE_TOUCH_RIGHT, 1u << 8);
    pe_touch_map(t, PE_TOUCH_EAST, 1u << 5);
    pe_touch_map(t, PE_TOUCH_SOUTH, 1u << 15);
    pe_touch_map(t, PE_TOUCH_NORTH, 1u << 6);
    pe_touch_map(t, PE_TOUCH_WEST, 1u << 7);
    pe_touch_map(t, PE_TOUCH_L1, 1u << 5);
    pe_touch_map(t, PE_TOUCH_R1, 1u << 4);
    pe_touch_map(t, PE_TOUCH_SELECT, 1u << 13);
    pe_touch_map(t, PE_TOUCH_START, 1u << 12);

    CHECK(pe_touch_layout(t, &l) == 1);
    CHECK(pe_touch_layout(t, &l) == 0); /* nothing moved */
    CHECK(!pe_touch_shown(t));
    CHECK(!pe_touch_rect(t, PE_TOUCH_UP, &r));
    /* The first touch only shows the controller. */
    answer = pe_touch_finger(t, PE_FINGER_DOWN, 9, 1200, 500);
    CHECK(answer == (PE_TOUCH_TAKEN | PE_TOUCH_CHANGED));
    pe_touch_finger(t, PE_FINGER_UP, 9, 1200, 500);
    CHECK(pe_touch_shown(t));
    CHECK(pe_touch_buttons(t) == 0);
    /* SNES has no L2 or R2. */
    CHECK(!pe_touch_rect(t, PE_TOUCH_L2, &r));
    CHECK(!pe_touch_rect(t, PE_TOUCH_R2, &r));

    /* Everything inside the screen and clear of the notch's two thirds; the
     * outer arrow and face button reach a fifth of a button past that
     * margin, their glass (a cross arm of 1.3, a circle of 0.46) inside it. */
    {
        int b;
        pe_touch_rect(t, PE_TOUCH_UP, &up);
        for (b = 0; b < PE_TOUCH_BUTTONS; b++) {
            if (!pe_touch_rect(t, (PeTouchButton)b, &r)) continue;
            CHECK(r.x >= 141 * 2 / 3 - up.w * 0.21f && r.x + r.w <= 2532 - 141 * 2 / 3 + up.w * 0.21f);
            CHECK(r.x >= 0 && r.x + r.w <= 2532);
            CHECK(r.y >= 0 && r.y + r.h <= 1170);
        }
    }
    /* At 3 pixels a point the buttons keep to the bars: 44 to 80 points. */
    pe_touch_rect(t, PE_TOUCH_UP, &up);
    CHECK(up.w >= 44 * 3 - 0.5f && up.w <= 80 * 3 + 0.5f);
    CHECK(up.y > 0);
    pe_touch_rect(t, PE_TOUCH_MENU, &menu);
    CHECK(menu.y + menu.h < up.y);
    printf("snes: unit %.1f, up at %.0f,%.0f, menu at %.0f,%.0f\n", up.w, up.x, up.y, menu.x, menu.y);

    /* East (A) sends the L bit, west (Y) the A bit; shown as held themselves. */
    middle(t, PE_TOUCH_EAST, &x, &y);
    answer = pe_touch_finger(t, PE_FINGER_DOWN, 1, x, y);
    CHECK(answer == (PE_TOUCH_TAKEN | PE_TOUCH_CHANGED));
    CHECK(pe_touch_buttons(t) == (1u << 5));
    CHECK(pe_touch_held(t, PE_TOUCH_EAST));
    CHECK(!pe_touch_held(t, PE_TOUCH_L1));
    pe_touch_finger(t, PE_FINGER_UP, 1, x, y);
    CHECK(pe_touch_buttons(t) == 0);
    middle(t, PE_TOUCH_WEST, &x, &y);
    pe_touch_finger(t, PE_FINGER_DOWN, 2, x, y);
    CHECK(pe_touch_buttons(t) == (1u << 7));
    /* Sliding over to the north button changes the press. */
    middle(t, PE_TOUCH_NORTH, &x, &y);
    pe_touch_finger(t, PE_FINGER_MOVE, 2, x, y);
    CHECK(pe_touch_buttons(t) == (1u << 6));
    pe_touch_finger(t, PE_FINGER_UP, 2, x, y);

    /* A tap inside one frame still counts once. */
    middle(t, PE_TOUCH_SOUTH, &x, &y);
    pe_touch_finger(t, PE_FINGER_DOWN, 3, x, y);
    pe_touch_finger(t, PE_FINGER_UP, 3, x, y);
    CHECK(pe_touch_buttons(t) == (1u << 15));
    CHECK(pe_touch_buttons(t) == 0);
    CHECK(pe_touch_take_presses(t) & (1u << 15));

    /* The D-pad's diagonal presses two arrows; its middle presses nothing. */
    middle(t, PE_TOUCH_UP, &x, &y);
    {
        PeTouchRect right_rect;
        float rx, ry;
        middle(t, PE_TOUCH_RIGHT, &rx, &ry);
        (void)right_rect;
        pe_touch_finger(t, PE_FINGER_DOWN, 4, rx, y); /* up and to the right */
        CHECK(pe_touch_buttons(t) == ((1u << 11) | (1u << 8)));
        CHECK(pe_touch_held(t, PE_TOUCH_UP) && pe_touch_held(t, PE_TOUCH_RIGHT));
        pe_touch_finger(t, PE_FINGER_MOVE, 4, x, ry); /* the middle */
        CHECK(pe_touch_buttons(t) == 0);
        pe_touch_finger(t, PE_FINGER_UP, 4, x, ry);
    }

    /* MENU: a tap, no game bit. */
    middle(t, PE_TOUCH_MENU, &x, &y);
    pe_touch_finger(t, PE_FINGER_DOWN, 5, x, y);
    CHECK(pe_touch_take_menu(t));
    CHECK(!pe_touch_take_menu(t));
    CHECK(pe_touch_held(t, PE_TOUCH_MENU));
    CHECK(pe_touch_buttons(t) == 0);
    pe_touch_finger(t, PE_FINGER_UP, 5, x, y);

    /* The picture's middle is not the controller's. */
    CHECK(!pe_touch_covers(t, 1266, 585));
    CHECK(pe_touch_finger(t, PE_FINGER_DOWN, 6, 1266, 585) == 0);
    CHECK(pe_touch_free_span(t, &left, &right) && left < 1266 && right > 1266);
    printf("snes: free span %.0f to %.0f (picture %d to %d)\n", left, right, l.picture_left, l.picture_right);

    /* Blocked by a panel: hidden, lets go, takes nothing. */
    middle(t, PE_TOUCH_SOUTH, &x, &y);
    pe_touch_finger(t, PE_FINGER_DOWN, 7, x, y);
    CHECK(pe_touch_block(t, 1) == 1);
    CHECK(pe_touch_buttons(t) == (1u << 15)); /* the press already made still counts once */
    CHECK(pe_touch_buttons(t) == 0);
    CHECK(pe_touch_finger(t, PE_FINGER_DOWN, 8, x, y) == 0);
    CHECK(pe_touch_block(t, 0) == 1);

    /* A gamepad button hides it (AUTO); the next touch shows it again. */
    CHECK(pe_touch_other_input(t) == 1);
    CHECK(!pe_touch_shown(t));
    pe_touch_finger(t, PE_FINGER_DOWN, 9, x, y);
    CHECK(pe_touch_shown(t));
    CHECK(pe_touch_buttons(t) == 0);
    pe_touch_finger(t, PE_FINGER_UP, 9, x, y);

    /* Turning the phone (a new layout) lets go and changes the generation. */
    {
        unsigned generation = pe_touch_generation(t);
        PeTouchLayout turned = phone(597, 1935);
        turned.inset_left = 0;
        turned.inset_right = 0;
        middle(t, PE_TOUCH_SOUTH, &x, &y);
        pe_touch_finger(t, PE_FINGER_DOWN, 10, x, y);
        CHECK(pe_touch_layout(t, &turned) == 1);
        CHECK(pe_touch_generation(t) != generation);
        pe_touch_buttons(t);
        CHECK(pe_touch_buttons(t) == 0);
    }
    pe_touch_destroy(t);
}

static void ps1(void)
{
    PeTouch *t = pe_touch_create(PE_TOUCH_PS1);
    PeTouchLayout l = phone(486, 2046); /* 4:3, 1560 wide */
    PeTouchRect l2, l1, r1, r2;
    float x, y;
    CHECK(pe_touch_set_mode(t, PE_TOUCH_SHOW) == 0); /* not laid out yet: nothing shown changes */
    pe_touch_layout(t, &l);
    CHECK(pe_touch_shown(t));
    CHECK(pe_touch_rect(t, PE_TOUCH_L2, &l2) && pe_touch_rect(t, PE_TOUCH_L1, &l1));
    CHECK(pe_touch_rect(t, PE_TOUCH_R1, &r1) && pe_touch_rect(t, PE_TOUCH_R2, &r2));
    CHECK(l2.x < l1.x && r1.x < r2.x && l1.x + l1.w < r1.x);
    /* Unmapped, each button sends its own bit. */
    middle(t, PE_TOUCH_L2, &x, &y);
    pe_touch_finger(t, PE_FINGER_DOWN, 1, x, y);
    CHECK(pe_touch_buttons(t) == (1u << PE_TOUCH_L2));
    pe_touch_finger(t, PE_FINGER_UP, 1, x, y);
    /* SHOW mode: the first touch already presses. */
    middle(t, PE_TOUCH_SOUTH, &x, &y);
    CHECK(pe_touch_finger(t, PE_FINGER_DOWN, 2, x, y) & PE_TOUCH_TAKEN);
    CHECK(pe_touch_buttons(t) == (1u << PE_TOUCH_SOUTH));
    CHECK(pe_touch_other_input(t) == 0); /* SHOW stays */
    pe_touch_finger(t, PE_FINGER_UP, 2, x, y);
    CHECK(pe_touch_set_mode(t, PE_TOUCH_HIDE) == 1);
    CHECK(pe_touch_finger(t, PE_FINGER_DOWN, 3, x, y) == 0);
    pe_touch_destroy(t);
}

/* A tablet: the picture fills the screen, the controller goes over it, and
 * the buttons stay at 80 points at most. */
static void tablet(void)
{
    PeTouch *t = pe_touch_create(PE_TOUCH_SNES);
    PeTouchLayout l = {2732, 2048, 0, 0, 2732, 2.0f, 0, 0, 40};
    PeTouchRect up;
    pe_touch_set_mode(t, PE_TOUCH_SHOW);
    pe_touch_layout(t, &l);
    CHECK(pe_touch_rect(t, PE_TOUCH_UP, &up));
    CHECK(fabsf(up.w - 160) < 0.5f);
    pe_touch_destroy(t);
}

int main(void)
{
    snes();
    ps1();
    tablet();
    if (failures) {
        fprintf(stderr, "%d failures\n", failures);
        return 1;
    }
    puts("touch: all good");
    return 0;
}
