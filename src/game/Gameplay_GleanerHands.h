#pragma once
// =============================================================================
// Gameplay_GleanerHands.h — GameplayState: the Gleaner itself in the fight on its
// back, and what binds its brood to it (docs/chapter-4-design.md §5, phase 2).
//
//   THE SWAT    while you fight the Tender on its roof, or the Sieve on its
//               shoulders, it lifts a fist up over its own back and brings it
//               down at the edge nearest you. Its shadow and a ring of fire on
//               the floor show where (about three seconds: get inward). Under
//               it when it lands: a heavy hit. Then a shockwave runs out across
//               the whole floor: jump it. Whatever of its brood is under the
//               fist is crushed too: a big hit and a stagger. Lure them there.
//   THE TETHERS each of the brood is tied to one of its limbs by a cable of
//               glowing slag (the Tender its left arm, the Sieve its right, the
//               Caller its head). Kill one and the cable burns out along its
//               length into the limb, and the limb dies: its light goes out, it
//               smokes, and that arm never swats again. The fight gets quieter
//               as you take it apart.
// Included at the end of GameplayState.h (after Gameplay_GleanerFight.h).
// =============================================================================

namespace hands {
constexpr float CRUSH_R = 40.f;     // the fist's footprint where it lands (metres)
constexpr float OUT = 22.f;         // its middle comes down this far outside the floor's edge
constexpr float RING_SPEED = 34.f;  // the shockwave runs out across the floor
constexpr float RING_LIFE = 3.8f;   // (long enough to cross the whole roof)
constexpr float RING_HALF = 1.8f;   // how thick it is (a jump clears it)
}

// The floor the brood fights on now (its roof, its shoulders): false when no swat belongs there
inline bool GameplayState::swatFloor(glm::vec2& lo, glm::vec2& hi, float& y) const {
    if (deckWallsFrom < 0 || !gleaner.kneel) return false;
    if (gleanerPhase == GleanerPhase::TENDER && tender.fighting()) { lo = deckLo; hi = deckHi; y = deckY; return true; }
    if (gleanerPhase == GleanerPhase::SIEVE && sieve.fighting()) { lo = saddleLo; hi = saddleHi; y = saddleY; return true; }
    return false;
}

// Its arms die with their brood (and stay dead on a retry past them)
inline void GameplayState::syncGleanerLimbs() {
    gleaner.arm[0].dead = gleanerPhase > GleanerPhase::TENDER;
    gleaner.arm[1].dead = gleanerPhase > GleanerPhase::SIEVE;
}

// Pick where a swat comes down: the floor's edge nearest you that a living arm reaches (its left arm
// the west edge, its right the east, either the front). Its knuckles' mark, outside the edge
inline bool GameplayState::pickSwat(int& arm, glm::vec3& at) const {
    glm::vec2 lo, hi; float y;
    if (!swatFloor(lo, hi, y)) return false;
    const glm::vec3 p = player.position;
    const float cx = (lo.x + hi.x) * 0.5f;
    struct Edge { float d; int arm; glm::vec3 at; };
    Edge e[3] = {
        {p.x - lo.x, 0, {lo.x - hands::OUT, y, std::clamp(p.z, lo.y + 8.f, hi.y - 8.f)}},
        {hi.x - p.x, 1, {hi.x + hands::OUT, y, std::clamp(p.z, lo.y + 8.f, hi.y - 8.f)}},
        {hi.y - p.z, p.x < cx ? 0 : 1, {std::clamp(p.x, lo.x + 8.f, hi.x - 8.f), y, hi.y + hands::OUT}}};
    if (gleaner.arm[e[2].arm].dead) e[2].arm = 1 - e[2].arm;
    int best = -1;
    for (int i = 0; i < 3; ++i) if (!gleaner.arm[e[i].arm].dead && (best < 0 || e[i].d < e[best].d)) best = i;
    if (best < 0) return false;
    arm = e[best].arm; at = e[best].at;
    return true;
}

// Each frame of the fight on its back: when to swat, what its fist does where it lands, its shockwave,
// the fist you cannot stand inside, the tethers' burn
inline void GameplayState::updateGleanerHands(float dt) {
    syncGleanerLimbs();
    for (float& b : tetherBurn) if (b >= 0.f) b += dt;
    glm::vec2 lo, hi; float y;
    const bool floor = swatFloor(lo, hi, y);
    if (floor && onGleanerBack() && !playerDead && !gleaner.screaming()) {
        swatCd -= dt;
        if (swatCd <= 0.f) {
            int a; glm::vec3 at;
            if (pickSwat(a, at) && gleaner.swat(a, at)) swatCd = (gleanerPhase == GleanerPhase::SIEVE ? 13.f : 15.f) + 5.f * meteors.rnd();
            else swatCd = 2.f;
        }
    }
    // Its fist where it lies on the floor: you cannot stand in it
    for (int a = 0; a < 2; ++a) {
        if (!gleaner.swatDown(a)) continue;
        const glm::vec3 c = gleaner.arm[a].swatAt;
        glm::vec3 d = player.position - c; d.y = 0.f;
        float len = glm::length(d);
        if (len < hands::CRUSH_R - 4.f && std::abs(player.position.y - c.y) < 20.f) player.position += (len > 0.1f ? d / len : glm::vec3{0.f, 0.f, -1.f}) * (hands::CRUSH_R - 4.f - len);
    }
    // The shockwave out of it, across the floor
    for (auto& r : swatRings) {
        r.t += dt;
        if (r.hit || playerDead) continue;
        const float rad = hands::CRUSH_R + r.t * hands::RING_SPEED;
        const glm::vec3 p = player.position;
        float dist = glm::length(glm::vec2{p.x - r.at.x, p.z - r.at.z});
        if (std::abs(dist - rad) < hands::RING_HALF && p.y - r.at.y < 1.1f && p.y - r.at.y > -2.f) {
            r.hit = true;
            damagePlayer(14.f * tune().damage, r.at, 0.25f, 0.03f);
            if (!g_godMode) { glm::vec3 out = glm::normalize(glm::vec3{p.x - r.at.x, 0.f, p.z - r.at.z} + glm::vec3{0.f, 0.f, 0.001f}); player.velocity += out * 8.f + glm::vec3{0.f, 6.f, 0.f}; }
        }
    }
    swatRings.erase(std::remove_if(swatRings.begin(), swatRings.end(), [](const SwatRing& r) { return r.t > hands::RING_LIFE; }), swatRings.end());
}

// Its fist comes up off the plain (a groan, the tell) / down on its own back
inline void GameplayState::onGleanerSwat(const GleanerEvent& e) {
    if (e.kind == GleanerEvent::SWAT_RAISE) {
        gleanerSound("gleaner_groan", e.at, 0.9f, 0.f);
        SfxMixer::Opts o; o.group = SoundGroup::ENEMY; o.priority = true; o.unducked = true; o.pitch = 0.5f; o.floor = 0.7f;
        audio.playOpts("telegraph", o);
        ui.feed("ITS FIST RISES - GET AWAY FROM THE EDGE", {1.f, 0.5f, 0.25f});
        return;
    }
    // Down: everything under it, then the ring
    gleanerSound("gleaner_slam", e.at, 1.f, 0.f);
    SfxMixer::Opts o; o.group = SoundGroup::ENEMY; o.priority = true; o.unducked = true; o.pitch = 0.45f; o.floor = 0.8f;
    audio.playOpts("explosion", o);
    shake(1.4f, 0.07f); hitStopFrames = std::max(hitStopFrames, 4); fovKick = std::max(fovKick, 8.f);
    gleanerDust.push_back({e.at, 0.f, 12.f * Gleaner::SCALE});
    fx.spawnBurst(e.at + glm::vec3{0.f, 3.f, 0.f}, {2.4f, 0.8f, 0.2f}, 80, 24.f, 1.2f, 12.f);
    auto under = [&](glm::vec3 q) { return glm::length(glm::vec2{q.x - e.at.x, q.z - e.at.z}) < hands::CRUSH_R && std::abs(q.y - e.at.y) < 6.f; };
    swatRings.push_back({e.at, 0.f, under(player.position)});   // (crushed already: its ring does not hit you again)
    if (!playerDead && under(player.position)) {
        damagePlayer(38.f * tune().damage, e.at, 0.5f, 0.08f);
        if (!g_godMode) {
            glm::vec3 in = glm::vec3{(deckLo.x + deckHi.x) * 0.5f, 0.f, (deckLo.y + deckHi.y) * 0.5f} - glm::vec3{e.at.x, 0.f, e.at.z};
            if (gleanerPhase == GleanerPhase::SIEVE) in = glm::vec3{(saddleLo.x + saddleHi.x) * 0.5f, 0.f, (saddleLo.y + saddleHi.y) * 0.5f} - glm::vec3{e.at.x, 0.f, e.at.z};
            player.velocity = glm::normalize(in + glm::vec3{0.001f, 0.f, 0.f}) * 18.f + glm::vec3{0.f, 9.f, 0.f};
            player.onGround = false;
        }
    }
    // Its own brood, caught under it
    const float CRUSH = 480.f;
    if (gleanerPhase == GleanerPhase::TENDER && tender.fighting() && under(tender.pos)) {
        tender.hurt(CRUSH, 0); if (tender.fighting()) tender.stagger();
        ui.toast("IT CRUSHED ITS OWN", "THE TENDER IS STAGGERED", {1.f, 0.7f, 0.25f}, 2.f);
        styleSystem.addStyle(150.f, StyleSource::PARRY);
    }
    if (gleanerPhase == GleanerPhase::SIEVE && sieve.fighting() && under(sieve.pos)) {
        sieve.hurt(CRUSH, 0); if (sieve.fighting()) sieve.topple();
        ui.toast("IT CRUSHED ITS OWN", "THE SIEVE IS DOWN", {1.f, 0.7f, 0.25f}, 2.f);
        styleSystem.addStyle(150.f, StyleSource::PARRY);
    }
}

// A brood member is dead: its tether burns out into its limb, and the limb dies
inline void GameplayState::killGleanerLimb(int which) {
    tetherBurn[which] = 0.f;
    glm::vec3 at = which < 2 ? gleaner.shoulder(which) : gleaner.toWorld(gleaner::heldHead());
    gleanerDust.push_back({at, 0.f, 20.f * Gleaner::SCALE});
    for (int k = 0; k < 3; ++k) gleanerDust.push_back({at + glm::vec3{meteors.rnd(-40.f, 40.f), meteors.rnd(-30.f, 10.f), meteors.rnd(-40.f, 40.f)}, -0.3f * k, 12.f * Gleaner::SCALE});
    const char* limb = which == 0 ? "ITS LEFT ARM DIES" : which == 1 ? "ITS RIGHT ARM DIES" : "ITS VOICE DIES";
    ui.feed(limb, {1.f, 0.45f, 0.25f});
}

// The cable from a brood member to its limb: a sagging line of glowing slag, light running along it
// toward the limb (it feeds it). Burning out: a white flare that runs into the limb, then it falls away
inline void GameplayState::drawTether(std::vector<BoxInstance>& out, glm::vec3 from, glm::vec3 to, float life, float burn, float hurt) {
    using namespace rig;
    if (burn > 3.f) return;
    const float ball = (float)meshlib::lumpBall();
    const int N = 26;
    const float sag = 0.18f * glm::length(to - from) + (burn > 0.f ? 60.f * burn * burn : 0.f);   // (burnt: it drops away)
    const glm::vec3 mid = (from + to) * 0.5f - glm::vec3{0.f, sag, 0.f};
    const glm::vec3 slag{2.4f, 0.65f, 0.15f}, white{3.2f, 2.6f, 1.8f};
    const float flicker = 1.f - 0.5f * hurt * (0.5f + 0.5f * std::sin(gameClock * 37.f));
    for (int k = 0; k < N; ++k) {
        float u = (k + 0.5f) / N;
        glm::vec3 p = (1.f - u) * (1.f - u) * from + 2.f * u * (1.f - u) * mid + u * u * to;
        float u2 = (k + 1.5f) / N;
        glm::vec3 q = (1.f - u2) * (1.f - u2) * from + 2.f * u2 * (1.f - u2) * mid + u2 * u2 * to;
        glm::vec3 d = q - p;
        float len = glm::length(d);
        if (len < 0.01f) continue;
        glm::vec3 glow = slag * 0.5f * flicker * life;
        float pulse = std::fmod(gameClock * 0.7f - u + 1.f, 0.25f) / 0.25f;   // light running along it, to the limb
        glow += slag * std::pow(pulse, 6.f) * 1.4f * life;
        if (burn >= 0.f) {   // the burn: a white front running from the brood into the limb, then out
            float front = burn / 1.1f;
            glow = u < front ? slag * std::max(0.f, 1.f - (burn - u * 1.1f) * 1.2f) * 0.6f : slag * 0.4f;
            if (std::abs(u - front) < 0.06f) glow = white * 2.f;
        }
        float w = 0.9f + 1.2f * u;   // thicker toward the limb
        push(out, gleaner::alongZ((p + q) * 0.5f, d) * S(glm::vec3{w, w, len * 1.05f}), glm::vec3{0.06f, 0.035f, 0.03f}, glow);
        if (k % 5 == 2) { push(out, T(p) * S(glm::vec3{w * 2.2f}), glm::vec3{0.05f, 0.03f, 0.025f}, glow * 0.6f); out.back().mesh = ball; }   // clinkers of slag on it
    }
}

// The swat's tell on the floor (its shadow, a ring of fire tightening), its shockwave, and the tethers
inline void GameplayState::gatherGleanerHands(std::vector<BoxInstance>& out) {
    using namespace rig;
    const float ball = (float)meshlib::lumpBall();
    for (int a = 0; a < 2; ++a) {
        if (!gleaner.swatting(a) || gleaner.swatDown(a)) continue;
        const float u = gleaner.swatTell(a);
        if (u >= 1.f) continue;
        const glm::vec3 c = gleaner.arm[a].swatAt;
        const float pulse = 0.6f + 0.4f * std::sin(gameClock * (6.f + 20.f * u));
        // its shadow, darkening and spreading as the fist comes over
        push(out, T(c + glm::vec3{0.f, 0.08f, 0.f}) * S(glm::vec3{hands::CRUSH_R * 2.f * (0.5f + 0.5f * u), 0.05f, hands::CRUSH_R * 2.f * (0.5f + 0.5f * u)}), glm::vec3{0.01f, 0.005f, 0.005f} * (1.f - u), glm::vec3{0.f});
        out.back().mesh = ball;
        for (int k = 0; k < 40; ++k) {   // the ring where it will crush: fire, faster as it comes
            float ang = k * 0.15708f, r = hands::CRUSH_R;
            push(out, T(c + glm::vec3{std::cos(ang) * r, 0.15f, std::sin(ang) * r}) * RY(-ang) * S(glm::vec3{0.6f, 0.2f, r * 0.17f}), glm::vec3{0.6f, 0.15f, 0.05f}, glm::vec3{2.8f, 0.6f, 0.1f} * pulse * (0.5f + 0.8f * u));
        }
        for (int k = 0; k < 6; ++k) {   // arrows on the floor pointing in, away from it
            float ang = k * 1.0472f + 0.5f;
            glm::vec3 q = c + glm::vec3{std::cos(ang), 0.f, std::sin(ang)} * (hands::CRUSH_R + 4.f);
            push(out, T(q + glm::vec3{0.f, 0.12f, 0.f}) * RY(std::atan2(std::cos(ang), std::sin(ang))) * S(glm::vec3{1.f, 0.1f, 5.f}), glm::vec3{0.f}, glm::vec3{2.6f, 1.6f, 0.4f} * pulse);
        }
    }
    for (const auto& r : swatRings) {   // its shockwave: a low wall of dust and fire, running out
        const float rad = hands::CRUSH_R + r.t * hands::RING_SPEED, fade = 1.f - r.t / hands::RING_LIFE;
        const int N = 64;
        for (int k = 0; k < N; ++k) {
            float ang = k * 6.2831853f / N;
            glm::vec3 q = r.at + glm::vec3{std::cos(ang) * rad, 0.5f, std::sin(ang) * rad};
            push(out, T(q) * RY(-ang) * S(glm::vec3{1.2f, 1.f + 0.6f * fade, rad * 6.2831853f / N * 1.1f}), glm::vec3{0.25f, 0.15f, 0.1f} * fade, glm::vec3{2.4f, 0.7f, 0.15f} * fade);
        }
    }
    // The tethers
    if (deckWallsFrom < 0 || gleanerPhase > GleanerPhase::HEART) return;
    if (tender.alive() || tetherBurn[0] >= 0.f) {
        glm::vec3 from = tender.lipPoint() - tender.fwd() * 1.5f;
        drawTether(out, from, gleaner.shoulder(0) + glm::vec3{0.f, 20.f, 0.f}, 1.f, tetherBurn[0], tender.alive() ? 1.f - tender.health01() : 0.f);
    }
    if ((sieve.alive() && sieve.state != Sieve::RISING) || tetherBurn[1] >= 0.f) {
        drawTether(out, sieve.hub() + glm::vec3{0.f, 2.f, 0.f}, gleaner.shoulder(1) + glm::vec3{0.f, 20.f, 0.f}, 1.f, tetherBurn[1], sieve.alive() ? 1.f - sieve.health01() : 0.f);
    }
    if ((caller.alive() && caller.state != Caller::ARRIVING) || tetherBurn[2] >= 0.f) {
        drawTether(out, caller.pos + caller.axis * (Caller::BELL_H * 0.5f), gleaner.toWorld(gleaner::heldHead() + glm::vec3{0.f, 14.f, 0.f}), 1.f, tetherBurn[2], caller.alive() ? 1.f - caller.health01() : 0.f);
    }
    (void)ball;
}
