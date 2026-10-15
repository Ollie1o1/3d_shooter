# "!!!!!" — THE ABSOLUTE overhaul — design

Date: 2026-10-14 · Status: agreed section by section with the user in a
brainstorm, written for review · Builds on
`2026-10-14-zenith-absolute-design.md` (shipped, main d8f1ee7)

## Decisions

- From the user:
  - He shouldn't stay on the ground: **he flies**.
  - The arena goes to a **darker sunset/night** setting.
  - More noticeable attacks; better wind-up sounds; punches that land like
    **boom boom boom**; a laser, a punch and every other attack must look and
    sound clearly different.
  - A **wider arsenal** of punches and attacks, as befits a humanoid.
  - Audio cues that **tell you when to parry and when to block/dodge**.
  - **Fix the sword animation** that sometimes goes the opposite way.
  - His **arms in a fighting stance**; **glowing red eyes**; **hands with a
    faint glow/outline**, like magic gloves.
  - **Remove the "BE JUDGED." line**; keep "KNEEL.", "TOO SLOW.", "AGAIN.",
    "...WORTHY.".
  - The **level is renamed "!!!!!"** (literally). The boss stays **THE
    ABSOLUTE**.
- Chosen in the brainstorm:
  - Flight: **dives from the air** — airborne often, but every melee blow
    lands at your height, so the sword stays the main weapon.
  - Setting: **blood sunset**.
  - Punches: **cosmetic boom** — heavy feedback on every blow, hit or miss;
    damage and fairness unchanged.
  - Flight is built **into the existing string machine** (an altitude plus
    air moves), not as a separate state machine.
- Unchanged: the colour rule (white = parry, gold = dodge/block, red = leave
  the area), the sword's controls and numbers, the phases' hp thresholds, the
  tell floors (0.35 s opener, 0.22 s follow-up), 150–180 s for BossSim's solid
  player, a whole string taken costing 60–70 of 100.

## 1. Flight

- **Altitude**: `abAlt` (height above the floor under him), eased toward
  `abAltWant` at up to 18 m/s (45 m/s in a dive). `abGround` and
  `absSafeSpot` set `y = floor + abAlt` instead of the floor.
- **Takeoff**: between strings (in place of a ground string from the
  rotation), a burst of light, a dust ring, a rising roar. He climbs to
  9–14 m and circles you at 14–20 m (XZ), clamped inside the floor's radius.
  One flight per ~3 strings in phase 1, per ~2 in phases 2–3.
- **Flight length**: at most **5 s** airborne, then he always comes down
  with an air move that lands (SKYFALL or METEOR) or a plain descent into a
  ground string. A test asserts he is never airborne longer than 6 s.
- **Air moves** (new `AttackKind`s, new strings, same colour rule):

| Move | Tell | What happens | Answer |
|---|---|---|---|
| **SKYFALL** | gold | A gold-rimmed circle (r 3.5) locks onto the floor under you for 0.7 s, then he crashes down at 45 m/s: crater, boom, a 3.5 m shockwave (28). He goes straight into a ground string from where he lands. | dash out of the circle |
| **METEOR** | white | A flare and a dive fist-first at your head (24). | parry: he's **knocked out of the sky**, staggered 1.5 s (×1.5) |
| **AIR RAY** | red | The ray, fired down from above. Pillars still block it. | strafe, or a pillar |
| **SERPENT** | white | The homing coil, from the air. | parry it back, or outrun it |

- **Shot down**: while airborne his guard is off (gunfire lands at any
  range). **6 % of his max hp in one flight** knocks him out of the sky: he
  spins down, crashes, staggered **1.5 s** (×1.5).
- **Shards aren't safe**: a shard camper gets SKYFALL onto the shard, in
  every phase (in place of the ray / ZENITH ray). Camping on the floor (far
  off, or one pillar) still gets TOO SLOW; anywhere else he can't reach on
  foot (up a pillar, past the edge) gets the AIR RAY in phase 1 and the
  ZENITH ray from phase 2.
- **Pillars**: at 15 m or higher he flies over them; lower, he blinks past
  them as dashes do. Never inside one (test).
- **Phases**: ASCENSION really lifts him ~12 m over the Dais for the 360°
  sweep (the sweep's beam stays at the player's chest height), then he dives
  back down. Phase 3 adds a **blade dive**: LUNGE from the air, straight down
  at you, gold.

## 2. The arsenal

New single moves (each its own pose, wind-up, cue and boom):

| Move | Tell | Damage | What it is |
|---|---|---|---|
| **CROSS** | white | 24 | straight rear-hand power punch; quicker tell than the hook |
| **BACKFIST** | white | 20 | spinning backhand, mostly straight after a teleport |
| **ELBOW** | white | 22 | short and fast, a string's closer |
| **ROUNDHOUSE** | white | 26 | wide spinning kick, 4 m reach: catches a back-step |
| **HAYMAKER** | gold | 32 | huge wound-up overhand, string ender, big knockback |
| **SHOULDER CHARGE** | gold | 24 | head down, a 14 m straight rush; sidestep it |
| **AXE KICK** | gold | 26 | heel straight down; leaves a small red crack (r 2) for 0.5 s |
| **CLAP** | red | 20 | both palms: a 6 m, 70° cone of air in front; step aside or behind |

New strings (tell times follow the existing table's style; phase 2 adds the
step marked `+2` and makes follow-ups 25 % quicker):

| String | Steps |
|---|---|
| **FLURRY** | JAB, JAB, JAB, JAB (+2), CROSS — all white, on an even beat |
| **BREAKER** | JAB (white), CROSS (white), HAYMAKER (gold) — stop parrying at the third |
| **CROSS-UP** | TELEPORT, BACKFIST (white), ROUNDHOUSE (white) |
| **BULLRUSH** | SHOULDER CHARGE (gold), ELBOW (white), CLAP (red) |
| **HEEL OF HEAVEN** | HOP, AXE KICK (gold), STOMP (red) |
| **SKYFALL** / **METEOR** / **AIR RAY** / **AIR SERPENT** | the air moves (section 1) |

Rotations: each phase's rotation gains the new strings and the flights;
phase 3 keeps blade strings mixed with fist strings. Two parries in one
string still break him. Mirages (phase 2) open any ground string.

**Voice**: "BE JUDGED." is removed from the lines table and the voice bank;
BE JUDGED and the last judgment play no line. The four other lines stay.

## 3. Blood sunset

The "!!!!!" `Theme`: a crimson-orange sun low on the horizon directly behind
the cracked eclipse disc (the disc a black silhouette, light leaking through
its cracks); sky gradient orange → wine → near-black indigo overhead, a few
stars; a warm, low key light (long shadows off the pillars); dim violet
ambient; dusky red-violet fog. The floor stays black stone, so his white and
the red seams are the brightest things there. Gold tell colours shift toward
a paler gold so they don't sink into the orange sky. Data only: `Theme`
values and emissive colours; no renderer change.

## 4. Sound: fight by ear

**The cue**: separate from the wind-up, every telegraphed move emits
`ev.abCue` when its remaining tell reaches **0.12 s** (the strike's real
landing, so a delayed strike's cue is late too). A parry pressed on the cue
is open (the sword's window is 0.15 s) when the strike lands.

| Colour | Wind-up (over the tell) | Cue | Means |
|---|---|---|---|
| white | an airy rising swish per move (jab short/tight, hook wide, backfist and roundhouse a spinning whirr, cross a sharp pull) | a bright crystal **TING** | parry |
| gold | a deep growling swell, a dragged gong | a low **DOOM** knock — never a ting | dodge or block |
| red | rays: a whine that climbs through the charge, then a crackling beam lasting exactly the beam; ground areas: a rumble with seam crackle | a sharp double pulse | get out |

- **Impacts**: every punch and kick lands with a layered **BOOM** (a
  sub-bass drop, a crack transient, a long open-sky tail), hit or miss, with
  a per-move variant (jab tight, cross heavier, hook wide, haymaker huge and
  driven, kicks thuddier, axe kick with stone crack) and ±4 % pitch so a
  FLURRY drums. A volume floor keeps them audible.
- **Flight**: a takeoff roar with a choir swell; a wind loop following him
  (`moveSource`) while airborne; SKYFALL's falling whistle then a crater
  boom; shot down: a crack, a falling wail, a crash.
- **Your side**:
  - parry: a big metallic CLANG + shimmer, a short hit-pause, the music
    dipping; a break adds a glass shatter and a choir hit;
  - block: a heavy clank whose pitch drops as the guard drains; under 30 %
    guard, a creak-and-crack on each blocked hit; guard break: a big crack;
  - timing: a parry press with nothing landing within 0.25 s after it shows
    **EARLY**; a white strike landing within 0.25 s after a window closed
    shows **LATE**; both with a dry *tsk*;
  - a parry tried on a gold strike: a "wrong" buzz and a small
    **UNPARRYABLE** flash.
- **Mix**: cues are priority with a floor; the music ducks ~2 dB through each
  tell. Teleports get their own crackling zap instead of the shared `dash`.
- All new sounds are generated in `tools/gen_sfx.py` and listed in
  `MixTable.h`, as the existing ones are.

## 5. His look

- **Fighting stance**: idle and between moves, a boxer's guard — fists up by
  the chin, lead hand forward, body bladed, a bounce on his toes. In the air
  a hover pose: arms swept back, legs trailing.
- **Red eyes**: two glowing crimson eye slits replace the gold face slit;
  they bloom, leave a faint streak on teleports and dashes, and brighten
  through a tell.
- **Gauntlets**: slightly bulkier hands with faintly glowing rune bands at
  the knuckles and wrists, pale violet-blue (none of the tell colours), and a
  thin mote trail. On a wind-up the striking gauntlet flares to the tell
  colour.
- **Impacts**: a light burst at the fist, a ring of air, dust, distance-scaled
  shake, a 2-frame hit-pause when you're within 6 m.
- Every new move gets its own pose (`AbsoluteRig.h`).

## 6. The sword fix

`gunkit::swordPose`'s overhead (stroke 2) swings its pitch from −60° (point
down) to +50° (point up) while the hand drops — the tip flicks up as you
chop, opposite to its trail. It becomes +60° → −50°: the point starts raised
and finishes down in front, its edge leading. The side strokes are checked
the same way in the build.

## 7. The name "!!!!!"

The arena's name, the level-select row, the victory card (**"!!!!!
CLEARED"**) and the best-time label read "!!!!!". The PixelFont has the
glyph. Code identifiers (`ZENITH_ARENA`, `level.zenith`, the ZENITH music
track) keep their names.

## 8. Testing (`make test`)

- Every move has a wind-up, cue and boom sound, and every named sound exists
  in the generated set.
- The cue fires 0.12 s before every strike lands, delayed strikes and every
  phase included; no gold move ever plays the ting.
- Flight: shot down at 6 % in one flight; never airborne over 6 s; never
  inside a pillar; never past the floor's edge.
- Each new string has an escape the scripted dodger finds; whites parryable,
  golds not; no tell under its floor.
- The overhead stroke's point moves downward over the stroke (a pose test).
- BossSim: the solid player in 150–180 s; guns-only still wins; standing
  still dies within 30 s.
- Audio level check of a `--audiodump` capture; native screenshots of the
  sunset, the stance, the eyes and gauntlets, and each sword stroke.

The user's hands-on play (and listening) is the real test.

## 9. Build order (branch `zenith-overhaul`)

1. The sword fix + the "!!!!!" rename + the line removal.
2. The sunset and his look (stance, eyes, gauntlets).
3. Flight and the air moves.
4. The arsenal (new moves and strings).
5. Sound: wind-ups, cues, booms, your-side feedback, the mix; tuning.

Each step leaves `make && make test` green.
