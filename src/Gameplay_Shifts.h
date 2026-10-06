#pragma once
// =============================================================================
// Gameplay_Shifts.h — GameplayState: arenas that change as the fight goes on
// (ArenaShifts.h): the lava and overload rings' damage, sounds and banners,
// and drawing the widened lava and the rings.
// Included at the end of GameplayState.h.
// =============================================================================

inline void GameplayState::updateShifts(float dt) {
    shifts.update(dt, level, director.arena, director.fighting() && !playerDead);
    if (shifts.lavaStarted)
        pushBanner("THE LAVA IS RISING", "GET TO THE CATWALKS - OR LURE THEM IN", {1.f, 0.45f, 0.1f}, 2.6f);
    if (shifts.floodStarted) {
        pushBanner("THE WATER RISES", "GET TO HIGH GROUND - OR SLIDE", {0.35f, 0.9f, 0.95f}, 2.6f);
        audio.play("explosion", 45);
        shake(0.5f, 0.03f);
    }
    // The overload: a tick as the reactor winds up, a boom as a ring goes out
    float warn = shifts.warning();
    if (warn > 0.f && pulseWarnCued == false) { audio.play("telegraph", 110); pulseWarnCued = true; }
    if (shifts.pulseFired) {
        pulseWarnCued = false;
        audio.play("slam", 110);
        shake(0.25f, 0.05f);
    }
    if (shifts.rings.empty()) return;
    // A ring sweeping under your feet: jump it, or be up on a walkway
    if (shifts.ringHits(player.position) &&
        damagePlayer(ArenaShifts::PULSE_DAMAGE * tune().damage, level.reactorPos, 0.3f, 0.07f))
        player.velocity.y = std::max(player.velocity.y, 6.f);
    for (auto& e : enemies)
        if (e.targetable() && !e.stats().flying && !isBoss(e.type) && e.type != EnemyType::CONDUIT &&
            shifts.ringHits(e.position))
            hurtEnemy(e, ArenaShifts::PULSE_DAMAGE * 2.f, e.position + glm::vec3{0, e.height() * 0.5f, 0}, 0.f, 0.f,
                      StyleSource::ENVIRONMENT);
}

inline void GameplayState::gatherShiftBoxes(std::vector<BoxInstance>& out) {
    using namespace rig;
    float t = gameClock;
    // The widened lava: a glowing slab over each channel as it spreads
    for (int a = 0; a < (int)level.arenas.size(); ++a) {
        if (level.arenas[a].shift != ArenaShift::LAVA_RISE || shifts.lava[a] <= 0.f) continue;
        for (auto& hz : level.hazards) {
            glm::vec3 c = (hz.box.min + hz.box.max) * 0.5f, sz = hz.box.max - hz.box.min;
            const AABB& z = level.arenas[a].zone;
            if (c.x < z.min.x || c.x > z.max.x || c.z < z.min.z || c.z > z.max.z) continue;
            float glow = 0.9f + 0.2f * std::sin(t * 2.3f + c.z);
            push(out, T(c) * S(sz), {0.8f, 0.2f, 0.03f}, glm::vec3{1.f, 0.2f, 0.02f} * glow);
        }
    }
    // Overload rings: a band of light racing out along the floor
    glm::vec3 col{1.6f, 0.35f, 0.15f};
    for (auto& r : shifts.rings) {
        float fade = 1.f - r.radius / ArenaShifts::PULSE_RANGE;
        int seg = (int)glm::clamp(r.radius * 2.4f, 24.f, 110.f);
        float segLen = 6.2832f * r.radius / seg;
        for (int k = 0; k < seg; ++k) {
            float ang = (k + 0.5f) / seg * 6.2832f;
            glm::vec3 p = r.centre + glm::vec3{std::cos(ang) * r.radius, ArenaShifts::PULSE_HEIGHT * 0.5f, std::sin(ang) * r.radius};
            push(out, T(p) * RY(-ang) * S({0.22f, ArenaShifts::PULSE_HEIGHT, segLen * 0.9f}), col * 0.3f, col * (1.2f + 1.5f * fade));
        }
    }
}
