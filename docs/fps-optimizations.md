# FPS optimization pass — 2026-11-06

These measurements describe the first pass. See the subsequent
[shared-systems improvements](shared-systems.md) for weapon/arm batching,
shared shader/collision work and gameplay input/feedback changes.

This pass preserves level content, enemy rigs, animations, effects, resolution,
MSAA and simulation frequency. It changes how existing geometry reaches the GPU.

## Changes

**Upload Chapter 3 glass and atmospheric geometry before drawing.** Previously,
each glass pane and light shaft called `glBufferSubData` immediately before its
draw. Even with separate offsets, those writes serialized rendering on the tested
macOS driver. Glass and atmosphere now each collect the same quads, orphan the
shared streaming buffer, and upload their vertices once before issuing draws.
Per-quad uniforms, draw order, blending and the existing capacity remain intact.

**Cull animated parts outside the camera frustum.** `BoxRenderer::drawVisible`
uses the existing static-world clip-plane test, now shared in `core/Frustum.h`.
It tests transformed bounds of actual mesh vertices (or the unit cube), including
rotation, negative/non-uniform scale and shear, with a small conservative margin.
Only fully offscreen parts are omitted from upload, mesh grouping and drawing.
Simulation, rig generation, contact shadows, attack tells and HUD markers still
run normally. No distance cutoff, reduced detail or occlusion assumptions were
added. `--skip cull` disables both world and animated-part culling for comparison.

## Measurements

Native macOS, Apple M2, 1280×720, 4× MSAA, lighting HIGH, ambient detail 2,
quality 3 with the resolution overridden. Both versions use the same settings.
Each run renders 900 frames uncapped, excludes the first 10% from the report,
and advances the simulation by 1/30 s per rendered frame. These are individual
local runs, not a hardware-independent performance guarantee.

| Scene | Before avg / FPS | After avg / FPS | Before / after 1% low FPS |
|---|---:|---:|---:|
| Living Ward, section 7 | 7.03 ms / 142 | 4.49 ms / 222 | 111 / 164 |
| Cold Store, section 8 | 3.81 ms / 263 | 3.69 ms / 271 | 160 / 158 |
| Core, wave 2, active combat | 3.45 ms / 290 | 3.43 ms / 291 | 176 / 178 |

The Living Ward improves about 56% in average FPS: CPU submission in the FX
zone falls from 5.04 ms to 0.05 ms. The Cold Store's box submission falls from
0.38 ms to 0.14 ms; overall improvement is smaller because GPU work and
presentation dominate there. The Core is essentially unchanged. Cold Store
1% lows remain within run-to-run variation rather than showing a clear gain.
The existing missing `story5.light` bake was reported by both versions; this
pass does not change lighting assets.

Reproduce with the settings above (run the same commands against each binary):

```sh
make
./shooter --bench 900 --story3 --arena 7 --god --res 720 --aa 4 --lighting 2 --ambient 2 --record 0 0 unused --frametimes build/lab.csv
./shooter --bench 900 --story5 --arena 8 --god --res 720 --aa 4 --lighting 2 --ambient 2 --record 0 0 unused --frametimes build/cold.csv
./shooter --bench 900 --arena 4 --wave 2 --god --res 720 --aa 4 --lighting 2 --ambient 2 --cam 0 1.7 -208 -90 -2 --record 0 0 unused --frametimes build/core.csv
```

The existing `--record 0 0 unused` option enables fixed 1/30 s simulation steps
without writing recording frames; `--bench` controls the run length. Avoid GPU
timer queries for these comparisons: query readback itself stalls on macOS.
Raw measurements for this session are in the ignored `build/fps-*.log` and
`build/fps-*.csv` files.

## Validation

- `make`: native build passes without warnings.
- `make test`: the full headless gameplay suite passes.
- `make test-render`: native OpenGL tests pass, including 18,001/40,001-instance
  scenes that retain the final visible object while submitting only one part.
  Four camera orientations compare every pixel to the unculled reference,
  including offset mesh geometry, thin/large parts crossing clip planes,
  mirrored scale and shear.
- `make test-feel-runtime`: native game/input/HUD integration checks pass.
- Emscripten syntax check of `src/main.cpp` with the SDL2/SDL2_mixer ports passes.
  Browser runtime performance was not measured.
- Fixed-timestep Living Ward screenshots were inspected before and after. With
  ambient particles disabled for the comparison, 12 of 921,600 pixels differ,
  all on view-model edges; the glass, lighting and world match. Normal benchmark
  runs retain ambient particles and effects.
