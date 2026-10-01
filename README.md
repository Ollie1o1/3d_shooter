# OVERDRIVE

[![CI](https://github.com/Ollie1o1/3d_shooter/actions/workflows/ci.yml/badge.svg)](https://github.com/Ollie1o1/3d_shooter/actions/workflows/ci.yml)

**[▶ Play it in your browser](https://oliver-raczka.vercel.app/work/overdrive/#play)**, no install needed.

![OVERDRIVE gameplay: Rippers charging across the Sunset Yard and breaking apart into their component blocks under shotgun fire](docs/overdrive.gif)

A 3D arena shooter built with **SDL2**, **OpenGL 3.3 Core Profile**, and **GLM**. ULTRAKILL-inspired movement with grapple hook, dashing, multi-weapon combat and style scoring, across **three themed arenas** (sunset yard, foundry, night-time reactor) of three waves each, **seven enemy types** built as animated block rigs, and a **boss fight** at the end.

Builds and runs on **macOS**, **Windows** (via MSYS2), and **in the browser** (WebAssembly + WebGL2 via Emscripten). It also builds and passes its physics tests on **Linux** (Ubuntu 24.04), though it hasn't been play-tested on a Linux desktop yet.

---

## Requirements

### macOS

```sh
brew install sdl2 sdl2_mixer glm llvm
```

> The Makefile prefers Homebrew LLVM (some Command Line Tools installs ship a `clang++` that can't find the C++ stdlib headers) and falls back to Apple clang if LLVM isn't installed. The macOS SDK path is detected with `xcrun`, so nothing needs editing.

### Linux (Debian/Ubuntu)

```sh
sudo apt install g++ make libsdl2-dev libsdl2-mixer-dev libglew-dev libglm-dev
make && make test
```

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

Shaders and assets are loaded relative to the binary, so `./shooter` can be launched from any directory.

### macOS

```sh
make        # compile → ./shooter
make run    # compile + run
make test   # headless tests: physics, level data, AI, a simulated full run
make clean  # delete binaries

./shooter --play                 # skip the main menu and start a run
./shooter --arena 3 --wave 3     # jump straight to an arena / wave (here: the boss)
./shooter --god                  # take no damage (for recording footage)
```

### Browser (WebAssembly)

The same C++ compiles to WebAssembly with [Emscripten](https://emscripten.org), rendering through WebGL2:

```sh
source ~/emsdk/emsdk_env.sh     # once per shell
make web                        # → web/dist/ (≈1.5 MB: wasm + preloaded shaders/sounds)
python3 -m http.server -d web/dist 8000   # open http://localhost:8000 (?play skips the menu; ?arena=3&wave=3 jumps to the boss)
```

What the port needed (all behind `#ifdef __EMSCRIPTEN__`, so the desktop build is unchanged):

- `gl.h` includes GLES3; `ShaderProgram` rewrites `#version 330 core` to `#version 300 es` plus default precision at load time, so one set of shaders serves both builds (keep them free of implicit int→float conversions and uniform initializers, which GLSL ES rejects).
- `main.cpp` runs one `App::frame()` per `requestAnimationFrame` instead of a blocking loop.
- Browsers use Escape to release the mouse, so losing pointer lock pauses the game; **P** also pauses.

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
| R | Reload / Retry the arena (on death) |
| F | Parry / Projectile boost |
| Left Ctrl / C | Crouch / Ground slam |
| Enter | New run (on death/win) |
| Escape / P | Pause / Resume (Escape quits to menu from death/win screens; in a browser, Escape releases the mouse and pauses) |

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
Every enemy is a rig of boxes on joints (hips, shoulders, wing roots) posed from its AI state each frame: legs swing with the walk cycle, wings flap, and arms come up to aim or slam during a wind-up, so you can read an attack before it lands. Each type teaches a different answer:

| Enemy | Looks like | What it does | Answer |
|-------|------------|--------------|--------|
| **Husk** | Humanoid rifleman, glowing visor | Holds mid range, strafes, fires slow orbs | Parry the orbs back (F) |
| **Ripper** | Low four-legged hound with blades | Zig-zags in, crouches, lunges | Dash out of the lunge |
| **Sentinel** | Tall cyclops sniper | Paints you with a laser, then fires a fast 3-round burst | Break line of sight while the laser is up |
| **Raptor** | Bird with a 4 m flapping wingspan | Circles overhead shooting, then dives at you | Watch the sky |
| **Brute** | 2.9 m heavy with a glowing chest core | Walks you down and slams the ground; lobs at range | Jump the shockwave |
| **Mite** | Small spider bomb | Rushes in and detonates | Shoot it early: its blast hurts its friends |
| **Warden** | 4.6 m crowned boss | Volleys, slams, summons Mites and Rippers; enrages at half health | Everything above |

- Ground enemies steer around cover with feeler probes (no pathfinding; the arenas are open by design) and keep apart with soft separation
- Gunners check line of sight before firing and sidestep out from behind cover when blocked
- Enemies materialise in a column of light (untargetable for 0.9 s) and break apart into their blocks when killed
- Health orbs drop from kills (Brutes always drop three) and home in when you're close
- Headshots on humanoids deal 1.5x damage

### Arenas and waves
- **Three arenas**, each with its own lighting, fog and sky: the **Sunset Yard** (open air, synthwave sun, side platforms), **the Foundry** (roofed, lava channels you can lure enemies into, a furnace to climb, catwalks), and **the Core** (night sky, a reactor ringed by pillars, corner perches)
- Each arena has three waves; the director keeps at most 6–10 enemies on the field and trickles the rest in as you kill, spawning them away from you
- New enemy types get a title card the first time they appear, with a one-line tip on how to beat them
- Clearing an arena opens its gate and points a waypoint at it; walking into the next arena closes the gate behind you
- **Jump pads** launch you onto platforms; the lighting blends between arenas as you walk the corridor
- The last three enemies of a wave get on-screen markers so you never hunt for a straggler
- Dying offers **retry this arena** (R) or a new run (Enter); enemy damage scales up from 70% in the first arena to 110% in the last

### Style System
- ULTRAKILL-inspired style meter (D → C → B → A → S → SSS)
- Style gained from kills, parries, dashes, slams, grenade multikills
- Overdrive mode at max style — dash charges refill on kill
- Style decays after 3 seconds of inactivity

### Visuals
- **Procedural textures** — grid concrete floor, brick walls, brushed metal ceiling (no external image files)
- **Procedural sky** — gradient, sun or moon (with synthwave bands in the Sunset Yard), stars and two mountain ridges, computed per pixel from the view direction
- Distance fog and hemisphere ambient per arena; self-lit neon trim; ACES filmic tone mapping
- World-space UV mapping for consistent texture tiling
- Bloom post-processing (bright pass → gaussian blur → composite)
- **CRT filter** (optional, toggle in settings) — barrel distortion, chromatic aberration, scanlines, vignette
- PSX-style vertex snapping (configurable)
- Hitscan tracers, muzzle flash, screen shake, hit-stop on kills
- Impact decals, explosion particles, shell casings
- Damage direction indicators on screen edges
- Dithered grapple rope rendering

### Audio
- **Procedural sound effects** — synthesized in Python (`tools/gen_sfx.py`), not external recordings; covers jumps, landings, dashing, slams, both weapons, reloads, grapple, hits, kills, parries, enemy telegraphs, explosions, wave stingers, spawns and pickups
- Regenerate/tweak by editing the generator and running `python3 tools/gen_sfx.py`

### Game Flow
- **Victory screen** — after the Warden: time, kills, accuracy, deaths and a letter grade (S/A/B/C/D)
- **Death screen** — where you died, with retry-arena and new-run options
- **Pause menu** — Escape mid-run pauses (Resume / Quit to Menu) instead of ending the run
- R retries the current arena; Enter starts a new run
- Settings menu: FOV, sensitivity, audio volume, FPS cap, show FPS, CRT filter
- Settings persist across launches (`settings.cfg`, written next to the binary)

---

## Project Structure

```
3d_shooter/
├── src/
│   ├── main.cpp              # entry point, SDL/OpenGL init, App::frame() loop (desktop + web)
│   ├── GameState.h           # base state interface (menu / gameplay)
│   ├── MenuState.h           # main menu + settings
│   ├── GameplayState.h       # core game loop: physics, combat, rendering
│   ├── Player.h              # kinematic character controller (Quake-style)
│   ├── Camera.h              # view/projection, mouselook
│   ├── Enemy.h               # enemy roster: stats + AI (no OpenGL, unit-tested)
│   ├── EnemyModel.h          # each enemy's animated box rig (no OpenGL)
│   ├── BoxRenderer.h         # one instanced draw for every box-built thing
│   ├── Projectile.h          # bullet/projectile system
│   ├── GrappleHook.h         # grapple hook physics
│   ├── StyleSystem.h         # style rank/score tracking
│   ├── Level.h               # the three arenas, corridors, doors, pads, lava, themes
│   ├── WaveDirector.h        # arena → wave → arena state machine (no OpenGL)
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
│   ├── box_inst.vert/frag    # instanced box shader (enemies, debris, doors, pads…)
│   ├── skybox.vert/frag      # procedural sky (sun/moon, stars, mountains)
│   ├── ui.vert/frag          # HUD shader
│   ├── tracer.vert/frag      # bullet tracer shader
│   ├── particle.vert/frag    # coloured, distance-scaled particle points
│   ├── grapple.vert/frag     # dithered grapple rope shader
│   ├── crt.frag              # CRT post-process effect
│   ├── postprocess.vert      # fullscreen quad vertex shader
│   ├── bloom_bright.frag     # bloom brightness threshold pass
│   ├── bloom_blur.frag       # bloom gaussian blur pass
│   └── bloom_composite.frag  # bloom composite pass
├── assets/
│   └── sfx/                  # procedurally-generated sound effects (.wav)
├── tests/
│   ├── test_physics.cpp      # headless player-physics tests
│   └── test_game.cpp         # level, AI, rigs and a full simulated run (`make test`)
├── tools/
│   └── gen_sfx.py            # synthesizes assets/sfx/*.wav — no external audio files
├── web/
│   └── index.html            # browser shell for the WebAssembly build (`make web`)
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
1. Render the procedural sky, then the static level (one draw per texture: floors/tops, sides, undersides, neon)
2. Gather everything built from boxes — enemies, debris, projectiles, doors, jump pads, pickups, the reactor — into one instanced draw
3. Bloom pass — extract bright regions → gaussian blur → composite with ACES tone mapping
4. Optional CRT pass — barrel distortion, chromatic aberration, scanlines
5. Render HUD (objective line, boss bar, waypoints, title cards) and screens on top

World geometry is batched into as few draw calls as possible. Everything dynamic is a unit cube instance, so a wave of enemies plus their debris is still one `glDrawElementsInstanced` call.

### Collision

The player is an AABB. Each wall/platform is also an AABB. Collision is resolved by finding the axis of minimum penetration and pushing the player out along it. A spatial grid accelerates queries so only nearby walls are tested. Perimeter walls are low enough to stand on; what keeps you in the fight is a zone check that only allows the current arena (plus the corridor and next arena once it's cleared).

---

## Tuning Movement Feel

All knobs are public members of `Player` in `Player.h`:

| Variable | Default | Effect |
|----------|---------|--------|
| `horizontalSpeed` | 7.0 | Max ground speed (m/s) |
| `gravity` | -24.0 | Fall acceleration |
| `jumpForce` | 8.5 | Jump height |
| `acceleration` | 80.0 | Ground responsiveness |
| `airAcceleration` | 42.0 | Mid-air steering |
| `friction` | 10.0 | Ground deceleration |

---

## How to Extend

### Add a wall / platform

In `Level.h`, inside the arena's block in `buildLevel()`:

```cpp
wall(x0, y0, z0, x1, y1, z1, color);   // solid, rendered
prop(x0, y0, z0, x1, y1, z1, color);   // rendered only
neon(x0, y0, z0, x1, y1, z1, color);   // self-lit trim
```

`make test` then checks that no spawn point ended up inside it and that every jump pad still lands on its platform.

### Add a weapon

In `GameplayState.h`:
1. Add ammo/cooldown members in the "Player weapon state" block.
2. Write a `fire___()` method modelled on `fireRevolver()`.
3. Add a `drawWeapon()` method in `ViewModel.h` and update the `draw()` dispatch.
4. Handle the key/button in `physicsTick()` where the shooting block is.
5. Add an ammo display in `UIRenderer::render()`.

### Add an enemy type

1. Add a value to `EnemyType` and a row to the stats table in `Enemy.h` (health, size, speed, colours, tip text).
2. Write its `think…()` behaviour; report attacks through `ev` (shots, melee, slam…).
3. Build its rig in `buildEnemy()` in `EnemyModel.h`.
4. Add it to a wave in `Level.h`.

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
