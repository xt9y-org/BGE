#ifndef STATE_H
#define STATE_H

#include "cam.h"
#include "text.h"
#include "util/types.h"

#include "editor.h"
#include "level.h"
#include "gun.h"

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_EDITOR,
    STATE_EXIT
} state_e;

typedef struct {
    u32 vao, vbo, ebo, program;
    i32 u_model, u_view, u_proj;
} data_t;

typedef struct {
    i32 w, h, ww, wh;
    f32 scale;
} framebuffer_t;

typedef struct {
    bool cursor_locked;
    state_e id;
    f32 dt;
    framebuffer_t* fb;
    data_t* data;
    camera_t* cam;

    editor_t* editor;

    texture_registry_t* text;
    gun_registry_t* gun;

    vertex_t text_vertices[MAX_TEXT_VERTICES];
    u32 text_vertex_count;

    i16 level_id;
    i16 level_count;
    level_data_t levels[MAX_LEVELS];

    vertex_t wall_vertices[MAX_WALL_VERTICES];
    u32 wall_vertex_count;

    bool debug_visible;
} state_t;

extern state_t state;

#endif
