# Combat feel, training and Reliquary

## Player-facing changes

- Real recorded steel for sword cuts, heavier cuts, thrust contacts and blocks.
  Impacts happen as the blade crosses the target. Short weapon-only pauses,
  directional sparks/recoil and boss deflection give resistance while movement
  stays responsive. Shielded contacts show resistance without false healing.
- Distinct normal/heavy parries, armour/body hits and stone/steel/wood/ice wall
  impacts, with three variants each. Parries briefly make space in the mix while
  enemy warning sounds remain audible. The live combat score uses faster
  techno/DnB drums and bass; the soundtrack liked in the sound pass is retained.
- Fire presses within 100 ms of readiness, reload or switch completion are
  remembered once for the correct gun. Dash presses can bridge the final charge
  ticks. Pausing clears pending actions. Fire rates and damage remain unchanged.
- A gold boss-health damage trail and stronger hitmarker priority make damage,
  headshots, kills and resistance easier to distinguish. Hit flashes preserve
  armour and attack colours.
- F3 combat practice works and provides 20 lessons. Added sword combos, guard,
  ripostes, SPLINTER and chains, RECALL, slide kick, slam, precision shots, gun
  alt fire, heavy grapples and shield counters. The guide has three pages with
  arrow, Page Up/Down, wheel and mouse navigation. F4 opens it during a run.
- Reliquary fragments have distinct masonry, furnace plate, marble, reactor
  circuitry and basalt surfaces; additional details follow each moving relic.
  Landing edges, furnace grille, columns, reactor cells and distant ruins make
  its silhouette and route clearer without adding collision obstacles.

## Bug fixes and safeguards

The F3 freeze was an invalid wave lookup in music selection: the training arena
has no authored waves. Training now has its own music path, and `bossWave()`
bounds-checks all lookups, including empty scripted arenas.

Reliquary walking enemies were handed Y=0 as their sampled floor despite the
arena being around Y=-240. Flyers were initialized relative to the abyss,
creating a huge hover offset. Both now use the real local platform floor.
Spawn and escort placement check the complete body and support beneath five
foot probes. Revenant soul destinations use the same checks, including scale.

Recovery previously used a relic's centre, which could lie inside its raised
dais. It now finds a supported deck or raised surface with clearance for the
player. This works on all twelve relics, in all four arrangements and during
the three glides. Layout, collision geometry and wave composition are retained.

Moving relics render through the instanced-box path rather than the static
map atlas. Their new material patterns use local coordinates so details travel
with the slab. Low quality keeps the simple surfaces and essential edge cues.

## Reproduce validation

```sh
make
make test                 # full physics, combat, Story, bosses, lighting and audio suite
make test-feel-runtime    # real native GL game; F3, lessons, tools, combat and void recovery
make test-render          # instancing/renderer regression
source ~/emsdk/emsdk_env.sh
make web
make test-web-story test-web-lighting
python3 tests/test_web_release.py
python3 -m http.server 8765 --bind 127.0.0.1 -d web/dist
# In another terminal, with Playwright and Chrome installed:
node tests/test_web_smoke.mjs http://127.0.0.1:8765/
```

`OVERDRIVE_PLAYWRIGHT=/absolute/path/to/playwright/index.mjs` optionally selects
an existing Playwright installation. The browser test uses a fresh context,
opens F3, enters all twenty lessons and captures guide pages. The native runtime
test resolves actual sword contacts, guard/riposte, SPLINTER chains, RECALL,
buffered gun/dash input, moving-deck enemies, scaled Revenant reformation and
actual recovery after falls. Spawn tests sample 80 seeds per enemy/formation.

Audio checks measure asset onset, headroom, target level and clean tails and
exercise synthesis at three sample rates. These are objective checks; physical
headphone/speaker playback and individual timbre preference still require the
player's listening judgement. Sources and research are in
[the audio notes](audio-impact-pass.md) and [asset credits](../assets/sfx/CREDITS.md).
