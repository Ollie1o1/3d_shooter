# MORNINGSTAR — implementation plan

Spec: `docs/superpowers/specs/2026-10-16-morningstar-design.md`. Branch
`parry-whack` (Oliver: keep the parry there; MorningStar builds on it).

## Architecture

MORNINGSTAR is the same `Enemy` (type ABSOLUTE) in a second form
(`abForm = 1`), not a new type. His lethal hit in phase 3 is intercepted in
`Enemy::takeDamage`: he stays alive (the wave director never sees a kill, so
"ACT II COMPLETE" waits for the real death), goes untargetable for the
swallow, then comes back with `scale = MS_SCALE`, a fresh health bar
(`MS_HP`) and phases 4-6. Everything already built for him — events, the
hazard pool, parries, the sword, the feel table, the rig — is reused.

| Phase | HP | What's new |
|---|---|---|
| 4 MORNINGSTAR | 100-55 % | the seven moves; single slams |
| 5 RISING | 55-12 % | aftershocks on every slam, 20 % quicker, corona flares under strings |
| 6 TOTALITY | < 12 % | the corona collapses (a ring closing in: jump it), then the lights go out: his eyes, his white flashes and the floor tells are all you see |

New attacks (`AttackKind`): MJAB, MBACKHAND (white), MGRASP, MDROP (gold,
floor-marked), MSUNDER, MFISSURE, MFLARES, MCOLLAPSE (red, floor-marked).

## Stages (each: `make && make test`, screenshots, commit)

1. **The form switch and the swallow.** `abForm`, `abSwallow`, MS_* constants;
   takeDamage intercept; kneel → rise into the black → meteor re-entry
   (MDROP with a long tell); scale; checkpoint (`msCheckpoint`, retry starts
   at his form); boss bar name; death ("...WORTHY.", dawn) only in form 2.
   Sky: `uSwallow` in skybox.frag (the disc grows to fill the sky, then the
   ring of fire settles on the horizon); `swallowedOf(theme)`; floor seams
   and the corona wall drawn as boxes. Rig palette: blood orange, white eyes,
   white cracks; white tells flash the whole body.
2. **The moveset.** Strings + chooser for phases 4-6, the hazards in
   AbsoluteHazards.h (rings, fissures, grasp, drop crater, lanes, collapse,
   aftershocks), floor tells in gatherAbsoluteBoxes, rig poses for the new
   moves (reusing the boxer keys, bigger), feel table entries.
3. **Totality.** The collapse set piece; the dark theme; eyes only.
4. **Sound and music.** Track 11: faster than 166 BPM, harder kick and bass;
   routed while `abForm == 1`. New booms/winds where the old ones don't fit
   (reuse first, generate only what's missing).
5. **BossSim + tests.** Form 1 still 150-180 s solid; form 2 ≥ form 1's time
   (170-200 s target); survivability; cheese punished; checkpoint; the
   intercept never lets "ACT II COMPLETE" fire early. Web build compiles.
