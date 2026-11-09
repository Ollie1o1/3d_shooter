# Surgical Observation, damage feedback and cosmetic randomness

This pass improves one existing encounter and the shared systems used to tune
combat. Launch the room with `./shooter --story3 --arena 4` or `?story3&arena=4`
in the browser. These checkpoint starts use the chapter's existing equipment.

## The theatre

Surgical Observation retains its twenty enemies, two waves, observation
galleries, sealed booth and original way forward. A third surgical station and
a low crossing beneath the operating lamps form a circuit with ramps at both
ends. Seven sterilisation banks interrupt long sightlines. Service steps give
their roofs a purpose during a fight, and amber edges identify the low crossing.
Two Patient starts move clear of the crossing's underside.

The room measures **25.6% cover**, **25.7 m mean sightlines**, **four standing
layers** and **0.79 light parts per 100 m²**. It previously had roughly 20% cover
and 29.6 m sightlines. Its Chapter 3 lighting has been rebaked. Tests walk the
new circuit with the real player controller, the original chapter route and
both gallery approaches; they also check every authored enemy's support,
clearance and separation. These measurements describe geometry, not a claim
that subjective encounter balance has been proven.

## Death feedback

Death cards can now show the source of the accepted damage and a short counter.
Ordinary projectile hits retain the shooter's enemy type even after that enemy
has disappeared. Simultaneous hits from different enemy types keep their total
damage and show a generic projectile source rather than arbitrarily blaming
one of them.

Common melee, beams, ground shockwaves, explosions, marked floor attacks,
Sovereign phantoms and burning edges, lava, falls, the furnace, freezing vents
and the Motor Assessment load have their own feedback. The Absolute's white,
unparryable and unblockable strikes explain the corresponding response. Other
boss contacts identify their source where available; unknown contacts clear
old advice and do not invent a counter. Gold strikes are labelled by their
unparryable property, so the wording also works with colour-blind tell settings.

The Sovereign's melee event carries the landed attack separately from his next
queued wind-up. This matters because the second sweep immediately queues a
cleave, and a cleave clears its active attack before GameplayState handles its
damage. His sweep, cleave and thrust feedback uses the event's identity.

DamageHistory is independent of rendering. Only positive accepted damage to a
living player updates it. Invulnerability, god mode and successful parries do
not replace the last contact. Further contacts after health reaches zero cannot
overwrite the lethal one. Player resets clear the history, including checkpoint
retries and training lesson changes. Existing retry controls are retained.

## Independent cosmetic randomness

`core/CosmeticRandom.h` owns the sequence for particles, debris, ambient detail,
camera kicks, sound variation and decorative ice cracks.
These paths no longer consume the gameplay `rand()` sequence used for AI,
spawning, weapon spread, loot rolls and pickup placement. Existing gameplay
seeding through `srand()` remains supported. Procedural level generators and
Endless wave generation keep their existing independent generators.
Enemy animation phases remain gameplay-seeded: they also drive Mite movement
and animated hitboxes, so they belong to simulation state.

This is a separation of random streams, not an input-replay system. Challenge
codes retain their existing promise: matching wave composition, objectives and
difficulty, rather than replaying all enemy decisions.

The regression runs the real AI for ten seconds with presentation disabled,
reduced and full. Positions, attack decisions, emitted attacks and interleaved
gameplay random draws must match exactly despite different cosmetic seeds and
particle workloads.

## Verification

```sh
make
make test
make test-iteration-runtime
make test-render test-feel-runtime
source ~/emsdk/emsdk_env.sh
make web test-web-story test-web-lighting
python3 tests/test_web_release.py
node tests/test_web_pointer_lock.mjs
```

`make test` includes `test-iteration`, the new AI/randomness and damage-source
checks, alongside the updated Lab route, spawn and map measurements. The native
runtime check resolves an actual Sovereign cleave, a projectile whose shooter
is gone, blocked contacts, successful parries, checkpoint retries and unknown
damage. It renders the revised theatre and the death cards for review under
`build/iteration/`.

The existing Playwright smoke test now also opens Surgical Observation:

```sh
python3 -m http.server 8876 --bind 127.0.0.1 -d web/dist
# In another terminal:
OVERDRIVE_PLAYWRIGHT="$PWD/build/browser-smoke/node_modules/playwright/index.mjs" \
  node tests/test_web_smoke.mjs http://127.0.0.1:8876/ build/iteration/web
```

Browser smoke captures check loading, menus, input and WebGL rendering. Native
runtime assertions cover the damage details; a fresh player's judgement of the
room and advice remains useful follow-up playtesting.
