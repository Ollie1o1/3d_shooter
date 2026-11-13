#pragma once
// =============================================================================
// Gameplay_Shore.h — Chapter 6, level 8: THE SHORE (docs/chapter-6-design.md
// sections 3 and 13.4). The walk-up. No enemies: the Shepherd fills the north.
//
//   the slope    down the wave's back from the summit: the sun goes behind the
//                Shepherd as you go down (ring::eclipseAt), and its ridge lamps
//                are the light from here on.
//   the weight   gravity comes back as you walk in (ring::gravityAt): the
//                Shepherd is the heaviest thing here. Full by 120 m onto the ice.
//   the sweeps   the near tier: 6 s, breakers. Small shelter shatters; only a
//                post still lit after the first second holds. They come one after
//                another here (it is close, and it has seen you).
//   the foot     its first words, through the floor, and the end of the crossing.
// =============================================================================

inline void GameplayState::resetShore() { shoreSweeps = 0; shoreBreakHint = shoreWeightHint = shoreSpoke = false; }

inline float GameplayState::ringEclipse() const {
    if (!ring() || g_devRingTest) return 0.f;
    return ring::eclipseAt(player.position.z);
}

inline void GameplayState::tickShore(float dt) {
    (void)dt;
    if (!ring() || g_devRingTest || playerDead) return;
    const glm::vec3 p = player.position;
    if (p.z > ring::summitN()) return;
    // No random sweeps from here: the Shore's come on its own clock, below
    sweepSched.timer = std::max(sweepSched.timer, 60.f);
    if (p.z < ring::shoreS() && !sweep.active() && ringScriptT < 0.f && storyEndT < 0.f)
        ringScriptT = shoreSweeps == 0 ? 5.f : cosmetic::range(14.f, 20.f);
    if (sweep.active() && sweep.phase == RingSweep::Phase::FLASH && p.z < ring::shoreS() + 4.f) {
        if (!shoreBreakHint) {
            shoreBreakHint = true;
            ui.toast("BREAKERS", "SMALL SHELTER SHATTERS. ITS POST GOES DARK: GET BEHIND ONE STILL LIT", POST_LIT * 0.4f, 5.f);
        }
    }
    if (sweep.active() && sweep.t < dt * 1.5f && p.z < ring::shoreS() + 4.f) ++shoreSweeps;
    if (!shoreWeightHint && ring::gravityAt(p.z) > 0.8f) {
        shoreWeightHint = true;
        ui.toast("FULL WEIGHT", "THE SHEPHERD PULLS. YOU JUMP AND FALL AS YOU DID BEFORE THE RING", SWEEP_ICE * 0.6f, 4.5f);
    }
    // Its first words: through the floor, not the air (13.1). The screen shakes with them
    if (!shoreSpoke && p.z < ring::shoreN() + 90.f) {
        shoreSpoke = true;
        pushBanner("YOU WANDERED.", "THEY ALWAYS WANDER.", glm::vec3{0.9f, 0.9f, 0.82f}, 4.5f);
        shake(1.4f, 0.035f);
        SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.pitch = 0.45f; o.volume = 0.9f;
        audio.playOpts("slam", o);
        o.delay = 0.9f; o.pitch = 0.4f; audio.playOpts("slam", o);
    }
}
