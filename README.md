# 3D Shooter

A minimal but solid 3D shooter base built with **SDL2**, **OpenGL 3.3 Core Profile**, and **GLM**. Designed to feel snappy (ULTRAKILL-style movement) and be easy to build on top of.

## Controls

| Key | Action |
|-----|--------|
| W A S D | Move |
| Mouse | Look around |
| Space | Jump |
| Escape | Quit |

## Building

### Requirements (macOS)

```sh
brew install sdl2 glm llvm
```

### Build & run

```sh
make        # compile
./shooter   # run (must be run from project root — shaders are loaded from src/)
make run    # compile + run in one step
make clean  # delete the binary
```

> **Note:** The system clang on macOS ships without C++ stdlib headers. The Makefile uses Homebrew LLVM instead. If you move the project, update the `LLVM` and `SDK` paths at the top of `Makefile`.

## Project structure

```
3d_shooter/
├── src/
│   ├── main.cpp          # game loop, world definition, render pass
│   ├── Camera.h          # view/projection matrices, mouselook
│   ├── Player.h          # physics, input, AABB collision
│   ├── Mesh.h            # VAO/VBO wrapper — one draw call per mesh
│   ├── ShaderProgram.h   # GLSL compile/link, uniform helpers
│   ├── shader.vert       # vertex shader — MVP transform
│   └── shader.frag       # fragment shader — texture + diffuse lighting
├── Makefile
└── README.md
```

## How the engine works

### Game loop (fixed timestep)

Physics always runs at exactly **60 Hz** regardless of framerate. Real elapsed time is accumulated and drained in fixed 1/60s steps. Any leftover time is used as an **interpolation factor** for rendering, so movement looks smooth at 144 Hz+ without changing the physics.

```
accumulator += elapsed_time
while accumulator >= PHYSICS_DT:
    physics.update(PHYSICS_DT)
    accumulator -= PHYSICS_DT
alpha = accumulator / PHYSICS_DT   # 0..1, used to lerp camera position
```

Reference: [Fix Your Timestep — Gaffer on Games](https://gafferongames.com/post/fix_your_timestep/)

### Movement (Quake-style)

Instead of `velocity = direction * speed`, acceleration is applied incrementally:

```
currentSpeed = dot(velocity_horizontal, wishDir)
addSpeed     = clamp(maxSpeed - currentSpeed, 0, accel * dt)
velocity    += wishDir * addSpeed
```

This means you keep your momentum mid-air, and strafe-jumping (changing direction while airborne) can gain a tiny speed boost — the same mechanic that makes Quake/Titanfall/ULTRAKILL movement feel good.

### Rendering pipeline

Every frame:
1. `shader.use()` — activate the GLSL program
2. Set uniforms: `model`, `view`, `projection` matrices + light values
3. Bind texture to slot 0
4. `worldMesh.draw()` — one `glDrawElements` call for the whole world

The entire world (floor, ceiling, all walls) is batched into **one Mesh** and drawn in a single call. Keeping draw calls low is the primary performance lever for a simple renderer.

### Collision

The player is an AABB (box). Each wall/platform is also an AABB. Collision is resolved by finding the axis of minimum penetration and pushing the player out along it. This is fast and handles stacking (standing on top of boxes) correctly.

## How to extend

### Add a new wall / platform

In `main.cpp`, add an entry to the `walls` array:

```cpp
{ AABB{{ minX, minY, minZ }, { maxX, maxY, maxZ }} },
```

It will automatically appear in the world visually and be solid for collision.

### Add a texture

Swap `makeWhiteTexture()` for a real image load (e.g., with [stb_image](https://github.com/nothings/stb)):

```cpp
// Load image from disk, upload to GPU the same way makeWhiteTexture does
GLuint myTex = loadTexture("assets/wall.png");
glBindTexture(GL_TEXTURE_2D, myTex);
```

### Add a new renderable object (e.g., enemy, prop)

1. Build vertices/indices and call `mesh.upload()`
2. Set `shader.setMat4("model", transform)` before drawing
3. Call `mesh.draw()`

### Add a new shader effect

Edit `src/shader.frag`. Ideas:
- **Fog**: `mix(FragColor, fogColor, clamp(distance/fogEnd, 0, 1))`
- **Specular**: Blinn-Phong with a `uniform vec3 cameraPos` and halfway vector
- **Vertex colour**: add a `vec3 color` to `Vertex` struct, pass through as `layout(location=3)`

### Change movement feel

All tuning knobs are public members of `Player` in `Player.h`:

| Variable | Default | Effect |
|----------|---------|--------|
| `horizontalSpeed` | 7.0 | Max ground speed |
| `gravity` | -24.0 | Fall speed |
| `jumpForce` | 8.5 | Jump height |
| `acceleration` | 80.0 | Ground responsiveness |
| `airAcceleration` | 12.0 | Mid-air steering |
| `friction` | 10.0 | Ground deceleration |

## Dependencies

| Library | Purpose |
|---------|---------|
| SDL2 | Window, input, OpenGL context |
| OpenGL 3.3 | Rendering (via macOS OpenGL.framework) |
| GLM | Math — vectors, matrices, transforms |
| LLVM/clang++ | Compiler (Homebrew) — system clang has broken C++ stdlib on this macOS version |
