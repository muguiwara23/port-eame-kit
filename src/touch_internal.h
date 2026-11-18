#ifndef PORTEAME_TOUCH_INTERNAL_H
#define PORTEAME_TOUCH_INTERNAL_H
/* Port/eame kit: what touch.c and touch_art.c share. Not a public header. */
#include "porteame/touch.h"

#define PE_FINGERS 10

typedef enum { PE_ZONE_NONE, PE_ZONE_DPAD, PE_ZONE_FACE, PE_ZONE_BUTTON } PeZone;

typedef struct {
    int used;
    uint64_t id;
    PeZone zone;
    PeTouchButton button; /* PE_ZONE_BUTTON's */
    uint32_t bits;
    float x, y; /* where it is now */
} PeFingerState;

typedef struct {
    uint8_t *rgba;
    int x, y, w, h;
} PePicture;

struct PeTouch {
    PeTouchStyle style;
    uint32_t bits[PE_TOUCH_BUTTONS];
    int present[PE_TOUCH_BUTTONS]; /* the buttons this style shows */
    PeTouchLayout layout;
    int laid_out;
    PeTouchMode mode;
    int touched, blocked;
    PeTouchRect rects[PE_TOUCH_BUTTONS];
    float unit, dpad_x, dpad_y, face_x, face_y;
    unsigned generation;
    PeFingerState fingers[PE_FINGERS];
    uint32_t held, tapped, presses;
    unsigned shown_held; /* on-screen buttons held, a bit each */
    int menu_taps;
    PePicture pictures[PE_TOUCH_SLOTS];
    unsigned pictures_generation;
};

void pe_touch_forget_pictures(PeTouch *touch); /* touch_art.c */

#endif
