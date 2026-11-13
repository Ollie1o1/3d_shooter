#pragma once
// =============================================================================
// Gameplay_Wake.h — Chapter 6, level 7: THE WAKE (docs/chapter-6-design.md
// sections 3, 6 and 13.7). The Shepherd's gravity has pulled the ring up into
// a frozen wave; you climb it in terraces (ring::wakeTreads).
//
//   the climb    each tread's ice mass shelters the tread below it: tucked
//                against the next riser, a wave goes over. A few treads are
//                deeper than the shadow: their south ends are open.
//   the sweep    the double sweep comes on the climb (two waves, the second
//                3.5 s behind: stay tucked in).
//   the summit   the first time you stand above the ring plane. The camera
//                holds on the view back (everything you crossed) for a few
//                seconds before THE BELLWETHER rings.
//   the bell     THE BELLWETHER's local sweeps (RING_BELL): a ring that runs
//                out along the ground from it. It hurts and throws but never
//                kills, and has no countdown: the corner number is the
//                Shepherd's alone. Jump it.
// =============================================================================

// (its numbers: wakebell:: in world/LevelRing.h)

inline void GameplayState::resetWake() { wakeSweepDone = false; wakeViewT = -1.f; wakeRings.clear(); }

inline void GameplayState::tickWake(float dt) {
    using namespace wakebell;
    if (!ring() || g_devRingTest) return;
    const glm::vec3 p = player.position;
    // The double sweep, a third of the way up
    const auto T = ring::wakeTreads();
    if (!wakeSweepDone && p.z < T[4].zS && p.z > ring::summitS() && !sweep.active() && !playerDead) {
        wakeSweepDone = true;
        SweepSpec s = sweeptier::mid(ring::sweepDir()).spec;   // (two waves)
        sweep.start(s); ++sweepCount;
        ui.toast("THE WAKE", "TWO WAVES: TUCK IN AGAINST THE STEP ABOVE YOU, AND STAY THERE", SWEEP_ICE * 0.6f, 4.f);
    }
    // The summit: hold on the view back before the bell
    if (wakeViewT < 0.f && p.z < ring::summitS() - 2.f && p.y > ring::summitY() - 1.f && !playerDead) {
        wakeViewT = 0.f;
        pushBanner("ABOVE THE RING", "EVERYTHING YOU CROSSED", SWEEP_ICE * 0.6f, 3.5f);
    }
    if (wakeViewT >= 0.f && wakeViewT < 4.f) {
        wakeViewT += dt;
        for (auto& e : enemies) if (e.type == EnemyType::BELLWETHER && e.alive) e.attackTimer = std::max(e.attackTimer, 4.f - wakeViewT);   // (it waits for you to look)
    }
    // The rings, running out
    for (auto& r : wakeRings) {
        const float before = r.radius;
        r.radius += SPEED * dt;
        const float d = glm::length(glm::vec2{p.x - r.at.x, p.z - r.at.z});
        const bool crossed = before - BAND < d && d <= r.radius + BAND * 0.5f;
        if (!r.hit && crossed && !playerDead && p.y - r.at.y < JUMP_CLEAR && p.y - r.at.y > -3.f) {   // (on its floor: the Shepherd's belfry has two)   // (in the air over it: it passes under you)
            r.hit = true;
            damagePlayer(DAMAGE, r.at, 0.25f, 0.04f, 0.4f, {EnemyType::BELLWETHER, DamageKind::SHOCKWAVE});
            const glm::vec3 out = glm::normalize(glm::vec3{p.x - r.at.x, 0.f, p.z - r.at.z} + glm::vec3{1e-4f});
            holdfast.cut();
            player.velocity += out * 11.f + glm::vec3{0.f, 5.f, 0.f}; player.launched();
            ui.feed("RUNG", HOLDFAST_STEEL);
        }
    }
    wakeRings.erase(std::remove_if(wakeRings.begin(), wakeRings.end(), [](const WakeRing& r) { return r.radius > wakebell::REACH; }), wakeRings.end());
}

inline void GameplayState::wakeRing(const Enemy& e) {
    wakeRings.push_back({glm::vec3{e.position.x, e.floorY, e.position.z}, 0.6f, false});
    audio.playAt("ab_ting", e.position + glm::vec3{0.f, 3.f, 0.f}, 128, SoundGroup::ENEMY, true, 0.6f);
    fx.spawnShockwave(e.position, 2.f, glm::vec3{1.9f, 1.4f, 0.4f});
}

// The rings on the ground, and the Bellwether's flock (three capsules on tethers, trailing it)
inline void GameplayState::gatherWake(std::vector<BoxInstance>& out) {
    using namespace rig;
    const glm::vec3 gold = abGoldTell(glm::vec3{1.9f, 1.15f, 0.15f});
    for (const WakeRing& r : wakeRings) {
        const int n = std::max(16, (int)(r.radius * 2.5f));
        const float fade = 1.f - r.radius / wakebell::REACH;
        for (int k = 0; k < n; ++k) {
            const float a = 6.2831853f * k / n;
            const glm::vec3 at = r.at + glm::vec3{std::cos(a) * r.radius, 0.35f, std::sin(a) * r.radius};
            push(out, T(at) * RY(-a) * S(glm::vec3{0.3f, 0.7f, 6.2831853f * r.radius / n + 0.05f}), gold * 0.2f, gold * (0.6f + 0.8f * fade));
        }
    }
    for (const auto& e : enemies) {
        if (e.type != EnemyType::BELLWETHER || !e.alive) continue;
        const glm::vec3 back{-std::sin(e.yaw), 0.f, -std::cos(e.yaw)}, side{back.z, 0.f, -back.x};
        for (int k = 0; k < 3; ++k) {
            const glm::vec3 c = e.position + back * (3.f + 1.6f * k) + side * ((k - 1) * 1.8f) + glm::vec3{0.f, 0.95f + 0.1f * std::sin(gameClock * 2.f + k), 0.f};
            push(out, T(c) * RY(e.yaw + 0.2f * (k - 1)) * S(glm::vec3{1.9f, 1.8f, 4.5f}), glm::vec3{0.62f, 0.64f, 0.64f});
            push(out, T(c + glm::vec3{0.f, 0.95f, 0.f}) * S(glm::vec3{0.2f}), glm::vec3{0.f}, glm::vec3{2.4f, 1.3f, 0.35f});
            fx.spawnTracer(e.position + glm::vec3{0.f, 2.5f, 0.f}, c + glm::vec3{0.f, 0.5f, 0.f}, 0.012f, 0.03f, HOLDFAST_STEEL * 0.6f);
        }
    }
}
