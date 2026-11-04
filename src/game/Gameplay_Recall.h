#pragma once
// =============================================================================
// Gameplay_Recall.h — RECALL in the world (the numbers: player/Recall.h).
//
// The button leaves a frozen statue of UNIT 24 where he stands; the second
// press snaps him back to it and it shatters. Owned with --recall for now;
// Chapter 5 hands it over at Rack 24 (docs/chapter-5-design.md).
// =============================================================================
#include "player/Recall.h"

inline bool GameplayState::recallOwned() const { return g_devRecall || (cold() && coldRecall); }

// Could UNIT 24 stand at `feet` right now? (a gate shut since, a mover in the way,
// or a fight he would leave)
inline bool GameplayState::recallClear(glm::vec3 feet) const {
    if (director.fighting() && level.arenaAt(feet) != level.arenaAt(player.position)) return false;
    AABB b{feet + glm::vec3{-player.radius + 0.05f, 0.1f, -player.radius + 0.05f},
           feet + glm::vec3{player.radius - 0.05f, player.height - 0.05f, player.radius - 0.05f}};
    auto hit = [&](const AABB& w) {
        return b.min.x < w.max.x && b.max.x > w.min.x && b.min.y < w.max.y && b.max.y > w.min.y && b.min.z < w.max.z && b.max.z > w.min.z;
    };
    for (const auto& w : level.walls) if (hit(w.box)) return false;
    return true;
}

inline void GameplayState::pressRecall() {
    recallPressedAt = gameClock;   // (THE RESTORER's ROLLBACK: a press in its last moment overrides it)
    const glm::vec3 f = player.camera.forward();
    const glm::vec3 eye = player.camera.position;
    switch (recall.press(player.position, std::atan2(f.x, f.z))) {
    case RecallSystem::Press::PLACED:
        fx.spawnBurst(player.position + glm::vec3{0.f, 1.f, 0.f}, RECALL_FROST, 14, 2.5f, 0.6f, -1.f);
        audio.playAt("cyl_close", player.position, 100, SoundGroup::PLAYER, true, 0.7f);
        if (recallHints < 1) { ++recallHints; ui.toast("RECALL", "PRESS AGAIN TO RETURN. THE STATUE SHATTERS", RECALL_FROST, 2.4f); }
        break;
    case RecallSystem::Press::RECALLED: {
        const glm::vec3 to = recall.feet;
        if (!recallClear(to)) {   // somewhere he can't be any more: it just breaks
            fx.spawnBurst(to + glm::vec3{0.f, 1.f, 0.f}, RECALL_FROST, 20, 4.f, 0.5f, 9.f);
            audio.playAt("ab_shatter", to, 90, SoundGroup::PLAYER, true, 1.3f);
            ui.feed("RECALL BLOCKED", RECALL_FROST);
            break;
        }
        fx.spawnTracer(eye, to + glm::vec3{0.f, player.eyeHeight, 0.f}, 0.12f, 0.3f, RECALL_FROST * 1.6f);
        fx.spawnBurst(eye, RECALL_FROST, 12, 3.f, 0.4f, -1.f);
        grapple.release();
        player.position = to;
        player.velocity = glm::vec3{0.f};
        player.camera.position = to + glm::vec3{0.f, player.eyeHeight, 0.f};   // (the view is untouched: yaw and pitch stay yours)
        prevCamPos = player.camera.position;
        // The statue goes off around him
        pendingBlasts.push_back({to + glm::vec3{0.f, 1.f, 0.f}, RecallSystem::SHATTER_RADIUS, RecallSystem::SHATTER_DAMAGE, 0.f, 0.f, StyleSource::EXPLOSIVE});
        fx.spawnBurst(to + glm::vec3{0.f, 1.f, 0.f}, RECALL_FROST, 40, 9.f, 0.7f, 12.f);
        fx.spawnShockwave(to + glm::vec3{0.f, 0.1f, 0.f}, RecallSystem::SHATTER_RADIUS, RECALL_FROST);
        audio.playAt("ab_shatter", to, 128, SoundGroup::PLAYER, true, 1.1f);
        audio.play("dash", 100, SoundGroup::PLAYER);
        shake(0.18f, 0.04f);
        ui.feed("RECALLED", RECALL_FROST);
        break;
    }
    case RecallSystem::Press::NONE: break;
    }
}

inline void GameplayState::tickRecall(float dt) {
    if (recall.tick(dt)) {   // left too long: it crumbles, harmlessly
        fx.spawnBurst(recall.feet + glm::vec3{0.f, 0.8f, 0.f}, RECALL_FROST, 18, 2.f, 0.8f, 9.f);
        audio.playAt("ab_shatter", recall.feet, 70, SoundGroup::PLAYER, true, 1.4f);
    }
}

// A nearby enemy, while the statue is fresh, takes it for you
inline bool GameplayState::recallLures(const Enemy& e) const {
    if (!recall.luring() || isBoss(e.type)) return false;
    glm::vec3 d = e.position - recall.feet;
    return glm::dot(d, d) < RecallSystem::LURE_RANGE * RecallSystem::LURE_RANGE;
}

// The statue: UNIT 24 standing, frosted white, his lights out. Cracks of
// light run up it in its last two seconds.
inline void GameplayState::gatherRecall(std::vector<BoxInstance>& out) {
    using namespace rig; using namespace enemyfile;
    if (!recall.placed) return;
    const auto* U = story::bodyModel("unit24");
    const float fade = std::clamp((RecallSystem::WINDOW - recall.age) / 2.f, 0.f, 1.f);   // 1 .. 0 over its last two seconds
    const float flick = (1.f - fade) * (0.5f + 0.5f * std::sin(gameClock * 30.f));
    const glm::vec3 glow = RECALL_FROST * (0.15f + 1.2f * flick);
    size_t from = out.size();
    if (U) {
        FilePose P;
        P.motion[E_ARMR] = RX(-0.25f); P.motion[E_ELBOW_R] = RX(-0.5f);   // mid-stride, as he was
        P.motion[E_ARML] = RX(0.15f);  P.motion[E_HIPL] = RX(-0.2f); P.motion[E_HIPR] = RX(0.15f);
        P.chan[C_EYE] = glm::vec3{0.f}; P.chan[C_CORE] = glow;
        Rig rg{out};
        drawFile(rg, T(recall.feet) * RY(recall.yaw), *U, P, glow);
    } else {   // (no model file: a frosted pillar his size)
        push(out, T(recall.feet + glm::vec3{0.f, 0.9f, 0.f}) * RY(recall.yaw) * S(glm::vec3{0.6f, 1.8f, 0.35f}), RECALL_FROST, glow);
    }
    for (size_t i = from; i < out.size(); ++i) {   // frost over whatever colour the part is
        out[i].color = glm::mix(out[i].color, RECALL_FROST, 0.75f);
        out[i].emissive = glm::max(out[i].emissive * 0.2f, glow * 0.4f);
        out[i].surface = 0.f;
    }
}
