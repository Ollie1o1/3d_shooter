# Combat learning, Story continuity and encounter choices

Implemented October 31, 2026. Chapter 3 remains a design discussion; see
[the surreal chapter directions](chapter-3-directions.md).

## Learning and practice

Main menu **F3 / COMBAT PRACTICE**, or Story → Combat Practice, opens a dedicated
arena. It has eight selectable lessons: parry, finishers, enemy grapple, shotgun
jump, grenade punch, Overdrive, movement and free practice. Targets respawn;
damage cannot kill the player. Finisher practice begins wounded with a depleted
magazine, so the health and reload reward is visible. The grapple target stands
on a ledge. Free practice offers six ordinary enemy types and Armory points.

**F4**, or Pause → Field Guide, reopens the combat explanations in any mode.
It displays current keyboard bindings. In practice, Enter/click starts the
selected lesson; Page Up/Down changes lessons during play and Backspace repeats
the current one. In free practice, `[` / `]` changes targets. A controller can
select Free Practice in the guide, press X to change its target and A to start.
Lesson completion lasts for the practice session and has no leaderboard reward.

Story introduces projectile parrying with the revolver, shotgun jumping with
the shotgun, enemy grappling with the grapple and grenade punching when a
grenade is first thrown. Existing finisher, stagger, alt-fire and Overdrive
cards remain. The field guide makes those explanations available again.

## Story saves

The Story page offers Continue when a valid save exists. `story.v1.cfg` on
desktop, or `overdrive.story.v1.cfg` in browser localStorage, stores chapter,
checkpoint, guns, grapple, upgrades and mod choices, wreck/terminal discoveries,
relay isolation, boss defeat and chapter statistics. Writes occur at important
discoveries/checkpoints/purchases, every five seconds during play and on leaving
the game. A new run replaces this one slot. Practice, god mode, boss selection,
fixed-camera and capture runs cannot overwrite it.

Continue returns to a checkpoint with full health and magazines. The current
encounter starts afresh; temporary projectiles, enemy damage and platform motion
are not serialized. Defeated Story bosses remain defeated and their exits stay
open. Continuing a completed Chapter 1 starts Chapter 2 with carried equipment
and upgrades. A completed Chapter 2 can be revisited at its last checkpoint.
Malformed, truncated and unsupported saves are rejected without partially
restoring their data. This is a local save, shared neither across devices nor
between browser and desktop.

The Boss page now includes the Pacemaker, Sorter and Ballast. Existing six boss
record indices remain stable. New bosses keep local bests; the existing remote
leaderboard API still supports only the original six bosses.

## Tactical choices

The Buried Relay has an optional north-side **SECURITY FEED** isolation console.
Using it stops reinforcements, leaving the present guards and required contact
repair. Players can take this detour or repair under the original pressure.
Isolation survives retries and saves.

The Turbine Graveyard's raised maintenance deck has a **SERVICE WINCH** control.
It drops a suspended counterweight into the Weaver's lane. A gold floor marking
and warning sound precede the drop by 1.3 seconds, followed by .45 seconds of
visible travel. The impact damages nearby enemies and the player; the landed
load becomes solid cover. Each encounter attempt gets one release. Retry
restores the load, its collision and control. This does not replace the three
required brake targets.

## Weapon modifications

Buying a gun's mod costs two points and unlocks both alternatives. Only one is
active at once. Enter/click on an owned mod swaps it free; M (controller X)
previews/selects the other alternative. Stat upgrades remain independent. Story
cannot spend points on undiscovered weapons.

| Gun | Existing option | Alternative |
| --- | --- | --- |
| Revolver | Piercing Rounds: two extra bodies | Deadeye: headshot multiplier +50%, body damage −20% |
| Shotgun | Dragon Breath: five extra pellets and tighter spread | Breach Kick: double-shot movement recoil +75%, enemy shove +50% |
| Lancer | Headhunter: headshot kills refund ammo and skip the bolt | Skirmisher: greatly reduced hip spread and no airborne spread penalty |
| Longshot | Explosive Tips: impact blast | Rail Driver: three extra bodies, without the blast |

## Readability

Ordinary enemy armor receives a small minimum light contribution while existing
attack glow colours remain intact. Normal depth testing still occludes enemies.
The relay doorway has a cyan frame; three unequal illuminated fins identify the
turbine exit from a distance. The maintenance deck edge, working controls and
winch floor zone have distinct visible treatments. Required information remains
independent of Ambient Detail.

## Checks

`make test` includes save validation, original boss-record compatibility, mod
tradeoffs, practice geometry and relay completion. `make test-dev` exercises
Continue and chapter handoff, boss launch/completion, actual parries, finishers,
grenade punches and shotgun recoil, Armory input, isolation and winch/retry.
Native review images are written under `build/`.

Launch directly with `./shooter --training` or `./shooter --continue`;
the browser shell accepts `?training` and `?continue`.
