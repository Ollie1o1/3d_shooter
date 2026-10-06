# Act II piece 2: Hollowed variants, the Seraph, the Anchor — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Three authored enemy variants (Enraged, Twinned, Haloed) and two new enemies (the Seraph's sweeping beam, the Anchor's no-dash field), used in the Drowned Nave's waves.

**Architecture:** Variants are a field on `Enemy` (`Hollow hollow`) set from `WaveEntry::hollow()` through `SpawnRequest`; their rules live in `Enemy.h` (headless-testable): Enraged swaps in a scaled copy of the difficulty tuning, Haloed is an incoming-damage multiplier broken by headshot/parry, Twinned yields two copies via `twinsOf()` that GameplayState spawns after the kill. The Seraph and Anchor are new `EnemyType`s with their own `think*`, rigs and head boxes; their effects reach the game through `EnemyEvents` and two helpers (`segmentHitsBox`, `inAnchorField`).

**Tech Stack:** C++17 header-only game, SDL2, OpenGL 3.3/WebGL2, headless tests (`tests/test_game.cpp`, `CHECK`).

**Spec:** `docs/superpowers/specs/2026-10-06-act2-hollowed-seraph-anchor-design.md`

## Global Constraints

- After every task `make && make test` pass; before finishing `source ~/emsdk/emsdk_env.sh && make web` passes.
- ENDLESS, DAILY, ARENA (Act I) and FAST are unchanged in content and balance: no Seraph, Anchor or variant appears there.
- Enraged: move ×1.3, telegraph ×0.65, attack interval ×0.75 (rate ×1/0.75), damage dealt ×1.25.
- Twinned: two copies, `hollow = NONE`, 35% of parent max health each, scale 0.75, 1.2 m apart.
- Haloed: incoming ×0.1 while up; headshot or parry breaks; 0.4 s stagger; ×2 incoming for 2 s; style +40 "HALO BROKEN".
- Hollowed kill: XP and kill style ×1.5.
- Bosses, CONDUIT, CONDUCTOR never take a variant.
- Seraph: health 120, speed 5, hover 10–14 m over floor, range 18–28 m; charge 1.0 s; sweep 3.0 s; start point 5 m to the side; ground point tracks ≤ 8 m/s; 30 dmg/s; cooldown 3.5 s; a hit ≥ 40 or a headshot during the charge cancels it.
- Anchor: health 260, speed 2.2, radius 1.0, height 2.8; field radius 10 m (13 m Enraged), 6 m tall centred on its feet; holds at 8 m; volley of 3 orbs every 2.8 s with 1.0 s wind-up; parried orb returns 120.
- Dev spawn ids: `--spawn 12` Seraph, `--spawn 13` Anchor.
- Commits: no Co-Authored-By / Claude trailer. `rm` not allowed.

## Review Focus

1. **A Twinned enemy killed by an explosion that also kills its twins' spawn spot neighbours** (blasts iterate `enemies` while kills happen): copies must be spawned after iteration, never pushed into `enemies` mid-loop. Pinned in Task 4 (pending list flushed after the dead-enemy erase).
2. **Halo plus Conductor tether plus difficulty**: multipliers stack multiplicatively and never reach 0 damage. Pinned in Task 3 (`incomingMult` test) — the tether is applied separately in `hurtEnemy`.
3. **A Seraph that dies mid-sweep** leaves no beam behind (no damage from a dead enemy's last event). Pinned in Task 6 (beam off when `!alive`).
4. **Anchor field while airborne above it** (grappling over an Anchor): dash and grapple come back as soon as you're more than 3 m above its feet. Pinned in Task 7 (`inAnchorField` 6 m tall test).
5. **Variant intro banners** for a type the player has already met plain: both banners show, once each per run. Pinned in Task 1 (director emits one NEW_TYPE per (type, variant)).

---

## File Structure

| File | Change |
|---|---|
| `src/Enemy.h` | `Hollow`, `canBeHollow`, `hollowName/Hint`; `Enemy::{uid, hollow, scale, halo, haloOpenTimer, beam*}`; `setHollow`, `incomingMult`, `damageMult`, `breakHalo`, `splitsOnDeath`, `twinsOf`, `onBeamHit`; Enraged tuning; SERAPH/ANCHOR stats + `thinkSeraph/thinkAnchor`; `segmentHitsBox`, `inAnchorField`, `fieldRadius`; `EnemyEvents::{beamOn, beamFrom, beamTo, shotParry}` |
| `src/Level.h` | `WaveEntry::variant` + `.hollow()` |
| `src/WaveDirector.h` | `SpawnRequest::hollow`, `Queued::variant`, variant intro events |
| `src/Projectile.h` | `Projectile::{owner, parryDamage}`; `fire()` returns `Projectile*` |
| `src/EnemyModel.h` | scale in roots; SERAPH, ANCHOR rigs; head boxes; `humanoidDims(ANCHOR)` |
| `src/Progression.h` | `xpForKill` SERAPH 60, ANCHOR 90 |
| `src/GameplayState.h` | `spawnEnemy(t, pos, h)`, `nextEnemyUid`, `pendingTwins`, `breakHalo`, `beamTickCd`, `pinnedCueCd`, `anchoredAt` |
| `src/Gameplay_Combat.h` | spawn/variants, damage multipliers, halo breaks (headshot, parry), twins, beam damage + fx, kill bonus |
| `src/Gameplay_Tick.h` | dash/grapple blocked in a field, twins flushed |
| `src/Gameplay_Render.h` | halo ring, enraged tint, twin seam, Seraph beams, Anchor field |
| `src/Gameplay_Flow.h` | variant intro banners |
| `src/LevelAct2.h` | the Nave's new waves |
| `tests/test_game.cpp` | new checks |

---

### Task 1: Variants as data — from wave entry to spawned enemy

**Files:** `src/Enemy.h`, `src/Level.h`, `src/WaveDirector.h`, `src/GameplayState.h`, `src/Gameplay_Combat.h`, `src/Gameplay_Tick.h`, `src/Gameplay_Flow.h`, `tests/test_game.cpp`

**Interfaces — Produces:**
- `enum class Hollow { NONE, ENRAGED, TWINNED, HALOED };`
- `inline bool canBeHollow(EnemyType t);` `inline const char* hollowName(Hollow);` `inline const char* hollowHint(Hollow);`
- `Hollow Enemy::hollow = Hollow::NONE; int Enemy::uid = 0; float Enemy::scale = 1.f; bool Enemy::halo = false; float Enemy::haloOpenTimer = 0.f;` `void Enemy::setHollow(Hollow h);`
- `Hollow WaveEntry::variant = Hollow::NONE;` `WaveEntry WaveEntry::hollow(Hollow h) const;`
- `Hollow SpawnRequest::hollow = Hollow::NONE;`
- NEW_TYPE event value: `(int)type | ((int)hollow << 8)`.
- `void GameplayState::spawnEnemy(EnemyType t, glm::vec3 pos, Hollow h = Hollow::NONE);`

- [ ] **Step 1: Failing test** (new block before the Gauntlet section):

```cpp
    // ---------------------------------------------------------------- Hollowed variants: data
    {
        CHECK(WaveEntry(EnemyType::HUSK, 3).hollow(Hollow::HALOED).variant == Hollow::HALOED &&
              WaveEntry(EnemyType::WARDEN, 1).hollow(Hollow::HALOED).variant == Hollow::NONE &&
              WaveEntry(EnemyType::CONDUIT, 1).hollow(Hollow::TWINNED).variant == Hollow::NONE &&
              WaveEntry(EnemyType::CONDUCTOR, 1).hollow(Hollow::ENRAGED).variant == Hollow::NONE,
              "bosses, conduits and conductors never take a variant");
        Enemy h(EnemyType::HUSK, {0, 0, 0}); h.setHollow(Hollow::HALOED);
        Enemy w(EnemyType::WARDEN, {0, 0, 0}); w.setHollow(Hollow::HALOED);
        CHECK(h.hollow == Hollow::HALOED && h.halo && w.hollow == Hollow::NONE && !w.halo, "a Haloed enemy spawns with its halo up");
        LevelData V; Arena a; a.name = "V"; a.subtitle = "";
        a.bounds = a.zone = LevelBuilder::aabb(-30, 0, -30, 30, 10, 30);
        a.groundSpawns = {{-20, 0, 0}, {20, 0, 0}, {0, 0, 20}};
        a.waves = {{WaveEntry(EnemyType::HUSK, 2), WaveEntry(EnemyType::HUSK, 2).hollow(Hollow::HALOED),
                    WaveEntry(EnemyType::SHIELDBEARER, 1).with({EnemyType::HUSK}).hollow(Hollow::ENRAGED)}};
        V.arenas.push_back(a);
        WaveDirector d; d.level = &V; d.startArena(0);
        std::vector<SpawnRequest> out;
        for (int i = 0; i < 60 * 30; ++i) d.update(DT, 0, {0, 0, 0}, out);
        int haloed = 0, enraged = 0, escortsPlain = 1;
        for (size_t k = 0; k < out.size(); ++k) {
            haloed += out[k].hollow == Hollow::HALOED;
            enraged += out[k].hollow == Hollow::ENRAGED;
            if (out[k].type == EnemyType::SHIELDBEARER && k + 1 < out.size() && out[k + 1].hollow != Hollow::NONE) escortsPlain = 0;
        }
        int introPlain = 0, introHaloHusk = 0;
        for (auto& ev : d.events) if (ev.kind == DirectorEvent::NEW_TYPE) {
            introPlain += ev.value == (int)EnemyType::HUSK;
            introHaloHusk += ev.value == ((int)EnemyType::HUSK | ((int)Hollow::HALOED << 8));
        }
        CHECK(haloed == 2 && enraged == 1 && escortsPlain, "spawn requests carry the variant; a squad's escort stays plain");
        CHECK(introPlain == 1 && introHaloHusk == 1, "a variant is introduced once, on top of its plain type");
    }
```

Note `d.update` with `aliveCount = 0` spawns the whole queue over time (cap respected); `d.events` is never cleared in this loop on purpose.

- [ ] **Step 2: Run** `make test` → compile errors (`Hollow` undefined).

- [ ] **Step 3: Implement**

`src/Enemy.h`, after `isBoss`:

```cpp
// Hollowed variants (Act II): a regular enemy made harder in one specific way.
//   ENRAGED  faster, shorter wind-ups, hits harder
//   TWINNED  splits in two smaller copies when it dies
//   HALOED   takes a tenth of the damage until a headshot or a parry breaks its halo
enum class Hollow { NONE, ENRAGED, TWINNED, HALOED };
inline bool canBeHollow(EnemyType t) {
    return !isBoss(t) && t != EnemyType::CONDUIT && t != EnemyType::CONDUCTOR;
}
inline const char* hollowName(Hollow h) {
    switch (h) { case Hollow::ENRAGED: return "ENRAGED"; case Hollow::TWINNED: return "TWINNED";
                 case Hollow::HALOED: return "HALOED"; default: return ""; }
}
inline const char* hollowHint(Hollow h) {
    switch (h) {
        case Hollow::ENRAGED: return "FASTER, QUICKER TO STRIKE, HITS HARDER - DON'T WAIT FOR ITS RHYTHM";
        case Hollow::TWINNED: return "SPLITS IN TWO WHEN IT DIES - FINISH THE JOB";
        case Hollow::HALOED:  return "SHRUGS OFF DAMAGE - HEADSHOT OR PARRY TO BREAK THE HALO";
        default: return "";
    }
}
```

In `Enemy` (with the other state): 

```cpp
    int        uid = 0;              // stable id (GameplayState numbers them): who fired a shot
    Hollow     hollow = Hollow::NONE;
    float      scale = 1.f;          // TWINNED copies are smaller
    bool       halo = false;         // HALOED: up until a headshot or parry
    float      haloOpenTimer = 0.f;  // > 0: the halo just broke, it takes double damage
    void setHollow(Hollow h) {
        hollow = canBeHollow(type) ? h : Hollow::NONE;
        halo = hollow == Hollow::HALOED;
    }
```

Change `radius()`/`height()` to multiply by `scale`.

`src/Level.h` `WaveEntry`: field `Hollow variant = Hollow::NONE;` and

```cpp
    // A Hollowed variant (Act II). For a squad it's the leader only
    WaveEntry hollow(Hollow h) const { WaveEntry w = *this; w.variant = canBeHollow(type) ? h : Hollow::NONE; return w; }
```

`src/WaveDirector.h`: `struct SpawnRequest { EnemyType type; glm::vec3 pos; Hollow hollow = Hollow::NONE; };`; `Queued` gains `Hollow variant;` (filled from `e.variant` in `buildQueue`: `queue.push_back({e.type, fixed, ..., e.escort, e.variant});`); in `emitSquad` the leader's request is `out.push_back({q.type, at, q.variant});`. Add `bool seenHollow[(int)EnemyType::COUNT][4] = {};` and in `beginWave` after `introduce(e.type)` for each entry:

```cpp
            if (e.variant != Hollow::NONE && !seenHollow[(int)e.type][(int)e.variant]) {
                seenHollow[(int)e.type][(int)e.variant] = true;
                events.push_back({DirectorEvent::NEW_TYPE, (int)e.type | ((int)e.variant << 8)});
            }
```

`src/GameplayState.h`: `void spawnEnemy(EnemyType t, glm::vec3 pos, Hollow h = Hollow::NONE);` and member `int nextEnemyUid = 1;`.

`src/Gameplay_Combat.h` `spawnEnemy`: signature as above; after `enemies.push_back(...)`: `enemies.back().uid = nextEnemyUid++; enemies.back().setHollow(h);`.

`src/Gameplay_Tick.h:262`: `for (auto& s : spawns) spawnEnemy(s.type, s.pos, s.hollow);`

`src/Gameplay_Flow.h` NEW_TYPE:

```cpp
        case DirectorEvent::NEW_TYPE: {
            EnemyType t = (EnemyType)(ev.value & 255);
            Hollow h = (Hollow)(ev.value >> 8);
            if (isBoss(t)) break;
            std::string name = h == Hollow::NONE ? std::string(statsOf(t).name)
                                                 : std::string(hollowName(h)) + " " + statsOf(t).name;
            const char* hint = h == Hollow::NONE ? statsOf(t).hint : hollowHint(h);
            if (fast()) ui.feed("NEW: " + name, statsOf(t).glow);
            else pushBanner("NEW: " + name, hint, statsOf(t).glow, 3.4f);
            break;
        }
```

- [ ] **Step 4: Run** `make && make test` → ALL PASSED.
- [ ] **Step 5: Commit** `git commit -m "Hollowed variants as data: wave entry to spawned enemy"`

---

### Task 2: Enraged

**Files:** `src/Enemy.h`, `src/Gameplay_Combat.h`, `tests/test_game.cpp`

**Interfaces — Produces:** `float Enemy::damageMult() const;` (1.25 Enraged, else 1).

- [ ] **Step 1: Failing test**

```cpp
    // ---------------------------------------------------------------- Enraged
    {
        EnemyWorld w; w.playerFeet = {30.f, 0.f, 0.f}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
        auto run = [&](Hollow h) {
            Enemy r(EnemyType::RIPPER, {0, 0, 0}); r.setHollow(h); r.state = EnemyState::ACTIVE;
            for (int i = 0; i < 30; ++i) r.update(DT, w);
            return r.position.x;
        };
        float plain = run(Hollow::NONE), fast = run(Hollow::ENRAGED);
        auto windup = [&](Hollow h) {
            Enemy b(EnemyType::BRUTE, {0, 0, 0}); b.setHollow(h); b.state = EnemyState::ACTIVE;
            EnemyWorld near = w; near.playerFeet = {4.f, 0.f, 0.f}; near.playerEye = near.playerFeet + glm::vec3{0, 1.7f, 0};
            for (int i = 0; i < 600 && b.telegraphDuration <= 0.f; ++i) b.update(DT, near);
            return b.telegraphDuration;
        };
        float wp = windup(Hollow::NONE), we = windup(Hollow::ENRAGED);
        std::printf("      ripper 0.5 s: plain %.2f m, enraged %.2f m; brute wind-up %.2f vs %.2f\n", plain, fast, wp, we);
        CHECK(fast > plain * 1.2f, "an Enraged enemy moves faster");
        CHECK(we > 0.f && std::fabs(we / wp - 0.65f) < 0.02f, "an Enraged enemy winds up in 65% of the time");
        Enemy e(EnemyType::HUSK, {0, 0, 0}); e.setHollow(Hollow::ENRAGED);
        CHECK(std::fabs(e.damageMult() - 1.25f) < 1e-4f, "an Enraged enemy hits 25% harder");
    }
```

- [ ] **Step 2: Run** → compile error (`damageMult`).
- [ ] **Step 3: Implement** — private member `DifficultyTuning hollowTune_{};`. In `update()` right after `tune_ = ...`:

```cpp
        if (hollow == Hollow::ENRAGED) {   // on top of the difficulty
            hollowTune_ = *tune_;
            hollowTune_.moveSpeed  *= 1.3f;
            hollowTune_.windup     *= 0.65f;
            hollowTune_.attackRate /= 0.75f;
            tune_ = &hollowTune_;
        }
```

Public: `float damageMult() const { return hollow == Hollow::ENRAGED ? 1.25f : 1.f; }`.

`src/Gameplay_Combat.h` `updateEnemies`: the per-enemy damage scale becomes `dmgScale * e.damageMult()` everywhere `dmgScale` is used inside the loop (shots, melee, slam, detonation): declare `const float eScale = dmgScale * enemies[i].damageMult();` at the top of the loop body and use it.

- [ ] **Step 4: Run** `make && make test` → PASSED (if the ripper distance check fails because a Ripper stops at lunge range, use a HUSK at 40 m instead and keep the ×1.2 bound; ledger the change).
- [ ] **Step 5: Commit** `"Enraged: faster, quicker to strike, hits harder"`

---

### Task 3: Haloed

**Files:** `src/Enemy.h`, `src/Projectile.h`, `src/GameplayState.h`, `src/Gameplay_Combat.h`, `src/Gameplay_Tick.h`, `tests/test_game.cpp`

**Interfaces — Produces:** `float Enemy::incomingMult() const; void Enemy::breakHalo();` `int Projectile::owner = -1; float Projectile::parryDamage = 0.f;` `Projectile* ProjectileSystem::fire(...)`; `void GameplayState::breakHalo(Enemy& e);`

- [ ] **Step 1: Failing test**

```cpp
    // ---------------------------------------------------------------- Haloed
    {
        Enemy e(EnemyType::HUSK, {0, 0, 0}); e.setHollow(Hollow::HALOED); e.state = EnemyState::ACTIVE;
        CHECK(std::fabs(e.incomingMult() - 0.1f) < 1e-4f, "a halo cuts damage to a tenth");
        e.breakHalo();
        CHECK(!e.halo && e.staggered() && std::fabs(e.incomingMult() - 2.f) < 1e-4f, "broken: staggered, and it takes double");
        EnemyWorld w; w.playerFeet = {20, 0, 0}; w.playerEye = {20, 1.7f, 0};
        for (int i = 0; i < 60 * 2 + 3; ++i) e.update(DT, w);
        CHECK(std::fabs(e.incomingMult() - 1.f) < 1e-4f, "the double-damage window ends after 2 s");
        Enemy plain(EnemyType::HUSK, {0, 0, 0});
        CHECK(plain.incomingMult() == 1.f, "a plain enemy takes normal damage");
        ProjectileSystem ps;
        Projectile* pr = ps.fire({0, 1, 0}, {1, 0, 0}, 10.f, false);
        CHECK(pr && pr->alive && pr->owner == -1 && pr->parryDamage == 0.f, "fire hands back the shot so its owner can be set");
    }
```

- [ ] **Step 2: Run** → compile errors.
- [ ] **Step 3: Implement**

`Enemy`:

```cpp
    // Damage multiplier from a halo: a tenth while it's up, double for 2 s once broken
    float incomingMult() const { return halo ? 0.1f : haloOpenTimer > 0.f ? 2.f : 1.f; }
    void breakHalo() {
        if (!halo) return;
        halo = false;
        haloOpenTimer = 2.f;
        stagger(0.4f);
    }
```

In `update()`, next to `hitFlashTimer`: `if (haloOpenTimer > 0.f) haloOpenTimer = std::max(0.f, haloOpenTimer - dt);` (before the spawning and stagger early-returns).

`Projectile`: `int owner = -1;   // uid of the enemy that fired it` and `float parryDamage = 0.f;   // > 0: what it does when parried back (else 60 / 400)`. `fire()` returns `Projectile*` (the slot used, `nullptr` if the pool is full) and resets `owner = -1; parryDamage = 0.f;`.

`src/Gameplay_Combat.h` `updateEnemies` shot loop:

```cpp
        for (int k = 0; k < ev.shots; ++k)
            if (Projectile* pr = projSystem.fire(ev.shotOrigin, ev.shotDir[k] * ev.shotSpeed, ev.shotDamage * eScale, false,
                                                 enemies[i].stats().shotColor, false, 0.f, ev.shotSize, ev.shotHeavy)) {
                pr->owner = enemies[i].uid;
                pr->parryDamage = ev.shotParry;
            }
```

(add `float shotParry = 0.f;` to `EnemyEvents`).

`GameplayState::breakHalo(Enemy& e)` (declare in GameplayState.h, define in Gameplay_Combat.h):

```cpp
inline void GameplayState::breakHalo(Enemy& e) {
    if (!e.halo) return;
    e.breakHalo();
    glm::vec3 at = e.position + glm::vec3{0, e.height() + 0.4f, 0};
    fx.spawnBurst(at, {1.f, 0.8f, 0.3f}, 36, 9.f, 0.5f, 6.f);
    styleSystem.addStyle(40.f, StyleSource::PARRY);
    ui.feed("HALO BROKEN", {1.f, 0.85f, 0.3f});
    audio.play("parry", 90);
}
```

`hurtEnemy`: after the armour line, `dmg *= e.incomingMult();` and if `e.halo` add gold sparks `fx.spawnHitSparks(at, {1.f, 0.85f, 0.3f});`.

Headshot: in the hitscan loop, right after the Shieldbearer block and before `float m = ...`: `if (hits[k].head && e.halo) breakHalo(e);`.

Parry (`punch`): in the projectile branch after `p.parried = true;`:

```cpp
        for (auto& o : enemies) if (o.alive && o.uid == p.owner && o.halo) { breakHalo(o); break; }
```

and the damage lines become `p.damage = p.parryDamage > 0.f ? p.parryDamage : 400.f;` (heavy) / `... : 60.f;` (normal). In the melee-parry branch, before `e.stagger(...)`: `if (e.halo) breakHalo(e);`.

- [ ] **Step 4: Run** `make && make test` → PASSED.
- [ ] **Step 5: Commit** `"Haloed: headshot or parry breaks the halo"`

---

### Task 4: Twinned

**Files:** `src/Enemy.h`, `src/EnemyModel.h`, `src/GameplayState.h`, `src/Gameplay_Combat.h`, `src/Gameplay_Tick.h`, `tests/test_game.cpp`

**Interfaces — Produces:** `bool Enemy::splitsOnDeath() const;` `inline std::vector<Enemy> twinsOf(const Enemy& parent);` `std::vector<Enemy> GameplayState::pendingTwins;`

- [ ] **Step 1: Failing test**

```cpp
    // ---------------------------------------------------------------- Twinned
    {
        Enemy p(EnemyType::BRUTE, {5, 0, 5}); p.setHollow(Hollow::TWINNED); p.yaw = 0.3f;
        CHECK(p.splitsOnDeath(), "a Twinned enemy splits when it dies");
        auto tw = twinsOf(p);
        bool ok = tw.size() == 2;
        for (auto& t : tw)
            ok &= t.type == EnemyType::BRUTE && t.hollow == Hollow::NONE && !t.splitsOnDeath() &&
                  std::fabs(t.maxHealth - p.maxHealth * 0.35f) < 1e-3f && t.health == t.maxHealth &&
                  std::fabs(t.scale - 0.75f) < 1e-4f && t.targetable();
        ok &= tw.size() == 2 && glm::length(tw[0].position - tw[1].position) > 2.f;
        CHECK(ok, "into two plain copies at 35% health, three-quarter size, apart, that don't split again");
        CHECK(std::fabs(tw[0].radius() - p.radius() * 0.75f) < 1e-4f, "a copy's hitbox is smaller too");
        std::vector<BoxInstance> big, small;
        buildEnemy(p, 0.f, big); buildEnemy(tw[0], 0.f, small);
        float hb = 0.f, hs = 0.f;
        for (auto& b : big) hb = std::max(hb, b.model[3].y); for (auto& b : small) hs = std::max(hs, b.model[3].y);
        CHECK(hs < hb * 0.85f, "and drawn smaller");
    }
```

- [ ] **Step 2: Run** → compile errors.
- [ ] **Step 3: Implement**

`Enemy`: `bool splitsOnDeath() const { return hollow == Hollow::TWINNED; }`. After the `Enemy` struct:

```cpp
// A TWINNED enemy's two copies: plain, 35% of its health, three-quarter size,
// either side of where it fell, ready to fight at once
inline std::vector<Enemy> twinsOf(const Enemy& p) {
    std::vector<Enemy> out;
    glm::vec3 side{std::cos(p.yaw), 0.f, -std::sin(p.yaw)};
    for (float s : {-1.f, 1.f}) {
        Enemy t(p.type, p.position + side * (1.2f * s), p.floorY);
        t.maxHealth = t.health = p.maxHealth * 0.35f;
        t.scale = 0.75f;
        t.yaw = p.yaw;
        t.state = EnemyState::ACTIVE; t.spawnTimer = 0.f;
        out.push_back(t);
    }
    return out;
}
```

`src/EnemyModel.h`: `buildEnemy`'s `root` and `headBox`'s `root` become `T(e.position) * RY(e.yaw) * S(vec3{e.scale}) * S({1.f, grow, 1.f})`; the RAPTOR `base` in both gets `* S(vec3{e.scale})` before `S(vec3{grow})`.

GameplayState: `std::vector<Enemy> pendingTwins;   // TWINNED copies, spawned once the tick's enemy loop is done`. In `onEnemyKilled` after `++totalKills;`:

```cpp
    if (e.splitsOnDeath()) {
        for (auto& t : twinsOf(e)) pendingTwins.push_back(t);
        fx.spawnBurst(e.position + glm::vec3{0, e.height() * 0.5f, 0}, e.stats().glow, 30, 7.f, 0.5f, 4.f);
        ui.feed("IT SPLITS", e.stats().glow);
    }
```

`src/Gameplay_Tick.h`, right after the dead-enemy erase (line ~323), before `linkConductors`:

```cpp
    for (auto& t : pendingTwins) { enemies.push_back(t); enemies.back().uid = nextEnemyUid++; }
    pendingTwins.clear();
```

Clear `pendingTwins` in `enterArena` with `enemies.clear()`.

- [ ] **Step 4: Run** `make && make test` → PASSED; the existing model/headbox checks still pass.
- [ ] **Step 5: Commit** `"Twinned: two smaller copies when it dies"`

---

### Task 5: Reading the variants, and their reward

**Files:** `src/Gameplay_Render.h`, `src/Gameplay_Combat.h`, `tests/test_game.cpp` (none new beyond suite), manual screenshots

- [ ] **Step 1: Kill bonus** — in `onEnemyKilled`: `float hm = e.hollow != Hollow::NONE ? 1.5f : 1.f;` multiply the kill style (`15.f`/`30.f`) and the XP (`xpForKill(e.type) * ... * hm`).
- [ ] **Step 2: Visuals** in `gatherBoxes`' enemy loop, after `buildEnemy(pose, t, out)` (record `size_t from = out.size();` before it):

```cpp
        if (e.hollow == Hollow::ENRAGED)   // everything that glows burns red
            for (size_t j = from; j < out.size(); ++j) {
                float g = std::max({out[j].emissive.x, out[j].emissive.y, out[j].emissive.z});
                if (g > 0.05f) out[j].emissive = glm::vec3{1.6f, 0.22f, 0.12f} * g;
            }
        if (e.hollow == Hollow::TWINNED)   // a seam of light down the middle
            push(out, T(pose.position + glm::vec3{0, e.height() * 0.5f, 0}) * RY(pose.yaw) * S({e.radius() * 2.1f, e.height() * 0.92f, 0.05f}),
                 {0.9f, 0.9f, 1.f}, glm::vec3{0.8f, 0.9f, 1.4f});
        if (e.halo) {                      // a thick gold ring turning over its head
            glm::mat4 h = T(pose.position + glm::vec3{0, e.height() + 0.5f, 0}) * RY(-t * 1.8f);
            for (int k = 0; k < 10; ++k)
                push(out, h * RY(k * 0.6283f) * T({0.f, 0.f, 0.5f}) * S({0.34f, 0.1f, 0.1f}),
                     {1.f, 0.8f, 0.35f}, glm::vec3{1.8f, 1.3f, 0.45f});
        }
```

An Enraged enemy also sheds embers: in `updateEnemies`, every ~0.15 s per Enraged enemy, `fx.spawnBurst(e.position + glm::vec3{0, e.height() * 0.6f, 0}, {1.f, 0.3f, 0.1f}, 2, 1.5f, 0.6f, -2.f);` (use `(int)(gameClock * 7 + e.uid) % 1` style gating: track `e.age` crossing multiples of 0.15).

- [ ] **Step 3: Verify** `make && make test` pass; then

```bash
S=/private/tmp/claude-501/-Users-ollie-Desktop-3d-shooter/fe78f86e-b521-4920-97a7-267683804fa3/scratchpad
./shooter --act2 --god --cam 0 -55.5 -470 -90 -6 --spawn 0 --spawn 0 --spawn 4 --res 720 --shot 120 $S/variants.bmp
```

(dev spawns are plain — for the screenshot temporarily add a `--hollow N` dev flag in `main.cpp` that applies `Hollow(N)` to `--spawn` enemies: `g_devHollow`, used in the dev spawn loop in `Gameplay_Flow.h`; keep it, it's a useful dev tool.) Take one shot per variant (`--hollow 1/2/3`), Read each: red glow, seam, gold ring visible.

- [ ] **Step 4: Commit** `"Hollowed variants you can read at a glance, and worth more"`

---

### Task 6: The Seraph

**Files:** `src/Enemy.h`, `src/EnemyModel.h`, `src/Progression.h`, `src/Gameplay_Combat.h`, `src/Gameplay_Render.h`, `tests/test_game.cpp`

**Interfaces — Produces:** `EnemyType::SERAPH` (= 12), `AttackKind::BEAM`; `Enemy::{beamTimer, beamPoint, beamEnd}`; `EnemyEvents::{beamOn, beamFrom, beamTo}`; `inline bool segmentHitsBox(glm::vec3 a, glm::vec3 b, const AABB& box);` `bool Enemy::onBeamHit(float dmg, bool head);` (cancels a charge; returns true if it did).

- [ ] **Step 1: Failing test**

```cpp
    // ---------------------------------------------------------------- the Seraph
    {
        CHECK((int)EnemyType::SERAPH == 12 && statsOf(EnemyType::SERAPH).flying, "the Seraph flies (dev spawn 12)");
        auto sweep = [&](glm::vec3 playerVel, bool wall, int& hitTicks, int& beamTicks, float& startGap, float& maxStep, bool& charged) {
            std::vector<Wall> walls;
            // low enough that it sees your head over it, high enough to stop a beam at your feet
            if (wall) walls.push_back(Wall{LevelBuilder::aabb(17, 0, -6, 18, 2.5f, 6)});
            SpatialGrid g; g.build(walls);
            Enemy s(EnemyType::SERAPH, {0, 12, 0}); s.state = EnemyState::ACTIVE; s.attackTimer = 99.f;
            EnemyWorld w; w.walls = walls.empty() ? nullptr : walls.data(); w.wallCount = (int)walls.size(); w.grid = &g;
            glm::vec3 feet{20.f, 0.f, 0.f};
            hitTicks = beamTicks = 0; startGap = -1.f; maxStep = 0.f; charged = false;
            glm::vec3 lastTo{0.f}; bool had = false;
            for (int i = 0; i < 60 * 6; ++i) {
                feet += playerVel * DT;
                w.playerFeet = feet; w.playerEye = feet + glm::vec3{0, 1.7f, 0}; w.playerVel = playerVel;
                s.update(DT, w);
                charged |= s.attack == AttackKind::BEAM && s.telegraphTimer > 0.f;
                if (!s.ev.beamOn) { had = false; continue; }
                ++beamTicks;
                glm::vec3 pt = s.beamPoint;
                if (startGap < 0.f) startGap = glm::length(glm::vec2(pt.x - feet.x, pt.z - feet.z));
                if (had) maxStep = std::max(maxStep, glm::length(pt - lastTo));
                lastTo = pt; had = true;
                AABB pb{feet + glm::vec3{-0.4f, 0, -0.4f}, feet + glm::vec3{0.4f, 1.8f, 0.4f}};
                hitTicks += segmentHitsBox(s.ev.beamFrom, s.ev.beamTo, pb);
            }
        };
        int hit, beam; float gap, step; bool charged;
        sweep({0, 0, 0}, false, hit, beam, gap, step, charged);
        std::printf("      seraph vs still player: beam %d ticks, hit %d, start %.1f m off, max step %.3f m\n", beam, hit, gap, step);
        CHECK(charged && beam > 60, "a Seraph charges, then sweeps its beam");
        CHECK(gap >= 4.f && step <= 8.f * DT + 1e-3f, "the beam starts metres to the side and turns no faster than 8 m/s");
        CHECK(hit > beam / 2, "standing still, you're caught");
        sweep({0, 0, 7.f}, false, hit, beam, gap, step, charged);
        std::printf("      seraph vs strafing player: hit %d of %d\n", hit, beam);
        CHECK(hit < beam / 5, "strafing at walking speed, you're grazed at most");
        sweep({0, 0, 0}, true, hit, beam, gap, step, charged);
        CHECK(beam > 0 && hit == 0, "a wall between you blocks the beam");
        Enemy c(EnemyType::SERAPH, {0, 12, 0}); c.state = EnemyState::ACTIVE; c.attackTimer = 99.f;
        EnemyWorld w; w.playerFeet = {20, 0, 0}; w.playerEye = {20, 1.7f, 0};
        for (int i = 0; i < 30 && !(c.attack == AttackKind::BEAM && c.telegraphTimer > 0.f); ++i) c.update(DT, w);
        bool cancelled = c.onBeamHit(45.f, false);
        bool beamed = false;
        for (int i = 0; i < 90; ++i) { c.update(DT, w); beamed |= c.ev.beamOn; }
        CHECK(cancelled && !beamed, "a big hit during the charge cancels the beam");
        c.alive = false; c.update(DT, w);
        CHECK(!c.ev.beamOn, "a dead Seraph's beam is gone");
    }
```

- [ ] **Step 2: Run** → compile errors.
- [ ] **Step 3: Implement**

`EnemyType` gains `SERAPH, ANCHOR` before `COUNT` (ANCHOR used in Task 7; add its stats row now so tables stay complete). `AttackKind` gains `BEAM`. Roster comment lines:

```
//   SERAPH   — winged, high and far. Charges, then sweeps a beam toward you
//              that turns slower than you run: keep moving, or break line of sight.
//   ANCHOR   — slow heavy. Inside its field you can't dash or grapple; it
//              lobs slow orbs you can parry. Kill it, or keep out of its field.
```

`statsOf` rows:

```cpp
        {"SERAPH", 120.f, 0.8f, 1.6f, 5.0f, 1.0f, 3.5f, true,
         {0.82f,0.78f,0.66f}, {1.0f,0.86f,0.5f}, {1.0f,0.9f,0.6f},
         "SERAPHS SWEEP A BEAM TOWARD YOU - KEEP MOVING OR BREAK LINE OF SIGHT"},
        {"ANCHOR", 260.f, 1.0f, 2.8f, 2.2f, 1.0f, 2.8f, false,
         {0.24f,0.27f,0.32f}, {0.95f,0.25f,0.3f}, {1.0f,0.35f,0.35f},
         "ANCHORS PIN YOU DOWN - NO DASH OR GRAPPLE IN THEIR FIELD"},
```

`EnemyEvents`: `bool beamOn = false; glm::vec3 beamFrom{0.f}, beamTo{0.f};`

Free helper (after `rayBoxHit`):

```cpp
// Does the segment a→b pass through the box?
inline bool segmentHitsBox(glm::vec3 a, glm::vec3 b, const AABB& box) {
    glm::vec3 d = b - a;
    float len = glm::length(d);
    if (len < 1e-4f) return a.x >= box.min.x && a.x <= box.max.x && a.y >= box.min.y && a.y <= box.max.y &&
                            a.z >= box.min.z && a.z <= box.max.z;
    float t = rayBoxHit(a, d / len, box);
    return t >= 0.f && t <= len;
}
```

`Enemy` members: `float beamTimer = 0.f; glm::vec3 beamPoint{0.f}, beamEnd{0.f};   // SERAPH: sweeping; where it's aimed; where the beam stops`. `attackReady` also returns false while `beamTimer > 0.f`. `update()`: `case EnemyType::SERAPH: thinkSeraph(dt, w, resolve); break;`.

```cpp
    // A hit during the charge: a big one, or the head, cancels it
    bool onBeamHit(float dmg, bool head) {
        if (type != EnemyType::SERAPH || attack != AttackKind::BEAM || telegraphTimer <= 0.f) return false;
        if (dmg < 40.f && !head) return false;
        attack = AttackKind::NONE; telegraphTimer = 0.f; attackTimer = 0.f;
        return true;
    }
```

Private:

```cpp
    void thinkSeraph(float dt, const EnemyWorld& w, bool resolve) {
        animPhase += dt * 4.f;
        glm::vec3 to = flatTo(w.playerFeet);
        float d = glm::length(to);
        glm::vec3 dir = norm2(to), side{-dir.z, 0.f, dir.x};
        bool busy = telegraphTimer > 0.f || beamTimer > 0.f;
        // Drift: a slow wide orbit at range, high up; still while it charges and sweeps
        if (busy) { velocity.x = velocity.z = 0.f; }
        else {
            strafeTimer -= dt;
            if (strafeTimer <= 0.f) { strafeTimer = frand(3.f, 5.f); strafeDir = -strafeDir; }
            float radial = d < 18.f ? -1.f : d > 28.f ? 1.f : 0.f;
            glm::vec3 mv = norm2(side * strafeDir + dir * radial);
            velocity.x = mv.x * stats().speed * tune_->moveSpeed;
            velocity.z = mv.z * stats().speed * tune_->moveSpeed;
        }
        float targetY = floorY + glm::clamp(hoverY, 10.f, 14.f);
        velocity.y = glm::clamp((targetY - position.y) * 2.f, -6.f, 6.f);
        turnToward(to, dt, 3.f);
        glm::vec3 feet{w.playerFeet.x, w.playerFeet.y, w.playerFeet.z};
        if (resolve && attack == AttackKind::BEAM) { beamTimer = 3.f; attack = AttackKind::NONE; }
        if (beamTimer > 0.f) {
            beamTimer -= dt;
            glm::vec3 gap = feet - beamPoint;
            float l = glm::length(gap), step = 8.f * dt;
            beamPoint = l <= step ? feet : beamPoint + gap / l * step;
            glm::vec3 from = eyePos();
            glm::vec3 seg = beamPoint - from;
            float len = glm::length(seg);
            beamEnd = beamPoint;
            if (w.walls && len > 1e-3f) {
                glm::vec3 u = seg / len;
                AABB q{glm::min(from, beamPoint) - glm::vec3{0.5f}, glm::max(from, beamPoint) + glm::vec3{0.5f}};
                static std::vector<int> cands;
                if (w.grid) w.grid->query(q, cands);
                else { cands.clear(); for (int i = 0; i < w.wallCount; ++i) cands.push_back(i); }
                float best = len;
                for (int i : cands) { float t = rayBoxHit(from, u, w.walls[i].box); if (t > 0.f && t < best) best = t; }
                beamEnd = from + u * best;
            }
            ev.beamOn = true; ev.beamFrom = from; ev.beamTo = beamEnd;
            if (beamTimer <= 0.f) { beamTimer = 0.f; attackTimer = 0.f; }
            return;
        }
        if (attackReady(dt) && lineOfSight(eyePos(), w)) {
            // Start 5 m to one side: behind where you're heading, or off to the side if you're still
            glm::vec2 v{w.playerVel.x, w.playerVel.z};
            glm::vec3 off = glm::length(v) > 1.f ? -glm::vec3{v.x, 0.f, v.y} / glm::length(v) : side * strafeDir;
            beamPoint = feet + off * 5.f;
            startAttack(AttackKind::BEAM, 1.f);
        }
    }
```

(Seraph `attackEvery` 3.5 is its cooldown; `attackTimer = 99` in tests makes the first attack immediate. `ev` is reset in `update()` before the `!alive` return, so a dead Seraph reports no beam.)

`src/Progression.h` `xpForKill`: `case EnemyType::SERAPH: return 60; case EnemyType::ANCHOR: return 90;`

`src/EnemyModel.h` SERAPH rig in `buildEnemy`:

```cpp
    case EnemyType::SERAPH: {
        bool charging = e.attack == AttackKind::BEAM && e.telegraphTimer > 0.f, sweeping = e.beamTimer > 0.f;
        float bob = std::sin(time * 2.f + e.animPhase * 0.1f) * 0.2f;
        mat4 base = T(e.position + vec3{0.f, bob, 0.f}) * RY(e.yaw) * S(vec3{e.scale}) * S(vec3{grow});
        vec3 bone = st.color, dark = st.color * 0.45f;
        r.box(base, {0.f, 0.8f, 0.f},  {0.42f, 0.9f, 0.3f}, bone);                 // body
        r.box(base, {0.f, 0.32f, 0.f}, {0.2f, 0.5f, 0.2f}, dark);                   // tail spine
        r.box(base, {0.f, 0.95f, 0.17f}, {0.18f, 0.18f, 0.06f}, glow * 0.3f, glow * (1.5f + (sweeping ? 3.f : 3.f * tp)));   // chest lens
        mat4 head = base * T({0.f, 1.42f, 0.f}) * RX(0.2f);
        for (int k = 0; k < 8; ++k)                                                // a ring for a head
            r.box(head * RZ(k * 0.7854f), {0.f, 0.26f, 0.f}, {0.14f, 0.08f, 0.06f}, bone, glow * 0.8f);
        float flare = charging ? smooth01(tp) : sweeping ? 1.f : 0.f;
        for (float s : {-1.f, 1.f}) for (int pair = 0; pair < 2; ++pair) {
            float lift = pair == 0 ? 0.5f : -0.15f;
            mat4 wr = base * T({s * 0.2f, 1.05f - pair * 0.35f, -0.05f})
                    * RZ(s * (lift + std::sin(e.animPhase + pair) * 0.18f + flare * 0.5f)) * RY(-s * 0.25f);
            r.box(wr, {s * 0.6f, 0.f, 0.f}, {1.2f - pair * 0.3f, 0.05f, 0.42f}, bone * (pair ? 0.8f : 1.f));
            r.box(wr, {s * 1.2f, 0.f, 0.2f}, {0.1f, 0.06f, 0.5f}, glow * 0.3f, glow * (1.f + 2.f * flare));
        }
        break;
    }
```

`headBox` case:

```cpp
    case EnemyType::SERAPH: {
        float bob = std::sin(e.animPhase * 0.1f) * 0.2f;
        frame  = T(e.position + vec3{0.f, bob, 0.f}) * RY(e.yaw) * S(vec3{e.scale}) * S(vec3{grow}) * T({0.f, 1.42f, 0.f});
        centre = {0.f, 0.f, 0.f};
        half   = {0.3f, 0.3f, 0.12f};
        break;
    }
```

(The model's bob uses `time`; the hitbox ignores the 0.2 m bob's time term — the 1.15× oversize covers it. If the head-box test in Step 1 of Task 7 flags it, drop the bob from both.)

`src/Gameplay_Combat.h`:
- In `updateEnemies`, after the shots: 

```cpp
        if (ev.beamOn) {
            AABB pb{player.position + glm::vec3{-player.radius, 0.f, -player.radius},
                    player.position + glm::vec3{player.radius, player.height, player.radius}};
            if (segmentHitsBox(ev.beamFrom, ev.beamTo, pb)) {
                beamTickCd -= dt;
                if (beamTickCd <= 0.f) { beamTickCd = 0.2f; damagePlayer(6.f * eScale, epos, 0.08f, 0.02f); }
            }
            if (rand() % 3 == 0) {
                bool wet = level.waterDepthAt(ev.beamTo) > 0.05f;
                fx.spawnBurst(ev.beamTo + glm::vec3{0, 0.1f, 0}, wet ? glm::vec3{0.85f, 0.88f, 0.9f} : glm::vec3{1.f, 0.85f, 0.5f},
                              3, wet ? 2.f : 5.f, wet ? 0.8f : 0.3f, wet ? -3.f : 9.f);
            }
            if (beamHissCd <= 0.f) { audio.play("skim", 55); beamHissCd = 0.45f; }
        }
```

(members `float beamTickCd = 0.f, beamHissCd = 0.f;` in GameplayState; `beamHissCd -= dt` once per tick at the top of `updateEnemies`.)
- `hurtEnemy`: before `takeDamage`, `e.onBeamHit(dmg, crit);` (crit is the headshot flag passed by the hitscan) and if it returns true `ui.feed("BEAM BROKEN", {1.f, 0.85f, 0.5f}); styleSystem.addStyle(25.f, src);`.

`src/Gameplay_Render.h` `renderLasers`: after the Sentinel loop, a second batch:

```cpp
    static std::vector<Beam> aims, sweeps;
    aims.clear(); sweeps.clear();
    for (auto& e : enemies) {
        if (!e.targetable() || e.type != EnemyType::SERAPH) continue;
        glm::vec3 eye = e.position + glm::vec3{0, e.height() * 0.6f * e.scale, 0};
        if (e.attack == AttackKind::BEAM && e.telegraphTimer > 0.f) {
            float a = 0.3f + 0.6f * e.telegraphProgress();
            aims.push_back({glm::vec4(eye, a), glm::vec4(e.beamPoint, a * 0.6f), 0.025f});
        } else if (e.beamTimer > 0.f) {
            float f = 0.85f + 0.15f * std::sin(gameClock * 40.f);
            sweeps.push_back({glm::vec4(eye, f), glm::vec4(e.beamEnd, f), 0.22f});
            sweeps.push_back({glm::vec4(eye, f * 0.5f), glm::vec4(e.beamEnd, f * 0.5f), 0.5f});
        }
    }
    drawBeams(aims, {1.f, 0.85f, 0.5f}, view, proj);
    drawBeams(sweeps, {1.4f, 1.15f, 0.7f}, view, proj);
```

- [ ] **Step 4: Run** `make && make test` → PASSED (models test now covers SERAPH and ANCHOR — ANCHOR's rig comes in Task 7; until then add a temporary `case EnemyType::ANCHOR:` that falls through to BRUTE's rig so the ≥ 8 parts check passes; Task 7 replaces it).
- [ ] **Step 5: Commit** `"The Seraph: a beam that turns slower than you run"`

---

### Task 7: The Anchor

**Files:** `src/Enemy.h`, `src/EnemyModel.h`, `src/GameplayState.h`, `src/Gameplay_Combat.h`, `src/Gameplay_Tick.h`, `src/Gameplay_Render.h`, `tests/test_game.cpp`

**Interfaces — Produces:** `float Enemy::fieldRadius() const;` `inline bool inAnchorField(const Enemy& a, glm::vec3 feet);` `bool GameplayState::anchoredAt(glm::vec3 feet) const;`

- [ ] **Step 1: Failing test**

```cpp
    // ---------------------------------------------------------------- the Anchor
    {
        CHECK((int)EnemyType::ANCHOR == 13 && !statsOf(EnemyType::ANCHOR).flying, "the Anchor walks (dev spawn 13)");
        Enemy a(EnemyType::ANCHOR, {0, 0, 0}); a.state = EnemyState::ACTIVE;
        CHECK(inAnchorField(a, {9.5f, 0, 0}) && !inAnchorField(a, {10.5f, 0, 0}) &&
              inAnchorField(a, {0, 2.9f, 5}) && !inAnchorField(a, {0, 3.2f, 5}), "its field: 10 m round, 6 m tall");
        Enemy r(EnemyType::ANCHOR, {0, 0, 0}); r.state = EnemyState::ACTIVE; r.setHollow(Hollow::ENRAGED);
        CHECK(inAnchorField(r, {12.5f, 0, 0}) && std::fabs(r.fieldRadius() - 13.f) < 1e-4f, "an Enraged Anchor's field is 13 m");
        a.alive = false;
        CHECK(!inAnchorField(a, {1, 0, 0}), "a dead Anchor's field is gone");
        // It walks in to 8 m, holds, and lobs a parryable volley of three
        Enemy b(EnemyType::ANCHOR, {0, 0, 0}); b.state = EnemyState::ACTIVE;
        EnemyWorld w; w.playerFeet = {20, 0, 0}; w.playerEye = {20, 1.7f, 0};
        int volleys = 0, shots = 0; float parry = 0.f;
        for (int i = 0; i < 60 * 12; ++i) { b.update(DT, w); if (b.ev.shots) { ++volleys; shots = b.ev.shots; parry = b.ev.shotParry; } }
        float dist = glm::length(glm::vec2(20.f - b.position.x, -b.position.z));
        std::printf("      anchor holds at %.1f m, %d volleys of %d\n", dist, volleys, shots);
        CHECK(dist > 7.f && dist < 9.5f, "an Anchor walks in and holds about 8 m away");
        CHECK(volleys >= 2 && shots == 3 && std::fabs(parry - 120.f) < 1e-4f, "it fires volleys of three orbs that parry back for 120");
        // Head box lines up with its humanoid model
        Enemy hb(EnemyType::ANCHOR, {0, 0, 0}); hb.spawnTimer = 0.f;
        rig::HumanoidLook L = rig::humanoidDims(EnemyType::ANCHOR);
        float neck = L.legLen + L.pelvisH + L.torsoH;
        AABB head; bool has = headBox(hb, head);
        glm::vec3 eye{0.f, 1.7f, 12.f};
        CHECK(has && rayBoxHit(eye, glm::normalize(glm::vec3{0, neck + L.headS * 0.5f, 0} - eye), head) > 0.f,
              "shooting an Anchor's head is a headshot");
        Enemy sh(EnemyType::SERAPH, {0, 12, 0}); sh.spawnTimer = 0.f;
        AABB sHead; CHECK(headBox(sh, sHead) && sHead.min.y > 12.9f, "a Seraph's head is its ring, up top");
    }
```

- [ ] **Step 2: Run** → compile errors.
- [ ] **Step 3: Implement**

`Enemy`:

```cpp
    float fieldRadius() const { return hollow == Hollow::ENRAGED ? 13.f : 10.f; }
```

Free function after `Enemy`:

```cpp
// Is the player's feet inside an ANCHOR's field? (A cylinder on its feet,
// 3 m up and down: grapple high over it and you're free.)
inline bool inAnchorField(const Enemy& a, glm::vec3 feet) {
    if (a.type != EnemyType::ANCHOR || !a.alive || a.state != EnemyState::ACTIVE) return false;
    if (std::fabs(feet.y - a.position.y) > 3.f) return false;
    glm::vec2 d{feet.x - a.position.x, feet.z - a.position.z};
    return glm::dot(d, d) <= a.fieldRadius() * a.fieldRadius();
}
```

`update()`: `case EnemyType::ANCHOR: thinkAnchor(dt, w, resolve); break;`. Private:

```cpp
    void thinkAnchor(float dt, const EnemyWorld& w, bool resolve) {
        glm::vec3 to = flatTo(w.playerFeet);
        float d = glm::length(to);
        if (telegraphTimer > 0.f || d < 8.f) velocity.x = velocity.z = 0.f;
        else { setMove(to, stats().speed, w); animPhase += dt * 3.f; }
        turnToward(to, dt, 1.5f);
        if (resolve && attack == AttackKind::VOLLEY) {
            fireAt(w.playerEye, 3, 0.12f, 11.f, 16.f, 1.8f);
            ev.shotParry = 120.f;
            attack = AttackKind::NONE;
        }
        if (attackReady(dt)) {
            if (lineOfSight(eyePos(), w)) startAttack(AttackKind::VOLLEY, 1.f);
            else attackTimer = stats().attackEvery * 0.6f;
        }
    }
```

`ledgeAware()` already includes non-flying non-Ripper/Mite types (ANCHOR included).

`src/EnemyModel.h`: `humanoidDims` row `case EnemyType::ANCHOR: return {0.95f, 0.4f, 0.5f, 0.2f, 1.05f, 1.25f, 0.8f, 0.4f, 1.15f, 0.4f, {}, {}, {}};`; add `ANCHOR` to `headBox`'s humanoid case list. Rig (replacing Task 6's temporary fall-through):

```cpp
    case EnemyType::ANCHOR: {
        HumanoidLook L = humanoidLook(e.type, st.color, st.color * 0.5f, glow);
        bool firing = e.attack == AttackKind::VOLLEY;
        mat4 hunch = root * RX(0.18f);                                       // bent under its weight
        auto f = humanoid(r, hunch, L, e.animPhase, std::max(stride, 0.3f), firing ? ArmPose::AIM_BOTH : ArmPose::SWING,
                          firing ? smooth01(tp * 1.5f) : 0.f);
        float pulse = 0.6f + 0.4f * std::sin(time * 2.5f);
        mat4 ring = f.torso * T({0.f, 0.75f, -0.62f});                      // the ring core on its back
        for (int k = 0; k < 10; ++k)
            r.box(ring * RZ(k * 0.6283f), {0.f, 0.55f, 0.f}, {0.3f, 0.14f, 0.2f}, st.color * 0.8f, glow * (0.6f + pulse));
        r.box(ring, {0.f, 0.f, 0.f}, {0.36f, 0.36f, 0.18f}, glow * 0.3f, glow * (2.f * pulse + 2.f * tp));
        for (float s : {-1.f, 1.f}) {
            r.box(f.torso, {s * 0.72f, 0.98f, 0.f}, {0.55f, 0.36f, 0.7f}, st.color * 1.2f);   // pauldrons
            r.box(s < 0.f ? f.armR : f.armL, {0.f, -1.3f, 0.f}, {0.5f, 0.45f, 0.5f}, st.color * 0.8f);   // fists
        }
        break;
    }
```

(If the hunch makes the head-box test miss, apply the same `RX(0.18f)` in `headBox` for ANCHOR — `base = root * RX(0.18f)` in the humanoid case when `e.type == EnemyType::ANCHOR`.)

GameplayState:

```cpp
inline bool GameplayState::anchoredAt(glm::vec3 feet) const {
    for (auto& e : enemies) if (inAnchorField(e, feet)) return true;
    return false;
}
```

Member `float pinnedCueCd = 0.f;` and `void pinnedCue()`:

```cpp
inline void GameplayState::pinnedCue() {
    if (pinnedCueCd > 0.f) return;
    pinnedCueCd = 0.6f;
    audio.play("clank", 45);
    ui.feed("PINNED - NO DASH OR GRAPPLE", {1.f, 0.3f, 0.3f});
}
```

`src/Gameplay_Tick.h` dash: `bool pinned = anchoredAt(player.position);` above the dash block; condition becomes `if (dashKey && !prevDashKey && dashCharges > 0 && !pinned)`, plus `else if (dashKey && !prevDashKey && pinned) pinnedCue();`. Grapple: after the GROUNDED line, `if (pendingGrapple && pinned && !grapple.active) { pendingGrapple = false; pinnedCue(); }` and `if (pinned && grapple.active) grapple.release();`. `pinnedCueCd -= dt` each tick.

On Anchor death (`onEnemyKilled`): `if (e.type == EnemyType::ANCHOR) { fx.spawnShockwave(e.position, e.fieldRadius(), e.stats().glow); styleSystem.addStyle(30.f, src); ui.feed("FIELD BROKEN", e.stats().glow); }`. Drops: `case EnemyType::ANCHOR: for (int i = 0; i < 2; ++i) drop(PickupKind::ORB); break;`.

`src/Gameplay_Render.h` `gatherBoxes`, in the enemy loop:

```cpp
        if (e.type == EnemyType::ANCHOR && e.targetable()) {   // its field: a ring on the ground and faint ribs
            float R = e.fieldRadius(), pulse = 0.55f + 0.45f * std::sin(t * 2.f + e.uid);
            glm::vec3 c = pose.position + glm::vec3{0, 0.06f, 0};
            glm::vec3 g = e.stats().glow * (0.8f + 0.6f * pulse);
            for (int k = 0; k < 40; ++k) {
                float a = k * 0.15708f;
                push(out, T(c + glm::vec3{std::cos(a) * R, 0.f, std::sin(a) * R}) * RY(-a) * S({0.12f, 0.06f, 1.62f}), g * 0.3f, g);
                if (k % 5 == 0)
                    push(out, T(c + glm::vec3{std::cos(a) * R, 1.5f, std::sin(a) * R}) * S({0.05f, 3.f, 0.05f}), g * 0.1f, g * 0.35f * pulse);
            }
        }
```

- [ ] **Step 4: Run** `make && make test` → PASSED.
- [ ] **Step 5: Commit** `"The Anchor: a field that pins you to the ground"`

---

### Task 8: The Nave's new waves

**Files:** `src/LevelAct2.h`, `tests/test_game.cpp`

- [ ] **Step 1: Failing test** — extend the simulated ACT II run block: count spawn requests by type and variant, and add (after the run):

```cpp
        CHECK(seraphs >= 4 && anchors >= 1 && haloed >= 3 && twinned >= 2 && enragedSpawns >= 1,
              "the Nave's waves bring Seraphs, an Anchor and every variant");
```

(in the loop: `for (auto& r : out) { alive.push_back(3.f); seraphs += r.type == EnemyType::SERAPH; anchors += r.type == EnemyType::ANCHOR; haloed += r.hollow == Hollow::HALOED; twinned += r.hollow == Hollow::TWINNED; enragedSpawns += r.hollow == Hollow::ENRAGED; }`). And a new block:

```cpp
    // ---------------------------------------------------------------- ENDLESS stays as it was
    {
        bool clean = true;
        EndlessWaves g; g.begin(99u, {}, true);
        for (int n = 0; n < 40; ++n)
            for (auto& e : g.wave(n).first) {
                if (e.type == EnemyType::SERAPH || e.type == EnemyType::ANCHOR || e.variant != Hollow::NONE) clean = false;
                for (auto t : e.escort) if (t == EnemyType::SERAPH || t == EnemyType::ANCHOR) clean = false;
            }
        CHECK(clean, "ENDLESS never brings a Seraph, an Anchor or a Hollowed enemy");
    }
```

- [ ] **Step 2: Run** → FAIL on the Nave counts (ENDLESS check passes; it guards the future).
- [ ] **Step 3: Implement** — `src/LevelAct2.h` waves:

```cpp
    a.waves = {
        {{EnemyType::HUSK, 4}, WaveEntry(EnemyType::HUSK, 3).hollow(Hollow::HALOED), {EnemyType::RIPPER, 3},
         WaveEntry(EnemyType::RIPPER, 2).hollow(Hollow::TWINNED), {EnemyType::SENTINEL, 3},
         WaveEntry(EnemyType::SHIELDBEARER, 2).with({EnemyType::HUSK})},
        {{EnemyType::RAPTOR, 3}, {EnemyType::SERAPH, 2}, {EnemyType::BRUTE, 1},
         WaveEntry(EnemyType::BRUTE, 1).hollow(Hollow::ENRAGED), {EnemyType::CONDUCTOR, 2}, {EnemyType::MITE, 6}},
        {{EnemyType::ANCHOR, 1}, {EnemyType::BRUTE, 2},
         WaveEntry(EnemyType::SHIELDBEARER, 2).with({EnemyType::SENTINEL}).hollow(Hollow::TWINNED),
         {EnemyType::CONDUCTOR, 2}, {EnemyType::SERAPH, 2}, WaveEntry(EnemyType::JUGGERNAUT, 1).hollow(Hollow::HALOED),
         {EnemyType::RIPPER, 4}},
    };
```

The Anchor is the first entry of wave 3, so the round-robin queue spawns it first. Update the LevelAct2.h header comment's wave line.

- [ ] **Step 4: Run** `make && make test` → PASSED; the Nave spawn/flood tests still pass (Seraphs use air spawns; the Anchor's 1.0 m radius fits ground spawns — the Juggernaut-sized check already covers it).
- [ ] **Step 5: Commit** `"The Nave's waves: halos, twins, Seraphs and an Anchor"`

---

### Task 9: Verification, web, docs

- [ ] **Step 1: Screenshots** (Read each):

```bash
S=/private/tmp/claude-501/-Users-ollie-Desktop-3d-shooter/fe78f86e-b521-4920-97a7-267683804fa3/scratchpad
./shooter --act2 --god --cam 0 -54 -470 -90 4 --spawn 12 --res 720 --shot 240 $S/seraph.bmp
./shooter --act2 --god --cam 0 -55 -470 -90 -10 --spawn 13 --res 720 --shot 120 $S/anchor.bmp
./shooter --act2 --god --wave 3 --cam 22 -50 -485 -110 -18 --res 720 --shot 600 $S/wave3.bmp
```

Expected: the Seraph's aim line and then a thick beam; the Anchor's ground ring; wave 3 with the Anchor first.

- [ ] **Step 2: Bench** `./shooter --act2 --god --wave 3 --cam 0 -48 -560 -90 -5 --bench 600 --res 1080` vs `./shooter --arena 5 --god --cam 0 1.7 -300 -90 -5 --bench 600 --res 1080` → within ~15%.
- [ ] **Step 3: Web** `source ~/emsdk/emsdk_env.sh && make web` → builds.
- [ ] **Step 4: README** — Enemies section: add the Seraph and the Anchor, and a line on Hollowed variants (Act II). `--spawn 12/13` and `--hollow N` in the flags list.
- [ ] **Step 5: Commit** `"README: the Seraph, the Anchor, Hollowed variants"`; update memory (`project_act2.md` progress: piece 2 done; next piece 3, the Orrery).
