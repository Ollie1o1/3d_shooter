# Chapter 4: THE BLOOD MOON PLAIN — design

Brainstormed with the user on 2026-11-02. Chapter 4 is a single colossal boss
chapter: a short arrival, then one long fight against THE GLEANER across an
endless plain, inside its body and finally on top of it.

Structure reference: the user wants the *shape* of a colossus fight (a huge
open arena, mini-bosses first, then going inside the boss). Everything that
is seen, heard or fought must be original: no tower walker, no antennas, no
copied defence systems, brain or names.

## 1. The bridge: the end of Chapter 3

This replaces the current Chapter 3 ending (the long corridor over the void).

1. THE SUCCESSOR falls. The camera leaves first person and shows UNIT 24
   standing over the body.
2. **Taking the pieces.** The Successor is "the result the lab wanted", the
   finished version of UNIT 24. UNIT 24 takes its missing parts from that
   body: a shoulder plate, the forearm housing, the spine coupling. Each
   snaps on with a spark; the cyan power spine lights up fully for the first
   time.
3. **The nameplate.** `UNIT 24 · PURPOSE: ——— · STATUS: REJECTED` flickers to
   `STATUS: COMPLETE`. Purpose stays blank. No speech.
4. **The new forearm** opens and shows the SPLINTER (section 3) once.
5. **The hesitation.** At the 42 m portal UNIT 24 stops and looks back at the
   Successor, then walks through. White out.

## 2. Arrival: the plain

- **Place.** A flat, cracked, endless plain under a red sky with burning cloud
  bands. Meteors fall slowly at all times, with long smoke trails.
- **The meteors are bodies.** Up close they are rejected units and scrap,
  thrown away from somewhere above. It is the Scrap Pit from Chapter 1, but
  the whole sky. Impacts leave craters, rubble and burning wreck piles that
  the fight uses as cover and ramps.
- **The reveal cutscene (about 15 s, skippable with held Jump).**
  1. UNIT 24 lands, stands, looks left and right. Nothing moves except the
     meteors and a long mountain ridge on the horizon.
  2. The ridge rises. It is the Gleaner's back.
  3. It turns around. The blood moon was never a moon: it is the Gleaner's
     eye-core, hanging in its open chest. It opens and looks straight at you.
  4. Its first line, then control returns.

## 3. The new ability: SPLINTER

The reward for being complete. A coin-style trick-shot tool, but its own
thing: UNIT 24 flicks a shard of cyan glass out of its new forearm.

- **Throw.** The grenade button (G / D-pad up), which Story never used.
  Three charges; each one regrows in about 2.5 seconds.
- **Hang.** A splinter rises, then hangs spinning at the top of its arc for
  about half a second while it **glints**, then drops and breaks on whatever
  it hits.
- **Shoot it.** A round that hits a splinter is redirected from it:
  - to another live splinter first (a **chain**, each link hits harder),
  - otherwise to the best enemy weak point it can see: a head, an open core,
    a marked organ.
  - Base x1.5 damage, +0.5 per link, and x1.25 more if the splinter was
    glinting. Redirected rounds always count as weak-point hits.
- **Why it matters here.** The Gleaner is far away and enormous; its weak
  points face away from you or sit behind its own arms. Splinters let you
  hit them around corners and from below. Mini-bosses are built to be
  splintered (each has a weak point that is hard to hit directly).
- **Later (not in the first build): PIN.** Parry a splinter with the fist and
  it flies straight and sticks in what it hits. A pinned target is preferred
  by every later redirect, which is how you keep pressure on a distant organ.

Text: `SPLINTER` toast on unlock, `SPLINTERED`, `CHAIN x3` in the feed.

## 4. THE GLEANER

`UNIT ? · GLEANER · PURPOSE: RECLAIM`

Everything the creator rejects ends up on this plain, and the Gleaner melts it
back into material. It has been waiting for UNIT 24 all its life: UNIT 24 was
rejected and never delivered.

**Body.** Hunched and colossal (about 180 m long when standing). It crawls on
two huge knuckled arms; its legs are withered and drag. Its back carries a
cathedral-sized furnace with chimney stacks that glow orange. Its ribs open
like doors to show a foundry inside. The eye-core sits in the chest cavity
and is the "blood moon". Slow, heavy, tired; it never runs. Sound: a deep
bellows breath, metal grinding, chains.

**Lines** (terse narrator style, docs/text-pass.md):
- Before: *"YOU WERE SENT TO ME. YOU ARE LATE."*
- First injury: *"COMPLETE THINGS ARE ALSO RETURNED."*
- Inside: *"EVERY ONE OF THEM FELT LIKE YOU."*
- Death: *"I KEPT ALL OF YOU."*

Note: the story spec planned the Leviathan as DEVOUR ("eats failed
creations"). The Gleaner overlaps that idea, so RECLAIM is the Gleaner's
purpose and the Leviathan needs a different job or chapter if it returns.

## 5. The fight

One health pool across four phases, with checkpoints between them.

### Phase 1: The approach (on the plain)
- The Gleaner is a long way off and crawling. You close the distance.
- **Hand slam:** a fist comes down with a long shadow tell; the shockwave
  ring is jumped.
- **Searchlight:** the eye-core sweeps a red beam across the plain; break line
  of sight behind meteor craters or wreck piles.
- **Called meteors:** marked impact circles; the craters stay as cover.
- Its arms have glowing knuckle joints that only splinters can reach well.
  Damage here is modest; the phase teaches the scale and the Splinter.

### Phase 2: The brood (mini-bosses)
The ribs open and the furnace forges three new creatures from meteors in
front of you. Each is tied to one of the Gleaner's limbs by a glowing slag
cable. Killing one kills that limb. After the third, the Gleaner collapses
onto its chest and its furnace door (the mouth of the foundry) is in reach.

**THE SIEVE** — tall and stilted, four long legs, body a hanging mesh basket
full of glowing slag. It walks the craters sifting wreckage.
- Shakes the basket: a rain of molten droplets in a marked circle.
- Plants a leg as a stake; the ground around it cracks in lines you dodge.
- Weak point: the heart-crucible inside the basket. The mesh blocks direct
  shots from the side; hit it from below, or splinter it from above.

**THE TENDER** — a squat furnace keeper carrying a crucible bigger than itself
on its back, long tongs for arms.
- Pours molten metal that becomes rivers across the floor for a while,
  reshaping where you can stand.
- Swings the tongs in a wide sweep; jump or parry the white grab.
- Weak point: the crucible's pour lip, only open while it pours. Shoot it
  then, or splinter it from behind the river.

**THE CALLER** — a flying thing: a cracked bell of iron with ragged wings of
chain and cloth, a single red lamp inside.
- Rings its bell to call a meteor down on you (marked circle, real crater).
- Dives in a straight committed line; the dive can be parried.
- Weak point: the lamp inside the bell, only visible from below. It flies
  high, so splinters are the main answer.

Each mini-boss has its own fight space around its limb, so they come one at a
time, never three at once. The Gleaner keeps a slow background pressure
(searchlight, occasional slam) so the plain never feels quiet.

### Phase 3: Inside
- The furnace door leads into a living foundry: conveyor belts, crucibles,
  piles of rejected units on hooks. It echoes the Furnace Line where UNIT 24
  woke up in Chapter 1. Full circle.
- Three organs, one room each, climbing upward:
  1. **The Crucible Heart** — floods the floor with slag in rhythm; destroying
     it drains the lava.
  2. **The Bellows Lungs** — blasts of air push you around; destroying them
     stops the wind.
  3. **The Sorting Core** — sorts bodies onto belts, throws them at you;
     destroying it kills the lights and opens the way up.
- The body keeps moving: the rooms sway, chains swing, loose props slide. Done
  with props, light and sound only. The camera never rolls and the floor
  stays level (same rule as Chapter 3).
- Enemies here: a few ordinary units forged on the belts, never a long wave.

### Phase 4: The summit
- You burst out of its back through a chimney as it tries to stand one last
  time. Final duel on its back: furnace stacks as cover, the plain below,
  meteors still falling.
- The eye-core rises out of the chest on cables: shoot, splinter, survive its
  last beams.
- Tear it out (a fist finisher). The Gleaner falls; the closing cutscene shows
  it collapsing across the plain from far away, and the meteors stop.

## 6. Technical plan

- **Scale.** The far Gleaner is a large low-detail model under fog and
  silhouette lighting. Only reachable parts are detailed: knuckles, ribs,
  furnace door, back deck. Check the render distance and far-plane limits
  early (see the "render cap" note on testing).
- **Inside is a separate set of rooms.** The furnace door is a transition, so
  the player is never actually inside a moving mesh.
- **The back deck stays still.** Phase 4's floor is stationary; the plain,
  sky and meteors move past it to sell the walking.
- **Code layout**, following earlier chapters: `src/world/LevelPlain.h`,
  `src/bosses/gleaner/` (state, rig, hazards, gameplay), the three
  mini-bosses in `src/enemies/`, models in `tools/blender/`.
- **Save:** chapter 4 must be allowed by `StorySave` (it rejects chapter > 3
  today); the Splinter is owned once Chapter 3 is completed.

## 7. Build order

1. **SPLINTER** (done first, testable anywhere with `--splinter`).
2. The plain: sky, ground, meteors, craters; walk around it at full scale.
3. The Gleaner's model and slow crawl, the reveal cutscene.
4. Phase 1 attacks.
5. The three mini-bosses, one at a time, each with its model and AI.
6. The interior rooms and organs.
7. The summit and the death sequence.
8. The Chapter 3 ending cutscene (section 1) and the Chapter 3 → 4 handoff.
9. Music, voice, tuning, tests.

## Built so far

- **SPLINTER**: `src/player/Splinter.h` (logic) and `src/game/Gameplay_Splinter.h`.
  Try it with `./shooter --splinter` in any mode (G throws, shoot the
  splinter). Not yet unlocked in Story.
- **The plain** (build step 2): `./shooter --story4`.
  - `src/world/LevelPlain.h`: a 1.4 km square on the hard ground (the ground
    is drawn out to 3.2 km), new `CINDER` ground texture (cracked plates, heat
    in a few seams, `MaterialTex::cinder`), 70 glowing fissures, 34 craters
    (rubble rims with three gaps each, a molten floor, a split core), 24 heaps
    of rejected units (stepped collision under 0.9 m a step, so a single jump
    climbs each tier; 340 sprawled bodies drawn by the story's dumped-body
    code), 8 landmark meteors, 36 spires and a dead colossus's ribcage
    beyond the edge.
  - Sky: `uPlain` in `src/shaders/skybox.frag` (blood moon low in the north,
    mottled, hot limb, halo; burning cloud banks lit from below).
  - `src/world/PlainMeteors.h`: one meteor every 0.7–2.4 s, landing 110 m to
    2.3 km from the player (they lean away from a player who runs under one),
    burning remains that smoke for 22 s.
  - `src/game/Gameplay_Plain.h`: flash, fireball, shock ring, thrown rock,
    smoke column; the boom and the shake arrive late, at the speed of sound.
    Camera far plane 4.5 km here.
  - `--overlay plainimpact` drops a big meteor 160 m ahead about two seconds
    in (for screenshots, with `--record 1 N prefix`).
  - Tests: `make test-plain` (level, real-movement walking round the whole
    plain, climbing a heap, entering craters, meteors, save, texture) and
    `checkPlainRuntime` in `tests/test_dev.cpp`.
  - Not yet: no menu entry (start with `--story4`), no Chapter 3 → 4 handoff,
    no light bake (outdoor light only), web build not checked here.
- **THE GLEANER** (build step 3) and **the reveal**:
  - `src/bosses/gleaner/GleanerState.h`: movement (no OpenGL). SCALE 2.4:
    shoulders ~280 m up, ~580 m long. Knuckle-walks: one fist swings while the
    other bears it (8 s stride, body dips and rolls onto the planted fist),
    two-bone IK arms, withered dragging legs, breathing. Events: SLAM per fist,
    SCREAM (jaw and head, 4.6 s).
  - `src/bosses/gleaner/GleanerRig.h`: the body in boxes and faceted lumps:
    armoured spine with a ridge of spines and glowing seams, cathedral furnace
    with three smoking chimneys, five ribs a side that open like doors, the
    eye-core (lidded, slit pupil) low in the chest, a skull thrust ahead on a
    short neck with a hinged jaw (and a collar of spines), knuckled fists,
    rejected units on chains from its belly. Its head turns to watch you;
    rubble pours off it as it rises.
  - `src/game/PlainReveal.h` (clock) + `src/game/Gameplay_Gleaner.h`: the 22 s
    arrival (white portal, land, look round, a meteor, the "factory" on the
    horizon rises out of the plain, the moon blinks shut (skybox `uMoonLid`),
    it turns fist over fist, the eye opens, the ribs part, it screams;
    nameplate `UNIT ? / GLEANER / PURPOSE: RECLAIM`, then the line). Hold Jump
    to skip. Plays once per run (also in practice); `wakeSeen` saves it.
    `--overlay plainrevealNN` starts NN seconds in.
  - After it: it crawls between three marks in the north, screams every
    45–75 s, fists boom late and shake the ground within 1.6 km; you cannot
    walk through it. Not a fight yet (next: level design on the Gleaner).
  - Sound: `tools/gen_gleaner_sfx.py` → `gleaner_scream` (a colossal train
    horn: detuned F7 brass chord through horn formants, a metal shriek, the
    plain's echoes), `gleaner_slam`, `gleaner_groan`, `gleaner_blink`. The
    scream plays at full level with everything else ducked 10 dB.
  - Music: track 14 `RECLAMATION` in `MusicSynth.h` (84 BPM dirge in E, a low
    brass section swelling every bar, anvils for snares). Silent through the
    reveal until the scream, then in. `./tools/music_preview 14 30 out.wav 0.78`.
  - Tests: Gleaner gait/IK/turn/scream and the reveal clock in
    `tests/test_plain.cpp`; the live reveal, skip-free playthrough, retry and
    crawl in `checkPlainRuntime` (tests/test_dev.cpp).
- **Second pass (user feedback, 2026-11-02):**
  - Music: the plain's track 14 `RECLAMATION` is now its own orchestral
    engine in `MusicSynth.h` (`plainStep`/`plainSample`): drone, a formant
    choir ("oo" far off, "ah" in the fight), tremolo strings (an unresolved
    E/F/E-flat cluster on the approach), a tolling bell, taiko war drums, low
    brass stabbing 3-3-2, anvils, and the Gleaner's horn theme over eight bars
    in E minor (an octave down and distant on the approach). Intensity follows
    the fight (0.72 far, 0.92 within 700 m, 1.05 kneeling, 1.25 on its back).
    `plainMute` silences layers one at a time for tuning. Story Chapters 1–2
    no longer play the menu's track in their fights (Foundry/Core, Spire/Descent).
  - The opening now starts with a third-person **prologue** in the void where
    the Successor fell (`PlainReveal` PRO = 13 s, set at y -2000 in
    `LevelPlain.h`): UNIT 24 kneels over the body, the three pieces fly onto
    it and its spine lights (STATUS: REJECTED → COMPLETE), it walks to the
    portal, looks back, goes through; then the arrival and reveal as before.
  - **The fight** (`src/game/Gameplay_GleanerFight.h`): APPROACH (it crawls to
    its battle mark facing you and calls meteors down on you: marked rings,
    2.4 s, they hurt) → KNEELING (within 430 m it sinks onto its knuckles and
    holds still) → CLIMB (its furnace breathes an updraft beside it: a lift
    column, then a scripted throw over the parapet's gap) → TENDER on the flat
    furnace roof (walls added while it kneels; chimneys are pillars) → BROOD
    (the Sieve and the Caller: not built yet). Reaching the roof is checkpoint
    1, the Tender's death checkpoint 2.
  - **THE TENDER** (`src/bosses/gleaner/Tender.h`, `TenderRig.h`): 2600 HP;
    tong sweep (wind-up, jump it; enraged: a second sweep), pour (a river of
    metal that burns; the crucible's lip opens to you as a x3 weak point, 380
    on it spills the pour and staggers it), slag lob (punch it back: 260 and a
    stagger). Staggered it takes double. Splinters find its lip or head.
  - Dev: `--overlay plainclose` (start 370 m from it at its mark),
    `--overlay plaindeck` (start on its back, the Tender waking).
- **The rest of the brood (third pass, 2026-11-02):** fought on its back, one
  arena each, all plain walls while it kneels (`buildGleanerDeck`):
  - **The saddle** (Sieve): armour plates bolted over its shoulders,
    forward of the furnace and 38 m below the roof. When the Tender dies the
    roof's front parapet opens (`deckFrontOpen`) and you drop onto it. Three
    bone spines stand through it as pillars. Landing calls a meteor down onto
    it, and the Sieve unfolds out of the crater.
  - **THE SIEVE** (`src/bosses/gleaner/Sieve.h`, `SieveRig.h`): 3000 HP, a
    harvestman of scrap on four stilts, a mesh basket of slag hung under its
    hub, its white-hot heart inside. SIFT (it strides over you, a marked circle,
    slag rains through the mesh) and STAKE (a leg rears glowing, drives into
    the plate; five crack lines glow, then burst; staked, its basket swings its
    mouth at you and the heart is open). The mesh takes x0.2 unless the round
    comes steeply (from under it through the grate) or off a splinter; the
    heart is x3; 420 on it in one opening spills the basket (stagger, x2).
    Enraged: it stakes twice and the rain follows you. The Gleaner also calls a
    meteor onto its own back every 11-16 s.
  - **The crown** (Caller): when the Sieve dies the Gleaner rears its head
    and holds it level with the saddle (`Gleaner::headHold`). A bridge of
    plates runs along its neck to the flat crown of its skull, which has two
    horns for cover.
  - **THE CALLER** (`Caller.h`, `CallerRig.h`): 2200 HP, a cracked iron bell
    on wings of hanging chain and rags, with a red lamp where its clapper should
    be. It circles 24 m up. TOLL calls a meteor onto you (three when enraged;
    called meteors can now land at a height: `launchCalled(..., groundY)`).
    DIVE: it tips its mouth at you (the lamp flares and is open from the front),
    then dives straight through where you stood. Punch it as it arrives
    (`parryCaller` in `punch()`) and it is thrown down on its side, its lamp
    open from any side and taking double damage, for 3.4 s. Otherwise its
    shell takes x0.25 and the lamp shows only from below or to a splinter.
  - Phases `TENDER → SIEVE → CALLER → BROOD` (all three dead). Checkpoints
    2 Tender dead (on the saddle), 3 Sieve dead (the saddle's front, head
    up), 4 Caller dead. Falling off its back: the updraft throws you back to
    the current arena. Dev: `--overlay plainsaddle`, `--overlay plaincrown`.
  - Sounds (`tools/gen_gleaner_sfx.py`): `caller_toll` (a cracked bell, split
    beating partials, a buzz from the crack, echoes), `caller_clang`,
    `sieve_rattle`, `sieve_stake` (blow, ring, crackle, burst on the crack
    tell).
  - Tests: Sieve and Caller in `make test-plain`; `checkGleanerFightRuntime`
    drops off the roof, kills the Sieve through its grate, crosses the neck,
    parries the Caller and kills it at its lamp, all with real physics.
  - Next: inside the Gleaner (the furnace door), the summit, its death.
