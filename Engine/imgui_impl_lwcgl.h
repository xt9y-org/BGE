#ifndef IMGUI_IMPL_LWCGL_H
#define IMGUI_IMPL_LWCGL_H

#define BGE_IMGUI_UI_SCALE 3.0f

bool ImGui_ImplLwcgl_Init(void);
void ImGui_ImplLwcgl_Shutdown(void);
void ImGui_ImplLwcgl_NewFrame(float dt,
                              int window_w,
                              int window_h,
                              int framebuffer_w,
                              int framebuffer_h);

#endif
