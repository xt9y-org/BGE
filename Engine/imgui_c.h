#ifndef IMGUI_C_H
#define IMGUI_C_H

#include "util/types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool imgui_init(void);
void imgui_newframe(f32 dt, i32 window_w, i32 window_h, i32 framebuffer_w, i32 framebuffer_h);
void imgui_render(void);
void imgui_shutdown(void);

bool imgui_want_capture_mouse(void);
bool imgui_want_capture_keyboard(void);
bool imgui_is_any_window_hovered(void);
void imgui_set_mouse_enabled(bool enabled);

#define IMGUI_COND_NONE            0
#define IMGUI_COND_ALWAYS          (1 << 0)
#define IMGUI_COND_ONCE            (1 << 1)
#define IMGUI_COND_FIRST_USE_EVER  (1 << 2)
#define IMGUI_COND_APPEARING       (1 << 3)

#define IMGUI_WINDOW_NONE                  0
#define IMGUI_WINDOW_NO_TITLE_BAR          (1 << 0)
#define IMGUI_WINDOW_NO_RESIZE             (1 << 1)
#define IMGUI_WINDOW_NO_MOVE               (1 << 2)
#define IMGUI_WINDOW_NO_SCROLLBAR          (1 << 3)
#define IMGUI_WINDOW_NO_COLLAPSE           (1 << 5)
#define IMGUI_WINDOW_ALWAYS_AUTO_RESIZE    (1 << 6)
#define IMGUI_WINDOW_NO_BACKGROUND         (1 << 7)
#define IMGUI_WINDOW_NO_SAVED_SETTINGS     (1 << 8)
#define IMGUI_WINDOW_NO_MOUSE_INPUTS       (1 << 9)
#define IMGUI_WINDOW_MENU_BAR              (1 << 10)
#define IMGUI_WINDOW_HORIZONTAL_SCROLLBAR  (1 << 11)
#define IMGUI_WINDOW_NO_FOCUS_ON_APPEARING (1 << 12)
#define IMGUI_WINDOW_NO_NAV                (1 << 16)

bool imgui_begin(const char* title, bool* open, i32 window_flags);
void imgui_end(void);
void imgui_set_next_window_pos(f32 x, f32 y, i32 cond);
void imgui_set_next_window_size(f32 w, f32 h, i32 cond);
void imgui_set_next_window_bg_alpha(f32 alpha);
void imgui_set_next_window_collapsed(bool collapsed, i32 cond);

void imgui_text(const char* fmt, ...);
void imgui_same_line(void);
void imgui_same_line_offset(f32 x);
void imgui_separator(void);
void imgui_spacing(void);
void imgui_new_line(void);
void imgui_indent(void);
void imgui_unindent(void);
void imgui_begin_disabled(bool disabled);
void imgui_end_disabled(void);
void imgui_push_item_width(f32 width);
void imgui_pop_item_width(void);
void imgui_align_text_to_frame_padding(void);

bool imgui_button(const char* label);
bool imgui_checkbox(const char* label, bool* v);
bool imgui_slider_float(const char* label, f32* v, f32 v_min, f32 v_max);
bool imgui_slider_int(const char* label, i32* v, i32 v_min, i32 v_max);
bool imgui_color_edit3(const char* label, f32 v[3]);
bool imgui_color_edit4(const char* label, f32 v[4]);
bool imgui_input_int(const char* label, i32* v);
bool imgui_input_float(const char* label, f32* v);
bool imgui_combo(const char* label, i32* current_idx, const char* const* items, i32 items_count);
bool imgui_selectable(const char* label, bool selected);
bool imgui_collapsing_header(const char* label);
bool imgui_radio(const char* label, i32* v, i32 v_button);

bool imgui_begin_child(const char* str_id, f32 width, f32 height, bool border);
void imgui_end_child(void);
void imgui_image(u32 texture_id, f32 width, f32 height);
bool imgui_image_button(const char* str_id, u32 texture_id, f32 width, f32 height);

bool imgui_is_item_clicked(i32 mouse_button);
bool imgui_is_item_hovered(void);
bool imgui_is_window_hovered(void);
bool imgui_is_any_item_hovered(void);
void imgui_set_tooltip(const char* fmt, ...);

void imgui_open_popup(const char* str_id);
bool imgui_begin_popup(const char* str_id);
void imgui_end_popup(void);
void imgui_close_current_popup(void);

void imgui_push_id_int(i32 id);
void imgui_pop_id(void);
f32 imgui_get_font_size(void);
f32 imgui_calc_text_width(const char* text);

#ifdef __cplusplus
}
#endif

#endif
