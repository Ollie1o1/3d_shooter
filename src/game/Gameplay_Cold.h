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
    if (!wakeSeen && director.arena == 0) pushBanner(level.story.title, "CHAPTER 5  /  NOTHING HERE IS THROWN AWAY", {0.75f, 0.86f, 0.95f}, 4.f);
    wakeSeen = true;
    // RECALL, in 24-A's chest
    Interactable it;
    it.position = cold::recallAt();
    it.box = {it.position - glm::vec3{0.7f, 1.3f, 0.7f}, it.position + glm::vec3{0.7f, 0.8f, 0.7f}};
    it.label = "TAKE THE RECALL MODULE FROM 24-A";
    it.active = !coldRecall;
    it.onInteract = [this]() { takeColdRecall(); };
    coldRecallItem = (int)interactables.size();
    interactables.push_back(std::move(it));
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

inline void GameplayState::takeColdRecall() {
    if (coldRecall) return;
    coldRecall = true;
    if (coldRecallItem >= 0 && coldRecallItem < (int)interactables.size()) interactables[coldRecallItem].active = false;
    const glm::vec3 at = cold::recallAt();
    fx.spawnBurst(at, RECALL_FROST, 30, 3.f, 0.8f, 2.f);
    audio.playAt("cyl_open", at, 120, SoundGroup::PLAYER, true, 0.8f);
    audio.play("upgrade", 110, SoundGroup::UI, true);
    pushBanner("RECALL", "PRESS " + binds().label(Act::RECALL) + ": LEAVE A STATUE  -  AGAIN: RETURN TO IT", RECALL_FROST, 4.f);
    ui.feed("24-A DOES NOT RESIST", RECALL_FROST);
}

inline void GameplayState::updateCold(float dt) {
    player.camera.farPlane = cold::FAR_PLANE;   // (a retry makes a fresh camera)
    // Sections: walking on is what moves the story on (the fights start at their triggers)
    const glm::vec3 p = player.position;
    int at = level.arenaAt(p);
    if (at > director.arena && !director.fighting() && director.phase != WaveDirector::Phase::INTRO) director.approach(at);
    if (at >= 0 && at > storySeenSection && !playerDead) {
        storySeenSection = at; pushBanner(level.arenas[at].name, level.arenas[at].subtitle, {0.75f, 0.86f, 0.95f}, 2.5f);
        audio.play("wave", 64, SoundGroup::UI, true);
    }
    if (at >= 0 && at > storyCheckpoint && !director.fighting()) { storyCheckpoint = at; ui.feed("RESTORE POINT", {0.75f, 0.86f, 0.95f}); }
    if (storyTerminal >= 0 && glm::length(level.story.terminals[storyTerminal].pos - p) > 6.f) storyTerminal = -1;
    // The end of the chapter so far: the shut door at the back of Rack 24, with RECALL in hand
    const AABB& E = level.story.end;
    if (coldRecall && storyEndT < 0.f && !playerDead && p.x > E.min.x && p.x < E.max.x && p.z > E.min.z && p.z < E.max.z) {
        storyEndT = 0.f; audio.play("wave", 90, SoundGroup::UI, true);
    }
    if (storyEndT >= 0.f) {
        storyEndT += dt; player.velocity = glm::vec3{0.f, std::max(0.f, player.velocity.y), 0.f};
        if (storyEndT > 3.f) captureMouse(false);
    }
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

// A stored unit, frosted over (its lights out unless `core` is lit)
inline void GameplayState::coldBody(std::vector<BoxInstance>& out, const char* model, const glm::mat4& at, const rig::FilePose& P, float frost, glm::vec3 core) {
    using namespace rig;
    const auto* M = story::bodyModel(model);
    if (!M) return;
    size_t from = out.size();
    Rig rg{out};
    FilePose Q = P;
    Q.chan[enemyfile::C_EYE] = core * 0.6f; Q.chan[enemyfile::C_CORE] = core;
    drawFile(rg, at, *M, Q, core);
    for (size_t i = from; i < out.size(); ++i) {
        out[i].color = glm::mix(out[i].color, RECALL_FROST, frost);
        out[i].emissive *= 1.f - 0.7f * frost * (glm::length(core) > 0.01f ? 0.f : 1.f);
        out[i].surface = 0.f;
    }
}

// The route's live parts: bodies on the Intake belt, units hung in the Thaw Room, the copies in
// Rack 24, a body slung under each Hook Line carrier
inline void GameplayState::gatherColdRoute(std::vector<BoxInstance>& out) {
    using namespace rig; using namespace enemyfile; using glm::vec3;
    const float t = gameClock;
    const glm::vec3 me = player.position;
    // Hung: arms down, head forward, feet together, swaying a little
    auto hung = [&](float sway) {
        FilePose P;
        P.motion[E_HEAD] = RX(0.55f); P.motion[E_ARML] = RZ(0.08f) * RX(sway); P.motion[E_ARMR] = RZ(-0.08f) * RX(-sway);
        P.motion[E_KNEE_L] = RX(0.15f); P.motion[E_KNEE_R] = RX(0.1f); P.motion[E_ANKLE_L] = RX(-0.4f); P.motion[E_ANKLE_R] = RX(-0.4f);
        return P;
    };
    // 1. the Intake belt: bodies on their backs riding north, tipping over the lip at its end
    if (glm::length(me - glm::vec3{cold::HOOK_X, 0.f, 0.f}) < 600.f) {
        const float z0 = 248.f, z1 = cold::lipZ() + 1.f, len = z0 - z1, speed = 0.9f, gap = 8.f;
        for (int k = 0; k < (int)(len / gap); ++k) {
            float u = std::fmod(t * speed + k * gap, len);
            float z = z0 - u, tip = glm::clamp((z1 + 2.5f - z) / 2.5f, 0.f, 1.f);
            if (glm::length(glm::vec2{me.x - cold::HOOK_X, me.z - z}) > 140.f) continue;
            FilePose P; P.motion[E_HEAD] = RY(0.3f * std::sin(k * 1.7f)); P.motion[E_ARML] = RZ(0.25f); P.motion[E_ARMR] = RZ(-0.2f);
            glm::mat4 M = T({cold::HOOK_X, 0.85f - 6.f * tip * tip, z}) * RX(-1.5708f * (1.f - 0.6f * tip)) * RY(3.1416f);
            coldBody(out, "unit_blank", M, P, 0.7f, vec3{0.f});
        }
    }
    // 2. the Hook Line: a body slung under each carrier, and the hanger up to the rail
    for (const auto& m : level.movers) {
        const AABB& b = level.walls[m.wall].box;
        vec3 c = (b.min + b.max) * 0.5f;
        if (glm::length(c - me) > 300.f) continue;
        push(out, T({c.x, (b.max.y + cold::RAIL_Y) * 0.5f, c.z}) * S(vec3{0.14f, cold::RAIL_Y - b.max.y, 0.14f}), {0.08f, 0.08f, 0.09f});
        push(out, T({c.x, cold::RAIL_Y - 0.2f, c.z}) * S(vec3{0.7f, 0.5f, 1.2f}), {0.1f, 0.1f, 0.11f}, vec3{0.6f, 0.3f, 0.08f});
        coldBody(out, "unit_blank", T({c.x, b.min.y - 2.3f, c.z}) * RY(1.2f), hung(0.1f * std::sin(t * 0.8f)), 0.75f, vec3{0.f});
    }
    if (me.z > cold::TOWER_FRONT + 80.f) return;   // (the tower's insides: only near it)
    // 3. the Thaw Room's hooks: all hung until its fight starts; then only the ones that stayed
    const bool dropped = director.arena > 2 || (director.arena == 2 && director.phase != WaveDirector::Phase::APPROACH);
    auto H = cold::thawHooks();
    for (int k = 0; k < (int)H.size(); ++k) {
        if (dropped && k < cold::THAW_SPAWNS) continue;
        float sway = 0.05f * std::sin(t * 0.7f + k);
        coldBody(out, "unit_blank", T(H[k] + vec3{0.f, 2.75f, 0.f}) * RY(0.4f * k) * RZ(sway), hung(sway), 0.8f, vec3{0.f});
    }
    // 4. Rack 24: ten of him, frosted, facing the aisle; a few turn their heads to follow you
    for (int k = 0; k < 10; ++k) {
        vec3 p = cold::rackSlot(k) + vec3{0.f, 0.25f, 0.f};
        float yaw = k % 2 == 0 ? 1.5708f : -1.5708f;
        FilePose P;
        if (k == 2 || k == 5 || k == 7 || k == 8) {
            vec3 d = me - p;
            float want = std::atan2(d.x, d.z) - yaw;
            want = std::remainder(want, 6.2832f);
            P.motion[E_HEAD] = RY(glm::clamp(want, -1.1f, 1.1f));
        }
        vec3 core{0.f};
        if (k == 0) core = coldRecall ? vec3{0.f} : RECALL_FROST * (0.8f + 0.4f * std::sin(t * 3.f));   // 24-A: the module in its chest
        coldBody(out, "unit24", T(p) * RY(yaw), P, k == 0 && !coldRecall ? 0.45f : 0.7f, core);
    }
}

// The capsules (a frosted pod, its engine burning amber) and their contrails
inline void GameplayState::gatherCold(std::vector<BoxInstance>& out) {
    using namespace rig;
    gatherColdRoute(out);
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
