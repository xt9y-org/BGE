#include "App.h"
#include "level.h"
#include "state.h"
#include "render.h"
#include "imgui_c.h"
#include "input.h"
#include <lwcgl/glmodern.h>
#include <lwcgl/lwcgl.h>
#include "util/types.h"
#include "cam.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static u32 g_editor_vao = 0;
static u32 g_editor_vbo = 0;
static u32 g_editor_ebo = 0;
static bool g_editor_vao_initialized = false;

static void ensure_editor_vao(void)
{
    if (g_editor_vao_initialized) return;

    GL30.glGenVertexArrays(1, &g_editor_vao);
    GL15.glGenBuffers(1, &g_editor_vbo);
    GL15.glGenBuffers(1, &g_editor_ebo);

    GL30.glBindVertexArray(g_editor_vao);
    GL15.glBindBuffer(GL_ARRAY_BUFFER, g_editor_vbo);
    GL15.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_editor_ebo);

    GL20.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void*)0);
    GL20.glEnableVertexAttribArray(0);
    GL20.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void*)(3 * sizeof(f32)));
    GL20.glEnableVertexAttribArray(1);
    GL20.glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void*)(6 * sizeof(f32)));
    GL20.glEnableVertexAttribArray(2);

    GL30.glBindVertexArray(0);
    g_editor_vao_initialized = true;
}

void editor_add_sector(level_data_t* level) 
{
    i32 selected_sector_id = state.editor->selected_sector ? state.editor->selected_sector->id : -1;
    
    if (level->sector_capacity <= level->sector_count) {
        i32 new_cap = (level->sector_capacity == 0) ? level->sector_count + 16 : level->sector_capacity * 2;
        level_sector_data_t* new_sectors = malloc(sizeof(level_sector_data_t) * new_cap);
        if (level->sector_count > 0) memcpy(new_sectors, level->sectors, sizeof(level_sector_data_t) * level->sector_count);
        if (level->sector_capacity > 0) free(level->sectors);
        level->sectors = new_sectors;
        level->sector_capacity = new_cap;
        
        if (selected_sector_id != -1)
            for (i32 i = 0; i < level->sector_count; i++)
                if (level->sectors[i].id == selected_sector_id) { state.editor->selected_sector = &level->sectors[i]; break; }
    }
    
    level->sectors[level->sector_count].id = 0; 
    for (i32 i = 0; i < level->sector_count; i++)
        if (level->sectors[i].id >= level->sectors[level->sector_count].id) level->sectors[level->sector_count].id = level->sectors[i].id + 1;

    level->sectors[level->sector_count].light = (vec3s){1.0f, 1.0f, 1.0f};
    level->sectors[level->sector_count].quads = NULL;
    level->sectors[level->sector_count].quad_count = 0;
    level->sectors[level->sector_count].quad_capacity = 0;
    
    level->sector_count++;
}

void editor_add_quad(level_sector_data_t* sector, const level_quad_t* template) 
{
    if (sector->quad_capacity <= sector->quad_count) {
        i32 new_cap = (sector->quad_capacity == 0) ? sector->quad_count + 16 : sector->quad_capacity * 2;
        level_quad_t* new_quads = malloc(sizeof(level_quad_t) * new_cap);
        if (sector->quad_count > 0) memcpy(new_quads, sector->quads, sizeof(level_quad_t) * sector->quad_count);
        if (sector->quad_capacity > 0) free(sector->quads);
        sector->quads = new_quads;
        sector->quad_capacity = new_cap;
    }

    level_quad_t new_q = template ? *template : state.editor->template_quad; 
    new_q.sector_id = sector->id;

    sector->quads[sector->quad_count] = new_q;
    sector->quad_count++;
    state.editor->selected_quad = &sector->quads[sector->quad_count - 1];
    state.editor->selected_sector = sector;
    state.editor->selected_wall_idx = sector->quad_count - 1;
    editor_open_wall_window(sector->id, sector->quad_count - 1);
}

void editor_delete_quad(level_sector_data_t* sector, i32 idx) 
{
    if (!sector || idx < 0 || idx >= sector->quad_count) return;

    for (i32 i = 0; i < state.editor->ui_wall_window_count; i++) {
        editor_wall_window_t* w = &state.editor->ui_wall_windows[i];
        if (w->sector_id != sector->id) continue;
        if (w->quad_idx == idx) {
            *w = state.editor->ui_wall_windows[--state.editor->ui_wall_window_count];
            i--;
        } else if (w->quad_idx > idx) {
            w->quad_idx--;
        }
    }

    for (i32 i = idx; i < sector->quad_count - 1; i++) sector->quads[i] = sector->quads[i + 1];    
    sector->quad_count--;
}

int count_portal_quads(level_data_t* level, int portal_id) 
{
    if (portal_id <= 0) return 0;
    int count = 0;
    for (int s = 0; s < level->sector_count; s++)
        for (int i = 0; i < level->sectors[s].quad_count; i++)
            if (level->sectors[s].quads[i].portal_id == portal_id) count++;

    return count;
}

level_sector_data_t* get_sector_by_id(i32 id) 
{
    for (i32 i = 0; i < state.editor->level->sector_count; i++) if (state.editor->level->sectors[i].id == id) return &state.editor->level->sectors[i];
    return NULL;
}

void editor_move_quad_to_sector(level_sector_data_t* old_sector, level_sector_data_t* new_sector, i32 quad_idx) {
    if (!old_sector || !new_sector || quad_idx < 0 || quad_idx >= old_sector->quad_count) return;
    bool sel_was_moved = state.editor->selected_quad == &old_sector->quads[quad_idx];
    old_sector->quads[quad_idx].sector_id = new_sector->id;
    
    if (new_sector->quad_capacity <= new_sector->quad_count) {
        i32 new_cap = (new_sector->quad_capacity == 0) ? new_sector->quad_count + 16 : new_sector->quad_capacity * 2;
        level_quad_t* new_quads = malloc(sizeof(level_quad_t) * new_cap);
        if (new_sector->quad_count > 0) memcpy(new_quads, new_sector->quads, sizeof(level_quad_t) * new_sector->quad_count);
        if (new_sector->quad_capacity > 0) free(new_sector->quads);
        new_sector->quads = new_quads;
        new_sector->quad_capacity = new_cap;
    }
    
    new_sector->quads[new_sector->quad_count++] = old_sector->quads[quad_idx];
    
    for (i32 i = quad_idx; i < old_sector->quad_count - 1; i++) old_sector->quads[i] = old_sector->quads[i + 1];
    old_sector->quad_count--;

    for (i32 i = 0; i < state.editor->ui_wall_window_count; i++) {
        editor_wall_window_t* w = &state.editor->ui_wall_windows[i];
        if (w->sector_id != old_sector->id) continue;
        if (w->quad_idx == quad_idx) { w->sector_id = new_sector->id; w->quad_idx = new_sector->quad_count - 1; }
        else if (w->quad_idx > quad_idx) w->quad_idx--;
    }

    if (sel_was_moved) {
        state.editor->selected_quad = &new_sector->quads[new_sector->quad_count - 1];
        state.editor->selected_sector = new_sector;
        state.editor->selected_wall_idx = new_sector->quad_count - 1;
    }
}

void editor_sync_template_from_selection(void)
{
    if (state.editor->selected_quad) {
        state.editor->template_quad = *state.editor->selected_quad;
        state.editor->template_mods = EDITOR_MOD_ALL;
    }
}

static void editor_sync_template_from_quad(const level_quad_t* q)
{
    if (!q) return;
    state.editor->template_quad = *q;
    state.editor->template_mods = EDITOR_MOD_ALL;
}

static i32 sector_idx_by_id(level_data_t* level, i32 id)
{
    for (i32 i = 0; i < level->sector_count; i++)
        if (level->sectors[i].id == id) return i;
    return -1;
}

void editor_open_wall_window(i32 sector_id, i32 quad_idx)
{
    for (i32 i = 0; i < state.editor->ui_wall_window_count; i++)
        if (state.editor->ui_wall_windows[i].sector_id == sector_id &&
            state.editor->ui_wall_windows[i].quad_idx == quad_idx) return;
    if (state.editor->ui_wall_window_count >= MAX_WALL_WINDOWS) return;

    static u32 g_next_wall_window_id = 1;
    editor_wall_window_t* w = &state.editor->ui_wall_windows[state.editor->ui_wall_window_count++];
    w->id = g_next_wall_window_id++;
    w->sector_id = sector_id;
    w->quad_idx = quad_idx;
}

static level_quad_t* wall_window_quad(const editor_wall_window_t* w, level_sector_data_t** out_sector, i32* out_sector_idx)
{
    if (!w) return NULL;
    i32 si = sector_idx_by_id(state.editor->level, w->sector_id);
    if (si < 0 || w->quad_idx < 0 || w->quad_idx >= state.editor->level->sectors[si].quad_count) return NULL;
    if (out_sector) *out_sector = &state.editor->level->sectors[si];
    if (out_sector_idx) *out_sector_idx = si;
    return &state.editor->level->sectors[si].quads[w->quad_idx];
}

static const char* texture_basename(const char* path)
{
    if (!path) return "";
    const char* base = path;
    for (const char* p = path; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;
    return base;
}

static char g_new_level_names[MAX_LEVELS][32];
static char g_new_level_paths[MAX_LEVELS][32];

void editor_add_level(void)
{
    if (state.level_count >= MAX_LEVELS) return;
    i32 idx = state.level_count;

    snprintf(g_new_level_names[idx], 32, "Level %d", idx + 1);
    snprintf(g_new_level_paths[idx], 32, "level%d.h", idx + 1);

    level_data_t* lvl = &state.levels[idx];
    memset(lvl, 0, sizeof(*lvl));
    lvl->name = g_new_level_names[idx];
    lvl->path = g_new_level_paths[idx];

    state.editor->selected_sector = NULL;
    editor_add_sector(lvl);
    lvl->cam = *state.cam;

    state.level_count++;
    state.level_id = idx;
    state.editor->level = lvl;
    state.editor->selected_quad = NULL;
    state.editor->ui_sector_open = false;
    state.editor->portal_link_mode = false;
    state.editor->portal_link_source = NULL;
    state.editor->ui_wall_window_count = 0;
    state.editor->template_quad = get_default_quad(state.cam);
    state.editor->template_mods = EDITOR_MOD_NONE;

    editor_save(lvl);
}

void editor_switch_level(i32 idx)
{
    if (idx < 0 || idx >= state.level_count || idx == state.level_id) return;

    editor_save(state.editor->level);
    state.level_id = idx;
    apply_level_camera(state.cam, &state.levels[idx]);
    state.editor->level = &state.levels[idx];
    state.editor->selected_quad = NULL;
    state.editor->selected_sector = NULL;
    state.editor->ui_sector_open = false;
    state.editor->portal_link_mode = false;
    state.editor->portal_link_source = NULL;
    state.editor->ui_wall_window_count = 0;
    state.editor->template_quad = get_default_quad(state.cam);
    state.editor->template_mods = EDITOR_MOD_NONE;
}

static void editor_ui_state(void)
{
    imgui_set_next_window_pos(0.0f, 0.0f, IMGUI_COND_ALWAYS);
    imgui_set_next_window_bg_alpha(0.85f);

    if (imgui_begin("STATE", NULL, IMGUI_WINDOW_ALWAYS_AUTO_RESIZE | IMGUI_WINDOW_NO_MOVE))
    {
        imgui_text("FPS: %.1f", app_get_fps());
        imgui_text("POS: %.2f  %.2f  %.2f", state.cam->pos.x, state.cam->pos.y, state.cam->pos.z);
        imgui_text("YAW: %.1f   PITCH: %.1f", state.cam->yaw, state.cam->pitch);
        imgui_text("RES: %d x %d   ASPECT: %.3f",
            state.fb->ww, state.fb->wh,
            state.fb->wh > 0 ? (f32)state.fb->ww / (f32)state.fb->wh : 0.0f);
        imgui_text("CURRENT LEVEL ID: %d", state.level_id);

        if (state.id == STATE_EDITOR) {
            const char* editor_modes[] = { "IDLE", "DRAG", "RESIZE_TOP", "RESIZE_RIGHT", "PAINT" };
            imgui_text("EDITOR MODE: %s", editor_modes[state.editor->id]);
        }

        imgui_separator();

        const char* level_names[MAX_LEVELS];
        for (i32 i = 0; i < state.level_count; i++) level_names[i] = state.levels[i].name ? state.levels[i].name : "?";
        i32 cur = state.level_id;
        if (imgui_combo("Level", &cur, level_names, state.level_count))
            if (cur != state.level_id) editor_switch_level(cur);

        if (imgui_button("Create New Level")) editor_add_level();

        imgui_separator();

        if (imgui_collapsing_header("Hotkey Atlas"))
        {
            imgui_text("VIEWPORT");
            imgui_text("  Click + Drag        Select / move quad");
            imgui_text("  Ctrl + Click + Drag Resize quad");
            imgui_text("  V                   Toggle paint mode");
            imgui_text("  N                   New quad (dup if selected)");
            imgui_text("  R                   Reset selected quad");
            imgui_text("  X / Del / Backslash Delete selected quad");
            imgui_text("  Enter               Deselect selected quad");
            imgui_text("  7 / 8 / 9           Rotate quad X / Y / Z");
            imgui_text("  Shift + 7/8/9       Reverse rotate");
            imgui_separator();
            imgui_text("GLOBAL");
            imgui_text("  E                   Toggle editor / play mode");
            imgui_text("  TAB                 Toggle cursor lock");
            imgui_text("  WASD                Move camera");
            imgui_text("  ESC                 Exit");
        }
    }
    imgui_end();
}

static bool editor_ui_texture_picker(level_quad_t* q)
{
    bool changed = false;
    imgui_text("Choose Texture");
    const char* cur = (q->tex_id >= 0 && q->tex_id < state.text->count) ? state.text->textures[q->tex_id].name : "none";
    char label[96];
    snprintf(label, sizeof(label), "%s  [open atlas]", texture_basename(cur));
    if (imgui_button(label)) imgui_open_popup("texture_atlas");

    if (imgui_begin_popup("texture_atlas"))
    {
        imgui_text("Texture Atlas");
        imgui_separator();
        if (imgui_selectable("No Texture (fallback)", q->tex_id < 0)) { q->tex_id = -1; changed = true; imgui_close_current_popup(); }

        if (imgui_begin_child("texgrid", 0.0f, 260.0f, true))
        {
            const i32 per_row = 4;
            i32 col = 0;
            for (i32 i = 0; i < state.text->count; i++)
            {
                texture_t* t = &state.text->textures[i];
                if (col > 0) imgui_same_line();

                f32 thumb_w = 56.0f, thumb_h = 56.0f;
                if (t->width > 0 && t->height > 0 && t->width != t->height) {
                    f32 aspect = (f32)t->width / (f32)t->height;
                    if (aspect > 1.0f) thumb_h = thumb_w / aspect;
                    else thumb_w = thumb_h * aspect;
                }

                imgui_push_id_int(i);
                bool clicked = imgui_image_button("##tex", t->id, thumb_w, thumb_h);
                imgui_pop_id();
                if (clicked) { q->tex_id = i; changed = true; imgui_close_current_popup(); }
                if (imgui_is_item_hovered()) imgui_set_tooltip("%s", texture_basename(t->name));
                col++;
                if (col >= per_row) col = 0;
            }
        }
        imgui_end_child();
        imgui_end_popup();
    }
    return changed;
}

static bool editor_ui_wall(editor_wall_window_t* w)
{
    level_sector_data_t* s = NULL;
    i32 si = -1;
    level_quad_t* q = wall_window_quad(w, &s, &si);
    if (!q) return false;

    imgui_set_next_window_bg_alpha(0.9f);
    f32 gx = (f32)((w->id - 1) % 3) * 390.0f;
    f32 gy = (f32)((w->id - 1) / 3) * 400.0f;
    imgui_set_next_window_pos(380.0f + gx, 10.0f + gy, IMGUI_COND_FIRST_USE_EVER);
    imgui_set_next_window_size(360.0f, 0.0f, IMGUI_COND_FIRST_USE_EVER);

    char title[32];
    snprintf(title, sizeof(title), "Wall##%u", w->id);

    bool open = true;
    if (imgui_begin(title, &open, 0))
    {
        if (imgui_button("Open Respected Sector Window"))
        {
            state.editor->ui_sector_open = true;
            state.editor->ui_sector_idx = si;
        }
        imgui_separator();

        imgui_text("Sector ID: %d", q->sector_id);
        imgui_text("Wall ID: %d", w->quad_idx);
        imgui_text("Pos: %.0f  %.0f  %.0f", q->pos.x, q->pos.y, q->pos.z);
        imgui_text("Rot: %.0f  %.0f  %.0f", q->rot.x, q->rot.y, q->rot.z);
        imgui_text("Size: %.0f x %.0f", q->size.x, q->size.y);
        imgui_text("Portal ID: %d", q->portal_id);
        imgui_text("Portal Side Flip: %s", q->portal_side_flip ? "ON" : "OFF");
        imgui_separator();

        {   // Choose Sector
            level_data_t* level = state.editor->level;
            const char* s_names[MAX_SECTORS_PER_LEVEL];
            static char s_buf[MAX_SECTORS_PER_LEVEL][16];
            i32 s_order[MAX_SECTORS_PER_LEVEL];
            i32 scount = level->sector_count < MAX_SECTORS_PER_LEVEL ? level->sector_count : MAX_SECTORS_PER_LEVEL;
            for (i32 i = 0; i < scount; i++) {
                s_order[i] = i;
                snprintf(s_buf[i], sizeof(s_buf[i]), "Sector %d", level->sectors[i].id);
                s_names[i] = s_buf[i];
            }
            i32 cur = 0;
            if (s) for (i32 i = 0; i < scount; i++) if (s_order[i] == si) { cur = i; break; }
            if (imgui_combo("Choose Sector", &cur, s_names, scount))
            {
                level_sector_data_t* target = &level->sectors[s_order[cur]];
                if (target != s)
                {
                    editor_move_quad_to_sector(s, target, w->quad_idx);
                    state.editor->ui_sector_idx = s_order[cur];
                    q = wall_window_quad(w, &s, &si);
                    if (!q) return false;
                }
            }
        }

        {   // Choose Portal
            bool link = state.editor->portal_link_mode;
            if (imgui_button(link ? "Cancel portal select" : "SELECT portal target"))
            {
                state.editor->portal_link_mode = !state.editor->portal_link_mode;
                state.editor->portal_link_source = state.editor->portal_link_mode ? q : NULL;
            }
            if (link) imgui_text("Click another wall in the viewport to link portal.");
            if (q->portal_id > 0 && imgui_button("Clear Portal")) q->portal_id = 0;
            if (imgui_checkbox("Flip Portal Side", &q->portal_side_flip))
                editor_sync_template_from_quad(q);
        }

        imgui_separator();

        if (imgui_color_edit3("Color", (f32*)&q->color))
            editor_sync_template_from_quad(q);

        if (editor_ui_texture_picker(q))
            editor_sync_template_from_quad(q);

        imgui_separator();

        if (imgui_checkbox("Billboard", &q->is_billboard)) editor_sync_template_from_quad(q);
        if (imgui_checkbox("Solid", &q->is_solid))       editor_sync_template_from_quad(q);
        if (imgui_checkbox("Invisible", &q->is_invisible)) editor_sync_template_from_quad(q);
    }
    imgui_end();
    return open;
}

static void editor_ui_wall_windows(void)
{
    i32 i = 0;
    while (i < state.editor->ui_wall_window_count)
    {
        bool keep = editor_ui_wall(&state.editor->ui_wall_windows[i]);
        if (keep) i++;
        else {
            state.editor->ui_wall_windows[i] = state.editor->ui_wall_windows[state.editor->ui_wall_window_count - 1];
            state.editor->ui_wall_window_count--;
        }
    }
}

static void editor_ui_sector(void)
{
    if (!state.editor->ui_sector_open) return;

    level_data_t* level = state.editor->level;
    i32 s_idx = state.editor->ui_sector_idx;
    if (state.editor->selected_sector)
        s_idx = (i32)(state.editor->selected_sector - level->sectors);
    if (s_idx < 0 || s_idx >= level->sector_count) s_idx = 0;
    level_sector_data_t* s = &level->sectors[s_idx];
    state.editor->ui_sector_idx = s_idx;

    imgui_set_next_window_pos(380.0f, 360.0f, IMGUI_COND_FIRST_USE_EVER);
    imgui_set_next_window_size(280.0f, 0.0f, IMGUI_COND_FIRST_USE_EVER);

    bool open = state.editor->ui_sector_open;
    if (imgui_begin("Sector", &open, 0))
    {
        imgui_text("Sector ID: %d", s->id);
        imgui_text("Wall Count: %d", s->quad_count);
        imgui_separator();
        imgui_text("Walls (click to open):");
        for (i32 i = 0; i < s->quad_count; i++)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "Wall %d##w%d", i, i);
            bool is_cur = state.editor->selected_quad == &s->quads[i];
            if (imgui_selectable(buf, is_cur))
            {
                state.editor->selected_quad = &s->quads[i];
                state.editor->selected_sector = s;
                state.editor->selected_wall_idx = i;
                editor_sync_template_from_selection();
                editor_open_wall_window(s->id, i);
            }
        }
        imgui_separator();
        imgui_color_edit3("Sector Light", (f32*)&s->light);
    }
    imgui_end();
    state.editor->ui_sector_open = open;
}

void editor_ui(void)
{
    if (state.id != STATE_EDITOR) return;
    editor_ui_state();
    editor_ui_wall_windows();
    editor_ui_sector();
}

static vec3s intersect_ray_plane(vec3s ray_origin, vec3s ray_dir, vec3s plane_pos, vec3s plane_normal) 
{
    f32 denom = vec3_dot(plane_normal, ray_dir);
    if (fabsf(denom) < 0.0001f) return plane_pos;
    f32 t = vec3_dot(vec3_sub(plane_pos, ray_origin), plane_normal) / denom;
    return vec3_add(ray_origin, vec3_scale(ray_dir, t));
}

editor_look_at_info_t editor_get_look_at_info_with_ray(level_data_t* level, vec3s ray_origin, vec3s ray_dir, f32 max_dist) 
{
    editor_look_at_info_t best = {0};
    best.distance = max_dist;
    
    for (i32 s = 0; s < level->sector_count; s++) {
        level_sector_data_t* sector = &level->sectors[s];
        for (i32 i = 0; i < sector->quad_count; i++) {
            level_quad_t* quad = &sector->quads[i];
            f32 t; vec3s hit, local_hit;
            if (level_ray_intersects_quad(ray_origin, ray_dir, quad, &t, &hit, &local_hit)) {
                if (t < best.distance) {
                    best.hit = true;
                    best.distance = t;
                    best.wall_id = i;
                    best.sector_id = sector->id;
                    best.hit_position = hit;
                    best.local_hit = local_hit;
                    best.quad = quad;
                    best.sector = sector;
                }
            }
        }
    }
    return best;
}

static void editor_pick_ray(vec3s* out_origin, vec3s* out_dir)
{
    const f64 mx_fb = (f64)Mouse.getX();
    const f64 my_fb = (f64)Mouse.getY();
    f64 mx = state.fb->w > 0 ? mx_fb * (f64)state.fb->ww / (f64)state.fb->w : 0.0;
    const f64 my_bottom = state.fb->h > 0 ? my_fb * (f64)state.fb->wh / (f64)state.fb->h : 0.0;
    f64 my = (f64)state.fb->wh - 1.0 - my_bottom;
    if (state.cursor_locked) {
        mx = (f64)state.fb->ww * 0.5;
        my = (f64)state.fb->wh * 0.5;
    }

    const f32 fov = DEG2RAD(45.0f);
    const f32 aspect = state.fb->wh > 0 ? (f32)state.fb->ww / (f32)state.fb->wh : 1.0f;

    vec3s right = vec3_normalize(vec3_cross(state.cam->front, state.cam->up));
    vec3s up = vec3_normalize(vec3_cross(right, state.cam->front));

    const f32 ndc_x = (f32)((2.0 * mx / (f64)state.fb->ww) - 1.0);
    const f32 ndc_y = (f32)(1.0 - (2.0 * my / (f64)state.fb->wh));

    const f32 tan_half = tanf(fov * 0.5f);
    vec3s dir = vec3_add(state.cam->front,
                 vec3_add(vec3_scale(right, ndc_x * tan_half * aspect),
                          vec3_scale(up, ndc_y * tan_half)));

    *out_origin = state.cam->pos;
    *out_dir = vec3_normalize(dir);
}

void editor_update()
{
    static bool template_init = false;
    if (!template_init) {
        state.editor->template_quad = get_default_quad(state.cam);
        state.editor->template_mods = EDITOR_MOD_NONE;
        template_init = true;
    }

    if (!state.editor->selected_quad) {
        state.editor->template_quad.pos = (vec3s){
            roundf(state.cam->pos.x + state.cam->front.x * 3.0f),
            roundf(state.cam->pos.y + state.cam->front.y * 3.0f),
            roundf(state.cam->pos.z + state.cam->front.z * 3.0f)
        };
    }

    if (state.editor->portal_link_source) {
        bool valid = false;
        for (i32 si = 0; si < state.editor->level->sector_count && !valid; si++) {
            level_sector_data_t* sec = &state.editor->level->sectors[si];
            for (i32 qi = 0; qi < sec->quad_count; qi++)
                if (&sec->quads[qi] == state.editor->portal_link_source) { valid = true; break; }
        }
        if (!valid) state.editor->portal_link_source = NULL;
    }

    vec3s ray_origin, ray_dir;
    editor_pick_ray(&ray_origin, &ray_dir);
    editor_look_at_info_t info = editor_get_look_at_info_with_ray(state.editor->level, ray_origin, ray_dir, 100.0f);

    const bool mouse_is_pressed = input_mouse_down(0);
    const bool ctrl_held = input_key_down(Keyboard.KEY_LCONTROL) || input_key_down(Keyboard.KEY_RCONTROL);
    bool ui_capture = !state.cursor_locked && imgui_want_capture_mouse();

    if (input_mouse_pressed(0) && !ui_capture)
    {
        if (state.editor->portal_link_mode)
        {
            level_quad_t* src = state.editor->portal_link_source ? state.editor->portal_link_source : state.editor->selected_quad;
            if (info.hit && info.quad != src)
            {
                i32 pid = 1;
                while (count_portal_quads(state.editor->level, pid) > 0) pid++;
                if (src) src->portal_id = pid;
                info.quad->portal_id = pid;
                editor_sync_template_from_selection();
            }
            state.editor->portal_link_mode = false;
            state.editor->portal_link_source = NULL;
        }
        else if (state.editor->id == EDITOR_PAINT) 
        {
            if (info.hit) {
                if (state.editor->template_mods & EDITOR_MOD_COLOR) info.quad->color = state.editor->template_quad.color;
                if (state.editor->template_mods & EDITOR_MOD_ROTATION) info.quad->rot = state.editor->template_quad.rot;
                if (state.editor->template_mods & EDITOR_MOD_TEXTURE) info.quad->tex_id = state.editor->template_quad.tex_id;
                if (state.editor->template_mods & EDITOR_MOD_SECTOR) {
                    if (info.quad->sector_id != state.editor->template_quad.sector_id) {
                        level_sector_data_t* target_sector = get_sector_by_id(state.editor->template_quad.sector_id);
                        if (target_sector) editor_move_quad_to_sector(info.sector, target_sector, info.wall_id);
                    }
                } state.editor->selected_quad = NULL;
            } else {
                state.editor->selected_quad = NULL;
                state.editor->template_quad = get_default_quad(state.cam);
                state.editor->template_mods = EDITOR_MOD_NONE;
                state.editor->id = EDITOR_IDLE; 
            }
        } 
        else if (info.hit) 
        {
            state.editor->selected_quad = info.quad;
            state.editor->selected_sector = info.sector;
            state.editor->selected_wall_idx = info.wall_id;
            state.editor->template_quad = *info.quad;
            state.editor->template_mods = EDITOR_MOD_ALL;
            editor_open_wall_window(info.sector->id, info.wall_id);
            
            state.editor->drag_start_hit = info.hit_position;
            state.editor->drag_plane_normal = vec3_scale(state.cam->front, -1.0f);
            state.editor->drag_cam_start_pos = state.cam->pos;
            state.editor->drag_quad_start_pos = info.quad->pos;
            state.editor->drag_quad_start_rot = info.quad->rot;
            state.editor->drag_quad_start_size = info.quad->size;

            if (ctrl_held) {
                f32 dy_t = info.quad->size.y - info.local_hit.y;
                f32 dx_r = info.quad->size.x - info.local_hit.x;
                f32 tol = 5.0f;
                if (dy_t < dx_r && dy_t < tol) state.editor->id = EDITOR_RESIZE_TOP;
                else if (dx_r < tol) state.editor->id = EDITOR_RESIZE_RIGHT;
                else state.editor->id = EDITOR_DRAG;
            } else state.editor->id = EDITOR_DRAG;
        } else 
        {
            state.editor->selected_quad = NULL;
            state.editor->template_quad = get_default_quad(state.cam);
            state.editor->template_mods = EDITOR_MOD_NONE;
            state.editor->id = EDITOR_IDLE;
        }
    }

    if (!mouse_is_pressed) {
        if (state.editor->id != EDITOR_PAINT) state.editor->id = EDITOR_IDLE;
        state.editor->hover_id = EDITOR_IDLE;

        if (state.editor->selected_quad && ctrl_held) {
            if (info.hit && info.quad == state.editor->selected_quad) {
                f32 dy_t = info.quad->size.y - info.local_hit.y, dx_r = info.quad->size.x - info.local_hit.x, tol = 5.0f;
                if (dy_t < dx_r && dy_t < tol) state.editor->hover_id = EDITOR_RESIZE_TOP;
                else if (dx_r < tol) state.editor->hover_id = EDITOR_RESIZE_RIGHT;
            }
        }
    }

    if (state.editor->id != EDITOR_IDLE && state.editor->selected_quad) 
    {
        vec3s plane_pos = vec3_add(state.editor->drag_start_hit, vec3_sub(state.cam->pos, state.editor->drag_cam_start_pos));
        vec3s current_hit = intersect_ray_plane(ray_origin, ray_dir, plane_pos, state.editor->drag_plane_normal);
        
        if (state.editor->id == EDITOR_DRAG) 
        {
            vec3s new_pos = vec3_add(state.editor->drag_quad_start_pos, vec3_sub(current_hit, state.editor->drag_start_hit));
            state.editor->selected_quad->pos = (vec3s){roundf(new_pos.x), roundf(new_pos.y), roundf(new_pos.z)};
        }
        else 
        {
            vec3s right = vec3_normalize(vec3_cross(state.cam->front, (vec3s){0, 1, 0}));
            vec3s up_vec = vec3_normalize(vec3_cross(right, state.cam->front));
            vec3s movement = vec3_add(vec3_scale(right, vec3_dot(vec3_sub(current_hit, plane_pos), right)), vec3_scale(up_vec, vec3_dot(vec3_sub(current_hit, plane_pos), up_vec)));

            f32 r_y[16], r_x[16], r_z[16], m[16], t[16];
            mat4_rotate_y(r_y, -DEG2RAD(state.editor->drag_quad_start_rot.y));
            mat4_rotate_x(r_x, -DEG2RAD(state.editor->drag_quad_start_rot.x));
            mat4_rotate_z(r_z, -DEG2RAD(state.editor->drag_quad_start_rot.z));
            mat4_multiply(t, r_y, r_x);
            mat4_multiply(m, t, r_z);

            vec3s local_right = { m[0], m[1], m[2] }, local_up = { m[4], m[5], m[6] };

            if (state.editor->id == EDITOR_RESIZE_TOP) state.editor->selected_quad->size.y = roundf(fmaxf(1.0f, state.editor->drag_quad_start_size.y + vec3_dot(movement, local_up)));
            if (state.editor->id == EDITOR_RESIZE_RIGHT) state.editor->selected_quad->size.x = roundf(fmaxf(1.0f, state.editor->drag_quad_start_size.x + vec3_dot(movement, local_right)));
            
            state.editor->selected_quad->pos = state.editor->drag_quad_start_pos;
        }
    }

}

static void render_border_segments(const level_quad_t* quad, const vec4s color, bool top, bool bottom, bool left, bool right)
{
    if (!quad) return;
    ensure_editor_vao();

    f32 rot_y[16], rot_x[16], rot_z[16];
    mat4_rotate_y(rot_y, -DEG2RAD(quad->rot.y));
    mat4_rotate_x(rot_x, -DEG2RAD(quad->rot.x));
    mat4_rotate_z(rot_z, -DEG2RAD(quad->rot.z));

    f32 temp[16], model[16];
    mat4_multiply(temp, rot_y, rot_x);
    mat4_multiply(model, temp, rot_z);

    model[12] = quad->pos.x;
    model[13] = quad->pos.y;
    model[14] = quad->pos.z;

    GL20.glUniformMatrix4fv(state.data->u_model, 1, GL_FALSE, model);
    texture_bind(texture_get_fallback(), 0);

    const f32 t = 0.08f;
    const f32 z = 0.01f;

    f32 vertices[16 * 8];
    u32 indices[4 * 6];
    u32 vcount = 0, icount = 0;

#define PUSH_QUAD(x0, y0, x1, y1) \
    do { \
        const u32 base = vcount / 8; \
        vertices[vcount++] = (x1); vertices[vcount++] = (y1); vertices[vcount++] = z; vertices[vcount++] = color.x; vertices[vcount++] = color.y; vertices[vcount++] = color.z; vertices[vcount++] = 0; vertices[vcount++] = 0; \
        vertices[vcount++] = (x1); vertices[vcount++] = (y0); vertices[vcount++] = z; vertices[vcount++] = color.x; vertices[vcount++] = color.y; vertices[vcount++] = color.z; vertices[vcount++] = 0; vertices[vcount++] = 0; \
        vertices[vcount++] = (x0); vertices[vcount++] = (y0); vertices[vcount++] = z; vertices[vcount++] = color.x; vertices[vcount++] = color.y; vertices[vcount++] = color.z; vertices[vcount++] = 0; vertices[vcount++] = 0; \
        vertices[vcount++] = (x0); vertices[vcount++] = (y1); vertices[vcount++] = z; vertices[vcount++] = color.x; vertices[vcount++] = color.y; vertices[vcount++] = color.z; vertices[vcount++] = 0; vertices[vcount++] = 0; \
        indices[icount++] = base + 0; indices[icount++] = base + 1; indices[icount++] = base + 3; \
        indices[icount++] = base + 1; indices[icount++] = base + 2; indices[icount++] = base + 3; \
    } while (0)

    if (top) PUSH_QUAD(0.0f, quad->size.y - t, quad->size.x, quad->size.y);
    if (bottom) PUSH_QUAD(0.0f, 0.0f, quad->size.x, t);
    if (left) PUSH_QUAD(0.0f, 0.0f, t, quad->size.y);
    if (right) PUSH_QUAD(quad->size.x - t, 0.0f, quad->size.x, quad->size.y);

#undef PUSH_QUAD

    GL30.glBindVertexArray(g_editor_vao);
    GL15.glBindBuffer(GL_ARRAY_BUFFER, g_editor_vbo);
    GL15.glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * vcount, vertices, GL_DYNAMIC_DRAW);
    GL15.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_editor_ebo);
    GL15.glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * icount, indices, GL_DYNAMIC_DRAW);
    glDrawElements(GL_TRIANGLES, (GLsizei)icount, GL_UNSIGNED_INT, 0);
    GL30.glBindVertexArray(0);
}

static void editor_render_sector(const level_sector_data_t *sector)
{
    for (i32 i = 0; i < sector->quad_count; i++) {
        if (state.editor->selected_quad == &sector->quads[i]) {
            editor_e border_id = state.editor->id != EDITOR_IDLE ? state.editor->id : state.editor->hover_id;
            bool top_yellow = true, bottom_yellow = true, left_yellow = true, right_yellow = true;

            if (border_id == EDITOR_RESIZE_TOP) top_yellow = false;
            if (border_id == EDITOR_RESIZE_RIGHT) right_yellow = false;

            const vec4s select_color = { 1.0f, 1.0f, 0.0f, 1.0f };
            render_border_segments(&sector->quads[i], select_color, top_yellow, bottom_yellow, left_yellow, right_yellow);

            const vec4s resize_color = { 1.0f, 0.0f, 1.0f, 1.0f };
            if (border_id == EDITOR_RESIZE_TOP) render_border_segments(&sector->quads[i], resize_color, true, false, false, false);
            if (border_id == EDITOR_RESIZE_RIGHT) render_border_segments(&sector->quads[i], resize_color, false, false, false, true);
        }
    }
}

void editor_render_borders()
{
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    for (i32 i = 0; i < state.editor->level->sector_count; i++) editor_render_sector(&state.editor->level->sectors[i]);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
}

void editor_render()
{
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    for (i32 i = 0; i < state.editor->level->sector_count; i++) editor_render_sector(&state.editor->level->sectors[i]);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
}

void editor_save(level_data_t* level)
{
    level->cam.pos = state.cam->pos;
    level->cam.yaw = state.cam->yaw;
    level->cam.pitch = state.cam->pitch;

    char full_path[256];
    sprintf(full_path, "Engine/res/%s", level->path);
    FILE* f = fopen(full_path, "w");
    if (!f) return;

    int level_num = 0;
    sscanf(level->name, "Level %d", &level_num);
    if (level_num == 0) sscanf(level->path, "level%d.h", &level_num);

    fprintf(f, "#ifndef LEVEL%d_H\n", level_num);
    fprintf(f, "#define LEVEL%d_H\n\n", level_num);
    fprintf(f, "#include \"level.h\"\n\n");

    for (int s = 0; s < level->sector_count; s++) {
        level_sector_data_t* sector = &level->sectors[s];
        if (sector->quad_count > 0) {
            fprintf(f, "static level_quad_t level%d_sector%d_quads[] = {\n", level_num, s);
            for (int q = 0; q < sector->quad_count; q++) {
                level_quad_t* quad = &sector->quads[q];
                fprintf(f, "    { .pos = {%.0f, %.0f, %.0f}, .rot = {%.0f, %.0f, %.0f}, .size = {%.0f, %.0f}, .tex_id = %d, .is_solid = %s, .is_invisible = %s, .is_billboard = %s, .portal_side_flip = %s, .color = {%.1ff, %.1ff, %.1ff}, .portal_id = %d, .sector_id = %d },\n",
                    roundf(quad->pos.x), roundf(quad->pos.y), roundf(quad->pos.z),
                    roundf(quad->rot.x), roundf(quad->rot.y), roundf(quad->rot.z),
                    roundf(quad->size.x), roundf(quad->size.y),
                    quad->tex_id,
                    quad->is_solid ? "true" : "false",
                    quad->is_invisible ? "true" : "false",
                    quad->is_billboard ? "true" : "false",
                    quad->portal_side_flip ? "true" : "false",
                    quad->color.x, quad->color.y, quad->color.z,
                    quad->portal_id,
                    quad->sector_id);
            }
            fprintf(f, "};\n\n");
        }
    }

    fprintf(f, "static level_sector_data_t level%d_sectors[] = {\n", level_num);
    for (int s = 0; s < level->sector_count; s++) {
        level_sector_data_t* sector = &level->sectors[s];
        if (sector->quad_count > 0) {
            fprintf(f, "    { .id = %d, .light = {%.1ff, %.1ff, %.1ff}, .quads = level%d_sector%d_quads, .quad_count = sizeof(level%d_sector%d_quads) / sizeof(level%d_sector%d_quads[0]) },\n", 
                sector->id, sector->light.x, sector->light.y, sector->light.z, level_num, s, level_num, s, level_num, s);
        } else {
            fprintf(f, "    { .id = %d, .light = {%.1ff, %.1ff, %.1ff}, .quads = NULL, .quad_count = 0 },\n",
                sector->id, sector->light.x, sector->light.y, sector->light.z);
        }
    }
    fprintf(f, "};\n\n");

    fprintf(f, "static inline level_data_t load_%d(void)\n{\n", level_num);
    fprintf(f, "    return (level_data_t){\n");
    fprintf(f, "        .name = \"%s\",\n", level->name);
    fprintf(f, "        .path = \"%s\",\n", level->path);
    fprintf(f, "        .sectors = level%d_sectors,\n", level_num);
    fprintf(f, "        .sector_count = sizeof(level%d_sectors) / sizeof(level%d_sectors[0]),\n", level_num, level_num);
    fprintf(f, "        .cam = { .pos = {%.3ff, %.3ff, %.3ff}, .yaw = %.3ff, .pitch = %.3ff }\n", 
        level->cam.pos.x, level->cam.pos.y, level->cam.pos.z,
        level->cam.yaw, level->cam.pitch);
    fprintf(f, "    };\n}\n\n");

    fprintf(f, "#endif\n");
    fclose(f);
}
