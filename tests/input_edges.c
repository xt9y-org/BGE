#define BGE_INPUT_TEST 1
#include "Engine/input.h"

#include <assert.h>
#include <math.h>

static void test_key_edges(void)
{
    const i32 key = 42;

    input_init();
    input_test_begin_frame();
    input_test_set_key(key, true);
    assert(input_key_down(key));
    assert(input_key_pressed(key));
    assert(!input_key_released(key));

    input_test_begin_frame();
    input_test_set_key(key, true);
    assert(input_key_down(key));
    assert(!input_key_pressed(key));
    assert(!input_key_released(key));

    input_test_begin_frame();
    input_test_set_key(key, false);
    assert(!input_key_down(key));
    assert(!input_key_pressed(key));
    assert(input_key_released(key));
}

static void test_mouse_edges_and_motion(void)
{
    const i32 button = 0;

    input_init();
    input_test_begin_frame();
    input_test_set_mouse(button, true);
    input_test_set_mouse_motion(7.0f, -3.0f, 1.0f);

    assert(input_mouse_down(button));
    assert(input_mouse_pressed(button));
    assert(fabsf(input_mouse_dx() - 7.0f) < 0.0001f);
    assert(fabsf(input_mouse_dy() + 3.0f) < 0.0001f);
    assert(fabsf(input_mouse_wheel() - 1.0f) < 0.0001f);

    input_test_begin_frame();
    input_test_set_mouse(button, false);
    assert(!input_mouse_down(button));
    assert(!input_mouse_pressed(button));
    assert(input_mouse_released(button));
    assert(fabsf(input_mouse_dx()) < 0.0001f);
    assert(fabsf(input_mouse_dy()) < 0.0001f);
    assert(fabsf(input_mouse_wheel()) < 0.0001f);
}

static void test_grab_state(void)
{
    input_init();
    assert(!input_is_grabbed());
    input_test_set_grabbed(true);
    assert(input_is_grabbed());
    input_test_set_grabbed(false);
    assert(!input_is_grabbed());
}

int main(void)
{
    test_key_edges();
    test_mouse_edges_and_motion();
    test_grab_state();
    return 0;
}
