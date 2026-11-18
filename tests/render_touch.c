/* Port/eame kit: draws the touch controller as an iPhone 12 Pro would show
 * it, with OpenGL ES (into a framebuffer of its own, as on iOS, where there
 * is no framebuffer 0) and with an SDL_Renderer, and saves the pictures as
 * BMP files for a person to look at. Also checks that the OpenGL ES drawing
 * puts back the state it changes. Runs with SDL's offscreen driver. */
#include "porteame/touch.h"
#include "porteame/touch_gles2.h"
#include "porteame/touch_sdl.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengles2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 2532
#define H 1170

static int failures;
#define CHECK(what)                                                            \
    do {                                                                       \
        if (!(what)) {                                                         \
            fprintf(stderr, "%s:%d: failed: %s\n", __FILE__, __LINE__, #what); \
            failures++;                                                        \
        }                                                                      \
    } while (0)

static PeTouchLayout phone(PeTouchStyle style)
{
    /* EarthBound's 8:7 picture or Yu-Gi-Oh!'s 4:3 one, the notch's insets. */
    PeTouchLayout l = {W, H, 0, 0, 0, 3.0f, 141, 141, 63};
    int width = style == PE_TOUCH_SNES ? 1337 : 1560;
    l.picture_left = (W - width) / 2;
    l.picture_right = l.picture_left + width;
    return l;
}

static void press(PeTouch *t, PeTouchButton b, uint64_t id)
{
    PeTouchRect r;
    if (pe_touch_rect(t, b, &r)) pe_touch_finger(t, PE_FINGER_DOWN, id, r.x + r.w / 2, r.y + r.h / 2);
}

static void hold_some(PeTouch *t)
{
    PeTouchRect up, right;
    press(t, PE_TOUCH_EAST, 1);
    press(t, PE_TOUCH_L1, 2);
    press(t, PE_TOUCH_START, 3);
    pe_touch_rect(t, PE_TOUCH_UP, &up);
    pe_touch_rect(t, PE_TOUCH_RIGHT, &right);
    pe_touch_finger(t, PE_FINGER_DOWN, 4, right.x + right.w / 2, up.y + up.h / 2);
}

/* A stand-in for the game's picture: a bright and a dark half, so the glass
 * is seen over both. */
static void picture_gl(const PeTouchLayout *l)
{
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
    glScissor(l->picture_left, 0, l->picture_right - l->picture_left, H / 2);
    glClearColor(0.20f, 0.42f, 0.30f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glScissor(l->picture_left, H / 2, l->picture_right - l->picture_left, H / 2);
    glClearColor(0.86f, 0.80f, 0.62f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}

static void save_gl(const char *path)
{
    unsigned char *pixels = malloc((size_t)W * H * 4), *flipped = malloc((size_t)W * H * 4);
    SDL_Surface *surface;
    int y;
    glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    for (y = 0; y < H; y++) memcpy(flipped + (size_t)y * W * 4, pixels + (size_t)(H - 1 - y) * W * 4, (size_t)W * 4);
    surface = SDL_CreateSurfaceFrom(W, H, SDL_PIXELFORMAT_ABGR8888, flipped, W * 4);
    CHECK(surface && SDL_SaveBMP(surface, path));
    SDL_DestroySurface(surface);
    free(pixels);
    free(flipped);
}

static void gl_pictures(const char *prefix)
{
    SDL_Window *window;
    SDL_GLContext context;
    GLuint fbo, color, decoy_texture, decoy_program;
    char path[1024];
    int s;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    window = SDL_CreateWindow("touch", W, H, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    context = window ? SDL_GL_CreateContext(window) : NULL;
    if (!context) {
        fprintf(stderr, "no OpenGL ES context: %s\n", SDL_GetError());
        failures++;
        return;
    }
    /* The window's own framebuffer, as SDL's on iOS. */
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &color);
    glBindRenderbuffer(GL_RENDERBUFFER, color);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, W, H);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color);
    CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    /* State the game had, which must come back. */
    glGenTextures(1, &decoy_texture);
    glBindTexture(GL_TEXTURE_2D, decoy_texture);
    decoy_program = glCreateProgram();
    for (s = 0; s < 2; s++) {
        PeTouchStyle style = s == 0 ? PE_TOUCH_SNES : PE_TOUCH_PS1;
        const char *name = s == 0 ? "snes" : "ps1";
        PeTouch *t = pe_touch_create(style);
        PeTouchGL *gl = pe_touch_gl_create();
        PeTouchLayout l = phone(style);
        GLint viewport[4], bound = 0, texture = 0, rb = 0;
        pe_touch_set_mode(t, PE_TOUCH_SHOW);
        pe_touch_layout(t, &l);
        picture_gl(&l);
        glViewport(10, 20, 300, 200);
        CHECK(pe_touch_gl_draw(gl, t));
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &rb);
        CHECK(viewport[0] == 10 && viewport[1] == 20 && viewport[2] == 300 && viewport[3] == 200);
        CHECK(bound == (GLint)fbo);
        CHECK(rb == (GLint)color);
        CHECK(texture == (GLint)decoy_texture);
        CHECK(!glIsEnabled(GL_BLEND));
        CHECK(glGetError() == GL_NO_ERROR);
        snprintf(path, sizeof(path), "%s-gl-%s-idle.bmp", prefix, name);
        save_gl(path);
        hold_some(t);
        picture_gl(&l);
        CHECK(pe_touch_gl_draw(gl, t));
        CHECK(glGetError() == GL_NO_ERROR);
        snprintf(path, sizeof(path), "%s-gl-%s-held.bmp", prefix, name);
        save_gl(path);
        pe_touch_gl_destroy(gl);
        pe_touch_destroy(t);
    }
    glDeleteProgram(decoy_program);
    glDeleteTextures(1, &decoy_texture);
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &color);
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
}

static void renderer_picture(const char *prefix)
{
    SDL_Window *window = SDL_CreateWindow("touch", W, H, SDL_WINDOW_HIDDEN);
    SDL_Renderer *renderer = window ? SDL_CreateRenderer(window, "software") : NULL;
    PeTouch *t = pe_touch_create(PE_TOUCH_SNES);
    PeTouchSDL *sdl;
    PeTouchLayout l = phone(PE_TOUCH_SNES);
    SDL_FRect bright = {0, 0, 0, H / 2.0f};
    SDL_Surface *shot;
    char path[1024];
    if (!renderer) {
        fprintf(stderr, "no renderer: %s\n", SDL_GetError());
        failures++;
        return;
    }
    sdl = pe_touch_sdl_create(renderer);
    pe_touch_set_mode(t, PE_TOUCH_SHOW);
    pe_touch_layout(t, &l);
    hold_some(t);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    bright.x = (float)l.picture_left;
    bright.w = (float)(l.picture_right - l.picture_left);
    SDL_SetRenderDrawColor(renderer, 219, 204, 158, 255);
    SDL_RenderFillRect(renderer, &bright);
    bright.y = H / 2.0f;
    SDL_SetRenderDrawColor(renderer, 51, 107, 77, 255);
    SDL_RenderFillRect(renderer, &bright);
    CHECK(pe_touch_sdl_draw(sdl, t));
    shot = SDL_RenderReadPixels(renderer, NULL);
    snprintf(path, sizeof(path), "%s-sdl-snes-held.bmp", prefix);
    CHECK(shot && SDL_SaveBMP(shot, path));
    SDL_DestroySurface(shot);
    pe_touch_sdl_destroy(sdl);
    pe_touch_destroy(t);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

int main(int argc, char **argv)
{
    const char *prefix = argc > 1 ? argv[1] : "touch";
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    gl_pictures(prefix);
    renderer_picture(prefix);
    SDL_Quit();
    if (failures) {
        fprintf(stderr, "%d failures\n", failures);
        return 1;
    }
    puts("render: all good");
    return 0;
}
