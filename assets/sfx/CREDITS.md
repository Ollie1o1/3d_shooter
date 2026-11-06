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
