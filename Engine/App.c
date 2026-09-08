#include "App.h"

#include "game.h"
#include "gfx.h"
#include "imgui_c.h"
#include "input.h"
#include "state.h"
#include "text.h"

#include <lwcgl/context.h>
#include <lwcgl/glmodern.h>
#include <lwcgl/lwcgl.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__has_include)
#  if __has_include(<rendercheck/capture.h>) && __has_include(<rendercheck/metrics.h>)
#    define RENDERCHECK_AVAILABLE 1
#    include <rendercheck/capture.h>
#    include <rendercheck/metrics.h>
#  endif
#endif
#ifndef RENDERCHECK_AVAILABLE
#  define RENDERCHECK_AVAILABLE 0
#endif

static bool g_game_initialized;
static double g_last_time;
static double g_fps_accum;
static u32 g_fps_frames;
static double g_fps_value;

u32 g_fbo;
static u32 g_fbo_color;
static u32 g_fbo_depth_stencil;
static i32 g_fbo_w;
static i32 g_fbo_h;

static u32 g_post_vao;
static u32 g_post_vbo;
static u32 g_post_program;

state_t state;

static int rendercheck_enabled(void)
{
#if RENDERCHECK_AVAILABLE
    return getenv("RENDERCHECK") != NULL;
#else
    return 0;
#endif
}

#if RENDERCHECK_AVAILABLE
static u32 g_rendercheck_gpu_queries[2];
static bool g_rendercheck_gpu_query_active;
static uint64_t g_rendercheck_frame_index;

static void rendercheck_gpu_begin(void)
{
    if (!rendercheck_enabled()) return;
    if (!g_rendercheck_gpu_queries[0])
        GL33.glGenQueries(2, g_rendercheck_gpu_queries);
    if (!g_rendercheck_gpu_queries[0] || !g_rendercheck_gpu_queries[1]) return;

    GL33.glQueryCounter(g_rendercheck_gpu_queries[0], GL_TIMESTAMP);
    g_rendercheck_gpu_query_active = true;
}

static void rendercheck_gpu_end(void)
{
    if (!g_rendercheck_gpu_query_active) return;

    GL33.glQueryCounter(g_rendercheck_gpu_queries[1], GL_TIMESTAMP);
    g_rendercheck_gpu_query_active = false;

    GLuint64 start_ns = 0;
    GLuint64 end_ns = 0;
    GL33.glGetQueryObjectui64v(g_rendercheck_gpu_queries[0], GL_QUERY_RESULT, &start_ns);
    GL33.glGetQueryObjectui64v(g_rendercheck_gpu_queries[1], GL_QUERY_RESULT, &end_ns);

    if (end_ns >= start_ns && rendercheck_gpu_ms((double)(end_ns - start_ns) / 1000000.0) < 0)
        fprintf(stderr, "RendererCheck: failed to write GPU metric\n");
}

static void rendercheck_capture_frame(uint64_t frame_index)
{
    if (!rendercheck_enabled() || !rendercheck_capture_due(frame_index)) return;

    const i32 width = Display.getWidth();
    const i32 height = Display.getHeight();
    if (width <= 0 || height <= 0) return;

    const size_t row_bytes = (size_t)width * 3u;
    unsigned char* pixels = (unsigned char*)malloc(row_bytes * (size_t)height);
    if (!pixels) return;

    GLModern.glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    GLModern.glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    for (i32 y = 0; y < height / 2; ++y) {
        unsigned char* top = pixels + (size_t)y * row_bytes;
        unsigned char* bottom = pixels + (size_t)(height - 1 - y) * row_bytes;
        for (size_t x = 0; x < row_bytes; ++x) {
            const unsigned char tmp = top[x];
            top[x] = bottom[x];
            bottom[x] = tmp;
        }
    }

    if (rendercheck_capture_rgb8(pixels, (uint32_t)width, (uint32_t)height, row_bytes) < 0)
        fprintf(stderr, "RendererCheck: failed to write frame capture\n");
    free(pixels);
}

static void rendercheck_gpu_shutdown(void)
{
    if (g_rendercheck_gpu_queries[0] || g_rendercheck_gpu_queries[1])
        GL33.glDeleteQueries(2, g_rendercheck_gpu_queries);
    g_rendercheck_gpu_queries[0] = 0;
    g_rendercheck_gpu_queries[1] = 0;
    g_rendercheck_gpu_query_active = false;
}
#else
static void rendercheck_gpu_begin(void) {}
static void rendercheck_gpu_end(void) {}
static void rendercheck_capture_frame(uint64_t frame_index) { (void)frame_index; }
static void rendercheck_gpu_shutdown(void) {}
#endif

static double app_now_seconds(void)
{
    const double resolution = (double)Sys.getTimerResolution();
    if (resolution <= 0.0) return 0.0;
    return (double)Sys.getTime() / resolution;
}

static bool required_gl_available(void)
{
    return lwcglModernGLAvailable() &&
           GL15.glGenBuffers && GL15.glBindBuffer && GL15.glBufferData && GL15.glDeleteBuffers &&
           GL20.glCreateShader && GL20.glCreateProgram && GL20.glUseProgram &&
           GL20.glGetUniformLocation && GL20.glUniform1i && GL20.glUniform1f &&
           GL20.glEnableVertexAttribArray && GL20.glVertexAttribPointer &&
           GL30.glGenVertexArrays && GL30.glBindVertexArray && GL30.glDeleteVertexArrays &&
           GL30.glGenFramebuffers && GL30.glBindFramebuffer && GL30.glFramebufferTexture2D &&
           GL30.glCheckFramebufferStatus && GL30.glDeleteFramebuffers &&
           GL33.glGenQueries && GL33.glDeleteQueries && GL33.glQueryCounter && GL33.glGetQueryObjectui64v &&
           GLModern.glActiveTexture && GLModern.glPixelStorei && GLModern.glReadPixels;
}

static void update_framebuffer_metrics(void)
{
    if (!state.fb) return;

    state.fb->w = Display.getWidth();
    state.fb->h = Display.getHeight();
    state.fb->ww = lwcglDisplayGetWindowWidth();
    state.fb->wh = lwcglDisplayGetWindowHeight();

    if (state.fb->ww <= 0) state.fb->ww = state.fb->w;
    if (state.fb->wh <= 0) state.fb->wh = state.fb->h;
    state.fb->scale = state.fb->ww > 0 ? (f32)state.fb->w / (f32)state.fb->ww : 1.0f;
}

void fbo_resize(const i32 w, const i32 h)
{
    if (w <= 0 || h <= 0) return;
    if (g_fbo_w == w && g_fbo_h == h) return;

    if (!g_fbo)
        GL30.glGenFramebuffers(1, &g_fbo);

    if (g_fbo_color) glDeleteTextures(1, &g_fbo_color);
    if (g_fbo_depth_stencil) glDeleteTextures(1, &g_fbo_depth_stencil);
    g_fbo_color = 0;
    g_fbo_depth_stencil = 0;

    glGenTextures(1, &g_fbo_color);
    glBindTexture(GL_TEXTURE_2D, g_fbo_color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenTextures(1, &g_fbo_depth_stencil);
    glBindTexture(GL_TEXTURE_2D, g_fbo_depth_stencil);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, w, h, 0,
                 GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GL30.glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    GL30.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_2D, g_fbo_color, 0);
    GL30.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                GL_TEXTURE_2D, g_fbo_depth_stencil, 0);

    const GLenum status = GL30.glCheckFramebufferStatus(GL_FRAMEBUFFER);
    GL30.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "BGE: framebuffer incomplete: 0x%x\n", (unsigned)status);
        state.id = STATE_EXIT;
        return;
    }

    g_fbo_w = w;
    g_fbo_h = h;
}

static bool post_init(void)
{
    static const char* vs =
        "#version 330 core\n"
        "layout(location=0) in vec2 aPos;\n"
        "out vec2 vUV;\n"
        "void main(){\n"
        "    vUV = aPos * 0.5 + 0.5;\n"
        "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
        "}\n";
    static const char* fs =
        "#version 330 core\n"
        "in vec2 vUV;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D u_screen;\n"
        "uniform float u_levels;\n"
        "void main(){\n"
        "    vec3 c = texture(u_screen, vUV).rgb;\n"
        "    c = round(c * (u_levels - 1.0)) / (u_levels - 1.0);\n"
        "    FragColor = vec4(c, 1.0);\n"
        "}\n";

    g_post_program = create_program(vs, fs);
    if (!g_post_program) return false;

    static const f32 quad[] = { -1,-1, 1,-1, 1,1, -1,-1, 1,1, -1,1 };
    GL30.glGenVertexArrays(1, &g_post_vao);
    GL15.glGenBuffers(1, &g_post_vbo);
    GL30.glBindVertexArray(g_post_vao);
    GL15.glBindBuffer(GL_ARRAY_BUFFER, g_post_vbo);
    GL15.glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    GL20.glEnableVertexAttribArray(0);
    GL20.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    GL30.glBindVertexArray(0);
    return g_post_vao != 0 && g_post_vbo != 0;
}

void post_blit(const i32 src_w, const i32 src_h, const i32 dst_w, const i32 dst_h)
{
    (void)src_w;
    (void)src_h;

    GL30.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, dst_w, dst_h);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    GL20.glUseProgram(g_post_program);
    GL20.glUniform1i(GL20.glGetUniformLocation(g_post_program, "u_screen"), 0);
    GL20.glUniform1f(GL20.glGetUniformLocation(g_post_program, "u_levels"), PALETTE_LEVELS);

    GLModern.glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_fbo_color);

    GL30.glBindVertexArray(g_post_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    GL30.glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

static bool allocate_state(void)
{
    state.fb = (framebuffer_t*)calloc(1, sizeof(*state.fb));
    state.data = (data_t*)calloc(1, sizeof(*state.data));
    state.cam = (camera_t*)calloc(1, sizeof(*state.cam));
    state.text = (texture_registry_t*)calloc(1, sizeof(*state.text));
    state.gun = (gun_registry_t*)calloc(1, sizeof(*state.gun));
    state.editor = (editor_t*)calloc(1, sizeof(*state.editor));

    if (state.fb && state.data && state.cam && state.text && state.gun && state.editor)
        return true;

    fprintf(stderr, "BGE: failed to allocate engine state\n");
    return false;
}

static void free_state(void)
{
    free(state.text);
    free(state.gun);
    free(state.cam);
    free(state.data);
    free(state.fb);
    free(state.editor);
    state.text = NULL;
    state.gun = NULL;
    state.cam = NULL;
    state.data = NULL;
    state.fb = NULL;
    state.editor = NULL;
}

static bool app_start(void)
{
    memset(&state, 0, sizeof(state));
    state.id = STATE_PLAYING;

    DisplayMode mode = DisplayMode(WIDTH, HEIGHT);
    if (Display.setDisplayMode(&mode) != 0) {
        fprintf(stderr, "BGE: failed to set display mode: %s\n", lwcglGetLastError());
        return false;
    }

    Display.setTitle(TITLE);
    Display.setResizable(LWCGL_TRUE);
    Display.setVSyncEnabled(LWCGL_FALSE);
    lwcglSetContextVersion(3, 3);
    lwcglSetContextProfile(LWCGL_CONTEXT_CORE_PROFILE);

    if (Display.create() != 0) {
        fprintf(stderr, "BGE: Display.create failed: %s\n", lwcglGetLastError());
        return false;
    }
    if (!required_gl_available()) {
        fprintf(stderr, "BGE: required OpenGL 3.3 functions are unavailable\n");
        return false;
    }
    if (Keyboard.create() != 0) {
        fprintf(stderr, "BGE: Keyboard.create failed: %s\n", lwcglGetLastError());
        return false;
    }
    if (Mouse.create() != 0) {
        fprintf(stderr, "BGE: Mouse.create failed: %s\n", lwcglGetLastError());
        return false;
    }

    input_init();
    if (!allocate_state()) return false;
    update_framebuffer_metrics();

    if (!post_init()) {
        fprintf(stderr, "BGE: failed to initialize post process\n");
        return false;
    }
    if (!imgui_init()) {
        fprintf(stderr, "BGE: failed to initialize ImGui\n");
        return false;
    }
    imgui_set_mouse_enabled(false);

    state.data->program = create_program(VS, FS);
    if (!state.data->program) {
        fprintf(stderr, "BGE: failed to create scene shader\n");
        return false;
    }
    state.data->u_model = GL20.glGetUniformLocation(state.data->program, "model");
    state.data->u_view = GL20.glGetUniformLocation(state.data->program, "view");
    state.data->u_proj = GL20.glGetUniformLocation(state.data->program, "projection");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);

    g_last_time = app_now_seconds();
#if RENDERCHECK_AVAILABLE
    g_rendercheck_frame_index = 0;
#endif
    return true;
}

static void app_end(void)
{
    if (g_game_initialized) {
        game_shutdown();
        g_game_initialized = false;
    }

    if (state.text) texture_registry_cleanup(state.text);
    text_shutdown();
    imgui_shutdown();
    rendercheck_gpu_shutdown();

    if (state.data && state.data->program)
        GL20.glDeleteProgram(state.data->program);

    if (g_fbo_color) glDeleteTextures(1, &g_fbo_color);
    if (g_fbo_depth_stencil) glDeleteTextures(1, &g_fbo_depth_stencil);
    if (g_fbo) GL30.glDeleteFramebuffers(1, &g_fbo);
    if (g_post_program) GL20.glDeleteProgram(g_post_program);
    if (g_post_vao) GL30.glDeleteVertexArrays(1, &g_post_vao);
    if (g_post_vbo) GL15.glDeleteBuffers(1, &g_post_vbo);

    g_fbo = 0;
    g_fbo_color = 0;
    g_fbo_depth_stencil = 0;
    g_post_program = 0;
    g_post_vao = 0;
    g_post_vbo = 0;
    g_fbo_w = 0;
    g_fbo_h = 0;

    free_state();

    if (Mouse.isCreated()) Mouse.destroy();
    if (Keyboard.isCreated()) Keyboard.destroy();
    if (Display.isCreated()) Display.destroy();
}

static void update_fps(void)
{
    g_fps_accum += (double)state.dt;
    ++g_fps_frames;
    if (g_fps_accum >= 0.5) {
        g_fps_value = (double)g_fps_frames / g_fps_accum;
        g_fps_accum = 0.0;
        g_fps_frames = 0;
    }
}

static bool app_frame(void)
{
    rendercheck_gpu_begin();

    const double now = app_now_seconds();
    state.dt = rendercheck_enabled() ? (1.0f / 60.0f) : (f32)(now - g_last_time);
    if (state.dt < 0.0f) state.dt = 0.0f;
    g_last_time = now;

    Display.processMessages();
    update_framebuffer_metrics();
    input_begin_frame();
    game_handle_input();
    game_update();
    game_render();

#if RENDERCHECK_AVAILABLE
    rendercheck_capture_frame(g_rendercheck_frame_index);
#else
    rendercheck_capture_frame(0);
#endif
    rendercheck_gpu_end();
    Display.updateNoMessages();
    update_fps();

#if RENDERCHECK_AVAILABLE
    if (rendercheck_enabled()) {
        const bool keep_running = !rendercheck_frame_is_last(g_rendercheck_frame_index);
        ++g_rendercheck_frame_index;
        return keep_running;
    }
#endif

    return Display.isCloseRequested() == LWCGL_FALSE && state.id != STATE_EXIT;
}

int app_run(void)
{
    if (!app_start()) {
        app_end();
        return 1;
    }

    if (!game_init()) {
        fprintf(stderr, "BGE: game initialization failed\n");
        app_end();
        return 1;
    }
    g_game_initialized = true;

    while (app_frame()) {}
    app_end();
    return 0;
}

void app_request_exit(void)
{
    state.id = STATE_EXIT;
}

double app_get_fps(void)
{
    return g_fps_value;
}
