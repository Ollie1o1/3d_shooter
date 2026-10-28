# FAST: the Gauntlet overhaul — design

Date: 2026-10-27 · Status: agreed direction, room specs to follow · Built room by room, the Canal first

## Context

The Gauntlet (FAST mode, `world/LevelGauntlet.h`) is seven rooms joined by boost tubes, with splits, par
ranks and a ghost. Every room plays the same way: two waves of Act I enemies, kill everything, the exit
opens. The time is set by the slowest enemy, not by how fast you move.

Decisions made with the user:

- **No room gimmicks** (no flooding, crumbling floors or closing walls). Each room is unique through its
  layout, its look and its mini-boss, never through a rule that makes the game weird.
- **A mini-boss in every room**, made in Blender like the bosses. **The last one is a real fight.**
- **The rooms get the Blender pass the arenas got** (props over box collision, `assets/props`,
  `tools/blender/props_build.py`), and are redesigned to play better.
- **New enemies are made in Blender** (`tools/blender/*_build.py`, `enemy_kit.py`).
- **Sound is part of the work**: every new thing gets its sound, and the game's feel improves with it.

## What's wrong now (from rendering every room)

- The rooms are code-built shapes (`Shapes.h`). The Well, the Pumpworks and the Tower's courtyard
  read as boxes; the courtyard is mostly empty floor.
- **Second waves spawn behind you**: the Well's gunners at 13 m after you've dropped to the floor,
  the Pumpworks' back by the entrance, the Tower's Sentinels by the door you came in.
- **Waiting**: Raptors at 24-30 m in four rooms; the Tower's first wave in all four corners.
- The Canal's exit is off your line of sight; the Sluice packs 11 enemies and a Juggernaut into 24 x 24 m.

## How every room is laid out

1. **One clear route**: the exit seen from the entry, marked by a big lit landmark.
2. **Enemies on the route**, not in corners.
3. **Waves spawn ahead of you**, between you and the exit.
4. **One built faster line** per room: visible, not obvious, worth 2-5 s.
5. **Health on the brave line**, not in safe corners.
6. **One movement skill each**, from the layout: Canal grapple lines, Sluice pad vaults, Ascent pad
   chain, Span ferry hooks, Well the drop, Pumpworks the crane, Tower the climb.

## Rooms and mini-bosses

| Room | Layout | Blender kit | Mini-boss |
|---|---|---|---|
| 1 Canal | Exit under the dam, in sight on arrival; a hookable truss at each bridge; Raptors down to ~11 m | stone bridges with iron rails, lamp posts, a moored barge, bollards, the sluice gate, a lit exit arch | **THE COURIER**: a runner holding the key, sprinting the walkways dropping Mites. Medium weight: the hook zips you to it |
| 2 Sluice | Widened to ~30 x 30; 8 in the first wave; pads at the pit's edge | pipe manifolds, valve wheels, grated mezzanine, the pump block, warning lamps | **THE BULWARK** (in place of the Juggernaut): a giant shield-soldier whose shield is a sluice gate. Vault the pads, hit its back |
| 3 Ascent | A pad chain up the terraces; waves one terrace above you | carved stairs, balustrades, statues, pews as cover, ring chandeliers, the rose window | **THE CANTOR**: under the rose window, a beam swept down the terraces and bell rings to jump |
| 4 Span | Island edges readable; more ferries; Raptors out of the far sky | gondolas on cables, pylons, broken viaduct arches, wind flags, hook rings | **THE HARRIER**: a giant hawk-machine strafing the canyon. Hook it low and ride its back (the fast line); punch its neck |
| 5 Well | Waves below you as you fall, never above; a spiral stair as the slow way | grated floors, ladder cages, the core pylon, cable bundles, lit landings | **THE PLUMB LINE**: a segmented core down the shaft. Shoot segments falling past; its head at the bottom |
| 6 Pumpworks | Brighter; waves toward the east end | pumps with moving pistons, tanks, consoles, the crane gantry, catwalk rails | **THE FOREMAN**: tethers a shield to the others and rides the crane between the control rooms. Hook the crane |
| 7 Tower | One courtyard wave from the far bastions; the climb is the boss fight | battlements, bastions, lift cages, banners, the beacon lens and brazier | **THE PACEMAKER** (below) |

## THE PACEMAKER (the final, tough)

The machine that set the par times, a lit clock face in its chest. Each phase tests a mini-boss's lesson.

- **Courtyard (100-65 %)**: arm blades and a scattergun; dash-slashes on lines (parry the white: the
  Courier), spreads a flank beats (the Bulwark).
- **The climb (65-30 %)**: it runs up the tower; rings sent down to jump (the Cantor); a weak point
  opens on each floor it lands on (the Plumb Line).
- **The crown (under 30 %)**: quicker strings, a diving strike you parry to knock it down, a pylon
  shielding it that you cut (the Foreman). At 10 % it channels the beacon for 8 s: hit its back or
  parry it; if it finishes it heals 15 % (costs time, never ends the run).
- **Rules**: every tell at least 0.3 s with a sound cue; never invulnerable; ~60 s for a skilled kill,
  ~2 min normally; a BOSS mode row and a BossSim kill-time case.

## New FAST enemies (Blender)

- **SPRINTER**: charges along lanes; parry the charge and it flies into its friends; a slide kick kills it.
- **VAULTER**: uses the jump pads like you, so tall rooms have enemies that follow you up.
- **SPOTTER**: a drone that marks you (Sentinels fire faster); shot on the run; replaces most high Raptors.

## Rewards

- **Kill the mini-boss and the exit opens; each enemy left alive adds 1 s** (Titanfall 2's Gauntlet adds 2).
- **Medals per room** (bronze, silver, gold, a hidden dev time), ROOM ATTACK with a ghost per room, sum of best.
- **The end screen per room**: time, medal, enemies skipped, damage, the gap to your best.
- **Unlocks**: all gold opens a Hollowed Gauntlet and the Pacemaker in BOSS mode; dev times give cosmetics.
- **Shared Blender props**: jump pads, tube ribs and doors, a health station per tube, glowing hook rings.

## Sound

### Built (2026-10-27)

Synthesized in `tools/gen_sfx.py`, levelled in `audio/MixTable.h`, held by the mix test.

| Sound | When | What it is |
|---|---|---|
| `anvil` (3 variants) | a FAST room cleared, an ARENA / ACT II arena won | a heavy hammer on a big anvil, struck low (~330 Hz): a gritty clank, the hammer's thunk, a sub boom, inharmonic steel modes ringing ~1.5 s, a bounce, driven a little, a dark hall |
| `split` / `split_behind` | just after the anvil, if there's a best run | two rising blips ahead, two falling behind |
| `anvil_final` | the beacon | clank... clank-CLANG, the last blow bigger and lower (~250 Hz) over a boom and a swell |
| `record` | a new best (any mode), 1.3 s after the finish | a strike an octave over the anvil's and a rising arpeggio of bells |
| `count_tick` / `count_go` | 3, 2, 1 / GO | beeps on a hard tick; GO is a starting gun under a higher beep |
| `overdrive_on` (reworked) | OVERDRIVE starts | the rush and chord, now struck on the anvil at the impact |
| `drive_warn` | 1.5, 1 and 0.5 s left of OVERDRIVE | a metal tick, higher each time |
| `drive_ready` | OVERDRIVE's cooldown ends ("OVERDRIVE RECHARGED") | a soft shing and two notes rising a fifth |

`--overlay clearwaves` wins every wave for you, one every 4 s; with `--audiodump` you hear the clears.

### To build with the rooms

- **Medals**: the room's anvil pitched by medal (bronze low, gold a double ring, dev time a third strike).
- **Ghost**: a whoosh as it passes you or you pass it.
- **Room start**: a heavy bolt as the door behind you locks.
- **Hook rings**: a small tink when the aim locks on one.
- **Mini-bosses**: a stinger each, and tells that follow the Absolute's rule (bright for parry, low for
  dodge, a double pulse for get out). The Pacemaker's tells tick like a clock.
- **Each room's props**: water lapping under the Canal's bridges, valves hissing in the Sluice, chandeliers
  creaking in the Ascent, cables humming on the Span, grates ringing in the Well, pistons in the Pumpworks.

## Order

1. The Canal as the template: layout, its kit, the Courier, medals, the skip rule.
2. Rooms 2-6, one at a time, each with its kit and mini-boss.
3. The three enemies as the rooms that need them arrive.
4. The Tower and the Pacemaker last.

Every room passes `make test` (spawns, pads, movers, BoundsScan) before the next.
