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
//   SIEVE     the roof's front parapet opens: drop onto the saddle over its
//             shoulders; a meteor is called down onto it and the Sieve unfolds
//             out of the crater (Sieve.h)
//   CALLER    it rears its head and holds it up: a bridge of plates along its
//             neck to the crown of its skull, and the Caller comes down out of
//             the sky to ring over it (Caller.h)
//   BROOD     all three are dead (the way inside is next: not built yet)
//
// Its back holds still while you are on it, so the roof, its parapet, the
// chimneys, the saddle, the neck and the crown are plain walls (added to the
// level while it kneels, taken away on a retry that starts before it).
// Checkpoints: 1 on its back, 2 the Tender dead, 3 the Sieve dead, 4 the Caller dead.
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
        case GleanerPhase::SIEVE:    return sieve.state == Sieve::DORMANT ? "DROP OFF THE ROOF'S FRONT / ONTO ITS SHOULDERS" : "THE SIEVE / ITS HEART SHOWS FROM BELOW";
        case GleanerPhase::CALLER:   return !deckCrown ? "IT REARS ITS HEAD" : caller.state == Caller::DORMANT ? "CROSS ITS NECK / ONTO ITS CROWN" : "THE CALLER / ITS LAMP SHOWS FROM BELOW";
        case GleanerPhase::BROOD:    return "THE BROOD IS DEAD / ITS FURNACE DOOR IS OPEN, BEHIND THE SADDLE";
        case GleanerPhase::HEART:    return "ITS CRUCIBLE HEART / SHOOT ITS VALVES WHILE IT FLOODS";
        case GleanerPhase::LUNGS:    return lungs.woke ? "ITS BELLOWS LUNGS / SHOOT THEIR THROATS AS THEY BREATHE IN" : "UP THE SHAFT / ITS LUNGS";
        case GleanerPhase::CORE:     return core.state != SortingCore::DORMANT ? "ITS SORTING CORE / PUNCH BACK WHAT IT THROWS" : "UP THE SHAFT / ITS CORE";
        default:                     return eye.fallen() ? "IT IS DOWN / TEAR IT OUT" : eye.state == EyeCore::DORMANT ? "THE SUMMIT / ONTO ITS ROOF" : "THE EYE / ITS LIDS OPEN WHEN IT FIRES";
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

// A rectangle of its back (its own frame, kneeling, facing +Z) in the world
inline void GameplayState::gleanerBackRect(float x0, float z0, float x1, float z1, float y, glm::vec2& lo, glm::vec2& hi, float& top) const {
    glm::vec3 a = gleaner.toWorld({x0, y, z0}), c = gleaner.toWorld({x1, y, z1});
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
    deckFrontOpen = gleanerPhase >= GleanerPhase::SIEVE;   // (the way down to the saddle, once the Tender is dead)
    deckCrown = gleanerPhase >= GleanerPhase::CALLER && gleaner.headHold >= 0.999f;
    const float cx = (lo.x + hi.x) * 0.5f, fg = gleaner::FRONT_GAP * K;
    if (!deckFrontOpen) wall(lo.x, top, hi.y, hi.x, top + ph, hi.y + t);
    else { wall(lo.x, top, hi.y, cx - fg, top + ph, hi.y + t); wall(cx + fg, top, hi.y, hi.x, top + ph, hi.y + t); }
    wall(hi.x, top, lo.y, hi.x + t, top + ph, hi.y);
    wall(lo.x - t, top, lo.y, lo.x, top + ph, cz - gap);
    wall(lo.x - t, top, cz + gap, lo.x, top + ph, hi.y);
    for (int c = 0; c < 3; ++c) {                   // the chimneys: pillars to fight round
        glm::vec3 p = gleaner.toWorld(gleaner::chimney(c));
        float h = 4.5f * K;
        wall(p.x - h, top, p.z - h, p.x + h, top + gleaner::chimneyTall(c) * K, p.z + h);
    }
    // The saddle over its shoulders (the Sieve's), a drop below the roof's front; a wall up the furnace's face behind it
    {
        using namespace gleaner;
        glm::vec2 a, b; float y;
        gleanerBackRect(-SADDLE_HX, SADDLE_Z0, SADDLE_HX, SADDLE_Z1, SADDLE_Y, a, b, y);
        saddleLo = a; saddleHi = b; saddleY = y;
        const float sx = (a.x + b.x) * 0.5f, nh = NECK_HX * K;
        wall(a.x, y - 6.f, a.y, b.x, y, b.y);
        wall(a.x - t, y, a.y, a.x, y + ph, b.y);
        wall(b.x, y, a.y, b.x + t, y + ph, b.y);
        furnaceDoor = gleanerPhase == GleanerPhase::BROOD;
        if (!furnaceDoor) wall(a.x, y, a.y - t, b.x, top - 6.f, a.y);   // the furnace's face, up to the roof
        else {   // its door open: a mouth of fire in the face
            const float dw = gleaner::DOOR_HX * K, dh = gleaner::DOOR_H * K;
            wall(a.x, y, a.y - t, sx - dw, top - 6.f, a.y);
            wall(sx + dw, y, a.y - t, b.x, top - 6.f, a.y);
            wall(sx - dw, y + dh, a.y - t, sx + dw, top - 6.f, a.y);
            wall(sx - dw, y - 1.f, a.y - 12.f, sx + dw, y, a.y);   // (the threshold: floor into it)
            wall(sx - dw - 1.f, y, a.y - 12.f, sx - dw, y + dh, a.y); wall(sx + dw, y, a.y - 12.f, sx + dw + 1.f, y + dh, a.y);
        }
        if (!deckCrown) wall(a.x, y, b.y, b.x, y + ph, b.y + t);
        else { wall(a.x, y, b.y, sx - nh, y + ph, b.y + t); wall(sx + nh, y, b.y, b.x, y + ph, b.y + t); }
        for (int i = 0; i < 3; ++i) {   // its spines, standing through the plates: pillars to fight round
            glm::vec3 p = gleaner.toWorld(saddlePillar(i));
            float r = PILLAR_R * K;
            wall(p.x - r, y, p.z - r, p.x + r, y + saddlePillarTall(i) * K, p.z + r);
        }
        // With its head up: the bridge along its neck, and its crown (the Caller's)
        if (deckCrown) {
            glm::vec2 na, nb, ca, cb; float ny, cy;
            gleanerBackRect(-NECK_HX, SADDLE_Z1, NECK_HX, CROWN_Z0, SADDLE_Y, na, nb, ny);
            gleanerBackRect(-CROWN_HX, CROWN_Z0, CROWN_HX, CROWN_Z1, SADDLE_Y, ca, cb, cy);
            neckLo = na; neckHi = nb; crownLo = ca; crownHi = cb;
            wall(na.x, ny - 4.f, na.y, nb.x, ny, nb.y);
            wall(na.x - t, ny, na.y, na.x, ny + ph, nb.y);
            wall(nb.x, ny, na.y, nb.x + t, ny + ph, nb.y);
            wall(ca.x, cy - 6.f, ca.y, cb.x, cy, cb.y);
            wall(ca.x - t, cy, ca.y, ca.x, cy + ph, cb.y);
            wall(cb.x, cy, ca.y, cb.x + t, cy + ph, cb.y);
            wall(ca.x, cy, cb.y, cb.x, cy + ph, cb.y + t);
            wall(ca.x, cy, ca.y - t, na.x, cy + ph, ca.y);
            wall(nb.x, cy, ca.y - t, cb.x, cy + ph, ca.y);
            for (int s2 = 0; s2 < 2; ++s2) {   // its horns
                glm::vec3 p = gleaner.toWorld(crownHorn(s2));
                float r = HORN_R * K;
                wall(p.x - r, cy, p.z - r, p.x + r, cy + HORN_TALL * K, p.z + r);
            }
        }
    }
    // The updraft: a column of heat outside its left side, then the vent's breath across the parapet's gap
    const float colX = lo.x - 92.f;   // (clear of its body: you cannot stand inside it)
    updraftBase = {colX, 0.f, cz};
    // (out of its top, the vent's breath throws you across onto the roof: updateGleanerFight)
    level.boosters.push_back({LevelBuilder::aabb(colX - 8.f, -2.f, cz - 8.f, colX + 8.f, top + 6.f, cz + 8.f), glm::vec3{0.f, 1.f, 0.f}, 70.f});
    spatialGrid.build(level.walls);
}

// The next place to reach. `beacon`: draw a ring and a light on it in the world too (the updraft
// and the Gleaner itself are their own beacons)
inline bool GameplayState::gleanerObjective(glm::vec3& at, const char*& label, bool& beacon) const {
    if (!plain() || plainReveal.active() || playerDead) return false;
    beacon = false;
    const GleanerPhase ph = gleanerPhase;
    if (ph == GleanerPhase::APPROACH || ph == GleanerPhase::KNEELING) { at = gleaner.eye(); label = "THE GLEANER"; return true; }
    if (insideGleaner()) {
        const int room = gut::roomAt(player.position);
        if (gutShaftOpen >= room && (int)ph - (int)GleanerPhase::HEART > room) {   // its organ dead: the shaft up
            at = gut::shaft(room) + glm::vec3{0.f, 2.f, 0.f}; label = room < 2 ? "SHAFT UP" : "CHIMNEY"; beacon = true; return true;
        }
        return false;
    }
    if (ph == GleanerPhase::SUMMIT) {
        if (summitEndT >= 0.f) return false;
        if (!onGleanerDeck() && deckWallsFrom >= 0) { at = updraftBase + glm::vec3{0.f, 4.f, 0.f}; label = "BACK UP / UPDRAFT OR HOOK IT"; return true; }
        if (eye.fallen()) { at = eye.pos + glm::vec3{0.f, EyeCore::R + 1.f, 0.f}; label = "TEAR IT OUT"; return true; }
        return false;
    }
    if (deckWallsFrom < 0 || ph > GleanerPhase::BROOD) return false;
    if (ph == GleanerPhase::CLIMB || !onGleanerBack()) {   // (fallen off it: the same way back up)
        at = updraftBase + glm::vec3{0.f, 4.f, 0.f}; label = ph == GleanerPhase::CLIMB ? "UPDRAFT / OR HOOK IT" : "BACK UP / UPDRAFT OR HOOK IT"; return true;
    }
    if (ph == GleanerPhase::SIEVE && sieve.state == Sieve::DORMANT && !sieveForged && !onSaddle()) {
        at = {(deckLo.x + deckHi.x) * 0.5f, deckY + 0.1f, deckHi.y - 1.f}; label = "DROP TO ITS SHOULDERS"; beacon = true; return true;
    }
    if (ph == GleanerPhase::CALLER && deckCrown && caller.state == Caller::DORMANT) {
        at = {(crownLo.x + crownHi.x) * 0.5f, saddleY + 0.1f, (crownLo.y + crownHi.y) * 0.5f}; label = "ITS CROWN"; beacon = true; return true;
    }
    if (ph == GleanerPhase::BROOD && furnaceDoor) {
        at = {(saddleLo.x + saddleHi.x) * 0.5f, saddleY + 0.1f, saddleLo.y - 2.f}; label = "FURNACE DOOR"; beacon = true; return true;
    }
    return false;
}

// Where you stand on the roof after a retry: just inside the gap, the fight ahead of you
inline glm::vec3 GameplayState::deckCheckpoint() const {
    return {deckLo.x + 10.f, deckY + 0.05f, (deckLo.y + deckHi.y) * 0.5f};
}
inline bool GameplayState::onGleanerDeck() const {
    const glm::vec3 p = player.position;
    return deckWallsFrom >= 0 && p.x > deckLo.x && p.x < deckHi.x && p.y > deckY - 1.5f && p.y < deckY + 30.f && p.z > deckLo.y && p.z < deckHi.y;
}
inline bool GameplayState::onSaddle() const {
    const glm::vec3 p = player.position;
    return deckWallsFrom >= 0 && p.x > saddleLo.x && p.x < saddleHi.x && p.y > saddleY - 1.5f && p.y < saddleY + 30.f && p.z > saddleLo.y && p.z < saddleHi.y;
}
inline bool GameplayState::onCrown() const {
    const glm::vec3 p = player.position;
    return deckWallsFrom >= 0 && deckCrown && p.x > crownLo.x && p.x < crownHi.x && p.y > saddleY - 1.5f && p.y < saddleY + 30.f && p.z > crownLo.y && p.z < crownHi.y;
}
inline bool GameplayState::onGleanerBack() const {
    const glm::vec3 p = player.position;
    const bool neck = deckCrown && p.x > neckLo.x && p.x < neckHi.x && p.y > saddleY - 1.5f && p.y < saddleY + 30.f && p.z > neckLo.y && p.z < neckHi.y;
    return onGleanerDeck() || onSaddle() || onCrown() || (deckWallsFrom >= 0 && neck);
}
// After a retry: on the saddle just under the roof's front (the Sieve), at the saddle's front facing its neck (the Caller)
inline glm::vec3 GameplayState::saddleCheckpoint() const { return {(saddleLo.x + saddleHi.x) * 0.5f, saddleY + 0.05f, saddleLo.y + 10.f}; }
inline glm::vec3 GameplayState::crownCheckpoint() const { return {(saddleLo.x + saddleHi.x) * 0.5f, saddleY + 0.05f, saddleHi.y - 6.f}; }
inline void GameplayState::wakeTender() {
    glm::vec3 hatch = gleaner.toWorld(gleaner::furnaceBase() + glm::vec3{8.f, 0.f, 6.f});
    hatch.y = deckY;
    tender.wake(hatch, -1.5708f, deckLo, deckHi, deckY);   // facing west, toward the gap you came in by
}
inline void GameplayState::wakeSieve() {
    glm::vec3 at = gleaner.toWorld({0.f, gleaner::SADDLE_Y, 21.f});
    sieve.wake(at, 3.14159f, saddleLo, saddleHi, saddleY);
}
inline void GameplayState::wakeCaller() {
    caller.wake({(crownLo.x + crownHi.x) * 0.5f, saddleY, (crownLo.y + crownHi.y) * 0.5f}, saddleY, 0.6f);
}

// Called from beginGleaner: where the fight stands after a start, a retry or a Continue
inline void GameplayState::resetGleanerFight(bool fresh) {
    if (fresh && g_devOverlay == "plaindeck") storyCheckpoint = std::max(storyCheckpoint, 1);     // --overlay plaindeck: on its back, the Tender waking
    if (fresh && g_devOverlay == "plainsaddle") storyCheckpoint = std::max(storyCheckpoint, 2);   // --overlay plainsaddle: on its shoulders, the Sieve coming
    if (fresh && g_devOverlay == "plaincrown") storyCheckpoint = std::max(storyCheckpoint, 3);    // --overlay plaincrown: its head up, the Caller coming
    if (fresh && g_devOverlay == "plainsummit") storyCheckpoint = std::max(storyCheckpoint, 8);   // --overlay plainsummit: on its roof, the eye to come
    if (fresh) gleanerPhase = storyCheckpoint >= 8 ? GleanerPhase::SUMMIT : storyCheckpoint >= 7 ? GleanerPhase::CORE : storyCheckpoint >= 6 ? GleanerPhase::LUNGS
                            : storyCheckpoint >= 5 ? GleanerPhase::HEART : storyCheckpoint >= 4 ? GleanerPhase::BROOD : storyCheckpoint >= 3 ? GleanerPhase::CALLER : storyCheckpoint >= 2 ? GleanerPhase::SIEVE
                            : storyCheckpoint >= 1 ? GleanerPhase::TENDER : GleanerPhase::APPROACH;
    if (fresh && g_devOverlay == "plainclose") {   // --overlay plainclose: it waits at its mark, you a few hundred metres off
        gleaner.place(gleanerplace::battleMark(), 0.f, 1.f); gleaner.eyeOpen = 1.f;
        player.position = {0.f, 0.f, 110.f};
        player.camera.position = player.position + glm::vec3{0.f, 1.7f, 0.f}; prevCamPos = player.camera.position;
    }
    gleanerScreamCd = gleanerPhase >= GleanerPhase::TENDER ? 14.f : gleanerScreamCd;   // (picked up on its back: a roar soon after)
    calledMeteorCd = 6.f; riverBurnCd = 0.f; sieveBurnCd = 0.f; broodCallCd = 12.f; callerClangCd = 0.f;
    sieve = Sieve{}; caller = Caller{}; sieveForged = false; broodWakeIn = -1.f;
    heart = CrucibleHeart{}; lungs = BellowsLungs{}; core = SortingCore{}; gutBurnCd = 0.f; gutWhite = 0.f;
    eye = EyeCore{}; summitEndT = -1.f; eyeHitCd = 0.f; meteors.stopped = false;
    gleaner.headHold = 0.f;
    if (gleanerPhase <= GleanerPhase::KNEELING) {
        gleanerPhase = GleanerPhase::APPROACH;
        clearGleanerDeck();
        tender = Tender{};
        return;
    }
    // On its back already: it is down on its knuckles at its mark, the roof is there, you are on it
    gleaner.place(gleanerplace::battleMark(), 0.f, 1.f);
    gleaner.eyeOpen = 1.f; gleaner.ribs = 0.3f; gleaner.kneel = 1.f;
    gleaner.headHold = gleanerPhase >= GleanerPhase::CALLER ? 1.f : 0.f;
    if (gleanerPhase > GleanerPhase::TENDER) tender = Tender{};
    buildGleanerDeck();
    if (gleanerPhase == GleanerPhase::CLIMB) return;
    clearGutShafts();
    if (gleanerPhase >= GleanerPhase::HEART && gleanerPhase <= GleanerPhase::CORE) {   // inside it: at the foot of its room
        int room = (int)gleanerPhase - (int)GleanerPhase::HEART;
        for (int r = 0; r < room; ++r) openGutShaft(r);
        placeInGut(room);
        if (room == 0) heart.wake(gut::heart(), gut::floorY(0));
        return;
    }
    const bool caller0 = gleanerPhase >= GleanerPhase::CALLER;
    player.position = gleanerPhase == GleanerPhase::TENDER || gleanerPhase == GleanerPhase::SUMMIT ? deckCheckpoint() : caller0 ? crownCheckpoint() : saddleCheckpoint();
    player.velocity = {0.f, 0.f, 0.f};
    player.camera.yaw = gleanerPhase == GleanerPhase::TENDER || gleanerPhase == GleanerPhase::SUMMIT ? 0.f : gleanerPhase == GleanerPhase::BROOD ? -90.f : 90.f; player.camera.pitch = 0.f;   // (into the roof; on toward its head; back to its furnace door)
    player.camera.position = player.position + glm::vec3{0.f, 1.7f, 0.f}; prevCamPos = player.camera.position;
    if (gleanerPhase == GleanerPhase::TENDER) wakeTender();
}

// Out of the top of the updraft: the vent's breath throws you on an arc that lands just inside the parapet's gap
inline void GameplayState::updraftThrow() {
    if (deckWallsFrom < 0) return;
    glm::vec3& p = player.position;
    glm::vec2 off{p.x - updraftBase.x, p.z - updraftBase.z};
    if (glm::length(off) > 10.f || p.y < deckY + 6.5f || p.y > deckY + 40.f || player.velocity.y < 5.f) return;
    const float G = -24.f;   // (Player's gravity: a launched player falls at it)
    // (fallen off its back mid-fight: the vent throws you back to wherever the fight is)
    glm::vec3 target = gleanerBackTarget();
    const float T = std::clamp(glm::length(glm::vec2{target.x - p.x, target.z - p.z}) / 45.f, 2.2f, 4.f);
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
    // Through the whole fight, on its back and inside it, it roars now and then (the approach has its own)
    if (gleanerPhase >= GleanerPhase::TENDER && !g.screaming() && !playerDead) {
        gleanerScreamCd -= dt;
        if (gleanerScreamCd <= 0.f) { g.scream(); gleanerScreamCd = 28.f + 16.f * meteors.rnd(); }
    }
    switch (gleanerPhase) {
    case GleanerPhase::APPROACH: {
        glm::vec3 d = gleanerplace::battleMark() - g.pos; d.y = 0.f;
        g.targetYaw = 0.f;
        g.wantSpeed = glm::length(d) > 6.f && !g.screaming() && flat > 430.f ? 3.4f : 0.f;   // (you reached it first: it stops where it is)
        gleanerScreamCd -= dt;
        if (gleanerScreamCd <= 0.f && !g.screaming()) { g.scream(); gleanerScreamCd = 24.f + 14.f * meteors.rnd(); }
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
    case GleanerPhase::SIEVE:
        if (tender.state == Tender::DEAD) updateTender(dt);   // (its river cools, its body lies there)
        // Onto its shoulders: the sky is called down onto the saddle, and the Sieve unfolds out of the crater
        if (sieve.state == Sieve::DORMANT && !sieveForged && onSaddle() && player.onGround) {
            sieveForged = true;
            glm::vec3 at = gleaner.toWorld({0.f, gleaner::SADDLE_Y, 21.f});
            meteors.launchCalled(at, 11.f, saddleY);
            broodWakeIn = MeteorRain::CALLED_FALL;
            gleaner.scream();
        }
        if (broodWakeIn > 0.f) {
            broodWakeIn -= dt;
            if (broodWakeIn <= 0.f) {
                wakeSieve();
                pushBanner("THE SIEVE", "IT SIFTS WHAT FALLS", {1.f, 0.6f, 0.2f}, 3.5f);
                audio.play("mini_sting", 128, SoundGroup::UI, true);
                shake(0.8f, 0.04f);
            }
        }
        if (sieve.alive() || sieve.state == Sieve::DEAD) updateSieve(dt);
        // The Gleaner itself: now and then the sky, onto its own back
        if (sieve.fighting() && !playerDead) {
            broodCallCd -= dt;
            if (broodCallCd <= 0.f) { meteors.launchCalled(player.position, 5.f, saddleY); broodCallCd = 11.f + 5.f * meteors.rnd(); }
        }
        break;
    case GleanerPhase::CALLER:
        if (gleaner.headHold < 1.f && !gleaner.screaming()) {   // it rears its head up and holds it there
            gleaner.headHold = std::min(1.f, gleaner.headHold + dt / 4.f);
            if (gleaner.headHold >= 1.f) {
                buildGleanerDeck();
                gleanerSound("gleaner_slam", gleaner.toWorld(gleaner::heldHead()), 0.8f, 0.f);
                shake(1.2f, 0.04f);
                pushBanner("ITS HEAD IS UP", "CROSS ITS NECK TO ITS CROWN", {1.f, 0.4f, 0.2f}, 4.f);
            }
        }
        if (caller.state == Caller::DORMANT && deckCrown && (onCrown() || player.position.z > neckLo.y + (neckHi.y - neckLo.y) * 0.5f) && onGleanerBack()) {
            wakeCaller();
            pushBanner("THE CALLER", "IT RINGS FOR THE SKY", {1.f, 0.25f, 0.15f}, 3.5f);
            audio.play("mini_sting", 128, SoundGroup::UI, true);
        }
        if (caller.state != Caller::DORMANT) updateCaller(dt);
        break;
    case GleanerPhase::BROOD: {
        if (caller.state == Caller::DEAD) updateCaller(dt);   // (it falls the rest of the way)
        if (!furnaceDoor && !gleaner.screaming()) {   // it slumps; its furnace door grinds open
            buildGleanerDeck();
            gleanerSound("gleaner_groan", gleaner.toWorld(gleaner::furnaceBase()), 1.f, 0.f);
            shake(1.5f, 0.03f);
            pushBanner("IT SLUMPS", "ITS FURNACE DOOR GRINDS OPEN, BEHIND THE SADDLE: INTO IT", {1.f, 0.45f, 0.2f}, 4.5f);
        }
        // Into the door's mouth: inside
        const glm::vec3 p = player.position;
        const float sx = (saddleLo.x + saddleHi.x) * 0.5f;
        if (furnaceDoor && std::abs(p.x - sx) < gleaner::DOOR_HX * Gleaner::SCALE && p.z < saddleLo.y - 4.f && p.z > saddleLo.y - 12.f && std::abs(p.y - saddleY) < 3.f) enterGut();
        break;
    }
    case GleanerPhase::HEART: case GleanerPhase::LUNGS: case GleanerPhase::CORE:
        updateGut(dt);
        break;
    case GleanerPhase::SUMMIT:
        updateGut(dt);   // (its chimney's throw, while you are still in it)
        if (!insideGleaner()) updateSummit(dt);
        break;
    default: break;
    }
}

// THE TENDER's fight: its tick, what it does to you, what you do back
inline void GameplayState::updateTender(float dt) {
    Tender& k = tender;
    k.tick(dt, player.position);
    if (gleanerPhase != GleanerPhase::TENDER) { k.events.clear(); return; }
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
            gleanerPhase = GleanerPhase::SIEVE;
            buildGleanerDeck();   // (the roof's front opens: the way down to its shoulders)
            gleaner.scream();
            pushBanner("THE TENDER FALLS", "THE ROOF OPENS AT ITS FRONT: DROP TO ITS SHOULDERS", {1.f, 0.4f, 0.2f}, 4.5f);
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

// THE SIEVE's fight: its tick, what it does to you
inline void GameplayState::updateSieve(float dt) {
    Sieve& v = sieve;
    v.tick(dt, player.position);
    const glm::vec3 p = player.position;
    SfxMixer::Opts o; o.group = SoundGroup::ENEMY; o.positional = true; o.pos = v.heart(); o.priority = true;
    for (const auto& e : v.events) {
        switch (e.kind) {
        case SieveEvent::STEP: { SfxMixer::Opts f = o; f.pos = e.at; f.pitch = 0.55f + 0.1f * v.rnd(); f.volume = 0.55f; f.priority = false; audio.playOpts("land_metal", f); break; }
        case SieveEvent::SHAKE: o.pitch = 1.f; audio.playOpts("sieve_rattle", o); break;
        case SieveEvent::RAIN_START: o.pitch = 0.6f; o.volume = 0.8f; audio.playOpts("ab_pulse", o); shake(0.3f, 0.012f); break;
        case SieveEvent::STAKE_TELL: { SfxMixer::Opts f = o; f.pos = e.at; f.pitch = 0.55f; audio.playOpts("telegraph", f); break; }
        case SieveEvent::STAKE: {
            SfxMixer::Opts f = o; f.pos = e.at; audio.playOpts("sieve_stake", f);
            shake(0.5f, 0.05f);
            fx.spawnBurst(e.at + glm::vec3{0.f, 0.5f, 0.f}, {2.4f, 0.8f, 0.2f}, 30, 10.f, 0.7f, 12.f);
            glm::vec2 d{p.x - e.at.x, p.z - e.at.z};
            if (glm::length(d) < 3.5f && p.y - saddleY < 2.f && !playerDead) damagePlayer(20.f * tune().damage, e.at, 0.3f, 0.05f);
            break;
        }
        case SieveEvent::CRACKS: { SfxMixer::Opts f = o; f.pos = e.at; f.pitch = 1.3f; f.volume = 0.7f; audio.playOpts("explosion", f); shake(0.35f, 0.03f); break; }
        case SieveEvent::ROAR: o.pitch = 1.25f; audio.playOpts("gleaner_groan", o); break;
        case SieveEvent::STAGGER:
            o.pitch = 0.8f; audio.playOpts("stun", o);
            ui.feed("SPILLED", {1.f, 0.8f, 0.3f});
            ui.toast("ITS BASKET SPILLS", "DOUBLE DAMAGE - ITS HEART IS OPEN", {1.f, 0.75f, 0.2f}, 1.8f);
            fx.spawnBurst(v.heart(), {2.4f, 0.8f, 0.2f}, 50, 10.f, 0.9f, 14.f);
            break;
        case SieveEvent::DIE:
            o.pitch = 0.55f; audio.playOpts("explosion", o); audio.playOpts("sieve_stake", o);
            fx.spawnBurst(v.heart(), {2.6f, 0.9f, 0.25f}, 90, 16.f, 1.3f, 10.f);
            shake(0.9f, 0.05f); hitStopFrames = std::max(hitStopFrames, 6);
            styleSystem.addStyle(300.f, StyleSource::PARRY); gainXp(150);
            gleanerPhase = GleanerPhase::CALLER;
            gleaner.scream();
            pushBanner("THE SIEVE FALLS", "THE GLEANER REARS ITS HEAD", {1.f, 0.4f, 0.2f}, 4.f);
            storyCheckpoint = std::max(storyCheckpoint, 3); saveStoryProgress();
            ui.feed("CHECKPOINT / ITS SHOULDERS", {0.35f, 0.85f, 1.f});
            break;
        default: break;
        }
    }
    v.events.clear();
    if (!v.alive()) return;
    // Its rain burns; its cracks burst under you (once a stake); the stake's leg and its feet are solid
    sieveBurnCd -= dt;
    if (v.inRain(p) && sieveBurnCd <= 0.f && !playerDead) {
        sieveBurnCd = 0.25f;
        damagePlayer(7.f * tune().damage, p + glm::vec3{0.f, 3.f, 0.f}, 0.1f, 0.01f, 0.f);
        fx.spawnBurst(p + glm::vec3{0.f, 1.8f, 0.f}, {2.4f, 0.8f, 0.2f}, 5, 2.f, 0.3f, -6.f);
    }
    if (v.bursting() && !v.burstDone && v.onCrack(p) && !playerDead) {
        v.burstDone = true;
        damagePlayer(24.f * tune().damage, p - glm::vec3{0.f, 1.f, 0.f}, 0.35f, 0.05f);
        if (!g_godMode) player.velocity.y = std::max(player.velocity.y, 9.f);
    }
    for (int i = 0; i < 4; ++i) {
        glm::vec3 f = v.footNow(i); f.y = player.position.y;
        glm::vec3 off = player.position - f;
        float len = glm::length(off), r = 1.1f;
        if (len < r && v.footNow(i).y < saddleY + 2.f) player.position = f + (len > 0.05f ? off / len : glm::vec3{1.f, 0.f, 0.f}) * r;
    }
}

// THE CALLER's fight
inline void GameplayState::updateCaller(float dt) {
    Caller& c = caller;
    c.tick(dt, player.position, player.camera.position);
    callerClangCd = std::max(0.f, callerClangCd - dt);
    const glm::vec3 p = player.position;
    SfxMixer::Opts o; o.group = SoundGroup::ENEMY; o.positional = true; o.pos = c.pos; o.priority = true;
    for (const auto& e : c.events) {
        switch (e.kind) {
        case CallerEvent::TOLL: {   // the loudest thing on its back: it carries
            SfxMixer::Opts f = o; f.floor = 0.6f; f.unducked = true; audio.playOpts("caller_toll", f);
            audio.duck(4.f, 1.2f);
            shake(0.6f, 0.012f);
            break;
        }
        case CallerEvent::CALL: meteors.launchCalled(e.at, 6.f, c.floorY); break;
        case CallerEvent::DIVE_TELL: o.pitch = 0.75f; audio.playOpts("telegraph", o); break;
        case CallerEvent::DIVE: o.pitch = 0.5f; audio.playOpts("boost", o); break;
        case CallerEvent::LAND:
            o.pitch = 0.6f; audio.playOpts("caller_clang", o); audio.playOpts("land_heavy", o);
            shake(0.5f, 0.05f);
            fx.spawnBurst(c.pos - glm::vec3{0.f, Caller::BELL_R * 0.8f, 0.f}, {0.4f, 0.3f, 0.25f}, 30, 8.f, 0.9f, 10.f);
            break;
        case CallerEvent::ROAR: { SfxMixer::Opts f = o; f.pitch = 0.7f; f.floor = 0.5f; audio.playOpts("caller_toll", f); break; }
        case CallerEvent::WINGBEAT: { SfxMixer::Opts f = o; f.pitch = 0.4f; f.volume = 0.45f; f.priority = false; audio.playOpts("sword_swing", f); break; }
        case CallerEvent::DIE: {
            SfxMixer::Opts f = o; f.pitch = 0.55f; f.floor = 0.6f; audio.playOpts("caller_toll", f); audio.playOpts("explosion", f);
            fx.spawnBurst(c.lamp(), {3.f, 0.3f, 0.15f}, 90, 14.f, 1.2f, 8.f);
            shake(0.9f, 0.05f); hitStopFrames = std::max(hitStopFrames, 6);
            styleSystem.addStyle(300.f, StyleSource::PARRY); gainXp(150);
            gleanerPhase = GleanerPhase::BROOD;
            gleaner.scream();
            pushBanner("THE CALLER FALLS", "THE BROOD IS DEAD", {1.f, 0.35f, 0.2f}, 4.5f);
            storyCheckpoint = std::max(storyCheckpoint, 4); saveStoryProgress();
            ui.feed("CHECKPOINT / ITS CROWN", {0.35f, 0.85f, 1.f});
            break;
        }
        default: break;
        }
    }
    c.events.clear();
    // Its dive: committed, straight through where you stood
    if (c.state == Caller::DIVE && !c.hitDone && !playerDead && glm::length(c.pos - (p + glm::vec3{0.f, 1.f, 0.f})) < Caller::HIT_R) {
        c.hitDone = true;
        damagePlayer(24.f * tune().damage, c.pos, 0.4f, 0.06f);
        if (!g_godMode) player.velocity += glm::vec3{c.diveDir.x, 0.f, c.diveDir.z} * 16.f + glm::vec3{0.f, 7.f, 0.f};
        o.pitch = 0.8f; audio.playOpts("caller_clang", o);
    }
    // Down on its side, it is in the way
    if (c.grounded()) {
        glm::vec3 d = player.position - c.pos; d.y = 0.f;
        float len = glm::length(d), r = Caller::BELL_R + 1.f;
        if (len < r && player.position.y < c.pos.y + Caller::BELL_R) player.position += (len > 0.05f ? d / len : glm::vec3{1.f, 0.f, 0.f}) * (r - len);
    }
}

// Punch THE CALLER as its dive reaches you: it goes down on its side
inline bool GameplayState::parryCaller() {
    if (!plain() || gleanerPhase != GleanerPhase::CALLER || !caller.parryable()) return false;
    const glm::vec3 eye = player.camera.position, fwd = player.camera.forward();
    glm::vec3 d = caller.pos - eye;
    float dist = glm::length(d);
    if (dist > Caller::PARRY_REACH || dist < 0.01f || glm::dot(fwd, d / dist) < 0.2f) return false;
    caller.parried(d);
    parryFeedback(eye + fwd * 1.2f, true);
    styleSystem.addStyle(90.f, StyleSource::PARRY);
    gainXp(40);
    hitStopFrames = std::max(hitStopFrames, 5);
    SfxMixer::Opts o; o.group = SoundGroup::ENEMY; o.positional = true; o.pos = caller.pos; o.priority = true; o.pitch = 1.1f;
    audio.playOpts("caller_clang", o);
    ui.toast("PARRIED", "IT IS DOWN - ITS LAMP IS OPEN", {1.f, 0.75f, 0.2f}, 1.8f);
    caller.events.clear();   // (its PARRIED event: read here)
    return true;
}

// A round meeting the brood member fighting now (fireWeapon and the SPLINTER ask before anything further along)
inline bool GameplayState::shootBrood(glm::vec3 o, glm::vec3 d, float& nearest, float dmg, int w, bool splinter) {
    if (!plain()) return false;
    if (gleanerPhase == GleanerPhase::TENDER) return shootTender(o, d, nearest, dmg, w);
    if (gleanerPhase == GleanerPhase::SUMMIT) {
        float t = 0.f;
        int zone = eye.ray(o, d, nearest, t, splinter);
        if (zone < 0) return false;
        glm::vec3 at = o + d * t;
        eye.hurt(dmg * EyeCore::zoneMult(zone), zone);
        if (zone == 2) { fx.spawnBurst(at, {3.f, 0.5f, 0.2f}, 12, 7.f, 0.4f, 10.f); ui.onHit(true); audio.play("hit_head", 90, SoundGroup::UI); styleSystem.addStyle(12.f, weaponSource((WeaponId)w)); }
        else { fx.spawnHitSparks(at, {1.f, 0.6f, 0.4f}); ui.onHit(false); audio.play("hit_armor", 80, SoundGroup::UI); }
        nearest = t;
        return true;
    }
    if (gleanerPhase >= GleanerPhase::HEART) return shootOrgan(o, d, nearest, dmg, w, splinter);
    float t = 0.f;
    if (gleanerPhase == GleanerPhase::SIEVE && sieve.fighting()) {
        int zone = sieve.ray(o, d, nearest, t, splinter);
        if (zone < 0) return false;
        glm::vec3 at = o + d * t;
        sieve.hurt(dmg * Sieve::zoneMult(zone), zone);
        if (zone == 2) { fx.spawnBurst(at, {2.6f, 1.f, 0.3f}, 12, 7.f, 0.4f, 12.f); ui.onHit(true); audio.play("hit_head", 90, SoundGroup::UI); }
        else if (zone == 0) { fx.spawnHitSparks(at, {1.f, 0.55f, 0.2f}); ui.onHit(false); audio.play("hit_armor", 80, SoundGroup::UI); }
        else { fx.spawnHitSparks(at, gunkit::glowOf(w)); ui.onHit(false); audio.play("hit_body", 90, SoundGroup::UI); }
        styleSystem.addStyle(zone == 2 ? 12.f : 3.f, weaponSource((WeaponId)w));
        nearest = t;
        return true;
    }
    if (gleanerPhase == GleanerPhase::CALLER && caller.fighting()) {
        int zone = caller.ray(o, d, nearest, t, splinter);
        if (zone < 0) return false;
        glm::vec3 at = o + d * t;
        caller.hurt(dmg * Caller::zoneMult(zone), zone);
        if (zone == 2) { fx.spawnBurst(at, {3.f, 0.4f, 0.2f}, 12, 7.f, 0.4f, 10.f); ui.onHit(true); audio.play("hit_head", 90, SoundGroup::UI); }
        else {
            fx.spawnHitSparks(at, {1.f, 0.8f, 0.5f}); ui.onHit(false);
            if (callerClangCd <= 0.f) {   // its shell rings when you hit it (not every pellet)
                callerClangCd = 0.18f;
                SfxMixer::Opts so; so.group = SoundGroup::ENEMY; so.positional = true; so.pos = caller.pos; so.volume = 0.7f; so.pitch = 0.9f + 0.2f * caller.rnd();
                audio.playOpts("caller_clang", so);
            }
        }
        styleSystem.addStyle(zone == 2 ? 14.f : 2.f, weaponSource((WeaponId)w));
        nearest = t;
        return true;
    }
    return false;
}

// The brood's weak point, for a SPLINTER: the Tender's lip while it pours (else its head), the Sieve's heart, the Caller's lamp
inline bool GameplayState::broodSplinterPoint(glm::vec3& at) const {
    if (!plain()) return false;
    if (gleanerPhase == GleanerPhase::SUMMIT) { if (!eye.fighting()) return false; at = eye.pos + eye.facing * (EyeCore::R * 0.9f); return true; }
    if (gleanerPhase >= GleanerPhase::HEART) return organSplinterPoint(at);
    if (gleanerPhase == GleanerPhase::TENDER && tender.fighting()) {
        if (tender.lipOpen()) { at = tender.lipPoint(); return true; }
        Tender::Box h = tender.headBox();
        at = (h.lo + h.hi) * 0.5f;
        return true;
    }
    if (gleanerPhase == GleanerPhase::SIEVE && sieve.fighting()) { at = sieve.heart(); return true; }
    if (gleanerPhase == GleanerPhase::CALLER && caller.fighting()) { at = caller.lamp(); return true; }
    return false;
}

inline void GameplayState::gatherGleanerFight(std::vector<BoxInstance>& out) {
    using namespace rig;
    const float ball = (float)meshlib::lumpBall();
    if (gleanerPhase >= GleanerPhase::TENDER) {
        tenderrig::draw(out, tender, ball);
        tenderrig::drawRiver(out, tender, ball);
    }
    if (gleanerPhase >= GleanerPhase::SIEVE) sieverig::draw(out, sieve, ball);
    if (gleanerPhase >= GleanerPhase::CALLER && gleanerPhase <= GleanerPhase::BROOD) callerrig::draw(out, caller, ball);
    if (gleanerPhase >= GleanerPhase::HEART && gleanerPhase <= GleanerPhase::CORE) gatherGut(out);
    if (gleanerPhase == GleanerPhase::SUMMIT) { if (insideGleaner()) gatherGut(out); gatherEye(out); }
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
    {   // The next place to reach: a ring of chevrons turning on it and a column of light over it
        glm::vec3 at; const char* label = nullptr; bool beacon = false;
        if (gleanerObjective(at, label, beacon) && beacon) {
            const float pulse = 0.65f + 0.35f * std::sin(gameClock * 4.f);
            const glm::vec3 gold{1.f, 0.85f, 0.35f}, foot{at.x, at.y + 0.08f, at.z};
            for (int k = 0; k < 12; ++k) {
                float a = gameClock * 0.8f + k * 0.5236f, r = 3.2f + 0.4f * std::sin(gameClock * 3.f);
                push(out, T(foot + glm::vec3{std::cos(a) * r, 0.f, std::sin(a) * r}) * RY(-a) * S(glm::vec3{0.18f, 0.06f, 0.9f}), glm::vec3{0.f}, gold * 2.2f * pulse);
            }
            push(out, T(foot + glm::vec3{0.f, 14.f, 0.f}) * S(glm::vec3{0.12f, 28.f, 0.12f}), glm::vec3{0.f}, gold * 1.6f * pulse);
            for (int k = 0; k < 3; ++k) {   // chevrons sliding down the light onto the spot
                float u = std::fmod(gameClock * 0.6f + k / 3.f, 1.f);
                glm::vec3 c = foot + glm::vec3{0.f, 1.2f + 9.f * (1.f - u), 0.f};
                for (float sd : {-1.f, 1.f})
                    push(out, T(c + glm::vec3{sd * 0.45f, 0.35f, 0.f}) * RY(gameClock) * RZ(sd * 0.75f) * S(glm::vec3{0.12f, 1.1f, 0.12f}), glm::vec3{0.f}, gold * 2.f * (1.f - u * 0.6f));
            }
        }
    }
    // The updraft: a pillar of heat you can see from across the plain, embers pouring up it
    if (gleanerPhase >= GleanerPhase::CLIMB && deckWallsFrom >= 0 && summitEndT < 0.f) {
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
