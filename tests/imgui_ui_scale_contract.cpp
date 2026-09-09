#include "imgui.h"

#include <cmath>
#include <cstdio>

extern "C" float bge_imgui_scale_size(float value);

static bool nearly_equal(float a, float b)
{
    return std::fabs(a - b) < 0.0001f;
}

int main()
{
    if (!nearly_equal(bge_imgui_scale_size(10.0f), 15.0f)) {
        std::fprintf(stderr, "expected explicit ImGui size 10 -> 15\n");
        return 1;
    }
    if (!nearly_equal(bge_imgui_scale_size(0.0f), 0.0f)) {
        std::fprintf(stderr, "expected zero size to remain zero\n");
        return 1;
    }
    if (!nearly_equal(bge_imgui_scale_size(-25.0f), -37.5f)) {
        std::fprintf(stderr, "expected negative ImGui size semantics to scale\n");
        return 1;
    }
    return 0;
}
