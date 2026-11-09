# OVERDRIVE: combat research and next experiments

Research date: 2026-11-09. These are design proposals, not implemented changes.
This is the review before implementation; the subsequent
[combat pass](combat-pass.md) records what was built and checked.

## Recommendation

Concentrate the next pass on encounters that make the existing arsenal,
movement and enemy roster work together. OVERDRIVE already has the ingredients
of a substantial action shooter. The most promising next step is a small set of
excellent fights that demonstrate why the player should use those ingredients.

This review inspected the current source, design notes, saved Motor Assessment
and Surgical Observation captures, and local run summaries. It also rebuilt
and ran `make audit ONLY=CH3`. It did not include a hands-on playthrough; claims
about enjoyment, difficulty and player understanding remain hypotheses.

## What the developer research suggests

- **Movement and opposition must be designed together.** In a direct
  [interview about DOOM Eternal](https://www.gamedeveloper.com/design/q-a-evolving-the-combat-design-of-id-software-s-i-doom-eternal-i-),
  Hugo Martin describes new mobility making the early game too easy until AI
  and spaces were redesigned around it. Both oversized and undersized spaces
  caused problems. Application: measure what a grapple, dash or jump accomplishes
  in each encounter, beyond simply providing more speed.
- **Style should encourage experimentation.** In
  [Hakita's ULTRAKILL interview](https://intothebluesky.com/2020/12/19/interview-with-arsi-hakita-patala/),
  he discusses the tension between fragile FPS enemies and extended action-game
  combos, style encouraging players beyond the easiest solution, and offensive
  parries. Application: create rewards for useful combinations across enemies;
  avoid extending ordinary enemy health merely to make combos possible.
- **Pressure needs contrast.** Michael Booth's
  [Left 4 Dead AI presentation](https://steamcdn-a.akamaihd.net/apps/valve/2009/ai_systems_of_l4d_mike_booth.pdf)
  explains buildup, peaks and relaxation, distinguishing pacing from difficulty.
  Application: author changes in pressure within fights and contrast between
  rooms. Its multiplayer timings should not be copied directly into OVERDRIVE.
- **Design proposals need observation.** Mike Ambinder's
  [Valve playtesting presentation](https://cdn.akamai.steamstatic.com/apps/valve/2009/GDC2009_ValvesApproachToPlaytesting.pdf)
  treats designs as hypotheses and playtests as experiments, combining direct
  observation with measured behavior. Application: test one room and one design
  question at a time, then expand changes that help.

The experiments below are my proposals for OVERDRIVE, inferred from that
research and the repository. They are not claims that those developers endorse
these particular changes.

## Existing strengths to build on

The game already implements attack tokens, offscreen warnings, hit reactions,
poise, finishers with healing and reloads, enemy grappling, slide kicks,
shotgun jumps, friendly fire, environmental kills, weapon freshness and
Overdrive. It also has practice lessons, phase practice, a boss rush, room
attack, ghosts, challenge codes and an encounter workshop.

Recent room passes already improved Motor Assessment and Surgical Observation.
Proposals to add these systems from scratch would miss the current state.
The workshop is particularly useful for testing enemy composition before
investing in more geometry or mechanics.

## Prioritized experiments

### 1. Give each fight a changing tactical problem

**Priority: first. Relative effort: small to medium.**

Build encounters around interactions among two or three roles: something
pressures movement, something controls a useful route, and something creates a
high-value target. The roster already supports these roles through Rippers,
Patients, Shieldbearers, Grafted, Suspended, Conductors and Clerks.

For example, a Suspended controls a crossing while Patients pressure the
approach and a Grafted occupies the direct route. The player can interrupt the
beam, use the lower circuit, or commit to the high route and expose a different
angle. Killing the beam enemy materially changes the remaining encounter.

Keep the initial enemy budget fixed while comparing compositions. A larger
roster on screen is not automatically a more interesting problem. Ensure the
combination remains solvable without one mandatory weapon or trick.

**Look for:** players changing targets for a reason, using multiple routes and
finding more than one successful approach. Constant backward circling should
be a useful diagnostic baseline.

### 2. Make movement routes pay off during combat

**Priority: first. Relative effort: medium.**

The rebuilt Chapter 3 audit confirms Motor Assessment and Surgical Observation
meet the current geometric standard. The next candidates include Load Testing
(18% cover), Long-Term Care (19%), Celestial Observation (22%, 33.4 m sightlines)
and Hadal Research (21%, 34.6 m). These are measurements against the game's
own standard, not universal requirements for fun.

Give each route a purpose: a sheltered approach, an exposed precision angle,
a grapple flank or a short route through danger. Make high ground connect back
into combat through a second exit, a slam opportunity or another firing angle.
Cover should let players cross and re-engage; playtest how often it encourages
stopping versus moving.

Avoid treating additional measured standing layers as proof of useful
verticality. Celestial Observation and Hadal Research have several counted
surfaces but relatively little measured rise.

**Look for:** voluntary route switching, fewer long exposed crossings and less
time chasing the final enemy. Inspect navigation and combat together.

### 3. Let later tools coexist in an optional remix

**Priority: early prototype. Relative effort: medium.**

Current ordinary ownership checks enable SPLINTER in Chapter 4 and RECALL in
Chapter 5, with separate developer and training exceptions:
[`Gameplay_Splinter.h`](../src/game/Gameplay_Splinter.h) and
[`Gameplay_Recall.h`](../src/game/Gameplay_Recall.h). There is little normal-play
space for their interaction.

Prototype a full-tool version of one existing encounter. RECALL already lures
nearby ordinary enemies and shatters on return; SPLINTER already redirects
shots. A player could leave a lure, flank to establish a shot, and return to
shatter enemies gathering at the original position. Existing grapple and sword
options could provide alternative follow-ups.

First discover whether the combination produces satisfying decisions. Changing
permanent campaign equipment ownership, save handling and progression can follow
if that experiment succeeds.

**Look for:** tool combinations players choose to repeat, readable resource
states and a distinct purpose for each tool. Watch for a single safe loop
making every other option unnecessary.

### 4. Reward deliberate setups more consistently

**Priority: early. Relative effort: small to medium.**

In [`StyleSystem::addStyle`](../src/player/StyleSystem.h), ENVIRONMENT and
FRIENDLY awards are clamped against `PASSIVE_CAP` before they reach `earned`,
the accumulated run score. At or above the cap, those awards can become zero.
This affects the score as well as the meter.

Prototype separate treatment of run score and meter contribution. A deliberate
shove into a hazard or baited siege shell can earn a visible setup award even
when passive kills are prevented from filling Overdrive indefinitely. Distinguish
player-caused setups from unrelated enemy deaths before expanding rewards.

Recognize short causal chains, such as a slide kick opening a finisher or an
interrupted attack followed by a precision kill. Use a small vocabulary of
meaningful awards rather than adding a message for every input.

**Look for:** players noticing and repeating tactical setups, with no idle
score farming. The award should reinforce a useful action, not demand a
prescribed sequence.

### 5. Extend attack coordination to combinations of threats

**Priority: early. Relative effort: medium.**

[`assignAttackers`](../src/game/Gameplay_Feel.h) already limits ordinary
attackers and prioritizes visible threats. An extension could consider which
responses concurrent attacks demand, their impact times and safe routes.

For example, avoid overlapping a required low slide with a ground attack that
requires jumping when there is no third escape. Sequence the two closely enough
to create a skillful response. Audit persistent hazards and special encounters
alongside regular attack tokens, since a count alone cannot describe the whole
threat picture.

Existing chevrons and audio tells are a useful foundation. Check whether the
most urgent tell stays legible during Overdrive, heavy impacts and multiple
warnings before adding further effects.

**Look for:** players correctly identifying the threatening attack and the
response they missed. Compare challenge with unexplained damage, not just deaths.

### 6. Give fights an opening, a turn and a release

**Priority: early. Relative effort: small to medium.**

Test authored pressure changes before building a large adaptive director. An
opening establishes the main threat; a reinforcement changes the route or
priority; the ending rewards control and clears promptly.

Use the existing finite enemy budget. Reinforcements can enter when a relevant
enemy dies or a position is reached, provided the entrance is clearly announced.
Keep FAST and ranked encounters reproducible. Avoid hidden health or damage
changes in response to good play.

Between fights, preserve contrasts already present in the campaign: a quiet
record, a traversal problem, an unsettling view, then a compact ambush. The
factory, hospital and storage machinery can explain why each fight behaves
as it does.

**Look for:** memorable changes within a room, anticipation between rooms and
less fatigue across a chapter. Choose timings through play, not a borrowed formula.

### 7. Make boss mastery create offensive opportunities

**Priority: after an encounter prototype. Relative effort: medium to large.**

The Sorter and Ballast already have committed attacks, precision openings,
destruction and parry cancellation. Use those as references when reviewing
the rest of the roster.

For a selected attack, compare two viable responses: evade to preserve a firing
angle, or take the harder parry/interrupt to create a stronger opening. Let the
player cause some exposure windows. Where appropriate, damaging machinery
could remove one attack while introducing a clearly announced variation.

Avoid extending ordinary enemy or boss health as the default way to create
depth. Later phases can recombine learned attacks and change positioning while
retaining the same visual and sound language.

**Look for:** experienced players earning faster clears through understanding,
and newer players being able to explain a mistake. Track forced downtime and
repetition alongside survival.

### 8. Check whether Overdrive arrives at useful moments

**Priority: inexpensive investigation. Relative effort: small to medium.**

Overdrive currently starts automatically when the meter fills. Check how often
the triggering action is the final kill of a wave, leaving little to do during
the power state.

If this occurs often, compare the current behavior with a brief ready state
consumed by the next attack or a player-controlled activation. Consider input
complexity and the flow of FAST before choosing either. Give the same earned
power an opportunity to produce a memorable offensive burst.

**Look for:** useful active seconds, player understanding and whether saving
the state slows play. Keep automatic activation if it produces better flow.

## A concrete first prototype

Use Surgical Observation as a controlled encounter remix: its recent geometry
pass already provides several connected approaches.

1. Keep the existing 20-enemy budget, player equipment, checkpoint and room
   geometry for the first comparison.
2. Make the initial Suspended's beam lane easy to read from the overlook.
3. Use Patients to pressure the approach and a Grafted to contest the direct
   route. Position them so the lower circuit and gallery offer different answers.
4. Announce a small reinforcement after the beam threat is removed, changing
   which route is safest while preserving a reachable escape.
5. Compare against the authored encounter in the workshop. Only add a new
   environmental interaction if composition and placement cannot achieve the
   intended change.

This is an encounter concept, not a final spawn list. Clearance, pathing,
equipment availability and actual human play should determine its final form.

## How to decide what to keep

Start with a small qualitative sample, for example five fresh players and two
players comfortable with movement shooters. This is an issue-finding exercise,
not a statistically representative study. Counterbalance which room version
players see first so learning does not automatically favor the revision.

Observe first without coaching. Afterwards ask what changed their target
priority, why they chose their route, what hurt them and what they wanted to
try on another attempt. A 1–5 fun rating is supporting evidence, not the whole
decision.

Use existing logs for deaths, damage sources, finishers, stuns, weapon use and
Overdrive. Add only the measurements needed by the experiment: movement routes,
time spent hunting the final enemy, interrupted attacks, or Overdrive seconds
with live targets. Interpret these alongside observation; maximum weapon
switching and minimum deaths are not universal goals.

The present ordinary log contains 32 stretches totaling about 22 minutes. It
records 147 Revolver kills, 23 Shotgun, 3 Lancer and 1 Longshot, plus seven
finishers. This suggests asking whether other tools have clear opportunities;
it does not establish a balance problem. Equipment access, encounter mix,
player familiarity and old log schema all limit that sample. In particular,
the current rows lack parry input/contact fields: the summary's zero values
are not evidence that nobody parried.

## Suggested order

1. Test one encounter composition and its routes using the workshop.
2. Improve one remaining Chapter 3 room using what that test teaches.
3. Prototype the environmental score treatment and full-tool remix separately.
4. Review attack overlap and Overdrive timing using observed failures.
5. Expand successful patterns to another room and one boss.

New weapons, modes and bosses can wait until these experiments identify a
specific missing role. Existing replay systems already provide a place for
players to master better fights.
