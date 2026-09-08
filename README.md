#### BGE - Basic Game Engine (cool name right)
- last showcase of engine: `https://youtu.be/8IT8n9sierU?si=2kPJQaqm_F3vs9J3`

#### THIS IS IT:

![IMG](./Engine/res/GAME2.png)

![IMG](./Engine/res/GAME1.png)

![IMG](./Engine/res/GAME3.png)

#### Dependencies

BGE uses the system-installed `lwcgl` v2.9.3 runtime. `build.c` does not download or install dependencies.

Install lwcgl separately:

```sh
git clone --depth 1 --branch v2.9.3 https://github.com/xt9y/lwcgl.git
cd lwcgl
make
sudo make install
```

The default install provides headers under `/usr/local/include/lwcgl-2.9.3` and the library under `/usr/local/lib`.

On Linux, lwcgl itself requires the normal OpenGL/GLFW development packages. BGE does not include or use GLFW/GLAD directly.

#### C build system

Build and run with [xt9y/C](https://github.com/xt9y/C-BuildSystem):

```sh
c build run
```

#### RendererCheck integration

With [xt9y/RendererCheck](https://github.com/xt9y/RendererCheck) installed:

```sh
rendercheck run
```

CI installs lwcgl v2.9.3 as a separate setup step before building BGE.
