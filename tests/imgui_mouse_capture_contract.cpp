#include <cstdio>

extern "C" bool bge_imgui_scene_mouse_blocked(bool cursor_locked,
                                                bool want_capture_mouse,
                                                bool any_window_hovered);

struct CaptureCase
{
    bool cursor_locked;
    bool want_capture_mouse;
    bool any_window_hovered;
    bool expected;
};

int main()
{
    const CaptureCase cases[] = {
        { false, false, false, false },
        { false, true,  false, true  },
        { false, false, true,  true  },
        { false, true,  true,  true  },
        { true,  true,  true,  false },
    };

    for (const CaptureCase& test : cases) {
        const bool actual = bge_imgui_scene_mouse_blocked(test.cursor_locked,
                                                          test.want_capture_mouse,
                                                          test.any_window_hovered);
        if (actual != test.expected) {
            std::fprintf(stderr,
                         "scene mouse block mismatch: locked=%d capture=%d hovered=%d expected=%d got=%d\n",
                         test.cursor_locked,
                         test.want_capture_mouse,
                         test.any_window_hovered,
                         test.expected,
                         actual);
            return 1;
        }
    }

    return 0;
}
