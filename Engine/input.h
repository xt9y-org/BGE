#ifndef BGE_INPUT_H
#define BGE_INPUT_H

#include "util/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void input_init(void);
void input_begin_frame(void);

bool input_key_down(i32 key);
bool input_key_pressed(i32 key);
bool input_key_released(i32 key);

bool input_mouse_down(i32 button);
bool input_mouse_pressed(i32 button);
bool input_mouse_released(i32 button);

f32 input_mouse_dx(void);
f32 input_mouse_dy(void);
f32 input_mouse_wheel(void);

void input_set_grabbed(bool grabbed);
bool input_is_grabbed(void);

#ifdef BGE_INPUT_TEST
void input_test_begin_frame(void);
void input_test_set_key(i32 key, bool down);
void input_test_set_mouse(i32 button, bool down);
void input_test_set_mouse_motion(f32 dx, f32 dy, f32 wheel);
void input_test_set_grabbed(bool grabbed);
#endif

#ifdef __cplusplus
}
#endif

#endif
