# Story boss combat overhaul

Implemented for the Cradle's Sorter and the Dead Span's Ballast. Both keep their
existing health totals. The change addresses downtime, predictable rotation and
whole-body vulnerability rather than adding health.

## The Sorter

- The scanner runs alongside pursuit. The carriage closes distance during
  preparation, between attacks and late in recovery.
- A gold gantry lane predicts movement, locks before release and carries a real
  claw into that route. It cannot turn after release. Braking exposes the blue
  strip, then a separately warned white claw can follow.
- Three physical press heads warn together and fall 0.38 seconds apart across
  the observed movement route. Their targets commit before the first stroke.
  Each head reaches its actual ground or platform before damaging it, holds
  and retracts. A parry clears pending heads and follow-ups.
- The ejector throws three or five physical scrap bodies, with a delayed second
  volley in later phases. It can aim upward at an airborne player. Scrap can
  be shot or parried, and impacts break authored inspection shields.
- Later phases purge one or two fixed furnace vents. Visible flames reach the
  damaging height; unused lanes and the raised routes remain available.
- Four corner belt panels fail progressively, with 2.2 / 3.3 / 4.4 / 5.5-second
  warnings and overhead strikes. The connecting crosswalk and rails remain.

## Ballast

- Pursuit continues between tools and during appropriate preparations. The
  grader steers around real walls, predicts a movement route, then locks and
  drives at 31–36 m/s. Jump its low blade or dash across the lane.
- Later phases brake, expose the rear turbine and prepare a second pass with
  a fresh tell. The final phase can finish the string with a cable weight.
- A white harpoon commits before firing. A caught player retains steering,
  can dash against the brief pull, parry the head or shoot the visible cable.
  Cutting it opens the drive. A miss can winch Ballast toward the committed
  point; the haul itself deals no contact damage.
- The planted blade swings through a visible gold arc. The chassis rotates
  with the sweep. Jumping clears its low damage volume; the miss opens the drive.
- A road tear sends three narrow fans of visible asphalt slabs along committed
  strips. Jump them or cross the gaps. Slabs stop at real cover and walls.
- Aimed turbine scrap reaches airborne players. Crosswind still respects real
  windbreaks; the final phase can reverse its direction.

## Damage, destruction and settings

Body shots remain effective: 90% damage against the closed Sorter and 85%
against closed Ballast, full damage during openings. The visible front blue
strip / rear turbine takes double damage when open. Parrying applies the
existing stagger bonus to the body; precision does not multiply it again.
Story bosses now shed two 40-health potions, at one-third and two-thirds damage,
rather than four. Ordinary enemy drop settings are unchanged by this pass.

Charges, scrap impacts, press strikes and road fronts can shatter only the
marked lightweight shields / windbreaks. Collision and the spatial grid update
with the break, including ray and shelter behavior. An active gust preserves
its final authored shelter. Structural walls do not break. Debris is bounded;
retry restores cover, floor panels and hazards.

Mandatory machinery, warning lanes, lock sounds, flames and asphalt remain
visible with Ambient Detail OFF. Optional debris and dust scale with settings.
No external audio files are added: six new machinery tell recipes bring the
synthesized voice bank to 294 named cues.

## Validation and limits

`make test-story-bosses` uses the real Player acceleration, gravity, collision,
weapon ammo / reloads, hitscan occlusion, projectiles, immunity and healing. Its
ideal-aim controller compares constant circling / backpedalling with responses
to committed tools, including dashes, jumps, parries and aimed weak-point shots.
It grants potion rewards immediately, without requiring collection. Reactive
wins take about 28–56 seconds with unupgraded Revolver / Longshot in this fixture;
passive movement is substantially more dangerous. Chapter 1 Longshot is an
extra pressure test, beyond its normal equipment progression.

Bounded tests also cover commitment after a reversal, cancellation on parry,
elevated targeting, real cover removal, shelter preservation, jumpable asphalt,
and front / rear precision damage. `make test-dev` exercises all 17 menu
checkpoint launches, real cable shots, progressive floor failure and retry.
`make test-web-story` renders both models in actual WebAssembly with 32 KiB of
stack. Native captures inspect required attack visuals with ambient detail off.

These are automated simulations and render checks, not a human playtest. The
weapon fixture does not model continuous grapple flight or final-phase floor
loss; separate arena and movement tests cover those systems. Player feedback
will still be useful for difficulty and readability tuning.
