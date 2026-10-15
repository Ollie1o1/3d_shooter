# The Zenith + THE ABSOLUTE + the sword — design

Date: 2026-10-14 · Status: agreed section by section with the user in a
brainstorm, written for review · The game's final piece: a secret encore after
Act II

## Decisions

- From the user:
  - A final encore level, reached after beating the Maw (and from the dev
    level select): the hardest boss in the game. Hard, never impossible.
  - The boss is a white-glowing humanoid, incredibly fast, with lasers, area
    attacks and attack combos, in the spirit of Minos Prime (ULTRAKILL). The
    Sovereign's duel was the right idea; this steps it up.
  - A big arena that fits a final boss.
  - A sword for the player, in the guns' aesthetic: left click swings, right
    click parries or blocks. Tried out in this fight.
- Chosen in the brainstorm:
  - The sword exists **only in this fight** (it can be rolled out later).
  - Right click: **tap = parry, hold = block** with a guard meter; F punch is
    unchanged.
  - Difficulty: **fast and lethal, short**. Combos are readable; a missed
    string costs 60–70 % of your health; you heal by fighting, mostly with the
    sword.
  - Loadout: **guns + sword, sword favoured**.
  - Access: **straight after the Leviathan, in the same run**. Act II's clear
    is saved first; a death in the encore retries the encore.
  - Structure: a **combo-string duellist** with colour-coded strikes, and a
    blade of light in the final phase (a nod to the mirror match).
- Placeholder names, all easy to change: the boss **THE ABSOLUTE**, the arena
  **THE ZENITH**, the sword **OATHBREAKER**.
- Out of scope: the sword anywhere else, a separate menu entry, a ranked
  board for the encore (a local best time only), ASCENT.

## 1. Getting there

- When the Leviathan dies it sinks as now, and "ACT II COMPLETE" shows for
  2 s. **At that moment** the Act II clear and time are saved exactly as they
  are today (so a death in the encore never costs the clear).
- Then a white-out, the banner **"THE ECLIPSE BREAKS" / "SOMETHING ABOVE IT
  DOES NOT"**, and the run moves to a sixth Act II arena, THE ZENITH. You
  arrive standing at the floor's south edge; nothing starts until a 2 s
  arrival (he descends onto the Dais, untargetable).
- The run's timer keeps going. Beating him ends the run with a victory card
  **"ZENITH CLEARED" / "THE LIGHT FALLS"** and records a second local time,
  *ACT II + ZENITH*.
- A death in the Zenith uses `retryArena()` and retries the Zenith only.
- Dev: the level select gets **ACT II › THE ZENITH**; `--act2 --arena 6`
  starts there; `--overlay absolute2|absolute3` jump to the later phases.

## 2. The Zenith

Centre **Z = (0, +400, −1068)**, high above the Maw, a floating ruin in open
sky.

| Part | Shape |
|---|---|
| **The Dais** | r 0–9, raised 1.2 m, white stone with a gold inlay ring. He arrives and dies here. |
| **The Floor** | r 9–44, polished black stone with a grid of thin white seams. The seams carry every ground telegraph (they glow red where an area attack will land). |
| **Eight pillars** | at r 34, 2.5 m wide, 14 m tall, white with glowing cracks. Cover from beams; grapple points. Four shatter in the phase 2 transition. |
| **The Shards** | six floating islands at r 52–62, past a 6 m gap; grapple or dash to reach them. Safe from ground areas, punished by the anti-camp rules. They fall away at phase 3. |
| **The Halo** | a vast ring of light overhead (r 70, 40 m up): backdrop, the Zenith Ray's source, lowering slowly in phase 3. |
| **The edge** | open void. `voidY` = Z.y − 40. A fall returns you to the floor's south edge with 20 damage. |

- The first bright arena: a pale white sky fading to deep violet at the
  horizon, the shattered eclipse behind him (a cracked black disc leaking
  white light), heavy bloom. He is the brightest thing in it.
- Music: an eleventh track, **ZENITH** (fast, ~170 BPM: a driving pulse, choir,
  distorted bells). Act II picks track `5 + arena`, so the Zenith is 10;
  `MUSIC_TRACKS` becomes 11.
- Reverb: a new `ReverbSpace::OPEN` (a long, open tail).
- Waves: one boss wave `{ABSOLUTE, 1}`.

## 3. THE ABSOLUTE

`EnemyType::ABSOLUTE`, a boss (`isBoss`). **9000 hp** before difficulty,
tuned so the scripted solid player kills him in **150–180 s**.

### Body

A 2.6 m humanoid box rig, every part emissive white: broad shoulders, narrow
waist, long arms, light-trailing wrist wraps, a featureless head with a thin
gold face slit, and a **broken halo** hovering behind the head that tilts with
his moves. Barehanded (fists, knees, kicks) until phase 3, when he draws a
**blade of light** from the halo. Every teleport and dash leaves
**afterimages**: fading white copies of his pose.

### Movement

Run 11 m/s. He closes distance with **teleports** (0.25 s flash where he
stands, an afterimage, then he's there) and **dashes** (32 m/s). Never slower
than the player.

### The colour rule

Every attack has exactly one tell kind, in light and in sound:

| Tell | Meaning | Answer |
|---|---|---|
| **White** flash on the striking limb + a bright *ting* | parryable | F punch, or the sword's parry |
| **Gold** flash + a low *thrum* | unparryable | dodge, or block with the sword (heavy guard drain) |
| **Red** seam lines / a red beam core | area or laser | leave the marked area / break line of sight |

### Parries, his guard, healing, damage

- Parrying a **white** strike interrupts the string: he **reels 0.8 s**
  (takes ×1.5). **Two parries in one string** break him: **staggered 3 s**
  (×2). A parried projectile returns to him for its parry value.
- **Guard**: as the Sovereign's, gunfire from beyond **14 m** is turned
  aside while he isn't staggered, reeling or mid-attack. Close in, guns hit.
  Sword hits always land.
- **Healing**: through the existing `styleSystem.heal`. A sword hit heals
  **3×** what a gun hit of the same damage heals; a successful parry heals
  **10**.
- **Damage**: single hits 18–30; a whole string taken is 60–70 of 100.
- Everything scales with difficulty through `tune()`; no tell is ever shorter
  than its floor (0.35 s for an opener, 0.22 s for a follow-up within a
  string).

### Voice

Short original lines in the voice bank: **"KNEEL."**, **"TOO SLOW."**,
**"AGAIN."**, **"BE JUDGED."** on strings and phase changes; **"...WORTHY."**
as he dies.

### Phase 1: JUDGMENT (100–60 %)

| String | Moves | Answer |
|---|---|---|
| **TOO SLOW** | teleport behind you → jab (15) → jab (15) → hook (22), all **white** | turn and parry; two parries break him |
| **KNEEL** | dash in → **gold** uppercut (20, launches you 6 m) → **gold** drop-kick at you in the air (28) | dash sideways off the uppercut, or block it |
| **BE JUDGED** | hop back → **RAY**: a 0.6 s red line, then a 1 s beam (25/s) that turns slower than you strafe → **STOMP**: a red ring rolling out along the seams (20) | strafe or use a pillar, then jump the ring |
| **SERPENT** | at range (> 20 m): a slow homing coil of light (20), **white** | parry it back for **300**, or outrun it |

**Anti-camping** (every phase): on a shard, beyond 30 m, or behind the same
pillar for **3 s**, he teleports to you and opens with TOO SLOW.

### Phase 2: ASCENSION (60–25 %)

Banner **"HE ASCENDS" / "STAY CLOSE"** (duck, 0.6 s pause). He rises over the
Dais and sweeps a **360° beam at chest height** (30): slide under it. It
shatters four of the eight pillars.

- Every string gains one more hit (another jab / a second drop-kick / a second
  ray); gaps within strings are 25 % shorter.
- **MIRAGE**: two afterimage copies open strings with him. Only the one with
  the halo is real; a copy dies to one hit and its blows do half damage.
- **SEAMS**: a checkerboard of 6 m floor cells burns red in two alternating
  waves 0.7 s apart (25): stand in the dark cells.
- **ZENITH RAY**: a beam from the Halo tracks you for 2.5 s, turning slower
  than you run (20/s). In this phase it is the punishment for camping on a
  shard (in place of the teleport); beyond 30 m or behind a pillar still
  brings the teleport.
- **Delayed strikes**: some white flashes land 0.2 s late. The parry window
  is on the hit, not the flash.

### Phase 3: ABSOLUTE (25–0 %)

Banner **"THE BLADE OF LIGHT" / "MEET IT"** (duck, 0.6 s pause). He draws the
blade from the Halo; the Shards fall away; the Halo starts lowering.

- **Sweep chain**: three sword sweeps (22 each), **white**.
- **Lunge**: a 20 m thrust (30), **gold**.
- **Crescents**: thrown arcs of light (18), **white**, parryable back for 200.
- **Blade clash**: a **sword** parry of a blade strike shatters his blade:
  staggered 3 s (×2), and he redraws it after. An F parry only interrupts.
- **Last judgment** (once, at 8 %): he rises; every seam goes red **except a
  6 m ring around him**. Be beside him for the 2 s, or take 60. Afterwards
  he's spent for **4 s** (×2).
- Phase 1–2 strings stay in the rotation, with the phase 2 timings.

### Death

He kneels, the halo cracks, "...WORTHY.", and he dissolves upward in light.
The victory card follows (section 1).

## 4. The sword: OATHBREAKER

### Look

Built from `GunKit`'s parts: a blackened gunmetal blade with worn steel edges,
a bone grip wrapped in brass wire, a brass crossguard. Glow colour **rose-red**
(1.0, 0.25, 0.45), distinct from his white and the guns' amber, red, cyan and
violet. Its **fuller holds 8 lit cells showing the guard meter**, the way the
guns' cells show ammo.

### Slot and switching

**Not a fifth `WeaponId`.** `WEAPON_COUNT` sizes the saved upgrade arrays
(`Progression.h`) and the armory and HUD loops; bumping it changes the save
format and puts a sword in every mode. Instead the sword is a separate
`swordOut` state: key **5** draws it, any gun key / scroll / D-pad puts it
away. It's only allowed in the Zenith. The HUD weapon strip shows a fifth slot
there.

### Left click: swing

- A 3-hit combo: right-to-left slash, left-to-right slash, overhead:
  **70 / 70 / 120**, ~0.3 s per swing, 3.2 m reach in a 100° arc. The combo
  resets after 0.6 s without a swing.
- **Lock-on step**: with him (or any target) within 6 m in front of you, each
  swing carries you up to 2 m toward him.
- **Hold 0.5 s, release: THRUST**, dashing you 8 m forward for **220**.
- **Slam with the sword out: PLUNGE**, 150 in a 4 m ring where you land.
- A glowing arc trail follows each swing; hits get hit-stop and a metallic
  *shing*.

### Right click: parry, then block

- **First 0.15 s: PARRY.** A white melee strike is deflected (the colour
  rule's result), and your next swing within 1 s is a **RIPOSTE** (×2.5). A
  projectile goes back where you aim. A RAY parried in its first 0.15 s is
  reflected into him for **250** and ends.
- **Held: BLOCK.** Frontal hits do 25 %; you move at 60 %; no swings. The
  **guard meter** (100) drains 25 per white hit, 60 per gold hit, 15/s inside
  a beam; it refills at 30/s after 1 s without blocking. **Empty: GUARD
  BREAK**, stunned 1 s, the fuller goes dark, a *crack*.
- A press within 0.4 s of a missed parry opens **no** parry window (spam
  guard); it still blocks.
- A small guard bar sits under the crosshair while blocking and just after a
  parry.

## 5. Architecture

| New file | Holds |
|---|---|
| `Sword.h` | All sword logic, no OpenGL: combo state, parry/block windows, the guard meter, the swing-arc hit test, thrust, plunge. |
| `EnemyAbsolute.h` | His AI: the string table (each string a list of moves with tell kind, timing and damage), phases, teleports, mirages, anti-camp clocks. Hooks into `Enemy::think` as the Leviathan's does. |
| `AbsoluteHazards.h` | Rays, the Zenith Ray, seam areas, stomp rings, serpents, crescents, the last judgment. |
| `Gameplay_Absolute.h` | Damage both ways, parry and clash results, banners, voice, phase transitions, pillar shattering, the shards falling. |
| `Gameplay_Sword.h` | Sword input, hits on enemies, riposte and healing, HUD pieces. |

Edits: `LevelAct2.h` (the sixth arena, breakable pillars, falling shards),
`Gameplay_Flow.h` (the Leviathan's victory saves Act II and hands off),
`EnemyModel.h` (his rig, halo, blade, afterimages), `GunKit.h` /
`ViewModel.h` (`buildSword`, swing/parry/block animation), `MusicSynth.h` and
the voice tables, `AudioTypes.h` (`ReverbSpace::OPEN`), `MenuState.h` (the
dev row), `Enemy.h` (the type and its stats).

## 6. Testing (`make test`, headless)

- **Sword**: the parry window's edges (0.15 s), no window on a spam press,
  block chip and guard break, combo damage and reset, riposte ×2.5, ray
  reflection, plunge radius.
- **Fairness**: every white attack is parryable and no gold one is; no tell
  under its floor in any phase or difficulty; each string has an escape a
  scripted dodger finds.
- **Anti-camp**: a scripted player on a shard, beyond 30 m, or behind a
  pillar is punished within 5 s in every phase.
- **Scripted fights**: the solid player (dodges well, parries half the white
  strikes, uses the sword) kills him in **150–180 s**; a dodge-only,
  guns-only player can still win, more slowly; a player who stands still dies
  within 30 s.
- **Flow**: the Leviathan's death saves the Act II clear before the Zenith
  loads; a death in the Zenith retries the Zenith.

Tuning is by numbers, like the other bosses; the user's hands-on play is the
real test.

## 7. Build order (one branch, `zenith`)

1. The sword (logic, model, input, HUD) with a dev way to try it.
2. The Zenith and the hand-off from the Leviathan.
3. THE ABSOLUTE, phase 1 and the colour rule.
4. Phases 2 and 3.
5. Sound, voice, music, polish, tuning.

Each step leaves `make && make test` green.
