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

    const ImGuiStyle before = ImGui::GetStyle();

    if (!ImGui_ImplLwcgl_Init()) {
        std::fprintf(stderr, "failed to initialize lwcgl ImGui backend\n");
        return 2;
    }

    const ImGuiStyle& style = ImGui::GetStyle();
    bool ok = true;

    if (!nearly_equal(style.WindowPadding.x, before.WindowPadding.x * 3.0f) ||
        !nearly_equal(style.WindowPadding.y, before.WindowPadding.y * 3.0f) ||
        !nearly_equal(style.FramePadding.x, before.FramePadding.x * 3.0f) ||
        !nearly_equal(style.FramePadding.y, before.FramePadding.y * 3.0f) ||
        !nearly_equal(style.ItemSpacing.x, before.ItemSpacing.x * 3.0f) ||
        !nearly_equal(style.ItemSpacing.y, before.ItemSpacing.y * 3.0f) ||
        !nearly_equal(style.FontScaleDpi, 3.0f)) {
        std::fprintf(stderr, "expected ImGui style/font scale to be 3x\n");
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
