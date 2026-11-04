# The Living Ward — verification record

The implementation and release are covered by the following checks. Source is the
authority for authored behavior; automated runs prove the named paths, not human feel.

| Requirement | Evidence |
|---|---|
| Ten spacious descending rooms, tenth boss, ordinary connected hospital routes | `LevelLab.h`; `test_lab.cpp` walks the complete descent with the actual Player controller and zone constraints, checks all ten checkpoint floors and every gallery in both directions. |
| Hospital → garden → pit → suspension → nebula → trench → final assessment | Authored geometry, skies and observation glass; native captures `build/lab-ward-ready.png`, `lab-garden-ready.png`, `lab-load-final.png`, `lab-celestial-release.bmp`, `lab-hadal-final.png`, `lab-surgical-audit.png`, `lab-boss-final.png`. Exteriors include actual middle-distance geometry. |
| Readable anomalies and details | Entry subject/shadow discrepancy, impossible window, sealed observation booth, dormant floating patients, repeated Subject 17 beds and C-17 trolley, exterior chair/tools, patient outside roof, explicit medical records. Low-detail Hadal and surgical/trolley views were captured natively. |
| Intro normal, skip, retry and save paths | `LabIntro.h`, `Gameplay_Lab.h`, `StorySave.h`; headless clock/skip tests plus `test_dev.cpp` live skip/retry checks. `build/lab-entry-preview.mp4` shows all 23 seconds with score and PA excerpts. It is a preview mix, not a claim to capture the exact live final mix. |
| Four original flesh enemy families and counters | Original GLBs and animation; `EnemyLab.h`; headless committed-lunge, directional-plate, locked-beam and announced-exit tests. Native tests resolve all four attack damage paths, then interrupt them through actual gun hit/poise processing. |
| Routing controls and secrets work physically | Tests exercise both physical bridge sides, live external-objective controls, retry persistence, gallery routes, both numbered secret records and their actual body files. |
| Fast near-human boss, robot face, independent moves | Original Successor GLB/rig and independent state/events/hazards. Live revolver node cut and fist parry; tests cover the seven attack kinds, phases, target commitment, hit timing, cancellation and elevated/ranged camping answers. |
| Finite recovery economy and a damageable finale | Two placed health pickups; bounded parry healing; no required add wave or sword. Actual revolver/ammo/reload/controller rehearsals at three opening clocks. Lethal damage uses the normal kill path; no finale invulnerability. |
| Original score, speech and tell priority | Original MusicSynth tracks 12/13, four-part formant choir, organ and fast percussion; original synthetic spoken WAVs; distinct per-attack cue recipes. Mixer/voice tests check tell priority, levels, bounded output and the 320-second voice-bank budget. 24 seconds of the phase-three score renders in roughly 0.37 s on this machine, peak 0.832/RMS 0.302. This is score-only CPU/level evidence, not an audition. |
| Boss victory opens a playable dimensional threshold | Headless real-controller corridor traversal, live kill cleanup/ending tests and retry-after-victory check; saved boss defeat suppresses respawning and opens the exit. |
| Existing game/save/mode integration survives | Version-one save migration, four-gun upgrades, three-chapter runtime handoff, all 27 dev checkpoints, all existing headless suites, unchanged original leaderboard IDs. Captures/practice do not write the Story slot. |
| Native and web asset/stack correctness | Native build, `make test`, `make test-dev`, native rendering regressions, `--verify-story`, `make web`, `make test-web-story`; the latter loads all three Story boss GLBs in actual WASM on a 32 KiB stack. All four maps are checked for closed surfaces and current bakes. |
| Publish a reviewable build | Adjacent website sync script; site build/dist validation, 35 website tests and Astro type checks. Production game files are checked byte-for-byte against the prepared release after deployment. |

Representative local logs are in `build/lab-release-tests.log`, `lab-final-lab.log`,
`lab-final-runtime.log`, `lab-final-linux.log`, `lab-release-handoff.log`,
`lab-final-web.log` and `lab-score-profile.log`.

Human review still matters: no in-app browser is connected, and this environment
does not accept audio input. Browser pointer-lock/controller feel, the subjective
choir/voice mix, and an end-to-end human balance pass have not been observed here.
The playable chapter and listening/video previews are supplied for that review.
The 35–50 minute exploratory clear time in the design remains a pacing hypothesis.
