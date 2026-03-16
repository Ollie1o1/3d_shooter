# 3D Shooter

A 3D arena shooter built with **SDL2**, **OpenGL 3.3 Core Profile**, and **GLM**. ULTRAKILL-inspired movement with grapple hook, dashing, multi-weapon combat, style scoring, and bloom post-processing.

Builds and runs on **macOS** and **Windows** (via MSYS2). Linux support is straightforward but untested.

---

## Requirements

### macOS

```sh
brew install sdl2 sdl2_mixer glm llvm
```

> The Makefile uses Homebrew LLVM instead of system clang because Apple's system clang ships without C++ stdlib headers. If you've changed your Homebrew prefix or macOS SDK version, update `LLVM` and `SDK` at the top of `Makefile`.

### Windows (MSYS2)

1. Install [MSYS2](https://www.msys2.org/) if you haven't already.
2. Open the **MSYS2 UCRT64** shell and run:

```sh
pacman -S mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-SDL2 \
          mingw-w64-ucrt-x86_64-SDL2_mixer \
          mingw-w64-ucrt-x86_64-glm \
          mingw-w64-ucrt-x86_64-glew \
          make
```

---

## Build & Run

All commands must be run from the **project root** directory — shaders are loaded relative to `src/`.

### macOS

```sh
make        # compile → ./shooter
make run    # compile + run
make clean  # delete binary
```

### Windows

Open the **MSYS2 UCRT64** shell, navigate to the project folder, then:

```sh
make              # compile → shooter.exe
make run          # compile + run
make clean        # delete binary
```

> **Important:** use the MSYS2 UCRT64 shell (not PowerShell or cmd). The Makefile auto-detects the platform.

---

## Controls

| Input | Action |
|-------|--------|
| W A S D | Move |
| Mouse | Look |
| Space | Jump / Double jump |
| Left Shift | Dash (directional) |
| Left Mouse | Fire revolver |
| Right Mouse | Fire shotgun |
| 1 / 2 | Switch weapon |
| R | Reload |
| F | Parry |
| E | Interact |
| Left Ctrl / C | Crouch / Ground slam |
| Escape | Quit to menu |

---

## Project Structure

```
3d_shooter/
├── src/
│   ├── main.cpp            # entry point, SDL/OpenGL init, game loop
│   ├── GameState.h         # base state interface (menu / gameplay)
│   ├── MenuState.h         # main menu
│   ├── GameplayState.h     # core game loop: physics, combat, rendering
│   ├── Player.h            # kinematic character controller (Quake-style)
│   ├── Camera.h            # view/projection, mouselook
│   ├── Enemy.h             # enemy types and AI
│   ├── Projectile.h        # bullet/projectile system
│   ├── GrappleHook.h       # grapple hook physics
│   ├── StyleSystem.h       # ULTRAKILL-style rank/score tracking
│   ├── Level.h             # map geometry (AABB walls)
│   ├── Mesh.h              # VAO/VBO wrapper
│   ├── ShaderProgram.h     # GLSL compile/link, uniform helpers
│   ├── PostProcess.h       # bloom post-processing pass
│   ├── UIRenderer.h        # HUD (crosshair, ammo, style rank)
│   ├── ViewModel.h         # first-person weapon model
│   ├── AudioSystem.h       # SDL2_mixer sound wrapper
│   ├── Interactable.h      # trigger volumes / interactable objects
│   ├── shader.vert/frag    # world geometry shader
│   ├── skybox.vert/frag    # skybox shader
│   ├── ui.vert/frag        # HUD shader
│   ├── tracer.vert/frag    # bullet tracer shader
│   ├── postprocess.vert    # fullscreen quad vertex shader
│   ├── bloom_bright.frag   # bloom brightness threshold pass
│   ├── bloom_blur.frag     # bloom gaussian blur pass
│   └── bloom_composite.frag # bloom composite pass
├── assets/
│   └── sfx/                # sound effects (jump, land, dash, guns, etc.)
├── Makefile
└── README.md
```

---

## How the Engine Works

### Game loop (fixed timestep)

Physics runs at exactly **60 Hz** regardless of display framerate. Elapsed time is accumulated and drained in fixed 1/60 s steps. Leftover time becomes an interpolation factor for rendering, so motion looks smooth at any framerate.

```
accumulator += elapsed_time
while accumulator >= PHYSICS_DT:
    physics.update(PHYSICS_DT)
    accumulator -= PHYSICS_DT
alpha = accumulator / PHYSICS_DT   # 0..1, used to lerp camera
```

Reference: [Fix Your Timestep — Gaffer on Games](https://gafferongames.com/post/fix_your_timestep/)

### Movement (Quake-style)

Instead of `velocity = direction * speed`, acceleration is applied incrementally each tick:

```
currentSpeed = dot(velocity_horizontal, wishDir)
addSpeed     = clamp(maxSpeed - currentSpeed, 0, accel * dt)
velocity    += wishDir * addSpeed
```

Momentum is preserved in the air, so strafe-jumping can gain a small speed boost — the same mechanic behind Quake/Titanfall/ULTRAKILL movement feel.

### Rendering pipeline

Each frame:
1. Render scene to an offscreen framebuffer
2. Bloom pass — extract bright regions → gaussian blur → composite
3. Blit result to the screen
4. Render HUD and weapon view model on top (no depth test)

World geometry is batched into as few draw calls as possible (one `glDrawElements` per mesh).

### Collision

The player is an AABB. Each wall/platform is also an AABB. Collision is resolved by finding the axis of minimum penetration and pushing the player out along it. This handles stacking (standing on top of boxes) correctly.

---

## Tuning Movement Feel

All knobs are public members of `Player` in `Player.h`:

| Variable | Default | Effect |
|----------|---------|--------|
| `horizontalSpeed` | 7.0 | Max ground speed (m/s) |
| `gravity` | -24.0 | Fall acceleration |
| `jumpForce` | 8.5 | Jump height |
| `acceleration` | 80.0 | Ground responsiveness |
| `airAcceleration` | 12.0 | Mid-air steering |
| `friction` | 10.0 | Ground deceleration |

---

## How to Extend

### Add a wall / platform

In `Level.h`, add an entry inside `buildLevel()`:

```cpp
walls.push_back({ AABB{{ minX, minY, minZ }, { maxX, maxY, maxZ }} });
```

It will appear in the world visually and be solid for collision — no other changes needed.

### Add a weapon

In `GameplayState.h`:
1. Add ammo/cooldown members in the "Player weapon state" block.
2. Write a `fire___()` method modelled on `fireRevolver()`.
   - Hitscan: use `rayAABBHit()` against enemies + walls, then call `spawnTracer()`.
   - Projectile: call `projSystem.fire(origin, dir * speed, damage, true, color)`.
3. Handle the key/button in `physicsTick()` where the shooting block is.
4. Add an ammo display in `render()`.

### Add an enemy type

1. Add a value to `EnemyType` in `Enemy.h`.
2. Handle movement/attack logic in `Enemy::update()`.
3. Set health in the `Enemy` constructor switch.
4. Spawn it in `spawnEnemiesForRoom()` or directly: `enemies.push_back(Enemy(EnemyType::YOURTYPE, pos))`.

### Add a shader effect

Edit `src/shader.frag`. Ideas:
- **Fog**: `mix(FragColor, fogColor, clamp(dist / fogEnd, 0.0, 1.0))`
- **Specular**: Blinn-Phong with `uniform vec3 cameraPos` and halfway vector
- **Vertex colour**: add `vec3 color` to `Vertex`, pass through as `layout(location=3)`

---

## Dependencies

| Library | Purpose |
|---------|---------|
| SDL2 | Window, input, OpenGL context |
| SDL2_mixer | Sound effects |
| OpenGL 3.3 | Rendering (via macOS `OpenGL.framework`) |
| GLM | Math — vectors, matrices, transforms |
| LLVM/clang++ | Compiler (Homebrew) — replaces system clang |
