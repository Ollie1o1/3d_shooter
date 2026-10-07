# Enemy voices + the mix — design

Date: 2026-10-06 · Status: approved in conversation (approach 1, sections 1–3) · Feel track, piece D of 5

## Why

Every enemy sounds the same. All fifteen types (bosses included) share one
generic `telegraph` beep on a wind-up, one `enemy_death` and one `spawn`; they
make no sound while they stalk you, and a Husk winding up sounds like a
Juggernaut. The mix is each call site's own volume number (~80 of them), never
balanced against the rest, so a crowded wave would bury the cues that matter.

The user asked for "how enemies sound and different effects" as part of the
feel track. **Success:**

- you can tell what an enemy is, and what it is about to do, by ear — including
  one out of sight (the boss bar's "a light *and* a sound tell");
- it fits "dark machine + glow";
- in a crowded wave the tells still cut through; guns, enemies, world, music
  and UI sit together without mush or clipping.

## Decisions made with the user

- **Character: machine + choir.** Machine bodies (servos, vents, gears,
  hydraulics) carrying a hollow voice inside: synthesized vowel "choir" moans,
  chants and screams through metal.
- **Mix: threat-aware.** Every enemy sound has a role; tells always cut
  through, chatter is capped to the nearest few and steps back under tells,
  the music dips a little under tells; plus a loudness pass of every sound.
- **Approach 1**: an offline-generated voice bank (`tools/gen_voices.py`), one
  voice table per type and a `VoiceDirector` in a GL- and audio-free header,
  roles in the mixer. (Not live synthesis; not hand-placed calls.)
- Assumed and not objected to: voices are generated, not recorded; no
  gameplay changes apart from tells becoming audible; the Warden and
  Sovereign get voices here, their fight changes wait for piece E.

## 1. The voices

Pitch follows size (small = high and quick, heavy = low and slow). Every type
has up to eight **kinds**, each with **three variants** (`name_1..3.wav`; the
mixer rotates variants already):

| Kind | What it is |
|---|---|
| `spawn` | a short rising breath + the type's machine signature |
| `idle` | a low hum or murmur, every few seconds |
| `move` | footfalls or engine: servo steps, skitter, wingbeats, treads |
| `tell` | the wind-up, per type and per attack kind where they differ |
| `attack` | the release: a shot, a lunge, a slam |
| `hurt` | a short choked note when hit |
| `death` | the choir cut off, then the machine winding down |
| `special` | one-offs: a halo or armour breaking, a fuse, a beam held |

| Enemy | Choir (vowel, register) | Machine body | Signature tell |
|---|---|---|---|
| Husk | "oh" murmur, tenor | servo steps | a rising hum before its orb shot |
| Ripper | breathy "ah" pant, high | skittering claws | a sharp hiss into the lunge |
| Sentinel | none — a cold machine | tripod hydraulics | a whine climbing over the laser paint, a click at fire |
| Raptor | shrill "ee" cry | wingbeats | a falling shriek as it dives |
| Brute | bass "uh" grunt | heavy servos | a huge inhale, then a gear grind before the slam |
| Mite | none | tiny skitter | a fuse beep that speeds up as it closes |
| Juggernaut | deep "oo" drone | treads, plating | a siren rising before a shell; a ratchet before the smash |
| Shieldbearer | chanted "ah", baritone | shield scrape | a shield clang, then a war cry into the bash |
| Conduit | a held choir chord | electrical crackle | the chord swells before it spawns |
| Conductor | a high hymn | hum, tether buzz | a tone as it links a tether |
| Seraph | angelic "ah" swell, soprano | wingbeats, glow hum | the chord brightens through the charge; a held tone while the beam is on |
| Anchor | very low "oo" | grinding | a muffled thump while its field is up; a lob "pop" |
| Warden | massed low choir | gears, vents | its own chord per attack kind (volley, slam, summon); a roar when it enrages |
| Sovereign | one solo baritone | armour, blade ring | a different sung syllable per attack (sweeps, cleave, dash, leap, blink, whirl, thrust, phantoms) |
| Penitent | a monk choir's chant | chains, censers | replaces today's tells: a low groan before a low sweep, a bell tone before a high one, a chant in the scourge |

**Hollowed variants** reuse their base voice, processed at play time (no new
files): **Enraged** pitched up ×1.12 with drive; **Twinned** doubled, the copy
detuned ~15 cents and 12 ms late; **Haloed** with a shimmer layer, plus a glass
shatter (`halo_break`) when the halo breaks.

**Generation:** `tools/gen_voices.py` (new) builds every file from shared
blocks — a formant choir (vowel formants over a sawtooth/pulse source, several
detuned voices), servo, vent, gear grind, metal resonance (damped inharmonic
partials), noise beds. Files are `v_<type>_<kind>[_<attack>]_<n>.wav`, mono.
Chatter (`idle`, `move`) is written at 22.05 kHz, everything else at 44.1 kHz,
so the web data grows by no more than ~1.5 MB. `assets/sfx/CREDITS.md` notes
the generator. Deterministic: a fixed seed, so re-running gives the same files.

## 2. Who gets to speak: the VoiceDirector

`src/EnemyVoice.h` (new; no GL, no audio device) holds:

- **`VoiceTable`**: per `EnemyType`, the base sound name of each kind, a tell
  and attack name per `AttackKind` where the type has several (Warden,
  Sovereign, Penitent, Juggernaut), the move cadence (steps/s at full speed)
  and the idle interval.
- **`VoiceDirector`**: each frame the game passes the living enemies (uid,
  type, hollow, position, speed, state, this tick's `EnemyEvents`, whether it
  was hit and by how much) and the player's position. It returns a short list
  of **`VoiceCue{name, pos, uid, role, volume, pitch, sustained}`**:

| Role | Rule |
|---|---|
| **TELL** | every wind-up within 40 m, at priority, out of sight included. No global cooldown (today's 0.22 s goes); the same enemy can't repeat a tell within 0.15 s; more than 4 starting in one frame → the nearest 4. |
| **ACTION** | attack releases and hurts. Hurt: once per enemy per 0.4 s and at most 6 a second overall (a shotgun blast is one hurt). |
| **DEATH** | always, with today's level floor (0.5). |
| **CHATTER** | idle, move, spawn — only from the **nearest 4** within 25 m. Idles every 3–7 s (random per enemy); move steps at the type's cadence while moving; a fresh spawn always gets its spawn cue. |

- **Bosses** skip the chatter cap and use their own rows; their big tells
  still call `duck()` as today.
- **Sustained** sounds (the Seraph's beam, the Mite's fuse, the Anchor's
  field) follow their enemy via `moveSource` and are stopped when the state
  ends or the enemy dies.
- Per-uid state lives in a small map pruned of uids no longer alive, so it is
  correct across retries, waves and Twinned splits.

## 3. The threat-aware mix

- **Roles in the mixer**: `SfxMixer::Opts` gains `role`
  (`SoundRole{CHATTER, TELL, ACTION, UI}`; default ACTION), alongside `group`.
- **Sidechain**: while any TELL voice is playing, CHATTER is pulled down
  **8 dB** and the music **3 dB** (attack 30 ms, release 250 ms). Separate from
  `duck()`, which keeps its job for boss moments.
- **Caps**: CHATTER has its own cap of **6** voices inside ENEMY. A TELL may
  steal a CHATTER voice, never another TELL.
- **One table of levels**: `src/MixTable.h` (new) gives every sound a role
  target and a trim, measured by a new `--mixreport` dev flag (peak and RMS of
  every loaded sound, and the resulting level). Order, loudest first: player
  guns and player hits → tells → attacks and deaths → world → chatter well
  below; UI fixed. Call sites stop carrying their own volume numbers: they pass
  the sound and its context (distance-based volume stays the mixer's job).
- **Master**: a gentle bus compressor (threshold −10 dBFS, ratio 2:1,
  attack 10 ms, release 150 ms) ahead of the existing soft clip, so a big wave
  doesn't pump.
- **The generic cues go**: enemy uses of `telegraph`, `enemy_death` and
  `spawn` are replaced by each type's own. `telegraph` stays for UI
  countdowns, purchases and hazard warnings.

## 4. Code and testing

### Code

- `tools/gen_voices.py` — the voice bank (section 1).
- `src/EnemyVoice.h` — `VoiceTable`, `VoiceDirector`, `VoiceCue`.
- `src/MixTable.h` — per-sound role targets and trims.
- `src/AudioTypes.h` — `SoundRole`.
- `src/SfxMixer.h` — `role` in `Opts`, the CHATTER cap and steal rule, the
  tell sidechain on chatter and music, the bus compressor; introspection for
  tests (`roleActive(role)`, `sidechainLevel()`, `musicGain()`).
- `src/AudioSystem.h` — load the voice bank (a name list built from the
  table, not hand-typed), `playCue(const VoiceCue&)`, trims from `MixTable`;
  `--mixreport`.
- `src/Gameplay_Combat.h` — the enemy loop feeds the director and plays its
  cues (replacing the `telegraph`/`enemy_death`/`spawn` uses for enemies);
  `Gameplay_Sovereign.h`, `Gameplay_Penitent.h` and the Warden's events use
  their boss rows.
- `Makefile` — new headers in `HEADERS` and the test deps.

### Automated tests (`tests/test_game.cpp`)

- **Table**: every `EnemyType` has spawn, tell, attack, hurt and death names;
  every `AttackKind` a boss uses resolves to a tell; every name the table can
  produce exists in the bank on disk (with its 3 variants).
- **Director**: a wind-up 35 m away behind a wall gives one TELL; two wind-ups
  from one enemy 0.1 s apart give one; 6 in a frame give the nearest 4;
  10 enemies within 25 m → chatter only from the nearest 4; a shotgun's
  8 pellets on one enemy → one hurt; a death always cues; a dead enemy's
  sustained sound is stopped; state is pruned when a uid goes away; Hollowed
  variants change pitch/processing and not the name.
- **Mixer**: while a TELL plays the chatter level drops ~8 dB and the music
  ~3 dB, and both recover within ~0.3 s after; a 7th chatter voice steals the
  oldest chatter, never a tell; a tell steals chatter when ENEMY is full; the
  compressor's gain reduction is 0 on quiet input and at most ~5 dB on a
  full-scale burst.
- **Mix**: every loaded sound lands within ±2 dB of its role target after trim
  (from the same measurement as `--mixreport`).

### Manual

- `--audiodump` of a full Act II wave and of each boss fight: tells audible
  above the rest (dump with tells soloed vs the full mix), the soft clip
  touching no more than the gun attacks (< 0.5 % of samples).
- Listen through each type with `--spawn N`; screenshots not needed.
- `--bench` against `main` (the director and compressor within ~0.1 ms);
  `make web`, web data growth ≤ 1.5 MB.

## Out of scope

Changes to how bosses fight (piece E); new enemies; recorded voice actors;
subtitles or captions; music changes beyond the sidechain dip.
