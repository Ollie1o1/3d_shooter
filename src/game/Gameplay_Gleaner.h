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
    const bool devFight = g_devOverlay == "plainclose" || g_devOverlay == "plaindeck" || g_devOverlay == "plainsaddle" || g_devOverlay == "plaincrown";   // (dev: straight to the fight)
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
        pushBanner("THE GLEANER", "\"YOU WERE SENT TO ME. YOU ARE LATE.\"", {1.f, 0.3f, 0.18f}, 4.5f);
        gleanerScreamCd = 40.f;
    }
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
    const bool above = g.kneel > 0.5f && deckWallsFrom >= 0 && player.position.y > std::min(deckY, saddleY) - 10.f;   // (on its back, or carried up to it)
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
    gleaner::Look look;
    look.ball = (float)meshlib::lumpBall();
    look.flash = gleaner.screaming() ? 0.35f * std::max(0.f, std::sin(gleaner.screamT * 9.f)) * gleaner.jaw : 0.f;
    look.watch = player.camera.position;
    look.watching = gleaner.eyeOpen > 0.5f;
    look.frontGap = deckFrontOpen && deckWallsFrom >= 0;
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
