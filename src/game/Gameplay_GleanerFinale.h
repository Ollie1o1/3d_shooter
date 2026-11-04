#pragma once
// =============================================================================
// Gameplay_GleanerFinale.h — GameplayState: how Chapter 4 ends.
//
//   THE TEARING  (first person, about five seconds) The Eye lies on the roof,
//                dim. Walk up and take it (the interact key):
//                  you punch its pupil (it cracks), punch it again (light out
//                  of the cracks), reach into it, grip its core, and pull: the
//                  threads that hold it snap one by one while the Gleaner
//                  screams, and it comes out in your hand, a white-hot heart
//                  the size of a fist. The great eye goes dark behind it. It
//                  beats three times. You crush it. White.
//   THE FALL     (Gameplay_GleanerSummit.h) the far shot: it goes down into the
//                plain, the meteors stop, the real moon opens.
//   THE QUIET    back on the plain, on your feet, in the first quiet of the
//                chapter. Then a white seam opens in the air in front of the
//                fallen thing: a portal, a cold grid of light inside it. UNIT
//                24's status: PURPOSE NOT YET ASSIGNED. Walk to it; it takes
//                you; the chapter's card.
// Included at the end of GameplayState.h (after Gameplay_GleanerSummit.h).
// =============================================================================

// (its timings: namespace finale, in Gameplay_Gleaner.h)

// ---- THE TEARING ------------------------------------------------------------------------------------

// The camera through it: up close on its pupil, drawn back as you pull, then down to what is in your hand
inline void GameplayState::tearShot(glm::vec3& eyeAt, glm::vec3& at) const {
    using namespace finale;
    const float t = tearT;
    const glm::vec3 P = tearPupil, out = tearOut;   // its pupil, and the way out of it toward you (level)
    const glm::vec3 close = P + out * 2.7f + glm::vec3{0.f, 0.35f, 0.f};
    eyeAt = glm::mix(tearFrom, close, sm(0.f, LOCK, t));
    eyeAt += out * (2.4f * sm(PULL0, PULL1, t));                               // hauling it out: you lean back
    float jerk = 0.f;
    for (int k = 0; k < THREADS; ++k) { float d = t - snapAt(k); if (d > 0.f && d < 0.12f) jerk += (0.12f - d) * 1.6f; }
    eyeAt += out * jerk;                                                       // (each thread that snaps: a jolt back)
    at = glm::mix(P, P + glm::vec3{0.f, 0.4f, 0.f}, sm(REACH, GRIP, t));
    // with it out: down to it in your hand, the dark eye behind it
    const glm::vec3 hand = tearCore();
    at = glm::mix(at, hand, sm(PULL1 - 0.3f, BEAT0 + 0.2f, t));
}

// Where its core is: in the pupil, then coming out with your hand, then held up in front of you
inline glm::vec3 GameplayState::tearCore() const {
    using namespace finale;
    const float t = tearT;
    const glm::vec3 P = tearPupil, out = tearOut;
    const float u = sm(PULL0, PULL1, t);
    glm::vec3 c = P - out * 0.6f + out * (u * 2.9f);   // (from inside the pupil, out to your hand)
    c.y += 0.25f * u;
    return c;
}

// Interact beside it where it lies: take it
inline bool GameplayState::tryTearEye() {
    if (!plain() || gleanerPhase != GleanerPhase::SUMMIT || summitEndT >= 0.f || tearT >= 0.f || playerDead || !eye.canTear(player.position)) return false;
    eye.tear();
    eye.events.clear();
    // it looks at you: its pupil on the side you came from, tipped down to you
    glm::vec3 to = player.camera.position - eye.pos;
    eye.facing = glm::normalize(to);
    tearOut = glm::normalize(glm::vec3{to.x, 0.f, to.z} + glm::vec3{0.f, 0.f, 0.0001f});
    tearPupil = eye.pos + eye.facing * (EyeCore::R * 1.02f);
    tearFrom = player.camera.position;
    tearT = 0.f; tearSnapped = 0; tearBeats = 0;
    player.velocity = glm::vec3{0.f};
    storyBossDown = true;
    SfxMixer::Opts o; o.group = SoundGroup::UI; o.priority = true; o.unducked = true;
    o.pitch = 0.5f; audio.playOpts("ab_pulse", o);
    audio.duck(8.f, 0.8f);
    return true;
}

inline void GameplayState::updateTear(float dt) {
    using namespace finale;
    const float t0 = tearT;
    tearT += dt;
    const float t = tearT;
    auto crossed = [&](float at) { return t0 < at && t >= at; };
    player.velocity = glm::vec3{0.f};
    director.hold = true;
    eye.flash = std::max(0.f, eye.flash - dt * 3.f);   // (its tick is not running: the punches' flash fades here)
    SfxMixer::Opts o; o.group = SoundGroup::UI; o.priority = true; o.unducked = true;
    auto punch = [&](float pitch) {
        o.pitch = pitch; audio.playOpts("finisher", o);
        o.pitch = pitch * 0.6f; audio.playOpts("anvil", o);
        hitStopFrames = std::max(hitStopFrames, 7);
        shake(0.5f, 0.06f);
        fovKick = std::max(fovKick, 6.f);
        fx.spawnBurst(tearPupil, {3.f, 0.6f, 0.25f}, 60, 9.f, 0.7f, 4.f);
        eye.flash = 1.f;
    };
    if (crossed(PUNCH1)) { punch(0.75f); }
    if (crossed(PUNCH2)) { punch(0.62f); gleanerSound("gleaner_groan", gleaner.toWorld(gleaner::furnaceBase()), 1.f, 0.f); }
    if (crossed(REACH)) { o.pitch = 0.4f; audio.playOpts("hook_yank", o); shake(0.3f, 0.02f); }
    if (crossed(GRIP)) {   // you have it: its last scream, everything
        gleaner.scream();
        o.pitch = 0.6f; audio.playOpts("stun", o);
        shake(4.f, 0.03f);
    }
    for (int k = 0; k < THREADS; ++k) if (crossed(snapAt(k))) {   // a thread of it snapping
        ++tearSnapped;
        o.pitch = 1.1f + 0.12f * k; audio.playOpts(k % 2 ? "caller_clang" : "sieve_stake", o);
        hitStopFrames = std::max(hitStopFrames, 2);
        shake(0.25f, 0.035f);
        glm::vec3 c = tearCore();
        float a = k * 2.4f;
        glm::vec3 d = glm::normalize(-tearOut + glm::vec3{std::cos(a) * 0.8f, std::sin(a) * 0.8f, 0.f});
        fx.spawnSpray(c, d, {3.f, 1.2f, 0.4f}, 18, 14.f, 0.4f, 0.4f);
    }
    if (crossed(PULL1)) {   // out: the great eye goes dark behind it; the world goes quiet
        o.pitch = 0.35f; audio.playOpts("explosion", o);
        audio.duck(18.f, CRUSH - PULL1 + 0.6f);
        hitStopFrames = std::max(hitStopFrames, 6);
        fx.spawnBurst(tearCore(), {3.f, 1.f, 0.4f}, 50, 6.f, 0.6f, 2.f);
    }
    for (int k = 0; k < 3; ++k) if (crossed(BEAT0 + k * 0.36f)) {   // it beats in your hand
        ++tearBeats;
        o.pitch = 0.75f - 0.06f * k; o.volume = 1.f; audio.playOpts("heartbeat", o);
        camFeel.punch(6.f, 0.f, 0.f);
    }
    if (crossed(CRUSH)) {   // crushed: white
        o.pitch = 0.5f; audio.playOpts("explosion", o);
        o.pitch = 0.7f; audio.playOpts("finisher", o);
        hitStopFrames = std::max(hitStopFrames, 10);
        shake(1.2f, 0.08f);
        fx.spawnBurst(tearCore(), {3.f, 1.f, 0.5f}, 160, 22.f, 1.4f, 2.f);
        styleSystem.addStyle(500.f, StyleSource::PARRY); gainXp(300);
    }
    if (t >= LEN) {   // the fall (the far shot), from the white
        tearT = -1.f;
        summitEndT = 0.f;
    }
}

// UNIT 24's hands in it (camera space: x right, y up, z ahead)
inline int GameplayState::tearArms(ViewModel::FreeArm* A) const {
    using namespace finale; using glm::vec3;
    const float t = tearT;
    int n = 0;
    auto arm = [&](float s, vec3 at, vec3 fingers, vec3 back, armfile::HandPose hand) {
        A[n].shoulder = {s * 0.34f, -0.62f, -0.02f};
        A[n].fist = armfile::handFrame(at, glm::normalize(fingers), glm::normalize(back), s);
        A[n].side = s; A[n].hand = hand;
        ++n;
    };
    auto jab = [&](float at, float len) { float u = (t - at) / len; return u < 0.f || u > 1.f ? 0.f : u < 0.3f ? sm(0.f, 0.3f, u) : 1.f - sm(0.3f, 1.f, u); };
    // the right hand: the first punch, then in after its core, holding it, crushing it
    {
        vec3 rest{0.3f, -0.37f, 0.33f};
        float j = jab(PUNCH1 - 0.12f, 0.6f);
        vec3 at = glm::mix(rest, vec3{0.06f, -0.06f, 0.95f}, j);
        armfile::HandPose pose = armfile::handFist();
        if (t >= REACH - 0.2f) {   // reaching in, open
            float r = sm(REACH - 0.2f, GRIP, t);
            at = glm::mix(rest, vec3{0.05f, -0.03f, 1.05f}, r);
            pose = r > 0.85f ? armfile::handGrip() : armfile::handSpread();
        }
        if (t >= PULL0) {   // hauling it out, then held up in front of you
            float u = sm(PULL0, PULL1, t);
            at = glm::mix(vec3{0.05f, -0.03f, 1.05f}, vec3{0.04f, -0.1f, 0.5f}, u);
            at += vec3{0.f, 0.015f * std::sin(t * 40.f), 0.f} * (1.f - u);   // (straining)
            pose = armfile::handGrip();
        }
        if (t >= CRUSH - 0.12f) { at += vec3{0.f, -0.02f, -0.03f}; pose = armfile::handFist(); }
        arm(1.f, at, vec3{-0.15f, 0.5f, 1.f}, vec3{0.9f, 0.3f, -0.1f}, pose);
    }
    // the left: the second punch, then bracing against it as you pull
    {
        vec3 rest{-0.3f, -0.37f, 0.33f};
        float j = jab(PUNCH2 - 0.12f, 0.6f);
        vec3 at = glm::mix(rest, vec3{-0.07f, -0.04f, 0.95f}, j);
        armfile::HandPose pose = armfile::handFist();
        if (t >= GRIP - 0.2f && t < PULL1 + 0.2f) {   // the flat of it against its shell, pushing off
            float b = sm(GRIP - 0.2f, GRIP + 0.1f, t) * (1.f - sm(PULL1 - 0.1f, PULL1 + 0.2f, t));
            at = glm::mix(rest, vec3{-0.22f, -0.12f, 0.85f}, b);
            pose = armfile::handFlat();
        }
        arm(-1.f, at, vec3{0.15f, 0.5f, 1.f}, vec3{-0.9f, 0.3f, -0.1f}, pose);
    }
    return n;
}

// Its cracks, the light out of them, its core and the threads that hold it, through the tearing
inline void GameplayState::gatherTear(std::vector<BoxInstance>& out) {
    using namespace rig; using namespace finale; using glm::vec3;
    if (tearT < 0.f) return;
    const float t = tearT, ball = (float)meshlib::lumpBall();
    const vec3 f = eye.facing, P = tearPupil;
    vec3 r = glm::cross(f, vec3{0.f, 1.f, 0.f}); r = glm::length(r) > 0.01f ? glm::normalize(r) : vec3{1.f, 0.f, 0.f};
    const vec3 u = glm::cross(r, f);
    const float crack = sm(PUNCH1, PUNCH1 + 0.2f, t) * 0.5f + sm(PUNCH2, PUNCH2 + 0.2f, t) * 0.5f;
    const float dark = sm(PULL1 - 0.2f, PULL1 + 0.4f, t);
    const vec3 hot{3.2f, 1.1f, 0.4f}, white{3.4f, 3.f, 2.6f};
    auto beam = [&](vec3 a, vec3 b, float w, vec3 col) {
        vec3 d = b - a; float len = glm::length(d); if (len < 0.01f) return;
        push(out, gleaner::alongZ((a + b) * 0.5f, d) * S(vec3{w, w, len}), vec3{0.f}, col);
    };
    // cracks across its iris from the pupil: more with each punch, bleeding white light
    const int CR = 12;
    for (int k = 0; k < CR; ++k) {
        if (k >= (int)(CR * crack + 0.5f)) break;
        float a = k * 0.5236f + 0.3f * std::sin(k * 2.1f);
        vec3 dir = r * std::cos(a) + u * std::sin(a);
        float len = EyeCore::R * (0.5f + 0.4f * std::fmod(k * 0.618f, 1.f));
        vec3 a0 = P + dir * 0.4f, a1 = P + dir * (len * 0.55f) + u * 0.3f * std::sin(k * 1.3f), a2 = P + dir * len - f * (len * 0.25f);
        if (dark > 0.6f) continue;   // (its core gone: its light goes out of the cracks)
        beam(a0, a1, 0.12f, hot * 2.f * (1.f - dark));
        beam(a1, a2, 0.08f, hot * 1.4f * (1.f - dark));
        if (t > PUNCH2) beam(a1, a1 + glm::normalize(dir + f * 1.5f) * (14.f + 6.f * std::sin((float)k)), 0.18f, hot * 0.5f * (1.f - dark));   // light out of the cracks
    }
    // its core: white-hot, coming out of the pupil into your hand; it beats there
    const vec3 c = tearCore();
    float beat = 0.f;
    for (int k = 0; k < 3; ++k) { float d = t - (BEAT0 + k * 0.36f); if (d > 0.f && d < 0.3f) beat = std::max(beat, 1.f - d / 0.3f); }
    if (t < CRUSH) {
        const float sz = 0.42f + 0.1f * beat + 0.03f * std::sin(t * 30.f);
        push(out, T(c) * RY(t * 3.f) * S(vec3{sz * 2.f}), vec3{0.8f, 0.2f, 0.1f}, white * (0.7f + 0.8f * beat) + hot); out.back().mesh = ball;
        push(out, T(c) * S(vec3{sz * 3.2f}), vec3{0.f}, hot * (0.25f + 0.4f * beat)); out.back().mesh = ball;   // its glow
    }
    // the threads holding it, stretching out of the pupil as you pull, snapping
    if (t > GRIP - 0.1f && t < PULL1 + 0.6f) {
        for (int k = 0; k < THREADS; ++k) {
            float a = k * 6.2831853f / THREADS + 0.2f;
            vec3 anchor = P - f * 0.3f + (r * std::cos(a) + u * std::sin(a)) * (1.2f + 0.4f * std::sin(k * 1.9f));
            float since = t - snapAt(k);
            if (since < 0.f) {
                float tense = sm(PULL0, snapAt(k), t);
                beam(anchor, c, 0.05f + 0.05f * tense, glm::mix(hot, white, tense) * (1.2f + 1.5f * tense * (0.5f + 0.5f * std::sin(t * 60.f + k))));
            } else if (since < 0.45f) {   // snapped: its two ends whipping back
                float w = 1.f - since / 0.45f;
                vec3 mid = glm::mix(anchor, c, 0.5f);
                vec3 kick = glm::normalize(mid - (anchor + c) * 0.5f + (r * std::cos(a) + u * std::sin(a)) * 0.5f) * (since * 6.f);
                beam(anchor, glm::mix(anchor, mid, 0.6f) + kick, 0.05f, white * w * 2.f);
                beam(c, glm::mix(c, mid, 0.5f) + kick, 0.05f, white * w * 2.f);
            }
        }
    }
    // its pupil, torn open: a ragged mouth of light (dark once its core is gone)
    push(out, gleaner::alongZ(P + f * 0.05f, f) * S(vec3{1.5f + 0.8f * crack, 2.6f + 1.f * crack, 0.2f}), vec3{0.f}, glm::mix(hot * 2.f, vec3{0.15f, 0.02f, 0.f}, dark));
    // the crush: a sphere of white light
    if (t > CRUSH) {
        float k = sm(CRUSH, LEN, t);
        push(out, T(c) * S(vec3{1.f + 30.f * k}), vec3{0.f}, white * (2.5f * (1.f - k * 0.5f))); out.back().mesh = ball;
    }
}

// Over the tearing: bars, the red at the edges, the white of the crush
inline void GameplayState::renderTear() {
    using namespace finale;
    ui.begin2D(); auto& U = ui.ui;
    const float t = tearT;
    const int bar = (int)(60.f * sm(0.f, 0.5f, t));
    if (bar > 0) { U.rect(0, 0, SCREEN_W, bar, {0.02f, 0.f, 0.f, 1}); U.rect(0, SCREEN_H - bar, SCREEN_W, bar, {0.02f, 0.f, 0.f, 1}); }
    float flash = 0.f;
    for (float a : {PUNCH1, PUNCH2}) { float d = t - a; if (d > 0.f && d < 0.25f) flash = std::max(flash, 0.55f * (1.f - d / 0.25f)); }
    if (flash > 0.f) U.rect(0, 0, SCREEN_W, SCREEN_H, {1.f, 0.85f, 0.75f, flash});
    float red = 0.25f * sm(GRIP, PULL1, t) * (1.f - sm(PULL1, PULL1 + 0.3f, t));
    if (red > 0.f) for (int k = 0; k < 6; ++k) {   // the edges of the view going red as you pull
        float w = 40.f + k * 22.f, a = red * (1.f - k / 6.f);
        U.rect(0, 0, (int)w, SCREEN_H, {0.6f, 0.f, 0.f, a}); U.rect(SCREEN_W - (int)w, 0, (int)w, SCREEN_H, {0.6f, 0.f, 0.f, a});
    }
    float white = sm(CRUSH, LEN, t);
    if (white > 0.001f) U.rect(0, 0, SCREEN_W, SCREEN_H, {1.f, 0.95f, 0.9f, white});
    ui.end2D();
}

// ---- THE QUIET --------------------------------------------------------------------------------------

inline glm::vec3 GameplayState::quietPortal() const { return quietPortalAt; }
// Where you stand when the quiet begins: off its right, the portal ahead
inline glm::vec3 quietStandOff() { return {16.f, 0.f, 46.f}; }

// A spot in front of the fallen thing with nothing in the way: open ground round the ring, and a clear
// look at it from where you stand
inline glm::vec3 GameplayState::pickQuietPortal() const {
    const float R = finale::RADIUS;
    const float dzs[] = {330.f, 380.f, 290.f, 430.f, 480.f}, dxs[] = {0.f, 70.f, -70.f, 140.f, -140.f};
    for (float dz : dzs) for (float dx : dxs) {
        glm::vec3 c = gleanerplace::battleMark() + glm::vec3{dx, R + 1.f, dz};
        glm::vec3 eyeAt = c + quietStandOff(); eyeAt.y = 1.7f;
        bool ok = true;
        for (const auto& w : level.walls) {
            const AABB& b = w.box;
            if (b.max.y < 0.3f) continue;   // (the ground itself)
            bool near = b.min.x < c.x + R + 8.f && b.max.x > c.x - R - 8.f && b.min.z < c.z + 14.f && b.max.z > c.z - 14.f && b.min.y < c.y + R;
            if (near || segmentHitsBox(eyeAt, c, b) || segmentHitsBox(eyeAt, c - glm::vec3{0.f, R * 0.8f, 0.f}, b)) { ok = false; break; }
        }
        if (ok) return c;
    }
    return gleanerplace::battleMark() + glm::vec3{0.f, R + 1.f, 330.f};
}

// From the fall's dark: on your feet on the plain, it lying sunk in front of you, the sky quiet
inline void GameplayState::beginQuiet() {
    summitEndT = -1.f;
    quietT = 0.f; quietThrough = -1.f; quietCaption = false;
    clearGleanerDeck();
    gleaner.rise = 0.f; gleaner.kneel = 0.f; gleaner.headHold = 0.f; gleaner.eyeOpen = 0.f; gleaner.wantSpeed = 0.f;
    eye.state = EyeCore::TORN;
    for (auto& pr : projSystem.pool) pr.alive = false;
    quietPortalAt = pickQuietPortal();
    const glm::vec3 portal = quietPortal();
    player.position = portal + quietStandOff(); player.position.y = 0.f;
    player.velocity = glm::vec3{0.f};
    glm::vec3 d = glm::normalize(glm::vec3{portal.x - player.position.x, 0.f, portal.z - player.position.z});
    player.camera.yaw = glm::degrees(std::atan2(d.z, d.x)); player.camera.pitch = 4.f;
    player.camera.position = player.position + glm::vec3{0.f, 1.7f, 0.f}; prevCamPos = player.camera.position;
    hasSafeGround = false;
}

inline void GameplayState::updateQuiet(float dt) {
    using namespace finale;
    const float t0 = quietT;
    quietT += dt;
    const float t = quietT;
    auto crossed = [&](float at) { return t0 < at && t >= at; };
    director.hold = true;
    const glm::vec3 portal = quietPortal();
    SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.positional = true; o.pos = portal; o.priority = true; o.floor = 0.6f;
    if (crossed(SEAM)) { o.pitch = 0.45f; audio.playOpts("gleaner_blink", o); o.pitch = 0.6f; audio.playOpts("ab_pulse", o); }
    if (crossed(SEAM + 1.2f)) { o.pitch = 0.5f; audio.playOpts("boost", o); shake(0.6f, 0.012f); }
    if (crossed(OPEN1)) { o.pitch = 0.8f; audio.playOpts("ab_pulse", o); quietCaption = true; }
    if (quietThrough >= 0.f) {   // taken: white, then the card
        quietThrough += dt;
        player.velocity = glm::vec3{0.f};
        if (quietThrough >= THROUGH && storyEndT < 0.f) {
            storyEndT = 0.f; saveStoryProgress(); logRun("end"); audio.play("wave", 100, SoundGroup::UI, true);
        }
        return;
    }
    if (t < OPEN1) return;
    // Close to it: it draws you in, then takes you
    glm::vec3 to = portal - (player.position + glm::vec3{0.f, 1.f, 0.f});
    glm::vec2 flat{to.x, to.z};
    float dist = glm::length(flat);
    if (dist < PULL_R) {
        float k = 1.f - dist / PULL_R;
        player.velocity += glm::vec3{flat.x, 0.f, flat.y} / std::max(dist, 0.1f) * (14.f * k * dt * 60.f / 60.f);
        audio.duck(4.f * k, 0.1f);
    }
    if (dist < 3.5f && !playerDead) {
        quietThrough = 0.f;
        SfxMixer::Opts u; u.group = SoundGroup::UI; u.priority = true; u.unducked = true; u.pitch = 0.5f;
        audio.playOpts("boost", u); u.pitch = 0.35f; audio.playOpts("ab_pulse", u);
        shake(1.f, 0.03f); fovKick = std::max(fovKick, 14.f);
    }
}

// The portal: a seam of white in the air that opens into a ring; inside it, a cold white with a grid of
// fine lines receding (whatever is through there is measured); light falling out of it onto the plain
inline void GameplayState::gatherQuiet(std::vector<BoxInstance>& out) {
    using namespace rig; using namespace finale; using glm::vec3;
    if (quietT < SEAM) return;
    const float t = quietT, ball = (float)meshlib::lumpBall();
    const vec3 P = quietPortal();
    const float open = sm(SEAM + 0.8f, OPEN1, t), seam = sm(SEAM, SEAM + 0.8f, t);
    const float R = RADIUS * (0.04f + 0.96f * open), H = RADIUS * (0.08f + 1.92f * std::max(seam * 0.9f, open)) * 0.5f;
    const vec3 white{2.6f, 2.6f, 2.8f}, cold{1.6f, 1.9f, 2.4f};
    // the seam / the ring: segments round an ellipse that opens from a vertical line to a circle
    const int N = 64;
    for (int k = 0; k < N; ++k) {
        float a0 = k * 6.2831853f / N, a1 = (k + 1) * 6.2831853f / N;
        vec3 p0 = P + vec3{std::cos(a0) * R, std::sin(a0) * H, 0.f}, p1 = P + vec3{std::cos(a1) * R, std::sin(a1) * H, 0.f};
        vec3 d = p1 - p0; float len = glm::length(d);
        if (len < 0.001f) continue;
        float flick = 0.8f + 0.2f * std::sin(t * 9.f + k * 0.7f);
        push(out, gleaner::alongZ((p0 + p1) * 0.5f, d) * S(vec3{0.35f, 0.35f, len * 1.05f}), vec3{0.f}, white * 2.f * flick);
    }
    // inside: a disc of cold white, and a grid of fine lines receding into it
    if (open > 0.02f) {
        push(out, T(P - vec3{0.f, 0.f, 0.3f}) * S(vec3{R * 2.f, H * 2.f, 0.1f}), vec3{0.f}, cold * 1.4f); out.back().mesh = ball;
        for (int depth = 1; depth <= 5; ++depth) {
            float z = -depth * 6.f, sc = 1.f - depth * 0.13f;
            for (int g = -4; g <= 4; ++g) {
                float x = g * R * 0.22f * sc;
                if (std::abs(x) > R * 0.9f * sc) continue;
                float hh = std::sqrt(std::max(0.f, 1.f - (x / (R * sc)) * (x / (R * sc)))) * H * sc;
                push(out, T(P + vec3{x, 0.f, z}) * S(vec3{0.06f, hh * 2.f, 0.06f}), vec3{0.f}, vec3{0.3f, 0.4f, 0.6f} * open * (1.f - depth * 0.15f));
                float y = g * H * 0.22f * sc;
                if (std::abs(y) > H * 0.9f * sc) continue;
                float ww = std::sqrt(std::max(0.f, 1.f - (y / (H * sc)) * (y / (H * sc)))) * R * sc;
                push(out, T(P + vec3{0.f, y, z}) * S(vec3{ww * 2.f, 0.06f, 0.06f}), vec3{0.f}, vec3{0.3f, 0.4f, 0.6f} * open * (1.f - depth * 0.15f));
            }
        }
    }
    // light falling out of it onto the plain, and motes drawn in toward it
    push(out, T(vec3{P.x, 0.06f, P.z + 6.f}) * S(vec3{R * 2.2f, 0.05f, 16.f}), vec3{0.f}, cold * 0.5f * open); out.back().mesh = ball;
    for (int k = 0; k < 50; ++k) {
        float u = std::fmod(t * 0.35f + k * 0.0618f * 3.f, 1.f), a = k * 2.399f;
        vec3 from = P + vec3{std::cos(a) * (R + 18.f), std::sin(a) * (H + 10.f), 24.f};
        vec3 q = glm::mix(from, P, u * u);
        push(out, T(q) * S(vec3{0.12f + 0.1f * (1.f - u)}), vec3{0.f}, white * open * (1.f - u));
    }
}

// Over the quiet: up out of the dark, UNIT 24's status, the white as it takes you
inline void GameplayState::renderQuiet() {
    using namespace finale;
    ui.begin2D(); auto& U = ui.ui;
    const float t = quietT;
    float dark = 1.f - sm(0.f, FADE, t);
    if (dark > 0.001f) U.rect(0, 0, SCREEN_W, SCREEN_H, {0.f, 0.f, 0.f, dark});
    float cap = sm(CAPTION0, CAPTION0 + 1.f, t) * (1.f - sm(CAPTION1 - 1.f, CAPTION1, t));
    if (cap > 0.f) {
        U.text("UNIT 24", SCREEN_W / 2, SCREEN_H - 120, 2, {0.85f, 0.92f, 1.f, cap}, true);
        U.text("STATUS: COMPLETE  /  PURPOSE: NOT YET ASSIGNED", SCREEN_W / 2, SCREEN_H - 92, 1, {0.6f, 0.8f, 1.f, cap * 0.9f}, true);
    }
    if (quietThrough >= 0.f) {
        float w = sm(0.f, THROUGH, quietThrough);
        U.rect(0, 0, SCREEN_W, SCREEN_H, {0.95f, 0.97f, 1.f, w});
    }
    ui.end2D();
}
