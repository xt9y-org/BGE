from pathlib import Path

# One-shot guarded migration for the ImGui C/C++ frame boundary; rerun after staging fix.
header = Path("Engine/imgui_c.h")
text = header.read_text()
old = "void imgui_newframe(void);\n"
new = "void imgui_newframe(f32 dt, i32 window_w, i32 window_h, i32 framebuffer_w, i32 framebuffer_h);\n"
if old not in text:
    raise SystemExit("missing imgui_newframe declaration")
header.write_text(text.replace(old, new))

cpp = Path("Engine/imgui_c.cpp")
text = cpp.read_text()
include = '#include "state.h"\n'
if include not in text:
    raise SystemExit("missing state include in imgui C++ wrapper")
text = text.replace(include, "")
old = '''void imgui_newframe(void)
{
    if (!ImGui::GetCurrentContext()) return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplLwcgl_NewFrame(
        state.dt,
        state.fb ? state.fb->ww : 0,
        state.fb ? state.fb->wh : 0,
        state.fb ? state.fb->w : 0,
        state.fb ? state.fb->h : 0);
    ImGui::NewFrame();
}
'''
new = '''void imgui_newframe(f32 dt, i32 window_w, i32 window_h, i32 framebuffer_w, i32 framebuffer_h)
{
    if (!ImGui::GetCurrentContext()) return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplLwcgl_NewFrame(dt, window_w, window_h, framebuffer_w, framebuffer_h);
    ImGui::NewFrame();
}
'''
if old not in text:
    raise SystemExit("missing imgui frame implementation")
text = text.replace(old, new)
if 'state.' in text or '#include "state.h"' in text:
    raise SystemExit("imgui C++ wrapper still depends on engine state")
cpp.write_text(text)

game = Path("Engine/game.c")
text = game.read_text()
old = "    imgui_newframe();\n"
new = "    imgui_newframe(state.dt, state.fb->ww, state.fb->wh, state.fb->w, state.fb->h);\n"
if old not in text:
    raise SystemExit("missing game imgui frame call")
game.write_text(text.replace(old, new))
