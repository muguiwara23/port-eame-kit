#ifndef PORTEAME_TOUCH_H
#define PORTEAME_TOUCH_H
/* Port/eame kit: the on-screen controller for iPhone and iPad.
 *
 * One controller for every Port/eame game, with the original console's
 * layout (SNES or PS1): a D-pad and the four face buttons at the sides of the
 * picture, the shoulder buttons as pills in the top corners, MENU under the
 * left shoulder (it opens the port's options), SELECT and START under the
 * D-pad and the face buttons, all clear of the notch and the home bar.
 *
 * This file is the logic only: the layout, the fingers and the game's bits.
 * It draws nothing and needs nothing but the C library, so a game drives it
 * from its own events and tests drive it directly. The pictures (dark glass
 * with a light rim and thin white symbols, monochrome) come from
 * pe_touch_sprites (touch_art.c), and touch_gles2.h / touch_sdl.h draw them.
 *
 * Every size and place is in drawable pixels (SDL_GetWindowSizeInPixels on
 * iOS, where SDL's window size is in points). Main thread only.
 *
 * Proven in Port/eame's Yu-Gi-Oh! Forbidden Memories (PS1) and EarthBound
 * (SNES) ports; see docs/es/lecciones.md. */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The buttons by place, so one set serves every console. The face buttons
 * are named by where they sit: SNES X (north), A (east), B (south), Y (west);
 * PS1 Triangle, Circle, Cross, Square. SNES's L and R are L1 and R1. */
typedef enum {
    PE_TOUCH_UP, PE_TOUCH_DOWN, PE_TOUCH_LEFT, PE_TOUCH_RIGHT,
    PE_TOUCH_NORTH, PE_TOUCH_EAST, PE_TOUCH_SOUTH, PE_TOUCH_WEST,
    PE_TOUCH_L1, PE_TOUCH_L2, PE_TOUCH_R1, PE_TOUCH_R2,
    PE_TOUCH_SELECT, PE_TOUCH_START,
    PE_TOUCH_MENU, /* no game bit: pe_touch_take_menu */
    PE_TOUCH_BUTTONS
} PeTouchButton;

/* Which console's controller: its buttons, labels and shoulders. */
typedef enum {
    PE_TOUCH_SNES, /* L R, X A B Y */
    PE_TOUCH_PS1   /* L2 L1 R1 R2, Triangle Circle Cross Square */
} PeTouchStyle;

/* AUTO shows the controller once the screen is touched and hides it again
 * when a key or a gamepad button is pressed (pe_touch_other_input); SHOW and
 * HIDE keep it so. */
typedef enum { PE_TOUCH_AUTO, PE_TOUCH_SHOW, PE_TOUCH_HIDE } PeTouchMode;

typedef enum { PE_FINGER_DOWN, PE_FINGER_MOVE, PE_FINGER_UP } PeFinger;

/* pe_touch_finger's answer, as bits. */
#define PE_TOUCH_TAKEN 1u   /* the finger is the controller's: not the game's or a menu's */
#define PE_TOUCH_CHANGED 2u /* what is shown changed (shown, a button held or let go): draw again */

typedef struct PeTouch PeTouch;

/* Where the controller goes. */
typedef struct {
    int width, height;               /* the drawable, in pixels */
    int top;                         /* pixels at the top kept free (a menu bar), usually 0 */
    int picture_left, picture_right; /* the game picture's columns; equal when unknown */
    float density;                   /* pixels per point (2 or 3 on a phone); 0 when unknown */
    int inset_left, inset_right, inset_bottom; /* the safe area's insets, in pixels */
} PeTouchLayout;

typedef struct {
    float x, y, w, h;
} PeTouchRect;

PeTouch *pe_touch_create(PeTouchStyle style);
void pe_touch_destroy(PeTouch *touch);
PeTouchStyle pe_touch_style(const PeTouch *touch);

/* The game's bits for a button (any bits of a 32-bit mask; 0 sends nothing).
 * This is also the remapping: in EarthBound the on-screen A (east) sends the
 * SNES L bit, so talking is one tap. */
void pe_touch_map(PeTouch *touch, PeTouchButton button, uint32_t bits);
uint32_t pe_touch_bits(const PeTouch *touch, PeTouchButton button);

/* Lays the controller out; cheap to call every frame (nothing moves unless
 * something changed). 1 when the layout changed. */
int pe_touch_layout(PeTouch *touch, const PeTouchLayout *layout);

/* The mode (PeTouchMode). 1 when what is shown changed. */
int pe_touch_set_mode(PeTouch *touch, PeTouchMode mode);
/* While a menu or a panel is up the controller steps aside: it draws
 * nothing, takes no finger and lets go of what it held. 1 when that changed
 * what is shown. */
int pe_touch_block(PeTouch *touch, int blocked);
/* A key or a gamepad button was pressed: AUTO hides the controller. 1 when
 * that changed what is shown. */
int pe_touch_other_input(PeTouch *touch);
int pe_touch_shown(const PeTouch *touch);

/* A finger, in drawable pixels; `id` is SDL's finger id. A finger that lands
 * on the controller stays its own until it lifts (sliding changes the
 * direction or the button); one that lands elsewhere is not its at all. The
 * first touch while hidden only shows the controller. PE_TOUCH_* bits. */
unsigned pe_touch_finger(PeTouch *touch, PeFinger kind, uint64_t id, float x, float y);
/* Whether a point is on the controller (so a mouse event SDL made of a
 * touch is not the game's or a menu's). 0 while hidden. */
int pe_touch_covers(const PeTouch *touch, float x, float y);

/* Once per frame: the game's bits held now, or pressed since the last call
 * and let go already (a tap shorter than a frame still counts once). */
uint32_t pe_touch_buttons(PeTouch *touch);
/* Bits newly pressed since the last call (for answering a notice). */
uint32_t pe_touch_take_presses(PeTouch *touch);
/* MENU was tapped since the last call. */
int pe_touch_take_menu(PeTouch *touch);

/* For drawing or for placing things over the picture. */
int pe_touch_rect(const PeTouch *touch, PeTouchButton button, PeTouchRect *rect); /* 0 when hidden or absent */
int pe_touch_held(const PeTouch *touch, PeTouchButton button); /* the on-screen button, whatever bits it sends */
/* The span between the controller's left and right columns. 0 while hidden. */
int pe_touch_free_span(const PeTouch *touch, float *left, float *right);
/* Changes whenever the layout does (every picture must be made again). */
unsigned pe_touch_generation(const PeTouch *touch);

/* --- pictures (touch_art.c) --------------------------------------------- */

/* One picture to draw: premultiplied RGBA, 8 bits a channel, rows from the
 * top, at (x, y) in drawable pixels, its soft shadow included. `slot` says
 * which picture it is (stable while the generation stays the same), so a
 * drawing backend keeps one texture per slot and generation. */
typedef struct {
    int slot;
    unsigned generation;
    int x, y, w, h;
    const uint8_t *rgba;
} PeTouchSprite;

#define PE_TOUCH_SLOTS (16 + 2 * PE_TOUCH_BUTTONS)

/* The pictures for what is shown now, in drawing order, made the first time
 * they are needed and kept. Returns how many (0 while hidden); `out` needs
 * PE_TOUCH_BUTTONS entries. */
int pe_touch_sprites(PeTouch *touch, PeTouchSprite *out, int max);

#ifdef __cplusplus
}
#endif
#endif
