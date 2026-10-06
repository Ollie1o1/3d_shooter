# Sound engine — design

Date: 2026-10-06 · Status: approved in conversation (sections 1–2) · Feel track, piece A of 5

## Where this fits

The user asked for four things alongside Act II piece 4: better audio (game
audio, enemy sounds, effects), guns whose look and animations are consistent,
and bosses that are hard, fun and challenging. Agreed order, each with its own
spec → plan → build:

- **A. Sound engine** (this spec) — positional mixing, variation, priorities,
  ducking, per-arena reverb. No new sounds.
- **B. Act II piece 4** — the Descent + the Penitent, built to the boss bar
  below, using the new engine from day one.
- **C. Arsenal pass** — one design language, animation grammar and sound family
  for the four guns, fist and grapple.
- **D. Enemy voices + mix** — a voice set per enemy type, then a full mix.
- **E. Boss pass** — the Warden and the Sovereign brought up to the same bar.

**Direction chosen by the user: "dark machine + glow".** Blackened gunmetal and
bone, one emissive accent per gun that reads its state; heavy mechanical
transients with a synth layer underneath, sitting in the darksynth soundtrack.
Every later piece follows it.

**Boss bar** (applies to B and E, and the Leviathan later): every attack has a
readable tell (light *and* sound), phases change the fight, camping and
range-cheese are punished, and there is always a skilful answer (parry, dash,
grapple, headshot window).

## Today

- `AudioSystem::play(name, vol)` → `Mix_PlayChannel` on 32 SDL_mixer channels.
  No position, no pan, no variation; 82 call sites, a few fake distance by
  scaling the volume by hand (`Gameplay_Flow.h:601`, `Gameplay_Combat.h:145,183`).
- Music is synthesized live by `MusicSynth` inside `Mix_HookMusic`'s callback.
- Settings: `audioVolume` (master) and `musicVolume`.

## 1. What the player hears

- **Positional sound.** A sound placed in the world falls off with distance and
  pans to its side (equal-power). Sounds behind the listener are slightly
  darker (low-pass), and distance darkens too, so front/back and near/far read.
  Moving sources (a dashing Raptor, a projectile) can be moved while playing.
- **Variation.** Every play gets a small random pitch and gain jitter (default
  ±4 % pitch, ±1.5 dB; UI sounds ±1 %). A name with numbered variants on disk
  (`name_1.wav`, `name_2.wav`…) picks one at random, avoiding an immediate repeat.
- **Priority and limits.** Each sound belongs to a group with a voice cap:
  PLAYER 12, ENEMY 20, WORLD 12, UI 4 (48 voices total). When a group is full
  the quietest current voice in it is stolen. Voices marked `priority`
  (player's weapon, boss attacks) are never stolen by a non-priority sound.
- **Ducking.** `duck(amount, seconds)` dips the music and the WORLD/ENEMY groups
  (attack 20 ms, hold `seconds`, release 250 ms). Used on player damage (−5 dB,
  0.15 s), wave start (−4 dB, 0.4 s), boss telegraphs/roars (−6 dB, 0.5 s).
  Overlapping ducks take the deepest.
- **Space.** A reverb send whose preset follows the current place:

  | Preset   | Used in                          | Character                 |
  |----------|----------------------------------|---------------------------|
  | OPEN     | Yard, menus                      | nearly dry, short         |
  | METAL    | Foundry, Spire, Core, FAST rooms | bright, medium            |
  | HALL     | Sanctum, Drowned Nave, Orrery    | long, dark, cathedral     |
  | SHAFT    | Descent (piece B)                | tight, fluttery, echoing  |

  Presets change with a 1 s crossfade of the reverb parameters. The UI group
  is never sent to the reverb.

Out of scope: new or replaced sounds, enemy voices, an SFX volume slider,
occlusion through walls, doppler.

## 2. How it's built

### `src/SfxMixer.h` (new, no SDL — unit-testable)

- **Bank**: `int addSample(std::vector<float> mono)`; samples are mono float at
  the mixer rate (44 100 Hz). Name → ids (variants) lookup lives here too.
- **Voice**: sample id, fractional playhead, pitch, base gain, group, priority,
  `positional` flag + world position, smoothed current L/R gain and filter
  state, handle id (generation-counted so a stale handle is ignored).
- **Listener**: position, right vector, forward vector.
- **Spatialization per voice, per buffer**:
  - distance gain `g = ref / (ref + rolloff·max(0, d − ref))`, ref 4 m,
    rolloff 1; silent (voice culled from mixing, not freed) beyond 90 m.
  - pan `p = dot(normalize(src − listener), right)`; L = cos((p+1)·π/4),
    R = sin((p+1)·π/4).
  - one-pole low-pass, cutoff from 18 kHz (near, in front) down to ~2.5 kHz at
    90 m; behind the listener (dot with forward < 0) lowers it by up to half.
  - target gains are interpolated linearly across the buffer (no zipper noise).
- **Mixing**: linear-interpolated resampling at `pitch`; dry sum per group →
  group gain (duck applies to WORLD/ENEMY) → master; reverb send per group
  (PLAYER 0.15, ENEMY 0.3, WORLD 0.35, UI 0) → reverb → master.
- **Reverb**: Freeverb-lite — 4 parallel damped combs + 2 series allpasses per
  channel, delay lines preallocated for the largest preset.
- **Output**: soft clip (`tanh`-style) at the master so overloads round off
  instead of wrapping; final values always within ±1.
- **Commands**: the game thread pushes `Play`, `Move`, `Stop`, `Listener`,
  `Space`, `Duck` into a fixed-size single-producer/single-consumer ring
  (atomics only). The audio thread drains it at the top of `render()`.
  No locks, no allocation on the audio thread. A full ring drops the command.
- **RNG**: a small xorshift owned by the game-thread side, so variation is
  chosen when the command is made (deterministic in tests with a fixed seed).

### `src/AudioSystem.h` (front end)

- Loads WAVs with `SDL_LoadWAV` + `SDL_AudioCVT` → mono float 44.1 kHz into the
  bank; also loads `name_N.wav` variants when present.
- The existing `Mix_HookMusic` callback renders music (`MusicSynth`) then
  `SfxMixer::render` adds into the same buffer before format conversion.
  `Mix_PlayChannel` and `Mix_Chunk`s are no longer used for effects.
- API (volume stays 0–128 so existing calls keep working):
  - `play(name, vol = 128, group = PLAYER)` — non-positional; menu/HUD call sites pass UI
  - `SoundHandle playAt(name, pos, vol = 128, group = WORLD, priority = false)`
  - `moveSource(handle, pos)`, `stop(handle)`
  - `setListener(const Camera&)` — called once per rendered frame
  - `setSpace(ReverbPreset)` — on arena change
  - `duck(dB, seconds)`
- Without SDL_mixer (`HAS_SDL_MIXER 0`) every call stays a no-op, as now.

### Call sites

- Enemy, projectile, door, hazard, explosion, pickup and arena-shift sounds
  move to `playAt` with their real position (group ENEMY or WORLD); the
  hand-written distance volume scaling is removed.
- The player's own movement, weapons, parry, HUD, menus and wave/announce
  sounds stay `play` (group PLAYER or UI).
- Boss (Warden, Sovereign) attack sounds: ENEMY group, `priority`.
- Ducks added on player damage, wave start, boss telegraphs.
- Projectiles that already make a sound when fired keep one-shot `playAt`;
  no looping per-projectile sounds in this piece.

### Web

Same path; the web build already uses the music hook. Buffer stays 2048
frames (~46 ms), unchanged latency. CPU budget: 48 voices + reverb is well
under 1 ms per buffer natively; check with `--bench` on the web build.

## 3. Testing and verification

New tests in `tests/test_game.cpp` driving `SfxMixer` offline:

- source on the listener's right → right channel RMS > left; on the left → the reverse
- same source behind vs in front → behind has less high-frequency energy
- RMS falls monotonically over 4, 15, 40 m; beyond 90 m → silent
- group cap: filling ENEMY then playing a louder sound steals the quietest; a
  priority voice is never stolen by a non-priority play
- pitch jitter stays within its bound over 1000 plays (fixed seed)
- duck: group gain dips by the requested dB and returns to unity after release
- no voices → output exactly 0; heavy overload → no sample outside ±1, no NaN
- stale handle `moveSource` is ignored
- command ring: a full ring drops without corrupting later commands

Dev: `--audiodump FILE.wav SECONDS` writes the final mix (music + effects) to a
WAV during a scripted run (`--spawn`, `--cam`), for checking levels and panning
numerically. Always run `make && make test` and `make web`.
