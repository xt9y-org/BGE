#include "imgui_c.h"

#include "imgui.h"
#include "imgui_impl_lwcgl.h"
#include "imgui_impl_opengl3.h"
#include "state.h"

#include <cstdarg>
#include <cstdio>

extern "C" {

bool imgui_init(void)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = "imgui.ini";

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.TabRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.WindowBorderSize = 1.0f;

    if (!ImGui_ImplLwcgl_Init()) {
        ImGui::DestroyContext();
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
        ImGui_ImplLwcgl_Shutdown();
        ImGui::DestroyContext();
        return false;
    }
    return true;
}

void imgui_newframe(void)
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

void imgui_render(void)
{
    if (!ImGui::GetCurrentContext()) return;
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void imgui_shutdown(void)
{
    if (!ImGui::GetCurrentContext()) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplLwcgl_Shutdown();
    ImGui::DestroyContext();
}

bool imgui_want_capture_mouse(void)
{
    return ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse;
}

bool imgui_want_capture_keyboard(void)
{
    return ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard;
}

bool imgui_is_any_window_hovered(void)
{
    return ImGui::GetCurrentContext() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
}

void imgui_set_mouse_enabled(bool enabled)
{
    if (!ImGui::GetCurrentContext()) return;
    ImGuiIO& io = ImGui::GetIO();
    if (enabled) io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    else io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
}

bool imgui_begin(const char* title, bool* open, i32 window_flags)
{
    return ImGui::Begin(title, open, (ImGuiWindowFlags)window_flags);
}

void imgui_end(void)
{
    ImGui::End();
}

void imgui_set_next_window_pos(f32 x, f32 y, i32 cond)
{
    ImGui::SetNextWindowPos(ImVec2(x, y), (ImGuiCond)cond);
}

void imgui_set_next_window_size(f32 w, f32 h, i32 cond)
{
    ImGui::SetNextWindowSize(ImVec2(w, h), (ImGuiCond)cond);
}

void imgui_set_next_window_bg_alpha(f32 alpha)
{
    ImGui::SetNextWindowBgAlpha(alpha);
}

void imgui_set_next_window_collapsed(bool collapsed, i32 cond)
{
    ImGui::SetNextWindowCollapsed(collapsed, (ImGuiCond)cond);
}

void imgui_text(const char* fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    ImGui::TextUnformatted(buf);
}

void imgui_same_line(void)
{
    ImGui::SameLine();
}

void imgui_same_line_offset(f32 x)
{
    ImGui::SameLine(x);
}

void imgui_separator(void)
{
    ImGui::Separator();
}

void imgui_spacing(void)
{
    ImGui::Spacing();
}

void imgui_new_line(void)
{
    ImGui::NewLine();
}

void imgui_indent(void)
{
    ImGui::Indent();
}

void imgui_unindent(void)
{
    ImGui::Unindent();
}

void imgui_begin_disabled(bool disabled)
{
    ImGui::BeginDisabled(disabled);
}

void imgui_end_disabled(void)
{
    ImGui::EndDisabled();
}

void imgui_push_item_width(f32 width)
{
    ImGui::PushItemWidth(width);
}

void imgui_pop_item_width(void)
{
    ImGui::PopItemWidth();
}

void imgui_align_text_to_frame_padding(void)
{
    ImGui::AlignTextToFramePadding();
}

bool imgui_button(const char* label)
{
    return ImGui::Button(label);
}

bool imgui_checkbox(const char* label, bool* v)
{
    return ImGui::Checkbox(label, v);
}

bool imgui_slider_float(const char* label, f32* v, f32 v_min, f32 v_max)
{
    return ImGui::SliderFloat(label, v, v_min, v_max);
}

bool imgui_slider_int(const char* label, i32* v, i32 v_min, i32 v_max)
{
    return ImGui::SliderInt(label, v, v_min, v_max);
}

bool imgui_color_edit3(const char* label, f32 v[3])
{
    return ImGui::ColorEdit3(label, v, ImGuiColorEditFlags_NoInputs);
}

bool imgui_color_edit4(const char* label, f32 v[4])
{
    return ImGui::ColorEdit4(label, v, ImGuiColorEditFlags_NoInputs);
}

bool imgui_input_int(const char* label, i32* v)
{
    return ImGui::InputInt(label, v);
}

bool imgui_input_float(const char* label, f32* v)
{
    return ImGui::InputFloat(label, v);
}

bool imgui_combo(const char* label, i32* current_idx, const char* const* items, i32 items_count)
{
    return ImGui::Combo(label, current_idx, items, items_count);
}

bool imgui_selectable(const char* label, bool selected)
{
    return ImGui::Selectable(label, selected);
}

bool imgui_collapsing_header(const char* label)
{
    return ImGui::CollapsingHeader(label);
}

bool imgui_radio(const char* label, i32* v, i32 v_button)
{
    return ImGui::RadioButton(label, v, v_button);
}

bool imgui_begin_child(const char* str_id, f32 width, f32 height, bool border)
{
    const ImGuiChildFlags flags = border ? ImGuiChildFlags_Borders : ImGuiChildFlags_None;
    return ImGui::BeginChild(str_id, ImVec2(width, height), flags);
}

void imgui_end_child(void)
{
    ImGui::EndChild();
}

void imgui_image(u32 texture_id, f32 width, f32 height)
{
    ImGui::Image(ImTextureRef((ImTextureID)(uintptr_t)texture_id), ImVec2(width, height));
}

bool imgui_image_button(const char* str_id, u32 texture_id, f32 width, f32 height)
{
    return ImGui::ImageButton(str_id, ImTextureRef((ImTextureID)(uintptr_t)texture_id), ImVec2(width, height));
}

bool imgui_is_item_clicked(i32 mouse_button)
{
    return ImGui::IsItemClicked((ImGuiMouseButton)mouse_button);
}

bool imgui_is_item_hovered(void)
{
    return ImGui::IsItemHovered();
}

bool imgui_is_window_hovered(void)
{
    return ImGui::IsWindowHovered();
}

bool imgui_is_any_item_hovered(void)
{
    return ImGui::IsAnyItemHovered();
}

void imgui_set_tooltip(const char* fmt, ...)
{
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    ImGui::SetTooltip("%s", buf);
}

void imgui_open_popup(const char* str_id)
{
    ImGui::OpenPopup(str_id);
}

bool imgui_begin_popup(const char* str_id)
{
    return ImGui::BeginPopup(str_id);
}

void imgui_end_popup(void)
{
    ImGui::EndPopup();
}

void imgui_close_current_popup(void)
{
    ImGui::CloseCurrentPopup();
}

void imgui_push_id_int(i32 id)
{
    ImGui::PushID(id);
}

void imgui_pop_id(void)
{
    ImGui::PopID();
}

f32 imgui_get_font_size(void)
{
    return ImGui::GetFontSize();
}

f32 imgui_calc_text_width(const char* text)
{
    return ImGui::CalcTextSize(text).x;
}

}
