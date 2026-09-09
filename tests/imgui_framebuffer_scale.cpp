#include "Engine/imgui_impl_lwcgl.h"
#include "Engine/input.h"
#include "imgui.h"

#include <cmath>
#include <cstdio>

static bool nearly_equal(float a, float b)
{
    return std::fabs(a - b) < 0.0001f;
}

static float imgui_scaled_size(float value)
{
    return std::trunc(value * 1.5f);
}

int main()
{
    ImGui::CreateContext();
    input_init();

    const ImGuiStyle before = ImGui::GetStyle();

    if (!ImGui_ImplLwcgl_Init()) {
        std::fprintf(stderr, "failed to initialize lwcgl ImGui backend\n");
        return 2;
    }

    const ImGuiStyle& style = ImGui::GetStyle();
    bool ok = true;

    if (!nearly_equal(style.WindowPadding.x, imgui_scaled_size(before.WindowPadding.x)) ||
        !nearly_equal(style.WindowPadding.y, imgui_scaled_size(before.WindowPadding.y)) ||
        !nearly_equal(style.FramePadding.x, imgui_scaled_size(before.FramePadding.x)) ||
        !nearly_equal(style.FramePadding.y, imgui_scaled_size(before.FramePadding.y)) ||
        !nearly_equal(style.ItemSpacing.x, imgui_scaled_size(before.ItemSpacing.x)) ||
        !nearly_equal(style.ItemSpacing.y, imgui_scaled_size(before.ItemSpacing.y)) ||
        !nearly_equal(style.FontScaleDpi, 1.5f)) {
        std::fprintf(stderr,
                     "expected ImGui 1.5x scale (integer-truncated style sizes): "
                     "window=(%.3f,%.3f) frame=(%.3f,%.3f) item=(%.3f,%.3f) font=%.3f\n",
                     style.WindowPadding.x, style.WindowPadding.y,
                     style.FramePadding.x, style.FramePadding.y,
                     style.ItemSpacing.x, style.ItemSpacing.y,
                     style.FontScaleDpi);
        ok = false;
    }

    // Scaled X11/XWayland desktops can report a logical window extent larger
    // than the framebuffer extent. The UI scale must not alter the actual
    // framebuffer-density contract.
    ImGui_ImplLwcgl_NewFrame(1.0f / 60.0f, 2000, 1200, 1000, 600);

    const ImVec2 framebuffer_scale = ImGui::GetIO().DisplayFramebufferScale;
    if (!nearly_equal(framebuffer_scale.x, 1.0f) || !nearly_equal(framebuffer_scale.y, 1.0f)) {
        std::fprintf(stderr, "expected framebuffer scale 1x1, got %.3fx%.3f\n",
                     framebuffer_scale.x, framebuffer_scale.y);
        ok = false;
    }

    ImGui_ImplLwcgl_Shutdown();
    ImGui::DestroyContext();
    return ok ? 0 : 1;
}
