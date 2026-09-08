# BGE lwcgl v2.9.3 Engine Rebuild Design

## Goal

Rebuild BGE around `xt9y/lwcgl` branch `v2.9.3` as the only window, input, timing, context, and OpenGL loading dependency exposed to BGE.

The rebuild must remove BGE's direct GLFW and GLAD integration without losing runtime or editor functionality. Existing sector/portal rendering, levels, collision, weapons, editor, low-resolution rendering, palette/post processing, text, RendererCheck support, and current gameplay behavior remain functional.

Horse is not part of this migration. Horse may be integrated later for generic ECS/model/animation functionality, but this rebuild must not force BGE's sector/portal architecture through Horse or reintroduce Horse's removed rasterizer.

## Non-goals

- Do not replace BGE's sector/portal renderer with Horse.
- Do not introduce a path tracer.
- Do not redesign levels or change saved level data semantics.
- Do not remove the editor, portal system, weapons, collision, text, post-processing, or RendererCheck integration.
- Do not keep a second GLFW/GL loader path as a fallback.
- Do not vendor another window/input library into BGE.
- Do not add a new general-purpose platform abstraction that merely mirrors lwcgl one-for-one.

## Resulting dependency boundary

```text
BGE
├── application/runtime lifecycle
├── gameplay + editor
├── sector/portal/collision systems
├── BGE raster renderer + post process
├── text + weapon rendering
├── ImGui lwcgl platform backend
└── lwcgl v2.9.3
    ├── Display
    ├── Keyboard
    ├── Mouse
    ├── Sys
    ├── context selection
    └── GL11 compatibility + GL15/20/30/31/32/33/42/43 modern API
```

BGE source files must contain no GLFW calls, GLFW key constants, GLFW window types, GLAD includes, GLAD loader calls, or vendored GLFW/GLAD build sources after the migration.

## Repository cleanup

Remove the obsolete duplicated platform stack:

- `Vendor/glfw/**`
- `Vendor/glad/**`
- `imgui_impl_glfw.cpp/.h`
- GLFW/GLAD source and include wiring from `build.c`
- direct GLFW/X11/Cocoa link plumbing that exists only because BGE currently builds GLFW itself
- `Engine/CMakeLists.txt`, because the repository's supported build path is C-BuildSystem and the CMake file duplicates the obsolete GLFW integration

Keep ImGui and any other vendor code still used by BGE.

The supported build remains the repository's C-BuildSystem `build.c` flow.

## Build integration

`build.c` must:

- include the installed lwcgl v2.9.3 headers from `/usr/local/include/lwcgl-2.9.3`;
- link `/usr/local/lib` and `-llwcgl`;
- keep only the platform system libraries required by lwcgl/OpenGL and stop compiling BGE's own GLFW and GLAD copies;
- retain C/C++ linkage needed by ImGui;
- keep strict warnings enabled;
- keep the executable target named `bge`.

CI must install `xt9y/lwcgl` from branch `v2.9.3` before building BGE so the dependency version is explicit and reproducible.

## Runtime lifecycle

Replace the current GLFW lifecycle in `Engine/App.c` with lwcgl:

1. request the required OpenGL context through `lwcglSetContextVersion` / `lwcglSetContextProfile`;
2. configure `DisplayMode`, title, resize policy, and VSync;
3. call `Display.create()`;
4. create `Keyboard` and `Mouse`;
5. initialize renderer, text, ImGui, framebuffer/post-process resources, and game state;
6. each frame, process Display messages and poll Keyboard/Mouse;
7. run input/update/render;
8. present with `Display.updateNoMessages()` exactly once per frame;
9. shutdown in reverse ownership order;
10. destroy Mouse, Keyboard, and Display.

Use `Sys.getTime()` / `Sys.getTimerResolution()` for engine timing rather than `glfwGetTime()`.

Do not access the native GLFW window from BGE runtime code.

## Application structure cleanup

The current `GL_START`, `GL_FRAME`, `GL_END`, `RUN`, `INPUT`, and `RENDER` split mixes platform, gameplay, and rendering concerns. The rebuild should keep behavior but make ownership explicit.

Target structure:

- `main.c`: only enters the engine/runtime.
- `Engine/App.c/.h`: application lifecycle, frame timing, subsystem ownership, and top-level frame ordering.
- `Engine/input.c/.h`: engine/game input state derived from lwcgl Keyboard/Mouse, including pressed/released edge detection used by editor/gameplay actions.
- existing renderer/editor/level/portal/gun/text modules retain their domain responsibilities.

Do not create wrappers for every lwcgl function. The input module exists to centralize BGE action/edge semantics, not to reproduce the lwcgl API.

## Input migration

Replace GLFW polling everywhere:

- `GLFW_KEY_*` -> `Keyboard.KEY_*`
- `glfwGetKey` -> `Keyboard.isKeyDown`
- `glfwGetMouseButton` -> `Mouse.isButtonDown`
- cursor lock -> `Mouse.setGrabbed`
- mouse look -> relative `Mouse.getDX()` / `Mouse.getDY()`
- window close -> `Display.isCloseRequested()`

Centralize one-shot action edges such as editor toggle, weapon switch, delete/reset, paint mode, level controls, and debug toggles so individual systems do not maintain unrelated static GLFW edge flags.

Preserve existing key bindings and input behavior unless an exact GLFW key has no lwcgl equivalent. Any such mismatch must be mapped to the corresponding LWJGL 2.9.3 key constant, not silently dropped.

## Window and framebuffer sizing

Use `Display.getWidth()` and `Display.getHeight()` for framebuffer dimensions.

Preserve BGE's low-resolution render target behavior:

- base render width remains `RENDER_BASE_W`;
- render height remains aspect-derived;
- the scene renders into the low-resolution framebuffer;
- the existing palette quantization/post pass remains;
- the result is scaled to the actual framebuffer;
- editor/text UI remains at display resolution as it is today.

The resize path must update framebuffer/post-process resources without requiring GLFW callbacks.

## OpenGL loader migration

GLAD is removed completely.

Use lwcgl's OpenGL surface:

- fixed/base GL calls from lwcgl's GL11 compatibility surface where appropriate;
- `GL15`, `GL20`, `GL30`, `GL31`, `GL32`, `GL33`, `GL42`, and `GL43` for modern function pointers;
- `lwcglLoadModernGL()` is owned by lwcgl Display creation and must not be duplicated by BGE.

BGE must fail initialization with a useful error if a required OpenGL feature is unavailable.

## Shader and mesh rendering

Preserve the existing shader behavior and visual output.

Convert GLAD-routed calls such as shader compilation/linking, uniforms, VBO/VAO management, framebuffer management, and queries to their matching lwcgl modern API calls.

Do not convert the renderer to Horse or fixed-function rendering as part of this rebuild.

## Framebuffer depth/stencil cleanup

lwcgl's current modern API does not expose the renderbuffer functions BGE currently uses for its depth/stencil attachment. Avoid extending lwcgl solely for this.

Replace BGE's depth/stencil renderbuffer with a depth/stencil texture attached to the framebuffer through the existing texture + `GL30.glFramebufferTexture2D` path.

Requirements:

- preserve depth testing;
- preserve stencil testing and portal/editor behavior that depends on stencil;
- preserve framebuffer completeness validation;
- recreate the attachment on resize.

## RendererCheck timing

Preserve RendererCheck capture, deterministic headless stepping, metrics, and visual regression behavior.

BGE currently uses `glBeginQuery(GL_TIME_ELAPSED)` / `glEndQuery`, while lwcgl exposes timestamp queries through `GL33.glQueryCounter` and `GL33.glGetQueryObjectui64v`.

Migrate GPU timing to a start/end timestamp pair:

1. issue start timestamp;
2. render the measured frame;
3. issue end timestamp;
4. read both query results;
5. report `(end - start)` in milliseconds through RendererCheck.

Delete the query objects during shutdown.

Headless RendererCheck frame timing must remain deterministic at 60 Hz.

## ImGui backend

Remove `imgui_impl_glfw`.

Add a small BGE-owned `imgui_impl_lwcgl.cpp/.h` platform backend that feeds Dear ImGui from lwcgl:

- display size/framebuffer scale;
- frame delta time;
- keyboard state and modifier keys;
- mouse position/buttons/wheel;
- cursor/grab interaction needed by BGE's editor.

Keep `imgui_impl_opengl3` as the renderer backend. Initialize it with the GLSL version matching BGE's lwcgl-created context. If the vendored ImGui OpenGL3 backend's loader configuration conflicts with lwcgl, configure that backend to use the already-loaded OpenGL symbols without adding GLAD, GLEW, GLFW, or another loader.

The custom platform backend must not call GLFW through `Display.getNativeWindow()`.

## State cleanup

Remove `GLFWwindow *` and any GLFW-specific types from `state_t`, `App.h`, camera headers, and ImGui bridge headers.

State should contain BGE domain/runtime state only: framebuffer dimensions, timing, camera/game/editor data, render resources, level state, input state, and subsystem ownership.

Do not store a native window handle unless a real BGE feature requires it.

## Camera

Replace GLFW cursor callbacks with lwcgl relative mouse deltas.

Preserve:

- current yaw/pitch behavior;
- sensitivity;
- first-frame mouse stabilization;
- cursor lock toggle;
- playing/editor movement differences;
- level camera initialization;
- collision and portal teleport behavior after movement.

## Editor

Preserve the complete editor feature set and UI:

- editor/play toggle;
- cursor/UI interaction;
- object/quad selection;
- add/delete/reset operations;
- transform editing;
- paint mode;
- level save behavior;
- all current ImGui windows and controls;
- current key-repeat behavior for adjustment operations.

Input implementation may change, behavior may not disappear.

## Levels, portals, and collision

Do not change level serialization or generated level headers as part of the lwcgl migration.

Preserve:

- all currently registered levels;
- sector and quad data;
- portal rendering/teleport behavior;
- height lookup;
- player collision;
- editor save output;
- texture/material assignments.

Refactors are allowed only when they make ownership or safety clearer without changing the external data format.

## Weapons and overlays

Preserve:

- all current weapon definitions;
- weapon switching;
- shooting;
- animation timing;
- muzzle flash/overlay rendering;
- debug bullet visualization;
- crosshair/text rendering.

Replace only the platform/input/time calls required by the migration.

## Error handling

Initialization failures must be explicit and leave no partially owned resources behind.

At minimum, report failures for:

- Display creation;
- Keyboard/Mouse creation;
- missing required modern OpenGL functions;
- shader compilation/linking;
- framebuffer incompleteness;
- allocation failures;
- ImGui backend initialization.

Use `lwcglGetLastError()` where it provides relevant platform/context detail.

Shutdown functions must be safe after partial initialization.

## Ownership and shutdown order

Each subsystem owns and destroys the resources it creates.

Top-level shutdown order should be deterministic:

1. gameplay/editor-specific transient resources;
2. renderer/post-process resources;
3. text/textures;
4. ImGui renderer/platform backends and context;
5. RendererCheck GPU queries;
6. Mouse;
7. Keyboard;
8. Display.

No resource cleanup should depend on GLFW being globally alive after Display destruction.

## CI and regression requirements

The existing headless CI behavior is part of the functionality contract and must remain green.

Update `.github/workflows/headless-ci.yml` to install lwcgl `v2.9.3` before BGE.

The final CI must validate:

- full C-BuildSystem compile/link;
- no direct BGE GLFW/GLAD references;
- headless software-OpenGL boot under Xvfb;
- RendererCheck runtime metrics;
- RendererCheck software-renderer classification;
- deterministic visual baseline creation/approve/run/diff flow;
- visual regression failure and PNG diff artifact generation;
- process performance failure policy;
- complete RendererCheck report/results artifacts.

Add a source-contract check that fails if non-vendored BGE code contains any of:

- `#include <GLFW/`
- `glfw`
- `glad`
- `GLFW_`

The only tolerated occurrence of the word GLFW should be inside documentation explaining that lwcgl may use it internally; BGE code/build wiring must not depend on it.

## Functional acceptance checklist

The rebuild is complete only when all of the following work after the migration:

- application launches and exits cleanly;
- resize works;
- uncapped/VSync behavior matches current settings;
- mouse grab/ungrab works;
- camera look works;
- playing movement works;
- editor movement works;
- portal teleport works;
- collision works;
- level height following works;
- editor opens and receives mouse/keyboard input correctly;
- editor add/delete/reset/paint/transform/save operations work;
- all existing levels load;
- textures render;
- low-resolution scene target renders;
- palette/post-processing renders;
- text/crosshair renders;
- weapons render, switch, animate, and shoot;
- debug visualization works;
- stencil-dependent behavior remains correct;
- RendererCheck frame capture works;
- RendererCheck GPU metric works;
- full headless CI passes.

## Migration strategy

Perform the rebuild in dependency order so the repository never needs two long-lived platform stacks:

1. add lwcgl build/CI dependency and compile contract;
2. introduce lwcgl runtime lifecycle and timing;
3. migrate input/camera;
4. migrate modern OpenGL calls away from GLAD;
5. replace depth/stencil renderbuffer path;
6. migrate RendererCheck GPU timing;
7. replace ImGui GLFW platform backend;
8. migrate remaining editor/gun/render GLFW usages;
9. remove GLFW/GLAD vendor trees and stale build wiring;
10. run full headless/visual/performance validation;
11. only then perform small ownership/name cleanup that is proven behavior-preserving.

No permanent compatibility shim should remain after step 9.

## Final architecture rule

After completion, BGE depends on lwcgl v2.9.3, not on GLFW or GLAD.

If BGE needs a platform/input/context feature that lwcgl genuinely does not expose and the feature belongs at the LWJGL compatibility layer, implement that capability in `xt9y/lwcgl` branch `v2.9.3` with its own tests first. Do not bypass lwcgl from BGE with direct GLFW calls.
