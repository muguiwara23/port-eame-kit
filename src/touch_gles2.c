/* Port/eame kit: the on-screen controller drawn with OpenGL ES 2.0
 * (porteame/touch_gles2.h). One texture per picture, blended premultiplied;
 * the textures are made again when the layout changes. */
#include "porteame/touch_gles2.h"
#include "touch_internal.h"
#include <SDL3/SDL_opengles2.h>
#include <stdio.h>
#include <stdlib.h>

enum { POSITION = 0, TEXCOORD = 1 };

struct PeTouchGL {
    GLuint program;
    GLint image;
    int failed;
    GLuint textures[PE_TOUCH_SLOTS];
    unsigned generation;
    int have_generation;
};

static const char vertex_source[] =
    "#version 100\n"
    "attribute vec2 Position;\n"
    "attribute vec2 TexCoord;\n"
    "varying vec2 UV;\n"
    "void main() { gl_Position = vec4(Position, 0.0, 1.0); UV = TexCoord; }\n";

static const char fragment_source[] =
    "#version 100\n"
    "precision mediump float;\n"
    "uniform sampler2D Image;\n"
    "varying vec2 UV;\n"
    "void main() { gl_FragColor = texture2D(Image, UV); }\n";

static GLuint compile(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    GLint ok = 0;
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512] = "";
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        fprintf(stderr, "Port/eame touch: shader: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static int make_program(PeTouchGL *gl)
{
    GLuint vertex = compile(GL_VERTEX_SHADER, vertex_source), fragment = compile(GL_FRAGMENT_SHADER, fragment_source);
    GLint ok = 0;
    if (!vertex || !fragment) {
        if (vertex) glDeleteShader(vertex);
        if (fragment) glDeleteShader(fragment);
        return 0;
    }
    gl->program = glCreateProgram();
    glAttachShader(gl->program, vertex);
    glAttachShader(gl->program, fragment);
    glBindAttribLocation(gl->program, POSITION, "Position");
    glBindAttribLocation(gl->program, TEXCOORD, "TexCoord");
    glLinkProgram(gl->program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glGetProgramiv(gl->program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512] = "";
        glGetProgramInfoLog(gl->program, sizeof(log), NULL, log);
        fprintf(stderr, "Port/eame touch: program: %s\n", log);
        glDeleteProgram(gl->program);
        gl->program = 0;
        return 0;
    }
    gl->image = glGetUniformLocation(gl->program, "Image");
    return 1;
}

PeTouchGL *pe_touch_gl_create(void) { return calloc(1, sizeof(PeTouchGL)); }

static void forget(PeTouchGL *gl)
{
    int k;
    for (k = 0; k < PE_TOUCH_SLOTS; k++)
        if (gl->textures[k]) {
            glDeleteTextures(1, &gl->textures[k]);
            gl->textures[k] = 0;
        }
}

void pe_touch_gl_destroy(PeTouchGL *gl)
{
    if (!gl) return;
    forget(gl);
    if (gl->program) glDeleteProgram(gl->program);
    free(gl);
}

static GLuint upload(const PeTouchSprite *sprite)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sprite->w, sprite->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, sprite->rgba);
    return texture;
}

static void enable(GLenum cap, GLboolean on)
{
    if (on) glEnable(cap);
    else glDisable(cap);
}

int pe_touch_gl_draw(PeTouchGL *gl, PeTouch *touch)
{
    PeTouchSprite sprites[PE_TOUCH_BUTTONS];
    PeTouchRect any;
    int count, k, width, height;
    GLint program = 0, texture = 0, active = 0, buffer = 0, viewport[4] = {0, 0, 0, 0}, alignment = 4;
    GLint blend_src_rgb = 0, blend_dst_rgb = 0, blend_src_a = 0, blend_dst_a = 0, attrib0 = 0, attrib1 = 0;
    GLboolean blend, depth, stencil, scissor;
    static const float uv[8] = {0, 0, 1, 0, 0, 1, 1, 1}; /* rows from the top: v 0 is the top */
    if (gl->failed) return 0;
    if (!pe_touch_rect(touch, PE_TOUCH_UP, &any)) return 1; /* hidden */
    if (!gl->program && !make_program(gl)) {
        gl->failed = 1;
        return 0;
    }
    if (!gl->have_generation || gl->generation != pe_touch_generation(touch)) {
        forget(gl);
        gl->generation = pe_touch_generation(touch);
        gl->have_generation = 1;
    }
    count = pe_touch_sprites(touch, sprites, PE_TOUCH_BUTTONS);
    if (!count) return 1;
    width = touch->layout.width; /* the layout's size is the drawable's */
    height = touch->layout.height;

    /* What this changes, to put back after. */
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
    glGetIntegerv(GL_BLEND_SRC_RGB, &blend_src_rgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &blend_dst_rgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blend_src_a);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blend_dst_a);
    glGetVertexAttribiv(POSITION, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attrib0);
    glGetVertexAttribiv(TEXCOORD, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attrib1);
    blend = glIsEnabled(GL_BLEND);
    depth = glIsEnabled(GL_DEPTH_TEST);
    stencil = glIsEnabled(GL_STENCIL_TEST);
    scissor = glIsEnabled(GL_SCISSOR_TEST);

    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); /* premultiplied pictures */
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glUseProgram(gl->program);
    glUniform1i(gl->image, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(POSITION);
    glEnableVertexAttribArray(TEXCOORD);
    glVertexAttribPointer(TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, uv);
    for (k = 0; k < count; k++) {
        const PeTouchSprite *s = &sprites[k];
        float x0 = (float)s->x / (float)width * 2 - 1, x1 = (float)(s->x + s->w) / (float)width * 2 - 1;
        float y0 = 1 - (float)s->y / (float)height * 2, y1 = 1 - (float)(s->y + s->h) / (float)height * 2;
        /* (x0,y0) is the picture's top left: v 0. */
        float positions[8];
        positions[0] = x0; positions[1] = y0;
        positions[2] = x1; positions[3] = y0;
        positions[4] = x0; positions[5] = y1;
        positions[6] = x1; positions[7] = y1;
        if (!gl->textures[s->slot]) gl->textures[s->slot] = upload(s);
        glBindTexture(GL_TEXTURE_2D, gl->textures[s->slot]);
        glVertexAttribPointer(POSITION, 2, GL_FLOAT, GL_FALSE, 0, positions);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    if (!attrib0) glDisableVertexAttribArray(POSITION);
    if (!attrib1) glDisableVertexAttribArray(TEXCOORD);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)buffer);
    glUseProgram((GLuint)program);
    glBindTexture(GL_TEXTURE_2D, (GLuint)texture);
    glActiveTexture((GLenum)active);
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    glBlendFuncSeparate((GLenum)blend_src_rgb, (GLenum)blend_dst_rgb, (GLenum)blend_src_a, (GLenum)blend_dst_a);
    enable(GL_BLEND, blend);
    enable(GL_DEPTH_TEST, depth);
    enable(GL_STENCIL_TEST, stencil);
    enable(GL_SCISSOR_TEST, scissor);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    return 1;
}
