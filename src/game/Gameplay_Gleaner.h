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
    const bool devFight = g_devOverlay == "plainclose" || g_devOverlay == "plaindeck";   // (dev: straight to the fight)
    if (devFight) { wakeSeen = plainRevealDone = true; }
    const bool reveal = (!wakeSeen || g_practice) && !plainRevealDone && director.arena == 0 && !g_devCam && g_bossRun < 0;
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
        if (r.prologue()) updatePlainPrologue(t0, t);
        else {   // The camera: it lands, steadies, looks round, then at what stands up
            player.camera.yaw = r.yaw(); player.camera.pitch = r.pitch();
            player.camera.position = player.position + glm::vec3{0.f, r.eye(), 0.f}; prevCamPos = player.camera.position;
        }
        if (crossed(PlainReveal::LAND)) { audio.play("land_heavy", 128, SoundGroup::PLAYER, true); shake(0.35f, 0.03f); }
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
        pushBanner("THE GLEANER", "\"YOU WERE SENT TO ME. YOU ARE LATE.\"", {1.f, 0.3f, 0.18f}, 4.5f);
        gleanerScreamCd = 40.f;
    }
}

// ---- the prologue: the void where the Successor fell ----

// Where UNIT 24 stands in it (its feet), and the camera's shot for the moment
inline glm::vec3 GameplayState::prologueUnit() const {
    return plain::prologueCentre() + glm::vec3{0.f, -0.55f * plainReveal.kneel(), plainReveal.unitZ()};
}
inline void GameplayState::prologueShot(glm::vec3& eye, glm::vec3& at) const {
    const PlainReveal& r = plainReveal;
    const float t = r.time;
    const glm::vec3 C = plain::prologueCentre(), unit = prologueUnit();
    if (t < 4.f) {   // round the two of them, slowly, from above
        float a = glm::mix(2.5f, 1.35f, PlainReveal::sm(0.f, 4.f, t));
        eye = C + glm::vec3{std::cos(a) * 5.2f, 2.1f - 0.5f * PlainReveal::sm(0.f, 4.f, t), 0.9f + std::sin(a) * 5.2f};
        at = C + glm::vec3{0.f, 0.6f, 0.8f};
    } else if (t < PlainReveal::STAND1 + 0.2f) {   // close: the pieces coming to it, then up it gets
        float u = PlainReveal::sm(4.f, PlainReveal::STAND1, t);
        eye = C + glm::mix(glm::vec3{2.7f, 1.35f, 2.9f}, glm::vec3{3.6f, 1.9f, 4.4f}, u);
        at = unit + glm::vec3{0.f, glm::mix(1.f, 1.35f, u), 0.f};
    } else if (t < PlainReveal::GO) {   // following it from behind and to one side: it walks to the portal, stops, looks back at the body
        float near = PlainReveal::sm(PlainReveal::STOP - 0.4f, PlainReveal::STOP + 0.4f, t);
        eye = unit + glm::mix(glm::vec3{-2.2f, 1.7f, -4.f}, glm::vec3{-1.5f, 1.55f, -2.6f}, near);
        at = unit + glm::vec3{0.f, 1.45f, 1.f - near};
    } else {   // behind it, through the portal with it
        eye = unit + glm::vec3{0.7f, 1.9f, -4.4f};
        at = C + glm::vec3{0.f, 3.2f, 18.f};
    }
}

inline void GameplayState::updatePlainPrologue(float t0, float t) {
    auto crossed = [&](float at) { return t0 < at && t >= at; };
    glm::vec3 eye, at;
    prologueShot(eye, at);
    glm::vec3 d = glm::normalize(at - eye);
    player.camera.position = eye; prevCamPos = eye;
    player.camera.yaw = glm::degrees(std::atan2(d.z, d.x));
    player.camera.pitch = glm::degrees(std::asin(glm::clamp(d.y, -1.f, 1.f)));
    const glm::vec3 unit = prologueUnit();
    SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true;
    for (int k = 0; k < 3; ++k)
        if (crossed(PlainReveal::PART0 + k * PlainReveal::PART_GAP + PlainReveal::PART_FLY)) {   // a piece seats: sparks, a clank, the spine lights further
            glm::vec3 p = prologuePartEnd(k);
            fx.spawnBurst(p, {0.4f, 1.f, 1.2f}, 26, 4.f, 0.5f, 9.f);
            o.pitch = 1.25f - 0.1f * k; o.volume = 1.f; audio.playOpts("clank", o);
            o.pitch = 0.8f + 0.15f * k; o.volume = 0.7f; audio.playOpts("hook_yank", o);
        }
    if (crossed(PlainReveal::COMPLETE)) { o.pitch = 0.85f; o.volume = 1.f; audio.playOpts("upgrade", o); }
    if (crossed(PlainReveal::STAND0 + 0.6f)) { o.pitch = 0.9f; o.volume = 0.8f; audio.playOpts("land_metal", o); }
    // its feet on the tiles as it walks
    if (plainReveal.walking() > 0.5f) {
        float ph0 = t0 * 3.4f, ph1 = t * 3.4f;
        if (std::floor(ph0) != std::floor(ph1)) { o.pitch = 0.95f + 0.1f * std::fmod(ph1, 2.f); o.volume = 0.55f; audio.playOpts("step_metal", o); }
    }
    if (crossed(PlainReveal::THROUGH - 0.3f)) { o.pitch = 0.7f; o.volume = 1.f; audio.playOpts("boost", o); }
    (void)unit;
}

// Where each piece leaves the Successor's body and seats on UNIT 24 (the shoulder, the forearm, the spine)
inline glm::vec3 GameplayState::prologuePartStart(int k) const {
    static const glm::vec3 FROM[3] = {{0.35f, 0.35f, 0.25f}, {-0.6f, 0.3f, 0.1f}, {0.95f, 0.3f, -0.1f}};
    return plain::prologueCentre() + FROM[k];
}
inline glm::vec3 GameplayState::prologuePartEnd(int k) const {
    const float yaw = plainReveal.facing(), c = std::cos(yaw), s = std::sin(yaw);
    static const glm::vec3 ON[3] = {{0.2f, 1.44f, 0.f}, {-0.3f, 1.06f, 0.06f}, {0.f, 1.2f, -0.14f}};
    glm::vec3 l = ON[k];
    l.y *= 1.f - 0.25f * plainReveal.kneel();
    return prologueUnit() + glm::vec3{l.x * c + l.z * s, l.y, -l.x * s + l.z * c};
}

inline void GameplayState::gatherPlainPrologue(std::vector<BoxInstance>& out) {
    using namespace rig; using namespace enemyfile; using glm::vec3;
    const PlainReveal& r = plainReveal;
    const vec3 C = plain::prologueCentre(), cyan{0.2f, 0.85f, 1.f};
    const float t = r.time;
    // The Successor: on its back, still, a last red flicker in its channels
    if (const auto* M = story::bodyModel("successor")) {
        FilePose P;
        P.motion[E_ARML] = RZ(0.6f) * RX(-0.3f); P.motion[E_ARMR] = RZ(-0.9f) * RX(0.2f);
        P.motion[E_HEAD] = RY(0.5f) * RX(0.2f); P.motion[E_HIPL] = RZ(0.15f); P.motion[E_HIPR] = RZ(-0.1f) * RX(-0.3f);
        P.motion[E_KNEE_R] = RX(0.5f);
        float ember = 0.15f * std::max(0.f, std::sin(t * 7.f)) * (1.f - r.spine());
        P.chan[C_CORE] = vec3{1.f, 0.1f, 0.05f} * ember; P.chan[C_EYE] = vec3{0.f};
        Rig rg{out};
        drawFile(rg, T(C + vec3{0.f, 0.18f, 0.f}) * RY(1.5708f) * RX(-1.5708f), *M, P, vec3{0.4f, 0.04f, 0.02f} * ember);
    }
    // UNIT 24: kneeling over it, then up, round, walking, a look back, gone
    if (t < PlainReveal::PRO - 0.25f)
        if (const auto* U = story::bodyModel("unit24")) {
            FilePose P;
            float k = r.kneel(), w = r.walking() * (1.f - k), ph = t * 6.8f, sw = std::sin(ph);
            P.motion[E_HIPL] = RX(-1.35f * k - 0.42f * sw * w); P.motion[E_KNEE_L] = RX(1.45f * k + 0.55f * std::max(0.f, -sw) * w);
            P.motion[E_HIPR] = RX(0.15f * k + 0.42f * sw * w); P.motion[E_KNEE_R] = RX(1.75f * k + 0.55f * std::max(0.f, sw) * w);
            P.motion[E_ANKLE_R] = RX(-0.65f * k);
            float lb = r.lookBack();
            P.motion[E_TORSO] = RX(0.38f * k) * RY(0.55f * lb);
            P.motion[E_HEAD] = RX(0.5f * k - 0.1f * lb) * RY(1.15f * lb);
            P.motion[E_ARMR] = RX(-0.85f * k + 0.35f * sw * w) * RZ(-0.1f * k);  // reaching down to the body
            P.motion[E_ARML] = RX(-0.2f * k - 0.35f * sw * w) * RZ(0.12f);
            P.motion[E_ELBOW_R] = RX(-0.4f * k); P.motion[E_FINGERS_R] = RX(0.5f * k);
            float lit = r.spine();
            P.chan[C_EYE] = cyan * (0.45f + 0.55f * lit);
            P.chan[C_CORE] = cyan * (0.12f + 0.88f * lit);
            Rig rg{out};
            drawFile(rg, T(prologueUnit()) * RY(r.facing()), *U, P, cyan * (0.2f + 0.8f * lit));
            // its new parts: dark plates edged in its own light, once they have seated
            for (int p = 0; p < 3; ++p) if (r.partIn(p) >= 1.f)
                push(out, T(prologuePartEnd(p)) * RY(r.facing()) * S(vec3{0.13f, 0.07f, 0.18f}), vec3{0.08f, 0.1f, 0.12f}, cyan * 0.55f);
        }
    // The pieces in flight: red as they tear free, its cyan as they seat
    for (int p = 0; p < 3; ++p) {
        float u = r.partIn(p);
        if (u <= 0.f || u >= 1.f) continue;
        vec3 a = prologuePartStart(p), b = prologuePartEnd(p);
        vec3 at = glm::mix(a, b, u) + vec3{0.f, 0.7f * std::sin(3.14159f * u), 0.f};
        push(out, T(at) * RY(u * 9.f) * RX(u * 6.f) * S(vec3{0.2f, 0.1f, 0.26f}), vec3{0.08f, 0.1f, 0.12f},
             glm::mix(vec3{1.4f, 0.15f, 0.05f}, cyan * 1.6f, u));
        for (int k = 1; k <= 4; ++k) {   // a short trail
            float v = std::max(0.f, u - 0.05f * k);
            vec3 tp = glm::mix(a, b, v) + vec3{0.f, 0.7f * std::sin(3.14159f * v), 0.f};
            push(out, T(tp) * S(vec3{0.06f}), vec3{0.f}, glm::mix(vec3{1.f, 0.1f, 0.05f}, cyan, v) * (1.f - 0.2f * k));
        }
    }
    // The portal: a turning ring of light, arms of colour wound into its middle
    const vec3 P0 = C + vec3{0.f, 4.6f, 17.5f};
    const float ball = (float)meshlib::lumpBall();
    for (int k = 0; k < 32; ++k) {
        float a = k * 0.19635f + t * 0.6f;
        push(out, T(P0 + vec3{std::cos(a) * 4.45f, std::sin(a) * 4.45f, 0.f}) * RZ(a) * S(vec3{0.22f, 0.9f, 0.22f}), vec3{0.f},
             glm::mix(vec3{1.4f, 0.12f, 0.35f}, vec3{0.6f, 0.2f, 1.4f}, 0.5f + 0.5f * std::sin(a * 3.f)) * 1.6f);
    }
    for (int arm = 0; arm < 5; ++arm)
        for (int k = 0; k < 14; ++k) {
            float u = k / 14.f, a = arm * 1.2566f - t * 1.4f + u * 4.f, rad = 4.2f * (1.f - u);
            push(out, T(P0 + vec3{std::cos(a) * rad, std::sin(a) * rad, 0.05f}) * RZ(a) * S(vec3{0.5f * (1.f - u) + 0.1f, 0.18f, 0.05f}), vec3{0.f},
                 glm::mix(vec3{0.9f, 0.1f, 0.4f}, vec3{1.6f, 0.9f, 1.4f}, u) * (1.f + 2.f * r.portalWhite()));
        }
    push(out, T(P0 + vec3{0.f, 0.f, 0.12f}) * S(vec3{8.6f, 8.6f, 0.08f}), vec3{0.f}, vec3{0.25f, 0.04f, 0.3f} * (1.f + 4.f * r.portalWhite()));
    out.back().mesh = ball;
}

// A sound from the Gleaner, out across the plain: placed toward it (inside the mixer's reach), as loud
// as `loud` at the far edge of the plain and louder close, `late` the share of the real sound delay
inline void GameplayState::gleanerSound(const char* name, glm::vec3 from, float loud, float late) {
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
        } else {   // the scream: the loudest thing in the game, now (no delay: it has to land with its jaw)
            gleanerSound("gleaner_scream", e.at, 1.f, 0.f);
            audio.duck(10.f, Gleaner::SCREAM_LEN);
            float near = glm::clamp(1.f - dist / 1800.f, 0.25f, 1.f);
            shake(Gleaner::SCREAM_LEN, 0.05f * near);
            fovKick = std::max(fovKick, 7.f * near);   // the air itself hits you
            audio.music.stinger();
        }
    }
    for (auto& d : gleanerDust) d.age += dt;
    gleanerDust.erase(std::remove_if(gleanerDust.begin(), gleanerDust.end(), [](const GleanerDust& d) { return d.age > 4.f; }), gleanerDust.end());

    // You cannot walk through it: out of its body (its own frame) and away from its fists
    glm::vec3 rel = player.position - g.pos;
    float lx = glm::dot(rel, g.right()), lz = glm::dot(rel, g.fwd());
    const float K = Gleaner::SCALE;
    const bool above = g.kneel > 0.5f && deckWallsFrom >= 0 && player.position.y > deckY - 10.f;   // (on its back, or carried up to it)
    if (!above && player.position.y < 160.f * K && std::abs(lx) < 58.f * K && lz > -205.f * K && lz < 112.f * K) {
        float push = (lx < 0.f ? -58.f : 58.f) * K - lx;
        player.position += g.right() * push;
    }
    for (int a = 0; a < 2; ++a) {
        glm::vec3 h = g.hand(a); h.y = player.position.y;
        glm::vec3 off = player.position - h;
        float len = glm::length(off);
        if (len < 26.f * K && g.hand(a).y < 40.f * K) player.position = h + (len > 0.1f ? off / len : glm::vec3{1.f, 0.f, 0.f}) * (26.f * K);
    }
}

// The sky's moon, shut: 0 open (before it wakes), 1 gone (it is the eye now)
inline float GameplayState::plainMoonLid() const {
    if (plainReveal.active()) return plainReveal.lid();
    return wakeSeen ? 1.f : 0.f;
}

inline void GameplayState::gatherGleaner(std::vector<BoxInstance>& out) {
    using namespace rig;
    if (plainReveal.prologue()) { gatherPlainPrologue(out); return; }
    gleaner::Look look;
    look.ball = (float)meshlib::lumpBall();
    look.flash = gleaner.screaming() ? 0.35f * std::max(0.f, std::sin(gleaner.screamT * 9.f)) * gleaner.jaw : 0.f;
    look.watch = player.camera.position;
    look.watching = gleaner.eyeOpen > 0.5f;
    look.vent = gleanerPhase >= GleanerPhase::CLIMB ? 0.7f + 0.3f * std::sin(gameClock * 2.5f) : 0.f;
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
    if (r.prologue()) {   // its nameplate: REJECTED, then the flicker, then COMPLETE
        float a = PlainReveal::sm(0.8f, 1.8f, r.time) * (1.f - PlainReveal::sm(3.6f, 4.3f, r.time))
                + PlainReveal::sm(6.4f, 6.8f, r.time) * (1.f - PlainReveal::sm(9.4f, 10.f, r.time));
        if (a > 0.01f) {
            bool done = r.complete();
            u.text("UNIT 24  /  PURPOSE: --------  /  STATUS:", SCREEN_W / 2 - 70, SCREEN_H - 52, 2, {0.55f, 0.9f, 1.f, a}, true);
            u.text(done ? "COMPLETE" : "REJECTED", SCREEN_W / 2 + 300, SCREEN_H - 52, 2, done ? glm::vec4{0.4f, 1.f, 1.f, a} : glm::vec4{1.f, 0.3f, 0.25f, a}, true);
        }
    }
    if (r.a() > 1.f && r.a() < 7.5f) {
        float a = PlainReveal::sm(1.f, 2.f, r.a()) * (1.f - PlainReveal::sm(6.5f, 7.5f, r.a()));
        u.text("THE BLOOD MOON PLAIN", 48, 24, 2, {1.f, 0.4f, 0.25f, a});
        u.text("CHAPTER 4  /  RECLAMATION", 48, 46, 1, {0.9f, 0.6f, 0.5f, a});
    }
    const char* cap = r.caption();
    if (*cap) {
        float a = PlainReveal::sm(17.f, 17.8f, r.a()) * (1.f - PlainReveal::sm(20.8f, 21.5f, r.a()));
        u.text(cap, SCREEN_W / 2, SCREEN_H - 50, 2, {1.f, 0.45f, 0.3f, a}, true);
    }
    std::string skip = "HOLD " + binds().label(Act::JUMP) + " TO SKIP";
    u.text(skip.c_str(), SCREEN_W / 2, SCREEN_H - 24, 1, {0.7f, 0.45f, 0.4f, 0.8f * r.bars()}, true);
    if (r.skip > 0.f) u.rect(SCREEN_W / 2 - 90, SCREEN_H - 12, (int)(180.f * r.skip / PlainReveal::SKIP_HOLD), 3, {1.f, 0.4f, 0.25f, 1});
    float w = r.white();
    if (w > 0.001f) u.rect(0, 0, SCREEN_W, SCREEN_H, {1.f, 0.96f, 0.92f, w});
    ui.end2D();
}
