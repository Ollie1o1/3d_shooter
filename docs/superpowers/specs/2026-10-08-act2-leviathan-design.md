# Act II piece 6: The Maw + THE LEVIATHAN — design

Date: 2026-10-08 · Status: written to the user's brief "start and finish the
Leviathan boss fight as the final piece" (no design conversation; every choice
below is assumed and listed so it can be overridden) · Piece 6 of 6 of Act II

## Decisions

- From the user: the Leviathan is Act II's final boss, built start to finish.
- Already decided in earlier pieces: it rises from the dark hole at the end of
  the Reliquary; Act II is its own run and was unranked "until the whole act
  exists"; the boss bar (every attack a light **and** sound tell, phases that
  change the fight, camping and range-cheese punished, always a skilful
  answer); "dark machine + glow"; voices in the voice bank; on-screen text in
  the terse-narrator voice (docs/text-pass.md).
- **Assumed** (not discussed):
  - A fifth Act II arena, **THE MAW**, a cavern under the Reliquary's hole. You
    reach it by jumping into the hole, so the act ends with a fall the way it
    began with one.
  - The Leviathan fights alone, as the Sovereign does. Three phases: **THE
    DEEP**, **THE HUNT** and **THE ECLIPSE**.
  - When it dies the run ends in a real victory screen, "ACT II COMPLETE". The
    menu loses "PREVIEW". A ranked ACT II leaderboard (local, world board, site
    key) and ASCENT stay **out of scope**: both were always listed as coming
    after piece 6.

## 1. The Maw

### Getting there

- The Reliquary's last arrangement leads to the rim, as it does now. The
  finish beacon goes; the hole itself is the way on. The hole is re-centred
  at (0, −240, −1050) with radius ~22, and no rocks sit on its south arc where
  you jump in.
- The Reliquary's zone now ends at the rim (Z −1022), so the hole's column is
  the Maw's (`Arena::zone` up to Y −200). `LevelData::corridors` gains the
  hole. When the Reliquary is cleared the HUD points at the hole, with the
  banner "THE HOLE IS OPEN" / "JUMP IN".
- Falling 60 m into the Maw. A retry starts you in the air in the hole, like
  the Nave's start. Nothing in the fight starts until you land.

### The space

Centre **M = (0, −300, −1068)**.

| Part | Shape |
|---|---|
| **The pool** | radius 13 at the centre: black water at −302 over an open void (falling in counts as a fall). The Leviathan's home. |
| **The ring** | stone floor from r 13 to r 46, cracked, with glowing seams; a low lip round the pool. |
| **Four wells** | shallow round basins (radius 3.5) at r 29 on the diagonals, dark water with a red glow. Phase 2 breaches come out of them. |
| **Six ribs** | colossal machine ribs rising from r 42 and arching inward overhead. Each has a pillar base (grapple) and a dry **ledge** at +5 m, with a step at +2.5 m. |
| **Cover** | four broken conduit pipes (1.6 m high, ~8 m long) at r 21 between the wells. |
| **The wall** | a 24-sided ring of wall boxes at r 46–48, from the floor up to −262. |

- Lit by the eclipse through the hole (a shaft of pale light on the south
  ring), by a dim red glow from the pool, and by the seams. Reverb: HALL.
  Theme: near-black, with a cold eclipse light above and a red glow below.
- `voidY` −315. A fall puts you back on the south ring with 15 damage.
- A tenth music track, **LEVIATHAN**: slow, huge, sub-bass and a distant
  choir. Act II picks track `5 + arena` (the Maw is 9); `MUSIC_TRACKS` 10.
- Waves: one boss wave `{LEVIATHAN, 1}`, `damageScale` 1.5.
- Water: one volume over the ring at −301, under the floor. It rises in phase 3.

## 2. THE LEVIATHAN

`EnemyType::LEVIATHAN`, **15000 hp** before difficulty (first written as 9000;
raised in tuning, when the scripted solid player killed 9000 in 104 s). Target:
the longest fight in the game. The scripted solid player (who dodges
perfectly and parries every other window) kills it in **150–220 s**; the
Sovereign takes ~115 s by the same measure. Real fights run longer.

### Body

A colossal serpent machine: a plated body ~2.5 m thick, a long skull of iron
plates, a jaw of turbine teeth, and the **eclipse eye**, a black disc with a
burning white-orange corona. The body comes out of its **root** (the pool, or
a well in phase 2) and curves up to the head. Its 14 segments follow a curve
from the root to the head, drawn as plated rings with glowing seams and dorsal
fins.

- `Enemy::position` is the **head**. The AI moves it directly; it skips
  `integrate()`. The root is `levRoot`.
- Hit zones, tested in `hitscanAll` and against blasts:

| Zone | Multiplier |
|---|---|
| **body** (segments) | ×0.2: rounds spark off the plates |
| **head** | ×1 |
| **eye** | ×3 while beached or staggered; ×2 in phase 3; ×1 otherwise |
| **throat** | ×3, only while it's inhaling (SWALLOW), and only from in front |

- While **hidden** (diving between wells) it is untargetable and not drawn
  except as a wake.
- Its arrival: it rises out of the pool over **3 s** (untargetable, a roar,
  the screen shaking). Then 0.6 s with no attack.

### Phase 1: THE DEEP (100–65 %)

It towers out of the pool, head ~12 m up, swaying toward you.

| Attack | Tell (light / sound) | Answer |
|---|---|---|
| **CRASH**: slams its head down along a strip from the pool to where you'll be as it lands (your motion led by its wind-up, up to 8 m; 5 m wide, out to the wall); 40 + knockback | 1.1 s: it rears back, the eye turns red, a red strip burns on the floor along the line; a deep rising roar | **dash out of the strip**. Afterwards it lies **beached** on the ring for **3 s** with the eye open (×3). **Or stand your ground and punch** in the last 0.25 s, within 6 m of where the head lands: parried, staggered **4 s** at ×2 |
| **TORRENT**: a fan of 9 slow orbs (11 on Standard+), 12 each | 0.8 s: the jaw opens, the throat glows acid-cyan; a gurgling swell | strafe; **parry an orb** and it homes into the eye for **180** |
| **TIDE**: its tail slams the water and a wave rolls out over the ring (to r 46, 1.2 m high); 25 | 0.9 s: the tail rises out of the pool behind it; the water churns and hisses | **jump it** |
| **SPIT** (anti-camping): a glob arcs onto where you'll be; bursts radius 4 for 30, then burns 4 s at 15/s | 1 s red ground marker; a hacking retch | keep moving. Fires only after **4 s** beyond 34 m from the root, out of the head's sight, or 4 m+ above the ring |

The rotation is CRASH, TORRENT, CRASH, TIDE, repeating, with SPIT whenever its
clock runs out. A CRASH always comes when you're within 10 m of the pool's lip
(no hugging the root).

### Phase 2: THE HUNT (65–30 %)

Banner **"IT GOES UNDER"** / **"WATCH THE WELLS"** (duck, 0.6 s pause).

- **DIVE**: the head plunges into its root (0.8 s), then it's **hidden** for
  1.4 s while a red wake runs under the cracks toward you.
- **BREACH**: it picks the site **nearest you**, out of the four wells and the
  pool (never the one it just left). Tell **1.2 s**: that well boils and glows
  red with a rising roar. Then it erupts straight up: **40** and a hard knock
  up to anyone within 6 m. Answer: get clear of the boiling well. A breach is
  followed by a **0.8 s** recovery while the head settles (eye ×1, head
  targetable).
- From its new root it makes **two attacks** (CRASH or TORRENT, or TIDE from
  a well as a smaller wave out to r 20), then dives again. A beaching or a
  stagger counts as its two attacks.
- SPIT continues.

### Phase 3: THE ECLIPSE (under 30 %)

Banner **"THE MAW FLOODS"** / **"GET HIGH - SHOOT THE EYE"** (duck, 0.6 s
pause). It dives into the pool and rises fully, eye blazing.

- The ring **floods**: the water rises over 4 s to 0.6 m above the floor (wading speed ~5 m/s).
  You wade, which is slow but does no damage. The six rib ledges stay dry.
- The eye is **always open** (×2; ×3 while beached or staggered).
- Every tell is **25 % quicker**, never under the floors (0.35 s; 0.28 s for a
  follow-up).
- **SWALLOW** (every third attack): tell **0.9 s**: the jaw unhinges, the
  throat lights white, a rising inhale. Then it **inhales for 3 s**, pulling
  you toward its mouth at **3.5 m/s** (slower than you wade, let alone run). Reach the
  mouth (4.5 m) and it bites: **45**, and you're thrown back. Answers: run, or
  dash and keep running; or **shoot down its throat** (×3). **450** damage
  into the throat during one inhale and it **chokes**: the inhale ends and
  it's staggered **4 s**.
- CRASH, TORRENT and TIDE continue. A TIDE in the flood still has to be
  jumped. Standing on a ledge for 4 s brings a SPIT.

### Death

It rears, screams, and its eye cracks. It sinks back into the pool, and the
eclipse light through the hole floods white. Banner **"THE LEVIATHAN SINKS"**,
then after **3 s** the victory screen: **"ACT II COMPLETE"** / **"THE ECLIPSE
BREAKS"**. Its hazards die with it.

### Its voice

`VoiceSynth` gives it a profile: a vast, low, whale-like groan over grinding
plates. The voice bank gets a tell for every attack (CRASH, TORRENT, TIDE,
SPIT, BREACH, SWALLOW, DIVE), plus specials: its rise, each phase's roar, the
choke and the death scream. Big tells duck the mix (6 dB), as the other
bosses' do.

### Text

| Where | Title | Subtitle |
|---|---|---|
| boss start | THE LEVIATHAN | DASH THE CRASH - SHOOT THE EYE WHILE IT'S DOWN |
| Reliquary cleared | THE HOLE IS OPEN | JUMP IN |
| phase 2 | IT GOES UNDER | WATCH THE WELLS |
| phase 3 | THE MAW FLOODS | GET HIGH - SHOOT THE EYE |
| CRASH parried | toast STAGGERED | DOUBLE DAMAGE - MAKE IT COUNT |
| beached (first time) | toast BEACHED | THE EYE'S OPEN - THREE SECONDS |
| choked | toast IT CHOKES | FOUR SECONDS - UNLOAD |
| killed | THE LEVIATHAN SINKS | — |
| victory screen | ACT II COMPLETE | THE ECLIPSE BREAKS |
| HUD under the bar, phase 2 | IT HUNTS BELOW - WATCH THE WELLS | |
| HUD under the bar, inhaling | SHOOT DOWN ITS THROAT | |
| menu | ACT II | 5 ARENAS - BENEATH THE ECLIPSE |

## 3. Code shape, testing

### Code (following the Warden and Penitent pattern)

- `src/Enemy.h`: `LEVIATHAN` (stats, `isBoss`, attack kinds `CRASH, TORRENT,
  TIDE, SPIT, BREACH, SWALLOW, DIVE`), its fields (phase, root, hidden, dive
  stage, beached, inhaling, choke damage, the strip's target), events `lv*`,
  `armorMult`/`parryWindow`/`staggerTime`/`targetable` rules, a tell floor
  like the Sovereign's and the Warden's. `EnemyWorld` gains `wells` and
  `levRing` (the ring floor's Y and centre).
- New `src/EnemyLeviathan.h`: `Enemy::thinkLeviathan`, the mind. It only
  emits events. Also `leviathanBody(e, out)`, the body curve's segment centres
  (shared by drawing, hit tests and the tests), and `leviathanEye`/
  `leviathanThroat`/`leviathanHead` boxes.
- New `src/LeviathanHazards.h` (no GL or audio): crash strips (marker, impact
  test, landing point), tides (rings with a jump test), spits (marker, burst,
  burning pool), breaches (boiling well, eruption), the swallow's pull and
  bite, and choke accounting.
- New `src/Gameplay_Leviathan.h`: events into hazards and damage, the flood,
  the hit zones in `hitscanAll` and blasts, parried orbs into the eye
  (generalising the Warden's homing), banners and toasts, the HUD line,
  drawing the body, head, eye, wake, strips, tides, spits and boiling wells,
  and the death sequence.
- `src/LevelAct2.h`: `buildMaw`. The Reliquary's rim, hole and zone change;
  the beacon goes.
- `src/Gameplay_Flow.h`: Act II's VICTORY becomes a real victory (a delay,
  then `finishRun`, still unranked). `Gameplay_HUD.h` shows "ACT II COMPLETE";
  the arena count is 5.
- `src/EnemyModel.h`: no generic rig for it; its body is drawn by
  `Gameplay_Leviathan.h`.
- `src/VoiceTable.h`/`src/VoiceSynth.h` for its voice; `src/MusicSynth.h` for
  LEVIATHAN (10 tracks); `src/MenuState.h` for the menu line. Dev:
  `--act2 --arena 4`, `--spawn 17`, `--overlay leviathan2/leviathan3` (start it
  in phase 2/3).
- The new headers go in the Makefile's `HEADERS` and `test` lists.

### Automated tests (`tests/test_game.cpp`)

- **Map**: five Act II arenas joined by four corridors. The Maw's ring floor
  is at −300 from r 13 to r 46. The pool is open (a fall). The four wells
  and the six ledges stand where specified; every ledge can be reached by its
  step (≤ 2.6 m) or a grapple pillar. Falling from the Reliquary's rim into
  the hole lands on the ring inside the Maw's zone, never in the Reliquary's
  void. A retry starts in the hole above the ring.
- **Phases**: 100→65→30 % moves it through DEEP, HUNT and ECLIPSE. It's
  untargetable while rising and while hidden. The multipliers are as in the
  table: body ×0.2, head ×1, eye ×3 beached, eye ×2 in phase 3, throat ×3
  inhaling and ×0 otherwise, ×2 staggered.
- **CRASH**: the strip hits a player standing in it and misses one 3 m to the
  side. A beached head opens the eye for 3 s. A punch in the last 0.25 s,
  within 6 m of the landing point, staggers it 4 s; a punch earlier or out of
  reach doesn't.
- **TORRENT**: a parried orb homes into the eye and deals 180.
- **TIDE**: hits a grounded player as the wave passes and misses a jumping
  one.
- **SPIT**: fires only after 4 s beyond 34 m, out of sight, or 4 m+ up, and
  never at a close, visible, grounded player.
- **HUNT**: a dive hides it, the breach picks the site nearest the player (not
  the last one) after a 1.2 s tell, and the eruption hits within 6 m and
  misses at 8 m.
- **ECLIPSE**: the flood rises to 0.6 m over the ring and the ledges stay dry.
  The pull is 3.5 m/s, under the player's wading speed in the flood. Reaching the mouth bites
  for 45. 450 damage into the throat in one inhale chokes it (staggered 4 s).
- **Tell floors**: on every difficulty and in every phase, an opening tell is
  ≥ 0.35 s and a follow-up ≥ 0.28 s, measured from the AI.
- **Cheese players** (BossSim): corner, perch, kite and ranged players each
  take damage within 8 s.
- **Kill time**: a scripted solid player kills it in 150–220 s on Standard.
- **Voices**: every attack it starts has a tell in the bank (the existing test
  covers this).
- **Act II run**: a simulated run goes Nave → Orrery → Descent → Reliquary →
  the Maw and ends in VICTORY when the Leviathan dies. Act I, Endless, Daily
  and the other Act II tests keep passing.

### Manual

Screenshots of each phase, a CRASH strip and a beached head, a breach, the
flood and a swallow, and the victory screen. `--bench` in the Maw vs the
Sanctum (within ~15 %). `make web`.

## Out of scope

A ranked ACT II leaderboard and the site's `act2` key; ASCENT; new
non-boss enemies; swimming.
