from pathlib import Path

path = Path("Engine/render.c")
text = path.read_text()

replacements = {
    "    g_debug_shots[slot].time = (f32)glfwGetTime();\n":
        "    const double timer_resolution = (double)Sys.getTimerResolution();\n"
        "    g_debug_shots[slot].time = timer_resolution > 0.0\n"
        "        ? (f32)((double)Sys.getTime() / timer_resolution)\n"
        "        : 0.0f;\n",
    "    f32 now = (f32)glfwGetTime();\n":
        "    const double timer_resolution = (double)Sys.getTimerResolution();\n"
        "    const f32 now = timer_resolution > 0.0\n"
        "        ? (f32)((double)Sys.getTime() / timer_resolution)\n"
        "        : 0.0f;\n",
    "    cam->firstMouse = true;\n": "",
}

for old, new in replacements.items():
    if old not in text:
        raise SystemExit(f"missing expected render pattern: {old.strip()}")
    text = text.replace(old, new)

if "glfw" in text or "GLFW_" in text:
    raise SystemExit("render.c still contains direct GLFW references")
if "firstMouse" in text:
    raise SystemExit("render.c still contains removed camera callback state")

path.write_text(text)
