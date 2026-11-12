#pragma once
// =============================================================================
// Gameplay_Spokes.h — Chapter 6, level 5: THE SPOKES (docs/chapter-6-design.md
// sections 3 and 10). Streaks of charged dust lie across the ring (ring::spokes()).
// Inside one there is no dash and no grapple: the ANCHOR enemy's field at the
// scale of the landscape, through the same check (anchoredAt) and the same
// refusal (pinnedCue). HOLDFAST still works: it is a spike, not a hook.
// You can't grapple to shelter in there; you walk to it, and the routes cross
// the spokes where they are narrow (the crossings are kept clear of rock).
//
// Their colour is a pale violet "static", not the ANCHOR's red: red belongs to
// the sweep's countdown alone in this chapter (design section 1).
// =============================================================================

inline const glm::vec3 SPOKE_STATIC{0.72f, 0.6f, 1.05f};

inline bool GameplayState::inSpoke(glm::vec3 feet) const { return ring() && !g_devRingTest && ring::spokeAt(feet) >= 0; }

inline void GameplayState::tickSpokes(float dt) {
    const bool in = inSpoke(player.position);
    if (in && !spokeHint) {
        spokeHint = true;
        ui.toast("CHARGED DUST", "INSIDE A SPOKE: NO DASH, NO GRAPPLE. WALK, OR CROSS WHERE IT NARROWS", SPOKE_STATIC, 5.f);
    }
    if (in && grapple.active) { grapple.release(); pinnedCue(); }   // (the line goes dead in the dust)
    // Static, crackling round you while you're in it
    if (in && (spokeCrackle -= dt) <= 0.f) {
        spokeCrackle = cosmetic::range(0.35f, 1.1f);
        SfxMixer::Opts o; o.group = SoundGroup::PLAYER; o.volume = 0.3f; o.pitch = cosmetic::range(1.3f, 2.f);
        audio.playOpts("ab_tsk", o);
        fx.spawnBurst(player.position + glm::vec3{cosmetic::range(-1.f, 1.f), cosmetic::range(0.2f, 1.4f), cosmetic::range(-1.f, 1.f)}, SPOKE_STATIC, 3, 2.f, 0.25f, 0.f);
    }
}

// The spokes, live: their edges marked by faint stakes of static (the ANCHOR's ribs, in violet),
// and sparks hanging in the dust near you
inline void GameplayState::gatherSpokes(std::vector<BoxInstance>& out) {
    using namespace rig;
    if (!ring() || g_devRingTest) return;
    const glm::vec3 eye = player.camera.position;
    if (eye.z > ring::SPOKES_S + 150.f || eye.z < ring::SPOKES_N - 150.f) return;
    const float t = gameClock;
    const auto all = ring::spokes();
    for (int i = 0; i < (int)all.size(); ++i) {
        const ring::Spoke& sp = all[i];
        if (std::fabs(ring::spokeZ(sp, eye.x) - eye.z) > 160.f) continue;
        for (float x = -ring::HALF_X; x <= ring::HALF_X; x += 6.f) {
            const float zc = ring::spokeZ(sp, x), h = ring::spokeHalf(sp, x);
            const float pulse = 0.55f + 0.45f * std::sin(t * 2.3f + x * 0.3f + i);
            for (int side = -1; side <= 1; side += 2) {   // the edges: a stake every 6 m, and a line on the ice
                const glm::vec3 at{x, 0.f, zc + side * h};
                if (glm::length(at - eye) > 140.f) continue;
                push(out, T(at + glm::vec3{0.f, 0.05f, 0.f}) * RY(std::atan(sp.slope)) * S(glm::vec3{6.f, 0.04f, 0.12f}), SPOKE_STATIC * 0.2f, SPOKE_STATIC * 0.8f * pulse);
                push(out, T(at + glm::vec3{0.f, 0.6f, 0.f}) * S(glm::vec3{0.05f, 1.2f, 0.05f}), SPOKE_STATIC * 0.1f, SPOKE_STATIC * 0.5f * pulse);
            }
        }
    }
    // Sparks in the dust round you: hanging, drifting, flickering
    for (int k = 0; k < 90; ++k) {
        const float fk = (float)k;
        const glm::vec3 p{eye.x + (ring::hash(fk, 120.f) - 0.5f) * 50.f + std::sin(t * 0.3f + fk) * 1.5f,
                          0.2f + 3.5f * ring::hash(fk, 121.f) + std::sin(t * 0.7f + fk * 1.3f) * 0.3f,
                          eye.z + (ring::hash(fk, 122.f) - 0.5f) * 50.f + std::cos(t * 0.25f + fk) * 1.5f};
        if (ring::spokeAt(p) < 0) continue;
        const float on = ring::hash(std::floor(t * 9.f) + fk, 123.f) > 0.55f ? 1.f : 0.25f;
        push(out, T(p) * S(glm::vec3{0.05f}), glm::vec3{0.f}, SPOKE_STATIC * 1.6f * on);
    }
}
