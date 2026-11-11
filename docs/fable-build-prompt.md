# OVERDRIVE: pathfinding, boss animation, polish and feel

You're working on OVERDRIVE, my C++/OpenGL first-person shooter (repo: `~/Projects/overdrive/3d_shooter`, branch `main`). It builds natively (macOS/Linux/Windows) and to the web with Emscripten, and it's live on my site. It's a large codebase (story chapters, Act I/II arenas, boss mode, ~10 bosses), so spend real time reading before changing anything.

There are three workstreams. Do them **in order, each on its own branch**. Keep going from one to the next without waiting for me, but stop and ask if you hit a design decision you can't settle from the code, the docs or this prompt.

---

## Before you start (read, don't skim)

1. `README.md`: especially *Code layout*, *How the engine works*, *Testing*, *Dev tools* (the `--shot`, `--record`, `--overlay`, `--spawn`, `--autoaim`, `--kite`, `--bench` flags are your eyes).
2. `docs/gameplay-standard.md`, `docs/shared-systems.md`, `docs/combat-research-and-next-steps.md`, `docs/next-pass.md`, and the latest `docs/*-pass.md` files, so you know what's already been done and the standard I hold things to.
3. `git log --oneline -40`, to see where recent work went (combat themes, encounter flow, music arrangements).
4. Then write a short plan for each workstream (`docs/fable-plan.md`): what you found, what you'll change, how you'll prove it. Update it as you go; it's also your notes if the session gets compacted.

## Ground rules (these come from past sessions; please respect them)

- **Branch per workstream. Commit freely on the branch. Never merge to main or push** until I play it and say "ship it."
- **`make test` must stay green** (it runs the whole suite, including BossSim kill times). The CI runs Linux, macOS and web, so don't break `make web`. Tests must be deterministic: no bare `rand()` in anything a test depends on (that broke Linux CI before); there's `core/CosmeticRandom.h` for visual-only randomness.
- **BossSim is order-sensitive and is a gameplay contract.** Attack timings, telegraph durations, hitboxes and damage are balance; animation work must not change them. If a move *needs* a timing change to read well, list it and ask me instead of doing it.
- **Tell colours keep their meaning:** white = parry, gold = dodge/block, red = area. Don't make tells harder to read for the sake of style.
- **No on-screen tutorial prompts or new HUD clutter.** I removed those on purpose ("clogs up the screen"). Rewards (OVERDRIVE, finishers) stay rare and inputs stay unambiguous. Never put two actions on one key where the wrong one can win silently.
- **Text:** no melodramatic or cringe banners/titles. If you add any text, keep it short and plain, and flag it for me.
- **Performance:** measure before and after with `--bench` on a busy arena (e.g. Act I arena 4 wave 2) and keep the web build's download size in mind (currently ~2.6 MB wasm). Report the numbers.
- **Claims stay true.** The README and source comments currently say there's no pathfinding (see `Enemy::steer` in `src/enemies/Enemy.h`). Update every such claim when it stops being true, and don't overstate what you built.
- Match the code's existing style (header-heavy, src-relative includes, comment style). Don't reorganise the code base as a side quest.

---

## Workstream 1: Enemy pathfinding (branch `pathfinding`)

**Today:** enemies use feeler steering (`Enemy::steer`, probe ±45/90/135°) with the comment "arenas are open by design." That's fine in open arenas but breaks down in the story levels, around big cover and on multi-level geometry: enemies grind into walls, get stuck behind cover, or can't reach a player on a ledge.

**Goal:** enemies find real routes to the player (and to flanking positions) on every map, cheaply enough to run on the web build at 60 fps with a full wave.

What I want:
- A navigation representation built **at level load** from the existing collision geometry (look at `core/Collision.h`, `world/`, and the route/bounds scanners in `tests/RouteScan.h`, `tests/BoundsScan.h`; there's likely reusable walkability logic). Choose the approach yourself (nav grid + A*, a flow field toward the player that all chasers share, a hierarchical grid, a navmesh), but justify it in the plan against our maps and the frame budget. A shared flow field per target, rebuilt incrementally, is my guess for the cheapest fit with many chasers; prove me right or wrong.
- Handle: steps/ledges the walkers can take (respect `ledgeAware()`/`supportedAt` and the void rules), jump-down links if enemies can drop, doors/moving geometry if any level has them, and **flyers** (keep them on steering unless they visibly get stuck).
- Keep feeler steering as the **local** avoidance layer (other enemies, small props) on top of the path, and keep direct chase when there's a clear line. The path only kicks in when it's needed.
- **Behaviour, not just routing:** stop conga lines (spread chasers across neighbouring routes or flank slots), let ranged enemies path to a spot with line of sight instead of standing behind a wall, and keep the existing fairness rules (attack tokens, off-screen enemies hold fire). Difficulty must not spike: smarter enemies plus the same damage could make waves much harder, so check the run telemetry/BossSim-style numbers and say if waves got harder.
- Bosses that move on the ground (Sovereign, Successor, Absolute on foot) should use it only if it doesn't fight their scripted movement. Ask me if unsure.

Prove it:
- A new headless test (add it to `make test`): for every level and arena, spawn each walker type at many reachable points and assert it reaches the player within a time limit; assert no enemy is stuck (no progress for N seconds) in a simulated wave; assert path queries stay under a time budget.
- A debug overlay/flag to draw the nav data and current paths, plus `--shot`/`--record` captures showing enemies routing around cover in at least one Act I arena and one story level.
- `--bench` before/after numbers.

---

## Workstream 2: Boss animation (branch `boss-animation`)

Bosses in scope: **the Absolute, MORNINGSTAR, the Successor, the Sovereign.**

Where they live:
- **Absolute + MORNINGSTAR:** `src/bosses/absolute/` (`AbsoluteRig.h` is the procedural spring rig with `AbsPose` channels, `absoluteGuard`, gait, sword arm; MORNINGSTAR is the same `Enemy` with `abForm=1`, scale 1.65). Models: `assets/enemies/absolute.glb`, `morningstar.glb`, built by `tools/blender/absolute_build.py` / `morningstar_build.py`.
- **Successor:** `src/bosses/successor/SuccessorRig.h` (its own spring rig, `successorGuard`, `successorCastPose`, `suBody` springs). Model `assets/enemies/successor.glb`.
- **Sovereign:** `src/bosses/sovereign/`, plus Blender keyframe clips through `src/render/AnimClip.h` (`assets/enemies/anim/sovereign.glb`, built by `tools/blender/anim_build.py`), mixed joint by joint over the code pose.
- Existing checks: `tests/test_boss_animation.cpp` (`make test-boss-animation`), `tools/boss_animation_preview.cpp` (CPU silhouette preview, no GL), and the pose overlays: `--overlay poseN` (Sovereign), `abposeN`, `absolute2/3/8`, `morning`, `morning5/6`, `swallowNN`.

Past complaints I've had, so you know my taste: bosses **gliding** (feet not matching ground covered), a **static "Doom-like" stance**, hits that look like **screenshots** (a pose snaps in, holds, snaps out), and the Absolute **holding his sword wrong** (punching with it instead of swinging it). All were patched, but the motion still isn't at the level of the rest of the game.

The goal is that each boss moves like a heavy, deliberate fighter with its own personality:
- **Anticipation → strike → follow-through → recovery** on every attack, with the telegraph pose readable at a glance (keep the existing telegraph durations exactly; shape the motion inside them).
- **Weight:** hips lead, the body shifts onto the planted foot, heavy blows drag the torso, landings compress. **No foot sliding:** planted feet stay planted; steps match distance moved at every speed and scale.
- **Overlapping action:** arms, head, cloth/ornaments, weapon tips lag and settle rather than moving rigidly with the torso.
- **Hit reactions:** taking damage, staggers, parried blows and phase transitions should all read physically (recoil away from the hit direction, scaled by damage).
- **Idle life:** breathing, guard adjustments, eyeing the player, never a frozen T-pose-ish hold.
- **Personality per boss:** the Absolute is a precise, fast duellist. MORNINGSTAR is 1.65× bigger and should feel heavier and slower-settling, not a scaled-up copy. Read the Successor's and Sovereign's states, hazards and lore text, and propose a distinct movement character for each in the plan before animating.
- **Frame-rate independence:** the springs must behave the same at 30/60/120/240 Hz (the existing test already checks some of this; extend it).

Prove it:
- Extend `test_boss_animation.cpp`: foot-slide distance under a threshold during gait, finite and bounded joints for every move at every frame rate, telegraph timings unchanged versus `main` (compare against the current values), and BossSim kill times within their existing bounds.
- For each boss, record before/after footage (`--record` with `--god --autoaim --kite --clean`, plus pose overlays) of its guard, gait and three signature attacks. Put stills in `docs/anim-review/` and look at them yourself, critically, before showing me. Fix what you see.

---

## Workstream 3: Polish and feel (branch `polish-feel`)

Check first what already exists (recent commits did combat themes, impact audio, music arrangements, room tone, screen effects, camera feel), so you build on it and don't redo it. Then pick the highest-value items from this list, tell me your picks in the plan, and build them:

1. **Places that move:** awnings, banners, cranes, birds, sparks, data pulses, lava, all cheap and per arena or level.
2. **Places that react:** neon flickers at nearby explosions, crates/props that break, lava flares, dust shaking off at heavy impacts. **Persistent scorch marks and shell-casing decals** with a cap and fade.
3. **Weather and atmosphere** tied to the arena shifts / story moments (respecting the existing `--ambient` detail levels and the web budget).
4. **Music that reacts:** drums only when the last enemy is left, a short clear stinger, intensity following the fight. Read `src/audio/MusicSynth.h`, `CombatScore.h`, `docs/music-score-pass.md` first; some of this may exist.
5. **Material impact sounds** (metal/stone/flesh/energy per surface and per enemy) and positional emitters in the world.
6. **Arena intro fly-overs** that can be skipped by any input and never block play.
7. **Your own critique:** record a few minutes of real fights in several arenas (`--record` + `--autoaim`), look at the frames, and list the five things that most hurt game feel or readability. Fix the ones that fit this workstream; list the rest for me.

All of it must respect the detail/accessibility settings (reduced motion, flashes), stay within the frame budget, and add no HUD clutter.

Prove it: tests for anything with logic (decal caps, music state transitions, prop break rules), `--bench` numbers, and before/after captures in `docs/polish-review/`.

---

## When you finish (or if you stop)

Give me a short report:
1. For each branch: what changed, in plain language, with the key files.
2. Test status (`make test`, `make web`), bench numbers before/after, web build size before/after.
3. **A playtest checklist:** exactly what to launch (commands/flags) and what to look for, so I can judge each branch in 10–15 minutes.
4. Anything you skipped, faked, couldn't verify, or think I should decide.
5. Which README/site claims need updating once I ship it (don't touch the website repo yourself).
