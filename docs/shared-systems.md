# Shared performance and gameplay improvements

This follows the [first FPS pass](fps-optimizations.md) and applies to shared
systems across maps and modes. Level content, effects, render resolution, enemy
counts, weapon damage and the 60 Hz simulation remain intact.

## Input and combat behavior

- Jump, dash and parry presses now survive a press/release that happens between
  physics ticks. Keyboard and mouse bindings are respected, as are controller
  A (jump), RB (dash) and Y (parry). Retained presses expire after 100 ms and are
  consumed once. Pause, restart, menus and Story control locks clear them.
- Selecting another gun during the 150 ms switch changes its destination
  without restarting the readiness timer. Wheel/D-pad cycling advances from
  the requested gun, so quick selections are retained. Story ownership checks
  still apply. Retargeting cancels any trigger belonging to the old destination.
- A trigger tapped during a switch survives that switch and fires the selected
  gun once it is ready. Reloads, magazines and fire-rate limits still apply.
- Critical hit/kill confirmations finish their display rather than being
  overwritten by a later ordinary or blocked contact. Ordinary hits do not
  prolong the critical marker. Contacts that deal no damage do not grant hit
  healing/style or display a successful damage marker.

## Collision consistency

Projectiles test their complete travel segment and resolve the nearest contact.
Thin walls and moving platforms block shots, fast rounds cannot skip through
targets, and a farther enemy appearing earlier in a vector cannot take a hit
through a nearer one. A siege shell retains its arming delay and ignores its
identified owner and bosses when resolving friendly fire.

Exceptional player moves that could cross an obstacle and the player's entire
body in one tick use a swept box, sliding the remaining movement along contact
faces. Normal movement keeps its established contact response and authored
jump-pad arcs. Tests cover fast horizontal movement, thin floors/ceilings,
moving cover and preservation of momentum parallel to a wall.

Correctly blocking shots exposed an existing Sorter perch problem: its presses
sampled support only below a hard-coded 10 m limit, placing warnings below tall
columns. They now target the actual supporting surface beneath the player.
The original boss simulation passes all three phases and its anti-camping
checks with cover remaining solid; attack timing/damage and map geometry are
unchanged.

## Rendering and repeated work

Weapons, sword, arms, parry and fanning parts use an instanced vertex path with
the existing world fragment shader. Consecutive parts of the same shape share
a draw call, preserving draw order at depth ties. Geometry is gathered before
upload, including gun/hand combinations, rather than updating the instance
buffer between their draws. The developer hand inspection keeps its separate
depth clear. The unbatched reference remains available with `--skip vmbatch`.

The render tests compare every pixel for eight poses. All match exactly at the
test resolution. Examples: 385 revolver parts use one draw; 105 shotgun parts
use one draw; an aimed Lancer uses 54 draws for 98 parts; sword guard uses 145
draws for 282 parts. Geometry and articulated fingers are retained.

Shared vertex shaders invert the affine transform's 3×3 linear portion for
normals, preserving non-uniform/negative scale and shear without inverting a
full 4×4 matrix. Fragment shaders skip point-light calculations when that light
has zero color; active lights keep the same shading.

Spatial-grid queries return each wall once, including walls spanning many
cells, without allocating a visited set or sorting. This benefits player,
enemy and projectile queries. Projectile target bounds are cached once per tick
when needed and rejected by swept bounds before exact tests. Projectile hit
results, spawn requests and weapon cue buffers retain their allocations.
Attack-token sorting uses enemy index to preserve equal-score order without
`stable_sort`'s per-tick temporary allocation.

## Measurements

Native macOS on Apple M2 at 1280×720, 4× MSAA, HIGH lighting and ambient detail
2, with the same quality settings before/after. The baseline is the completed
first FPS pass, so these are additional gains. Runs alternate before/after,
advance 1/30 s of simulation per frame, render uncapped, and exclude the first
10% as warm-up. Values below are medians of three 900-frame runs per version.

| Scene | Before average | After average | Average FPS gain | Before / after 1% low |
|---|---:|---:|---:|---:|
| Core wave 2, active combat | 3.05 ms / 328 FPS | 2.72 ms / 368 FPS | 12% | 201 / 210 FPS |
| Living Ward section 7 | 4.39 ms / 228 FPS | 3.78 ms / 264 FPS | 16% | 163 / 181 FPS |
| Cold Store section 8 | 2.94 ms / 341 FPS | 2.77 ms / 360 FPS | 6% | 197 / 184 FPS |

View-model CPU submission drops from median 0.62–0.74 ms to 0.02–0.03 ms across
these scenes. End-to-end FPS remains dependent on GPU execution and presentation.

The Cold Store's average improves, but its 1% lows regress. A separate set of
three 3,000-frame before/after runs confirms that tradeoff: median averages
347 → 365 FPS, 1% lows 190 → 179 FPS. Frame breakdowns put the slow frames
primarily in GPU waiting/presentation, not weapon submission. A tested buffer
orphaning alternative worsened frame times and was discarded. This pass does
not claim a consistent frame-pacing improvement in the Cold Store.

Raw per-frame CSVs and logs are under the ignored `build/shared-*.csv` and
`build/shared-*.log`; summaries are `build/shared-benchmarks.json` and
`build/shared-cold-long.json`. Reproduction commands are the same as in the
[first-pass notes](fps-optimizations.md#measurements), run three times for each
version with identical settings. The Cold Store still reports its pre-existing
missing `story5.light` bake in both versions. Browser FPS was not measured.

## Validation

- `make`, `make test`, `make test-render` and `make test-feel-runtime` pass.
- `make web` and `python3 tests/test_web_release.py` pass. The browser connector
  has no available session, so live browser interaction was not verified.
- Native pixel comparisons cover all four guns, aimed fire, parry, sword guard,
  a sword swing and free arms, with explicit GL error and visible-output checks.
- Runtime tests exercise between-tick inputs, expiration, pause cancellation,
  rapid weapon selection, buffered fire, critical-marker priority and the real
  game renderer. Headless tests also cover projectile travel order, thin cover,
  moving obstacles, player collision, all jump pads, bosses and Story routes.

Set `OVERDRIVE_TEST_NO_SCREENSHOTS=1` when running `make test-feel-runtime` to
perform its assertions without saving optional BMP captures. Pixel comparison
tests in `make test-render` still run normally. Native runtime captures made in
this session were archived losslessly as `build/feel-pass/*.bmp.gz` to conserve
disk space; source assets were retained.
