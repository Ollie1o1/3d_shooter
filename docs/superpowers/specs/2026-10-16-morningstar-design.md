# "!!!!!" phase two — MORNINGSTAR — design (draft)

The Absolute's fight gets a second form, in the spirit of Promised Consort
Radahn's second phase: once his three phases are done, the eclipse swallows
the Zenith and he comes back as MORNINGSTAR, a 1.65x giant in blood orange
with white eyes. Much harder than the first form and at least as long.
The last light before the dawn: the fight ends at daybreak.

## Decisions (Oct 16 2026, with Oliver)

- **When:** after all three of the Absolute's phases (JUDGMENT, ASCENSION,
  ABSOLUTE + the last judgment). The first fight stays as it is.
- **Transition:** the eclipse swallows the arena (option C). Same Zenith
  geometry, re-themed; no new arena.
- **Look:** scaled ~1.65x (head ~4 m), blood-orange emissive body, blank
  white eyes, white light leaking through cracks.
- **Name:** MORNINGSTAR (boss bar and banner).
- **Tells:** parry moves flash his whole body white-hot; dodge/area moves are
  read from the floor (lines, circles, lanes, shadows) and the corona wall.
  The white / gold / red rules still hold, but gold and red show up on the
  floor, not on his body.
- **Difficulty / length:** Radahn phase 2: far harder, BossSim target at least
  the first form's (~169 s), aiming at 170-200 s.
- **Moveset:** all eight moves below.
- **Entrance:** meteor re-entry: he rises into the black and crashes down
  (the Eclipse drop as his entrance).
- **Checkpoint:** dying to MORNINGSTAR restarts at the start of his form, not
  the whole fight (the run's time can still count the whole fight).
- **Music:** a new track, faster than track 10's 166 BPM, with harder bass
  and drums.

## 1. The swallow (transition)

1. The Absolute's HP hits 0 in his phase 3. He goes to his knees (the start
   of his death pose) — no "...WORTHY.", no dawn.
2. The skybox eclipse grows until it fills the sky: everything goes black
   but the white ring of fire, which settles onto the horizon.
3. The floor turns to black glass with thin orange seams; the ring becomes
   the arena wall (the edge hurts — the existing edge stays the boundary).
4. He stands; white light breaks out of the cracks; he swells to 1.65x in
   one burst and comes out blood orange with white eyes.
5. A new HP bar fills: MORNINGSTAR. His real death ("...WORTHY.", dawn)
   moves to the end of this form.

Entrance (decided): he doesn't stand up in place. He goes up into the black
and comes down as a meteor where the player is: an Eclipse drop with a
long first tell.

## 2. Moveset

| Move | Tell | Answer |
|---|---|---|
| **Sunder**: fist into the floor, a shockwave ring spreads out | ring line glows on the floor | jump it |
| **Fissure**: both fists overhead, a crack of light rips along the floor at you | floor line | sidestep |
| **Colossus backhand**: a huge sweep at chest height (the player's parry turned on them) | white flash | parry (big stagger) |
| **Solar string**: jab, jab, backhand, fast | white flash per hit | parry the chain |
| **Grasp**: reaches down to grab; caught = lifted and slammed | floor circle where the hand lands | dash out |
| **Eclipse drop**: leaps into the black, crashes where you were | a shadow grows on the floor | keep moving |
| **Corona flares**: the ring-of-fire wall shoots flame lanes across the floor | lanes light on the floor | stand between them |
| **Totality** (finale, ~10 %): all light goes out but his eyes and the floor tells | eye glints + sound | survive, parry by eye and ear |

What makes it Radahn-hard, beyond the numbers:
- **Aftershocks:** slams (Sunder, Fissure, Eclipse drop) leave light that
  erupts again ~0.6 s later, so dodging early or rushing in gets punished.
- **Few openings:** strings chain into each other; he only stops when parried
  or after the big moves (Grasp whiff, Eclipse drop landing).
- **Escalation:** stages inside the form (open question: where the cuts are),
  each adding aftershocks / speed, ending in Totality.

## 3. As built (Oct 16 2026)

- Stages: MORNINGSTAR 100-55 %, RISING 55-12 % (aftershocks on every slam,
  18 % quicker, the ring's flares under every third string), TOTALITY < 12 %
  (opens with the COLLAPSE: the ring of fire closing in, jump it; then the
  light goes out of him and the arena, 28 % quicker).
- 11000 HP (difficulty-scaled like his first bar). BossSim SOLID: first form
  174 s, MORNINGSTAR 199 s, lowest health 28 (50 against the first form).
- Parrying his backhand breaks him outright (3 s, double damage).
- Track 11 MORNINGSTAR: 178 BPM, techno voicing 2.
- Dev: `--overlay morning|morning5|morning6`, `swallowNN`, `abposeNms` /
  `abposeNms6`, poses 24-30 for his moves.
- Not play-tested; tuned by the sim.
