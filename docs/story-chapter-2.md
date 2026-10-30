# THE DEAD SPAN — Chapter 2

Start through STORY → 2 THE DEAD SPAN, continue from Chapter 1's completion
card, or run `./shooter --story2`. A direct start grants the revolver, shotgun,
Lancer and grapple. Continuing carries your equipment. UNIT 11 holds the Longshot.
F2/backtick on the main menu opens the developer checkpoint menu for both chapters.

The current narrative and encounter direction is in [Story revision](story-overhaul.md),
including research links and the writing rules. Chapter 2's earlier Warden reuse
has been removed. Its final sibling is **Ballast**, a clearance machine built
for this landscape.

## Route and encounters

| Place | What changes the play |
| --- | --- |
| Ash Approach | Factory exterior, prevailing ash wind, clearance marks and a service notice pointing to the relay. No spawn ambush. |
| Dead Convoy | One crossfire ambush with sentinels, flankers, a shield and a beam flyer. Freight breaks sight lines; an open-ended fallen canopy offers a sheltered flank. |
| Buried Relay | A broad ramp descends five metres. Stand on the service pad for nine uncontested seconds to seat a failed contact. Small security details refill at bounded intervals; enemies on the pad stop the repair. |
| Broken Span | Grapple beams cross the broken deck. A six-metre-wide maintenance bypass provides a walking alternative. Falling returns you to a safe ledge and costs health. |
| Turbine Graveyard | Shoot three visible seized brake locks. A finite guard detail includes a Weaver and aerial threats. Clear it first or repair under pressure. Releasing the brakes starts the rotors and opens the gate. A broad ramp reaches an optional maintenance deck and an overhead hook. |
| Last Toll | Quiet crew booths, failed repair requests and a rooftop wreck; Ballast is visible beyond the gate. |
| The Cut | A dedicated boss space: blade scars, marked working lanes, real windbreaks and overhead grapple structures. No reactor or conduit phase. |
| Far Road | The machinery falls quiet and the next service road leads toward the tower. Enter returns to the menu. |

Cyan lamps identify optional records. Look within reach and press the interaction
key (E/R3 by default). Weapons are separate pickups. Records describe particular
incidents: abandoned lorries, a service light left on, a rifle zeroed on a pylon,
and a crossing interrupted by Ballast's warning siren. Terminals wrap to fit their
panels; dialogue is never the only instruction for a required action.

## Ballast

A four-legged road grader with an exposed turbine, a worn full-width blade,
cable winch, narrow cab visor and service markings. It has a separate articulated
Blender model, AI and mechanical sound profile. It addresses UNIT 24 as road crew.

- **Grading pass:** the marked strip commits before release. Jump the low blade
  or dash sideways. The stalled drive is vulnerable afterwards.
- **Crosswind:** the turbine warns before a grit front moves down the marked lane.
  Move out or use actual solid cover. The final phase alternates blowing and
  reverse draw, with arrows showing the committed direction.
- **Cable weight:** a white weight descends onto a committed position. Parrying
  it stalls the machine; moving out also avoids the impact.
- **Loose ballast:** heavy road fragments can be shot apart or parried back.

Damage changes its braking and recovery timing. The drive takes increased damage
while exposed or staggered. Range, high cover and arena corners remain within its
cable and projectile reach. Defeat stops its hazards and hostile projectiles and
opens the far road. Retrying after the boss preserves the cleared state.

## Atmosphere and performance

Settings → VIDEO → **AMBIENT DETAIL**: OFF / LOW / MEDIUM / HIGH. It is separate
from render resolution and persists on desktop and web. It controls ambient ash,
dust, falling grit and vent emissions, loose clearance tape and optional machinery
animation. Ambient particle caps are 0 / 32 / 112 / 240; the remaining pool stays
available for combat. Lowering the setting removes surplus ambient particles
immediately. Required targets, markers, attack fronts, collision and encounters
stay the same at every setting. Wind audio remains part of each district's sound.

## Editing and verification

- `src/core/StoryText.h`: the siblings' lines and wreck records.
- `src/world/LevelWasteland.h`: route, cover, alternate deck, encounters and brake targets.
- `src/game/Gameplay_Wasteland.h`: repair completion, checkpoints and decorative machinery.
- `src/bosses/ballast/`: independent state, AI, shared crosswind geometry and gameplay drawing.
- `tools/blender/ballast_build.py`: model recipe; editable scene at `build/ballast/ballast.blend`.
- `tools/blender/turbine_frame_build.py`: the stationary frame used by animated rotors.

```sh
make
make test
make test-dev
./shooter --verify-story --res 360 --nopace
./shooter --story2 --arena 7 --god
./shooter --story2 --arena 7 --god --overlay ballast3
./shooter --story2 --arena 5 --ambient 0
```

`make test` includes real-controller traversal of the main route, both secrets,
the convoy shelter and turbine deck; spawn clearance and every encounter;
Ballast simulation against moving, stationary, corner, ranged and perched
policies; crosswind cover; repair progress and reinforcement bounds; settings
persistence, particle budgets and record layout. `make test-dev` checks actual
weapon hits on the brakes, gate opening and retry, practice skips and every
checkpoint. It also writes settings and record-layout images under `build/`.
The native `--verify-story` path exercises chapter handoff, found equipment,
Longshot, explicit reading, fall recovery, Ballast's defeat, gate, ending and menu.

BossSim is a deterministic combat regression. Its ideal movement policy is a
useful check of attack answers, rather than a claim about first-play difficulty.

After geometry edits, rebuild `tools/light_bake` and run `tools/light_bake story2`.
`make level-dump` exports the map for Blender. To rebuild the boss or turbine frame:

```sh
blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/ballast_build.py
blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/turbine_frame_build.py
```

## Native review views

[Ballast model](story-overhaul/ballast.png) ·
[Relay contact and service pad](story-overhaul/relay-contact.png) ·
[Turbine maintenance deck and brake targets](story-overhaul/turbine-deck.png) ·
[Ambient Detail settings](story-overhaul/ambient-settings.png)
