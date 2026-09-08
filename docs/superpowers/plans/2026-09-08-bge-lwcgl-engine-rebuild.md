# BGE lwcgl v2.9.3 Engine Rebuild Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rebuild BGE so all BGE-owned window, input, timing, context, and OpenGL loading code uses an already-installed `xt9y/lwcgl` `v2.9.3`, while preserving the complete existing BGE renderer, levels, portals, collision, editor, weapons, text, post-processing, and RendererCheck behavior.

**Architecture:** BGE keeps its own sector/portal raster renderer and game/editor modules. `lwcgl` becomes the only platform-facing boundary exposed to BGE: `Display`, `Keyboard`, `Mouse`, `Sys`, context configuration, GL11 compatibility calls, and modern GL dispatch. BGE does not download dependencies from `build.c`; local builds consume a system-installed lwcgl, while CI installs the exact `v2.9.3` branch in a separate setup step before invoking C-BuildSystem.

**Tech Stack:** C11/C++11 mixed BGE code, C-BuildSystem, lwcgl `v2.9.3`, OpenGL 3.3 core, Dear ImGui OpenGL3 renderer backend, RendererCheck, GitHub Actions/Xvfb/Mesa.

**Spec:** `docs/superpowers/specs/2026-09-08-lwcgl-engine-rebuild-design.md`

## Global Constraints

- BGE must use `xt9y/lwcgl` branch `v2.9.3` as the only window, input, timing, context, and OpenGL loading dependency exposed to BGE.
- `build.c` must never clone, download, fetch, install, or update lwcgl or any other dependency.
- Local builds consume the machine-installed lwcgl include/library paths only.
- CI may clone/build/install lwcgl `v2.9.3` in a setup step before `c build`; that setup is outside `build.c`.
- Do not integrate Horse in this migration.
- Do not add path tracing.
- Do not change level serialization or generated level-header semantics.
- Preserve editor, portals, collision, weapons, text, post-processing, low-resolution rendering, palette quantization, and RendererCheck behavior.
- Remove all direct BGE source/build references to GLFW, GLAD, `GLFW_*`, and `imgui_impl_glfw`.
- Preserve the C-BuildSystem `build.c` flow as the supported build path.
- Request an OpenGL 3.3 core context through lwcgl before `Display.create()`.
- Shutdown must be safe after partial initialization.

---

## File Structure

### New files

- `Engine/input.h` — BGE action/edge-state interface backed by lwcgl Keyboard/Mouse.
- `Engine/input.c` — per-frame keyboard/mouse snapshots, press/release edges, relative mouse delta, grab state.
- `Engine/game.h` — game bootstrap/update/render/shutdown interface used by the application runtime.
- `Engine/game.c` — texture/weapon/level/editor/camera bootstrap currently living in `main.c`, plus gameplay/editor action handling.
- `Engine/imgui_impl_lwcgl.h` — small Dear ImGui platform backend API.
- `Engine/imgui_impl_lwcgl.cpp` — Dear ImGui platform backend using lwcgl only.

### Files modified

- `main.c` — reduced to the engine entry point.
- `build.c` — system-installed lwcgl include/linking, new sources, no vendored GLFW/GLAD compilation.
- `Engine/App.h` — lwcgl-facing runtime declarations, no GLFW declarations.
- `Engine/App.c` — Display/Keyboard/Mouse/Sys lifecycle, framebuffer/post-process ownership, RendererCheck timing/capture.
- `Engine/state.h` — remove `GLFWwindow*`; retain BGE-only state.
- `Engine/cam.h` / `Engine/cam.c` — relative-mouse-delta camera API; no GLAD/GLFW dependency.
- `Engine/gfx.h` / `Engine/gfx.c` — lwcgl GL types/modern dispatch for shaders and programs.
- `Engine/text.c` — lwcgl GL dispatch.
- `Engine/level.c` — lwcgl GL dispatch.
- `Engine/render.c` — lwcgl GL dispatch.
- `Engine/editor.c` — lwcgl GL dispatch and centralized input use.
- `Engine/gun.c` — lwcgl GL dispatch and centralized input use.
- `Engine/imgui_c.h` / `Engine/imgui_c.cpp` — remove GLFW window API; use the custom lwcgl backend.
- `.github/workflows/headless-ci.yml` — install lwcgl `v2.9.3` before BGE build and enforce no-direct-GLFW/GLAD contract.
- `README.md` — document the system-installed lwcgl prerequisite and unchanged `c build run` workflow.

### Files/directories removed after migration

- `Vendor/glfw/**`
- `Vendor/glad/**`
- `Vendor/imgui/imgui_impl_glfw.cpp`
- `Vendor/imgui/imgui_impl_glfw.h`
- `Engine/CMakeLists.txt` if it remains only as the unsupported vendored-GLFW path.

---

### Task 1: Make lwcgl an external system dependency and lock CI contract

**Files:**
- Modify: `build.c`
- Modify: `.github/workflows/headless-ci.yml`
- Modify: `README.md`

**Interfaces:**
- Consumes: installed lwcgl headers under `/usr/local/include/lwcgl-2.9.3` and library under `/usr/local/lib`.
- Produces: BGE link against `-llwcgl`; CI installs lwcgl separately before `c build`; source-contract grep gate.

- [ ] **Step 1: Add the CI contract before changing build wiring**

Add a step before the BGE build:

```yaml
      - name: Reject direct GLFW and GLAD dependencies
        shell: bash
        run: |
          set -euo pipefail
          if grep -RInE \
            --exclude-dir=.git \
            --exclude-dir=docs \
            --exclude-dir=Vendor/glfw \
            --exclude-dir=Vendor/glad \
            'GLFW_|glfw|glad|#include[[:space:]]*<GLFW/' \
            main.c Engine build.c; then
            echo 'BGE still contains a direct GLFW/GLAD dependency' >&2
            exit 1
          fi
```

Keep it temporarily expected-failing until the later migration tasks remove the matches.

- [ ] **Step 2: Add CI installation of exact lwcgl branch**

Before `c build`, add:

```yaml
      - name: Install lwcgl v2.9.3
        shell: bash
        run: |
          set -euo pipefail
          clone_retry() {
            local url="$1" dest="$2"
            for attempt in 1 2 3; do
              rm -rf "$dest"
              if git clone --depth 1 --branch v2.9.3 "$url" "$dest"; then
                return 0
              fi
              sleep $((attempt * 2))
            done
            return 1
          }
          clone_retry https://github.com/xt9y/lwcgl.git /tmp/lwcgl
          make -C /tmp/lwcgl -j2
          sudo make -C /tmp/lwcgl install
```

This is CI setup only. Do not place any equivalent clone/install logic in `build.c`.

- [ ] **Step 3: Replace BGE's vendored platform build wiring**

Change `build.c` so it no longer calls `add_glfw`, no longer compiles `Vendor/glad/src/glad.c`, and no longer includes GLFW/GLAD include directories. Add:

```c
c_include(app, "/usr/local/include/lwcgl-2.9.3");
c_link_path(app, "/usr/local/lib");
c_link_system(app, "lwcgl");
```

Add the new BGE sources as they are introduced:

```c
c_sources(app, "Engine/input.c");
c_sources(app, "Engine/game.c");
c_sources(app, "Engine/imgui_impl_lwcgl.cpp");
```

Keep ImGui core and `imgui_impl_opengl3.cpp`; remove `imgui_impl_glfw.cpp`.

- [ ] **Step 4: Preserve platform linker requirements only where the installed static lwcgl needs them**

Retain platform libraries/frameworks required to resolve the installed lwcgl static library. Do not add BGE source-level GLFW includes or calls. On Linux retain the current GL/X11/pthread/dl/rt link set if required by lwcgl; on macOS retain the current OpenGL/Cocoa/IOKit/CoreFoundation/CoreVideo/QuartzCore frameworks if required by lwcgl.

- [ ] **Step 5: Document the local prerequisite**

Update README build notes to state:

```markdown
BGE uses the system-installed lwcgl v2.9.3 compatibility library.
Install https://github.com/xt9y/lwcgl/tree/v2.9.3 first, then build BGE normally:

```sh
c build run
```

`build.c` does not download or install lwcgl.
```

- [ ] **Step 6: Commit**

```sh
git add build.c .github/workflows/headless-ci.yml README.md
git commit -m "build: make lwcgl an external dependency"
```

---

### Task 2: Replace GLFW runtime ownership with lwcgl lifecycle

**Files:**
- Modify: `Engine/App.h`
- Modify: `Engine/App.c`
- Modify: `Engine/state.h`

**Interfaces:**
- Consumes: global lwcgl `Display`, `Keyboard`, `Mouse`, `Sys`; `lwcglSetContextVersion(int,int)`; `lwcglSetContextProfile(...)`.
- Produces: `int app_start(void)`, `int app_frame(void)`, `void app_end(void)`, `double app_get_fps(void)`; `state_t` without native window ownership.

- [ ] **Step 1: Replace the platform include contract in `App.h`**

Use:

```c
#pragma once
#include <lwcgl/lwcgl.h>
#include <lwcgl/context.h>

int app_start(void);
int app_frame(void);
void app_end(void);
double app_get_fps(void);
void app_request_exit(void);
```

Retain engine constants such as `TITLE`, `WIDTH`, `HEIGHT`, `RENDER_BASE_W`, and `PALETTE_LEVELS`.

- [ ] **Step 2: Remove native window state**

Delete `GLFWwindow* win` from `state_t`. Keep `cursor_locked`, `id`, `dt`, framebuffer data, camera, editor, text, gun, levels, vertices, and debug state.

- [ ] **Step 3: Implement explicit partial-init ownership flags**

In `App.c`, add internal flags:

```c
static bool g_display_created;
static bool g_keyboard_created;
static bool g_mouse_created;
static bool g_imgui_created;
```

Use these flags in `app_end()` so shutdown is safe after initialization failure.

- [ ] **Step 4: Create the OpenGL context through lwcgl**

Before `Display.create()`:

```c
lwcglSetContextVersion(3, 3);
lwcglSetContextProfile(LWCGL_CONTEXT_PROFILE_CORE);

Display.setDisplayMode(newDisplayMode(WIDTH, HEIGHT));
Display.setTitle(TITLE);
Display.setVSyncEnabled(false);
Display.create();
```

Use the exact `DisplayMode` constructor/helper exported by the installed lwcgl C header; do not access GLFW native handles.

- [ ] **Step 5: Create input devices**

After Display creation:

```c
Keyboard.create();
Mouse.create();
Mouse.setGrabbed(true);
```

If an lwcgl create call exposes an error return, validate it. Otherwise check `lwcglGetLastError()` after failed Display creation paths and report the error.

- [ ] **Step 6: Replace timing and frame processing**

Create:

```c
static double app_now_seconds(void)
{
    const double ticks = (double)Sys.getTime();
    const double resolution = (double)Sys.getTimerResolution();
    return resolution > 0.0 ? ticks / resolution : 0.0;
}
```

Each frame:

```c
Display.processMessages();
input_begin_frame();
```

Present exactly once:

```c
Display.updateNoMessages();
```

Exit when either `Display.isCloseRequested()` or `state.id == STATE_EXIT`.

- [ ] **Step 7: Replace callback-driven framebuffer sizing**

At frame start read:

```c
state.fb->w = Display.getWidth();
state.fb->h = Display.getHeight();
state.fb->ww = state.fb->w;
state.fb->wh = state.fb->h;
state.fb->scale = 1.0f;
```

No GLFW resize callback remains.

- [ ] **Step 8: Commit**

```sh
git add Engine/App.h Engine/App.c Engine/state.h
git commit -m "refactor: move application lifecycle to lwcgl"
```

---

### Task 3: Centralize input and convert camera to relative deltas

**Files:**
- Create: `Engine/input.h`
- Create: `Engine/input.c`
- Modify: `Engine/cam.h`
- Modify: `Engine/cam.c`

**Interfaces:**
- Produces:

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
```

- Camera interface:

```c
void camera_apply_mouse_delta(camera_t* cam, f32 dx, f32 dy);
```

- [ ] **Step 1: Write a deterministic edge-state unit contract**

Make `input.c` keep previous/current arrays and a test-only update helper under `#ifdef BGE_INPUT_TEST`:

```c
void input_test_set_key(i32 key, bool down);
```

Create `tests/input_edges.c` that verifies `down`, `pressed`, and `released` over three frames for one key.

- [ ] **Step 2: Run the input test before implementation**

Compile the small contract independently with the installed lwcgl headers. Expected result before implementation: missing input symbols/link failure.

- [ ] **Step 3: Implement key/mouse snapshots**

Snapshot the LWJGL 2.9.3 key range using `Keyboard.isKeyDown(key)` and mouse buttons using `Mouse.isButtonDown(button)` once per frame. Compute edge queries from current/previous arrays.

Read relative mouse motion once per frame:

```c
g_mouse_dx = (f32)Mouse.getDX();
g_mouse_dy = (f32)Mouse.getDY();
```

- [ ] **Step 4: Implement cursor grab through lwcgl**

```c
void input_set_grabbed(bool grabbed)
{
    g_grabbed = grabbed;
    Mouse.setGrabbed(grabbed);
}
```

- [ ] **Step 5: Replace absolute callback camera math**

`camera_t` no longer needs `lastX`, `lastY`, or `firstMouse`. Implement:

```c
void camera_apply_mouse_delta(camera_t* cam, f32 dx, f32 dy)
{
    if (!cam) return;
    const f32 sensitivity = 0.1f;
    cam->yaw += dx * sensitivity;
    cam->pitch -= dy * sensitivity;
    if (cam->pitch > 89.0f) cam->pitch = 89.0f;
    if (cam->pitch < -89.0f) cam->pitch = -89.0f;
    update_camera_vectors(cam);
}
```

- [ ] **Step 6: Run the edge-state contract**

Expected: all pressed/released/down assertions pass.

- [ ] **Step 7: Commit**

```sh
git add Engine/input.h Engine/input.c Engine/cam.h Engine/cam.c tests/input_edges.c
git commit -m "refactor: centralize lwcgl input handling"
```

---

### Task 4: Move game bootstrap/actions out of `main.c`

**Files:**
- Create: `Engine/game.h`
- Create: `Engine/game.c`
- Modify: `main.c`
- Modify: `Engine/App.c`

**Interfaces:**
- Produces:

```c
bool game_init(void);
void game_handle_input(void);
void game_update(void);
void game_render(void);
void game_shutdown(void);
```

- [ ] **Step 1: Move texture, weapon, level, editor, and camera bootstrap verbatim into `game_init()`**

Preserve the current texture list, weapon definitions, level loader order, editor-level pointer initialization, and `apply_level_camera()` call.

- [ ] **Step 2: Move input actions from `main.c::INPUT()` into `game_handle_input()`**

Replace direct GLFW polling with the Task 3 API. Examples:

```c
if (input_key_pressed(Keyboard.KEY_H))
    state.debug_visible = !state.debug_visible;

if (input_key_pressed(Keyboard.KEY_TAB)) {
    state.cursor_locked = !state.cursor_locked;
    input_set_grabbed(state.cursor_locked);
}

if (state.id == STATE_PLAYING && state.cursor_locked &&
    input_mouse_pressed(0)) {
    shoot_bullet();
    gun_shot();
}
```

Keep every existing binding and repeat timer, including H, TAB, mouse shoot, G, E, N, X/Delete/Backslash, R, 7/8/9, V, Enter, movement, editor movement, and shift-modified adjustment.

- [ ] **Step 3: Apply mouse delta only when cursor is locked**

```c
if (state.cursor_locked)
    camera_apply_mouse_delta(state.cam, input_mouse_dx(), input_mouse_dy());
```

- [ ] **Step 4: Preserve gameplay movement/portal/collision/height code byte-for-byte except input predicates**

Keep the same movement speed `18.5f * state.dt`, forward/right math, `portal_try_teleport`, `player_collide_quads`, `level_get_height`, and lerp behavior.

- [ ] **Step 5: Move current `RENDER()` body into `game_render()`**

Use `state.fb->w/h` instead of GLFW framebuffer-size queries. Preserve low-resolution aspect-derived render dimensions, `render_main`, `post_blit`, text crosshair, editor UI, and ImGui order.

- [ ] **Step 6: Move per-frame editor update to `game_update()`**

```c
state.editor->level = &state.levels[state.level_id];
if (state.id == STATE_EDITOR)
    editor_update();
```

- [ ] **Step 7: Move shutdown ownership into `game_shutdown()`**

Preserve:

```c
gun_shutdown();
render_shutdown();
editor_save(state.editor->level);
```

- [ ] **Step 8: Reduce `main.c` to entry only**

Target:

```c
#include "Engine/App.h"

int main(void)
{
    return app_run();
}
```

Expose `int app_run(void)` from `App.h`; it performs app start, game init, frame loop, game shutdown, and app end.

- [ ] **Step 9: Commit**

```sh
git add main.c Engine/App.c Engine/App.h Engine/game.c Engine/game.h
git commit -m "refactor: move game runtime behind engine entry"
```

---

### Task 5: Replace GLAD shader/render dispatch with lwcgl modern GL

**Files:**
- Modify: `Engine/gfx.h`
- Modify: `Engine/gfx.c`
- Modify: `Engine/text.c`
- Modify: `Engine/level.c`
- Modify: `Engine/render.c`
- Modify: `Engine/editor.c`
- Modify: `Engine/gun.c`
- Modify: `Engine/App.c`

**Interfaces:**
- Consumes: `<lwcgl/lwcgl.h>` and `<lwcgl/glmodern.h>`.
- Produces: identical renderer semantics with no `glad` includes/calls.

- [ ] **Step 1: Replace GLAD headers**

Use:

```c
#include <lwcgl/lwcgl.h>
#include <lwcgl/glmodern.h>
```

Remove every `<glad/glad.h>` include.

- [ ] **Step 2: Convert shader/program functions in `gfx.c`**

Map modern calls directly:

```c
GL20.glCreateShader(...)
GL20.glShaderSource(...)
GL20.glCompileShader(...)
GL20.glGetShaderiv(...)
GL20.glGetShaderInfoLog(...)
GL20.glCreateProgram()
GL20.glAttachShader(...)
GL20.glLinkProgram(...)
GL20.glGetProgramiv(...)
GL20.glGetProgramInfoLog(...)
GL20.glDeleteShader(...)
```

Preserve existing compile/link error messages and failure behavior.

- [ ] **Step 3: Convert buffer/VAO calls**

Use lwcgl modern dispatch consistently:

```c
GL15.glGenBuffers(...)
GL15.glBindBuffer(...)
GL15.glBufferData(...)
GL15.glDeleteBuffers(...)
GL30.glGenVertexArrays(...)
GL30.glBindVertexArray(...)
GL30.glDeleteVertexArrays(...)
GL20.glEnableVertexAttribArray(...)
GL20.glVertexAttribPointer(...)
```

- [ ] **Step 4: Convert program/uniform calls**

Use `GL20` for `glUseProgram`, uniform lookup/uploads, and vertex attributes in App/text/level/render/editor/gun.

- [ ] **Step 5: Keep core fixed/base operations through lwcgl GL compatibility surface**

For state/texture/draw calls that are provided directly by lwcgl compatibility headers, keep the exact behavior: depth/stencil enable, blending, texture upload/filter/wrap, viewport, clear, stencil mask/function/op, draw calls, and pixel store/read buffer.

- [ ] **Step 6: Run source search**

Expected after this task:

```sh
! grep -RIn --exclude-dir=Vendor --exclude-dir=docs 'glad' main.c Engine build.c
```

- [ ] **Step 7: Commit**

```sh
git add Engine/gfx.h Engine/gfx.c Engine/text.c Engine/level.c Engine/render.c Engine/editor.c Engine/gun.c Engine/App.c
git commit -m "refactor: route rendering through lwcgl OpenGL"
```

---

### Task 6: Replace framebuffer renderbuffer and migrate RendererCheck queries

**Files:**
- Modify: `Engine/App.c`

**Interfaces:**
- Produces: texture-backed `GL_DEPTH24_STENCIL8` attachment; timestamp-pair GPU metric.

- [ ] **Step 1: Replace depth/stencil renderbuffer with texture**

Use one texture object:

```c
GL11.glGenTextures(1, &g_fbo_depth_stencil);
GL11.glBindTexture(GL_TEXTURE_2D, g_fbo_depth_stencil);
GL11.glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8,
                  w, h, 0, GL_DEPTH_STENCIL,
                  GL_UNSIGNED_INT_24_8, NULL);
GL11.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
GL11.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
GL30.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_TEXTURE_2D, g_fbo_depth_stencil, 0);
```

Delete it with the texture deletion API during resize/shutdown.

- [ ] **Step 2: Add framebuffer completeness validation**

After attachments:

```c
const u32 status = GL30.glCheckFramebufferStatus(GL_FRAMEBUFFER);
if (status != GL_FRAMEBUFFER_COMPLETE) {
    fprintf(stderr, "BGE: framebuffer incomplete: 0x%x\n", status);
    state.id = STATE_EXIT;
}
```

- [ ] **Step 3: Replace one elapsed query with two timestamp queries**

Store:

```c
static u32 g_rendercheck_gpu_queries[2];
```

Begin:

```c
GL33.glQueryCounter(g_rendercheck_gpu_queries[0], GL_TIMESTAMP);
```

End:

```c
GL33.glQueryCounter(g_rendercheck_gpu_queries[1], GL_TIMESTAMP);
GLuint64 start_ns = 0, end_ns = 0;
GL33.glGetQueryObjectui64v(g_rendercheck_gpu_queries[0], GL_QUERY_RESULT, &start_ns);
GL33.glGetQueryObjectui64v(g_rendercheck_gpu_queries[1], GL_QUERY_RESULT, &end_ns);
if (end_ns >= start_ns)
    rendercheck_gpu_ms((double)(end_ns - start_ns) / 1000000.0);
```

- [ ] **Step 4: Capture with Display dimensions**

Remove the GLFW window parameter from `rendercheck_capture_frame`; use `Display.getWidth()` / `Display.getHeight()`.

- [ ] **Step 5: Preserve deterministic RendererCheck stepping**

Keep `state.dt = 1.0f / 60.0f` when RendererCheck is active and preserve `rendercheck_frame_is_last` behavior.

- [ ] **Step 6: Commit**

```sh
git add Engine/App.c
git commit -m "refactor: migrate framebuffer and gpu timing to lwcgl"
```

---

### Task 7: Replace Dear ImGui GLFW backend with lwcgl backend

**Files:**
- Create: `Engine/imgui_impl_lwcgl.h`
- Create: `Engine/imgui_impl_lwcgl.cpp`
- Modify: `Engine/imgui_c.h`
- Modify: `Engine/imgui_c.cpp`
- Modify: `build.c`

**Interfaces:**
- Produces:

```cpp
bool ImGui_ImplLwcgl_Init();
void ImGui_ImplLwcgl_Shutdown();
void ImGui_ImplLwcgl_NewFrame();
void ImGui_ImplLwcgl_SetMouseEnabled(bool enabled);
```

- [ ] **Step 1: Implement backend lifecycle without native window access**

`ImGui_ImplLwcgl_Init()` sets `ImGuiBackendFlags` only for capabilities actually implemented and initializes backend time state.

- [ ] **Step 2: Feed display and delta-time each frame**

```cpp
ImGuiIO& io = ImGui::GetIO();
io.DisplaySize = ImVec2((float)Display.getWidth(), (float)Display.getHeight());
io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
io.DeltaTime = state.dt > 0.0f ? state.dt : (1.0f / 60.0f);
```

Avoid a dependency from the backend on broad engine state if possible: pass delta-time through a setter or backend frame parameter rather than including `state.h`.

- [ ] **Step 3: Feed keyboard state with modern ImGui key events**

Map each binding BGE/editor needs to `ImGuiKey_*` and call `io.AddKeyEvent(...)`. At minimum include letters A-Z used by BGE, digits, Enter, Escape, Tab, Delete, Backslash, Left/Right Shift, Ctrl, Alt, arrows, Home/End/PageUp/PageDown, Space, and editing keys used by ImGui widgets.

- [ ] **Step 4: Feed mouse state**

Use lwcgl mouse button states and wheel events. When ungrabbed, use the absolute mouse coordinates exposed by lwcgl; if the current `v2.9.3` API lacks an absolute mouse-position getter required by ImGui, add that capability to lwcgl `v2.9.3` first with its own test rather than calling GLFW from BGE.

- [ ] **Step 5: Update `imgui_c` facade**

Change:

```c
void imgui_init(void);
```

Remove all `GLFWwindow*` parameters and `g_window`. Initialization becomes:

```cpp
ImGui_ImplLwcgl_Init();
ImGui_ImplOpenGL3_Init("#version 330 core");
```

Frame order becomes:

```cpp
ImGui_ImplOpenGL3_NewFrame();
ImGui_ImplLwcgl_NewFrame();
ImGui::NewFrame();
```

Shutdown reverses those calls.

- [ ] **Step 6: Remove `imgui_impl_glfw` from build wiring**

Keep only ImGui core, `imgui_impl_opengl3.cpp`, and BGE's new lwcgl platform backend.

- [ ] **Step 7: Commit**

```sh
git add Engine/imgui_impl_lwcgl.h Engine/imgui_impl_lwcgl.cpp Engine/imgui_c.h Engine/imgui_c.cpp build.c
git commit -m "refactor: add lwcgl Dear ImGui backend"
```

---

### Task 8: Remove all remaining GLFW/GLAD platform leakage and dead vendor code

**Files:**
- Modify: any remaining `Engine/*.c`, `Engine/*.h`, `main.c`, `build.c`
- Delete: `Vendor/glfw/**`
- Delete: `Vendor/glad/**`
- Delete: `Vendor/imgui/imgui_impl_glfw.cpp`
- Delete: `Vendor/imgui/imgui_impl_glfw.h`
- Delete: `Engine/CMakeLists.txt` if it is only the unsupported old GLFW build path.

**Interfaces:**
- Produces: source tree where BGE code/build is platform-clean and all platform access routes through lwcgl.

- [ ] **Step 1: Run exact leakage search**

```sh
grep -RInE \
  --exclude-dir=.git \
  --exclude-dir=docs \
  --exclude-dir=Vendor/glfw \
  --exclude-dir=Vendor/glad \
  'GLFW_|glfw|glad|#include[[:space:]]*<GLFW/' \
  main.c Engine build.c
```

Expected: no output.

- [ ] **Step 2: Fix each remaining result by using the existing lwcgl/input/App interface**

Do not add a generic wrapper that mirrors GLFW or GLAD. Do not call `Display.getNativeWindow()` from BGE.

- [ ] **Step 3: Delete the dead vendor trees/backends**

Remove GLFW, GLAD, and ImGui GLFW backend only after the source search is clean.

- [ ] **Step 4: Remove unsupported CMake path**

If `Engine/CMakeLists.txt` still exclusively configures vendored GLFW/GLAD and is not used by the supported C-BuildSystem workflow, delete it rather than maintaining a second build architecture.

- [ ] **Step 5: Re-run the leakage search including Vendor except documentation**

Expected: no GLFW/GLAD implementation remains in BGE.

- [ ] **Step 6: Commit**

```sh
git add -A
git commit -m "cleanup: remove vendored glfw and glad stack"
```

---

### Task 9: Full functional and headless regression validation

**Files:**
- Modify only if a regression requires a behavior-preserving fix.
- Validate: `.github/workflows/headless-ci.yml`, `rendercheck.toml`.

**Interfaces:**
- Consumes: completed lwcgl migration.
- Produces: evidence that existing BGE behavior remains intact.

- [ ] **Step 1: Build locally with already-installed dependencies**

```sh
c build
```

Expected: `build/debug/bge` exists and link succeeds without BGE compiling GLFW/GLAD.

- [ ] **Step 2: Run the source contract**

Expected: no prohibited direct dependency matches.

- [ ] **Step 3: Run headless boot smoke test**

```sh
xvfb-run -a env LIBGL_ALWAYS_SOFTWARE=1 \
  timeout --signal=TERM --kill-after=2s 5s ./build/debug/bge
```

Accept exit codes `0`, `124`, or `143` as in existing CI.

- [ ] **Step 4: Run RendererCheck runtime**

```sh
renderercheck run runtime
```

Verify:

```sh
test -s .rendercheck/runtime/metrics.txt
grep -q '^gpu_ms=' .rendercheck/runtime/metrics.txt
grep -q '"renderer_mode": "software"' .rendercheck/results.json
grep -q '"timing_kind": "software_render"' .rendercheck/results.json
```

- [ ] **Step 5: Run deterministic visual baseline flow**

Use the existing CI sequence: remove baseline, require first visual run to fail as missing baseline, approve, run again, diff, invert baseline to force regression, require failure and PNG diff, re-approve, rerun.

- [ ] **Step 6: Run full RendererCheck suite**

```sh
renderercheck run
```

Verify `.rendercheck/report.md`, `.rendercheck/results.json`, runtime stdout, and visual PNG output.

- [ ] **Step 7: Manual behavior matrix**

Run BGE interactively and verify:

```text
launch/exit
resize
mouse grab/ungrab
camera look
play movement
editor movement
portal rendering + teleport
collision
height following
editor UI input
editor add/delete/reset/paint/transform/save
all current levels
textures
low-resolution target
palette post-process
text/crosshair
weapon switch/shoot/animation/muzzle flash
debug bullet visualization
stencil-dependent behavior
```

Any failure must be fixed without removing the feature or changing level data semantics.

- [ ] **Step 8: Verify CI on the final commit**

Run/inspect `BGE Headless CI`. Require all build, smoke, RendererCheck, visual-regression, performance-policy, and artifact steps to pass.

- [ ] **Step 9: Final commit for regression-only fixes if needed**

```sh
git add -A
git commit -m "fix: preserve bge behavior after lwcgl rebuild"
```

---

## Final Acceptance

The implementation is complete only if all of the following are simultaneously true:

1. `build.c` contains no download/clone/install/update logic for lwcgl.
2. A local user installs lwcgl `v2.9.3` separately and then runs the normal BGE `c build run` flow.
3. CI installs lwcgl `v2.9.3` separately before BGE build.
4. BGE code/build contains no direct GLFW or GLAD dependency.
5. BGE no longer vendors GLFW or GLAD.
6. `main.c` is only the application entry point.
7. The BGE sector/portal raster renderer remains BGE-owned.
8. The editor, levels, portals, collision, weapons, text, post-processing, low-resolution rendering, palette effect, and RendererCheck behavior remain functional.
9. Full headless CI passes.
