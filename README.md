# OVERDRIVE

A 3D arena shooter built with **SDL2**, **OpenGL 3.3 Core Profile**, and **GLM**. ULTRAKILL-inspired movement with grapple hook, dashing, multi-weapon combat, style scoring, wave-based progression, and post-processing effects.

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

Open the **MSYS2 UCRT64** shell (`C:\msys64\ucrt64.exe`), navigate to the project folder, then:

```sh
make              # compile → shooter.exe
make run          # compile + run
make clean        # delete binary
```

> **Important:** use the MSYS2 UCRT64 shell, not PowerShell or cmd. The Makefile auto-detects the platform.

> **Runtime DLLs:** The game links against DLLs in `C:\msys64\ucrt64\bin` (`SDL2.dll`, `glew32.dll`, `libstdc++-6.dll`, etc.). `make run` works automatically because the UCRT64 shell already has that directory on `PATH`. If you want to run `shooter.exe` outside the shell (e.g. by double-clicking), either add `C:\msys64\ucrt64\bin` to your system `PATH`, or copy those DLLs next to `shooter.exe`.

---

## Controls

| Input | Action |
|-------|--------|
| W A S D | Move |
| Mouse | Look |
| Space | Jump / Double jump |
| Left Shift | Dash (directional) |
| Left Mouse | Fire weapon |
| Right Mouse | Grapple hook |
| 1 / Scroll Up | Revolver |
| 2 / Scroll Down | Shotgun |
| G | Throw grenade |
| R | Reload / Restart (on death/win) |
| F | Parry / Projectile boost |
| E | Interact |
| Left Ctrl / C | Crouch / Ground slam |
| Enter | Restart (on death/win) |
| Escape | Quit to menu |

---

## Features

### Combat
- **Revolver** (slot 1) — 8-round hitscan with auto-reload
- **Shotgun** (slot 2) — 2-shell pump-action, 10 pellets per shot with spread
- **Grenades** (G key) — parabolic arc, 5m blast radius, refill every 2 kills
- **Parry** (F) — deflect enemy projectiles back at 2x speed for 50 damage
- **Projectile Boost** (F near own grenade) — detonate for 3x damage AoE
- **Recoil recovery** — camera kick smoothly returns to center instead of drifting
- **Weapon switch animation** — smooth drop/raise transition with firing blocked during switch

### Movement
- Quake-style air strafing with momentum preservation
- Double jump, directional dash (2 charges), ground slam
- Grapple hook with slingshot release
- Sliding with momentum boost

### Enemies
- **GRUNT** — wide, tough (50 HP), patrol-shooter at 14-22m range
- **SHOOTER** — tall and thin (35 HP), maintains distance, longest telegraph
- **STALKER** — low and fast (25 HP), aggressive strafing, charges at close range
- **FLYER** — small diamond shape (40 HP), hovers and orbits with bobbing animation
- Each type has a distinct silhouette (per-type scaling and rotation)
- Health bars appear above damaged enemies
- Death spawns colored cube debris particles

### Progression
- **Wave system** — each room has 3 waves of increasing difficulty
  - Wave 1: mostly GRUNTs
  - Wave 2: mixed GRUNTs, SHOOTERs, STALKERs
  - Wave 3: harder mix with more STALKERs and FLYERs
- 3-second pause between waves with "WAVE N" banner
- Spawn pads pulse brighter when the next wave is incoming
- Room door slides open when all waves are cleared

### Style System
- ULTRAKILL-inspired style meter (D → C → B → A → S → SSS)
- Style gained from kills, parries, dashes, slams, grenade multikills
- Overdrive mode at max style — dash charges refill on kill
- Style decays after 3 seconds of inactivity

### Visuals
- **Procedural textures** — grid concrete floor, brick walls, brushed metal ceiling (no external image files)
- World-space UV mapping for consistent texture tiling
- Bloom post-processing (bright pass → gaussian blur → composite)
- **CRT filter** (optional, toggle in settings) — barrel distortion, chromatic aberration, scanlines, vignette
- PSX-style vertex snapping (configurable)
- Hitscan tracers, muzzle flash, screen shake, hit-stop on kills
- Impact decals, explosion particles, shell casings
- Damage direction indicators on screen edges
- Dithered grapple rope rendering

### Game Flow
- **Win screen** — "ARENA CLEARED" with stats (time, kills, accuracy, letter grade S/A/B/C/D)
- **Death screen** — "YOU DIED" with red vignette and stats
- Quick restart with R or Enter from either screen
- Settings menu: FOV, sensitivity, audio volume, FPS cap, show FPS, CRT filter

---

## Project Structure

```
3d_shooter/
├── src/
│   ├── main.cpp              # entry point, SDL/OpenGL init, game loop
│   ├── GameState.h           # base state interface (menu / gameplay)
│   ├── MenuState.h           # main menu + settings
│   ├── GameplayState.h       # core game loop: physics, combat, rendering
│   ├── Player.h              # kinematic character controller (Quake-style)
│   ├── Camera.h              # view/projection, mouselook
│   ├── Enemy.h               # enemy types, AI, and instanced renderer
│   ├── Projectile.h          # bullet/projectile system
│   ├── GrappleHook.h         # grapple hook physics
│   ├── StyleSystem.h         # style rank/score tracking
│   ├── Level.h               # map geometry (AABB walls), room/door management
│   ├── Mesh.h                # VAO/VBO wrapper
│   ├── ShaderProgram.h       # GLSL compile/link, uniform helpers
│   ├── PostProcess.h         # bloom + optional CRT post-processing
│   ├── UIRenderer.h          # HUD, win/death screens, damage indicators
│   ├── ViewModel.h           # first-person weapon models (revolver, shotgun)
│   ├── AudioSystem.h         # SDL2_mixer sound wrapper
│   ├── TextureGen.h          # procedural texture generation
│   ├── PixelFont.h           # bitmap font for UI text
│   ├── Settings.h            # game settings (FOV, sensitivity, CRT, etc.)
│   ├── Interactable.h        # trigger volumes / interactable objects
│   ├── shader.vert/frag      # world geometry shader (Blinn-Phong, point lights)
│   ├── enemy_inst.vert/frag  # instanced enemy shader
│   ├── skybox.vert/frag      # skybox gradient shader
│   ├── ui.vert/frag          # HUD shader
│   ├── tracer.vert/frag      # bullet tracer + particle shader
│   ├── grapple.vert/frag     # dithered grapple rope shader
│   ├── crt.frag              # CRT post-process effect
│   ├── postprocess.vert      # fullscreen quad vertex shader
│   ├── bloom_bright.frag     # bloom brightness threshold pass
│   ├── bloom_blur.frag       # bloom gaussian blur pass
│   └── bloom_composite.frag  # bloom composite pass
├── assets/
│   ├── level.txt             # level geometry (hot-editable)
│   └── sfx/                  # sound effects (.wav)
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
1. Render scene to an offscreen framebuffer (split into floor/wall/ceiling draws with per-surface procedural textures)
2. Bloom pass — extract bright regions → gaussian blur → composite
3. Optional CRT pass — barrel distortion, chromatic aberration, scanlines
4. Render HUD, win/death screens, and weapon view model on top

World geometry is batched into as few draw calls as possible. Enemies use GPU instancing (one `glDrawElementsInstanced` call for all enemies).

### Collision

The player is an AABB. Each wall/platform is also an AABB. Collision is resolved by finding the axis of minimum penetration and pushing the player out along it. A spatial grid accelerates queries so only nearby walls are tested.

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

In `Level.h`, add an entry inside `buildLevel()`, or edit `assets/level.txt`:

```cpp
r.walls.push_back(W({{minX, minY, minZ}, {maxX, maxY, maxZ}}, ArenaColor::Cover));
```

It will appear in the world visually and be solid for collision — no other changes needed.

### Add a weapon

In `GameplayState.h`:
1. Add ammo/cooldown members in the "Player weapon state" block.
2. Write a `fire___()` method modelled on `fireRevolver()`.
3. Add a `drawWeapon()` method in `ViewModel.h` and update the `draw()` dispatch.
4. Handle the key/button in `physicsTick()` where the shooting block is.
5. Add an ammo display in `UIRenderer::render()`.

### Add an enemy type

1. Add a value to `EnemyType` in `Enemy.h`.
2. Handle movement/attack logic in `Enemy::update()`.
3. Set health in the `Enemy` constructor switch.
4. Add per-type scaling in `EnemyRenderer::draw()`.
5. Spawn it in `spawnWaveEnemies()` or directly: `enemies.push_back(Enemy(EnemyType::YOURTYPE, pos))`.

---

## Dependencies

| Library | Purpose | Platform |
|---------|---------|----------|
| SDL2 | Window, input, OpenGL context | all |
| SDL2_mixer | Sound effects | all |
| OpenGL 3.3 | Rendering | all |
| GLM | Math — vectors, matrices, transforms | all |
| GLEW | OpenGL function loader | Windows |
| LLVM/clang++ | Compiler (Homebrew) — replaces system clang | macOS |
