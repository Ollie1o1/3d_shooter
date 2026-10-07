# The boss pass: the Warden rebuilt, the Sovereign tuned — design

Date: 2026-10-07 · Status: approved in conversation (approach 1, sections 1–3) · Feel track, piece E of 5

## Why

The user asked for bosses that are "difficult and fun and challenging". The
boss bar (feel track): every attack has a readable **light and sound** tell,
phases change the fight, **camping and range-cheese are punished**, and there is
**always a skilful answer**.

- **The Warden** (the Core, end of Act I's arenas; also Endless's boss) fails
  most of it: three attacks (volley, slam, summon), one enrage at 50 %, no
  skilful answer, nothing against camping.
- **The Sovereign** (the Sanctum) already meets most of it, but at the doubled
  6400 hp its pacing is unproven, some enraged/difficulty-scaled wind-ups fall
  to ~0.2 s, and a parry only breaks him for 1.6 s at ×1.5.

## Decisions made with the user

- **Rebuild the Warden, tune the Sovereign** (no new Sovereign moves).
- **The Warden is a reactor-fed guardian**: conduits feed it, venting exposes
  its core, phase 2 draws on the reactor, phase 3 is a meltdown.
- **Approach 1**: the Warden gets its own files on the Penitent's pattern
  (`EnemyWarden.h`, `WardenHazards.h`, `Gameplay_Warden.h`); conduits generalise
  the Penitent's anchors; phase 2 reuses the Core's overload rings; the
  Sovereign is tuned in place.

## 1. The Warden

Health **2000 → 3600** (before difficulty). Target: a solid player kills it in
**90–120 s**. It guards the reactor at the Core's centre (`L.reactorPos`).

### Conduits

- **Four conduits**: cables from four of the Core's pillars to the Warden's
  back, each with a **node** on its pillar (a wall box at ~3 m, 250 hp, glowing
  cyan) you shoot. Hitscan and blasts damage it.
- While **any** conduit is attached the Warden takes **50 %** damage.
- A cut conduit whips loose (debris, sparks, a crack of sound); **all four cut
  → 4 s stagger** and full damage. They re-attach **one at a time, 5 s apart,
  starting 20 s after the last was cut** (a reconnect tell: the node relights,
  an arc crawls along the cable for 1 s).
- Conduits exist in phase 1 only; at phase 2 they burn out for good.
- Implementation generalises `LevelData::anchors` (the Penitent's chain
  anchors) so both bosses' wall targets share damage/break code; ripping by
  grapple stays a Penitent-only rule.

### Phase 1 — CHARGING (100–60 %)

It stalks round the reactor (as today). Attacks:

| Attack | Tell (light / sound) | Answer |
|---|---|---|
| **VOLLEY** — a fan of slow orbs (7; Standard+ 9) | chest glows magenta / its chord | strafe; **parry an orb**: it homes into the core for **150** |
| **SLAM** — shockwave ring, radius 11 | fists raised / the inhale-and-grind | jump |
| **SUMMON** — 3 Mites | a swell | kill them early |
| **VENT** — after every 3rd attack: chest plates open 2.5 s, core white-hot | white glow, steam jets / a rising hiss | hit the core (**×3**); steam scalds within 4 m (15/s) |

### Phase 2 — OVERLOAD (60–25 %)

Banner **"THE WARDEN FEEDS ON THE CORE"** (duck, 0.6 s attack pause). It walks
to the reactor and plunges its arms in; it stays there all phase (turning).

- The reactor fires **overload rings every 3.5 s** (the Core's existing rings,
  `ArenaShifts`) — jump them.
- **LANCE**: a beam from its chest sweeping a **120° arc** toward you at
  **35°/s** (slower than you run round it at 12 m). Tell 1.0 s: the chest
  brightens to a line, a rising whine. Damage 25/s while it touches you.
  Answer: keep moving, or a pillar between you. Each lance ends in a **2 s
  vent** (core ×3).
- **Anti-camping — SEEKER**: if you're **out of its line of sight or beyond 28 m
  for 4 s**, it lobs an orb that arcs over cover and bursts where you stand
  (radius 4, 35). A red ground marker for **1 s** first. Never fires while you're
  within 28 m and in sight.
- VOLLEY continues (every 2nd attack); orbs still parryable into the core.

### Phase 3 — MELTDOWN (under 25 %)

Banner **"MELTDOWN"** (duck, 0.6 s pause). It tears free and burns.

- The core is **always exposed** (×2; ×3 in a vent). Every attack **30 %
  faster** (tells never under the floors in §3).
- A **40 s meltdown clock** on the HUD. At zero it **detonates**: 60 damage and
  a big knockback to you, and it **heals to 25 %** with the clock reset (a
  setback, never an instant death). The clock stops when it dies.
- **LUNGE**: it drags itself at you (tell 0.8 s: a lurch back, a groan). Answer:
  sidestep; or **punch in its last 0.25 s** → staggered 2 s (×2 damage).
- SLAM continues; overload rings every **5 s**.

### Its voice

New attacks join the voice bank (piece D): tells for VENT, LANCE, SEEKER,
LUNGE and the meltdown detonation, a phase-change roar for each phase, all in
the Warden's massed low choir + gears. Big tells duck the mix (as today).

### Where it fights

The Core (Arena 4's boss wave) and Endless's boss waves (also the Core). Both
need the reactor and the four conduit pillars; the Warden code must not assume
a reactor exists elsewhere (if `!L.hasReactor`, phase 2 happens where it
stands and rings come off the Warden itself).

## 2. The Sovereign — a tuning pass

No new moves; health stays **6400**.

1. **Readable tells**: after every multiplier (his rage, his `quick`, the
   difficulty's `windup`), an **opening** attack's wind-up is never under
   **0.35 s**, and a **follow-up inside a combo** (the 2nd/3rd sweep, chained
   dashes) never under **0.28 s**. Each family keeps its blade glow colour and
   its sung syllable.
2. **Damage windows**: every combo ends with a **0.8 s recovery** (guard
   down); a **parry breaks his guard for 3 s at ×2** (was 1.6 s at ×1.5).
3. **Phase changes** (enraged at 50 %, last stand at 20 %) give **0.6 s with no
   new attack** so the banner and the new rules land.
4. **Cheese audit** (§3): anything a scripted cheese player survives is fixed
   with his existing tools (blink, blades, leap, crescents) and their timing.

Target: a solid player kills him in **3–4 minutes**.

## 3. Testing

Headless (`tests/test_game.cpp`), as the Penitent's:

- **Warden phases**: 100→60→25 % move it through CHARGING, OVERLOAD, MELTDOWN;
  ×0.5 while any conduit holds, ×1 with all cut, ×3 in a vent, ×2 in meltdown;
  all four cut → staggered 4 s; conduits re-attach one at a time from 20 s;
  conduits are gone in phase 2.
- **Warden answers**: a parried orb homes into the core and deals 150; a lance
  sweeps at ≤ 35°/s and is blocked by a pillar; the seeker fires only after 4 s
  out of sight or past 28 m and never at a close, visible player; the meltdown
  at zero deals 60 and heals to 25 %, never kills, and stops when it dies; a
  lunge punched in its last 0.25 s staggers it 2 s.
- **Tell floors**: for both bosses on every difficulty, enraged or not, every
  wind-up ≥ 0.35 s (combo follow-ups ≥ 0.28 s), measured from the AI.
- **Cheese players** (scripted inputs against the real AI, the game's floor
  rule, and the arena's walls): a corner-camper, a highest-perch sitter, an
  edge-kiter, and a ranged-only shooter — each takes damage within **8 s** from
  both bosses (Penitent unchanged; already covered).
- **Kill time**: a scripted "solid player" (dodges tells with a fixed reaction
  time, parries a fixed share, shoots at a fixed DPS on Standard) kills the
  Warden in 90–120 s and the Sovereign in 180–240 s.
- **Voices**: every new attack has a tell in the bank (the existing "every
  attack an enemy starts has a tell" test covers it).
- **Regression**: Act I simulated run, Endless and Daily tests keep passing.

Manual: screenshots of each Warden phase, the conduits and a vent; the HUD
meltdown clock; `--bench` in the Core vs `main` (within ~0.1 ms); `make web`.

## Out of scope

The Penitent (done in piece B); new Sovereign moves; new arenas; the Reliquary
(Act II piece 5); difficulty-curve changes outside these two bosses.
