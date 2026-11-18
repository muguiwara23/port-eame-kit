/* Port/eame kit: the on-screen controller drawn with an SDL_Renderer
 * (porteame/touch_sdl.h). One texture per picture, blended premultiplied. */
#include "porteame/touch_sdl.h"
#include <stdlib.h>

struct PeTouchSDL {
    SDL_Renderer *renderer;
    SDL_Texture *textures[PE_TOUCH_SLOTS];
    unsigned generation;
    int have_generation;
};

PeTouchSDL *pe_touch_sdl_create(SDL_Renderer *renderer)
{
    PeTouchSDL *sdl = calloc(1, sizeof(*sdl));
    if (sdl) sdl->renderer = renderer;
    return sdl;
}

static void forget(PeTouchSDL *sdl)
{
    int k;
    for (k = 0; k < PE_TOUCH_SLOTS; k++)
        if (sdl->textures[k]) {
            SDL_DestroyTexture(sdl->textures[k]);
            sdl->textures[k] = NULL;
        }
}

void pe_touch_sdl_destroy(PeTouchSDL *sdl)
{
    if (!sdl) return;
    forget(sdl);
    free(sdl);
}

static SDL_Texture *upload(SDL_Renderer *renderer, const PeTouchSprite *sprite)
{
    /* ABGR8888 is R, G, B, A in memory: the pictures' byte order. */
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, sprite->w, sprite->h);
    if (!texture) return NULL;
    if (!SDL_UpdateTexture(texture, NULL, sprite->rgba, sprite->w * 4) ||
        !SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND_PREMULTIPLIED)) {
        SDL_DestroyTexture(texture);
        return NULL;
    }
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    return texture;
}

int pe_touch_sdl_draw(PeTouchSDL *sdl, PeTouch *touch)
{
    PeTouchSprite sprites[PE_TOUCH_BUTTONS];
    int count, k, ok = 1;
    if (!sdl->have_generation || sdl->generation != pe_touch_generation(touch)) {
        forget(sdl);
        sdl->generation = pe_touch_generation(touch);
        sdl->have_generation = 1;
    }
    count = pe_touch_sprites(touch, sprites, PE_TOUCH_BUTTONS);
    for (k = 0; k < count; k++) {
        const PeTouchSprite *s = &sprites[k];
        SDL_FRect place;
        if (!sdl->textures[s->slot]) sdl->textures[s->slot] = upload(sdl->renderer, s);
        if (!sdl->textures[s->slot]) {
            ok = 0;
            continue;
        }
        place.x = (float)s->x;
        place.y = (float)s->y;
        place.w = (float)s->w;
        place.h = (float)s->h;
        if (!SDL_RenderTexture(sdl->renderer, sdl->textures[s->slot], NULL, &place)) ok = 0;
    }
    return ok;
}
