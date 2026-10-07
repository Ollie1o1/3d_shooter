# Sound credits

The enemy voices are not files: they are synthesized in code when the game loads
(`src/VoiceSynth.h`), with no recordings.

Most sounds are synthesized by `tools/gen_sfx.py`. The arsenal's sounds (the four
gunshots, three variants each, and the reload / bolt / pump / cylinder / cell / dry /
switch foley) are built by `tools/gen_arsenal.py` from one recipe: the first instant of
each CC0 gunshot recording below (frozen in `assets/sfx/src/`) layered with a
synthesized thump, a metallic ring in the gun's pitch and a tail. The ones below are built
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
