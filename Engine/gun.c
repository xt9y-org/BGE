#include "gun.h"
#include "input.h"
#include "state.h"

#include <lwcgl/lwcgl.h>
#include <math.h>
#include <string.h>

static u32 g_vao = 0, g_vbo = 0;
static i32 g_current = 0;
static f32 g_swing = 0.0f, g_swing_vel = 0.0f;
static f32 g_flash_timer = 0.0f;
static f32 g_anim_time = 0.0f;
static f32 g_bob_time = 0.0f;

void gun_reg_init(gun_registry_t* reg)
{
    memset(reg, 0, sizeof(*reg));
    reg->count = 0;
}

void gun_init(void)
{
    g_swing = 0.0f;
    g_swing_vel = 0.0f;
    g_flash_timer = 0.0f;
    g_anim_time = 0.0f;
    g_bob_time = 0.0f;

    vertex_t verts[6] = {
        {{0,0,0}, {0,0}, {1,1,1,1}},
        {{1,1,0}, {1,1}, {1,1,1,1}},
        {{1,0,0}, {1,0}, {1,1,1,1}},
        {{0,0,0}, {0,0}, {1,1,1,1}},
        {{0,1,0}, {0,1}, {1,1,1,1}},
        {{1,1,0}, {1,1}, {1,1,1,1}},
    };

    GL30.glGenVertexArrays(1, &g_vao);
    GL15.glGenBuffers(1, &g_vbo);
    GL30.glBindVertexArray(g_vao);
    GL15.glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    GL15.glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    GL20.glEnableVertexAttribArray(0);
    GL20.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)0);
    GL20.glEnableVertexAttribArray(1);
    GL20.glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)(sizeof(f32) * 3));
    GL20.glEnableVertexAttribArray(2);
    GL20.glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)(sizeof(f32) * 5));
    GL30.glBindVertexArray(0);
}

void gun_shutdown(void)
{
    if (g_vbo) GL15.glDeleteBuffers(1, &g_vbo);
    if (g_vao) GL30.glDeleteVertexArrays(1, &g_vao);
    g_vbo = 0;
    g_vao = 0;
}

static i32 gun_current_frame(const weapon_def_t* w)
{
    if (w->frame_duration <= 0.0f || w->tex_count <= 1) return 0;
    i32 frame = (i32)(g_anim_time / w->frame_duration);
    if (frame >= w->tex_count) {
        g_anim_time = 0.0f;
        return 0;
    }
    return frame;
}

void gun_render(i32 rw, i32 rh)
{
    if (state.id != STATE_PLAYING || !g_vao) return;
    if (g_current < 0 || g_current >= state.gun->count) return;

    const weapon_def_t* w = &state.gun->defs[g_current];
    const i32 frame = gun_current_frame(w);
    const texture_t* tex = texture_get_by_name(w->tex_names[frame]);
    if (!tex || tex->width <= 0 || tex->height <= 0) return;

    if (g_anim_time > 0.0f) g_anim_time += state.dt;

    if (input_key_pressed(Keyboard.KEY_A)) g_swing_vel = -120.0f;
    if (input_key_pressed(Keyboard.KEY_D)) g_swing_vel = 120.0f;
    g_swing_vel += (-150.0f * g_swing - 12.0f * g_swing_vel) * state.dt;
    g_swing += g_swing_vel * state.dt;

    g_bob_time += state.dt;
    const f32 t = g_bob_time;
    const f32 aspect = (f32)tex->width / (f32)tex->height;
    const f32 gh = (f32)rh * w->gun_size;
    const f32 gw = gh * aspect;
    const f32 gx = (f32)rw * w->gun_xy.x - gw * 0.5f + sinf(t * 1.8f) * 3.0f + g_swing;
    const f32 gy = (f32)rh * w->gun_xy.y - gh + cosf(t * 2.2f) * 2.5f;

    f32 proj[16];
    mat4_ortho(proj, 0.0f, (f32)rw, (f32)rh, 0.0f, -1.0f, 1.0f);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const u32 program = text_get_program();
    GL20.glUseProgram(program);
    GL20.glUniformMatrix4fv(GL20.glGetUniformLocation(program, "u_proj"), 1, GL_FALSE, proj);
    GL20.glUniform1i(GL20.glGetUniformLocation(program, "u_font"), 0);

    g_flash_timer -= state.dt;
    if (g_flash_timer > 0 && w->flash_tex_name) {
        const texture_t* flash = texture_get_by_name(w->flash_tex_name);
        if (flash && flash->width > 0) {
            const f32 fs = w->flash_size * (f32)rh;
            const f32 fx = gx + gw * w->flash_xy.x - fs * 0.5f;
            const f32 fy = gy + gh * w->flash_xy.y - fs * 0.5f;
            vertex_t fverts[6] = {
                {{fx, fy, 0}, {0, 0}, {1,1,1,1}},
                {{fx+fs, fy+fs, 0}, {1, 1}, {1,1,1,1}},
                {{fx+fs, fy, 0}, {1, 0}, {1,1,1,1}},
                {{fx, fy, 0}, {0, 0}, {1,1,1,1}},
                {{fx, fy+fs, 0}, {0, 1}, {1,1,1,1}},
                {{fx+fs, fy+fs, 0}, {1, 1}, {1,1,1,1}},
            };
            texture_bind((texture_t*)flash, 0);
            GL30.glBindVertexArray(g_vao);
            GL15.glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
            GL15.glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(fverts), fverts);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

    texture_bind((texture_t*)tex, 0);
    vertex_t verts[6] = {
        {{gx, gy, 0}, {0, 0}, {1,1,1,1}},
        {{gx+gw, gy+gh, 0}, {1, 1}, {1,1,1,1}},
        {{gx+gw, gy, 0}, {1, 0}, {1,1,1,1}},
        {{gx, gy, 0}, {0, 0}, {1,1,1,1}},
        {{gx, gy+gh, 0}, {0, 1}, {1,1,1,1}},
        {{gx+gw, gy+gh, 0}, {1, 1}, {1,1,1,1}},
    };

    GL30.glBindVertexArray(g_vao);
    GL15.glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    GL15.glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    GL30.glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void gun_shot(void)
{
    if (state.id != STATE_PLAYING) return;
    g_flash_timer = 0.1f;
    g_anim_time = 0.001f;
}

i32 gun_get_current(void)
{
    return g_current;
}

void gun_select(i32 idx)
{
    if (idx >= 0 && idx < state.gun->count) g_current = idx;
}

void gun_next(void)
{
    if (state.gun->count > 0)
        g_current = (g_current + 1) % state.gun->count;
}

void gun_prev(void)
{
    if (state.gun->count > 0)
        g_current = (g_current - 1 + state.gun->count) % state.gun->count;
}
