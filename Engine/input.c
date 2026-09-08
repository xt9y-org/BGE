#include "input.h"

#include <string.h>

#ifndef BGE_INPUT_TEST
#include <lwcgl/lwcgl.h>
#endif

#define INPUT_MAX_KEYS 512
#define INPUT_MAX_MOUSE_BUTTONS 32

static bool g_keys[INPUT_MAX_KEYS];
static bool g_prev_keys[INPUT_MAX_KEYS];
static bool g_mouse[INPUT_MAX_MOUSE_BUTTONS];
static bool g_prev_mouse[INPUT_MAX_MOUSE_BUTTONS];
static i32 g_key_count = INPUT_MAX_KEYS;
static i32 g_mouse_count = INPUT_MAX_MOUSE_BUTTONS;
static f32 g_mouse_dx;
static f32 g_mouse_dy;
static f32 g_mouse_wheel;
static bool g_grabbed;

static bool valid_key(i32 key)
{
    return key >= 0 && key < g_key_count && key < INPUT_MAX_KEYS;
}

static bool valid_mouse(i32 button)
{
    return button >= 0 && button < g_mouse_count && button < INPUT_MAX_MOUSE_BUTTONS;
}

static void begin_snapshot(void)
{
    memcpy(g_prev_keys, g_keys, sizeof(g_keys));
    memcpy(g_prev_mouse, g_mouse, sizeof(g_mouse));
    memset(g_keys, 0, sizeof(g_keys));
    memset(g_mouse, 0, sizeof(g_mouse));
    g_mouse_dx = 0.0f;
    g_mouse_dy = 0.0f;
    g_mouse_wheel = 0.0f;
}

void input_init(void)
{
    memset(g_keys, 0, sizeof(g_keys));
    memset(g_prev_keys, 0, sizeof(g_prev_keys));
    memset(g_mouse, 0, sizeof(g_mouse));
    memset(g_prev_mouse, 0, sizeof(g_prev_mouse));
    g_mouse_dx = 0.0f;
    g_mouse_dy = 0.0f;
    g_mouse_wheel = 0.0f;
    g_grabbed = false;

#ifndef BGE_INPUT_TEST
    const i32 key_count = Keyboard.getKeyCount();
    const i32 mouse_count = Mouse.getButtonCount();
    g_key_count = key_count > 0 && key_count < INPUT_MAX_KEYS ? key_count : INPUT_MAX_KEYS;
    g_mouse_count = mouse_count > 0 && mouse_count < INPUT_MAX_MOUSE_BUTTONS ? mouse_count : INPUT_MAX_MOUSE_BUTTONS;
#else
    g_key_count = INPUT_MAX_KEYS;
    g_mouse_count = INPUT_MAX_MOUSE_BUTTONS;
#endif
}

void input_begin_frame(void)
{
    begin_snapshot();

#ifndef BGE_INPUT_TEST
    for (i32 key = 0; key < g_key_count; ++key)
        g_keys[key] = Keyboard.isKeyDown(key) != LWCGL_FALSE;

    for (i32 button = 0; button < g_mouse_count; ++button)
        g_mouse[button] = Mouse.isButtonDown(button) != LWCGL_FALSE;

    g_mouse_dx = (f32)Mouse.getDX();
    g_mouse_dy = (f32)Mouse.getDY();
    g_mouse_wheel = (f32)Mouse.getDWheel() / 120.0f;
    g_grabbed = Mouse.isGrabbed() != LWCGL_FALSE;
#endif
}

bool input_key_down(i32 key)
{
    return valid_key(key) && g_keys[key];
}

bool input_key_pressed(i32 key)
{
    return valid_key(key) && g_keys[key] && !g_prev_keys[key];
}

bool input_key_released(i32 key)
{
    return valid_key(key) && !g_keys[key] && g_prev_keys[key];
}

bool input_mouse_down(i32 button)
{
    return valid_mouse(button) && g_mouse[button];
}

bool input_mouse_pressed(i32 button)
{
    return valid_mouse(button) && g_mouse[button] && !g_prev_mouse[button];
}

bool input_mouse_released(i32 button)
{
    return valid_mouse(button) && !g_mouse[button] && g_prev_mouse[button];
}

f32 input_mouse_dx(void)
{
    return g_mouse_dx;
}

f32 input_mouse_dy(void)
{
    return g_mouse_dy;
}

f32 input_mouse_wheel(void)
{
    return g_mouse_wheel;
}

void input_set_grabbed(bool grabbed)
{
    g_grabbed = grabbed;
#ifndef BGE_INPUT_TEST
    Mouse.setGrabbed(grabbed ? LWCGL_TRUE : LWCGL_FALSE);
#endif
}

bool input_is_grabbed(void)
{
    return g_grabbed;
}

#ifdef BGE_INPUT_TEST
void input_test_begin_frame(void)
{
    begin_snapshot();
}

void input_test_set_key(i32 key, bool down)
{
    if (valid_key(key)) g_keys[key] = down;
}

void input_test_set_mouse(i32 button, bool down)
{
    if (valid_mouse(button)) g_mouse[button] = down;
}

void input_test_set_mouse_motion(f32 dx, f32 dy, f32 wheel)
{
    g_mouse_dx = dx;
    g_mouse_dy = dy;
    g_mouse_wheel = wheel;
}

void input_test_set_grabbed(bool grabbed)
{
    g_grabbed = grabbed;
}
#endif
