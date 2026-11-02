#pragma once
// =============================================================================
// Gameplay_GleanerFight.h — GameplayState: the fight with THE GLEANER, from the
// end of the reveal (docs/chapter-4-design.md §5):
//
//   APPROACH  it crawls toward you to its battle mark and waits there; on the way
//             it calls meteors down on you (each marked on the ground, each one
//             that lands on you hurts)
//   KNEELING  close enough and it sinks onto its knuckles, screaming, and holds
//             still: its back is a battlefield now
//   CLIMB     its furnace breathes out an updraft beside it: step into the heat
//             and it carries you up and over the parapet onto the furnace roof
//   TENDER    the first of its brood climbs out of the hatch (Tender.h)
//   BROOD     the Tender is down; the rest of the brood (the Sieve, the Caller)
//             come next (not built yet)
//
// Its back holds still while you are on it, so the roof, its parapet and the
// chimneys are plain walls (added to the level while it kneels, taken away on a
// retry that starts before it). Reaching the roof is a checkpoint.
// Included at the end of GameplayState.h.
// =============================================================================

namespace gleanerplace {
inline glm::vec3 battleMark() { return {0.f, 0.f, -260.f}; }   // where it stops and waits, facing you
}

inline const char* GameplayState::gleanerPhaseName() const {
    switch (gleanerPhase) {
        case GleanerPhase::APPROACH: return "REACH THE GLEANER";
        case GleanerPhase::KNEELING: return "IT IS GOING DOWN";
        case GleanerPhase::CLIMB:    return "STEP INTO THE UPDRAFT / ONTO ITS BACK";
        case GleanerPhase::TENDER:   return "THE TENDER / IT KEEPS THE FURNACE";
        default:                     return "THE TENDER IS DOWN / MORE OF ITS BROOD ARE COMING";
    }
}

// The roof in the world (the Gleaner kneeling, facing +Z: its frame is the world's turned by nothing)
inline void GameplayState::gleanerDeckRect(glm::vec2& lo, glm::vec2& hi, float& top) const {
    const glm::vec3 b = gleaner::furnaceBase();
    glm::vec3 a = gleaner.toWorld({b.x - gleaner::DECK_HX, gleaner::deckTop(), b.z - gleaner::DECK_HZ});
    glm::vec3 c = gleaner.toWorld({b.x + gleaner::DECK_HX, gleaner::deckTop(), b.z + gleaner::DECK_HZ});
    lo = {std::min(a.x, c.x), std::min(a.z, c.z)}; hi = {std::max(a.x, c.x), std::max(a.z, c.z)};
    top = a.y;
}

// Take the roof's walls and the updraft away (a retry from before it knelt; a fresh start)
inline void GameplayState::clearGleanerDeck() {
    if (deckWallsFrom >= 0 && (int)level.walls.size() >= deckWallsFrom) level.walls.resize((size_t)deckWallsFrom);
    if (deckBoostersFrom >= 0 && (int)level.boosters.size() >= deckBoostersFrom) level.boosters.resize((size_t)deckBoostersFrom);
    deckWallsFrom = deckBoostersFrom = -1;
    spatialGrid.build(level.walls);
}

// The roof, its parapet (a gap on the west, where the updraft brings you), the chimneys, and the updraft
inline void GameplayState::buildGleanerDeck() {
    clearGleanerDeck();
    deckWallsFrom = (int)level.walls.size();
    deckBoostersFrom = (int)level.boosters.size();
    glm::vec2 lo, hi; float top;
    gleanerDeckRect(lo, hi, top);
    deckLo = lo; deckHi = hi; deckY = top;
    const float K = Gleaner::SCALE, gap = gleaner::GAP_HALF * K, cz = (lo.y + hi.y) * 0.5f;
    auto wall = [&](float x0, float y0, float z0, float x1, float y1, float z1) {
        Wall w{LevelBuilder::aabb(x0, y0, z0, x1, y1, z1), {0.3f, 0.2f, 0.15f}};
        w.hidden = true; w.mat = Mat::METAL;   // (drawn by the Gleaner's rig)
        level.walls.push_back(w);
    };
    wall(lo.x, top - 6.f, lo.y, hi.x, top, hi.y);   // the roof
    const float ph = 1.7f, t = 1.f;                 // the parapet
    wall(lo.x, top, lo.y - t, hi.x, top + ph, lo.y);
    wall(lo.x, top, hi.y, hi.x, top + ph, hi.y + t);
    wall(hi.x, top, lo.y, hi.x + t, top + ph, hi.y);
    wall(lo.x - t, top, lo.y, lo.x, top + ph, cz - gap);
    wall(lo.x - t, top, cz + gap, lo.x, top + ph, hi.y);
    for (int c = 0; c < 3; ++c) {                   // the chimneys: pillars to fight round
        glm::vec3 p = gleaner.toWorld(gleaner::chimney(c));
        float h = 4.5f * K;
        wall(p.x - h, top, p.z - h, p.x + h, top + gleaner::chimneyTall(c) * K, p.z + h);
    }
    // The updraft: a column of heat outside its left side, then the vent's breath across the parapet's gap
    const float colX = lo.x - 92.f;   // (clear of its body: you cannot stand inside it)
    updraftBase = {colX, 0.f, cz};
    // (out of its top, the vent's breath throws you across onto the roof: updateGleanerFight)
    level.boosters.push_back({LevelBuilder::aabb(colX - 8.f, -2.f, cz - 8.f, colX + 8.f, top + 6.f, cz + 8.f), glm::vec3{0.f, 1.f, 0.f}, 70.f});
    spatialGrid.build(level.walls);
}

// Where you stand on the roof after a retry: just inside the gap, the fight ahead of you
inline glm::vec3 GameplayState::deckCheckpoint() const {
    return {deckLo.x + 10.f, deckY + 0.05f, (deckLo.y + deckHi.y) * 0.5f};
}
inline bool GameplayState::onGleanerDeck() const {
    const glm::vec3 p = player.position;
    return deckWallsFrom >= 0 && p.x > deckLo.x && p.x < deckHi.x && p.y > deckY - 1.5f && p.y < deckY + 30.f && p.z > deckLo.y && p.z < deckHi.y;
}
inline void GameplayState::wakeTender() {
    glm::vec3 hatch = gleaner.toWorld(gleaner::furnaceBase() + glm::vec3{8.f, 0.f, 6.f});
    hatch.y = deckY;
    tender.wake(hatch, -1.5708f, deckLo, deckHi, deckY);   // facing west, toward the gap you came in by
}

// Called from beginGleaner: where the fight stands after a start, a retry or a Continue
inline void GameplayState::resetGleanerFight(bool fresh) {
    if (fresh && g_devOverlay == "plaindeck") storyCheckpoint = std::max(storyCheckpoint, 1);   // --overlay plaindeck: on its back, the Tender waking
    if (fresh) gleanerPhase = storyCheckpoint >= 2 ? GleanerPhase::BROOD : storyCheckpoint >= 1 ? GleanerPhase::TENDER : GleanerPhase::APPROACH;
    if (fresh && g_devOverlay == "plainclose") {   // --overlay plainclose: it waits at its mark, you a few hundred metres off
        gleaner.place(gleanerplace::battleMark(), 0.f, 1.f); gleaner.eyeOpen = 1.f;
        player.position = {0.f, 0.f, 110.f};
        player.camera.position = player.position + glm::vec3{0.f, 1.7f, 0.f}; prevCamPos = player.camera.position;
    }
    calledMeteorCd = 6.f; riverBurnCd = 0.f;
    if (gleanerPhase <= GleanerPhase::KNEELING) {
        gleanerPhase = GleanerPhase::APPROACH;
        clearGleanerDeck();
        tender = Tender{};
        return;
    }
    // On its back already: it is down on its knuckles at its mark, the roof is there, you are on it
    gleaner.place(gleanerplace::battleMark(), 0.f, 1.f);
    gleaner.eyeOpen = 1.f; gleaner.ribs = 0.3f; gleaner.kneel = 1.f;
    buildGleanerDeck();
    if (gleanerPhase == GleanerPhase::CLIMB) return;
    player.position = deckCheckpoint(); player.velocity = {0.f, 0.f, 0.f};
    player.camera.yaw = 0.f; player.camera.pitch = 0.f;
    player.camera.position = player.position + glm::vec3{0.f, 1.7f, 0.f}; prevCamPos = player.camera.position;
    if (gleanerPhase == GleanerPhase::TENDER) wakeTender();
}

// Out of the top of the updraft: the vent's breath throws you on an arc that lands just inside the parapet's gap
inline void GameplayState::updraftThrow() {
    if (deckWallsFrom < 0) return;
    glm::vec3& p = player.position;
    glm::vec2 off{p.x - updraftBase.x, p.z - updraftBase.z};
    if (glm::length(off) > 10.f || p.y < deckY + 6.5f || p.y > deckY + 40.f || player.velocity.y < 5.f) return;
    const float T = 2.2f, G = -24.f;   // (Player's gravity: a launched player falls at it)
    glm::vec3 target = deckCheckpoint() + glm::vec3{4.f, 0.f, 0.f};
    glm::vec3 d = target - p;
    player.velocity = {d.x / T, (d.y - 0.5f * G * T * T) / T, d.z / T};
    player.launched(); player.onGround = false;
    SfxMixer::Opts o; o.group = SoundGroup::PLAYER; o.pitch = 0.6f; o.priority = true;
    audio.playOpts("boost", o);
    shake(0.4f, 0.02f);
}

// The Gleaner's own will in the fight (it replaces the roaming of before)
inline void GameplayState::updateGleanerFight(float dt) {
    Gleaner& g = gleaner;
    if (gleanerPhase >= GleanerPhase::CLIMB) updraftThrow();
    const glm::vec3 p = player.position;
    const float flat = glm::length(glm::vec2{p.x - g.pos.x, p.z - g.pos.z});
    switch (gleanerPhase) {
    case GleanerPhase::APPROACH: {
        glm::vec3 d = gleanerplace::battleMark() - g.pos; d.y = 0.f;
        g.targetYaw = 0.f;
        g.wantSpeed = glm::length(d) > 6.f && !g.screaming() ? 3.4f : 0.f;
        gleanerScreamCd -= dt;
        if (gleanerScreamCd <= 0.f && !g.screaming()) { g.scream(); gleanerScreamCd = 40.f + 25.f * meteors.rnd(); }
        // It calls the sky down on you while you come
        calledMeteorCd -= dt;
        if (calledMeteorCd <= 0.f && flat > 300.f && !playerDead) {
            glm::vec3 lead = p + glm::vec3{player.velocity.x, 0.f, player.velocity.z} * (MeteorRain::CALLED_FALL * 0.8f);
            for (int k = 0; k < (flat < 700.f ? 2 : 1); ++k) {
                glm::vec3 off = k == 0 ? glm::vec3{0.f} : glm::vec3{meteors.rnd(-14.f, 14.f), 0.f, meteors.rnd(-14.f, 14.f)};
                meteors.launchCalled(lead + off, meteors.rnd(5.f, 8.f));
            }
            calledMeteorCd = glm::mix(3.2f, 5.5f, glm::clamp((flat - 300.f) / 900.f, 0.f, 1.f)) + meteors.rnd(0.f, 1.2f);
        }
        if (flat < 430.f && g.speed < 0.5f && !g.screaming()) {   // close: it goes down
            gleanerPhase = GleanerPhase::KNEELING;
            g.wantSpeed = 0.f; g.scream();
            pushBanner("IT GOES DOWN", "ITS BACK IS A BATTLEFIELD NOW", {1.f, 0.4f, 0.2f}, 3.5f);
        }
        break;
    }
    case GleanerPhase::KNEELING:
        g.wantSpeed = 0.f;
        if (!g.screaming()) {
            float was = g.kneel;
            g.kneel = std::min(1.f, g.kneel + dt / 3.f);
            if (was < 1.f && g.kneel >= 1.f) {
                g.yaw = g.targetYaw = 0.f;
                for (int a = 0; a < 2; ++a) { g.arm[a].plant = g.home(a, 0.f); gleanerDust.push_back({g.arm[a].plant, 0.f, 30.f * Gleaner::SCALE}); }
                gleanerSound("gleaner_slam", g.pos, 1.f, 1.f);
                plainQuakes.push_back({flat / 343.f, 1.f});
                buildGleanerDeck();
                gleanerPhase = GleanerPhase::CLIMB;
                pushBanner("THE UPDRAFT", "ITS FURNACE BREATHES OUT AT ITS SIDE: RIDE IT UP", {1.f, 0.55f, 0.25f}, 4.f);
            }
        }
        break;
    case GleanerPhase::CLIMB:
        if (onGleanerDeck() && player.onGround) {
            gleanerPhase = GleanerPhase::TENDER;
            wakeTender();
            storyCheckpoint = std::max(storyCheckpoint, 1); saveStoryProgress();
            ui.feed("CHECKPOINT / ON ITS BACK", {0.35f, 0.85f, 1.f});
            pushBanner("THE TENDER", "IT KEEPS THE FURNACE", {1.f, 0.5f, 0.2f}, 3.5f);
            audio.play("mini_sting", 128, SoundGroup::UI, true);
        }
        break;
    case GleanerPhase::TENDER:
        updateTender(dt);
        break;
    default: break;
    }
}

// THE TENDER's fight: its tick, what it does to you, what you do back
inline void GameplayState::updateTender(float dt) {
    Tender& k = tender;
    k.tick(dt, player.position);
    const glm::vec3 p = player.position;
    SfxMixer::Opts o; o.group = SoundGroup::ENEMY; o.positional = true; o.pos = k.base() + glm::vec3{0.f, 4.f, 0.f}; o.priority = true;
    for (const auto& e : k.events) {
        switch (e.kind) {
        case TenderEvent::SWEEP:
            o.pitch = 0.6f; audio.playOpts("sword_swing", o);
            if (k.inSweep(p) && !playerDead) {
                damagePlayer(22.f * tune().damage, k.base(), 0.25f, 0.04f);
                if (!g_godMode) player.velocity += glm::normalize(glm::vec3{p.x - k.pos.x, 0.f, p.z - k.pos.z} + glm::vec3{0.f, 0.001f, 0.f}) * 14.f + glm::vec3{0.f, 6.f, 0.f};
            }
            break;
        case TenderEvent::LOB: {   // a glob of slag at you (punch it back into it)
            glm::vec3 aim = player.camera.position - glm::vec3{0.f, 0.4f, 0.f} + glm::vec3{tender.rnd() - 0.5f, 0.f, tender.rnd() - 0.5f} * 3.f;
            glm::vec3 v = glm::normalize(aim - e.at) * 30.f;
            if (Projectile* pr = projSystem.fire(e.at, v, 16.f * tune().damage, false, {2.2f, 0.6f, 0.12f}, false, 0.f, 1.7f)) pr->parryDamage = 260.f;
            o.pitch = 0.7f; audio.playOpts("telegraph", o);
            break;
        }
        case TenderEvent::POUR_START: o.pitch = 0.55f; audio.playOpts("ab_pulse", o); shake(0.4f, 0.015f); break;
        case TenderEvent::STEP: o.pitch = 0.6f; o.volume = 0.7f; audio.playOpts("land_heavy", o); break;
        case TenderEvent::ROAR: o.pitch = 0.75f; audio.playOpts("gleaner_groan", o); break;
        case TenderEvent::STAGGER:
            o.pitch = 0.9f; audio.playOpts("stun", o);
            ui.feed("STAGGERED", {1.f, 0.8f, 0.3f});
            fx.spawnBurst(k.lipPoint(), {2.2f, 0.6f, 0.12f}, 40, 9.f, 0.8f, 14.f);
            break;
        case TenderEvent::DIE:
            o.pitch = 0.6f; audio.playOpts("explosion", o);
            fx.spawnBurst(k.base() + glm::vec3{0.f, 5.f, 0.f}, {2.4f, 0.7f, 0.15f}, 80, 14.f, 1.2f, 10.f);
            shake(0.8f, 0.05f); hitStopFrames = std::max(hitStopFrames, 6);
            styleSystem.addStyle(300.f, StyleSource::PARRY); gainXp(150);
            gleanerPhase = GleanerPhase::BROOD;
            gleaner.scream();
            pushBanner("THE TENDER FALLS", "THE GLEANER FEELS IT", {1.f, 0.4f, 0.2f}, 4.f);
            storyCheckpoint = std::max(storyCheckpoint, 2); saveStoryProgress();
            break;
        default: break;
        }
    }
    k.events.clear();
    // Standing in its river burns
    riverBurnCd -= dt;
    if (k.inRiver(p) && riverBurnCd <= 0.f && !playerDead) {
        riverBurnCd = 0.25f;
        damagePlayer(6.f * tune().damage, p - glm::vec3{0.f, 1.f, 0.f}, 0.1f, 0.01f, 0.f);
        fx.spawnBurst(p + glm::vec3{0.f, 0.3f, 0.f}, {2.f, 0.5f, 0.1f}, 6, 3.f, 0.4f, -2.f);
    }
    // Its slag punched back into it
    for (auto& pr : projSystem.pool) {
        if (!pr.alive || !pr.parried || !k.fighting()) continue;
        Tender::Box b = k.bodyBox();
        b.lo -= glm::vec3{1.f}; b.hi += glm::vec3{1.f, 4.f, 1.f};
        glm::vec3 q = pr.position;
        if (q.x > b.lo.x && q.x < b.hi.x && q.y > b.lo.y && q.y < b.hi.y && q.z > b.lo.z && q.z < b.hi.z) {
            pr.alive = false;
            k.hurt(pr.parryDamage > 0.f ? pr.parryDamage : 200.f, 0);
            k.stagger();
            fx.spawnBurst(q, {2.2f, 0.6f, 0.12f}, 30, 8.f, 0.6f, 10.f);
            styleSystem.addStyle(60.f, StyleSource::PARRY);
            ui.feed("RETURNED", {1.f, 0.7f, 0.3f});
        }
    }
    // You cannot walk through it
    if (k.alive()) {
        glm::vec3 d = player.position - k.pos; d.y = 0.f;
        float len = glm::length(d), r = Tender::RADIUS + 0.6f;
        if (len < r && player.position.y < k.base().y + Tender::HEIGHT) player.position += (len > 0.05f ? d / len : glm::vec3{1.f, 0.f, 0.f}) * (r - len);
    }
}

// A round meeting the Tender (fireWeapon asks before anything further along). True: it took the round
inline bool GameplayState::shootTender(glm::vec3 o, glm::vec3 d, float& nearest, float dmg, int w) {
    if (!plain() || gleanerPhase != GleanerPhase::TENDER || !tender.fighting()) return false;
    float t = 0.f;
    int zone = tender.ray(o, d, nearest, t);
    if (zone < 0) return false;
    glm::vec3 at = o + d * t;
    bool killed = tender.hurt(dmg * Tender::zoneMult(zone) * (w == (int)WeaponId::REVOLVER ? 1.f : 1.f), zone);
    fx.spawnHitSparks(at, zone == 2 ? glm::vec3{2.2f, 0.7f, 0.2f} : zone == 1 ? glm::vec3{1.f, 0.9f, 0.3f} : gunkit::glowOf(w));
    if (zone == 2) { fx.spawnBurst(at, {2.4f, 0.7f, 0.15f}, 10, 6.f, 0.4f, 12.f); ui.onHit(true); }
    else ui.onHit(zone == 1);
    audio.play(zone == 2 ? "hit_head" : zone == 1 ? "hit_head" : "hit_body", 90, SoundGroup::UI);
    styleSystem.addStyle(zone == 2 ? 12.f : 4.f, weaponSource((WeaponId)w));
    (void)killed;
    nearest = t;
    return true;
}

// Its weak point, for a SPLINTER: the lip while it pours, else its head
inline bool GameplayState::tenderSplinterPoint(glm::vec3& at) const {
    if (!plain() || gleanerPhase != GleanerPhase::TENDER || !tender.fighting()) return false;
    if (tender.lipOpen()) { at = tender.lipPoint(); return true; }
    Tender::Box h = tender.headBox();
    at = (h.lo + h.hi) * 0.5f;
    return true;
}

inline void GameplayState::gatherGleanerFight(std::vector<BoxInstance>& out) {
    using namespace rig;
    const float ball = (float)meshlib::lumpBall();
    if (gleanerPhase == GleanerPhase::TENDER || gleanerPhase == GleanerPhase::BROOD) {
        tenderrig::draw(out, tender, ball);
        tenderrig::drawRiver(out, tender, ball);
    }
    // Called meteors: a ring of fire on the ground where each will land, tightening as it comes
    for (const auto& m : meteors.pool) {
        if (!m.alive || !m.called) continue;
        float u = MeteorRain::calledIn(m), r = glm::mix(16.f, 7.f, u);
        float pulse = 0.6f + 0.4f * std::sin(gameClock * (8.f + 14.f * u));
        for (int k = 0; k < 18; ++k) {
            float a = k * 0.349f;
            push(out, T(m.target + glm::vec3{std::cos(a) * r, 0.12f, std::sin(a) * r}) * RY(-a) * S(glm::vec3{0.35f, 0.12f, r * 0.36f}),
                 glm::vec3{0.6f, 0.15f, 0.05f}, glm::vec3{2.6f, 0.5f, 0.1f} * pulse * (0.4f + 0.6f * u));
        }
        push(out, T(m.target + glm::vec3{0.f, 0.1f, 0.f}) * S(glm::vec3{1.2f, 0.1f, 1.2f}), glm::vec3{0.f}, glm::vec3{2.6f, 0.5f, 0.1f} * pulse);
    }
    // The updraft: a pillar of heat you can see from across the plain, embers pouring up it
    if (gleanerPhase >= GleanerPhase::CLIMB && deckWallsFrom >= 0) {
        const glm::vec3 c = updraftBase;
        for (int k = 0; k < 70; ++k) {
            float u = std::fmod(gameClock * 0.18f + k * 0.0143f * 7.f, 1.f);
            float a = k * 2.399f + gameClock * 0.7f, rr = 3.f + 5.f * std::fmod(k * 0.618f, 1.f);
            glm::vec3 q = c + glm::vec3{std::cos(a) * rr, u * (deckY + 12.f), std::sin(a) * rr};
            push(out, T(q) * RY(a) * S(glm::vec3{0.5f, 2.2f + 3.f * u, 0.5f}), glm::vec3{0.f}, glm::vec3{2.4f, 0.75f, 0.2f} * (1.f - 0.5f * u));
        }
        for (int k = 0; k < 12; ++k) {   // shimmering bands of heat
            float u = std::fmod(gameClock * 0.08f + k / 12.f, 1.f);
            glm::vec3 q = c + glm::vec3{0.f, u * (deckY + 12.f), 0.f};
            push(out, T(q) * RY(gameClock * 0.3f + k) * S(glm::vec3{15.f, 0.25f, 15.f}), glm::vec3{0.f}, glm::vec3{1.2f, 0.35f, 0.08f} * 0.35f * (1.f - u));
        }
        push(out, T(c + glm::vec3{0.f, 0.15f, 0.f}) * S(glm::vec3{16.f, 0.2f, 16.f}), glm::vec3{0.f}, glm::vec3{2.4f, 0.6f, 0.15f});
    }
}
