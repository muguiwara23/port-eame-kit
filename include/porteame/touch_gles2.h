#ifndef PORTEAME_TOUCH_GLES2_H
#define PORTEAME_TOUCH_GLES2_H
/* Port/eame kit: draws the on-screen controller with OpenGL ES 2.0 calls
 * (which iOS's OpenGL ES 3 context runs unchanged), for games that draw with
 * OpenGL ES themselves (EarthBound).
 *
 * pe_touch_gl_draw draws into whatever framebuffer is bound: on iOS that is
 * SDL's own, never 0 (see docs/es/lecciones.md). It sets the viewport to the
 * layout's size and puts back what it changes: the program, the 2D texture of
 * unit 0, the active texture unit, blending, the depth, stencil and scissor
 * tests, the array buffer, the viewport, the unpack alignment and whether
 * vertex attributes 0 and 1 are enabled. It overwrites attributes 0 and 1's
 * client pointers, which a game sets again before it draws anyway. It needs
 * no vertex array object bound (OpenGL ES 2 has none).
 *
 * Create, draw and destroy with the game's GL context current. */
#include "porteame/touch.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PeTouchGL PeTouchGL;

PeTouchGL *pe_touch_gl_create(void);
/* Draws what is shown now (nothing while hidden). 0 if the shaders failed. */
int pe_touch_gl_draw(PeTouchGL *gl, PeTouch *touch);
void pe_touch_gl_destroy(PeTouchGL *gl);

#ifdef __cplusplus
}
#endif
#endif
