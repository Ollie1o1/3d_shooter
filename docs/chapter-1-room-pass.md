# Chapter 1: room identity and player experience

The review found three causes of the chapter's empty feel: the upper rooms
shared one sound and lighting recipe; substantial pieces of wall and floor
had no visible function; and inherited duct fog swallowed distant details.
A few small props at the edges could not explain spaces this large.

The pass gives each room a different working purpose, scale reference and
sound. It preserves the linear chapter, the optional E-to-read records,
existing weapons, both secrets, encounter waves and the three freight stops.

| Room | What changed | Intended player experience |
| --- | --- | --- |
| Furnace Line | Deep trusses, exhaust stacks, extraction banks, refractory panels and numbered reject feeds. | The opening ceiling view has weight and depth; the furnace belongs to a working disposal system. |
| Scrap Pit | Compressed salvage replaces broad plain heap faces; cable runs and extraction equipment give the deep shaft a believable service structure. | The piles read as discarded machinery while their flat jump surfaces and marked exit remain usable. |
| Maintenance Ducts | A detailed pump, exposed flanges and pipe returns, panel services, an abandoned tool cart and low-clearance stencils. | Tight spaces feel maintained and repurposed, with the slide opening and Lancer room easy to distinguish. |
| Assembly Hall | Fabrication arches, hydraulic feeds, large extraction banks, deep roof bracing, lower work lights, rough production concrete and a catwalk rail with a broad descent opening. | The overlook reveals a substantial production floor. Movement has human-sized references instead of an enormous blank slab. |
| Archive | Detailed server banks face the aisles, with extra storage, memory drawers, cable trays, a floor runner and slow data indicators. Cool light and server-room ambience replace steam and pump noise. | A quiet, legible reading break after the hall; the records feel physically stored here. |
| Pod Vault | Numbered cradles, coolant spines, overhead supply pipes, larger painted bay footprints and pale floor tiles. Lights and ambient colour change after the glass breaks. | Cold containment becomes an alarm state during the ambush. Side trial stations remain different from the central pods. |
| Freight Shaft | Cross-bracing, bundled cables, grilles, counterweights and door machinery. Longer visibility and shaft acoustics. | The height and machinery explain the ride; floor numbers make its progress readable. |
| Inspection Line | Repeated equipment booths, calibration paint, extraction heads, status lamps and stencilled protocols. A clock-driven ambience distinguishes its rhythm. | The player reads a deliberate inspection process and can see the sequence ahead without confusing decoration with scanner hit volumes. |
| Sorting Floor and ash exit | Tall ribs, roof trusses, suspended rejection cages, industrial ventilation, wall markings and ash banks outside. | The Sorter's room feels built around its job. The fighting cross and grapple rails stay visually and physically accessible; the exit opens toward the tower. |

Seven additional Blender assets supply the pump, Archive bank, fabrication
frame, pod spine, extractor, compacted scrap and tool cart. Their source recipe
is `tools/blender/cradle_room_props_build.py`. Editable scenes are generated
under `build/cradle-props/`; the game-ready GLBs live in `assets/props/`.

The assembly presses now stop above their belts: the previous stroke extended
below the floor. Extraction fans, data indicators, pressure exhaust and moving
counterweights add small ongoing actions. Decorative effects do no damage;
combat tells and the cyan readable lamps keep their existing meanings.

Room ambience follows the room the player physically occupies, including when
backtracking. The vault changes from standby to containment-alarm lighting when
its fronts break. Sorting's drifting embers give way to ash after the boss.

Clearance checks caught an early pod-frame placement crossing the established
route to the freight lift. The uprights were moved inside the existing pod
footprints. The existing traversal, enemy-spawn and GPU-capacity checks cover
the resulting room pass, including the previously repaired lift entrances.

## Review views

[Assembly overlook](story-chapter-1/room-assembly.png) ·
[Archive aisle](story-chapter-1/room-archive.png) ·
[Pod Vault](story-chapter-1/room-vault.png) ·
[Inspection](story-chapter-1/room-inspection.png) ·
[Sorter chamber](story-chapter-1/room-sorter.png)

Before-and-after captures for matching viewpoints are generated locally under
`build/room-pass/`. Screenshots establish presentation and readable routes;
the normal run remains the test of whether each pause and fight feels right.
