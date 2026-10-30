# Story revision: work left unfinished

The route should tell the same story as the people who died on it. The factory
sorts functioning machines into a furnace; outside, a road crew still clears a
road nobody uses. Unit 24 is asking who assigned that work. This pass leaves
the creator's answer open.

## Writing rules

- Signs and terminals describe a process, a fault or a record. They do not
  deliver philosophical slogans or address the player as a narrator.
- Wrecks describe something that happened to that particular unit, in that
  place. Concrete mistakes, observations and practical advice make them people.
- Bosses speak as workers. The Sorter repeats a classification it cannot
  resolve. Ballast warns another worker out of its cutting lane, then loses
  control of its machinery. No taunt wheel and no speeches about purpose.
- HUD instructions explain the current task plainly. Dialogue never carries
  the only explanation of an attack or objective.
- Silence matters: no room-entry quip, repeated bark or forced log reading.

## Encounter rhythm

| Place | Identity |
| --- | --- |
| Scrap Pit | One short revolver fight, followed by the climb; no second wave on the same heap. |
| Ducts | One confined rush with explosive mites; learn to make space. |
| Assembly | Crossfire between the floor and catwalk, then one heavy clean-up; two contrasting beats. |
| Archive | Silence and specific records of earlier units. |
| Vault | One simultaneous containment breach with shields and a tethering controller; break the formation. |
| Lift | Keep the three stops because each changes height and firing angles. |
| Inspection | Avoid the scanners or deal with collectors; traversal is the encounter. |
| Convoy | One ambush across freight cover, with a beam flyer, flankers and an open-ended sheltered route. |
| Relay | Restore a contact at the maintenance console while a small, bounded security detail contests it. |
| Span | Traversal and recovery, with wind showing the crossing. |
| Turbines | Shoot three visible seized brake locks. A finite security group can be cleared first; an optional ramp and maintenance deck provide a second firing angle. Completing the repair opens the service gate and sets the turbines turning. |
| Toll | Breather, repair records and the first clear view of Ballast at work. |
| Cut | New boss: Ballast. Open lanes, scarred road, staggered windbreaks, hooks. |
| Far road | The machinery falls quiet; the road and tower remain. |

## Ballast / clearance unit

A broad, low road grader on articulated outriggers. A salvaged turbine turns
above its back; the blade wears the same road paint and rust as the convoy.
It has its own asset, AI, attack sounds, tells and hitbox. No reactor, conduit
phase, Warden attacks or Warden silhouette.

- **Grading pass:** commits to a marked strip, then drives the blade through
  it. Dash sideways or get above its cutting height.
- **Crosswind:** its turbine warns, then sends a moving wall of grit down a
  marked lane. Cross the lane or shelter behind real cover. Detail settings
  cannot hide the warning or the damaging front.
- **Anchor strike:** a cable weight lands at a committed point. White tell:
  parry the weight to stall the motor and expose the drive.
- **Ballast throw:** a heavy piece of road can be shot or parried back.
- Damaged brakes shorten recovery and chain crosswind into a grading pass.
  The final phase adds a cable-weight follow-up and reverses some wind lanes.
  Each follow-up has a fresh warning; the blade lane remains visible throughout
  the charge. Predictable movement is led, but cable targets lock before impact.

## Boss pressure and healing

Random enemy potion drops use a base 6% chance (previously 18%); Brutes use
20% (previously 50%). The ordinary drop rate scales with difficulty. Placed health
and guaranteed FAST miniboss rewards retain their existing rules.

The [boss combat overhaul](story-boss-combat-design.md) adds concurrent pursuit,
committed interception, staggered press rows, furnace purges, pivoting grading
runs, a cuttable winch, blade sweeps and flying road slabs. Machinery and damage
volumes agree, including attacks aimed at an elevated player. Light cover breaks
through actual impacts and restores on retry. Floor panels fail progressively.
Both bosses keep their existing health totals; precision hits on exposed
machinery earn the damage bonus, and Story bosses shed two potions instead of
four. Parries cancel pending attacks and follow-ups.

## Armory

Tab opens the shared armory during playable Story sections in both chapters,
including dev checkpoints. Spend points earned from XP with Enter, Space or a
click; Tab or Escape returns to play. Combat pauses while shopping. Purchases
remain through checkpoint retries and the transition into Chapter 2. The opening
wake-up and chapter ending retain control of their input.

## Atmosphere and performance

Separate optional detail from combat information. Ambient quality is saved as
OFF / LOW / MEDIUM / HIGH. It controls ambient particle budgets and decorative
machinery, drifting grit, tape and vent emissions. Turning it down immediately
removes optional particles. Attack markers, projectiles, objectives, collision
and encounter timing remain identical. Keep ambient allocations bounded and
leave most of the particle pool available to combat.

## Developer launch and HUD checks

The WebAssembly build reserves a 1 MiB stack with overflow detection. File-based
enemy rendering calls a separate procedural fallback only when it is needed;
the fallback's large temporary rig arrays no longer consume stack while Story
previews draw their model files. `make test-web-story` renders both Story boss
previews as real WebAssembly under a deliberately restricted 32 KiB stack.
The native dev check now selects every checkpoint through menu input, destroys
the menu during the deferred handoff, and draws the first gameplay frames.

Settings → GAMEPLAY → **ENEMY MARKERS** saves LOW / DETAILED. LOW is the default:
small icons above the last three regular enemies, one short direction/distance
label when needed, and overlapping icons combined. DETAILED retains the full
names, distance and cover labels. Bosses and mini-bosses have no tracking pings
in either mode; their health bars and attack warnings remain.

## References and application

- [Michael Booth, Valve: The AI Systems of Left 4 Dead](https://steamcdn-a.akamaihd.net/apps/valve/2009/ai_systems_of_l4d_mike_booth.pdf):
  peaks and recovery periods. Applied as authored changes of activity and
  bounded reinforcement delays, without copying its procedural population system.
- [Kurt Loudy and Jake Campbell, id: Embracing Push Forward Combat in DOOM](https://gdcvault.com/play/1024940/Embracing-Push-Forward-Combat-in):
  aggression and movement. Applied through threats at different angles,
  exposed objectives and readable boss commitments that reward repositioning.
- [Loan Verneau, Mobius: From Design to Art With Love](https://www.mobiusdigitalgames.com/news/from-design-to-art-with-love):
  levels and art developed together for environmental storytelling. Applied
  through road-cut marks, stranded freight, failed repairs and the boss's tools.
- [Harvey Smith and Matthias Worch: What Happened Here?](https://www.gdcvault.com/play/1012647/what-happened-here-environmental):
  history inferred from the environment. Logs corroborate visible failures
  instead of explaining abstract lore.

These are design interpretations, not claims that the games use our mechanics.

## Verification completed

- Native and WebAssembly builds pass.
- `make test` passes the physics, game, Zenith, Cradle, wasteland and Story
  overhaul suites, including Ballast's asset joints and visible head bounds.
- `make test-dev` passes all checkpoint, real weapon/brake, gate/retry and
  paused-settings checks.
- Native `--verify-story` passes chapter handoff, equipment, explicit reading,
  fall recovery, Ballast's death, gate opening, retry, ending and menu selection.
- Native images were reviewed for the boss, relay contact, maintenance deck,
  sheltered convoy route, wrapped records and settings layout. Views are linked
  from [Chapter 2 notes](story-chapter-2.md#native-review-views).

The boss's Blender scene is `build/ballast/ballast.blend`; its recipe and shared
editor kit both register the articulated joints for further editing.
