#include "Engine/imgui_impl_lwcgl.h"
#include "Engine/input.h"
#include "imgui.h"

#include <cmath>
#include <cstdio>

static bool nearly_equal(float a, float b)
{
    return std::fabs(a - b) < 0.0001f;
}

int main()
{
    ImGui::CreateContext();
    input_init();

    if (!ImGui_ImplLwcgl_Init()) {
        std::fprintf(stderr, "failed to initialize lwcgl ImGui backend\n");
        return 2;
    }

    // Scaled X11/XWayland desktops can report a logical window extent larger
    // than the framebuffer extent. Dear ImGui must never shrink its rendering
    // viewport below the framebuffer in that case, or the UI is rendered tiny
    // into a corner of the window.
    ImGui_ImplLwcgl_NewFrame(1.0f / 60.0f, 2000, 1200, 1000, 600);

    const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
    const bool ok = nearly_equal(scale.x, 1.0f) && nearly_equal(scale.y, 1.0f);
    if (!ok)
        std::fprintf(stderr, "expected framebuffer scale 1x1, got %.3fx%.3f\n", scale.x, scale.y);

    ImGui_ImplLwcgl_Shutdown();
    ImGui::DestroyContext();
    return ok ? 0 : 1;
}
