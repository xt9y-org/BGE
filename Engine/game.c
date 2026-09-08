#include "game.h"

#include "App.h"
#include "editor.h"
#include "gun.h"
#include "imgui_c.h"
#include "input.h"
#include "level.h"
#include "portal.h"
#include "render.h"
#include "state.h"
#include "text.h"
#include "util/math.h"

#include "res/level1.h"
#include "res/level2.h"
#include "res/level3.h"
#include "res/level4.h"
#include "res/level5.h"
#include "res/level6.h"
#include "res/level7.h"
#include "res/level8.h"

#include <lwcgl/lwcgl.h>
#include <lwcgl/glmodern.h>

#include <math.h>

static void register_saved_level(level_data_t (*loader)(void))
{
    const level_data_t level = loader();
    if (level.sector_count > 0 && state.level_count < MAX_LEVELS)
        state.levels[state.level_count++] = level;
}

bool game_init(void)
{
    texture_registry_init(state.text);
    state.text->textures[state.text->count++] = *texture_create("Engine/res/ground.png", TEX_FILTER_LINEAR, TEX_WRAP_REPEAT);
    state.text->textures[state.text->count++] = *texture_create("Engine/res/stone.png", TEX_FILTER_LINEAR, TEX_WRAP_REPEAT);
    state.text->textures[state.text->count++] = *texture_create("Engine/res/awesomeface.png", TEX_FILTER_LINEAR, TEX_WRAP_REPEAT);
    state.text->textures[state.text->count++] = *texture_create_solid(255, 255, 255);
    state.text->textures[state.text->count++] = *texture_create("Engine/res/metal_a.png", TEX_FILTER_LINEAR, TEX_WRAP_REPEAT);
    state.text->textures[state.text->count++] = *texture_create("Engine/res/metal_b.png", TEX_FILTER_LINEAR, TEX_WRAP_REPEAT);
    state.text->textures[state.text->count++] = *texture_create("Engine/res/grate.png", TEX_FILTER_LINEAR, TEX_WRAP_REPEAT);
    state.text->textures[state.text->count++] = *texture_create("Engine/res/spider.png", TEX_FILTER_LINEAR, TEX_WRAP_REPEAT);
    state.text->textures[state.text->count++] = *texture_create("Engine/res/banana.png", TEX_FILTER_LINEAR, TEX_WRAP_REPEAT);
    state.text->textures[state.text->count++] = *texture_create("Engine/res/water.png", TEX_FILTER_LINEAR, TEX_WRAP_REPEAT);
    state.text->textures[state.text->count++] = *texture_create("Engine/res/gun_doom.png", TEX_FILTER_LINEAR, TEX_WRAP_REPEAT);
    state.text->textures[state.text->count++] = *texture_create("Engine/res/gun_portal.png", TEX_FILTER_LINEAR, TEX_WRAP_REPEAT);
    state.text->textures[state.text->count++] = *texture_create("Engine/res/hand_shoot_flash.png", TEX_FILTER_LINEAR, TEX_WRAP_REPEAT);
    state.text->textures[state.text->count++] = *texture_create("Engine/res/hand_shoot_3.png", TEX_FILTER_LINEAR, TEX_WRAP_REPEAT);
    state.text->textures[state.text->count++] = *texture_create("Engine/res/hand_shoot_4.png", TEX_FILTER_LINEAR, TEX_WRAP_REPEAT);
    state.text->textures[state.text->count++] = *texture_create("Engine/res/hand_shoot_5.png", TEX_FILTER_LINEAR, TEX_WRAP_REPEAT);
    text_init();
    render_init();

    gun_reg_init(state.gun);
    state.gun->defs[state.gun->count++] = (weapon_def_t){
        .tex_names = { "Engine/res/gun_doom.png" },
        .tex_count = 1,
        .flash_tex_name = "Engine/res/hand_shoot_flash.png",
        .gun_xy = {0.5f, 1.0f},
        .gun_size = 0.43f,
        .flash_size = 0.12f,
        .flash_xy = {0.53f, 0.44f},
        .frame_duration = 0.0f,
    };
    state.gun->defs[state.gun->count++] = (weapon_def_t){
        .tex_names = { "Engine/res/gun_portal.png" },
        .tex_count = 1,
        .flash_tex_name = "Engine/res/hand_shoot_flash.png",
        .gun_xy = {0.78f, 1.0f},
        .gun_size = 0.33f,
        .flash_size = 0.22f,
        .flash_xy = {0.32f, 0.39f},
        .frame_duration = 0.0f,
    };
    state.gun->defs[state.gun->count++] = (weapon_def_t){
        .tex_names = { "Engine/res/hand_shoot_3.png", "Engine/res/hand_shoot_4.png", "Engine/res/hand_shoot_5.png" },
        .tex_count = 3,
        .flash_tex_name = "Engine/res/hand_shoot_flash.png",
        .gun_xy = {0.67f, 1.0f},
        .gun_size = 0.52f,
        .flash_size = 0.07f,
        .flash_xy = {0.32f, 0.26f},
        .frame_duration = 0.04f,
    };
    gun_init();

    state.level_count = 0;
    state.level_id = 0;
    state.levels[state.level_count++] = load_2();
    state.levels[state.level_count++] = load_3();
    state.levels[state.level_count++] = load_1();
    register_saved_level(load_4);
    register_saved_level(load_5);
    register_saved_level(load_6);
    register_saved_level(load_7);
    register_saved_level(load_8);

    state.editor->level = &state.levels[state.level_id];

    state.cam->front = (vec3s){0.0f, 0.0f, -1.0f};
    state.cam->up = (vec3s){0.0f, 1.0f, 0.0f};
    state.cursor_locked = false;
    input_set_grabbed(false);
    imgui_set_mouse_enabled(true);
    apply_level_camera(state.cam, &state.levels[state.level_id]);
    return true;
}

void game_handle_input(void)
{
    const bool shift_held = input_key_down(Keyboard.KEY_LSHIFT) || input_key_down(Keyboard.KEY_RSHIFT);
    const bool no_ui = !imgui_want_capture_keyboard();
    bool grab_changed = false;

    if (input_key_down(Keyboard.KEY_ESCAPE))
        state.id = STATE_EXIT;

    if (input_key_pressed(Keyboard.KEY_H))
        state.debug_visible = !state.debug_visible;

    if (input_key_pressed(Keyboard.KEY_TAB)) {
        state.cursor_locked = !state.cursor_locked;
        input_set_grabbed(state.cursor_locked);
        imgui_set_mouse_enabled(!state.cursor_locked);
        grab_changed = true;
    }

    if (state.cursor_locked && !grab_changed)
        camera_apply_mouse_delta(state.cam, input_mouse_dx(), input_mouse_dy());

    if (state.id == STATE_PLAYING && state.cursor_locked && input_mouse_pressed(0)) {
        shoot_bullet();
        gun_shot();
    }

    if (state.id == STATE_PLAYING && input_key_pressed(Keyboard.KEY_G))
        gun_next();

    if (input_key_pressed(Keyboard.KEY_E))
        state.id = state.id == STATE_EDITOR ? STATE_PLAYING : STATE_EDITOR;

    if (no_ui && input_key_pressed(Keyboard.KEY_N)) {
        i32 sector_index = state.editor->template_quad.sector_id;
        if (sector_index < 0 || sector_index >= state.editor->level->sector_count)
            sector_index = 0;
        editor_add_quad(&state.editor->level->sectors[sector_index], NULL);
    }

    if (no_ui && (input_key_pressed(Keyboard.KEY_X) ||
                  input_key_pressed(Keyboard.KEY_DELETE) ||
                  input_key_pressed(Keyboard.KEY_BACKSLASH))) {
        if (state.editor->selected_quad) {
            editor_delete_quad(state.editor->selected_sector, state.editor->selected_wall_idx);
            state.editor->selected_quad = NULL;
        }
    }

    if (no_ui && input_key_pressed(Keyboard.KEY_R) && state.editor->selected_quad) {
        *state.editor->selected_quad = get_default_quad(state.cam);
        state.editor->selected_quad->sector_id = state.editor->selected_sector ? state.editor->selected_sector->id : 0;
        state.editor->template_quad = *state.editor->selected_quad;
        state.editor->template_mods = EDITOR_MOD_ALL;
    }

    {
        static bool adjustment_held[3] = { false, false, false };
        static f32 adjustment_timer[3] = { 0.0f, 0.0f, 0.0f };
        const i32 keys[3] = { Keyboard.KEY_7, Keyboard.KEY_8, Keyboard.KEY_9 };

        for (i32 i = 0; i < 3; ++i) {
            bool triggered = false;
            const bool down = no_ui && input_key_down(keys[i]);

            if (down) {
                if (!adjustment_held[i]) {
                    triggered = true;
                    adjustment_held[i] = true;
                    adjustment_timer[i] = 0.3f;
                } else {
                    adjustment_timer[i] -= state.dt;
                    if (adjustment_timer[i] <= 0.0f) {
                        triggered = true;
                        adjustment_timer[i] = 0.05f;
                    }
                }
            } else {
                adjustment_held[i] = false;
                adjustment_timer[i] = 0.0f;
            }

            if (triggered) {
                level_quad_t* quad = state.editor->selected_quad ? state.editor->selected_quad : &state.editor->template_quad;
                f32* value = i == 0 ? &quad->rot.x : (i == 1 ? &quad->rot.y : &quad->rot.z);
                *value += shift_held ? -1.0f : 1.0f;
                if (*value >= 360.0f) *value = 0.0f;
                if (*value < 0.0f) *value = 359.0f;
                *value = roundf(*value);

                if (state.editor->selected_quad) {
                    state.editor->template_quad = *state.editor->selected_quad;
                    state.editor->template_mods = EDITOR_MOD_ALL;
                } else {
                    state.editor->template_mods |= EDITOR_MOD_ROTATION;
                }
            }
        }
    }

    if (no_ui && input_key_pressed(Keyboard.KEY_V))
        state.editor->id = state.editor->id == EDITOR_PAINT ? EDITOR_IDLE : EDITOR_PAINT;

    if (no_ui && input_key_pressed(Keyboard.KEY_RETURN)) {
        state.editor->selected_quad = NULL;
        state.editor->template_quad = get_default_quad(state.cam);
        state.editor->template_mods = EDITOR_MOD_NONE;
        if (state.editor->id == EDITOR_PAINT)
            state.editor->id = EDITOR_IDLE;
    }

    {
        const f32 speed = 18.5f * state.dt;
        const vec3s right = vec3_normalize(vec3_cross(state.cam->front, state.cam->up));

        if (state.id == STATE_PLAYING) {
            const vec3s previous_position = state.cam->pos;
            vec3s move = {0.0f, 0.0f, 0.0f};
            vec3s forward = {state.cam->front.x, 0.0f, state.cam->front.z};
            if (vec3_magnitude(forward) > 0.0001f)
                forward = vec3_normalize(forward);

            if (input_key_down(Keyboard.KEY_W)) move = vec3_add(move, forward);
            if (input_key_down(Keyboard.KEY_S)) move = vec3_sub(move, forward);
            if (input_key_down(Keyboard.KEY_A)) move = vec3_sub(move, right);
            if (input_key_down(Keyboard.KEY_D)) move = vec3_add(move, right);

            if (vec3_magnitude(move) > 0.0001f) {
                move = vec3_normalize(move);
                state.cam->pos = vec3_add(state.cam->pos, vec3_scale(move, speed));
                portal_try_teleport(state.editor->level, previous_position, state.cam);
                player_collide_quads(state.editor->level, state.cam);
            }

            f32 height;
            if (level_get_height(state.editor->level, state.cam->pos, &height))
                state.cam->pos.y = lerp(state.cam->pos.y, height + 4.5f, 0.04f);
            else
                state.cam->pos = vec3_lerp(state.cam->pos, previous_position, 0.04f);
        }

        if (state.id == STATE_EDITOR) {
            if (input_key_down(Keyboard.KEY_W)) state.cam->pos = vec3_add(state.cam->pos, vec3_scale(state.cam->front, speed));
            if (input_key_down(Keyboard.KEY_S)) state.cam->pos = vec3_sub(state.cam->pos, vec3_scale(state.cam->front, speed));
            if (input_key_down(Keyboard.KEY_A)) state.cam->pos = vec3_sub(state.cam->pos, vec3_scale(right, speed));
            if (input_key_down(Keyboard.KEY_D)) state.cam->pos = vec3_add(state.cam->pos, vec3_scale(right, speed));
        }
    }
}

void game_update(void)
{
    state.editor->level = &state.levels[state.level_id];
    if (state.id == STATE_EDITOR)
        editor_update();
}

void game_render(void)
{
    const i32 fbw = state.fb->w;
    const i32 fbh = state.fb->h;
    if (fbw <= 0 || fbh <= 0) return;

    const f32 aspect = (f32)fbw / (f32)fbh;
    const i32 render_w = RENDER_BASE_W;
    const i32 render_h = (i32)((f32)RENDER_BASE_W / aspect);

    render_main(render_w, render_h);
    post_blit(render_w, render_h, fbw, fbh);
    GL30.glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, fbw, fbh);
    glDisable(GL_DEPTH_TEST);

    imgui_newframe();

    text_begin();
    text_draw((vec2s){(f32)state.fb->ww * 0.5f - 5.0f, (f32)state.fb->wh * 0.5f - 10.0f}, "+");
    text_flush(state.fb->ww, state.fb->wh);

    if (state.id == STATE_EDITOR)
        editor_ui();

    imgui_render();
    glEnable(GL_DEPTH_TEST);
}

void game_shutdown(void)
{
    gun_shutdown();
    render_shutdown();
    if (state.editor && state.editor->level)
        editor_save(state.editor->level);
}
