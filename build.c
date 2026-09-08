#include <cbuild.h>

static void add_imgui(C_Target *app)
{
    c_sources(app, "Vendor/imgui/imgui.cpp");
    c_sources(app, "Vendor/imgui/imgui_draw.cpp");
    c_sources(app, "Vendor/imgui/imgui_tables.cpp");
    c_sources(app, "Vendor/imgui/imgui_widgets.cpp");
    c_sources(app, "Vendor/imgui/imgui_impl_opengl3.cpp");
    c_sources(app, "Engine/imgui_impl_lwcgl.cpp");
}

void build(C_Build *b)
{
    C_Target *app = c_executable(b, "bge");

    c_sources(app, "main.c");
    c_sources(app, "Engine/App.c");
    c_sources(app, "Engine/game.c");
    c_sources(app, "Engine/input.c");
    c_sources(app, "Engine/cam.c");
    c_sources(app, "Engine/gfx.c");
    c_sources(app, "Engine/text.c");
    c_sources(app, "Engine/level.c");
    c_sources(app, "Engine/portal.c");
    c_sources(app, "Engine/editor.c");
    c_sources(app, "Engine/gun.c");
    c_sources(app, "Engine/render.c");
    c_sources(app, "Engine/util/math.c");
    c_sources(app, "Engine/imgui_c.cpp");
    add_imgui(app);

    c_include(app, ".");
    c_include(app, "Engine");
    c_include(app, "Vendor/imgui");
    c_include(app, "/usr/local/include/lwcgl-2.9.3");

    c_define(app, "GL_SILENCE_DEPRECATION");
    c_flag(app, "-std=c++11");
    c_flag(app, "-Wall");
    c_flag(app, "-Wextra");

    c_link_flag(app, "-L/usr/local/lib");
    c_link_flag(app, "-llwcgl");
    c_link_flag(app, "-Wl,-rpath,/usr/local/lib");

#ifdef __APPLE__
    c_link_system(app, "c++");
    c_framework(app, "OpenGL");
#else
    c_link_system(app, "stdc++");
    c_link_system(app, "GL");
    c_link_system(app, "m");
    c_link_system(app, "dl");
    c_link_system(app, "pthread");
#endif
}
