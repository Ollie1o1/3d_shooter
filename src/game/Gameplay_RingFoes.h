#pragma once
// =============================================================================
// Gameplay_RingFoes.h — GameplayState: Chapter 6's DISPATCHED and the sweep
// (their minds: enemies/EnemyRing.h), and the drifting chunks drawn.
//
//   STRAYS   while the posts are lit, each is handed the nearest lit post's
//            spot and runs for it; one that hits you during a countdown shoves
//            you out of the lee, hard (it wants that shelter).
//   HERDERS  during a countdown each takes the loose shelter nearest you that
//            nobody else is shoving, flies to its far side and pushes it along
//            its rail (a DRIVEN mover) and off your line. Kill it and it stops.
//   TALLIES  counted in tickSweep: while one stands, the gap between sweeps halves.
// =============================================================================

inline void GameplayState::ringFoes(float dt) {
    const bool lit = sweep.active() && sweep.lethal();
    std::vector<int> taken;
    for (auto& e : enemies) {
        if (!e.alive) continue;
        if (e.type == EnemyType::STRAY) {
            e.ringSeekSet = false;
            if (!lit) continue;
            float best = 60.f;
            for (const SweepShelter& s : sweepShelters) {
                if (!sweep.shelterLit(s)) continue;
                const float d = glm::length(glm::vec2{s.post.x - e.position.x, s.post.z - e.position.z});
                if (d < best && std::fabs(s.post.y - e.position.y) < 2.5f) { best = d; e.ringSeek = s.post - sweep.spec.dir * 0.8f; e.ringSeekSet = true; }
            }
        } else if (e.type == EnemyType::HERDER) {
            e.ringPush = -1;
            if (!lit || e.stunned() || !e.targetable()) continue;
            float best = 45.f; int pick = -1;
            for (int i = 0; i < (int)sweepShelters.size(); ++i) {
                const SweepShelter& s = sweepShelters[i];
                if (s.mover < 0 || !s.alive || std::find(taken.begin(), taken.end(), i) != taken.end()) continue;
                const Mover& m = level.movers[s.mover];
                if (m.drive >= 0.999f) continue;
                const glm::vec3 c = (s.box.min + s.box.max) * 0.5f;
                const float d = glm::length(glm::vec2{c.x - player.position.x, c.z - player.position.z});
                if (d < best) { best = d; pick = i; }
            }
            if (pick < 0) continue;
            taken.push_back(pick);
            const SweepShelter& s = sweepShelters[pick];
            Mover& m = level.movers[s.mover];
            const glm::vec3 c = (s.box.min + s.box.max) * 0.5f, half = (s.box.max - s.box.min) * 0.5f;
            const glm::vec3 dir = glm::normalize(m.b - m.a);
            e.ringPush = pick;
            e.ringPushAt = c - dir * (std::fabs(dir.x) * half.x + std::fabs(dir.z) * half.z + 1.4f);
            e.ringPushAt.y = s.box.min.y;
            if (glm::length(glm::vec2{e.ringPushAt.x - e.position.x, e.ringPushAt.z - e.position.z}) < ringfoe::PUSH_REACH) {
                m.drive = std::min(1.f, m.drive + dt * 0.3f);   // about three seconds to shove it clear
                if (std::fmod(gameClock, 0.1f) < dt) {
                    fx.spawnBurst(glm::vec3{c.x, s.box.min.y + 0.2f, c.z} + dir * half.x, SWEEP_ICE * 0.6f, 3, 3.f, 0.7f, -0.5f);
                    audio.playAt("skim", c, 70, SoundGroup::ENEMY, true, 0.6f);
                }
            }
        }
    }
}

inline void GameplayState::ringFoeEvents(Enemy& e, const EnemyEvents& ev) {
    // A Stray's blow during a countdown throws you out of its way (low gravity: a long way)
    if (e.type == EnemyType::STRAY && ev.meleeHit && sweep.active() && sweep.lethal() && !playerDead) {
        glm::vec3 side{player.position.x - e.position.x, 0.f, player.position.z - e.position.z};
        if (glm::length(side) < 0.01f) side = {1.f, 0.f, 0.f};
        side = glm::normalize(side);
        player.velocity += side * 9.f + glm::vec3{0.f, 4.f, 0.f};
        player.launched();
        ui.feed("SHOVED OUT", SWEEP_ICE);
    }
    if (e.type == EnemyType::HERDER && ev.dashStarted) audio.playAt("dash", e.position, 110, SoundGroup::ENEMY, true, 0.7f);
}

// The drifting chunks (Mover::look 1): a slab of ice you stand on, lumps under and round it
// so it reads as a broken piece of the ring, not a platform
inline void GameplayState::gatherRing(std::vector<BoxInstance>& out) {
    using namespace rig;
    const float ball = (float)meshlib::lumpBall();
    const glm::vec3 eye = player.camera.position;
    for (const Mover& m : level.movers) {
        if (m.look != 1 || m.gone) continue;
        AABB bx = level.walls[m.wall].box;
        bx.min -= m.delta * (1.f - renderAlpha); bx.max -= m.delta * (1.f - renderAlpha);
        const glm::vec3 c = (bx.min + bx.max) * 0.5f, sz = bx.max - bx.min;
        if (glm::length(c - eye) > 900.f) continue;
        const float k = m.base.min.x * 0.37f + m.base.min.z * 0.11f;
        const glm::vec3 col = m.color * 0.72f;   // (the sun is hard out here: keep the ice off white)
        push(out, T(c) * S(sz), col);
        out.back().surface = 6.f;   // (distant ground: a grain that travels with it)
        push(out, T(glm::vec3{c.x, bx.max.y + 0.005f, c.z}) * S(glm::vec3{sz.x * 0.98f, 0.01f, sz.z * 0.98f}), glm::mix(col, glm::vec3{0.5f, 0.43f, 0.33f}, 0.4f));
        // Underneath and at the ends: lumps, so the edges are ragged and it hangs over nothing
        const int lumps = 3 + (int)(sz.x / 5.f);
        for (int i = 0; i < lumps; ++i) {
            const float u = (i + 0.5f) / lumps - 0.5f;
            const float r = 0.5f + 0.5f * ring::hash(k + i, 3.f);
            BoxInstance bi; bi.color = col * (0.82f + 0.12f * ring::hash(k + i, 4.f)); bi.mesh = ball;
            bi.model = T(glm::vec3{c.x + u * sz.x, bx.min.y - sz.y * 0.1f * r, c.z + (ring::hash(k + i, 5.f) - 0.5f) * sz.z * 0.4f})
                     * RY(ring::hash(k + i, 6.f) * 6.28f) * S(glm::vec3{sz.x / lumps * 1.7f, sz.y * (1.2f + 1.4f * r), sz.z * (0.9f + 0.3f * r)});
            out.push_back(bi);
        }
        for (int e = -1; e <= 1; e += 2) {   // its two ends, rounded off
            BoxInstance bi; bi.color = col * 0.92f; bi.mesh = ball;
            bi.model = T(glm::vec3{c.x + e * sz.x * 0.5f, c.y - sz.y * 0.1f, c.z}) * RY(k + e) * S(glm::vec3{sz.z * 0.7f, sz.y * 1.15f, sz.z * 1.05f});
            out.push_back(bi);
        }
    }
    // The HOLDFAST capsule: a glint on its spike until it's taken, so you find it from the lanes
    if (!holdfastFound && !g_devRingTest) {
        const float on = 0.6f + 0.4f * std::sin(gameClock * 3.f);
        push(out, T(ring::holdfastCapsule() + glm::vec3{2.f, 1.5f, 0.f}) * S(glm::vec3{0.25f}), HOLDFAST_STEEL, HOLDFAST_STEEL * 2.5f * on);
        push(out, T(ring::holdfastCapsule() + glm::vec3{2.f, 12.f, 0.f}) * S(glm::vec3{0.05f, 20.f, 0.05f}), glm::vec3{0.f}, HOLDFAST_STEEL * 0.5f * on);
    }
    // The hull's berths: the one still occupied, its lamp counting
    if (!g_devRingTest) {
        const AABB H = ring::quietHull();
        const glm::vec3 lamp{H.min.x + 1.3f, 1.25f, H.min.z + 3.f};
        if (glm::length(lamp - eye) < 120.f) {
            const float beat = std::fmod(gameClock, 1.12f) < 0.15f ? 1.f : 0.15f;
            push(out, T(lamp) * S(glm::vec3{0.18f}), {1.f, 0.7f, 0.3f}, glm::vec3{2.6f, 1.4f, 0.35f} * beat);
            push(out, T(lamp + glm::vec3{0.f, -0.1f, 0.4f}) * S(glm::vec3{0.9f, 0.5f, 1.6f}), glm::vec3{0.75f, 0.77f, 0.77f});   // its berth's lid, shut
        }
    }
}
