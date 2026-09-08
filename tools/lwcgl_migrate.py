from pathlib import Path

FILES = [
    Path("Engine/text.c"),
    Path("Engine/level.c"),
    Path("Engine/render.c"),
    Path("Engine/editor.c"),
]

MODERN = {
    "glGenBuffers": "GL15",
    "glDeleteBuffers": "GL15",
    "glBindBuffer": "GL15",
    "glBufferData": "GL15",
    "glBufferSubData": "GL15",
    "glGetBufferSubData": "GL15",
    "glCreateShader": "GL20",
    "glShaderSource": "GL20",
    "glCompileShader": "GL20",
    "glGetShaderiv": "GL20",
    "glGetShaderInfoLog": "GL20",
    "glDeleteShader": "GL20",
    "glCreateProgram": "GL20",
    "glAttachShader": "GL20",
    "glDetachShader": "GL20",
    "glLinkProgram": "GL20",
    "glGetProgramiv": "GL20",
    "glGetProgramInfoLog": "GL20",
    "glUseProgram": "GL20",
    "glDeleteProgram": "GL20",
    "glGetUniformLocation": "GL20",
    "glUniform1i": "GL20",
    "glUniform1f": "GL20",
    "glUniform2f": "GL20",
    "glUniform3f": "GL20",
    "glUniform4f": "GL20",
    "glUniformMatrix4fv": "GL20",
    "glBindAttribLocation": "GL20",
    "glEnableVertexAttribArray": "GL20",
    "glDisableVertexAttribArray": "GL20",
    "glVertexAttribPointer": "GL20",
    "glDrawBuffers": "GL20",
    "glGenVertexArrays": "GL30",
    "glDeleteVertexArrays": "GL30",
    "glBindVertexArray": "GL30",
    "glGenFramebuffers": "GL30",
    "glDeleteFramebuffers": "GL30",
    "glBindFramebuffer": "GL30",
    "glFramebufferTexture2D": "GL30",
    "glFramebufferTextureLayer": "GL30",
    "glCheckFramebufferStatus": "GL30",
    "glBlitFramebuffer": "GL30",
    "glClearBufferfv": "GL30",
    "glGenerateMipmap": "GL30",
    "glBindBufferBase": "GL30",
    "glBindBufferRange": "GL30",
    "glMapBufferRange": "GL30",
    "glFlushMappedBufferRange": "GL30",
    "glUnmapBuffer": "GL30",
    "glGetStringi": "GL30",
    "glActiveTexture": "GLModern",
}


def require_replace(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        raise SystemExit(f"missing expected pattern: {label}")
    return text.replace(old, new)


for path in FILES:
    text = path.read_text()
    text = text.replace("#include <glad/glad.h>\n", "#include <lwcgl/glmodern.h>\n#include <lwcgl/lwcgl.h>\n")

    for name, api in MODERN.items():
        text = text.replace(f"{name}(", f"{api}.{name}(")

    path.write_text(text)

editor = Path("Engine/editor.c")
text = editor.read_text()

if '#include "input.h"\n' not in text:
    text = text.replace('#include "imgui_c.h"\n', '#include "imgui_c.h"\n#include "input.h"\n')

text = text.replace("    state.cam->firstMouse = true;\n", "")
text = text.replace("GL_GETFPS()", "app_get_fps()")

text = require_replace(
    text,
    "    f64 mx, my;\n"
    "    glfwGetCursorPos(state.win, &mx, &my);\n",
    "    const f64 mx_fb = (f64)Mouse.getX();\n"
    "    const f64 my_fb = (f64)Mouse.getY();\n"
    "    f64 mx = state.fb->w > 0 ? mx_fb * (f64)state.fb->ww / (f64)state.fb->w : 0.0;\n"
    "    const f64 my_bottom = state.fb->h > 0 ? my_fb * (f64)state.fb->wh / (f64)state.fb->h : 0.0;\n"
    "    f64 my = (f64)state.fb->wh - 1.0 - my_bottom;\n",
    "editor cursor picking",
)

text = require_replace(
    text,
    "    static bool mouse_was_pressed = false;\n"
    "    bool mouse_is_pressed = glfwGetMouseButton(state.win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;\n"
    "    bool ctrl_held = glfwGetKey(state.win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(state.win, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;\n",
    "    const bool mouse_is_pressed = input_mouse_down(0);\n"
    "    const bool ctrl_held = input_key_down(Keyboard.KEY_LCONTROL) || input_key_down(Keyboard.KEY_RCONTROL);\n",
    "editor mouse/control input",
)

text = require_replace(
    text,
    "    if (mouse_is_pressed && !mouse_was_pressed && !ui_capture)\n",
    "    if (input_mouse_pressed(0) && !ui_capture)\n",
    "editor click edge",
)
text = text.replace("\n    mouse_was_pressed = mouse_is_pressed;\n", "\n")

if "glfw" in text or "GLFW_" in text:
    raise SystemExit("editor still contains direct GLFW references")
editor.write_text(text)

for path in FILES:
    text = path.read_text()
    if "glad" in text:
        raise SystemExit(f"{path} still contains GLAD")
    for name in MODERN:
        bare = f"{name}("
        if bare in text:
            raise SystemExit(f"{path} still contains bare modern call {name}")
