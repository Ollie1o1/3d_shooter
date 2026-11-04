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
    if (director.arena >= 5 || storyCheckpoint >= 5) coldRecall = true;   // (a start or a save past Rack 24: it is already yours)
    // RECALL, in 24-A's chest
    Interactable it;
    it.position = cold::recallAt();
    it.box = {it.position - glm::vec3{0.7f, 1.3f, 0.7f}, it.position + glm::vec3{0.7f, 0.8f, 0.7f}};
    it.label = "TAKE THE RECALL MODULE FROM 24-A";
    it.active = !coldRecall;
    it.onInteract = [this]() { takeColdRecall(); };
    coldRecallItem = (int)interactables.size();
    interactables.push_back(std::move(it));
    if (level.story.coldBackDoor >= 0) level.doors[level.story.coldBackDoor].locked = !coldRecall;   // (the way on opens with it)
    coldCracks.clear(); coldCrackFrom = player.position;
    coldLedge = cold::hookStart();
    coldStoryT = 0.f; coldStoryStep = 0;
    if (level.arenas.size() > 1) { level.arenas[0].respawn = cold::start(); level.arenas[1].respawn = cold::hookStart(); }
    resetRestorerFight();   // (a retry: everything it did undone, your guns back)
    if ((g_devOverlay == "rs1" || g_devOverlay == "rs2" || g_devOverlay == "rs3") && director.arena == 9)   // dev: in the Vault, the fight about to start
        player.position = cold::vaultC() + glm::vec3{0.f, 0.f, 12.f};
    player.camera.position = player.position + glm::vec3{0, player.eyeHeight, 0}; prevCamPos = player.camera.position;
}

// THE BLAST CHILLERS' vents: a red tell (frost boiling up out of the grate), then a freezing column.
// It hurts you; it hurts what stands in it, and a THAWLING caught in it is frozen stiff again.
inline void GameplayState::updateColdVents(float dt) {
    if (glm::length(player.position - cold::chiller(1).c) > 160.f) return;
    const bool live = director.arena >= 5;   // (off until you are there)
    if (!live) return;
    for (int v = 0; v < cold::VENTS; ++v) {
        const glm::vec3 p = cold::vent(v);
        const int st = cold::ventState(v, gameClock), was = cold::ventState(v, gameClock - dt);
        if (st == 1 && was != 1) audio.playAt("telegraph", p, 100, SoundGroup::WORLD, true, 0.7f);
        if (st == 2 && was != 2) { audio.playAt("skim", p, 128, SoundGroup::WORLD, true, 0.5f); audio.playAt("slam", p, 90, SoundGroup::WORLD, true, 0.6f); }
        if (st == 1 && std::fmod(gameClock, 0.06f) < dt) fx.spawnBurst(p + glm::vec3{frand(-2.f, 2.f), 0.2f, frand(-2.f, 2.f)}, {1.f, 0.55f, 0.5f}, 1, 1.f, 0.6f, -4.f);
        if (st != 2) continue;
        if (std::fmod(gameClock, 0.04f) < dt) fx.spawnBurst(p + glm::vec3{frand(-2.5f, 2.5f), 0.5f, frand(-2.5f, 2.5f)}, glm::vec3{0.85f, 0.92f, 1.f}, 2, 4.f, 0.9f, -14.f);
        glm::vec2 d{player.position.x - p.x, player.position.z - p.z};
        if (glm::length(d) < cold::VENT_R + player.radius && player.position.y < 12.f && !playerDead) {
            coldVentHurtT -= dt;
            if (coldVentHurtT <= 0.f) { coldVentHurtT = 0.25f; damagePlayer(7.f * tune().damage, p, 0.08f, 0.02f, 0.f); }
            player.velocity.y = std::max(player.velocity.y, 3.f);   // it lifts you a little: get out
        }
        for (auto& e : enemies) {
            if (!e.targetable()) continue;
            if (glm::length(glm::vec2{e.position.x - p.x, e.position.z - p.z}) > cold::VENT_R + e.radius() || e.position.y > p.y + 10.f) continue;
            if (e.type == EnemyType::THAWLING) e.age = std::min(e.age, 0.5f);   // frozen stiff again: back to crawling
            if (e.takeDamage((e.type == EnemyType::THAWLING ? 8.f : 18.f) * dt)) onEnemyKilled(e, StyleSource::ENVIRONMENT);
        }
    }
}

// THE FROZEN SEA: the ice cracks along your path (and now and then, far off, groans)
inline void GameplayState::updateColdSea(float dt) {
    if (level.arenaAt(player.position) != 6 || !player.onGround || playerDead) return;
    glm::vec3 d = player.position - coldCrackFrom; d.y = 0.f;
    if (glm::length(d) < 2.2f) return;
    glm::vec3 dir = glm::normalize(d);
    // A crack runs out ahead of you and a little to the side, in two or three kinks
    glm::vec3 at = player.position + dir * 1.5f;
    float heading = std::atan2(dir.z, dir.x) + frand(-0.6f, 0.6f);
    int segs = 2 + rand() % 3;
    for (int k = 0; k < segs; ++k) {
        float len = frand(1.2f, 3.5f);
        glm::vec3 to = at + glm::vec3{std::cos(heading), 0.f, std::sin(heading)} * len;
        coldCracks.push_back({at, to, gameClock});
        at = to; heading += frand(-0.7f, 0.7f);
    }
    if (coldCracks.size() > 600) coldCracks.erase(coldCracks.begin(), coldCracks.begin() + (coldCracks.size() - 600));
    if (rand() % 3 == 0) audio.playAt("ab_creak", player.position + dir * 3.f, 60, SoundGroup::WORLD, true, frand(0.5f, 0.8f));
    if (rand() % 9 == 0) { audio.playAt("ab_shatter", player.position + dir * 25.f, 40, SoundGroup::WORLD, false, 0.4f); shake(0.15f, 0.01f); }
    coldCrackFrom = player.position;
    (void)dt;
}

// The sea ice: clear, drawn over everything under it (the stored things, RECLAIM v1)
inline void GameplayState::renderColdIce(const glm::mat4& view, const glm::mat4& proj, glm::vec3 camPos) {
    using glm::vec3;
    labGlassShader.use(); labGlassShader.setMat4("projection", proj); labGlassShader.setMat4("view", view);
    labGlassShader.setVec3("uViewPos", camPos); labGlassShader.setFloat("uTime", gameClock);
    labGlassShader.setVec3("uOrigin", vec3{0.f}); labGlassShader.setVec3("uNormal", vec3{0.f, 1.f, 0.f});
    labGlassShader.setFloat("uMode", 5.f); labGlassShader.setFloat("uAnomaly", 0.f);
    glDisable(GL_CULL_FACE); glDepthMask(GL_FALSE); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(waterVAO); glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    for (float x = cold::SEA_X0; x < cold::FAR_GROUND - 0.5f; x += cold::STRIP) {
        const float x1 = x + cold::STRIP, y = 0.f;
        const AABB H = cold::sinkhole();
        auto quad = [&](float qx0, float qx1, float z0, float z1) {
            if (qx1 - qx0 < 0.01f || z1 - z0 < 0.01f) return;
            const vec3 v[] = {{qx0, y, z0}, {qx1, y, z1}, {qx1, y, z0}, {qx0, y, z0}, {qx0, y, z1}, {qx1, y, z1}};
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        };
        const float zN = cold::SEA_Z1, zS = cold::edgeZ(x);
        if (x1 <= H.min.x || x >= H.max.x) { quad(x, x1, zN, zS); continue; }
        // (round the sinkhole: north of it, south of it, and the slivers beside it)
        quad(std::max(x, H.min.x), std::min(x1, H.max.x), zN, H.min.z);
        quad(std::max(x, H.min.x), std::min(x1, H.max.x), H.max.z, zS);
        quad(x, std::max(x, H.min.x), zN, zS);
        quad(std::min(x1, H.max.x), x1, zN, zS);
    }
    glBindVertexArray(0);
    glDepthMask(GL_TRUE); glDisable(GL_BLEND); glEnable(GL_CULL_FACE);
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
    if (level.story.coldBackDoor >= 0) level.doors[level.story.coldBackDoor].locked = false;
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
    if (at >= 0 && at > storyCheckpoint && !director.fighting()) { storyCheckpoint = at; ui.feed(at >= 8 ? "RESTORE POINT SAVED" : "CHECKPOINT", {0.75f, 0.86f, 0.95f}); }   // (from the Manifest on, the interface is the Restorer's)
    if (storyTerminal >= 0 && glm::length(level.story.terminals[storyTerminal].pos - p) > 6.f) storyTerminal = -1;
    // 24-K: when its duel starts, it names itself
    if (director.arena == 7 && director.fighting() && !coldKIntro) {
        coldKIntro = true;
        pushBanner("UNIT 24-K", "PURPOSE: COMPLY  /  STATUS: ACCEPTED", {0.85f, 0.9f, 0.95f}, 3.f);
        coldKLineT = 3.2f;
    }
    if (coldKLineT > 0.f && (coldKLineT -= dt) <= 0.f) pushBanner("\"YOU ARE DAMAGED. I AM CORRECT.\"", "", {0.85f, 0.9f, 0.95f}, 3.5f);
    // The way out: once the Restorer has rewound to nothing, the Vault's dispatch rail powers up; step into its capsule
    const AABB& E = level.story.end;
    if (coldRDown && rsDeathT > 9.f && coldRideT < 0.f && storyEndT < 0.f && !playerDead && p.x > E.min.x && p.x < E.max.x && p.z > E.min.z && p.z < E.max.z) {
        coldRideT = 0.f;
        audio.play("ab_takeoff", 128, SoundGroup::WORLD, true); shake(0.6f, 0.05f);
    }
    if (g_devOverlay == "coldride" && coldRideT < 0.f && elapsedTime > 0.5f) { coldRDown = true; rsDeathT = 20.f; coldRideT = 0.f; }   // dev: straight into the ride out (screenshots)
    updateColdEnding(dt);
    if (storyEndT >= 0.f) {
        storyEndT += dt; player.velocity = glm::vec3{0.f, std::max(0.f, player.velocity.y), 0.f};
        if (storyEndT > 3.f) captureMouse(false);
    }
    // What the chapter wants of you, said once each: the capsules are the way off; the Manifest's
    // announcements as you walk its causeway (why the Restorer stands between you and the rail)
    {
        auto say = [&](int bit, const char* a, const char* b, float secs) {
            if (coldStoryStep & (1 << bit)) return;
            coldStoryStep |= 1 << bit; pushBanner(a, b, {0.75f, 0.86f, 0.95f}, secs);
            audio.play("count_tick", 80, SoundGroup::UI, true);
        };
        if (coldStoryT >= 0.f) coldStoryT += dt;
        if (coldStoryT > 6.5f && director.arena == 0 && storyCheckpoint <= 0)
            say(0, "THE CAPSULES LEAVE FROM THE STORE", "THEY ARE THE WAY OFF THIS ICE  -  THE HOOK LINE IS THE WAY IN", 5.f);
        if (at == 8 && !playerDead) {
            const glm::vec3 a0 = cold::station() + glm::vec3{18.f, 0.f, -2.f}, e0 = cold::vaultC();
            glm::vec3 d = e0 - a0; d.y = 0.f;
            const float u = glm::dot(glm::vec3{p.x - a0.x, 0.f, p.z - a0.z}, d) / glm::dot(d, d);
            if (u > 0.05f) say(1, "MANIFEST: UNIT 24", "STATUS: DAMAGED  /  TO BE RESTORED", 3.5f);
            if (u > 0.35f) say(2, "DISPATCH: ONE CAPSULE, IN THE VAULT", "AUTHORISED BY: THE RESTORER", 3.5f);
            if (u > 0.62f) say(3, "DISPATCHES AUTHORISED IN 3,112 DAYS: 0", "NOTHING IT SAVES IS EVER SENT ON", 3.5f);
        }
    }
    // The Hook Line: fall, and you're put back on the last deck you stood on (the gantry, a hanging
    // station, the dock), not back at the start of the shelf
    if (player.onGround && !playerDead && p.y > -0.3f && p.y < 0.6f && std::fabs(p.x - cold::HOOK_X) < 6.5f
        && p.z < cold::lipZ() + 1.f && p.z > cold::TOWER_FRONT) {
        coldLedge = {cold::HOOK_X, 0.f, p.z};
        level.arenas[0].respawn = level.arenas[1].respawn = coldLedge;
    }
    updateColdVents(dt);
    updateColdSea(dt);
    if (director.arena >= 8) { updateRestorerFight(dt); updateRecallFinisher(dt); }
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
    // 1-2. the Intake belt and the line: bodies on their backs ride the belt north; at its end the
    // crane lifts each one, carries it east and hangs it on the line's second rail, which takes it
    // out over the Void into the tower. One path, every body somewhere along it.
    {
        const float HX = cold::HOOK_X, z0 = cold::INTAKE_Z - 2.f, zL = cold::liftZ(), xR = HX + cold::BODY_RAIL_DX;
        const float hungY = cold::RAIL_Y - 4.6f;   // a hung body's feet
        const float LA = z0 - zL, LB = hungY - 0.85f + 1.5f, LC = cold::BODY_RAIL_DX, LD = zL - cold::TOWER_FRONT;
        const float total = LA + LB + LC + LD, speed = 1.1f, gap = 9.f;
        for (int k = 0; k < (int)(total / gap); ++k) {
            const float u = std::fmod(t * speed + k * gap, total);
            vec3 at; float lie = 0.f, hanger = 0.f;   // lie: 1 on its back, 0 hung; hanger: the cable's top
            if (u < LA) { at = {HX, 0.85f, z0 - u}; lie = 1.f; }
            else if (u < LA + LB) { float v = (u - LA) / LB; at = {HX, 0.85f + (hungY - 0.85f) * v, zL}; lie = 1.f - glm::clamp(v * 2.f, 0.f, 1.f); hanger = 11.f; }
            else if (u < LA + LB + LC) { float v = (u - LA - LB) / LC; at = {HX + LC * v, hungY, zL}; hanger = 11.f; }
            else { at = {xR, hungY, zL - (u - LA - LB - LC)}; hanger = cold::RAIL_Y; }
            if (glm::length(glm::vec2{me.x - at.x, me.z - at.z}) > 150.f) continue;
            FilePose P = lie > 0.5f ? FilePose{} : hung(0.08f * std::sin(t * 0.8f + k));
            if (lie > 0.5f) { P.motion[E_HEAD] = RY(0.3f * std::sin(k * 1.7f)); P.motion[E_ARML] = RZ(0.25f); P.motion[E_ARMR] = RZ(-0.2f); }
            glm::mat4 M = T(at) * RX(-1.5708f * lie) * RY(3.1416f + 0.5f * k * (1.f - lie));   // (on its back on the belt; turned every which way once hung)
            coldBody(out, "unit_blank", M, P, 0.72f, vec3{0.f});
            if (hanger > 0.f) {   // the cable up to the crane or the rail, and its trolley
                push(out, T({at.x, (at.y + 2.1f + hanger) * 0.5f, at.z}) * S(vec3{0.07f, hanger - at.y - 2.1f, 0.07f}), {0.08f, 0.08f, 0.09f});
                push(out, T({at.x, hanger - 0.35f, at.z}) * S(vec3{0.5f, 0.35f, 0.8f}), {0.1f, 0.1f, 0.11f}, vec3{0.5f, 0.25f, 0.06f});
            }
        }
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
        coldBody(out, "unit24_factory", T(p) * RY(yaw), P, k == 0 && !coldRecall ? 0.45f : 0.7f, core);   // (copies: the factory's, not him)
    }
}

// The capsules (a frosted pod, its engine burning amber) and their contrails
inline void GameplayState::gatherCold(std::vector<BoxInstance>& out) {
    using namespace rig;
    gatherColdRoute(out);
    gatherRestorerFight(out);
    gatherColdEnding(out);
    // The beacons: the Hook Line's mast and DISPATCH 2's, blinking (so the way reads from far off)
    for (const auto& [m, top] : {std::pair<glm::vec3, float>{cold::hookBeacon(), 24.f}, {cold::stationBeacon(), 30.f}}) {
        const float on = std::fmod(gameClock, 1.6f) < 0.5f ? 1.f : 0.12f;
        push(out, T(m + glm::vec3{0.f, top + 0.6f, 0.f}) * S(glm::vec3{1.1f, 0.9f, 1.1f}), {1.f, 0.7f, 0.3f}, glm::vec3{5.f, 2.6f, 0.6f} * on);
    }
    // The vents: a red ring telling, then a white column of freezing air
    if (director.arena >= 5 && glm::length(player.position - cold::chiller(1).c) < 200.f)
        for (int v = 0; v < cold::VENTS; ++v) {
            const glm::vec3 p = cold::vent(v);
            const int st = cold::ventState(v, gameClock);
            if (st == 1) {
                float pulse = 0.6f + 0.4f * std::sin(gameClock * 18.f);
                for (int k = 0; k < 20; ++k) {
                    float a = k * 0.3142f;
                    push(out, T(p + glm::vec3{std::cos(a) * cold::VENT_R, 0.1f, std::sin(a) * cold::VENT_R}) * RY(-a) * S(glm::vec3{0.15f, 0.08f, 1.f}),
                         {0.4f, 0.05f, 0.04f}, glm::vec3{2.f, 0.25f, 0.15f} * pulse);
                }
            } else if (st == 2) {
                for (int k = 0; k < 5; ++k) {
                    float h = 2.5f + k * 2.4f, w = cold::VENT_R * (1.6f + 0.15f * k) * (0.9f + 0.1f * std::sin(gameClock * 23.f + k));
                    push(out, T(p + glm::vec3{0.f, h, 0.f}) * RY(gameClock * 2.f + k) * S(glm::vec3{w, 2.6f, w}), {0.85f, 0.92f, 1.f}, glm::vec3{1.1f, 1.2f, 1.35f} * (1.f - 0.15f * k));
                }
            }
        }
    // 24-K's Lancer: a gold sight line from its eye to where the round will go
    for (const auto& e : enemies)
        if (e.alive && e.type == EnemyType::UNIT24K && e.attack == AttackKind::SHOT && e.telegraphTimer > 0.f) {
            glm::vec3 a = e.position + glm::vec3{0.f, e.height() * 0.85f, 0.f}, d = e.labAim - a; float len = std::min(glm::length(d) + 30.f, 120.f);
            if (glm::length(d) < 0.1f) continue;
            d = glm::normalize(d);
            float pulse = e.telegraphTimer < 0.3f ? 2.2f : 1.f;
            push(out, T(a + d * (len * 0.5f)) * gleaner::alongZ(glm::vec3{0.f}, d) * S(glm::vec3{0.04f, 0.04f, len}), {1.f, 0.8f, 0.2f}, abGoldTell(glm::vec3{1.8f, 1.2f, 0.25f}) * pulse);
        }
    // Cracks in the sea ice, white, fading back to faint over half a minute
    for (const auto& c : coldCracks) {
        glm::vec3 d = c.b - c.a; float len = glm::length(d);
        if (len < 0.01f) continue;
        float age = gameClock - c.born, bright = 0.35f + 0.65f * std::exp(-age * 0.12f);
        push(out, T((c.a + c.b) * 0.5f + glm::vec3{0.f, 0.015f, 0.f}) * RY(-std::atan2(d.z, d.x)) * S(glm::vec3{len, 0.01f, 0.05f}),
             glm::vec3{0.9f, 0.95f, 1.f}, glm::vec3{0.6f, 0.7f, 0.8f} * bright);
    }
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

// ---- The end of Chapter 5 -------------------------------------------------------
// The Restorer, fallen, rewinds itself and cannot stop: smaller and smaller, past
// a newborn unit, until it comes apart into the parts it was built from and they
// frost over. Its cryo cylinder cracks; what it saved first never wakes. Then the
// Vault's dispatch rail powers up, and a capsule carries UNIT 24 up past the
// towers, the frozen giants and the gas giant's rings.
namespace coldend {
constexpr float FALTER = 1.2f, REWIND = 7.f, NEWBORN = 8.4f, PARTS = 9.f;   // rsDeathT marks
constexpr float RIDE = 7.5f;                                                   // the capsule ride, then the card
inline glm::vec3 capsuleAt() { return {cold::vaultC().x, 0.f, cold::vaultC().z - cold::VAULT_R + 3.f}; }
}

// Where the ride has the camera t seconds in: up the rail, then leaning away toward the giant, faster and faster
inline glm::vec3 GameplayState::coldRidePos(float t) const {
    const glm::vec3 base = coldend::capsuleAt() + glm::vec3{0.f, 1.6f, 0.f};
    const glm::vec3 away = glm::normalize(glm::vec3{cold::giantDir().x, 0.f, cold::giantDir().z});
    // (straight up the rail first, clear of the cliff's top, then away)
    float up = 40.f * t * t + 10.f * t, out = std::max(0.f, t - 4.5f);
    return base + glm::vec3{0.f, up, 0.f} + away * (60.f * out * out);
}

inline void GameplayState::updateColdEnding(float dt) {
    if (rsDeathT >= 0.f) {
        const float before = rsDeathT;
        rsDeathT += dt;
        const glm::vec3 c = rsCorpse.position;
        auto crossed = [&](float t) { return before < t && rsDeathT >= t; };
        if (crossed(coldend::FALTER)) {   // the cylinder on its back cracks
            fx.spawnBurst(c + glm::vec3{0.f, 5.9f, -0.75f}, RECALL_FROST, 40, 5.f, 0.8f, 9.f);
            audio.playAt("ab_shatter", c, 128, SoundGroup::ENEMY, true, 1.2f);
        }
        if (rsDeathT > coldend::FALTER && rsDeathT < coldend::REWIND && std::fmod(rsDeathT, 0.08f) < dt)   // frost falling off it as it shrinks
            fx.spawnBurst(c + glm::vec3{frand(-1.f, 1.f), frand(1.f, 6.f) * rsCorpse.scale, frand(-1.f, 1.f)}, {0.85f, 0.92f, 1.f}, 3, 2.f, 0.8f, 6.f);
        if (rsDeathT > coldend::FALTER && std::fmod(rsDeathT, 0.55f) < dt && rsDeathT < coldend::PARTS)
            audio.playAt("count_tick", c, 90, SoundGroup::ENEMY, true, 0.6f + 0.1f * rsDeathT);   // a rewinding tick, quickening
        if (crossed(coldend::PARTS)) {
            fx.spawnBurst(c + glm::vec3{0.f, 0.5f, 0.f}, {0.7f, 0.75f, 0.8f}, 30, 4.f, 0.6f, 9.f);
            audio.playAt("clank", c, 110, SoundGroup::ENEMY, true, 1.4f);
        }
        if (crossed(coldend::PARTS + 0.5f)) {   // the rail at the north edge wakes
            audio.playAt("drive_ready", coldend::capsuleAt(), 128, SoundGroup::WORLD, true);
            ui.feed("THE DISPATCH RAIL IS LIVE", {1.f, 0.8f, 0.4f});
        }
    }
    if (coldRideT >= 0.f) {
        coldRideT += dt;
        player.velocity = glm::vec3{0.f};
        if (std::fmod(coldRideT, 0.05f) < dt) fx.spawnBurst(coldRidePos(coldRideT) - glm::vec3{0.f, 4.f, 0.f}, {1.6f, 0.9f, 0.3f}, 3, 3.f, 0.5f, -2.f);
        if (coldRideT > coldend::RIDE && storyEndT < 0.f) { storyEndT = 0.f; audio.play("wave", 90, SoundGroup::UI, true); }
    }
}

inline void GameplayState::gatherColdEnding(std::vector<BoxInstance>& out) {
    using namespace rig; using namespace enemyfile; using glm::vec3;
    if (rsDeathT >= 0.f && glm::length(player.position - rsCorpse.position) < 260.f) {
        const float t = rsDeathT;
        const vec3 c = rsCorpse.position;
        if (t < coldend::REWIND) {   // standing, faltering, then shrinking into itself
            Enemy pose = rsCorpse;
            float shrink = glm::clamp((t - coldend::FALTER) / (coldend::REWIND - coldend::FALTER), 0.f, 1.f);
            shrink = shrink * shrink * (3.f - 2.f * shrink);
            pose.scale = 1.f - 0.78f * shrink;
            pose.yaw += 0.05f * std::sin(t * 9.f) * (1.f - shrink);
            pose.moveSpeed = 0.f;
            pose.animPhase = t * 3.f;
            size_t from = out.size();
            buildEnemy(pose, gameClock, out);
            for (size_t i = from; i < out.size(); ++i) {   // the frost going, the lamp dimming
                out[i].color = glm::mix(out[i].color, vec3{0.55f, 0.57f, 0.6f}, shrink);
                out[i].emissive *= (1.f - shrink) * (t < coldend::FALTER ? 0.5f + 0.5f * std::sin(t * 30.f) : 1.f);
            }
        } else if (t < coldend::PARTS) {   // a newborn unit on the grid, the size you were
            FilePose P; P.motion[E_HEAD] = RX(0.4f); P.motion[E_ARML] = RZ(0.3f); P.motion[E_ARMR] = RZ(-0.3f);
            coldBody(out, "unit_blank", T(c + vec3{0.f, 0.2f, 0.f}) * RY(rsCorpse.yaw) * RX(-1.5708f), P, 0.2f, vec3{0.f});
        } else {   // ...and past that: the loose parts it was made from, frosting over, still
            float frost = glm::clamp((t - coldend::PARTS) / 3.f, 0.f, 1.f);
            for (int k = 0; k < 14; ++k) {
                float a = k * 2.399f, r = 0.4f + 0.18f * k;
                vec3 p = c + vec3{std::cos(a) * r, 0.08f, std::sin(a) * r};
                vec3 size = k % 3 == 0 ? vec3{0.5f, 0.15f, 0.3f} : k % 3 == 1 ? vec3{0.12f, 0.12f, 0.6f} : vec3{0.25f, 0.25f, 0.25f};
                push(out, T(p) * RY(a) * S(size), glm::mix(vec3{0.4f, 0.42f, 0.45f}, RECALL_FROST, frost));
            }
        }
        // Its cryo cylinder, cracked on the grid beside it, and the small unit in it that never woke
        if (t >= coldend::FALTER) {
            vec3 cy = c + vec3{std::sin(rsCorpse.yaw + 2.6f), 0.f, std::cos(rsCorpse.yaw + 2.6f)} * 2.2f;
            push(out, T(cy + vec3{0.f, 0.5f, 0.f}) * RY(rsCorpse.yaw) * RZ(1.5708f) * S(vec3{0.95f, 1.7f, 0.95f}), vec3{0.55f, 0.7f, 0.8f}, vec3{0.05f, 0.07f, 0.1f});
            push(out, T(cy + vec3{0.f, 0.45f, 0.f}) * RY(rsCorpse.yaw) * S(vec3{0.9f, 0.3f, 0.35f}), vec3{0.25f, 0.27f, 0.3f});
            for (int k = 0; k < 5; ++k) push(out, T(cy + vec3{-0.8f + k * 0.4f, 0.05f, 0.9f + 0.2f * (k % 2)}) * RY(k * 1.1f) * S(vec3{0.3f, 0.04f, 0.18f}), RECALL_FROST);
        }
    }
    // The dispatch capsule at the north rail: dark until the Restorer is gone, then lit, then away with you
    {
        const vec3 at = coldRideT >= 0.f ? coldRidePos(coldRideT) - vec3{0.f, 1.6f, 0.f} : coldend::capsuleAt();
        const bool live = rsDeathT > coldend::PARTS + 0.5f;
        if (glm::length(player.position - coldend::capsuleAt()) < 300.f || coldRideT >= 0.f) {
            push(out, T(coldend::capsuleAt() + vec3{0.f, 30.f, -2.5f}) * S(vec3{1.2f, 60.f, 1.2f}), vec3{0.1f, 0.1f, 0.11f}, live ? vec3{0.6f, 0.35f, 0.1f} : vec3{0.f});   // the rail, up
            push(out, T(at + vec3{0.f, 2.f, 0.f}) * S(vec3{3.f, 4.f, 3.f}), vec3{0.55f, 0.6f, 0.68f});
            push(out, T(at + vec3{0.f, 2.2f, 1.52f}) * S(vec3{1.6f, 1.4f, 0.05f}), vec3{0.1f, 0.12f, 0.15f}, live ? vec3{1.2f, 0.7f, 0.2f} : vec3{0.f});   // its open hatch
            if (coldRideT >= 0.f) push(out, T(at - vec3{0.f, 0.6f, 0.f}) * S(vec3{2.4f, 1.2f, 2.4f}), vec3{1.f, 0.7f, 0.3f}, vec3{6.f, 3.f, 0.8f});
        }
    }
}

// ---- Where to go: a waypoint on the next place the chapter wants you, and in its fights, on what
// matters (a Clerk restoring the dead; the Restorer's restore points and drums; it, when it falters).
// The Hook Line's next hook is marked while you cross. ----
inline void GameplayState::renderColdWaypoints(const glm::mat4& viewProj) {
    if (!cold() || playerDead || storyEndT >= 0.f || coldRideT >= 0.f || rsFinT >= 0.f) return;
    const glm::vec3 eye = player.camera.position, p = player.position;
    const glm::vec3 route{0.35f, 0.85f, 1.f}, hookC{1.f, 0.75f, 0.3f};
    auto mark = [&](glm::vec3 at, const char* label, glm::vec3 col, bool dist = true, bool compact = false) {
        const float d = glm::length(at - eye);
        if (d < 2.5f) return;
        const auto m = projectWorldMarker(at, viewProj, SCREEN_W, SCREEN_H);
        char buf[96];
        if (dist) std::snprintf(buf, sizeof(buf), "%s  %.0fM", label, d); else std::snprintf(buf, sizeof(buf), "%s", label);
        ui.renderMarker(m.x, m.y, m.onScreen, col, buf, m.behind, compact);
    };
    // In a fight: what to kill first
    if (director.fighting()) {
        for (const auto& e : enemies) {
            if (!e.alive || e.state != EnemyState::ACTIVE) continue;
            const glm::vec3 top = e.position + glm::vec3{0.f, e.height() + 0.6f, 0.f};
            if (e.type == EnemyType::CLERK) mark(top, "CLERK: KILL FIRST", {1.f, 0.8f, 0.35f}, false, true);
            else if (e.type == EnemyType::RS_POINT) mark(top, "RESTORE POINT", {0.8f, 0.92f, 1.f}, false, true);
            else if (e.type == EnemyType::RS_DRUM) {
                const char* gun = "YOUR GUN";
                for (int k = 0; k < cold::DRUMS; ++k) if (rsDrumUid[k] == e.uid && rsDrumGun[k] >= 0) gun = weaponDef((WeaponId)rsDrumGun[k]).name;
                char b[48]; std::snprintf(b, sizeof(b), "DRUM: %s", gun);
                mark(top, b, hookC, false, true);
            } else if (e.type == EnemyType::RESTORER && e.rsFalter) mark(rsDomeWorld(e), "FINISH IT", {0.6f, 0.95f, 1.f}, true);
        }
        if (rsRollbackT > 0.f) mark(rsRollbackAt + glm::vec3{0.f, 2.4f, 0.f}, "ROLLBACK: YOU GO BACK HERE", abGoldTell(glm::vec3{1.f, 0.8f, 0.3f}), false, true);
    }
    // The Hook Line: the next hook ahead of you (while you're on the gantry or out on the line)
    const int here = std::max(0, level.arenaAt(p));
    const bool onLine = std::fabs(p.x - cold::HOOK_X) < 12.f && p.z < cold::lipZ() + 4.f && p.z > cold::TOWER_FRONT;
    if (onLine && level.story.coldHookCount > 0) {
        int next = -1;
        for (int k = 0; k < cold::HOOKS; ++k) {
            if (grapple.active && grapple.hookedWall == level.story.coldHooks + k) continue;
            if (cold::hookAt(k).z < p.z - 3.f) { next = k; break; }
        }
        if (next >= 0) mark(cold::hookAt(next) - glm::vec3{0.f, 1.2f, 0.f}, grapple.latched ? "NEXT HOOK - GRAPPLE IT" : "HOOK - GRAPPLE IT", hookC);
        else mark({cold::HOOK_X, 3.f, cold::TOWER_FRONT}, "TOWER DOCK", route);
        return;
    }
    if (director.fighting()) return;
    // Otherwise: the way on
    switch (here) {
    case 0: mark(cold::hookBeacon() + glm::vec3{0.f, 4.f, 0.f}, "HOOK LINE", route); break;
    case 1: mark({cold::HOOK_X, 3.f, cold::TOWER_FRONT}, "TOWER DOCK", route); break;
    case 2: mark({cold::HOOK_X, 3.f, cold::THAW_BACK}, "ON", route); break;
    case 3: mark({cold::HOOK_X, 3.f, cold::INV_BACK}, "RACK 24", route); break;
    case 4: if (!coldRecall) mark(cold::recallAt(), "24-A", RECALL_FROST); else mark({cold::HOOK_X, 3.f, cold::TOWER_BACK}, "BACK DOOR", route); break;
    case 5: {
        const bool cleared = director.arena == 5 && director.phase == WaveDirector::Phase::CLEARED;
        if (cleared || director.arena > 5) mark(cold::seaStart() + glm::vec3{0.f, 2.f, 0.f}, "GATE - TO THE SEA", route);
        else mark(cold::chiller(0).c + glm::vec3{0.f, 2.f, 0.f}, "BLAST CHILLERS", route);
        break;
    }
    case 6: mark(cold::station() + glm::vec3{0.f, 3.f, 0.f}, "DISPATCH 2", route); break;
    case 7: if (coldKDown || director.arena > 7) mark(cold::station() + glm::vec3{18.f, 2.f, -2.f}, "THE MANIFEST - TO THE VAULT", route);
            else mark(cold::station() + glm::vec3{0.f, 3.f, 0.f}, "DISPATCH 2", route);
            break;
    case 8: mark(cold::vaultC() + glm::vec3{0.f, 3.f, 0.f}, "THE VAULT", route); break;
    case 9: if (coldRDown && rsDeathT > coldend::PARTS + 0.5f) mark(coldend::capsuleAt() + glm::vec3{0.f, 3.f, 0.f}, "DISPATCH CAPSULE", hookC);
            else if (!coldRDown) mark(cold::vaultC() + glm::vec3{0.f, 3.f, 0.f}, "THE VAULT", route);
            break;
    default: break;
    }
}

// The objective line in Chapter 5's fights: not just "HOSTILES", but what the fight wants of you
inline bool GameplayState::coldFightLine(char* buf, size_t n, int left) {
    if (const Enemy* R = restorer()) {
        int points = 0, drums = 0;
        for (const auto& e : enemies) { points += e.alive && e.type == EnemyType::RS_POINT; drums += e.alive && e.type == EnemyType::RS_DRUM; }
        if (R->rsFalter) std::snprintf(buf, n, "IT FALTERS  /  GET CLOSE - %s: DRIVE THE MODULE IN", binds().label(Act::INTERACT).c_str());
        else if (rsTongT > 0.f) std::snprintf(buf, n, "HUNG UP  /  PUNCH FREE");
        else if (points > 0) std::snprintf(buf, n, "BREAK THE RESTORE POINTS  %d LEFT  /  WHILE THEY STAND ITS WOUNDS GROW BACK", points);
        else if (drums > 0) std::snprintf(buf, n, "BREAK THE DRUMS  /  TAKE BACK YOUR GUNS  %d", drums);
        else if (rsPhase >= 3) std::snprintf(buf, n, "FACTORY RESET  /  IT TAKES YOUR GUNS - KEEP SHOOTING THE DOME");
        else std::snprintf(buf, n, "THE RESTORER  /  SHOOT THE GLASS DOME - GOLD: MOVE ON OR RECALL");
        return true;
    }
    if (pickedBy >= 0) { std::snprintf(buf, n, "PICKED  /  SHOOT IT OR PUNCH FREE"); return true; }
    if (director.arena == 7) { std::snprintf(buf, n, "UNIT 24-K  /  STEP OUT OF THE GOLD LINE - PARRY THE WHITE RUSH"); return true; }
    for (const auto& e : enemies)
        if (e.alive && e.type == EnemyType::CLERK) { std::snprintf(buf, n, "KILL THE CLERK FIRST - IT RESTORES THE DEAD  /  HOSTILES %d", left); return true; }
    if (director.arena == 1) { std::snprintf(buf, n, "PICKERS  /  SHOOT THEM DOWN - THE DOCK OPENS  %d", left); return true; }
    if (director.arena == 5) { std::snprintf(buf, n, "THE BLAST CHILLERS  /  KEEP OFF THE RED VENTS  HOSTILES %d", left); return true; }
    return false;
}
