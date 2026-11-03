#pragma once
// =============================================================================
// Gameplay_Cold.h — GameplayState: Chapter 5, THE COLD STORE (LevelCold.h,
// docs/chapter-5-design.md). The planet's live parts: the long view (the
// camera sees 5 km here), the wind, the dispatch capsules firing up off the
// Store's towers toward the gas giant, and shards of ice rising out of the Void
// along the shelf's lip.
// Included at the end of GameplayState.h.
// =============================================================================

inline void GameplayState::beginCold() {
    storyOwned |= 15; storyArmed = storyGrapple = true;
    wakeT = -1; clawT = -1; beltBodies.clear(); player.eyeHeight = 1.7f;
    player.camera.farPlane = cold::FAR_PLANE;
    coldCapsules.clear(); coldLaunchT = 3.f; coldShardT = 0.f; coldLaunches = 0;
    player.camera.position = player.position + glm::vec3{0, player.eyeHeight, 0}; prevCamPos = player.camera.position;
}

// A capsule off tower `k`'s dispatch rail: straight up it, then curving away toward the gas giant
inline void GameplayState::launchColdCapsule(int k) {
    const auto T = cold::towers();
    const auto& t = T[k % T.size()];
    ColdCapsule c;
    c.pos = t.base + glm::vec3{t.half + 4.f, 30.f, 0.f};
    c.vel = {0.f, 40.f, 0.f};
    c.trailT = 0.f;
    coldCapsules.push_back(c);
    // Too far to hear where it is: a dull boom placed toward you, late and quiet
    glm::vec3 to = c.pos - player.position; to.y = 0.f;
    if (glm::length(to) > 1.f) audio.playAt("ab_takeoff", player.position + glm::normalize(to) * 70.f, 55, SoundGroup::WORLD, false, 0.55f);
    ++coldLaunches;
}

inline void GameplayState::updateCold(float dt) {
    player.camera.farPlane = cold::FAR_PLANE;   // (a retry makes a fresh camera)
    // Wind: always, gusting; stronger out at the lip
    const float lip = glm::clamp(1.f - (player.position.z - cold::edgeZ(player.position.x)) / 120.f, 0.f, 1.f);
    audio.setWind(playerDead ? 0.f : 0.36f + 0.1f * std::sin(gameClock * 0.17f) + 0.08f * std::sin(gameClock * 0.61f) + 0.2f * lip);

    // Dispatch launches: now and then, off a tower somewhere along the Store
    coldLaunchT -= dt;
    if (coldLaunchT <= 0.f) {
        coldLaunchT = 16.f + 14.f * cold::hash((float)coldLaunches, 501.f);
        launchColdCapsule((int)(cold::hash((float)coldLaunches, 502.f) * 15.f));
    }
    const glm::vec3 away = cold::giantDir();
    for (auto& c : coldCapsules) {
        c.age += dt;
        // Up the rail for two seconds, then it leans over toward the giant and keeps accelerating
        glm::vec3 want = c.age < 2.f ? glm::vec3{0.f, 1.f, 0.f} : glm::normalize(glm::vec3{away.x, 0.9f, away.z});
        float speed = glm::length(c.vel) + (60.f + 40.f * c.age) * dt;
        c.vel = glm::normalize(glm::mix(glm::normalize(c.vel), want, std::min(1.f, dt * 0.6f))) * speed;
        c.pos += c.vel * dt;
        c.trailT -= dt;
        if (c.trailT <= 0.f) { c.trailT = 0.2f; c.trail.push_back({c.pos, c.age}); }
    }
    for (auto& c : coldCapsules)   // the contrail thins and goes after half a minute
        c.trail.erase(std::remove_if(c.trail.begin(), c.trail.end(), [&](const ColdCapsule::Puff& p) { return c.age - p.born > 30.f; }), c.trail.end());
    coldCapsules.erase(std::remove_if(coldCapsules.begin(), coldCapsules.end(), [](const ColdCapsule& c) { return c.age > 45.f; }), coldCapsules.end());

    // Shards of ice drifting up out of the Void, near where you stand at the lip
    coldShardT -= dt;
    if (coldShardT <= 0.f && lip > 0.f) {
        coldShardT = 0.08f;
        float x = player.position.x + frand(-60.f, 60.f);
        glm::vec3 p{x, frand(-40.f, -4.f), cold::edgeZ(x) - frand(4.f, 50.f)};
        fx.spawnBurst(p, glm::vec3{0.7f, 0.82f, 0.95f}, 1, 0.4f, 5.f, -1.6f);
    }
}

// The capsules (a frosted pod, its engine burning amber) and their contrails
inline void GameplayState::gatherCold(std::vector<BoxInstance>& out) {
    using namespace rig;
    for (const auto& c : coldCapsules) {
        glm::vec3 d = glm::normalize(c.vel);
        glm::mat4 R = gleaner::alongZ(glm::vec3{0.f}, d);
        push(out, T(c.pos) * R * S(glm::vec3{4.f, 4.f, 11.f}), {0.55f, 0.6f, 0.68f});
        push(out, T(c.pos - d * 7.f) * R * S(glm::vec3{3.f, 3.f, 4.f}), {1.f, 0.7f, 0.3f}, glm::vec3{6.f, 3.f, 0.8f});
        for (size_t i = 1; i < c.trail.size(); ++i) {
            const auto& a = c.trail[i - 1]; const auto& b = c.trail[i];
            float age = c.age - b.born, w = 3.f + age * 1.6f;
            float fade = glm::clamp(1.f - age / 30.f, 0.f, 1.f);
            glm::vec3 seg = b.pos - a.pos;
            float len = glm::length(seg);
            if (len < 0.01f) continue;
            push(out, T((a.pos + b.pos) * 0.5f) * gleaner::alongZ(glm::vec3{0.f}, seg / len) * S(glm::vec3{w, w, len}),
                 glm::vec3{0.75f, 0.8f, 0.88f} * fade, glm::vec3{0.25f, 0.28f, 0.32f} * fade * fade);
        }
    }
}
