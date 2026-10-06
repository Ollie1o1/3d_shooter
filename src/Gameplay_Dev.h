#pragma once
// =============================================================================
// Gameplay_Dev.h — GameplayState: Dev and footage tools: Sovereign poses, the scripted camera
// (--cam, --campath, --autoaim, --kite) and the practice wave skip.
// Included at the end of GameplayState.h.
// =============================================================================

inline void GameplayState::devPenitentPose(Enemy& e) {
    int n = std::atoi(g_devOverlay.c_str() + 8);
    e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
    glm::vec3 to = player.position - e.position;
    e.yaw = e.prevYaw = std::atan2(to.x, to.z);
    e.prevPosition = e.position;
    e.attack = AttackKind::NONE; e.telegraphTimer = 0.f; e.telegraphDuration = 1.f; e.riseTimer = 0.f;
    e.risen = n >= 3; e.scourging = n == 4;
    e.anchorsLeft = e.risen ? 0 : level.anchorsAlive();
    if (n == 1) { e.attack = AttackKind::CENSER_LOW;  e.telegraphTimer = 0.3f; }
    if (n == 2) { e.attack = AttackKind::CENSER_HIGH; e.telegraphTimer = 0.3f; }
    if (n == 4) e.yaw = e.prevYaw = e.yaw + 3.14159265f;   // its back to you: the wound
}

inline void GameplayState::devPose(Enemy& e) {
    int n = std::atoi(g_devOverlay.c_str() + 4);
    e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
    glm::vec3 to = player.position - e.position;
    e.yaw = e.prevYaw = std::atan2(to.x, to.z);
    e.prevPosition = e.position;
    e.attack = AttackKind::NONE; e.telegraphTimer = 0.f; e.telegraphDuration = 1.f;
    e.dashTimer = e.leapTimer = e.swingTimer = e.staggerTimer = e.whirlTimer = e.thrustTimer = 0.f;
    auto windup = [&](AttackKind k) { e.attack = k; e.telegraphTimer = 0.3f; };
    switch (n) {
        case 1: windup(AttackKind::DASH); break;
        case 2: e.dashTimer = 0.3f; e.diveDir = glm::normalize(glm::vec3{to.x, 0.f, to.z}); e.moveSpeed = 34.f; break;
        case 3: windup(AttackKind::SWEEP); break;
        case 4: e.swingTimer = Enemy::SWING_TIME * 0.5f; e.lastSwing = AttackKind::SWEEP; break;
        case 5: windup(AttackKind::CLEAVE); break;
        case 6: e.swingTimer = Enemy::SWING_TIME * 0.4f; e.lastSwing = AttackKind::CLEAVE; break;
        case 7: e.leapTimer = 1.f; break;
        case 8: e.staggerTimer = 1.f; break;
        case 9: e.enraged = true; break;
        case 10: windup(AttackKind::BLINK); break;
        case 11: windup(AttackKind::JUDGMENT); break;
        case 12: windup(AttackKind::WHIRL); break;
        case 13: e.whirlTimer = 1.f; break;
        case 14: windup(AttackKind::THRUST); break;
        case 15: e.thrustTimer = 0.2f; break;
        case 16: windup(AttackKind::PHANTOMS); break;
        default: break;
    }
}

inline void GameplayState::devCamera(float dt) {
    float u = glm::clamp(g_recProgress, 0.f, 1.f);
    u = u * u * (3.f - 2.f * u);
    glm::vec3 p = g_devCamPath ? glm::mix(g_devCamPos, g_devCamPos2, u) : g_devCamPos;
    if (g_devKite) {
        // Keep 8 m from the nearest enemy, circling a little as a player would
        const Enemy* near = nullptr; float nd = 1e9f;
        for (auto& e : enemies)
            if (e.targetable()) { float d = glm::length(glm::vec2(e.position.x - p.x - devKite.x, e.position.z - p.z - devKite.z)); if (d < nd) { nd = d; near = &e; } }
        if (near && nd < 8.f && nd > 0.01f) {
            glm::vec3 away = p + devKite - near->position; away.y = 0.f; away = glm::normalize(away);
            devKite += (away * 9.f + glm::vec3{-away.z, 0.f, away.x} * 3.f) * dt;
        }
        const AABB& z = level.arenas[director.arena].zone;
        glm::vec3 q = p + devKite;
        q.x = glm::clamp(q.x, z.min.x + 3.f, z.max.x - 3.f); q.z = glm::clamp(q.z, z.min.z + 3.f, z.max.z - 3.f);
        AABB body{q - glm::vec3{0.6f, 1.6f, 0.6f}, q + glm::vec3{0.6f, 0.3f, 0.6f}};
        bool inWall = false;
        for (auto& wl : level.walls)
            if (body.max.x > wl.box.min.x && body.min.x < wl.box.max.x && body.max.y > wl.box.min.y &&
                body.min.y < wl.box.max.y && body.max.z > wl.box.min.z && body.min.z < wl.box.max.z) { inWall = true; break; }
        if (inWall) q = p + devKiteSafe;   // back to the last spot that was clear
        devKite = devKiteSafe = q - p;
        p = q;
    }
    player.position = p - glm::vec3{0, player.eyeHeight, 0};
    player.camera.position = p;
    prevCamPos = p;
    if (g_devClean) { banners.clear(); controlHintTimer = 0.f; }
    if (!g_devAutoAim) {
        if (g_devCamPath) {
            player.camera.yaw = g_devCamYaw + std::remainder(g_devCamYaw2 - g_devCamYaw, 360.f) * u;
            player.camera.pitch = glm::mix(g_devCamPitch, g_devCamPitch2, u);
        }
        return;
    }
    // Nearest enemy in sight, in front of us first
    const Enemy* best = nullptr; float bestScore = 1e9f;
    glm::vec3 fwd = player.camera.forward();
    for (auto& e : enemies) {
        if (!e.targetable()) continue;
        glm::vec3 c = e.position + glm::vec3{0, e.height() * 0.6f, 0};
        glm::vec3 d = c - p;
        float dist = glm::length(d);
        if (dist > 45.f || dist < 0.5f) continue;
        bool clear = true;
        for (auto& wl : level.walls) { float t = rayBoxHit(p, d / dist, wl.box); if (t > 0.f && t < dist - 0.6f) { clear = false; break; } }
        if (!clear) continue;
        float score = dist * (1.6f - glm::dot(fwd, d / dist));
        if (score < bestScore) { bestScore = score; best = &e; }
    }
    if (!best) return;
    glm::vec3 d = best->position + glm::vec3{0, best->height() * 0.6f, 0} - p;
    float wantYaw = glm::degrees(std::atan2(d.z, d.x));
    float wantPitch = glm::degrees(std::asin(glm::clamp(d.y / glm::length(d), -1.f, 1.f)));
    float k = 1.f - std::exp(-7.f * dt);
    float dy = std::remainder(wantYaw - player.camera.yaw, 360.f), dp = wantPitch - player.camera.pitch;
    player.camera.yaw += dy * k; player.camera.pitch += dp * k;
    devFireCd -= dt;
    if (std::fabs(dy) < 3.f && std::fabs(dp) < 3.f && devFireCd <= 0.f) { pendingFire = true; devFireCd = 0.3f; }
}

inline void GameplayState::devClearWave() {
    director.queue.clear();
    for (auto& e : enemies)
        if (e.alive) {
            e.alive = false; e.state = EnemyState::DEAD; e.health = 0.f;
            spawnDebrisFor(e);
        }
    ui.feed("DEV: WAVE CLEARED", {0.3f, 1.f, 0.8f});
}
