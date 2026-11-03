#pragma once
// =============================================================================
// Gameplay_GleanerInside.h — GameplayState: inside THE GLEANER (Organs.h), after
// its brood. The Caller dead, it slumps and the furnace door in the face of its
// furnace (behind the saddle) grinds open. Through it: its foundry, three rooms
// climbing up through it (gut:: in LevelPlain.h), one organ each:
//
//   HEART  the Crucible Heart floods the floor in rhythm: the islands
//   LUNGS  the Bellows Lungs: gusts down the room, pillars to shelter behind
//   CORE   the Sorting Core throws bodies: punch them back, shoot its lens
//
// Each organ dead opens a shaft of the furnace's breath up into the next room;
// the last one throws you up out of a chimney onto its back (the summit: next).
// Checkpoints: 5 inside, 6 the heart dead, 7 the lungs dead, 8 the core dead.
// Included at the end of GameplayState.h (after Gameplay_GleanerFight.h).
// =============================================================================

inline bool GameplayState::insideGleaner() const { return plain() && gut::inside(player.position); }

// Where you stand on a retry from one of its checkpoints, and which organ is there
inline void GameplayState::placeInGut(int room) {
    player.position = gut::arrive(room); player.velocity = glm::vec3{0.f};
    player.camera.yaw = gut::arriveYaw(room); player.camera.pitch = 0.f;
    player.camera.position = player.position + glm::vec3{0.f, 1.7f, 0.f}; prevCamPos = player.camera.position;
}

// The shafts of breath up out of a room (boosters), open once its organ is dead
inline void GameplayState::openGutShaft(int room) {
    const glm::vec3 s = gut::shaft(room);
    const float R = gut::SHAFT_R, top = room < 2 ? gut::ceilY(room) + 4.f : gut::ceilY(room) - 1.f;
    level.boosters.push_back({LevelBuilder::aabb(s.x - R, gut::floorY(room) - 1.f, s.z - R, s.x + R, top, s.z + R), glm::vec3{0.f, 1.f, 0.f}, 34.f});
    gutShaftOpen = std::max(gutShaftOpen, room);
}
inline void GameplayState::clearGutShafts() {
    if (gutBoostersFrom >= 0 && (int)level.boosters.size() >= gutBoostersFrom) level.boosters.resize((size_t)gutBoostersFrom);
    gutBoostersFrom = (int)level.boosters.size();
    gutShaftOpen = -1;
}

// Through the furnace door: into it
inline void GameplayState::enterGut() {
    gleanerPhase = GleanerPhase::HEART;
    storyCheckpoint = std::max(storyCheckpoint, 5); saveStoryProgress();
    clearGutShafts();
    placeInGut(0);
    heart.wake(gut::heart(), gut::floorY(0));
    gutWhite = 1.f;
    pushBanner("INSIDE IT", "\"EVERY ONE OF THEM FELT LIKE YOU.\"", {1.f, 0.4f, 0.2f}, 4.5f);
    ui.feed("CHECKPOINT / INSIDE IT", {0.35f, 0.85f, 1.f});
    SfxMixer::Opts o; o.group = SoundGroup::UI; o.priority = true; o.pitch = 0.5f; audio.playOpts("door_close", o);
    o.pitch = 0.6f; audio.playOpts("gleaner_groan", o);
}

// Out of each shaft's top: the breath throws you on an arc onto the next room's floor (the last: up out of a chimney)
inline void GameplayState::gutShaftThrow() {
    for (int r = 0; r <= gutShaftOpen && r < 3; ++r) {
        const glm::vec3 s = gut::shaft(r), p = player.position;
        if (glm::length(glm::vec2{p.x - s.x, p.z - s.z}) > gut::SHAFT_R + 0.5f || player.velocity.y < 4.f) continue;
        if (r < 2 && p.y > gut::ceilY(r) + 2.5f) {   // into the next room: across onto its floor
            const float T = 0.9f, G = -24.f;
            glm::vec3 d = gut::arrive(r + 1) - p;
            player.velocity = {d.x / T, (d.y - 0.5f * G * T * T) / T, d.z / T};
            player.launched(); player.onGround = false;
            audio.play("boost", 100, SoundGroup::PLAYER, true);
        }
        if (r == 2 && p.y > gut::ceilY(2) - 4.f) {   // up the chimney and out: onto its back, the plain below
            glm::vec3 top = deckCheckpoint() + glm::vec3{6.f, 0.f, 0.f};
            player.position = top + glm::vec3{0.f, 26.f, 0.f};
            player.velocity = {0.f, 6.f, 0.f}; player.launched(); player.onGround = false;
            player.camera.position = player.position + glm::vec3{0.f, 1.7f, 0.f}; prevCamPos = player.camera.position;
            gutWhite = 0.8f;
            SfxMixer::Opts o; o.group = SoundGroup::UI; o.priority = true; o.pitch = 0.5f; audio.playOpts("explosion", o);
            pushBanner("THE SUMMIT", "OUT OF ITS CHIMNEY, ONTO ITS BACK (THE EYE IS NEXT: NOT BUILT YET)", {1.f, 0.5f, 0.2f}, 5.f);
        }
    }
}

// A projectile it throws, that can be punched back (its slag, its sorted bodies)
inline void GameplayState::gutThrow(glm::vec3 from, float speed, float dmg, glm::vec3 col, float size, float parry) {
    glm::vec3 aim = player.camera.position - glm::vec3{0.f, 0.4f, 0.f} + glm::vec3{frand(-1.f, 1.f), 0.f, frand(-1.f, 1.f)} * 1.5f;
    glm::vec3 v = glm::normalize(aim - from) * speed;
    if (Projectile* pr = projSystem.fire(from, v, dmg * tune().damage, false, col, false, 0.f, size)) pr->parryDamage = parry;
}
// Punched back into an organ's box: true if one arrived (its damage in `dmg`)
inline bool GameplayState::gutReturned(const organ::Box& b, float& dmg) {
    for (auto& pr : projSystem.pool) {
        if (!pr.alive || !pr.parried) continue;
        glm::vec3 q = pr.position;
        if (q.x > b.lo.x && q.x < b.hi.x && q.y > b.lo.y && q.y < b.hi.y && q.z > b.lo.z && q.z < b.hi.z) {
            pr.alive = false;
            dmg = pr.parryDamage > 0.f ? pr.parryDamage : 250.f;
            fx.spawnBurst(q, {2.2f, 0.6f, 0.12f}, 30, 8.f, 0.6f, 10.f);
            styleSystem.addStyle(60.f, StyleSource::PARRY);
            ui.feed("RETURNED", {1.f, 0.7f, 0.3f});
            return true;
        }
    }
    return false;
}

inline void GameplayState::updateGut(float dt) {
    gutWhite = std::max(0.f, gutWhite - dt * 1.2f);
    gutShaftThrow();
    const glm::vec3 p = player.position;
    auto organSound = [&](const char* name, glm::vec3 at, float pitch, float vol = 1.f) {
        SfxMixer::Opts o; o.group = SoundGroup::ENEMY; o.positional = true; o.priority = true; o.pos = at; o.pitch = pitch; o.volume = vol; o.floor = 0.35f;
        audio.playOpts(name, o);
    };
    switch (gleanerPhase) {
    case GleanerPhase::HEART: {
        CrucibleHeart& h = heart;
        if (h.state == CrucibleHeart::DORMANT) h.wake(gut::heart(), gut::floorY(0));
        h.tick(dt);
        for (const auto& e : h.events) {
            switch (e.kind) {
            case organ::Event::BEAT: organSound("land_heavy", e.at, 0.42f, 0.9f); if (h.state == CrucibleHeart::SWELL) shake(0.15f, 0.01f); break;
            case organ::Event::SWELL: organSound("telegraph", e.at, 0.5f); ui.feed("IT SWELLS / GET UP OFF THE FLOOR", {1.f, 0.6f, 0.3f}); break;
            case organ::Event::FLOOD: organSound("ab_pulse", e.at, 0.45f); organSound("explosion", e.at, 0.6f, 0.6f); shake(0.6f, 0.03f); break;
            case organ::Event::DRAIN: organSound("gleaner_groan", e.at, 1.3f, 0.6f); break;
            case organ::Event::SPIT: gutThrow(e.at, 24.f, 15.f, {2.2f, 0.6f, 0.12f}, 1.6f, 300.f); organSound("telegraph", e.at, 0.7f, 0.7f); break;
            case organ::Event::DIE:
                organSound("explosion", e.at, 0.5f); organSound("gleaner_scream", e.at, 1.4f, 0.8f);
                fx.spawnBurst(e.at, {2.6f, 0.8f, 0.2f}, 90, 16.f, 1.3f, 8.f);
                shake(1.f, 0.05f); hitStopFrames = std::max(hitStopFrames, 6);
                styleSystem.addStyle(300.f, StyleSource::PARRY); gainXp(150);
                gleanerPhase = GleanerPhase::LUNGS; openGutShaft(0);
                storyCheckpoint = std::max(storyCheckpoint, 6); saveStoryProgress();
                pushBanner("ITS HEART IS OUT", "THE SLAG DRAINS. THE SHAFT IN THE CORNER BREATHES: RIDE IT UP", {1.f, 0.45f, 0.2f}, 4.5f);
                ui.feed("CHECKPOINT / ITS HEART", {0.35f, 0.85f, 1.f});
                break;
            default: break;
            }
        }
        h.events.clear();
        gutBurnCd -= dt;
        if (h.inSlag(p) && gutBurnCd <= 0.f && !playerDead) {
            gutBurnCd = 0.25f;
            damagePlayer(9.f * tune().damage, p - glm::vec3{0.f, 1.f, 0.f}, 0.1f, 0.01f, 0.f);
            fx.spawnBurst(p + glm::vec3{0.f, 0.3f, 0.f}, {2.f, 0.5f, 0.1f}, 6, 3.f, 0.4f, -2.f);
        }
        float back = 0.f;
        if (h.alive() && gutReturned(organ::around(h.pos, glm::vec3{CrucibleHeart::R + 1.f}), back)) h.hurt(back);
        break;
    }
    case GleanerPhase::LUNGS: {
        BellowsLungs& L = lungs;
        if (!L.woke && gut::roomAt(p) == 1 && player.onGround) {
            L.wake(gut::bellows(0), gut::bellows(1), gut::blow(), gut::floorY(1));
            pushBanner("ITS LUNGS", "STAND BEHIND THE PILLARS WHEN THEY BREATHE OUT", {1.f, 0.5f, 0.25f}, 4.f);
            audio.play("mini_sting", 128, SoundGroup::UI, true);
        }
        if (!L.woke) break;
        L.tick(dt);
        for (const auto& e : L.events) {
            switch (e.kind) {
            case organ::Event::INHALE: organSound("ab_pulse", e.at, 0.32f, 0.8f); break;
            case organ::Event::HISS: organSound("telegraph", e.at, 0.4f); break;
            case organ::Event::GUST: organSound("boost", e.at, 0.35f); organSound("explosion", e.at, 1.6f, 0.5f); shake(0.35f, 0.02f); break;
            case organ::Event::DIE:
                organSound("explosion", e.at, 0.55f); fx.spawnBurst(e.at, {1.6f, 0.6f, 0.3f}, 70, 14.f, 1.f, 6.f);
                shake(0.8f, 0.04f); styleSystem.addStyle(150.f, StyleSource::PARRY); gainXp(80);
                if (!L.alive()) {
                    gleanerPhase = GleanerPhase::CORE; openGutShaft(1);
                    storyCheckpoint = std::max(storyCheckpoint, 7); saveStoryProgress();
                    organSound("gleaner_scream", e.at, 1.3f, 0.8f);
                    pushBanner("ITS LUNGS ARE TORN", "THE AIR GOES STILL. UP THE SHAFT", {1.f, 0.45f, 0.2f}, 4.5f);
                    ui.feed("CHECKPOINT / ITS LUNGS", {0.35f, 0.85f, 1.f});
                } else ui.feed("ONE LUNG LEFT", {1.f, 0.7f, 0.3f});
                break;
            default: break;
            }
        }
        L.events.clear();
        // Its gusts: down the room, unless a pillar stands between you and the bellows
        for (int k = 0; k < 2 && gut::roomAt(p) == 1; ++k) {
            glm::vec3 push = L.gustAt(k, p);
            if (glm::length(push) < 0.01f) continue;
            glm::vec3 a = L.throat(k), d = p - a;
            bool shelter = false;
            for (int q = 0; q < 4 && !shelter; ++q) {   // (a 2D slab test of the line from the throat to you against each pillar)
                glm::vec3 c = gut::pillar(q);
                organ::Box b{{c.x - gut::PILLAR_HALF, -1e4f, c.z - gut::PILLAR_HALF}, {c.x + gut::PILLAR_HALF, 1e4f, c.z + gut::PILLAR_HALF}};
                float t = organ::rayBox({a.x, 0.f, a.z}, glm::normalize(glm::vec3{d.x, 0.f, d.z}), b);
                shelter = t >= 0.f && t < glm::length(glm::vec2{d.x, d.z});
            }
            if (!shelter && !g_godMode) player.velocity += push * dt;
        }
        gutBurnCd -= dt;
        if (gut::roomAt(p) == 1 && p.z < gut::grateZ() && p.y < gut::floorY(1) + 1.f && gutBurnCd <= 0.f && !playerDead) {
            gutBurnCd = 0.25f;
            damagePlayer(8.f * tune().damage, p - glm::vec3{0.f, 1.f, 0.f}, 0.1f, 0.01f, 0.f);
        }
        break;
    }
    case GleanerPhase::CORE: {
        SortingCore& c = core;
        if (c.state == SortingCore::DORMANT && gut::roomAt(p) == 2 && player.onGround) {
            c.wake(gut::core());
            pushBanner("ITS SORTING CORE", "IT THROWS WHAT IT SORTS: PUNCH THEM BACK", {1.f, 0.5f, 0.25f}, 4.f);
            audio.play("mini_sting", 128, SoundGroup::UI, true);
        }
        if (c.state == SortingCore::DORMANT) break;
        c.tick(dt);
        for (const auto& e : c.events) {
            switch (e.kind) {
            case organ::Event::ROAR: organSound("gleaner_groan", e.at, 1.1f, 0.7f); break;
            case organ::Event::THROW: gutThrow(e.at, 28.f, 18.f, {0.9f, 0.85f, 0.75f}, 2.2f, 350.f); organSound("hook_yank", e.at, 0.6f); break;
            case organ::Event::OPEN: organSound("stun", e.at, 0.8f); ui.feed("ITS LENS IS OPEN", {1.f, 0.8f, 0.3f}); break;
            case organ::Event::CLOSE: organSound("cyl_close", e.at, 0.5f); break;
            case organ::Event::DIE:
                organSound("explosion", e.at, 0.45f); organSound("gleaner_scream", e.at, 1.2f, 0.9f);
                fx.spawnBurst(e.at, {2.6f, 0.9f, 0.3f}, 100, 18.f, 1.4f, 6.f);
                shake(1.2f, 0.06f); hitStopFrames = std::max(hitStopFrames, 8);
                styleSystem.addStyle(300.f, StyleSource::PARRY); gainXp(200);
                gleanerPhase = GleanerPhase::SUMMIT; openGutShaft(2);
                storyCheckpoint = std::max(storyCheckpoint, 8); saveStoryProgress();
                pushBanner("ITS CORE STOPS", "THE CHIMNEY OPENS OVER IT: UP AND OUT", {1.f, 0.45f, 0.2f}, 4.5f);
                ui.feed("CHECKPOINT / ITS CORE", {0.35f, 0.85f, 1.f});
                break;
            default: break;
            }
        }
        c.events.clear();
        float back = 0.f;
        if (c.alive() && gutReturned(organ::around(c.pos, glm::vec3{SortingCore::R + 1.f, 4.f, SortingCore::R + 1.f}), back)) { c.hurt(back); c.open(); }
        break;
    }
    default: break;
    }
}

// A round meeting an organ (fireWeapon and the SPLINTER ask, through shootBrood)
inline bool GameplayState::shootOrgan(glm::vec3 o, glm::vec3 d, float& nearest, float dmg, int w, bool splinter) {
    float t = 0.f; int zone = -1, who = 0;
    if (gleanerPhase == GleanerPhase::HEART && heart.alive()) { zone = heart.ray(o, d, nearest, t, splinter); if (zone >= 0) heart.hurt(dmg * CrucibleHeart::zoneMult(zone)); }
    else if (gleanerPhase == GleanerPhase::LUNGS && lungs.alive()) { zone = lungs.ray(o, d, nearest, t, who, splinter); if (zone >= 0) lungs.hurt(who, dmg * BellowsLungs::zoneMult(zone)); }
    else if (gleanerPhase == GleanerPhase::CORE && core.alive()) { zone = core.ray(o, d, nearest, t, splinter); if (zone >= 0) core.hurt(dmg * SortingCore::zoneMult(zone)); }
    if (zone < 0) return false;
    glm::vec3 at = o + d * t;
    if (zone == 2) { fx.spawnBurst(at, {2.6f, 0.9f, 0.3f}, 12, 7.f, 0.4f, 12.f); ui.onHit(true); audio.play("hit_head", 90, SoundGroup::UI); }
    else { fx.spawnHitSparks(at, {1.f, 0.55f, 0.2f}); ui.onHit(false); audio.play("hit_armor", 80, SoundGroup::UI); }
    styleSystem.addStyle(zone == 2 ? 12.f : 3.f, weaponSource((WeaponId)w));
    nearest = t;
    return true;
}
inline bool GameplayState::organSplinterPoint(glm::vec3& at) const {
    const glm::vec3 p = player.position;
    if (gleanerPhase == GleanerPhase::HEART && heart.alive()) {
        int best = 0; for (int k = 1; k < 3; ++k) if (glm::length(heart.valve(k) - p) < glm::length(heart.valve(best) - p)) best = k;
        at = heart.valve(best); return true;
    }
    if (gleanerPhase == GleanerPhase::LUNGS && lungs.alive()) {
        int k = lungs.b[0].dead ? 1 : lungs.b[1].dead ? 0 : (lungs.throatOpen(1) && !lungs.throatOpen(0) ? 1 : 0);
        at = lungs.throat(k); return true;
    }
    if (gleanerPhase == GleanerPhase::CORE && core.alive()) { at = core.lens(); return true; }
    return false;
}

inline void GameplayState::gatherGut(std::vector<BoxInstance>& out) {
    const float ball = (float)meshlib::lumpBall();
    if (gleanerPhase >= GleanerPhase::HEART) organrig::drawHeart(out, heart, ball, gameClock);
    if (gleanerPhase >= GleanerPhase::LUNGS) organrig::drawLungs(out, lungs, gameClock);
    if (gleanerPhase >= GleanerPhase::CORE) organrig::drawCore(out, core, ball, gameClock);
    for (int r = 0; r <= gutShaftOpen && r < 3; ++r)
        organrig::drawShaft(out, gut::shaft(r), (r < 2 ? gut::ceilY(r) + 4.f : gut::ceilY(r)) - gut::floorY(r), gameClock);
}
