#include "text.h"
#include "state.h"
#include <lwcgl/glmodern.h>
#include <lwcgl/lwcgl.h>
#include "util/math.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../Vendor/stb_image.h"

static texture_registry_t g_registry;
glyph_uv_t glyphs[128];

static u32 g_text_vao = 0;
static u32 g_text_vbo = 0;
static u32 g_text_program = 0;
static texture_t* g_font_tex = NULL;

static void texture_upload_fallback(texture_t* tex)
{
    tex->filter = TEX_FILTER_NEAREST;
    tex->width = tex->height = 2;
    tex->channels = 3; tex->has_alpha = (tex->channels == 4);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex->width, tex->height, 0, GL_RGB, GL_UNSIGNED_BYTE, (u8[12]){ 180,180,180, 255,255,255, 255,255,255, 180,180,180 });
}

static void texture_set_params(const texture_t* tex)
{
    glBindTexture(GL_TEXTURE_2D, tex->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 
        tex->wrap_s == TEX_WRAP_REPEAT ? GL_REPEAT :
        tex->wrap_s == TEX_WRAP_CLAMP_TO_EDGE ? GL_CLAMP_TO_EDGE :
        tex->wrap_s == TEX_WRAP_CLAMP_TO_BORDER ? GL_CLAMP_TO_BORDER : GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
        tex->wrap_t == TEX_WRAP_REPEAT ? GL_REPEAT :
        tex->wrap_t == TEX_WRAP_CLAMP_TO_EDGE ? GL_CLAMP_TO_EDGE :
        tex->wrap_t == TEX_WRAP_CLAMP_TO_BORDER ? GL_CLAMP_TO_BORDER : GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        tex->filter == TEX_FILTER_NEAREST ? GL_NEAREST :
        tex->filter == TEX_FILTER_LINEAR ? GL_LINEAR :
        tex->filter == TEX_FILTER_NEAREST_MIPMAP ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
        tex->filter == TEX_FILTER_LINEAR || tex->filter == TEX_FILTER_LINEAR_MIPMAP ? GL_LINEAR : GL_NEAREST);
}

static texture_t* texture_alloc_slot()
{
    if (g_registry.count >= MAX_TEXTURES) {
        printf("Texture registry full!\n");
        return 0;
    }
    return &g_registry.textures[g_registry.count++];
}

texture_t* texture_create(const char* path, const tex_filter_t filter, const tex_wrap_t wrap)
{
    texture_t* tex = texture_alloc_slot();
    if (!tex) return &g_registry.fallback;

    glGenTextures(1, &tex->id);
    glBindTexture(GL_TEXTURE_2D, tex->id);

    tex->filter = filter;
    tex->wrap_s = wrap;
    tex->wrap_t = wrap;
    tex->has_alpha = false;
    strncpy(tex->name, path, sizeof(tex->name) - 1);
    tex->name[sizeof(tex->name) - 1] = '\0';

    i32 w, h, channels;
    stbi_set_flip_vertically_on_load(0);
    u8* data = stbi_load(path, &w, &h, &channels, 0);

    if (data) {
        tex->width = w;
        tex->height = h;
        tex->channels = channels;
        tex->has_alpha = (tex->channels == 4);

        const GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
        GL30.glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    } else {
        printf("Failed to load texture: %s, using fallback\n", path);
        texture_upload_fallback(tex);
    }

    texture_set_params(tex);
    return tex;
}

texture_t* texture_create_solid(const u32 r, const u32 g, const u32 b)
{
    texture_t* tex = texture_alloc_slot();
    if (!tex) return &g_registry.fallback;

    glGenTextures(1, &tex->id);
    glBindTexture(GL_TEXTURE_2D, tex->id);

    tex->filter = TEX_FILTER_LINEAR;
    tex->wrap_s = TEX_WRAP_REPEAT;
    tex->wrap_t = TEX_WRAP_REPEAT;
    tex->width = tex->height = 1;
    tex->channels = 3;
    tex->has_alpha = false;
    strcpy(tex->name, "solid_color");

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, (u8[3]){ (u8)r,(u8)g,(u8)b });
    texture_set_params(tex);
    return tex;
}

texture_t* texture_get_by_name(const char* name)
{
    for (i32 i = 0; i < g_registry.count; i++)
        if (strcmp(g_registry.textures[i].name, name) == 0) return &g_registry.textures[i];
    return 0;
}

void texture_bind(texture_t* tex, const u32 unit)
{
    if (!tex) tex = &g_registry.fallback;
    GLModern.glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, tex->id);
}

void texture_destroy(texture_t* tex)
{
    if (!tex || tex == &g_registry.fallback) return;
    glDeleteTextures(1, &tex->id);
    tex->id = 0;
}

void texture_registry_init(texture_registry_t* reg)
{
    memset(reg, 0, sizeof(*reg));
    reg->count = 0;

    memset(&g_registry, 0, sizeof(g_registry));
    g_registry.count = 0;

    // Create fallback texture
    glGenTextures(1, &reg->fallback.id);
    glBindTexture(GL_TEXTURE_2D, reg->fallback.id);
    reg->fallback.filter = TEX_FILTER_NEAREST;
    reg->fallback.wrap_s = TEX_WRAP_REPEAT;
    reg->fallback.wrap_t = TEX_WRAP_REPEAT;
    reg->fallback.width = reg->fallback.height = 2;
    reg->fallback.channels = 3;
    strcpy(reg->fallback.name, "fallback");

    texture_upload_fallback(&reg->fallback);
    texture_set_params(&reg->fallback);

    g_registry.fallback = reg->fallback;
}

void texture_registry_cleanup(texture_registry_t* reg)
{
    for (i32 i = 0; i < reg->count; i++) glDeleteTextures(1, &reg->textures[i].id);
    glDeleteTextures(1, &reg->fallback.id);
    memset(reg, 0, sizeof(*reg));
}

const texture_t* texture_get_fallback(void)
{
    return &g_registry.fallback;
}

static void text_init_glyphs(void)
{
    memset(glyphs, 0, sizeof(glyphs));
    glyphs[' '] = (glyph_uv_t){0, 2};
    glyphs['('] = (glyph_uv_t){8, 2};
    glyphs[')'] = (glyph_uv_t){9, 2};
    glyphs['*'] = (glyph_uv_t){10, 2};
    glyphs['+'] = (glyph_uv_t){11, 2};
    glyphs['-'] = (glyph_uv_t){13, 2};
    glyphs['.'] = (glyph_uv_t){14, 2};
    glyphs['/'] = (glyph_uv_t){15, 2};
    glyphs[':'] = (glyph_uv_t){10, 3};
    glyphs[';'] = (glyph_uv_t){11, 3};
    glyphs['<'] = (glyph_uv_t){12, 3};
    glyphs['='] = (glyph_uv_t){13, 3};
    glyphs['>'] = (glyph_uv_t){14, 3};
    glyphs['?'] = (glyph_uv_t){15, 3};
    glyphs['0'] = (glyph_uv_t){0, 3};
    glyphs['1'] = (glyph_uv_t){1, 3};
    glyphs['2'] = (glyph_uv_t){2, 3};
    glyphs['3'] = (glyph_uv_t){3, 3};
    glyphs['4'] = (glyph_uv_t){4, 3};
    glyphs['5'] = (glyph_uv_t){5, 3};
    glyphs['6'] = (glyph_uv_t){6, 3};
    glyphs['7'] = (glyph_uv_t){7, 3};
    glyphs['8'] = (glyph_uv_t){8, 3};
    glyphs['9'] = (glyph_uv_t){9, 3};
    glyphs['A'] = (glyph_uv_t){1, 4};
    glyphs['B'] = (glyph_uv_t){2, 4};
    glyphs['C'] = (glyph_uv_t){3, 4};
    glyphs['D'] = (glyph_uv_t){4, 4};
    glyphs['E'] = (glyph_uv_t){5, 4};
    glyphs['F'] = (glyph_uv_t){6, 4};
    glyphs['G'] = (glyph_uv_t){7, 4};
    glyphs['H'] = (glyph_uv_t){8, 4};
    glyphs['I'] = (glyph_uv_t){9, 4};
    glyphs['J'] = (glyph_uv_t){10, 4};
    glyphs['K'] = (glyph_uv_t){11, 4};
    glyphs['L'] = (glyph_uv_t){12, 4};
    glyphs['M'] = (glyph_uv_t){13, 4};
    glyphs['N'] = (glyph_uv_t){14, 4};
    glyphs['O'] = (glyph_uv_t){15, 4};
    glyphs['P'] = (glyph_uv_t){0, 5};
    glyphs['Q'] = (glyph_uv_t){1, 5};
    glyphs['R'] = (glyph_uv_t){2, 5};
    glyphs['S'] = (glyph_uv_t){3, 5};
    glyphs['T'] = (glyph_uv_t){4, 5};
    glyphs['U'] = (glyph_uv_t){5, 5};
    glyphs['V'] = (glyph_uv_t){6, 5};
    glyphs['W'] = (glyph_uv_t){7, 5};
    glyphs['X'] = (glyph_uv_t){8, 5};
    glyphs['Y'] = (glyph_uv_t){9, 5};
    glyphs['Z'] = (glyph_uv_t){10, 5};
    glyphs['_'] = (glyph_uv_t){15, 5};
    glyphs['a'] = (glyph_uv_t){1, 6};
    glyphs['b'] = (glyph_uv_t){2, 6};
    glyphs['c'] = (glyph_uv_t){3, 6};
    glyphs['d'] = (glyph_uv_t){4, 6};
    glyphs['e'] = (glyph_uv_t){5, 6};
    glyphs['f'] = (glyph_uv_t){6, 6};
    glyphs['g'] = (glyph_uv_t){7, 6};
    glyphs['h'] = (glyph_uv_t){8, 6};
    glyphs['i'] = (glyph_uv_t){9, 6};
    glyphs['j'] = (glyph_uv_t){10, 6};
    glyphs['k'] = (glyph_uv_t){11, 6};
    glyphs['l'] = (glyph_uv_t){12, 6};
    glyphs['m'] = (glyph_uv_t){13, 6};
    glyphs['n'] = (glyph_uv_t){14, 6};
    glyphs['o'] = (glyph_uv_t){15, 6};
    glyphs['p'] = (glyph_uv_t){0, 7};
    glyphs['q'] = (glyph_uv_t){1, 7};
    glyphs['r'] = (glyph_uv_t){2, 7};
    glyphs['s'] = (glyph_uv_t){3, 7};
    glyphs['t'] = (glyph_uv_t){4, 7};
    glyphs['u'] = (glyph_uv_t){5, 7};
    glyphs['v'] = (glyph_uv_t){6, 7};
    glyphs['w'] = (glyph_uv_t){7, 7};
    glyphs['x'] = (glyph_uv_t){8, 7};
    glyphs['y'] = (glyph_uv_t){9, 7};
    glyphs['z'] = (glyph_uv_t){10, 7};
}

static void text_ortho(f32* m, const f32 left, const f32 right, const f32 bottom, const f32 top)
{
    const f32 rl = right - left;
    const f32 tb = top - bottom;
    mat4_identity(m);
    m[0] = 2.0f / rl;
    m[5] = 2.0f / tb;
    m[10] = -1.0f;
    m[12] = -(right + left) / rl;
    m[13] = -(top + bottom) / tb;
}

void text_init()
{
    text_init_glyphs();

    g_font_tex = texture_create("Engine/res/font.png", TEX_FILTER_NEAREST, TEX_WRAP_CLAMP_TO_EDGE);
    if (!g_font_tex) {
        printf("Failed to load font texture, using fallback\n");
        g_font_tex = texture_get_fallback();
    }

    static const char* vs =
        "#version 330 core\n"
        "layout(location=0) in vec3 aPos;\n"
        "layout(location=1) in vec2 aUV;\n"
        "layout(location=2) in vec4 aColor;\n"
        "out vec2 vUV;\n"
        "out vec4 vColor;\n"
        "uniform mat4 u_proj;\n"
        "void main(){\n"
        "    vUV = aUV;\n"
        "    vColor = aColor;\n"
        "    gl_Position = u_proj * vec4(aPos, 1.0);\n"
        "}\n";
    static const char* fs =
        "#version 330 core\n"
        "in vec2 vUV;\n"
        "in vec4 vColor;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D u_font;\n"
        "void main(){\n"
        "    vec4 tex = texture(u_font, vUV);\n"
        "    FragColor = tex * vColor;\n"
        "}\n";

    g_text_program = create_program(vs, fs);

    GL30.glGenVertexArrays(1, &g_text_vao);
    GL15.glGenBuffers(1, &g_text_vbo);
    GL30.glBindVertexArray(g_text_vao);
    GL15.glBindBuffer(GL_ARRAY_BUFFER, g_text_vbo);
    GL15.glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_t) * MAX_TEXT_VERTICES, NULL, GL_DYNAMIC_DRAW);

    GL20.glEnableVertexAttribArray(0);
    GL20.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)0);
    GL20.glEnableVertexAttribArray(1);
    GL20.glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)(sizeof(f32) * 3));
    GL20.glEnableVertexAttribArray(2);
    GL20.glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)(sizeof(f32) * 5));

    GL30.glBindVertexArray(0);
}

u32 text_get_program(void)
{
    return g_text_program;
}

void text_shutdown(void)
{
    if (g_text_vbo) GL15.glDeleteBuffers(1, &g_text_vbo);
    if (g_text_vao) GL30.glDeleteVertexArrays(1, &g_text_vao);
    if (g_text_program) GL20.glDeleteProgram(g_text_program);
    g_text_vbo = 0;
    g_text_vao = 0;
    g_text_program = 0;
    g_font_tex = NULL;
}

void text_begin(void)
{
    state.text_vertex_count = 0;
}

static void _draw_char(const char c, const float x, const float y, const float char_width, const float char_height)
{
    if (state.text_vertex_count + 6 > MAX_TEXT_VERTICES) return;

    const float u0 = glyphs[(u8)c].x * 1.0f / 16.0f;
    const float v0 = glyphs[(u8)c].y * 1.0f / 16.0f;
    const float u1 = u0 + 1.0f / 16.0f;
    const float v1 = v0 + 1.0f / 16.0f;
    const float z = 0.0f;

    state.text_vertices[state.text_vertex_count++] = (vertex_t){{x, y, z}, {u0, v0}, {1.0f, 1.0f, 1.0f, 1.0f}};
    state.text_vertices[state.text_vertex_count++] = (vertex_t){{x + char_width, y + char_height, z}, {u1, v1}, {1.0f, 1.0f, 1.0f, 1.0f}};
    state.text_vertices[state.text_vertex_count++] = (vertex_t){{x + char_width, y, z}, {u1, v0}, {1.0f, 1.0f, 1.0f, 1.0f}};
    state.text_vertices[state.text_vertex_count++] = (vertex_t){{x, y, z}, {u0, v0}, {1.0f, 1.0f, 1.0f, 1.0f}};
    state.text_vertices[state.text_vertex_count++] = (vertex_t){{x, y + char_height, z}, {u0, v1}, {1.0f, 1.0f, 1.0f, 1.0f}};
    state.text_vertices[state.text_vertex_count++] = (vertex_t){{x + char_width, y + char_height, z}, {u1, v1}, {1.0f, 1.0f, 1.0f, 1.0f}};
}

void text_draw(const vec2s pos, const char* fmt, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (!buffer) return;
    float x = pos.x;
    for (const char* p = buffer; *p; p++) {
        _draw_char(*p, x, pos.y, CHAR_WIDTH, CHAR_HEIGHT);
        x += CHAR_WIDTH * CHAR_SPACING;
    }

}

void text_flush(const int fbw, const int fbh)
{
    if (state.text_vertex_count == 0 || !g_font_tex) return;
    if (fbw <= 0 || fbh <= 0) return;

    f32 proj[16];
    text_ortho(proj, 0.0f, (f32)fbw, (f32)fbh, 0.0f);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GL20.glUseProgram(g_text_program);
    GL20.glUniformMatrix4fv(GL20.glGetUniformLocation(g_text_program, "u_proj"), 1, GL_FALSE, proj);
    GL20.glUniform1i(GL20.glGetUniformLocation(g_text_program, "u_font"), 0);

    texture_bind(g_font_tex, 0);

    GL30.glBindVertexArray(g_text_vao);
    GL15.glBindBuffer(GL_ARRAY_BUFFER, g_text_vbo);
    GL15.glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex_t) * state.text_vertex_count, state.text_vertices);
    glDrawArrays(GL_TRIANGLES, 0, (i32)state.text_vertex_count);
    GL30.glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
