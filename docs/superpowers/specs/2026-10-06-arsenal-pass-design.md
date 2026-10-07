# The arsenal pass — design

Date: 2026-10-06 · Status: approved in conversation (approach 1, sections 1–4) · Feel track, piece C of 5

## Why

The four guns were built at different times in four different styles: a
gunmetal revolver with glowing yellow strips, a grey-and-wood shotgun sitting
mostly off-screen, a wooden WWII Kar98, a plain grey Longshot. Each sits at its
own place on screen with its own recoil, and the gunshots are raw recordings
that don't sit with the game's synthesized sound. The user asked for the gun
aesthetics and animations to be consistent with one another, so the game has one
vibe and feel.

**Direction (feel track): "dark machine + glow".** Success: any two guns side by
side look like one arsenal, move by the same rules and sound like one family,
while each still feels like itself.

## Decisions made with the user

- **Same four roles, restyled.** What each gun does, its stats and upgrades are
  untouched. The **Kar98 is renamed the LANCER** (a WWII name clashes with the
  look); same stats.
- **Each gun glows its own colour**: revolver amber, shotgun ember red, Lancer
  cyan, Longshot violet.
- **Gunshots are layered**: the real recorded crack + a synthesized thump + a
  metallic machine ring in the gun's pitch + a tail.
- **Approach**: a shared parts kit with a recipe per gun, and one motion grammar
  driven by a few numbers per gun (not four hand-tuned draw functions).

## 1. The look

- **One body**: blackened gunmetal (≈0.10), worn steel edges (≈0.32), bone grip
  panels (0.76, 0.71, 0.61), brass only for small details.
- **One glow per gun**, used for its accent strips, muzzle flash, tracer, shell
  sparks, impact sparks and its HUD slot:

  | Gun | Glow (emissive) | Weight |
  |---|---|---|
  | Revolver | amber (1.0, 0.68, 0.22) | light, snappy |
  | Shotgun | ember red (1.0, 0.28, 0.10) | heavy, short |
  | Lancer | cyan (0.30, 0.90, 1.00) | medium, long |
  | Longshot | violet (0.70, 0.35, 1.00) | heavy, long |

- **Ammo you can read on the gun**: glowing cells, one per round of the current
  magazine size (upgrades add cells), lit for the rounds left, dark when spent,
  relit one by one during a reload. Revolver: its chambers on the cylinder face.
  Shotgun: a shell window on its right side. Lancer: a strip along the receiver.
  Longshot: a row on the scope.
- **Distinct silhouettes**: compact revolver with a drum; short, wide pump
  shotgun; long, slim bolt rifle; huge Longshot with scope and muzzle brake.
- **One framing rule**: every gun's hip position is in the same lower-right
  region, similar visual size, barrel converging on the crosshair. The shotgun
  moves up into view. The rifles' aimed positions (iron sights, scope) stay.
- **Fist and grapple** in the same kit: a blackened gauntlet with bone knuckles
  and a seam glowing in the current gun's colour; the grapple launcher on the
  left wrist.

## 2. How they move

One motion grammar for all four; each gun supplies a weight row:
`kickBack, kickUp, pitchKick, roll, settle (s), swayAmp, switchDown (s), switchUp (s)`.

- **Fire**: kick back/up in one frame, then a critically damped spring settles it
  over `settle` (revolver ~0.12 s … Longshot ~0.45 s); heavy guns roll. Glow strip
  and muzzle flare in the gun's colour; one cell goes dark.
- **Cycling** after each shot, on the same beat as its sound: revolver hammer +
  cylinder click, shotgun pump, Lancer/Longshot bolt.
- **Switch** (the signature beat): the current gun drops out down-right
  (`switchDown` ≈ 0.12 s), the new one rises from below with a small settle
  overshoot (`switchUp` ≈ 0.2 s) while its cells light up in sequence with a
  rising chime. Total ≈ 0.32 s.
- **Reload**: three beats for every gun, as fractions of its reload time —
  **OPEN** (0–25 %: tilt toward centre, action opens, glow flickers), **FEED**
  (25–85 %: cells relight one at a time, evenly spaced, a click each), **CLOSE**
  (85–100 %: snap back, glow pulse). The existing detailed reloads (cylinder
  swing-out, shell-by-shell feed, stripper clip) keep their content, retimed into
  these beats.
- **Empty**: cells dark, accent dimmed; firing plays a dry click.
- **Moving and idle**: a slow breathing sway; the shared walk bob; tilt in on a
  dash; dip and roll on a slide; a landing dip scaled by fall speed; the grapple
  launcher kicks on the left wrist.
- **Inspect (V)**: turn to show the side, run the cells, pulse the glow.

## 3. The sound

- **`tools/gen_arsenal.py`** (new) builds every gun sound from one recipe. It reads
  frozen copies of today's recorded shots in `assets/sfx/src/` (copied once;
  re-running never stacks layers). The game never loads `src/`.
- **A shot = four layers**: crack (the recording's transient, trimmed so the
  shot is never late) + thump (a falling sine: revolver 90→50 Hz, shotgun 70→40,
  Lancer 80→45, Longshot 55→30; longer for heavier) + machine ring (a few damped
  inharmonic partials in the gun's pitch — the family sound) + tail (filtered
  noise decay; the Longshot a long distant boom). **Three variants per gun**
  (`revolver_1..3.wav` etc.; the sound engine rotates them).
- **Mechanical foley, one recipe pitched per gun**: cycling (hammer/cylinder,
  pump rack, bolt up/back/forward), reload open/close, a **cell** tick per relit
  cell, a **switch_up** chime in the gun's glow pitch, a **dry** click. Existing
  names stay (`reload`, `bolt`, `pump`, `cyl_open`, `cyl_close`, `eject`,
  `shell_in`); new ones: `switch_up`, `cell`, `dry`.
- `assets/sfx/CREDITS.md` notes the layering.
- **Mix**: in an `--audiodump` of a firing sequence the four shots rank by weight
  in loudness and nothing clips.

## 4. Code and testing

- **`src/GunKit.h`** (new, no GL): materials, glow table, part builders (cells,
  muzzle, grip, glow strip) and each gun's recipe → a list of boxes (gun space)
  that `ViewModel` draws. Also the fist/gauntlet recipe.
- **`src/GunMotion.h`** (new, no GL): the weight table and one motion state (kick
  spring, switch phases, reload beats, sway, landing dip) → offsets/angles for the
  view model, plus **sound cues with their times** (replacing the hand-placed
  reload cue table in `Gameplay_Combat.h`).
- **`src/ViewModel.h`**: shrinks to framing (one shared hip rule, aimed positions
  for the rifles) + calls into the kit and motion; keeps the rifles' detailed
  reload parts by expressing them as kit parts driven by the beat clock.
- **`src/Weapons.h`**: `"KAR98"` → `"LANCER"`; `WeaponId::KAR` may stay as the
  internal id. The armory, DAILY's "THE KAR98 ONLY" → "THE LANCER ONLY", the
  README and comments follow.
- Effects (muzzle flash, tracer, shell casing, impact sparks) take the gun's glow
  colour; HUD weapon slots are tinted with it; the dry click on an empty trigger.
- **Tests (`tests/test_game.cpp`)**: every gun's boxes sit inside its framing
  volume at the hip; each gun has exactly one cell per round of its magazine
  (with and without MAG upgrades) and the lit cells equal the ammo; the kick
  settles within `settle`; a switch lasts ≈0.32 s and ends at rest; the reload
  beats sum to the reload time and FEED relights cells evenly; sound cues fire
  once each, in order; the renamed gun's stats are unchanged.
- **Checks**: screenshots of each gun at the hip, aimed, mid-reload, mid-switch,
  and the fist (`--weapon N`, `--aim`, overlays as needed); an `--audiodump` of a
  firing sequence; `--bench` against `main`; `make web`.

## Out of scope

New guns, fire modes or balance changes; enemy voices (piece D); the
Warden/Sovereign pass (piece E); recording new source sounds.
