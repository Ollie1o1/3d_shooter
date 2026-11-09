# Encounters, combat chains and Combat Remix

Implemented November 9, 2026, from the experiments in
[the combat research](combat-research-and-next-steps.md).

## Surgical Observation

The theatre retains two waves, twenty enemies, its original four Suspended,
two Grafted and fourteen Patients, and the existing checkpoint and objective.
Each wave now opens with one Suspended, one Grafted and three Patients. The
beam controls a firing lane, the Grafted contests the direct approach, and the
Patients pressure movement. The lower circuit and galleries offer other angles.

Three casualties announce the remaining Suspended and four Patients. A sound
and feed warning precede their release by 1.2 seconds, followed by the normal
visible enemy materialisation. Their positions shift pressure toward the east
and the galleries. A 1.25-second simulation breather separates the two waves.
These are authored changes in pressure, not adaptive health or damage changes.

`WaveEntry::after(kills)` supports the staging in WaveDirector. Warning timers
and casualty accounting reset with the wave and retry. The director scales an
unreachable casualty threshold down to the preceding groups' actual population
when difficulty reduces automatically placed enemies. Authored fixed spawns
retain their exact counts. Queued reinforcements prevent premature wave clears.

The encounter workshop exposes the casualty threshold. Version-two patch
content preserves it; existing version-one patches still load as immediate
groups. Validation rejects a wave whose reinforcement conditions cannot be met.
The existing `encounter.v1.cfg` filename remains supported.

## Long-Term Care

Six diagnostic banks break the long diagonals and provide low service steps
and 2.4 m fighting roofs. The forward pair sits clear of both station ramps.
The low east-west circuit and both observation-gallery approaches remain open.
Amber roof edges identify the optional fighting surfaces. Enemy counts,
assessment locks and required routes are unchanged. Chapter 3's lighting is
rebaked for the new collision and geometry.
The final room measures 25.4% cover and 26.0 m mean sightlines, against roughly
19% cover and 29.7 m before this pass.

## Combat Remix

Choose **Story → Combat Remix**, launch `./shooter --remix`, or use `?remix` in
the browser. This is one normal-damage encounter in Surgical Observation with
all four guns, grapple, Oathbreaker on 5, SPLINTER on the grenade binding and
RECALL on its normal binding. The opening toast shows current tool bindings.
Finish both waves for a completion card; Enter starts the same encounter again
with its equipment and ability state restored. Backspace retries the encounter.

The remix is practice: no leaderboard, ordinary records, campaign save or
archive progress. A later normal menu start clears its tool access. Ordinary
campaign acquisition and chapter equipment rules remain as before.

## Rewards and learning

A kill within 2.5 seconds of a player-caused stagger earns **STAGGER FINISH**.
A substantial player shove followed within 2.5 seconds by an environmental or
friendly-fire kill earns **HAZARD SETUP** or **CROSSFIRE SETUP**. These are
per-enemy causal windows, not bonuses for arbitrary input sequences.

Deliberate setups retain run-score points even above the passive style-meter
cap. Their meter contribution still stops at rank B; unrelated passive deaths
retain the existing score cap, avoiding unattended environmental point farming.

Successful beam interrupts and setup chains can introduce one brief contextual
card in ordinary play. The existing taught bitmask remembers it. FAST and
practice keep their existing teaching-card behavior; F4 remains available.

## Pressure and boss mastery

Attack scheduling retains its existing attacker cap, visibility preference and
waiting priority. Seraphs, Suspended, Weavers, Brutes, Grafted and Juggernauts
also cost two pressure slots against a budget one larger than the attacker cap.
Active beams continue reserving their pressure. An interrupted beam releases
that reservation. This reduces simultaneous major threats; it is not a general
proof that every possible hazard combination has compatible escape routes.

During **24-K**'s gold Lancer charge, a headshot worth at least 40 damage cancels
the shot and creates a 0.9-second stagger with 1.1 seconds of recovery. Body
hits and small pellets cannot interrupt it. The existing evade and projectile
parry options remain useful; its white rush retains its parry response. A
**COUNTERSHOT** confirmation and the enemy hint explain the new opportunity.

## Overdrive

Filling the meter banks a charge and shows **OVERDRIVE READY**. The next valid
gun shot, sword attack, punch, finisher or thrown explosive with an active
enemy present consumes it. Shooting an empty room cannot spend the charge.
Readiness has no timer and prevents idle meter decay; the charge survives wave
breaks. Retry resets it. Active duration, kill extensions, power bonuses and
cooldown retain their existing values. The Field Guide explains activation.

## Verification

`make test-combat` exercises staged wave completion, warning timing, retries,
patch compatibility, reduced populations, readiness, passive-score limits and
24-K's real AI recovery. It is included in `make test`.

`make test-combat-runtime` exercises the actual Story menu transition, input
bindings, combined tools, shot-triggered Overdrive, pressure reservations,
causal rewards, precision counters, completion/retry, bank climbing and save
isolation. Review captures are written under `build/combat-pass/`.

The existing Chapter 3 tests walk the revised low circuit with the real player
controller and check spawns and map measurements. Browser smoke coverage adds
Combat Remix and Long-Term Care to the existing training, chapter and replay
starts. Human observation remains necessary to judge enjoyment and difficulty;
automated traversal and combat checks establish functional behavior.

Release verification completed on November 9:

- Native build and full `make test` suite.
- Live combat, development-menu, render and combat-feel regressions.
- WebAssembly build, Story previews, lighting, release identity and pointer-lock checks.
- Chromium smoke run through twenty lessons and all existing replay/chapter starts,
  plus Combat Remix and Long-Term Care, without browser errors.
- Website: 35 unit tests and production build/link verification for all five pages.
  Game, website source assets and built website assets share build `603639fb3077`.
