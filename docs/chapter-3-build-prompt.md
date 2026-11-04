# Fresh-context handoff: OVERDRIVE Chapter 3

**Implementation now exists. Read [the implementation notes](chapter-3-implementation.md) first.**
The original build prompt below preserves the agreed scope; do not restart completed work.

Copy the prompt below into a new conversation in this workspace. The detailed
brief is saved alongside it, so this handoff does not depend on conversation memory.

---

We are working on OVERDRIVE in `/Users/ollie/Projects/overdrive/3d_shooter`.
Read `docs/chapter-3-design.md` in full first. It consolidates our design discussion
and replaces the older transit/Surveyor brainstorm. Inspect current git state,
AGENTS.md instructions if present, and the actual game before changing anything.

The next task is to implement the complete Chapter 3 described in that brief.
Proceed through its build order, validating each meaningful milestone and
continuing until the chapter is complete. The first vertical slice is a development
milestone, not permission to stop after a prototype. Preserve unrelated changes.

User-established requirements:

- A laboratory developing/testing newer flesh-like robot bodies. Exactly ten main
  rooms, with the final boss in room ten. The largest, most experimental Story
  chapter so far, progressively stranger as the player descends.
- Spacious, often brightly lit hospital/testing architecture, wide traversal and
  combat areas. Strong environmental details and subtle implications. Early
  anomalies grow into floating bodies and genuine dimensional crossings.
- Late rooms include a huge glass roof/windowed hall looking onto a nebula, then
  a bright hall overlooking a deep ocean trench. These are real incompatible
  environments. Preserve the ordinary hospital connections between them.
- A garden that should not exist, a broad straight crossing over a seemingly
  bottomless pit, recurring props, impossible observations and memorable scale.
  ULTRAKILL is inspiration for surreal juxtaposition and energetic combat; create
  original spaces, dialogue, models, moves and music.
- A short entry cutscene that strongly implies this is an unusual chapter. It
  must skip cleanly and not replay on normal retries/Continue.
- The final boss is a perfect test subject: near-human flesh body, familiar robot
  face, extremely fast/agile/strong, confident original one-liners, then a loss of
  composure under pressure. Match the intensity of Absolute/Morningstar with an
  independent combat identity and new attacks.
- Original fast choir-led dramatic boss music, with the emotional reference of
  Gabriel's Apostate fight. Audible choir, memorable theme, organ/harmonic weight,
  fast percussion and a distinct arrangement from existing hard-techno tracks.
- Boss victory lets the player physically enter fractured dimensions. Build a
  short playable threshold ending; the later full fractured-world chapter is
  outside Chapter 3's scope.

Working defaults chosen in the brief, adjustable through implementation and play:
chapter THE LIVING WARD; boss THE SUCCESSOR; one health bar with three phases at
65%/30%; approximately 176 BPM; competent Standard kill-time target 150–190 s.
Names, HP, timings and exact dialogue are recommendations, not extra approvals
already supplied by the user. Use the defaults and tune rather than restarting
brainstorming over routine details.

Ten-room sequence:
1 Admissions; 2 Recovery Ward; 3 Motor Assessment; 4 Surgical Observation;
5 The Conservatory (quiet garden); 6 Load Testing (open pit crossing);
7 Long-Term Care (suspension atrium); 8 Celestial Observation (nebula);
9 Hadal Research (trench); 10 Final Assessment (boss).

Opening cinematic: a routine motor test behind observation glass; the subject
rises above the floor while its shadow continues along a wall; the instruments
report normal contact; the window reveals an impossible exterior while reflecting
an ordinary ward. A sustained readable anomaly, no strobe montage or lore speech.
"Proceed to observation." Control returns facing the descent.

Boss identity: a body engineered to cross spatial boundaries. The brief details
Incision, Vector Run, Suture, Transposition, Recall, Perfusion and Double Exposure.
Use committed trajectories, physical tells, cuttable filaments, announced crossings
and delayed location marks. Preserve a stable player floor/aim and readable
white/gold/red plus sound/shape rules. Parries interrupt pending follow-ups and
supply bounded healing. Guns, grapple and movement are sufficient; no Story sword
unlock. No ordinary add waves as a mandatory healing source. No giant Morningstar
reskin, copied halo, reused jab strings, silent telefragging or invulnerable finale.

Key dialogue recommendations:
- "Unit Twenty-Four. Still running."
- "Come closer. I want a proper comparison."
- "That was not in your design."
- "There. Something they missed."
- "No more corrections."
- "I was supposed to leave."
Full trigger/delivery requirements are in the brief. Never make dialogue the only
attack cue. The existing procedural voice system supplies wordless choir sounds,
not intelligible spoken acting. Produce original speech if tools/assets allow;
otherwise complete subtitles and distinctive vocal cues, explicitly identify the
remaining speech asset work, and continue the rest of implementation.

The four enemy families are Patient, Grafted, Suspended and Phase Subject. Give
them original silhouettes/models/animation and genuinely distinct behavior. Clearly
distinguish harmless floating bodies from active suspended enemies. Introduce new
threats individually and combine them in authored encounters with recovery beats.

Existing state to preserve:
- Chapters 1/2, all other modes, the four guns and run upgrades.
- F3 Combat Practice, F4 Field Guide, alternative weapon mods, Story Continue,
  the relay isolation and turbine winch improvements.
- `StorySave.h` currently accepts only chapters 1/2: migrate/version it safely.
  Include cutscene-seen, chapter-3 checkpoint/gear/discoveries/objectives and boss
  defeat. Old saves must remain readable; practice/capture cannot overwrite saves.
- Boss select currently has nine entries. Append this boss and preserve IDs.
  The shared web leaderboard only supports the original six boss modes; new bosses
  currently use local records. Do not invent unsupported API submissions.
- Use real engine code as reference: `GameplayState.h`, `Gameplay_Story.h`,
  `Gameplay_Wasteland.h`, `LevelStory*.h`, `LevelWasteland.h`, the independent
  boss event/state/hazard split, Blender asset tools and headless regression tests.
- Extend chapter selection, Chapter 2 handoff, dev/CLI/web starts, light bake,
  exports and music routing. No seamless-portal or arbitrary-gravity rewrite is
  required to achieve the agreed spaces.

Build order: complete traversable route/save integration; representative hospital
and nebula/trench slice; boss body/locomotion and three core attacks early; finish
new enemy families and encounters; complete late routing rooms; full boss, score,
dialogue and ending; detail/readability/audio passes; native/web validation.
Review the whole brief's acceptance checks. Validate collision/traversal, checkpoint
and cutscene paths, real combat input, boss timing/counters/reset, old-save migration,
visuals in motion, low detail, accessibility, browser behavior and performance.
Listen to the actual music. Do not claim a new track or readable fight from table
values alone, or human playtesting from scripted simulation alone.

Current project/release context at planning time: game branch `story-cradle`, last
feature release `135d380`; adjacent website `/Users/ollie/Projects/overdrive/Personal-Website`,
last release `4e793a5`. These may have advanced: inspect rather than resetting.
The site uses `scripts/sync-overdrive.sh`, `npm run verify` and a push to its main
branch for production. Implement and verify locally; publish the eventual Chapter 3
only when that release is requested in the new conversation. Do not alter the
existing published game just to display the chapter's plan.

Keep the user informed. Carry out routine implementation decisions autonomously;
ask only when missing information materially blocks progress. Do not replace the
agreed lab chapter with the superseded transit/Surveyor idea or generic dark tunnels.
