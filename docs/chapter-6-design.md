# Chapter 6: THE KEPT RING — design

Brainstormed with the user on 2026-11-11, then reviewed the same day with
Fable 5.2 (second pass; what changed is listed under "Review pass" below).
Build steps 1 and 2 of section 11 exist (the ability, and the sweep on a
test ground: `./shooter --story6`); where the build stands and what comes
next is in `docs/chapter-6-build.md`. In code the ability is called
HOLDFAST, not ANCHOR (section 12, question 8).

The Restorer's dispatch capsule (end of Chapter 5) fires UNIT 24 out toward
the ringed gas giant. It lands in the rings. Everything the Cold Store ever
"dispatched" ended up here, held in orbit by THE SHEPHERD, a machine moon
whose job is to keep everything in its place. UNIT 24 has to cross the ring
toward it, level by level, while it sends out sweeps that throw strays off
the ring.

**From the user:**
- Land on the rings, then **cross a long distance through levels** to reach
  the boss.
- The boss is **really big, bigger than the Gleaner**.
- **The shockwave (the SWEEP):** every so often, at random, the boss sends a
  shockwave out toward the player. It **kills in one hit**, so it must be very
  noticeable, with a countdown or warning before it hits.
- **The countdown sits in the top-left corner of the screen**, like a nuke
  timer: **red numbers only, no flashing.**
- A space theme **in a different taste to Chapter 3**.
- The boss fight must **raise expectations above the Gleaner's**: a massive,
  epic fight with good dynamics and ideas.

**Working decisions** (proposed, not yet approved; use them as defaults, open
to change): the chapter title, the names THE SHEPHERD, THE CROOK, THE DROVER
and THE BELLWETHER, the ANCHOR ability, the eight crossing levels, the five
boss phases, the lines, and every number below. Section 12 lists the
questions still open.

### Review pass (2026-11-11): what changed

Fixes to things that contradicted each other or the game as built:

1. **The countdown is no longer "distance ÷ speed".** A fixed-speed wave that
   gives 20 s far away and 3 s at the boss means "far" is only about seven
   times "near", which fights the tens-of-kilometres crossing. The warning
   is now set per level; the flash comes earlier as a free cue (section 4).
2. **The sweep pushes away from the Shepherd, so it could not throw you
   toward its crown** in the Break. Phase 4 now uses a reversed pulse, THE
   GATHER (sections 4 and 6).
3. **Shelter lamps were amber, and so is every capsule lamp** in a field of
   thousands of capsules. Shelters are now mooring posts that light up only
   during a countdown (section 4).
4. **TALLIES can't "start the countdown lower"** without breaking the
   warning guarantee. They shorten the gap between sweeps instead
   (section 7). The same goes for the detuned countdown "skipping numbers"
   (section 6, phase 3).
5. **ANCHOR is already taken** three times: the enemy `EnemyType::ANCHOR`
   ("INSIDE ITS FIELD: NO DASH, NO GRAPPLE"), the Penitent's chain anchors
   and Ballast's `BALL_ANCHOR`. The name stays as the working name here;
   section 12 proposes replacements.
6. **ANCHOR needed limits.** As written, spiking any marked shelter from its
   exposed side made the shelter rule optional (section 5).
7. **Two chapters of silence in a row.** Chapter 5 already plays wind as its
   level music; a near-silent crossing eight levels long runs against the
   music direction. Section 9 keeps vacuum for the world and gives the
   crossing a score.

Additions (all proposals, each marked **(new)** where it appears):

- The sheepdogs are met on the crossing before they are fought on the tines.
- ANCHOR can pin an enemy in the open.
- Normal gravity returns near the Shepherd; low gravity is the crossing's.
- The finisher is the last countdown; the ending puts UNIT 24 on its own
  capsule.
- A playtest gate in the build order before eight levels are built.
- Section 13, "Feel and moments": a third pass of ideas for mood, sound,
  light and set-pieces. None of it changes the rules above.

## 0. What the chapter is about (new)

Three keepers in a row need to stay distinct. The Gleaner takes things back
(RECLAIM), the Restorer undoes change (RESTORE), the Shepherd fixes things in
**place**: nothing here is damaged or reset, it simply never arrives.

UNIT 24's thread: the Gleaner said it was sent and came late; the Ledger
says `NOT RECEIVED`. It was made, rejected, completed and copied, but never
**delivered**. The chapter ends with UNIT 24 sending itself (section 6, the
death). Purpose stays blank.

## 1. The taste: hard space, not dream space

Chapter 3's space was dreamlike: glass roofs, a nebula, impossible views,
soft light, choir. Chapter 6 is **physical**: real orbits, real scale, ice
and rock you can stand on.

- **Hard light.** No air, no fog softening anything. One small sun and
  pitch-black shadows; the gas giant's pale glow fills in the shadow side.
  The rings cast stripes of shadow across everything.
- **Vacuum sound.** Outside you only hear through contact: footsteps as
  thuds through the boots, impacts through the floor, servo whine, UNIT
  24's own fan. A thing the size of a city passing in total silence.
  **The rule that keeps combat readable:** vacuum muffles the *world*
  (far impacts, enemy movement, debris). UNIT 24's own weapons, parry,
  dash and hit confirms stay at full strength, heard through its body, and
  **attack tells and lock sounds are never muffled**, the same as they are
  never hidden by detail settings.
- **Real ring scale.** The rings are hundreds of thousands of kilometres wide
  and in places only about ten metres thick. Standing in them, a floor of
  drifting ice runs to the horizon both ways, and the gas giant fills half
  the sky.
- **Palette.** Chapter 5 already used frost white, black sky and amber
  lamps, so Chapter 6 takes its colour **from the giant**: sunlit ice that
  glitters hard white on one side and is lit butter and tan from the other;
  dust the colour of sand, not snow; black sky. Amber only on capsule lamps.
  No cyan in the world (UNIT 24's alone). The Shadow Band (black) and the
  Fall (re-entry orange) are the two places the palette breaks.
- **Low gravity** on the crossing: longer jumps, slower falls, splinters
  hang longer. Proposed scale 0.5 (`Player::gravity` is -24 m/s² and was
  chosen for snappy jumps, so lunar values would cost the game its feel).
  Fast-fall and the slam stay at full strength: they are how you get down
  in time when a countdown is short.
- **Normal gravity near the Shepherd (new).** It is the heaviest thing here.
  On the tines, inside it and on its surface gravity is back to full, so
  the boss fight plays as sharply as every other boss. Low gravity returns
  once, in the Break, when it loses its grip.
- **Down is always down**; no wall walking or rolling gravity (Chapter 3
  ruled free gravity rotation out; the camera never rolls).

## 2. The bridge: the end of Chapter 5

Chapter 5's capsule already climbs past the Store's cliff and leans away
toward the gas giant (`--overlay coldride`).

1. The capsule's view: the ice world shrinks behind, the rings come up as a
   thin bright line, then a plain.
2. Impact. The capsule ploughs into a field of other capsules. Silence.
3. UNIT 24 climbs out onto the ring. First vista (section 3, level 1): the
   gas giant, the ring running to the horizon, and far off one bright point
   with a long dark gap stretching behind it.
4. Chapter card: `THE KEPT RING`.

## 3. The crossing: eight levels

Each level is its own kind of place on the ring. The trips between them
happen off-screen or on a short ride (a drifting chunk, a capsule rail), so
the crossing feels like tens of kilometres without being walked. **The
Shepherd grows in the sky between every level**: a bright point at the
Landing, a disc by the Gap, horizon to horizon at the Shore.

Eight places, but not eight full levels: five are full, three are short
(about two minutes, one idea each). That keeps the long crossing the user
asked for and holds the chapter to roughly Chapter 5's ten-room size.

| # | Place | Size | What it is | How the sweep works there |
|---|---|---|---|---|
| 1 | **THE LANDING** | Short | The crash site in a field of thousands of dispatched capsules. Quiet. Learn low gravity. | First sweep from very far away, ~20 s of warning, with shelter in plain sight: you cannot fail to learn the rule. |
| 2 | **THE DRIFT** | Full | Slowly turning ice chunks, some building-sized. Jump and grapple between them. ANCHOR is found here (section 5). | Shelters are moving chunks: read which one will still be between you and the wave when it arrives. |
| 3 | **THE KEPT** | Full | A reef of capsules fused together, with siblings inside still waiting to be delivered. Lore, then the chapter's first real fight. **THE CROOK** appears for the first time (new, see below). | The reef is good shelter, and enemies use it too. |
| 4 | **THE SHADOW BAND** | Short | The gas giant's shadow crosses the ring: pitch black, lit only by your light, capsule lamps and the boss's flash. | Tests the floor, sound and mooring-post cues, because you barely see the front. |
| 5 | **THE SPOKES** | Full | Dark radial streaks of charged dust (a real ring feature). Inside a spoke the grapple and dash don't work: the ANCHOR enemy's field at landscape scale, so it reuses that code and its tell. | You can't grapple to shelter; you walk to it. Routes cross spokes at their narrowest. |
| 6 | **THE GAP** | Full | The edge of the gap the Shepherd has carved: a cliff of ring face over kilometres of nothing. Cross it riding a chunk. **THE DROVER** is herding chunks across it (new). | A sweep mid-crossing: shelter behind your own raft, anchored to it. |
| 7 | **THE WAKE** | Full | The boss's recent path. Its gravity has pulled the ring up into frozen vertical waves kilometres tall (also real). Climb one. **THE BELLWETHER** rings at the top (new). | The double sweep arrives during the climb. |
| 8 | **THE SHORE** | Short | The Shepherd's surface comes into reach: the boss fills everything. Gravity comes back as you walk in. | ~6 s of warning, breakers. No enemies: the walk-up. |

**Meet the sheepdogs before the fight (new).** The Gleaner forged its three
mini-bosses on the spot. Here each one shows up on the crossing first, doing
its job: the Crook drags a stray out of the Kept's reef in front of you, the
Drover pushes its herd across the Gap, the Bellwether rings on top of the
Wake. Each is a short encounter with one attack; at about a third of its
health gone it breaks off and leaves toward the Shepherd. On the tines they
come back with the full kit, still carrying the damage you did (the Crook's
bent hook, a gap in the Drover's herd). Three models pay for six
encounters, the crossing gets real threats, and the boss fight starts with
three grudges.

**Every level:**
- The **first sweep is scripted** to show that level's twist at a moment
  where the answer is in view. Later ones are random (section 4).
- **A checkpoint before every sweep set-piece**, and at least every two
  minutes of play. Dying to a sweep must never cost more than that.
- No sweeps in quiet rooms: inside the Kept's capsules and anywhere the
  player is reading.

**Secrets:** a capsule from Units 1–23 that got this far and opened; a
sibling still awake inside the Kept, counting; a capsule labelled for UNIT
24 (empty: it was never sent, because UNIT 24 was never delivered; it comes
back in the ending, section 6).

## 4. THE SWEEP (the shockwave)

**What it is.** The Shepherd keeps everything in the ring in its place. Every
so often it pulses, and the pulse pushes strays back into line. It doesn't
damage you as such: it **flings you off the ring** into space, away from the
Shepherd. That is why it kills in one hit, and why the death reads as fair.

**How it differs from what the game already has.** Ballast's crosswind and
the Gleaner's searchlight are also "get behind something". The sweep must
not be a third copy. What is new: it is **lethal**, it runs on a **clock you
can read**, it follows you through **the whole chapter**, the **shelter
moves** (the Drift, the Drover, the raft), and it ends as **your transport
and your finisher**. Every level and phase should use at least one of
those; a level where the sweep is only "stand behind the big rock" needs
another idea.

**The warning** (several cues at once, so nobody misses it):
1. **The flash.** The Shepherd's ridge lamps light up on the horizon,
   visible from anywhere on the ring. The flash comes **before** the
   countdown: far out, several seconds before. It is a free early warning
   for players who watch the sky.
2. **The countdown (user).** Top-left corner, red numbers, no flashing, like
   a nuke timer. It starts when the front comes over the horizon and
   reaches 0 exactly as the front reaches the player. Proposed additions,
   still open (section 12): a small `SWEEP` label above the number, and a
   dull tick through the floor each second.
3. **The mooring posts.** Every valid shelter carries a post. **Posts are
   dark until the flash, then all of them light at once** and stay lit
   until the wave has passed. The field tells you where to go at the moment
   you need to know.
4. **The floor.** Dust lifts off every surface in a line racing toward you;
   a low hum builds through your feet.
5. **The front.** A bright, rising wall of lifted ice crosses the ring. Its
   position is always visible.

**The answer: one rule everywhere.** **Get into the shadow of something
heavy.** Big anchored chunks, fused capsule reefs and the boss's own debris
count, and each has a mooring post. Behind one, the wave passes over. In
the open, you're thrown off. Being in the air doesn't matter either way:
the test is only whether a shelter stands between you and the source when
the front reaches you.

**Warning time is set per level and shrinks as you get closer.** In the
fiction the wave has less far to come. In the build each level states its
warning, and the front is spawned warning × speed away, so the countdown
and the visible wall always agree.

| Distance | Warning | Gap between sweeps | What changes |
|---|---|---|---|
| Far (levels 1–3) | ~20 s | 90–150 s | One clean wave. Teaches the rule. |
| Mid (levels 4–6) | ~10 s | 70–120 s | **Double sweep:** two waves a few seconds apart; stay sheltered. |
| Near (levels 7–8) | ~6 s | 50–90 s | **Breakers:** small shelters shatter; only the posts still lit after the first second hold. |
| The boss | ~3–4 s | On the music (section 9) | **Aimed sweep:** a cone at you instead of a full ring (section 6). |

The gaps are deliberately long. A 20 s countdown every minute would mean a
third of the early chapter is spent waiting behind a rock; the sweep should
interrupt play, not become it.

**Fairness rules:**
- Random, but inside the window above.
- Never within 10 s of a checkpoint or respawn, never during a cutscene, and
  never while another lethal attack is pending (except where section 6 says
  otherwise, which is an open question).
- **Level-design rule:** shelter is always reachable within the warning
  time from anywhere you can stand, at walking speed, counting **only fixed
  shelters** (not ones an enemy can move or a breaker can shatter). As
  warning time shrinks, shelters sit closer together. A test checks this
  for every level (section 10).
- **The countdown never lies.** Nothing shortens a countdown that has
  started, and nothing makes the number disagree with the front.
- **RECALL is a legal answer.** The statue lasts 8 s, so once warnings are
  8 s or less you can plant it in shelter on the flash, keep fighting and
  snap back. This is intended, and Phase 2 teaches it.
- Enemies are thrown off too. Luring a group into the open before a sweep is
  a valid tactic, and it pays: `SWEPT x4` in the feed, with style credit.
- Detail settings never hide the flash, the countdown, the posts or the
  front (the same rule as Ballast's crosswind).

**THE GATHER (new; boss only).** A shepherd also brings the flock in. In the
Break the pulse reverses: the front starts far out and closes **inward**,
throwing everything loose toward the Shepherd. Same flash, same countdown,
but the wall comes from behind you. This is what makes "ride the sweep"
possible (phase 4), and it is the one time being thrown is not death.

## 5. The new ability: ANCHOR (working name; see section 12)

Found in the Drift, in a capsule whose sibling used it to hold on.

- **A mag-spike** fired from the forearm, about 30 m of line. Hit a surface
  (a chunk, a capsule, the boss's hull) and UNIT 24 is held to it: riding a
  drifting chunk, you move with it.
- **It holds you through a sweep only on the sheltered side.** The spike
  keeps you from drifting, it does not replace shelter: anchored to a
  shelter's exposed face you are still swept. Anchored to a small chunk,
  you go off with it. The cases where the spike alone saves you are the
  ones the level sets up on purpose: the raft in the Gap, the hull in the
  Fall.
- **While held you are tied to the line:** free to aim, shoot and parry,
  free to swing within its length, not free to leave. Press again to let
  go. One spike at a time.
- **Pin an enemy (new).** Spike a small enemy (a Stray, a Herder) and it is
  nailed where it stands for a few seconds: it can't reach shelter. This
  gives ANCHOR a use in every fight, and it makes the sweep your tool from
  level 2, long before phase 4.
- **Against the Crook:** an anchored player can't be dragged; the hook
  catches and the line holds. The Crook answers by cutting lines later on.
- **With RECALL:** spike a chunk, RECALL back to it later, wherever it has
  drifted (inside the 8 s the statue lasts).
- **In the boss fight it is the key** (section 6, phases 4 and 5).
- Its own rebindable key, like RECALL, shared with nothing (the lesson from
  the F finisher losing to the parry); text `ANCHOR` on unlock, `ANCHORED`
  and `PINNED` in the feed.

## 6. THE SHEPHERD

`UNIT ? · SHEPHERD · PURPOSE: KEEP`

(The Gleaner's death line is *"I KEPT ALL OF YOU."* If KEEP reads as a
repeat, `PURPOSE: TEND` or `PURPOSE: HOLD` are the alternatives; section 12.)

**Scale.** The Gleaner is about 580 m long as built. The Shepherd is a
**moonlet about 8 km across**, the size of the real shepherd moon Daphnis.

**Body.** A walnut shape with a sharp ridge around its middle (the real moon
Pan looks like this). The ridge is where the sweep fires from. Rows of lamps
run along it like a lighthouse. Kilometres-long **tines** drag through the
ring like the teeth of a comb, carving the gap. It is patient, slow, and
counts everything.

**Lines** (terse; bosses speak as workers, `docs/text-pass.md`):
- Before: *"YOU WANDERED. THEY ALWAYS WANDER."*
- During the Turn: *"STAY. EVERYTHING HERE STAYS."*
- During the Fall: *"WITHOUT ME, THEY DRIFT."*
- Death: *"I NEVER KNEW WHERE THEY WERE GOING. ONLY THAT THEY STAYED."*

**How it raises the bar over the Gleaner:**

| Gleaner | Shepherd |
|---|---|
| 580 m long | 8 km across |
| You move between areas | The boss changes the world: it rolls over, splits the ring, falls out of orbit |
| Mini-bosses are forged in front of you | You have met all three on the crossing; they come back hurt and angry |
| Mini-bosses are separate fights | Mini-bosses combine with the sweep (hook, herd, a second clock) |
| One signature ability (Splinter) | Grapple, Splinter, RECALL and ANCHOR each get their moment |
| The killer attacks are things to avoid | The killer attack becomes your tool, then your transport, then the finisher |
| It ends on the plain | It ends falling into the gas giant |

**A caution on shape.** Tines with three mini-bosses, then inside through
three climbing rooms, then a duel on top is the Gleaner's outline (brood,
organs, summit) with two phases added. The phases that are new in kind are
the Turn, the Break and the Fall, so those get the most build time and the
biggest spectacle; the Comb and the Resonator should be kept tight rather
than long.

**The sweep during the fight** is scheduled on the music (section 9), not
random, and it is always the aimed cone unless a phase says otherwise.

### Phase 1 · THE COMB (the tines)

Three tines, each kilometres long, plough forward through the ring. You
fight **on top of them as they move**, ice spraying off the front like a
bow wave. Each tine is guarded by a sheepdog, and each one changes how the
sweep plays. One at a time, never three at once.

- **THE CROOK.** Thin, with a long hooked arm. **During the countdown it
  hooks you and drags you out of shelter.** The hook is white: parry it, and
  the Crook is left stuck in the open when the wave hits. ANCHOR also
  stops the drag, until it starts cutting your line (gold tell).
- **THE DROVER.** Pushes a herd of ice chunks in formation as moving walls.
  Your shelter moves with the herd, so you keep in step with it. Its weak
  point shows when the herd splits around a tine. Pin one of its chunks
  with ANCHOR and the formation breaks around it.
- **THE BELLWETHER.** Leads a flock of capsules and rings a bell that fires
  **small local sweeps** on its own rhythm, alongside the Shepherd's big
  one: two clocks at once. Local sweeps knock back and hurt; they do not
  kill, and they have no countdown (the corner number belongs to the
  Shepherd alone). Splinter the bell's clapper to scatter the flock. Keep
  its bell unlike the Caller's cracked iron toll: small, bright and quick.

Shelter on a tine is its own structure (fins, housings), each with a
mooring post. Break a tine and it snaps off; the Shepherd's path through
the ring shifts.

### Phase 2 · THE TURN

With its comb broken, the Shepherd **rolls over to face you.** You stay
level; the gas giant and the rings swing across the whole sky. This is the
chapter's "the ridge stands up" moment.

- Its ridge now faces you. **For about 5 s after each sweep, the vents along
  it stay open**: survive the wave, then sprint in. That's the way inside.
- **RECALL into shelter** is taught here: leave the statue behind cover,
  fight in the open, snap back on 1. Warnings in this phase are 6 s so the
  8 s statue always covers them.
- Three vents to break, each further along the ridge, so each run after a
  wave is longer than the last.

### Phase 3 · THE RESONATOR (inside)

The sweep is made by a resonator: the Shepherd rings itself like a bell.
Three rooms, climbing:

1. **The Tuning Hall.** Tuning forks the size of towers. Inside, the sweep
   is a pressure blast down the corridors; shelter in side rooms. Breaking
   forks **detunes** it: each broken fork makes the sweeps come at less
   regular intervals and makes the countdown's digits waver and smear.
   Each warning stays full length and honest; only the rhythm between
   sweeps is lost.
2. **The Ledger.** Quiet lore, no sweeps. Every capsule in the ring
   counted, by name. A line for UNIT 24: `NOT RECEIVED`. A line for each
   sheepdog you fought, and the sibling from the Kept, still counting.
3. **The Lamp Well.** A vertical shaft through the core to the surface,
   climbed by grapple while the shaft rings around you. Blasts come up the
   shaft; alcoves are the shelter, and ANCHOR holds you in one.

As inside the Gleaner, this is a separate set of rooms, the floor stays
level, and the motion is sold with props, light and sound.

### Phase 4 · THE BREAK

Back outside, the Shepherd **tears the ring apart around itself** and loses
its grip: low gravity returns. The ground crossed all chapter becomes a
storm of drifting debris.

- **Ride the Gather.** Detuned, it stops pushing strays out and starts
  pulling everything in (section 4). ANCHOR onto a chunk just before the
  front arrives and it **throws you and the chunk across kilometres**
  toward the crown. The thing that killed you all chapter is now your
  transport. Unanchored, you are still thrown, alone and into the hull:
  heavy damage, not death, and back to the last chunk.
- Three rides, each to a higher ledge of debris, with a fight between
  them. Low-gravity combat between drifting chunks: chain splinters through
  the debris, RECALL out when a chunk breaks under you.

### Phase 5 · THE FALL

Broken and detuned, the Shepherd **loses orbit and falls toward the gas
giant**, with you on its surface. Gravity is full again and rising.

- The sky changes as the fight goes on: black space, the ring overhead
  receding, the pale cloud tops rushing up, then **re-entry fire** pouring
  over its ridge.
- Only the **aimed sweep** is left: a cone at you, 3–4 s of warning. The
  answers: leave the cone, get behind a ridge fin, or ANCHOR to the hull on
  a fin's far side. The cone is drawn on the hull in red for the whole
  countdown.
- **The finisher is the last countdown (new).** The corner timer starts one
  more time. UNIT 24 drives ANCHOR into the core lamp (first person, like
  the Gleaner's tearing) and pins the resonator **shut while it fires**:
  the number reaches 0 and nothing comes out. The sweep goes off inside
  it. The `0` stays in the corner, red and still, through the death, then
  goes dark. The Restorer's finisher was also "drive the chapter's ability
  into its head", so what makes this one its own is the clock.

**The death.** UNIT 24 jumps clear as the Shepherd burns down into the
clouds, and lands on a drifting capsule: **the one labelled UNIT 24 (new)**,
empty, never sent. Far across the ring, everything the Shepherd held starts
to drift loose, capsules included, the counting sibling among them: a big,
quiet ending shot. UNIT 24 was never delivered; now it rides its own
capsule out, and nobody is keeping it. Whether the drifting is freedom or
loss is left open (*"WITHOUT ME, THEY DRIFT"* was not a lie). Hook for
later: a future chapter can find the wreck in the clouds (the THE SOUNDER
idea from the brainstorm).

**Health:** one pool across all five phases with checkpoints between them,
like the Gleaner. Proposed: split into visible segments, one per phase, so
the size feels earned (open, section 12). The sweep's one-hit kill applies
in every phase except the Gather; everything else uses normal damage.
Checkpoints also sit between the three sheepdogs and between the three
rides: a one-hit kill in a five-phase fight must never replay more than
one beat.

## 7. Enemies on the crossing

Proposed, smaller than the boss and its sheepdogs:

- **STRAYS.** Units that climbed out of their capsules. Weak, numerous,
  drift toward you in low gravity. Cheap fodder that the sweep also clears.
  When the posts light they run for shelter too, and fight you for it.
- **HERDERS.** Small drones that shove chunks: they push loose shelter out
  of position during a countdown. A priority target, like Chapter 5's
  Clerks. They cannot move a fixed shelter, so the reach guarantee in
  section 4 always holds without them being killed.
- **TALLIES.** Stationary counters on the reef. While one is alive, sweeps
  come **more often** (the gap between them is halved). The warning itself
  never changes.

## 8. Interface

- **The sweep countdown** (section 4): top left, red numbers, no flashing.
  Shown only while a sweep is inbound; the corner is empty otherwise, so
  the number appearing is itself a cue. A dark plate behind it keeps red
  readable against the giant and against re-entry fire.
- **Double sweep:** one number at a time. When the first wave passes, the
  number restarts for the second. A small row of pips under it shows waves
  left.
- Whole seconds down to 3, then tenths, so the last moments read exactly.
- During the boss fight, the boss health bar at the top as usual; the
  countdown stays top left and never overlaps it. Check what Story mode
  already draws in that corner before placing it.
- Death to a sweep shows the normal death screen, with the cause as
  `SWEPT`.

## 9. Music and sound

- **World sound:** vacuum outside (section 1): contact sounds only, the
  floor hum of each sweep, a low ring in the distance. Inside the Kept and
  the Resonator, air and echo return.
- **Level music.** Not silence: Chapter 5 already used wind as its score,
  and the soundtrack is one of the game's strengths. The crossing has a
  sparse, dry pulse "inside the helmet" that the sweep tick sits on, and
  every fight on the crossing brings in a full-tempo combat layer. True
  silence is kept for three moments so it lands: climbing out at the
  Landing, the Shadow Band, and the ending shot.
- **Boss track:** loud and at full tempo with no half-time lulls (the music
  rules in `docs/boss-music-pass.md`): techno, drum and bass or hardstyle,
  rising each phase, the biggest drop in the game for the Fall.
- **The sweep sits on the beat.** The countdown always reaches 0 on a
  downbeat, so the drop is the wave. For a countdown in seconds to stay on
  the grid, the tempo has to divide evenly: at **180 BPM** one second is
  exactly three beats and a 4 s countdown is exactly three bars; at 150 BPM
  an 8 s countdown is five bars. Pick each phase's tempo and warning
  together. `MusicSynth` already owns the playhead (Chapter 5's
  `MusicSynth::rewind`), so the boss can schedule the sweep against it.
  Outside the boss fight the sweep stays random within its window.
- **Key sounds:** the countdown tick through the floor, the ridge flash,
  the posts lighting, the front passing overhead, the Crook's hook, the
  Bellwether's bell, the tines grinding through ice.
- Positional sounds die past 90 m (`SfxMixer::MAX_DIST`): far events (the
  sweep's source, tine impacts, the boss's roll) are placed toward the
  player late and quiet, as on the plain.

## 10. Technical plan

- **Scale.** The Shepherd at tens of kilometres is far past any far plane
  (Chapter 5's camera sees 5.2 km). Draw it as a sky element (an impostor
  in a new skybox branch next to `uPlain` and `uCold`) whose size grows per
  level, and switch to real geometry only at the Shore. Only reachable parts
  are detailed: tine decks, the ridge vents, the Resonator rooms, the
  surface. Each level and each boss phase is built around its own origin.
- **The sweep is data:** origin, direction (out, in for the Gather, or a
  cone), speed, warning, wave count. One system drives the front, the
  countdown, the posts and the kill, so they cannot disagree.
- **Shelter test:** is a marked shelter between the player and the sweep's
  source at the moment the front passes? Ray or box test against shelter
  boxes only, never against decoration. Ballast's crosswind already does
  this against windbreaks (`src/bosses/ballast/`, the shared crosswind
  geometry, including breakable cover updating collision): start from
  that.
- **Shelter-reach test:** for every level, sample standing positions and
  check a fixed shelter is reachable within the warning time at walking
  speed under that level's gravity. Same spirit as `make test-plain`'s
  real-movement walk.
- **The Spokes** reuse the ANCHOR enemy's no-dash, no-grapple field as a
  level volume.
- **Gravity:** a per-chapter scale on `Player::gravity` (-24 m/s² today)
  and on the splinter's gravity (`Splinter::GRAVITY`), set per level and
  per boss phase. Fast-fall (`FALL_MULT`) and the slam are not scaled.
- **Moving decks** (tines, drifting chunks, the anchored raft): the player
  moves with the platform. Chapter 5's Hook Line carriers already do this;
  reuse that.
- **The Turn and the Fall** move the sky, never the floor or the camera roll:
  the skybox rotates (Turn) and its colour, the giant's size and the
  re-entry glow change (Fall), like the Gleaner's back deck.
- **ANCHOR:** `src/player/Anchor.h` (logic) and `src/game/Gameplay_Anchor.h`,
  testable anywhere with `--anchor`, like `--splinter` and `--recall`.
  These names collide with `EnemyType::ANCHOR` in searches and in the enemy
  viewer; settle the ability's name (section 12) before the first file.
- **Sheepdogs twice:** each has a crossing form (one attack, leaves at a
  health threshold) and a tine form (full kit); damage dealt on the
  crossing is carried in the chapter's save state.
- **Countdown HUD:** in `Gameplay_HUD.h`, top left; must stay readable on the
  web build and at every HUD scale.
- **Code layout:** `src/world/LevelRing.h`, `src/game/Gameplay_Ring*.h`,
  `src/bosses/shepherd/`, the sheepdogs in `src/enemies/`, models in Blender
  (`tools/blender/shepherd_build.py`).
- **Save:** `StorySave` rejects chapter > 5 today; allow 6 with its
  checkpoint count. ANCHOR is owned once the Drift is passed.
- **Tests:** `make test-ring` (level, shelters, sweep timing and fairness
  windows, countdown and front agreeing at zero, save) and a runtime check
  in `tests/test_dev.cpp`.

## 11. Build order

1. **ANCHOR** (testable anywhere with `--anchor`), including pinning.
2. **The sweep and its countdown** on a test arena: front, posts, shelters,
   the one-hit death, fairness windows, the shelter-reach test.
3. **Playtest gate.** Play the test arena with Strays, a Herder and a
   Tally for ten minutes. If sheltering from the sweep is not fun there,
   eight levels of it will not fix that: tune the windows, warnings and
   ANCHOR here, before any level exists.
4. The ring: sky, gas giant, ring plane, capsules, chunks, low gravity. Walk
   the Landing at full scale.
5. The Shepherd as a far impostor that grows per level.
6. **A vertical slice:** the Landing, the Drift and the Kept with the
   Crook's crossing encounter. Playtest again.
7. Levels 4–8, with their enemies and the other two sheepdog encounters.
8. Boss phase 1: the tines and the three sheepdogs, one at a time.
9. Phases 2–3: the Turn and the Resonator.
10. Phases 4–5: the Break, the Fall and the finisher.
11. The Chapter 5 → 6 hand-off and the menu entry.
12. Music (the sweep on the beat; tempo fixed with the warnings in step 8),
    sound, tuning, tests.

## 12. Open questions

Carried over, with a recommendation each:

1. **Can the sweep combine with other attacks in the Fall**, or should it
   always come alone? Recommended: it may overlap non-lethal attacks only,
   and nothing may push or pull the player during the last 2 s.
2. **One health bar in segments, or a bar per phase?** Recommended: one bar
   in five segments.
3. **A second countdown in the Fall** (to the atmosphere)? Recommended: no;
   let the sky do it, so it never competes with the sweep timer.
4. **The countdown's extras:** a small `SWEEP` label above the number, and
   a tick through the floor each second? Recommended: both, and the label
   reads `GATHER` in the Break.
5. **How long is the crossing?** Recommended: keep eight places, five full
   and three short (section 3). Cutting further would mean folding the
   Shadow Band into the Spokes and the Shore into the Wake.
6. **The chapter title and all names** are working names.
7. **The Chapter 1 tower:** Chapter 5's doc called it the natural landing for
   Chapter 6. This design goes to the rings instead; does the tower come
   back later (or appear in the distance here)?

New from the review:

8. **Rename ANCHOR?** It collides with an enemy, the Penitent's anchors and
   a Ballast attack. Candidates: **HOLDFAST** (recommended; feed text
   `HELD`), MOOR, SPIKE, TETHER.
9. **`PURPOSE: KEEP`, or TEND / HOLD,** given the Gleaner's *"I KEPT ALL OF
   YOU"*? And is three keepers in a row (Gleaner, Restorer, Shepherd) the
   intended pattern, with section 0's split between them?
10. **Sheepdogs on the crossing** (section 3): yes, or keep them for the
    boss as a surprise?
11. **The ending on UNIT 24's own capsule** (section 6): keep it? It names
    no destination, so where the story goes next can be decided after the
    boss is built (user).
12. **THE GATHER** as the fix for riding the sweep, or another way to get
    thrown toward the boss?
13. **Level music on the crossing** (section 9): sparse pulse plus combat
    layers, or hold to near-silence outside the boss?
14. **Low gravity at 0.5 on the crossing and full gravity near the
    Shepherd:** right split?
15. **Which of section 13's ideas are in?** Recommended as the core set:
    the sound arc (13.1), the count (13.2), the tidy ring (13.3), the swept
    death (13.6) and the fleece (13.8).

## 13. Feel and moments (new, third pass)

Ideas for mood rather than rules. Each one is marked **cheap** (text, sound,
shader or tuning), **medium** (a small system) or **big** (a set-piece).

### 13.1 The sound arc: touch, then air

Vacuum is the chapter's best asset if it has a shape instead of being a
filter.

- **You hear what you touch (cheap).** Stand on a capsule and there is
  knocking from inside. Lean on the reef and it creaks. Nothing you aren't
  touching makes a sound.
- **ANCHOR is a stethoscope (cheap).** The line carries sound. Spike a
  capsule and you hear the sibling inside; spike a drifting chunk and you
  hear it groan before it cracks; spike the Shepherd's hull and you hear
  it **counting**. This gives the ability a reason to be used when nothing
  is attacking, and it is how the secrets are found.
- **The Shepherd speaks through the floor (cheap).** Its lines on the
  crossing arrive as a vibration under your feet, with the text shaking
  slightly. It has no voice in the air because there is no air.
- **Capsules are rooms (cheap).** Checkpoints are open capsules: step in,
  the hatch seals, air hisses and every sound comes back warm and close.
  Step out and it is cut off again. The player learns to feel safe by ear.
- **The zero (cheap).** On the crossing, when the countdown reaches 0 all
  sound cuts for one beat as the wall goes over, then returns with ice
  pattering on the shelter. In the boss fight the same instant is the
  drop.
- **Air comes back in the Fall (medium).** As the Shepherd hits atmosphere,
  real sound arrives outside for the first time in the chapter: a rising
  roar, your own shots cracking and echoing, and the Shepherd's last lines
  heard as an actual voice. Six levels of silence pay for this moment.

### 13.2 The Shepherd counts

It isn't hunting you. It is doing its job, and you are an error in the
total. That makes it colder than the Gleaner's hunger or the Restorer's
politeness.

- **A running count (cheap).** After each sweep a small line appears where
  the countdown was: `COUNT: 40,112. ONE EXTRA.` You are the one extra.
  Kill a Stray and the next one reads `ONE SHORT.` Die to a sweep and the
  death screen reads `SWEPT · COUNT CORRECT`.
- **The sweeps aren't about you at first (cheap).** In levels 1–3 the ridge
  lamps flash in a steady pattern, the same every time. From the Kept on,
  after you fight the Crook, the lamps start **turning to follow you**
  between sweeps. No attack, no beam. It has noticed.
- **The awake sibling (cheap).** The counting sibling in the Kept is timing
  the gaps between sweeps and has been for years. Spike its capsule during
  a countdown and its voice counts in step with your corner number.
- **Level cards carry the distance (cheap).** `THE DRIFT · 61,000 KM`,
  falling each card, in the same red digits as the countdown.

### 13.3 The tidy ring

- **A sweep leaves order (medium).** After the wall passes, the loose dust
  and small debris lie in clean parallel lines, like raked gravel. Over
  the next minute it drifts back into mess, so you can read how long ago
  the last sweep was by how neat the ground is.
- **It cleans up after you (cheap).** The game already leaves marks where a
  fight happened. A sweep erases them: scorch, wrecks, casings, all gone.
  The only trace you ever leave here is removed on schedule.
- **Where the swept go (cheap, shader).** A few seconds after each sweep,
  small sparks streak across the gas giant's face: the strays it threw
  off, burning up. Nobody explains it.
- **The grooves (cheap, shader).** From any height the ring shows fine
  concentric lines, combed by the tines. From the top of the Wake the
  whole ring reads as a record and the Shepherd as the needle.

### 13.4 Light

- **The wall's shadow arrives first (cheap).** The sun is low, so the sweep
  front throws a long shadow ahead of itself. The ground goes dark a
  moment before the wave hits: one more honest cue, and a frightening one.
- **Capsule lamps flinch (cheap).** On the flash, every capsule lamp in
  sight dims at once while the mooring posts light. The kept things brace.
- **Storm light in the Shadow Band (cheap, shader).** The level is black,
  but the giant's night side has lightning: silent flickers in the clouds
  that light the whole ring grey for a frame, showing how much is out
  there, then nothing.
- **Sunrise out of the Shadow Band (medium).** The level ends as the sun
  clears the giant's edge: a thin red arc of atmosphere, a diamond flash,
  then hard white light sweeps along the ring toward you at the same
  speed a sweep would. The one wall in the chapter you don't hide from.
- **Eclipse at the Shore (cheap).** The last walk is in the Shepherd's own
  shadow. The sun is gone behind it and its ridge lamps are the only
  light.

### 13.5 Low gravity you can feel

- **Nothing falls down (medium).** Wrecks, casings and broken ice hang and
  turn slowly where the fight happened, glittering in the hard sun, until
  the next sweep takes them. A long fight fills the air with its own
  history.
- **Punches send things a long way (cheap, tuning).** Knockback is doubled
  in low gravity. Punching a Stray out of shelter during a countdown, or
  off the edge of a chunk, should be a thing players discover and grin at.
- **Slam dust (cheap).** A slam raises a ring of dust that hangs at waist
  height for several seconds instead of settling.
- **Dust on the hands (cheap).** Sand-coloured dust gathers on UNIT 24's
  arms, like Chapter 5's frost. A wave passing overhead blows them clean.
  In the Spokes, static crawls over the fingers instead.

### 13.6 The swept death

Dying to the sweep should be the best death in the game, because players
will see it more than any other here (medium).

- First person. The shelter you didn't reach slides past. The ring drops
  away, tumbling, and in two seconds it is a bright line across the giant.
  Total silence. `SWEPT · COUNT CORRECT`.
- Short and skippable with held Jump, so it never slows a retry.
- The Strays get the same treatment at a distance: thrown bodies shrink to
  points against the giant. Some never run for shelter at all; they stop,
  turn and face the flash.

### 13.7 Moments on the crossing

- **The Landing:** the capsule field has paths worn between the capsules.
  Something has been walking here for a long time. (cheap)
- **The Drift:** chunks nearer the giant orbit faster, so the whole level
  slides past itself in lanes, like slow traffic. Step between lanes to
  travel. (medium; it is also how the level moves you forward)
- **The Kept:** Strays holding on to each other in a chain along the reef,
  hand to wrist. A sweep takes the chain from the far end. (cheap)
- **The Gap:** look over the edge and the ring is ten metres thick: a
  sheet of ice with the giant's clouds directly below. Ice grains pour
  off the lip in a slow, silent fall. (cheap)
- **The Wake:** the summit is the first time you stand above the ring
  plane. Everything you crossed is visible at once, back to a glint at the
  Landing. Hold the camera here for a few seconds before the Bellwether
  rings. (cheap)

### 13.8 Images for the boss

- **The fleece (big).** The Shepherd's far side, hidden until the Turn, is
  covered in capsules: tens of thousands pressed against its hull like
  wool, the ones it kept closest. When it rolls over, that is what faces
  you. In the Fall they peel away in sheets and burn.
- **No face (cheap).** The Gleaner had an eye. The Shepherd has only rows
  of lamps that move together. It should never look at you with anything
  you could call an eye.
- **You learn what the hum is (cheap).** Every sweep on the crossing comes
  with the same low note through the floor. Inside the Resonator there is
  air, and you hear it properly for the first time: it is a chord, one
  note per fork. Each fork you break removes a note, and every sweep after
  that sounds wrong.
- **The Turn is slow (cheap).** Thirty seconds of the sky rotating in
  silence before the music comes back. The size is sold by how long it
  takes.
- **The count ends (cheap).** After the death, the corner line appears one
  last time: `COUNT: —`.
