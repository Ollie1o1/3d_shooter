#pragma once
// =============================================================================
// Gameplay_Gap.h — Chapter 6, level 6: THE GAP (docs/chapter-6-design.md
// sections 3, 5, 6 and 13.7). The Shepherd has carved the ring through: a cliff,
// 480 m of nothing, the ring going on beyond.
//
//   the raft     a slab of ring ice moored at the near lip (LevelData::raftMover,
//                raftBoulder: DRIVEN movers). Step on and it carries you across
//                at RAFT_SPEED. Its boulder is a shelter that HOLDS: spiked to it
//                with HOLDFAST, the wave can't take you (the one place, with the
//                hull in the Fall, where the spike alone saves you).
//   the sweep    one comes mid-crossing (the mid tier): behind your own raft's
//                boulder, or spiked to it.
//   THE DROVER   the second sheepdog, with its herd of chunks. It hangs beside
//                your raft and sends a chunk sliding across it (gold: jump it,
//                or get off its line); hit, you're thrown, maybe into the gap.
//                A third gone and it breaks off; when the raft docks it goes.
//   the lip      ice grains pour off it in a slow, silent fall.
// A fall into the gap is the arena's voidY: back to the near side, raft moored.
// =============================================================================

// (its numbers: gapride:: in world/LevelRing.h)

inline void GameplayState::resetGap() {
    gapRide = false; gapSweepDone = false; gapDroverUid = -1; gapCharge.on = false; gapSide = 1.f; gapSideT = 0.f;
}

inline bool GameplayState::onRaft() const {
    if (level.raftMover < 0 || !player.onGround) return false;
    const int m = level.moverOfWall(player.groundWall);
    return m == level.raftMover || m == level.raftBoulder;
}

inline void GameplayState::tickGap(float dt) {
    using namespace gapride;
    if (level.raftMover < 0 || g_devRingTest) return;
    Mover& raft = level.movers[level.raftMover];
    Mover& boulder = level.movers[level.raftBoulder];
    // Aboard: off it goes, and the Drover comes with its herd
    if (!gapRide && (onRaft() || (g_devOverlay == "raftride" && elapsedTime > 1.f)) && !playerDead) {   // (--overlay raftride: cast off without boarding, for screenshots)
        gapRide = true;
        ui.feed("CAST OFF", SWEEP_ICE * 0.7f);
        const AABB& rb = level.walls[raft.wall].box;
        spawnEnemy(EnemyType::DROVER, glm::vec3{-32.f, 9.f, (rb.min.z + rb.max.z) * 0.5f - 30.f}, Hollow::NONE);
        gapDroverUid = enemies.back().uid;
        pushBanner("THE DROVER", "IT MOVES THE HERD", HOLDFAST_STEEL, 3.f);
    }
    if (gapRide && raft.drive < 1.f) {
        raft.drive = boulder.drive = std::min(1.f, raft.drive + dt * ring::RAFT_SPEED / ring::raftRun());
        if (raft.drive >= 1.f) {   // docked at the far lip: the Drover goes on ahead
            ui.feed("ACROSS", SWEEP_ICE * 0.7f);
            if (Enemy* d = enemyByUid(gapDroverUid); d && d->alive && d->crookLeave < 0.f) d->crookLeave = 0.f;
        }
        // Halfway over, the Shepherd pulses: shelter behind your own raft, or spike yourself to it
        if (!gapSweepDone && raft.drive > 0.33f && !sweep.active()) {
            gapSweepDone = true; sweep.start(nextSweep()); ++sweepCount;
        }
    }
    const AABB& rb = level.walls[raft.wall].box;
    const glm::vec3 rc = (rb.min + rb.max) * 0.5f;
    // The Drover's place: beside the raft, a little ahead, swapping sides now and then
    if ((gapSideT += dt) > 13.f) { gapSideT = 0.f; gapSide = -gapSide; }
    if (Enemy* d = enemyByUid(gapDroverUid); d && d->alive)
        d->droverPost = glm::vec3{gapSide * 26.f, rb.max.y + 7.f, rc.z - 8.f};
    // A chunk of the herd sliding across the raft
    if (gapCharge.on) {
        gapCharge.pos += glm::vec3{gapCharge.dir * CHARGE_SPEED * dt, 0.f, raft.delta.z};   // (it rides the raft's way while it crosses)
        const glm::vec3 p = player.position, h = CHUNK_HALF;
        if (!gapCharge.hit && !playerDead && std::fabs(p.x - gapCharge.pos.x) < h.x + 0.4f && std::fabs(p.z - gapCharge.pos.z) < h.z + 0.4f &&
            p.y < gapCharge.pos.y + h.y && p.y + player.height > gapCharge.pos.y - h.y) {
            gapCharge.hit = true;
            damagePlayer(CHARGE_DAMAGE, gapCharge.pos, 0.3f, 0.05f, 0.f, {EnemyType::DROVER, DamageKind::MELEE});
            holdfast.cut();
            player.velocity += glm::vec3{gapCharge.dir * 13.f, 6.f, 0.f}; player.launched();
            ui.feed("RAMMED", HOLDFAST_STEEL);
        }
        if (std::fabs(gapCharge.pos.x - gapCharge.from) > 80.f) gapCharge.on = false;
    }
    // Ice grains pouring off the lips, near you
    for (float lip : {ring::GAP_S, ring::GAP_N})
        if (std::fabs(player.position.z - lip) < 90.f && std::fmod(gameClock, 0.08f) < dt) {
            const float x = player.position.x + cosmetic::range(-40.f, 40.f);
            fx.spawnBurst({x, -0.2f, lip + (lip == ring::GAP_S ? -0.6f : 0.6f)}, SWEEP_ICE * 0.6f, 2, 0.3f, 4.f, -1.2f);
        }
}

// THE DROVER's charge: a chunk launched from its side of the raft, across, at your height on the deck
inline void GameplayState::gapLaunch(const Enemy& d) {
    if (level.raftMover < 0) return;
    const AABB& rb = level.walls[level.movers[level.raftMover].wall].box;
    gapCharge.on = true; gapCharge.hit = false;
    gapCharge.dir = d.position.x < player.position.x ? 1.f : -1.f;
    gapCharge.from = d.position.x;
    gapCharge.pos = {d.position.x, rb.max.y + gapride::CHUNK_HALF.y, player.position.z};
    audio.playAt("ab_rumble", gapCharge.pos, 120, SoundGroup::ENEMY, true, 0.7f);
}

// The herd (chunks in formation round the Drover), the one it's about to send (gold), the one sliding
inline void GameplayState::gatherGap(std::vector<BoxInstance>& out) {
    using namespace rig;
    const glm::vec3 ice{0.62f, 0.6f, 0.55f};
    auto chunk = [&](glm::vec3 c, glm::vec3 half, glm::vec3 glow, float seed) {
        push(out, T(c) * RY(seed) * S(half * 2.f), ice * (0.85f + 0.1f * ring::hash(seed, 1.f)), glow);
        BoxInstance bi; bi.mesh = (float)meshlib::lumpBall(); bi.color = ice * 0.8f;
        bi.model = T(c - glm::vec3{0.f, half.y * 0.6f, 0.f}) * RY(seed * 2.f) * S(glm::vec3{half.x * 1.7f, half.y * 1.4f, half.z * 1.6f});
        out.push_back(bi);
    };
    if (Enemy* d = enemyByUid(gapDroverUid); d && d->alive) {
        const glm::vec3 fwd{std::sin(d->yaw), 0.f, std::cos(d->yaw)}, side{fwd.z, 0.f, -fwd.x};
        for (int k = 0; k < 5; ++k) {   // its herd: a loose wedge ahead of its forks
            const float fk = (float)k;
            const glm::vec3 c = d->position + fwd * (4.f + 3.f * (k / 2)) + side * ((k % 2 ? 1.f : -1.f) * (1.5f + 2.2f * (k / 2))) + glm::vec3{0.f, -1.f + 0.3f * std::sin(gameClock + fk), 0.f};
            chunk(c, glm::vec3{1.4f, 0.9f, 1.2f} * (0.8f + 0.3f * ring::hash(fk, 3.f)), glm::vec3{0.f}, fk + 1.f);
        }
        if (d->attack == AttackKind::RING_CHARGE && d->telegraphTimer > 0.f) {   // the one it means to send, glowing gold
            const glm::vec3 g = abGoldTell(glm::vec3{1.9f, 1.15f, 0.15f}) * (0.6f + 1.4f * d->telegraphProgress());
            chunk(d->position + fwd * 2.5f - glm::vec3{0.f, 1.2f, 0.f}, gapride::CHUNK_HALF * 0.9f, g, 9.f);
        }
    }
    if (gapCharge.on) chunk(gapCharge.pos, gapride::CHUNK_HALF, abGoldTell(glm::vec3{1.9f, 1.15f, 0.15f}) * 0.6f, gapCharge.from);
}
