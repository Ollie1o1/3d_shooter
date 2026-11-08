# Encounter and replay pass

The first implementation covers each of the agreed work areas with a playable
feature or a development tool. It starts with one room so later level work can
use the same measurements and tools.

1. Motor Assessment: improve cover and movement choices; verify routes, spawns,
   collision, lighting and the actual rendered room.
2. Add an optional, clearly announced environmental attack to that encounter.
3. Add boss phase practice and a boss rush using the existing fights.
4. Add a small encounter editor for wave composition and spawn placement.
5. Improve local playtest reports with parry and retry data, separating tools
   and captures from ordinary play.
6. Expose shareable Endless wave seeds, with explicit challenge identification.
7. Keep new encounter state separate from GameplayState and run the browser
   smoke test in CI.
8. Add an optional discovery reachable through the revised room's upper routes.

Each feature must be reachable through the game, document its controls, and
preserve existing saves and ordinary records. Automated checks cover mechanics;
fresh-player observation and subjective balance remain human playtest work.

## Motor Assessment

Chapter 3, room 3 (`./shooter --story3 --arena 3`), keeps its existing fourteen
enemies, assessment lock and upper circuits. Six climbable diagnostic banks and
five smaller screens split long exposed crossings. The current audit measures
26.2% cover (previously 18%), 24.8 m mean sightlines (previously 28.1 m), seven
standing layers and 0.79 light parts per 100 m². Its lighting has been rebaked.

The optional west-gallery console releases the suspended calibration load over
the amber square. There is a 1.3 s warning, then a 0.6 s descent. Impact deals
500 armour-piercing damage to regular enemies beneath it and 55 damage to the
player. A surviving player is pushed to the nearest clear, supported side.
The load remains solid cover until retry; it can be released once per attempt.
It is optional and never replaces the assessment lock.

A service record on the northwest bank roof rewards using the new upper
surface. Its terminal ID is appended after all existing IDs, preserving saved
discovery bitmasks. Normal Story checkpoint saves remember that it was read.

## Boss replay

On the BOSS page, select the Sovereign, Sorter, Ballast or Successor and press
**P**, controller **X**, or the phase button. This cycles whole fight / phase
1 / phase 2 / phase 3. Enter or controller A launches it. Practice starts at
the chosen phase's health threshold and can progress naturally into later
phases. Backspace immediately retries that same start. Practice has normal
combat damage and cannot write ordinary records, leaderboards or Story saves.

**R**, controller **Y**, or the rush button starts six fights: Warden,
Sovereign, Penitent, Leviathan, Absolute, MORNINGSTAR. One life; retry or restart
begins again at the Warden. Upgrades carry, later bosses grant the difference
in their normal upgrade-point allowances, magazines refill, and moving to the
next boss restores 25 health, capped at 100. Enter/controller A on a victory
card advances. The final card totals the six fight clocks, excluding loading
and Armory time. Best total times are stored separately by difficulty in
`bossrush.v1.cfg`; individual boss records and online boards are untouched.
Difficulty is fixed during the rush.

Developer starts: `./shooter --boss 2 --boss-phase 3`, `./shooter --boss-rush`.
Browser equivalents: `?boss=2&phase=3`, `?bossrush`.

## Encounter workshop

Desktop **F1 → ENCOUNTER WORKSHOP** opens the editor. Enable editing to mark
the run as practice before changing anything. Pick a room, wave and entry;
change enemy type/count, add/remove entries, or edit explicit spawn coordinates.
Use **MOVE SPAWN TO PLAYER** and the world-space bounds preview for placement.
**APPLY AND RESTART ROOM** validates the draft and restarts the chosen room.
Close F1 to resume. The window can be moved/resized to inspect the room.

Validation checks supported spawn floors, complete body clearance, room bounds,
overlapping bodies, enemy types and bounded counts. Boss/scripted encounters,
training and generated Endless waves are read-only. Authored wave counts and
objectives are retained. The editor changes encounter data, not geometry.

**SAVE PATCH** writes `encounter.v1.cfg`; **LOAD PATCH** checks the map/room,
format and spawns before replacing a draft. Patches require explicit loading
and applying. **RESTORE AUTHORED** restores the selected room's original draft.
These tools cannot overwrite Story progress or normal records. The optional
web build flag `WEB_BALANCE_PANEL` also includes the workshop; ordinary browser
releases retain the existing smaller build without ImGui.

## Endless codes

ENDLESS opens a page offering a random run or a challenge code. Select the code
field to type or paste (Ctrl+V / Command+V). Codes look like `OD1-12345-2`: wave
generator version, unsigned 32-bit seed, difficulty 0–3. Pause any Endless run
to see its code; **F8** copies it. Launch directly with
`./shooter --challenge OD1-12345-2` or `?challenge=OD1-12345-2`.

Codes reproduce wave composition, objective sequence and difficulty. They do
not replay AI decisions, spawn positions or player inputs. Difficulty is fixed
for an active challenge; retries keep its seed. Seeded challenges are unranked
and cannot enter the ordinary Endless leaderboard. Random runs keep the normal
ranking rules. Future incompatible generator changes need a new code version.

## Playtest feedback and infrastructure

Ordinary desktop sessions write `runs.jsonl`; practice, god-mode sessions and
captures write `runs.dev.jsonl`. Logs include accepted punch/guard presses,
successful parry contacts, explicit retries, and exploration even when no gun
was fired. A single defensive input can produce multiple contacts, so these
are counters rather than a claimed parry-success percentage.

`python3 tools/runlog.py` reports normal play, including retries per Story
section. `python3 tools/runlog.py runs.dev.jsonl --include-dev` inspects tools
and captures. Older logs remain supported. The browser retains its existing
behaviour of not writing playtest logs.

`EncounterTrap`, `EncounterDraft`, `EndlessChallenge` and `BossRush` keep the new
mechanics independent of rendering and menus. App owns rush state across
deferred level transitions. CI now installs a pinned Playwright/Chromium and
runs the existing browser smoke test against the built release, including the
new challenge/practice/rush starts and Motor Assessment. Captures are retained
as CI artifacts, including on failure.

## Verification

```sh
make
make test
make test-next-runtime
make test-dev
make test-render test-feel-runtime
source ~/emsdk/emsdk_env.sh
make web test-web-story test-web-lighting
python3 tests/test_web_release.py
node tests/test_web_pointer_lock.mjs
OVERDRIVE_PLAYWRIGHT="$PWD/build/browser-smoke/node_modules/playwright/index.mjs" \
  node tests/test_web_smoke.mjs http://127.0.0.1:8765/ build/next-pass/web
```

The browser command needs a local server serving `web/dist`. `make test-next`
(also in `make test`) checks the measured room standard, real boss AI phase
starts, edited waves through WaveDirector, safe patch import, challenge-code
validation, fifty generated waves, trap timing, rush progression/persistence
and backward-compatible log reports. The native runtime test exercises actual
menu/input/App transitions, collision, the drop control and impact, roof access,
the optional record, save/record isolation and GL output. Local screenshots
are under `build/next-pass/`.

The expanded browser run also exposed a pointer-lock lifecycle bug in the web
shell: if both raw-input and plain requests failed, the wrapper returned a
rejected fallback promise to a caller that did not consume it. The shell now
settles both failures and avoids reacquiring the cursor after cancellation or
document detachment. A Node regression test executes the actual shipped wrapper
for denied locks, synchronous errors, navigation, cancellation and success.

Local release checks completed on 2026-11-08: the full headless suite, native
App/input/Story integration, native rendering and combat-feel checks, WebAssembly
Story and lighting checks, release-manifest checks, and the complete Chromium
smoke run. The website's 35 unit tests, Astro type check and production content
verification also completed. Browser captures use a fresh profile at 720p
without MSAA to keep software WebGL practical; native renderer checks retain
their existing pixel comparisons. The deployed artifact is verified separately
against `web/dist/build-info.json`.

## Next iteration

Use fresh-player observation to assess the revised room before carrying the
layout changes into Surgical Observation and Long-Term Care, then Celestial
Observation/Hadal Research and Chapter 5's weaker spaces. Human balance and
subjective feel have not been proven by the automated checks. Rush medal/par
times, additional boss-specific drills, challenge modifiers and more optional
discoveries remain follow-up content work, rather than untested promises in
this first pass.
