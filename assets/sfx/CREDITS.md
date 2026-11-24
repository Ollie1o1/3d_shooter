# Sound credits

The enemy voices are not files: they are synthesized in code when the game loads
(`src/VoiceSynth.h`), with no recordings.

Most sounds are synthesized by `tools/gen_sfx.py`. The arsenal's sounds (the four
gunshots, three variants each, and the reload / bolt / pump / cylinder / cell / dry /
switch foley) are built by `tools/gen_arsenal.py`: each gunshot is its whole CC0
recording below (frozen in `assets/sfx/src/`) with a synthesized thump under it and a
faint metallic ring in the gun's pitch; each piece of reload foley is its original
recording with a faint ring in its gun's pitch. The ones below are built
by `tools/import_sfx.py` from free recordings released under **CC0** (public
domain: no attribution required, credited here anyway).

| Game sound | Source recording | Pack (OpenGameArt) |
|---|---|---|
| `revolver` | CZ-52 pistol shot | [Gunshot Sounds](https://opengameart.org/content/gunshot-sounds) |
| `shotgun` | shotgun shot | [Gunshot Sounds](https://opengameart.org/content/gunshot-sounds) |
| `kar` | Mosin-Nagant rifle shot | [Gunshot Sounds](https://opengameart.org/content/gunshot-sounds) |
| `longshot` | Mosin-Nagant, pitched down, over a synthesized boom | [Gunshot Sounds](https://opengameart.org/content/gunshot-sounds) |
| `dash` | heavy swish, slowed, with an air layer | [Swishes Sound Pack](https://opengameart.org/content/swishes-sound-pack), [100 CC0 SFX #2](https://opengameart.org/content/100-cc0-sfx-2) |
| `grapple_fire` | light swish + metal clink | [Swishes Sound Pack](https://opengameart.org/content/swishes-sound-pack), [100 CC0 SFX #2](https://opengameart.org/content/100-cc0-sfx-2) |
| `punch` | light swish | [Swishes Sound Pack](https://opengameart.org/content/swishes-sound-pack) |
| `step1`–`step4` | footsteps (two recordings, two pitch variants) | [100 CC0 SFX #2](https://opengameart.org/content/100-cc0-sfx-2) |
| `clank` | metal clang + metal hit | [Metal clang sounds](https://opengameart.org/content/metal-clang-sounds), [100 CC0 SFX #2](https://opengameart.org/content/100-cc0-sfx-2) |

The sword, normal/heavy parries, body/armour hits and stone/metal/wood/ice
impacts are authored by `tools/gen_combat_audio.py` (three deterministic variants
each). They combine synthesized air, low body, inharmonic plate modes and grit.
Blocks and parries also use the existing CC0 clank texture, frozen as
`src/clank_rec.wav` from the processed `clank.wav`; its recordings come
from the same [Metal clang sounds](https://opengameart.org/content/metal-clang-sounds)
and [100 CC0 SFX #2](https://opengameart.org/content/100-cc0-sfx-2) packs used by
`tools/import_sfx.py`. Run this generator after the older generators/importer.
The faster techno/DnB soundtrack is original oscillator/noise synthesis in
`src/audio/MusicSynth.h`; no commercial samples or music were downloaded.

Sword contact revision: `sword_hit`, `sword_heavy`, `sword_pierce` and
`sword_block` use actual blade-on-blade recordings by **Ben Jaszczak & Brian
Nelson / Still North Media**, released under **CC0**, from
[Medieval sound effects — Weapon impacts](https://opengameart.org/content/medieval-sound-effects-weapon-impacts).
The source release is `medieval_sfx_weapon_on_weapon_1_of_2.7z`.
Three isolated strikes are frozen as mono 44.1 kHz PCM in `src/sword_steel_0.wav`
through `src/sword_steel_2.wav`: Norse Sword Katana Blade on Blade, starting at
4.886893 s and 19.644785 s, and Sabre Norse Sword Blade on Blade, starting at
14.232154 s (0.68 s each). `gen_combat_audio.py` applies a small pitch variation,
rumble removal, transient control and tail fades. These contact cues contain
no synthesized boom, noise crunch or oscillator-based metallic substitute.

Chapter 6's 30 cue families (`shep_*` and `ring_*`, three variants each) are
original deterministic synthesis from `tools/gen_shepherd_audio.py`: short
inharmonic steel, low mechanical body and filtered friction. They also use the
same frozen CC0 `src/clank_rec.wav` texture credited above, resampled with
interpolation and filtered. Capsule double knocks, hull creaks, sweep warnings,
debris, gaze discharge, beam ignition, heavy body contacts and load-bearing
torso/arm movement have separate recipes. No new external
recordings or music are shipped by this pass.

The attack-signature revision also uses the frozen CC0 `pump_rec.wav` and
`cyl_close_rec.wav` recordings credited above for discrete winch teeth, latches
and idle clunks, and the three credited `sword_steel_*.wav` strikes for cage and
heavy-contact fractures. These are short contact transients; their tails are
original inharmonic/physical-mode synthesis. Idle and attack wind-ups contain
no sustained filtered-noise bed.

## Recorded core combat sounds (November 23 2026)

`explosion`, `slam`, `hit_body`, `hit_armor`, `player_hit`, `impact`, `impact_metal`,
`impact_wood`, `impact_ice`, `land` and `land_heavy` (three variants each) are no
longer synthesized. `tools/build_real_sfx.py` layers real recordings (a transient,
a body, a tail or debris layer), compresses, limits and decays them, and levels each
family to its mix class. The recordings are frozen (mono 44.1 kHz) in
`assets/sfx/src/cc0/`; every one is **CC0** from OpenGameArt:

| Frozen as | Pack |
|---|---|
| `bang_*`, `cannon_*` | [25 CC0 bang / firework SFX](https://opengameart.org/content/25-cc0-bang-firework-sfx) |
| `scifi_explosion_*` | [50 CC0 Sci-Fi SFX](https://opengameart.org/content/50-cc0-sci-fi-sfx) |
| `chunky_explosion` | [Chunky Explosion](https://opengameart.org/content/chunky-explosion) |
| `oga_explosion` | [Explosion](https://opengameart.org/content/explosion-0) |
| `rock_*`, `metal_fall_*`, `metal_hit_*`, `hit_*`, `wood_*`, `glass_*`, `falling_*` | [75 CC0 breaking / falling / hit sfx](https://opengameart.org/content/75-cc0-breaking-falling-hit-sfx) |
| `punch_1`–`punch_5` | [Punch](https://opengameart.org/content/punch) (qubodup) |
| `slam_*`, `metal_08`, `metal_09`, `door_close_*` | [100 CC0 SFX](https://opengameart.org/content/100-cc0-sfx) |
| `clink_*`, `thud_3` | [Metal Impact Sounds](https://opengameart.org/content/metal-impact-sounds) |

## Recorded enemy voices (November 23 2026)

Every enemy voice (`assets/sfx/voices/`, 596 files: death, hurt, idle, spawn, move, every tell and every release)
is built by `tools/build_real_voices.py` (`make voices`) from recordings frozen in
`assets/sfx/src/cc0/voice/`. Each is a creature vocal pitched to the enemy's register,
put through a short metal comb (a chorus for the massed ones), over a recorded
mechanism for its body; the cold machines have no vocal. Tells keep the synthesized
gestures (rise, hiss, fall, ratchet, inhale, drawn metal, sung, siren), built from
recordings and exactly as long as their wind-ups. The shared specials (halo, enrage,
rise, link, choke, the Absolute's line) are still synthesized by `src/audio/VoiceSynth.h`, the fallback
(`OVERDRIVE_SYNTH_VOICES=1` forces the old voices). All sources are **CC0** from
OpenGameArt:

| Frozen as | Pack |
|---|---|
| `c_*` | [80 CC0 creature SFX](https://opengameart.org/content/80-cc0-creature-sfx), [80 CC0 creature SFX #2](https://opengameart.org/content/80-cc0-creture-sfx-2) |
| `sn_*` | [16 Monster Growls](https://opengameart.org/content/16-monster-growls) |
| `ms_*` | [Monster Sound Effects Pack](https://opengameart.org/content/monster-sound-effects-pack) |
| `v2_*` | [Monster Sound Pack, Volume 2](https://opengameart.org/content/monster-sound-pack-volume-1) |
| `beast_*`, `demon_growl` | [Monster or beast sounds](https://opengameart.org/content/monster-or-beast-sounds) |
| `big_roar` | [CC0 Deep Monster Roar](https://opengameart.org/content/cc0-deep-monster-roar) |
| `troll_idle_*` | [Big scary troll sounds](https://opengameart.org/content/big-scary-troll-sounds) |
| `alien_hiss` | [Alien Sounds](https://opengameart.org/content/alien-sounds-0) |
| `snarl_*`, `troll_roar_*` | [Monster snarls](https://opengameart.org/content/monster-snarls), [Big scary troll sounds](https://opengameart.org/content/big-scary-troll-sounds) |
| `b_shot_*`, `m_shoot_01`, `m_loop_machine_03` | [25 CC0 bang / firework SFX](https://opengameart.org/content/25-cc0-bang-firework-sfx), [50 CC0 Sci-Fi SFX](https://opengameart.org/content/50-cc0-sci-fi-sfx) |
| `power_off` | [Machine shutting down](https://opengameart.org/content/machine-shutting-down) |
| `m_loop_*`, `m_misc_*`, `m_teleport_*` | [50 CC0 Sci-Fi SFX](https://opengameart.org/content/50-cc0-sci-fi-sfx) |
| `m_tools_*`, `m_machine_*` | [100 CC0 SFX](https://opengameart.org/content/100-cc0-sfx) |
