# BGE lwcgl v2.9.3 Engine Rebuild Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rebuild BGE so all BGE-owned window, input, timing, context, and OpenGL loading code uses an already-installed `xt9y/lwcgl` `v2.9.3`, without losing BGE renderer, editor, level, portal, collision, weapon, text, post-processing, resize, or RendererCheck behavior.

**Architecture:** BGE keeps its own sector/portal raster renderer. `lwcgl` is the only platform boundary visible to BGE. Local BGE builds consume `/usr/local/include/lwcgl-2.9.3` and the installed shared `/usr/local/lib/liblwcgl` only; `build.c` performs no network or installation work. CI installs lwcgl separately before invoking `c build`. A tiny source-compatible lwcgl extension exposes logical window size so BGE can preserve its existing framebuffer-vs-window sizing on HiDPI displays without touching `Display.getNativeWindow()`.

**Tech Stack:** C/C++11 BGE, C-BuildSystem, lwcgl `v2.9.3`, OpenGL 3.3 core, Dear ImGui OpenGL3 backend, RendererCheck, GitHub Actions/Xvfb/Mesa.

**Spec:** `docs/superpowers/specs/2026-09-08-lwcgl-engine-rebuild-design.md`

## Global Constraints

- `build.c` must never clone, download, fetch, install, or update lwcgl or any other dependency.
- BGE links only the installed shared `liblwcgl`; BGE must not link `-lglfw` directly.
- GLFW may remain an internal dependency of the installed lwcgl shared library.
- BGE source/build files contain no GLFW calls, GLFW types, `GLFW_*`, GLAD includes/calls, or `imgui_impl_glfw` after migration.
- Request OpenGL 3.3 core with `lwcglSetContextVersion(3, 3)` and `lwcglSetContextProfile(LWCGL_CONTEXT_CORE_PROFILE)` before `Display.create()`.
- Do not integrate Horse or path tracing.
- Do not change level serialization/generated level headers.
- Preserve all existing editor/gameplay/render/RendererCheck behavior.
- Do not use `Display.getNativeWindow()` from BGE.
- Keep `main.c` as entry-only code.

---

## Task 1: Add logical window-size access to lwcgl v2.9.3

**Repository:** `xt9y/lwcgl`, branch `v2.9.3`

**Files:**
- Modify: `include/lwcgl/lwcgl.h`
- Modify: `src/display.c`
- Modify: `tests/runtime_smoke.c`

**Interfaces:**
- Produce:

```c
int lwcglDisplayGetWindowWidth(void);
int lwcglDisplayGetWindowHeight(void);
```

These are free functions rather than new `DisplayAPI` fields so the `DisplayAPI` struct ABI remains unchanged.

- [ ] **Step 1: Extend the runtime smoke contract first**

After `Display.create()` add:

```c
if (lwcglDisplayGetWindowWidth() <= 0) return 10;
if (lwcglDisplayGetWindowHeight() <= 0) return 11;
```

- [ ] **Step 2: Verify the test fails before implementation**

Run:

```sh
make check
```

Expected: compile/link failure for the two new symbols.

- [ ] **Step 3: Add declarations without changing `DisplayAPI`**

In `lwcgl.h` near `lwcglDisplayUpdateNoMessages()`:

```c
int lwcglDisplayGetWindowWidth(void);
int lwcglDisplayGetWindowHeight(void);
```

- [ ] **Step 4: Implement logical window-size getters**

In `src/display.c`:

```c
int lwcglDisplayGetWindowWidth(void)
{
    int value = mode.width;
    if (window && !fullscreen)
        glfwGetWindowSize(window, &value, NULL);
    return value > 0 ? value : mode.width;
}

int lwcglDisplayGetWindowHeight(void)
{
    int value = mode.height;
    if (window && !fullscreen)
        glfwGetWindowSize(window, NULL, &value);
    return value > 0 ? value : mode.height;
}
```

For fullscreen, returning `mode.width/height` preserves logical display-mode semantics.

- [ ] **Step 5: Run lwcgl tests**

```sh
make check
```

Expected: all existing contracts plus the two new assertions pass.

- [ ] **Step 6: Commit on `v2.9.3`**

```sh
git add include/lwcgl/lwcgl.h src/display.c tests/runtime_smoke.c
git commit -m "display: expose logical window dimensions"
```

---

## Task 2: Add BGE input core and relative camera math

**Repository:** `xt9y/BGE`

**Files:**
- Create: `Engine/input.h`
- Create: `Engine/input.c`
- Create: `tests/input_edges.c`
- Modify: `Engine/cam.h`
- Modify: `Engine/cam.c`

**Interfaces:**

```c
void input_init(void);
void input_begin_frame(void);
bool input_key_down(i32 key);
bool input_key_pressed(i32 key);
bool input_key_released(i32 key);
bool input_mouse_down(i32 button);
bool input_mouse_pressed(i32 button);
f32 input_mouse_dx(void);
f32 input_mouse_dy(void);
void input_set_grabbed(bool grabbed);
bool input_is_grabbed(void);
void camera_apply_mouse_delta(camera_t *cam, f32 dx, f32 dy);
```

- [ ] **Step 1: Write the failing edge-state test**

`tests/input_edges.c` verifies one key over three snapshots: up -> down produces `pressed`, held remains `down` but not `pressed`, down -> up produces `released`. Compile the test with `-DBGE_INPUT_TEST` and test-only setters declared under that define.

- [ ] **Step 2: Run the test and confirm missing symbols**

```sh
cc -DBGE_INPUT_TEST -I. -I/usr/local/include/lwcgl-2.9.3 tests/input_edges.c Engine/input.c -L/usr/local/lib -llwcgl -Wl,-rpath,/usr/local/lib -o /tmp/bge-input-test
```

Expected before implementation: compile/link failure.

- [ ] **Step 3: Implement snapshots using lwcgl**

Use `Keyboard.getKeyCount()` for the key-array bound, `Mouse.getButtonCount()` for mouse bounds, `Keyboard.isKeyDown()`, `Mouse.isButtonDown()`, `Mouse.getDX()`, `Mouse.getDY()`, and `Mouse.setGrabbed()`.

- [ ] **Step 4: Convert camera to relative deltas**

Remove `lastX`, `lastY`, `firstMouse`, the GLFW forward declaration, and `camera_mouse_callback()`. Implement:

```c
void camera_apply_mouse_delta(camera_t *cam, f32 dx, f32 dy)
{
    if (!cam) return;
    cam->yaw += dx * 0.1f;
    cam->pitch += dy * 0.1f;
    if (cam->pitch > 89.0f) cam->pitch = 89.0f;
    if (cam->pitch < -89.0f) cam->pitch = -89.0f;
    update_camera_vectors(cam);
}
```

`Mouse.getDY()` in lwcgl is already positive upward, matching the old callback's `lastY - ypos` behavior.

- [ ] **Step 5: Run the edge-state test**

Expected: zero exit status.

- [ ] **Step 6: Commit**

```sh
git add Engine/input.h Engine/input.c Engine/cam.h Engine/cam.c tests/input_edges.c
git commit -m "refactor: add lwcgl input core"
```

---

## Task 3: Cut BGE application lifecycle over to lwcgl

**Files:**
- Modify: `Engine/App.h`
- Modify: `Engine/App.c`
- Modify: `Engine/state.h`

**Interfaces:**

```c
int app_run(void);
double app_get_fps(void);
void fbo_resize(i32 w, i32 h);
void post_blit(i32 src_w, i32 src_h, i32 dst_w, i32 dst_h);
```

- [ ] **Step 1: Remove GLFW ownership from public state**

Delete `GLFWwindow *win` and GLFW includes/forward declarations from `App.h`, `state.h`, and camera headers.

- [ ] **Step 2: Initialize lwcgl exactly**

```c
DisplayMode mode = DisplayMode(WIDTH, HEIGHT);
if (Display.setDisplayMode(&mode) != 0) return -1;
Display.setTitle(TITLE);
Display.setResizable(LWCGL_TRUE);
Display.setVSyncEnabled(LWCGL_FALSE);
lwcglSetContextVersion(3, 3);
lwcglSetContextProfile(LWCGL_CONTEXT_CORE_PROFILE);
if (Display.create() != 0) {
    fprintf(stderr, "BGE: Display.create failed: %s\n", lwcglGetLastError());
    return -1;
}
if (Keyboard.create() != 0 || Mouse.create() != 0) {
    fprintf(stderr, "BGE: input creation failed: %s\n", lwcglGetLastError());
    return -1;
}
```

Do not call `lwcglLoadModernGL()` manually; `Display.create()` owns that.

- [ ] **Step 3: Preserve logical and framebuffer sizing**

Each frame:

```c
state.fb->w = Display.getWidth();
state.fb->h = Display.getHeight();
state.fb->ww = lwcglDisplayGetWindowWidth();
state.fb->wh = lwcglDisplayGetWindowHeight();
state.fb->scale = state.fb->ww > 0 ? (f32)state.fb->w / (f32)state.fb->ww : 1.0f;
```

- [ ] **Step 4: Replace timing**

```c
static double app_now_seconds(void)
{
    const double resolution = (double)Sys.getTimerResolution();
    return resolution > 0.0 ? (double)Sys.getTime() / resolution : 0.0;
}
```

Keep RendererCheck's deterministic `1.0f / 60.0f` override.

- [ ] **Step 5: Replace event/present/close flow**

Frame order:

```c
Display.processMessages();
input_begin_frame();
/* game input/update/render */
Display.updateNoMessages();
```

Exit on `Display.isCloseRequested()` or `STATE_EXIT`.

- [ ] **Step 6: Make shutdown partial-init safe**

Destroy GPU/game resources before input/display. Use `Mouse.isCreated()`, `Keyboard.isCreated()`, and `Display.isCreated()` before each corresponding destroy call.

- [ ] **Step 7: Commit**

```sh
git add Engine/App.h Engine/App.c Engine/state.h
git commit -m "refactor: run bge through lwcgl display lifecycle"
```

---

## Task 4: Move gameplay/bootstrap/input/render out of `main.c`

**Files:**
- Create: `Engine/game.h`
- Create: `Engine/game.c`
- Modify: `main.c`
- Modify: `Engine/App.c`
- Modify: `Engine/editor.c`
- Modify: `Engine/gun.c`

**Interfaces:**

```c
bool game_init(void);
void game_handle_input(void);
void game_update(void);
void game_render(void);
void game_shutdown(void);
```

- [ ] **Step 1: Move texture, weapon, level, editor, and camera initialization verbatim from `RUN()` into `game_init()`**

Preserve texture order, all weapon definitions, level loader order, editor level pointer, and `apply_level_camera()`.

- [ ] **Step 2: Replace every GLFW key/button predicate with input-core predicates**

Examples:

```c
if (input_key_down(Keyboard.KEY_ESCAPE)) state.id = STATE_EXIT;
if (input_key_pressed(Keyboard.KEY_H)) state.debug_visible = !state.debug_visible;
if (input_key_pressed(Keyboard.KEY_TAB)) {
    state.cursor_locked = !state.cursor_locked;
    input_set_grabbed(state.cursor_locked);
}
if (state.id == STATE_PLAYING && state.cursor_locked && input_mouse_pressed(0)) {
    shoot_bullet();
    gun_shot();
}
```

Preserve all existing bindings and repeat timing, including G/E/N/X/Delete/Backslash/R/7/8/9/V/Enter, shift modifiers, editor movement, and play movement.

- [ ] **Step 3: Preserve movement/portal/collision/height behavior**

Do not change speed `18.5f * state.dt`, forward/right math, `portal_try_teleport`, `player_collide_quads`, `level_get_height`, or lerp constants.

- [ ] **Step 4: Replace gun/editor direct GLFW input**

Use `input_key_down`, `input_key_pressed`, `input_mouse_down`, and ImGui capture flags. Do not leave subsystem-local GLFW edge state.

- [ ] **Step 5: Apply relative camera motion**

```c
if (state.cursor_locked)
    camera_apply_mouse_delta(state.cam, input_mouse_dx(), input_mouse_dy());
```

- [ ] **Step 6: Move current `RENDER()` into `game_render()`**

Use `state.fb->w/h` for framebuffer rendering and `state.fb->ww/wh` for text/editor logical sizing. Preserve `render_main`, `post_blit`, crosshair, editor UI, and ImGui order.

- [ ] **Step 7: Reduce `main.c` to entry only**

```c
#include "Engine/App.h"

int main(void)
{
    return app_run();
}
```

- [ ] **Step 8: Commit**

```sh
git add main.c Engine/App.c Engine/game.h Engine/game.c Engine/editor.c Engine/gun.c
git commit -m "refactor: move game runtime behind app entry"
```

---

## Task 5: Remove GLAD and route modern OpenGL through lwcgl

**Files:**
- Modify: `Engine/gfx.h`
- Modify: `Engine/gfx.c`
- Modify: `Engine/App.c`
- Modify: `Engine/text.c`
- Modify: `Engine/level.c`
- Modify: `Engine/render.c`
- Modify: `Engine/editor.c`
- Modify: `Engine/gun.c`

**Interfaces:**
- Use `<lwcgl/lwcgl.h>` and `<lwcgl/glmodern.h>`.
- Base OpenGL 1.x state/texture/draw calls remain normal `gl*` calls from lwcgl's public OpenGL headers.
- Modern calls use `GL15`, `GL20`, `GL30`, `GL33`, or `GLModern`.

- [ ] **Step 1: Replace every `<glad/glad.h>` include**

Use:

```c
#include <lwcgl/lwcgl.h>
#include <lwcgl/glmodern.h>
```

- [ ] **Step 2: Convert shader/program calls**

Use `GL20.glCreateShader`, `glShaderSource`, `glCompileShader`, shader/program status/log calls, program creation/link/use/delete, uniform lookup/uploads, and vertex-attrib functions.

- [ ] **Step 3: Convert VBO/VAO calls**

Use `GL15` for buffers and `GL30` for VAOs/FBOs. Keep base `glDrawArrays`, `glDrawElements`, texture calls, state calls, stencil calls, and viewport calls as normal public OpenGL calls.

- [ ] **Step 4: Replace active texture and readback helpers**

Use:

```c
GLModern.glActiveTexture(GL_TEXTURE0);
GLModern.glPixelStorei(GL_PACK_ALIGNMENT, 1);
GLModern.glReadPixels(...);
```

- [ ] **Step 5: Verify GLAD is gone from BGE source**

```sh
! grep -RIn --exclude-dir=Vendor --exclude-dir=docs 'glad' main.c Engine build.c
```

- [ ] **Step 6: Commit**

```sh
git add Engine
git commit -m "refactor: route bge rendering through lwcgl gl dispatch"
```

---

## Task 6: Replace depth/stencil renderbuffer and RendererCheck query API

**Files:**
- Modify: `Engine/App.c`

- [ ] **Step 1: Make depth/stencil attachment texture-backed**

Use normal texture calls plus `GL30.glFramebufferTexture2D`:

```c
glGenTextures(1, &g_fbo_depth_stencil);
glBindTexture(GL_TEXTURE_2D, g_fbo_depth_stencil);
glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8,
             w, h, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
GL30.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_TEXTURE_2D, g_fbo_depth_stencil, 0);
```

Delete it with `glDeleteTextures` during resize/shutdown.

- [ ] **Step 2: Validate framebuffer completeness**

```c
const GLenum status = GL30.glCheckFramebufferStatus(GL_FRAMEBUFFER);
if (status != GL_FRAMEBUFFER_COMPLETE) {
    fprintf(stderr, "BGE: framebuffer incomplete: 0x%x\n", (unsigned)status);
    state.id = STATE_EXIT;
}
```

- [ ] **Step 3: Replace elapsed query with timestamp pair**

Generate two queries with `GL33.glGenQueries`. Begin/end with `GL33.glQueryCounter(query, GL_TIMESTAMP)`. Read with `GL33.glGetQueryObjectui64v(..., GL_QUERY_RESULT, ...)`, subtract nanoseconds, and report milliseconds through `rendercheck_gpu_ms()`.

- [ ] **Step 4: Remove the RendererCheck window parameter**

Capture dimensions from `Display.getWidth()/getHeight()` and use `GLModern.glReadPixels`.

- [ ] **Step 5: Commit**

```sh
git add Engine/App.c
git commit -m "refactor: move framebuffer and gpu timing to lwcgl"
```

---

## Task 7: Replace ImGui GLFW platform backend

**Files:**
- Create: `Engine/imgui_impl_lwcgl.h`
- Create: `Engine/imgui_impl_lwcgl.cpp`
- Modify: `Engine/imgui_c.h`
- Modify: `Engine/imgui_c.cpp`
- Modify: `build.c`

**Interfaces:**

```cpp
bool ImGui_ImplLwcgl_Init();
void ImGui_ImplLwcgl_Shutdown();
void ImGui_ImplLwcgl_NewFrame(float dt, int window_w, int window_h, int framebuffer_w, int framebuffer_h);
void ImGui_ImplLwcgl_SetMouseEnabled(bool enabled);
```

- [ ] **Step 1: Initialize a custom platform backend without native handles**

Set `io.BackendPlatformName = "imgui_impl_lwcgl"`. Do not claim backend flags for unsupported features.

- [ ] **Step 2: Feed display size and framebuffer scale**

```cpp
io.DisplaySize = ImVec2((float)window_w, (float)window_h);
io.DisplayFramebufferScale = ImVec2(
    window_w > 0 ? (float)framebuffer_w / (float)window_w : 1.0f,
    window_h > 0 ? (float)framebuffer_h / (float)window_h : 1.0f);
io.DeltaTime = dt > 0.0f ? dt : 1.0f / 60.0f;
```

- [ ] **Step 3: Feed mouse position/buttons/wheel**

Convert lwcgl framebuffer-space mouse coordinates to logical top-left ImGui coordinates:

```cpp
const float mx = framebuffer_w > 0 ? (float)Mouse.getX() * window_w / framebuffer_w : 0.0f;
const float my_bottom = framebuffer_h > 0 ? (float)Mouse.getY() * window_h / framebuffer_h : 0.0f;
io.AddMousePosEvent(mx, (float)window_h - 1.0f - my_bottom);
```

Use `Mouse.isButtonDown(0..4)` and `Mouse.getDWheel() / 120.0f`.

- [ ] **Step 4: Feed keyboard/modifier events**

Map LWJGL keys required by BGE/ImGui widgets to `ImGuiKey_*`: A-Z, 0-9, arrows, Enter, Escape, Tab, Backspace, Delete, Insert, Home, End, PageUp/PageDown, Space, punctuation used by widgets, Shift/Ctrl/Alt/Super. Send current state with `io.AddKeyEvent` and modifier events with `ImGuiMod_*`.

- [ ] **Step 5: Remove `GLFWwindow*` from `imgui_c`**

`imgui_init(void)` creates context, initializes `ImGui_ImplLwcgl_Init()`, then `ImGui_ImplOpenGL3_Init("#version 330 core")`. New frame calls OpenGL3, lwcgl backend, then `ImGui::NewFrame()`. Shutdown reverses the order.

The OpenGL3 renderer backend keeps its own embedded GL loader and requires no GLAD/GLFW loader from BGE.

- [ ] **Step 6: Commit**

```sh
git add Engine/imgui_impl_lwcgl.h Engine/imgui_impl_lwcgl.cpp Engine/imgui_c.h Engine/imgui_c.cpp build.c
git commit -m "refactor: replace imgui glfw backend with lwcgl"
```

---

## Task 8: Final BGE build cutover and repository cleanup

**Files:**
- Modify: `build.c`
- Modify: `.github/workflows/headless-ci.yml`
- Modify: `README.md`
- Delete: `Vendor/glfw/**`
- Delete: `Vendor/glad/**`
- Delete: `Vendor/imgui/imgui_impl_glfw.cpp`
- Delete: `Vendor/imgui/imgui_impl_glfw.h`
- Delete: `Engine/CMakeLists.txt` if it only represents the old unsupported GLFW build path.

- [ ] **Step 1: Make `build.c` consume only installed lwcgl**

Remove `add_glfw()`, `Vendor/glad/src/glad.c`, GLFW/GLAD include paths, and `imgui_impl_glfw.cpp`.

Add:

```c
c_include(app, "/usr/local/include/lwcgl-2.9.3");
c_link_flag(app, "-L/usr/local/lib");
c_link_flag(app, "-llwcgl");
c_link_flag(app, "-Wl,-rpath,/usr/local/lib");
```

Do not add `-lglfw`; the installed shared `liblwcgl` already carries its GLFW dependency. Keep only system GL/framework libraries BGE itself still directly needs for public OpenGL symbols and C++/ImGui linkage.

- [ ] **Step 2: Add new source files**

```c
c_sources(app, "Engine/input.c");
c_sources(app, "Engine/game.c");
c_sources(app, "Engine/imgui_impl_lwcgl.cpp");
```

- [ ] **Step 3: Update CI graphics dependencies for building lwcgl**

Linux apt list must include `pkg-config`, `libglfw3-dev`, `libglu1-mesa-dev` in addition to the existing Mesa/X11/Xvfb packages.

- [ ] **Step 4: Install lwcgl in CI outside BGE build**

```yaml
      - name: Install lwcgl v2.9.3
        shell: bash
        run: |
          set -euo pipefail
          git clone --depth 1 --branch v2.9.3 https://github.com/xt9y/lwcgl.git /tmp/lwcgl
          make -C /tmp/lwcgl -j2 all
          sudo make -C /tmp/lwcgl install
```

No equivalent code goes into `build.c`.

- [ ] **Step 5: Add final direct-dependency contract**

```yaml
      - name: Reject direct GLFW and GLAD dependencies
        shell: bash
        run: |
          set -euo pipefail
          if grep -RInE --exclude-dir=.git --exclude-dir=docs \
            'GLFW_|glfw|glad|#include[[:space:]]*<GLFW/' main.c Engine build.c; then
            echo 'BGE still contains a direct GLFW/GLAD dependency' >&2
            exit 1
          fi
```

- [ ] **Step 6: Delete dead vendor/platform code**

Delete GLFW, GLAD, the ImGui GLFW backend, and stale CMake wiring only after source migration is complete.

- [ ] **Step 7: Update README**

Document only this local workflow:

```markdown
Install lwcgl v2.9.3 system-wide first, then:

```sh
c build run
```

BGE's `build.c` does not download dependencies.
```

- [ ] **Step 8: Commit**

```sh
git add -A
git commit -m "build: complete lwcgl-only platform cutover"
```

---

## Task 9: Verification before completion

**Files:**
- Validate: full repository and CI.

- [ ] **Step 1: Build with only installed lwcgl**

```sh
c build
test -x build/debug/bge
```

- [ ] **Step 2: Run source contract**

Expected: zero forbidden BGE GLFW/GLAD matches.

- [ ] **Step 3: Run input unit contract**

Expected: zero exit status.

- [ ] **Step 4: Run headless smoke test**

```sh
xvfb-run -a env LIBGL_ALWAYS_SOFTWARE=1 \
  timeout --signal=TERM --kill-after=2s 5s ./build/debug/bge
```

Accept existing CI exit codes `0`, `124`, or `143`.

- [ ] **Step 5: Run RendererCheck runtime**

```sh
renderercheck run runtime
test -s .rendercheck/runtime/metrics.txt
grep -q '^gpu_ms=' .rendercheck/runtime/metrics.txt
```

Keep software-renderer and timing-kind classifications unchanged.

- [ ] **Step 6: Run existing full visual regression flow**

Preserve baseline-missing failure, approve, pass, diff, deliberately-corrupt baseline failure, PNG diff artifact, re-approve, and final pass.

- [ ] **Step 7: Run full suite**

```sh
renderercheck run
```

Require `.rendercheck/report.md`, `.rendercheck/results.json`, runtime stdout, and visual PNG artifacts.

- [ ] **Step 8: Manual behavior matrix**

Verify launch/exit, resize/HiDPI sizing, mouse grab, camera look, play/editor movement, portals, teleport, collision, height following, editor UI/input/add/delete/reset/paint/transform/save, all current levels, textures, low-resolution render target, palette post process, text/crosshair, weapon switching/shooting/animation/muzzle flash, debug bullets, and stencil behavior.

- [ ] **Step 9: Verify GitHub Headless CI is green on the final BGE commit**

Do not claim completion until the workflow passes.

---

## Final Acceptance

1. `build.c` has no dependency download/install/network logic.
2. Local setup is: install lwcgl `v2.9.3` once, then `c build run` BGE.
3. BGE links installed `liblwcgl`, not GLFW.
4. GLFW is implementation detail of lwcgl only.
5. BGE no longer vendors GLFW or GLAD.
6. `main.c` is entry-only.
7. Existing BGE raster/portal/editor/game functionality remains.
8. RendererCheck/headless CI passes.
