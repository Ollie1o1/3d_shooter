#pragma once
// =============================================================================
// Gameplay_Gleaner.h — GameplayState: THE GLEANER on the plain (GleanerState.h,
// GleanerRig.h) and the arrival cutscene that reveals it (PlainReveal.h).
//
// Not a fight yet (that is the next level-design step): it crawls the north of
// the plain between three marks, screams now and then, and every fist that
// lands booms across the plain (late, at the speed of sound) and shakes the
// ground if you are near. You cannot walk through it.
// Included at the end of GameplayState.h.
// =============================================================================

// The end of Chapter 4's timings (Gameplay_GleanerFinale.h)
namespace finale {
// THE TEARING
constexpr float LOCK = 0.35f;             // the camera onto its pupil
constexpr float PUNCH1 = 0.4f, PUNCH2 = 1.15f;
constexpr float REACH = 1.9f, GRIP = 2.45f;
constexpr float PULL0 = 2.45f, PULL1 = 4.f;
constexpr int   THREADS = 7;              // what holds its core in, snapping as you pull
inline float snapAt(int k) { return 2.85f + k * 0.17f + 0.04f * (float)(k % 3); }
constexpr float BEAT0 = 4.15f;            // in your hand: it beats (three times)
constexpr float CRUSH = 5.3f;             // you crush it
constexpr float LEN = 5.55f;              // then the white of the fall
// THE QUIET
constexpr float FADE = 2.2f;              // up out of the dark
constexpr float SEAM = 3.6f, OPEN1 = 7.f; // the portal: a seam of white in the air, opening
constexpr float PULL_R = 30.f;            // close: it draws you in
constexpr float THROUGH = 1.4f;           // white, then the card
constexpr float RADIUS = 21.f;
inline float sm(float a, float b, float x) { float u = std::clamp((x - a) / (b - a), 0.f, 1.f); return u * u * (3.f - 2.f * u); }
}  // namespace finale

namespace gleanerplace {
inline glm::vec3 lair() { return {0.f, 0.f, -420.f}; }   // where it lies at the arrival, its back to you
inline glm::vec3 mark(int i) {
    static const glm::vec3 M[3] = {{-520.f, 0.f, -620.f}, {520.f, 0.f, -660.f}, {0.f, 0.f, -420.f}};
    return M[((i % 3) + 3) % 3];
}
}

inline void GameplayState::beginGleaner() {
    gleanerDust.clear();
    gleanerWaypoint = 0;
    gleanerScreamCd = 35.f;
    // It plays on a fresh arrival (and in practice from the first checkpoint, as Chapter 3's does);
    // never again once seen in a run or saved as seen
    const bool devFight = g_devOverlay == "plainclose" || g_devOverlay == "plaindeck" || g_devOverlay == "plainsaddle" || g_devOverlay == "plaincrown" || g_devOverlay == "plainsummit";   // (dev: straight to the fight)
    if (devFight) { wakeSeen = plainRevealDone = true; }
    const bool reveal = (!wakeSeen || g_practice) && !plainRevealDone && director.arena == 0 && storyCheckpoint == 0 && !g_devCam && g_bossRun < 0;   // (a later checkpoint is past it)
    if (reveal || g_devOverlay.rfind("plainreveal", 0) == 0) {
        gleaner.place(gleanerplace::lair(), 3.14159265f, 0.f);   // lying curled, facing away: the ridge
        gleaner.eyeOpen = gleaner.ribs = 0.f;
        plainReveal.start();
        if (g_devOverlay.rfind("plainreveal", 0) == 0) {   // --overlay plainrevealNN: start NN seconds in (screenshots)
            float at = (float)std::atof(g_devOverlay.c_str() + 11);
            plainReveal.time = std::clamp(at, 0.f, PlainReveal::DURATION - 0.1f);
            plainReveal.skipReady = false;
        }
        director.hold = true;
    } else {
        gleaner.place(gleanerplace::lair(), 0.f, 1.f);   // already awake, facing you
        if (storyCheckpoint > 0) wakeSeen = true;   // (past the arrival: it has been seen)
        gleaner.eyeOpen = 1.f; gleaner.ribs = 0.25f;
        plainReveal = PlainReveal{};
    }
    // Where the fight stands: a first start (or a Continue) picks it up from the save; a retry keeps it
    resetGleanerFight(!gleanerFightStarted);
    gleanerFightStarted = true;
}

// The cutscene's tick: it has the camera and the Gleaner
inline void GameplayState::updatePlainReveal(float dt) {
    PlainReveal& r = plainReveal;
    const float t0 = r.time;
    const bool done = r.update(dt, storySkipKey);
    const float t = done ? PlainReveal::DURATION : r.time;
    auto crossed = [&](float at) { return t0 < at && t >= at; };
    if (!done) {
        player.position = plain::start(); player.velocity = {0.f, 0.f, 0.f};
        {   // The camera: it lands, steadies, looks round, then at what stands up
            player.camera.yaw = r.yaw(); player.camera.pitch = r.pitch();
            player.camera.position = player.position + glm::vec3{0.f, r.eye(), 0.f}; prevCamPos = player.camera.position;
        }
        if (crossed(0.02f)) {   // out of the portal: the air of the plain hits, a boom rolling off across it
            SfxMixer::Opts o; o.group = SoundGroup::UI; o.priority = true; o.pitch = 0.45f; o.volume = 1.f;
            audio.playOpts("explosion", o); o.pitch = 0.6f; audio.playOpts("boost", o);
        }
        if (crossed(PlainReveal::LAND)) { audio.play("land_heavy", 128, SoundGroup::PLAYER, true); audio.play("land_metal", 110, SoundGroup::PLAYER, true); shake(0.45f, 0.04f); }
        if (crossed(PlainReveal::METEOR)) {   // one comes down out to the left while it looks that way
            glm::vec3 at = plain::start() + glm::vec3{-240.f, 0.f, -150.f};
            int i = meteors.launch(at, 10.f);
            if (i >= 0) { auto& m = meteors.pool[i]; m.pos = at - m.vel * 1.1f; m.prev = m.pos; m.age = 3.f; }
        }
        if (crossed(PlainReveal::RISE0)) {
            gleanerSound("gleaner_groan", gleaner.toWorld({0.f, 60.f, 0.f}), 1.f, 0.6f);
            shake(3.6f, 0.025f);
        }
        if (crossed(PlainReveal::BLINK0)) {
            SfxMixer::Opts o; o.group = SoundGroup::UI; o.priority = true; o.volume = 1.f;
            audio.playOpts("gleaner_blink", o);
        }
        if (crossed(PlainReveal::SCREAM)) gleaner.scream();
        // The Gleaner follows the clock: up, round fist over fist (fast strides), the eye, the ribs
        gleaner.rise = r.rise();
        gleaner.gaitPeriod = r.time < PlainReveal::TURN1 + 1.f ? 3.4f : 8.f;
        gleaner.yaw = gleaner.targetYaw = 3.14159265f * (1.f - r.turn());
        gleaner.eyeOpen = r.eyeOpen();
        gleaner.ribs = r.ribs();
    }
    if (done) {   // played through or skipped: everything where the scene leaves it
        gleaner.rise = 1.f; gleaner.yaw = gleaner.targetYaw = 0.f; gleaner.eyeOpen = 1.f; gleaner.ribs = 1.f;
        gleaner.gaitPeriod = 8.f;
        for (int a = 0; a < 2; ++a) if (!gleaner.arm[a].swinging) gleaner.arm[a].plant = gleaner.home(a, 0.f);
        player.position = plain::start(); player.velocity = {0.f, 0.f, 0.f};
        player.camera.yaw = -90.f; player.camera.pitch = 2.f;
        player.camera.position = player.position + glm::vec3{0.f, 1.7f, 0.f}; prevCamPos = player.camera.position;
        director.hold = false;
        wakeSeen = true; plainRevealDone = true; saveStoryProgress();
        pushBanner("THE GLEANER", "", {1.f, 0.3f, 0.18f}, 2.6f);
        gleanerScreamCd = 16.f;
    }
}

// A sound from the Gleaner, out across the plain: placed toward it (inside the mixer's reach), as loud
// as `loud` at the far edge of the plain and louder close, `late` the share of the real sound delay
// `roar`: its scream, the loudest thing in the game wherever you are (inside it too): full level, and
// it is never dipped by the duck it calls to clear the mix for itself
inline void GameplayState::gleanerSound(const char* name, glm::vec3 from, float loud, float late, bool roar) {
    const glm::vec3 ear = player.camera.position;
    glm::vec3 to = from - ear;
    float dist = glm::length(to);
    SfxMixer::Opts o;
    o.group = SoundGroup::WORLD; o.positional = true; o.priority = true;
    o.pos = ear + (dist > 1.f ? to / dist : glm::vec3{0.f, 0.f, -1.f}) * std::min(dist, 30.f);
    float near = glm::clamp(1.f - dist / 2200.f, 0.f, 1.f);
    o.floor = glm::clamp(loud * (0.45f + 0.55f * near), 0.05f, 1.f);
    o.volume = 1.f;
    o.delay = late * dist / 343.f;
    if (roar) {
        o.unducked = true; o.floor = 1.f;
        o.pos = ear + (dist > 1.f ? to / dist : glm::vec3{0.f, 0.f, -1.f}) * SfxMixer::REF_DIST;   // (full level, and bright: no distance low-pass)
    }
    audio.playOpts(name, o);
}

inline void GameplayState::updateGleaner(float dt) {
    Gleaner& g = gleaner;
    if (!plainReveal.active()) {   // the fight has it now (Gameplay_GleanerFight.h)
        g.ribs += (0.25f - g.ribs) * std::min(1.f, dt * 0.2f);
        updateGleanerFight(dt);
    }
    g.tick(dt);
    for (const auto& e : g.events) {
        float dist = glm::length(e.at - player.camera.position);
        if (e.kind == GleanerEvent::SLAM) {
            gleanerSound("gleaner_slam", e.at, 0.35f + 0.5f * e.size, 1.f);
            float near = glm::clamp(1.f - dist / 1600.f, 0.f, 1.f);
            if (near > 0.f) plainQuakes.push_back({dist / 343.f, glm::clamp(near * near * e.size * 0.9f, 0.05f, 1.f)});
            gleanerDust.push_back({e.at, 0.f, 26.f * Gleaner::SCALE * e.size});
        } else if (e.kind == GleanerEvent::SWAT_RAISE || e.kind == GleanerEvent::SWAT) {
            onGleanerSwat(e);
        } else {   // the scream: the loudest thing in the game, now (no delay: it has to land with its jaw)
            gleanerSound("gleaner_scream", e.at, 1.f, 0.f, true);
            audio.duck(14.f, Gleaner::SCREAM_LEN + 1.f);   // (the music and the fight step back under it)
            float near = glm::clamp(1.f - dist / 1800.f, 0.25f, 1.f);
            shake(Gleaner::SCREAM_LEN, 0.05f * near);
            fovKick = std::max(fovKick, 7.f * near);   // the air itself hits you
        }
    }
    for (auto& d : gleanerDust) d.age += dt;
    gleanerDust.erase(std::remove_if(gleanerDust.begin(), gleanerDust.end(), [](const GleanerDust& d) { return d.age > 4.f; }), gleanerDust.end());

    gleanerFistHitCd = std::max(0.f, gleanerFistHitCd - dt);
    if (gleanerHaulT > 0.f) gleanerHaulT -= dt;
    if (gleanerHaulRise > 0.f) {   // up beside the roof's edge: now the throw over the parapet onto it
        gleanerHaulRise -= dt;
        if (gleanerHaulRise <= 0.f && !playerDead) {
            const float G = -24.f, T = 1.3f;
            glm::vec3 d = gleanerHaulLand - player.position;
            player.velocity = {d.x / T, (d.y - 0.5f * G * T * T) / T, d.z / T};
            player.launched(); player.onGround = false;
            SfxMixer::Opts o; o.group = SoundGroup::PLAYER; o.pitch = 0.6f; o.priority = true;
            audio.playOpts("boost", o);
        }
    }
    if (gleanerHaulRope > 0.f) gleanerHaulRope -= dt;
    if (insideGleaner()) return;
    // You cannot walk through it: out of its body (its own frame) and away from its fists.
    // Each pushes you out the nearest way. (It used to shove you out of its body sideways,
    // up to 140 m in a frame, into the reach of its fist, and the two held you there.)
    const float K = Gleaner::SCALE;
    const bool above = g.kneel > 0.5f && deckWallsFrom >= 0 && player.position.y > std::min(deckY, saddleY) - 10.f;   // (on its back, or carried up to it)
    if (above || gleanerHaulT > 0.f) return;   // (a haul's flight passes it)
    const float BX = 58.f * K, BF = 112.f * K, BB = -205.f * K, FIST_R = 24.f * K, FIST_TOP = 34.f * K;
    for (int pass = 0; pass < 3; ++pass) {   // (where a fist meets its body there is a corner: a few passes settle it)
        if (player.position.y < 160.f * K) {
            glm::vec3 rel = player.position - g.pos;
            float lx = glm::dot(rel, g.right()), lz = glm::dot(rel, g.fwd());
            if (std::abs(lx) < BX && lz > BB && lz < BF) {
                const float left = lx + BX, right = BX - lx, front = BF - lz, back = lz - BB;
                const float m = std::min({left, right, front, back});
                if (m == left) player.position -= g.right() * left;
                else if (m == right) player.position += g.right() * right;
                else if (m == front) player.position += g.fwd() * front;
                else player.position -= g.fwd() * back;
            }
        }
        for (int a = 0; a < 2; ++a) {
            const glm::vec3 hand = g.hand(a);
            if (hand.y > FIST_TOP || player.position.y > hand.y + FIST_TOP) continue;
            glm::vec3 off = player.position - hand; off.y = 0.f;
            float len = glm::length(off);
            if (len >= FIST_R) continue;
            glm::vec3 out = len > 0.1f ? off / len : glm::normalize(glm::vec3{player.position.x - g.pos.x, 0.f, player.position.z - g.pos.z} + glm::vec3{0.001f, 0.f, 0.f});
            const float deep = FIST_R - len;
            // A fist that comes down on you knocks you away (it doesn't pick you up and put you somewhere)
            if (pass == 0 && deep > 1.5f && g.arm[a].swinging && gleanerFistHitCd <= 0.f && !playerDead) {
                gleanerFistHitCd = 1.5f;
                damagePlayer(30.f * tune().damage, hand, 0.4f, 0.06f);
                if (!g_godMode) player.velocity = out * 26.f + glm::vec3{0.f, 11.f, 0.f};
                player.onGround = false;
            }
            player.position += out * deep;
        }
    }
}

// Where a throw onto its back lands: just inside the parapet's gap; the saddle (the Sieve); its crown (the Caller, the brood dead)
inline glm::vec3 GameplayState::gleanerBackTarget() const {
    return gleanerPhase == GleanerPhase::SIEVE ? saddleCheckpoint()
         : gleanerPhase >= GleanerPhase::CALLER && gleanerPhase <= GleanerPhase::BROOD ? crownCheckpoint() : deckCheckpoint();
}

// The grapple's ray against the Gleaner: its arms (bone by bone), its fists, its hunched body.
// Out to 220 m: it is a mountain, and the hook is the way up it.
inline bool GameplayState::gleanerHookPoint(glm::vec3& at, float& dist) const {
    if (!plain() || plainReveal.active() || insideGleaner() || onGleanerBack() || gleaner.rise < 0.99f) return false;
    if (gleanerPhase > GleanerPhase::BROOD) return false;   // (inside it: its gut has its own walls)
    const Gleaner& g = gleaner;
    const glm::vec3 o = player.camera.position, d = player.camera.forward();
    const float K = Gleaner::SCALE;
    float best = 220.f;
    // Ray against a capsule (a bone): the nearest approach of the ray to the segment, then back along the ray to the skin
    auto capsule = [&](glm::vec3 a, glm::vec3 b, float r) {
        glm::vec3 u = b - a, w = o - a;
        float uu = glm::dot(u, u), ud = glm::dot(u, d), uw = glm::dot(u, w), dw = glm::dot(d, w);
        float den = uu - ud * ud;   // (d is unit length)
        float t = den > 1e-6f ? (ud * uw - uu * dw) / den : -dw;
        t = std::max(t, 0.f);
        float s = uu > 1e-6f ? std::clamp((glm::dot(o + d * t - a, u)) / uu, 0.f, 1.f) : 0.f;
        glm::vec3 q = a + u * s;
        t = std::max(0.f, glm::dot(q - o, d));   // (again from the clamped point)
        float miss = glm::length(o + d * t - q);
        if (miss > r) return;
        float hit = t - std::sqrt(std::max(0.f, r * r - miss * miss));
        if (hit > 0.5f && hit < best) best = hit;
    };
    for (int a = 0; a < 2; ++a) {
        const glm::vec3 sh = g.shoulder(a), el = g.elbow(a), hd = g.hand(a) + glm::vec3{0.f, 14.f * K, 0.f};
        capsule(sh, el, 24.f * K);
        capsule(el, hd, 21.f * K);
        capsule(hd, hd, 19.f * K);
    }
    {   // its body: a box in its own frame, as it sits now (kneeling, it is lower)
        glm::vec3 rel = o - g.pos;
        glm::vec3 lo{glm::dot(rel, g.right()), rel.y, glm::dot(rel, g.fwd())}, ld{glm::dot(d, g.right()), d.y, glm::dot(d, g.fwd())};
        const float base = g.bob();
        AABB box{{-46.f * K, base + 30.f * K, -130.f * K}, {46.f * K, base + 185.f * K, 40.f * K}};
        float t = rayBoxHit(lo, ld, box);
        if (t > 0.5f && t < best) best = t;
    }
    if (best >= 220.f) return false;
    dist = best; at = o + d * best;
    return true;
}

// Hooked into it: the rope snaps taut and hauls you up the side of it to just outside its roof's
// edge on your side, then over the parapet onto the roof (at a spot clear of the chimneys); from
// there the saddle and its crown are a walk. Before it kneels there is nothing to stand on up
// there: a long leap at it instead.
inline void GameplayState::haulOntoGleaner(glm::vec3 at) {
    const float G = -24.f;
    glm::vec3 p = player.position, target;
    float T;
    gleanerHaulRise = 0.f;
    if (deckWallsFrom >= 0 && gleanerPhase >= GleanerPhase::CLIMB) {
        const glm::vec2 c = (deckLo + deckHi) * 0.5f, h = (deckHi - deckLo) * 0.5f;
        const glm::vec2 rel{p.x - c.x, p.z - c.y};
        glm::vec2 land, out;   // where you land on the roof, and the way out from it to your side
        if (std::abs(rel.x) / h.x > std::abs(rel.y) / h.y) {
            if (rel.x < 0.f) { land = {deckLo.x + 14.f, c.y}; out = {-1.f, 0.f}; }   // the west, by the parapet's gap
            else             { land = {deckHi.x - 12.f, c.y + 20.f}; out = {1.f, 0.f}; }
        } else {
            if (rel.y < 0.f) { land = {c.x + 10.f, deckLo.y + 14.f}; out = {0.f, -1.f}; }
            else             { land = {c.x + 20.f, deckHi.y - 14.f}; out = {0.f, 1.f}; }
        }
        gleanerHaulLand = {land.x, deckY + 0.05f, land.y};
        const glm::vec2 edge = out.x != 0.f ? glm::vec2{out.x < 0.f ? deckLo.x : deckHi.x, land.y} : glm::vec2{land.x, out.y < 0.f ? deckLo.y : deckHi.y};
        target = {edge.x + out.x * 18.f, deckY + 22.f, edge.y + out.y * 18.f};
        // (arrive there still just rising: dy/T + G T / 2 = 4)
        const float dy = std::max(1.f, target.y - p.y);
        T = (-4.f + std::sqrt(16.f + 48.f * dy)) / 24.f;
        T = std::max(T, glm::length(glm::vec2{target.x - p.x, target.z - p.z}) / 70.f);
        gleanerHaulRise = T;
        gleanerHaulT = T + 1.6f;   // (its body lets you through on the way)
    } else {
        glm::vec3 flat{at.x - p.x, 0.f, at.z - p.z};
        float len = glm::length(flat);
        float go = std::clamp(len - 75.f, 0.f, 140.f);   // (lands short of its fists)
        target = p + (len > 0.1f ? flat / len : glm::vec3{0.f, 0.f, -1.f}) * go;
        target.y = 0.f;
        T = std::clamp(go / 40.f, 0.9f, 3.2f);
        gleanerHaulT = 0.f;
    }
    glm::vec3 dlt = target - p;
    player.velocity = {dlt.x / T, (dlt.y - 0.5f * G * T * T) / T, dlt.z / T};
    player.launched(); player.onGround = false;
    gleanerHaulAt = at; gleanerHaulRope = std::min(0.9f, T * 0.6f);
    grappleCooldown = GRAPPLE_COOLDOWN;
    viewModel.triggerGrapple();
    audio.play("grapple_fire");
    SfxMixer::Opts o; o.group = SoundGroup::PLAYER; o.pitch = 0.55f; o.priority = true;
    audio.playOpts("hook_yank", o);
    shake(0.3f, 0.02f);
    fovKick = std::max(fovKick, 6.f);
}

// The sky's moon, shut: 0 open (before it wakes), 1 gone (it is the eye now)
inline float GameplayState::plainMoonLid() const {
    if (plainReveal.active()) return plainReveal.lid();
    if (quietT >= 0.f) return 0.f;   // (after it: the real moon)
    if (summitEndT >= 0.f) return 1.f - summitMoonOpen();   // (it dead: the real moon opens again)
    return wakeSeen ? 1.f : 0.f;
}

inline void GameplayState::gatherGleaner(std::vector<BoxInstance>& out) {
    using namespace rig;
    gleaner::Look look;
    look.ball = (float)meshlib::lumpBall();
    look.flash = gleaner.screaming() ? 0.35f * std::max(0.f, std::sin(gleaner.screamT * 9.f)) * gleaner.jaw : 0.f;
    look.watch = player.camera.position;
    look.watching = gleaner.eyeOpen > 0.5f;
    look.frontGap = deckFrontOpen && deckWallsFrom >= 0;
    look.door = furnaceDoor ? 1.f : 0.f;
    look.vent = 0.f;   // (the updraft is in front of its face now, out of the plain: no vent in its side)
    for (int a = 0; a < 2; ++a) {   // its arms die with their brood: the light runs out of them as the tether burns in
        const bool dead = a == 0 ? gleanerPhase > GleanerPhase::TENDER : gleanerPhase > GleanerPhase::SIEVE;
        look.armLife[a] = !dead || gleanerPhase > GleanerPhase::SUMMIT ? 1.f : tetherBurn[a] < 0.f ? 0.f : 1.f - std::clamp((tetherBurn[a] - 1.f) / 1.2f, 0.f, 1.f);
        if (summitEndT >= 0.f || quietT >= 0.f) look.armLife[a] = 0.f;
    }
    look.headLife = gleanerPhase > GleanerPhase::CALLER ? 0.f : 1.f;
    look.shedding = gleaner.rise > 0.001f && gleaner.rise < 0.999f ? std::sin(gleaner.rise * 3.14159f) : 0.f;
    gleaner::draw(out, gleaner, look);
    // The dust a fist throws up: a ring of lumps rolling outward, settling
    for (const auto& d : gleanerDust) {
        float u = d.age / 4.f;
        for (int k = 0; k < 14; ++k) {
            float a = k * 0.4488f + d.size;
            float rr = d.size * (0.6f + 2.4f * std::sqrt(u));
            glm::vec3 p = d.at + glm::vec3{std::cos(a) * rr, d.size * (0.15f + 0.5f * u), std::sin(a) * rr};
            float sz = d.size * (0.35f + 0.6f * u) * (1.f - u * 0.6f);
            push(out, T(p) * RY(a + u) * S(glm::vec3{sz}), glm::vec3{0.22f, 0.13f, 0.1f} * (1.f - 0.5f * u), glm::vec3{0.f});
            out.back().mesh = look.ball;
        }
    }
}

// Over everything in the cutscene: the white of the portal, the bars, its name, the skip prompt
inline void GameplayState::renderPlainReveal() {
    ui.begin2D(); auto& u = ui.ui;
    const PlainReveal& r = plainReveal;
    int bar = (int)(70.f * r.bars());
    if (bar > 0) { u.rect(0, 0, SCREEN_W, bar, {0.02f, 0.f, 0.f, 1}); u.rect(0, SCREEN_H - bar, SCREEN_W, bar, {0.02f, 0.f, 0.f, 1}); }
    if (r.a() > 1.f && r.a() < 7.5f) {
        float a = PlainReveal::sm(1.f, 2.f, r.a()) * (1.f - PlainReveal::sm(6.5f, 7.5f, r.a()));
        u.text("THE BLOOD MOON PLAIN", 48, 24, 2, {1.f, 0.4f, 0.25f, a});
        u.text("CHAPTER 4", 48, 46, 1, {0.9f, 0.6f, 0.5f, a});
    }
    std::string skip = "HOLD " + binds().label(Act::JUMP) + " TO SKIP";
    u.text(skip.c_str(), SCREEN_W / 2, SCREEN_H - 24, 1, {0.7f, 0.45f, 0.4f, 0.8f * r.bars()}, true);
    if (r.skip > 0.f) u.rect(SCREEN_W / 2 - 90, SCREEN_H - 12, (int)(180.f * r.skip / PlainReveal::SKIP_HOLD), 3, {1.f, 0.4f, 0.25f, 1});
    float w = r.white();
    if (w > 0.001f) u.rect(0, 0, SCREEN_W, SCREEN_H, {1.f, 0.96f, 0.92f, w});
    ui.end2D();
}
