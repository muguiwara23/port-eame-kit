#ifndef PORTEAME_TOUCH_SDL_H
#define PORTEAME_TOUCH_SDL_H
/* Port/eame kit: draws the on-screen controller with an SDL_Renderer, for
 * games that present through one (on iOS SDL picks Metal or OpenGL ES by
 * itself). Call pe_touch_sdl_draw after the game's frame and before
 * SDL_RenderPresent. The renderer's draw state (target, viewport, clip) is
 * the game's: the controller draws in output pixels, so the renderer should
 * have no logical presentation or scale set while it draws, or the caller
 * sets the layout in those units instead. */
#include "porteame/touch.h"
#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PeTouchSDL PeTouchSDL;

PeTouchSDL *pe_touch_sdl_create(SDL_Renderer *renderer);
/* Draws what is shown now (nothing while hidden). 0 on an SDL error. */
int pe_touch_sdl_draw(PeTouchSDL *sdl, PeTouch *touch);
void pe_touch_sdl_destroy(PeTouchSDL *sdl);

#ifdef __cplusplus
}
#endif
#endif
