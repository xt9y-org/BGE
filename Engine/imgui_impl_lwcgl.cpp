#include "imgui_impl_lwcgl.h"

#include "imgui.h"
#include "input.h"

#include <lwcgl/lwcgl.h>

static bool g_initialized = false;

static void add_key(ImGuiIO& io, ImGuiKey imgui_key, int lwcgl_key)
{
    io.AddKeyEvent(imgui_key, input_key_down(lwcgl_key));
}

bool ImGui_ImplLwcgl_Init(void)
{
    if (g_initialized) return true;
    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = "imgui_impl_lwcgl";
    g_initialized = true;
    return true;
}

void ImGui_ImplLwcgl_Shutdown(void)
{
    if (!g_initialized) return;
    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = nullptr;
    g_initialized = false;
}

void ImGui_ImplLwcgl_NewFrame(float dt,
                              int window_w,
                              int window_h,
                              int framebuffer_w,
                              int framebuffer_h)
{
    if (!g_initialized) return;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)window_w, (float)window_h);

    const float framebuffer_scale_x =
        window_w > 0 ? (float)framebuffer_w / (float)window_w : 1.0f;
    const float framebuffer_scale_y =
        window_h > 0 ? (float)framebuffer_h / (float)window_h : 1.0f;
    io.DisplayFramebufferScale = ImVec2(
        framebuffer_scale_x < 1.0f ? 1.0f : framebuffer_scale_x,
        framebuffer_scale_y < 1.0f ? 1.0f : framebuffer_scale_y);
    io.DeltaTime = dt > 0.0f ? dt : (1.0f / 60.0f);

    const float mx = framebuffer_w > 0
        ? (float)Mouse.getX() * (float)window_w / (float)framebuffer_w
        : 0.0f;
    const float my_bottom = framebuffer_h > 0
        ? (float)Mouse.getY() * (float)window_h / (float)framebuffer_h
        : 0.0f;
    const float my = (float)window_h - 1.0f - my_bottom;
    io.AddMousePosEvent(mx, my);

    for (int button = 0; button < 5; ++button)
        io.AddMouseButtonEvent(button, input_mouse_down(button));
    io.AddMouseWheelEvent(0.0f, input_mouse_wheel());

    add_key(io, ImGuiKey_Tab, Keyboard.KEY_TAB);
    add_key(io, ImGuiKey_LeftArrow, Keyboard.KEY_LEFT);
    add_key(io, ImGuiKey_RightArrow, Keyboard.KEY_RIGHT);
    add_key(io, ImGuiKey_UpArrow, Keyboard.KEY_UP);
    add_key(io, ImGuiKey_DownArrow, Keyboard.KEY_DOWN);
    add_key(io, ImGuiKey_PageUp, Keyboard.KEY_PRIOR);
    add_key(io, ImGuiKey_PageDown, Keyboard.KEY_NEXT);
    add_key(io, ImGuiKey_Home, Keyboard.KEY_HOME);
    add_key(io, ImGuiKey_End, Keyboard.KEY_END);
    add_key(io, ImGuiKey_Insert, Keyboard.KEY_INSERT);
    add_key(io, ImGuiKey_Delete, Keyboard.KEY_DELETE);
    add_key(io, ImGuiKey_Backspace, Keyboard.KEY_BACK);
    add_key(io, ImGuiKey_Space, Keyboard.KEY_SPACE);
    add_key(io, ImGuiKey_Enter, Keyboard.KEY_RETURN);
    add_key(io, ImGuiKey_Escape, Keyboard.KEY_ESCAPE);
    add_key(io, ImGuiKey_Apostrophe, Keyboard.KEY_APOSTROPHE);
    add_key(io, ImGuiKey_Comma, Keyboard.KEY_COMMA);
    add_key(io, ImGuiKey_Minus, Keyboard.KEY_MINUS);
    add_key(io, ImGuiKey_Period, Keyboard.KEY_PERIOD);
    add_key(io, ImGuiKey_Slash, Keyboard.KEY_SLASH);
    add_key(io, ImGuiKey_Semicolon, Keyboard.KEY_SEMICOLON);
    add_key(io, ImGuiKey_Equal, Keyboard.KEY_EQUALS);
    add_key(io, ImGuiKey_LeftBracket, Keyboard.KEY_LBRACKET);
    add_key(io, ImGuiKey_Backslash, Keyboard.KEY_BACKSLASH);
    add_key(io, ImGuiKey_RightBracket, Keyboard.KEY_RBRACKET);
    add_key(io, ImGuiKey_GraveAccent, Keyboard.KEY_GRAVE);
    add_key(io, ImGuiKey_CapsLock, Keyboard.KEY_CAPITAL);
    add_key(io, ImGuiKey_ScrollLock, Keyboard.KEY_SCROLL);
    add_key(io, ImGuiKey_NumLock, Keyboard.KEY_NUMLOCK);
    add_key(io, ImGuiKey_PrintScreen, Keyboard.KEY_SYSRQ);
    add_key(io, ImGuiKey_Pause, Keyboard.KEY_PAUSE);

    add_key(io, ImGuiKey_0, Keyboard.KEY_0);
    add_key(io, ImGuiKey_1, Keyboard.KEY_1);
    add_key(io, ImGuiKey_2, Keyboard.KEY_2);
    add_key(io, ImGuiKey_3, Keyboard.KEY_3);
    add_key(io, ImGuiKey_4, Keyboard.KEY_4);
    add_key(io, ImGuiKey_5, Keyboard.KEY_5);
    add_key(io, ImGuiKey_6, Keyboard.KEY_6);
    add_key(io, ImGuiKey_7, Keyboard.KEY_7);
    add_key(io, ImGuiKey_8, Keyboard.KEY_8);
    add_key(io, ImGuiKey_9, Keyboard.KEY_9);

    add_key(io, ImGuiKey_A, Keyboard.KEY_A);
    add_key(io, ImGuiKey_B, Keyboard.KEY_B);
    add_key(io, ImGuiKey_C, Keyboard.KEY_C);
    add_key(io, ImGuiKey_D, Keyboard.KEY_D);
    add_key(io, ImGuiKey_E, Keyboard.KEY_E);
    add_key(io, ImGuiKey_F, Keyboard.KEY_F);
    add_key(io, ImGuiKey_G, Keyboard.KEY_G);
    add_key(io, ImGuiKey_H, Keyboard.KEY_H);
    add_key(io, ImGuiKey_I, Keyboard.KEY_I);
    add_key(io, ImGuiKey_J, Keyboard.KEY_J);
    add_key(io, ImGuiKey_K, Keyboard.KEY_K);
    add_key(io, ImGuiKey_L, Keyboard.KEY_L);
    add_key(io, ImGuiKey_M, Keyboard.KEY_M);
    add_key(io, ImGuiKey_N, Keyboard.KEY_N);
    add_key(io, ImGuiKey_O, Keyboard.KEY_O);
    add_key(io, ImGuiKey_P, Keyboard.KEY_P);
    add_key(io, ImGuiKey_Q, Keyboard.KEY_Q);
    add_key(io, ImGuiKey_R, Keyboard.KEY_R);
    add_key(io, ImGuiKey_S, Keyboard.KEY_S);
    add_key(io, ImGuiKey_T, Keyboard.KEY_T);
    add_key(io, ImGuiKey_U, Keyboard.KEY_U);
    add_key(io, ImGuiKey_V, Keyboard.KEY_V);
    add_key(io, ImGuiKey_W, Keyboard.KEY_W);
    add_key(io, ImGuiKey_X, Keyboard.KEY_X);
    add_key(io, ImGuiKey_Y, Keyboard.KEY_Y);
    add_key(io, ImGuiKey_Z, Keyboard.KEY_Z);

    add_key(io, ImGuiKey_F1, Keyboard.KEY_F1);
    add_key(io, ImGuiKey_F2, Keyboard.KEY_F2);
    add_key(io, ImGuiKey_F3, Keyboard.KEY_F3);
    add_key(io, ImGuiKey_F4, Keyboard.KEY_F4);
    add_key(io, ImGuiKey_F5, Keyboard.KEY_F5);
    add_key(io, ImGuiKey_F6, Keyboard.KEY_F6);
    add_key(io, ImGuiKey_F7, Keyboard.KEY_F7);
    add_key(io, ImGuiKey_F8, Keyboard.KEY_F8);
    add_key(io, ImGuiKey_F9, Keyboard.KEY_F9);
    add_key(io, ImGuiKey_F10, Keyboard.KEY_F10);
    add_key(io, ImGuiKey_F11, Keyboard.KEY_F11);
    add_key(io, ImGuiKey_F12, Keyboard.KEY_F12);

    add_key(io, ImGuiKey_Keypad0, Keyboard.KEY_NUMPAD0);
    add_key(io, ImGuiKey_Keypad1, Keyboard.KEY_NUMPAD1);
    add_key(io, ImGuiKey_Keypad2, Keyboard.KEY_NUMPAD2);
    add_key(io, ImGuiKey_Keypad3, Keyboard.KEY_NUMPAD3);
    add_key(io, ImGuiKey_Keypad4, Keyboard.KEY_NUMPAD4);
    add_key(io, ImGuiKey_Keypad5, Keyboard.KEY_NUMPAD5);
    add_key(io, ImGuiKey_Keypad6, Keyboard.KEY_NUMPAD6);
    add_key(io, ImGuiKey_Keypad7, Keyboard.KEY_NUMPAD7);
    add_key(io, ImGuiKey_Keypad8, Keyboard.KEY_NUMPAD8);
    add_key(io, ImGuiKey_Keypad9, Keyboard.KEY_NUMPAD9);
    add_key(io, ImGuiKey_KeypadDecimal, Keyboard.KEY_DECIMAL);
    add_key(io, ImGuiKey_KeypadDivide, Keyboard.KEY_DIVIDE);
    add_key(io, ImGuiKey_KeypadMultiply, Keyboard.KEY_MULTIPLY);
    add_key(io, ImGuiKey_KeypadSubtract, Keyboard.KEY_SUBTRACT);
    add_key(io, ImGuiKey_KeypadAdd, Keyboard.KEY_ADD);
    add_key(io, ImGuiKey_KeypadEnter, Keyboard.KEY_NUMPADENTER);
    add_key(io, ImGuiKey_KeypadEqual, Keyboard.KEY_NUMPADEQUALS);

    add_key(io, ImGuiKey_LeftShift, Keyboard.KEY_LSHIFT);
    add_key(io, ImGuiKey_LeftCtrl, Keyboard.KEY_LCONTROL);
    add_key(io, ImGuiKey_LeftAlt, Keyboard.KEY_LMENU);
    add_key(io, ImGuiKey_LeftSuper, Keyboard.KEY_LMETA);
    add_key(io, ImGuiKey_RightShift, Keyboard.KEY_RSHIFT);
    add_key(io, ImGuiKey_RightCtrl, Keyboard.KEY_RCONTROL);
    add_key(io, ImGuiKey_RightAlt, Keyboard.KEY_RMENU);
    add_key(io, ImGuiKey_RightSuper, Keyboard.KEY_RMETA);

    io.AddKeyEvent(ImGuiMod_Ctrl,
        input_key_down(Keyboard.KEY_LCONTROL) || input_key_down(Keyboard.KEY_RCONTROL));
    io.AddKeyEvent(ImGuiMod_Shift,
        input_key_down(Keyboard.KEY_LSHIFT) || input_key_down(Keyboard.KEY_RSHIFT));
    io.AddKeyEvent(ImGuiMod_Alt,
        input_key_down(Keyboard.KEY_LMENU) || input_key_down(Keyboard.KEY_RMENU));
    io.AddKeyEvent(ImGuiMod_Super,
        input_key_down(Keyboard.KEY_LMETA) || input_key_down(Keyboard.KEY_RMETA));

    while (Keyboard.next() != LWCGL_FALSE) {
        const uint32_t c = Keyboard.getEventCharacter();
        if (c != 0)
            io.AddInputCharacter(c);
    }
}
