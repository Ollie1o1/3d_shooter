#pragma once
// =============================================================================
// Gameplay_LabEnding.h — GameplayState: the end of Chapter 3 (LabEnding.h is its
// clock), played in the final assessment itself. Through the doors the
// Successor held shut, UNIT 24 turns back to its body, takes the pieces it was
// missing off it by hand, walks out along the platform to its end, looks back,
// and the portal over the end takes it. Then Chapter 4.
//
// The Successor's body lies where it fell from its death on (gameplay and the
// cutscene both draw it here), its three pieces on it until they are taken.
// Included at the end of GameplayState.h.
// =============================================================================


// Where the body lies (on the floor where it fell; the arena's middle if it was never seen falling)
inline glm::vec3 GameplayState::successorBody() const {
    if (successorFellSet) return successorFell;
    return level.arenas.size() > 9 ? level.arenas[9].bossSpawn : labend::arena() + glm::vec3{0.f, 0.f, -20.f};
}

inline void GameplayState::startLabEnding() {
    labEnding.start();
    labEndDoor = player.position; labEndDoor.y = labend::floorY();
    labEndDoor.x = glm::clamp(labEndDoor.x, -5.f, 5.f);
    labEndDoor.z = std::min(labEndDoor.z, labend::doorZ() - 1.f);
    player.velocity = glm::vec3{0.f};
    director.hold = true;
    for (auto& p : projSystem.pool) if (!p.isPlayer) p.alive = false;
    SfxMixer::Opts o; o.group = SoundGroup::UI; o.priority = true; o.pitch = 0.6f; o.volume = 0.8f;
    audio.playOpts("door_close", o);
}

// UNIT 24, where the clock has it: its feet, its yaw (forward = (sin yaw, 0, cos yaw))
inline glm::vec3 GameplayState::labEndUnit(float& yaw) const {
    const LabEnding& L = labEnding;
    const float t = L.time;
    const glm::vec3 B = successorBody();
    if (t < LabEnding::DOOR1) { yaw = 3.14159265f; return labEndDoor; }   // in the doorway, walking out
    if (t < LabEnding::WALK0) {   // back at the body (the last steps of the way: the cut takes the rest)
        glm::vec3 away = labend::flat(labEndDoor - B);
        glm::vec3 A = B + away * 7.f, K = B + away * 1.15f;
        A.y = K.y = B.y;
        yaw = std::atan2(-away.x, -away.z);
        return glm::mix(A, K, L.approach()) - glm::vec3{0.f, 0.5f * L.kneel(), 0.f};
    }
    yaw = 3.14159265f;
    glm::vec3 W1 = labend::edge();
    if (t < LabEnding::PULL0) return glm::mix(labend::walkFrom(), W1, L.platformU());
    // Taken: up off the edge and along a curve into the middle of the vortex
    const glm::vec3 P = level.story.labPortal, mid = W1 + glm::vec3{0.f, 10.f, -14.f};
    const float u = L.pull(), v = 1.f - u;
    return v * v * W1 + 2.f * v * u * mid + u * u * P;
}

// The camera for the moment: where it is and what it looks at
inline void GameplayState::labEndShot(glm::vec3& eye, glm::vec3& at) const {
    using namespace labend;
    const LabEnding& L = labEnding;
    const float t = L.time, st = L.shotT();
    float yaw = 0.f;
    const glm::vec3 U = labEndUnit(yaw), B = successorBody();
    const glm::vec3 fwd{std::sin(yaw), 0.f, std::cos(yaw)}, side{std::cos(yaw), 0.f, -std::sin(yaw)}, up{0.f, 1.f, 0.f};
    auto sm = LabEnding::sm;
    switch (L.shot()) {
    case LabEnding::DOORWAY: {   // ahead of it in the doorway, low, drifting back as it stops and looks over its shoulder
        float u = sm(0.f, LabEnding::DOOR1, st);
        eye = U + fwd * glm::mix(4.6f, 3.4f, u) + side * glm::mix(1.5f, 0.9f, u) + up * glm::mix(1.0f, 1.35f, u);
        at = glm::mix(U + up * 1.45f, U + up * 1.5f - fwd * 6.f + side * 0.6f, sm(0.9f, 2.4f, st));   // past it, to the floor it is looking back at
        break;
    }
    case LabEnding::CROSSING: {   // a crane over the floor coming down on the two of them
        float u = sm(0.f, LabEnding::KNEEL0 - LabEnding::ARRIVE0, st);
        glm::vec3 away = flat(labEndDoor - B), s2{away.z, 0.f, -away.x};
        eye = B + away * glm::mix(16.f, 7.f, u) + s2 * glm::mix(11.f, 5.5f, u) + up * glm::mix(10.f, 2.6f, u);
        at = glm::mix(U, B, 0.4f) + up * glm::mix(0.6f, 1.f, u);
        break;
    }
    case LabEnding::KNEEL: {   // slowly round them at its shoulder's height
        float a = glm::mix(-0.9f, -0.3f, sm(0.f, 1.4f, st));
        glm::vec3 o = side * std::cos(a) + fwd * std::sin(a);
        eye = U + o * 3.2f + up * 1.55f;
        at = glm::mix(U, B, 0.55f) + up * 0.5f;
        break;
    }
    case LabEnding::PART: {   // close on its hands, a new angle for each piece, pushing in
        int k = std::clamp((int)((t - LabEnding::PART0) / LabEnding::PART_LEN), 0, 2);
        float u = sm(0.f, LabEnding::PART_LEN, t - L.partStart(k));
        static const glm::vec3 OFF[3] = {{1.9f, 1.25f, 1.1f}, {-1.7f, 0.9f, 1.6f}, {0.9f, 1.7f, -2.2f}};   // (side, up, forward)
        glm::vec3 o = OFF[k];
        float pushIn = glm::mix(1.15f, 0.82f, u);
        eye = U + (side * o.x + fwd * o.z) * pushIn + up * o.y;
        glm::vec3 src = labPartFrom(k), seat = labPartSeat(k);
        at = glm::mix(glm::mix(glm::mix(U, B, 0.6f) + up * 0.5f, src, sm(0.f, 0.9f, t - L.partStart(k))), seat, L.carry(k));
        break;
    }
    case LabEnding::RISE: {   // low in front of it as it stands, tilting up with it
        float u = sm(0.f, LabEnding::STAND1 - LabEnding::STAND0 + 0.6f, st);
        eye = U + fwd * 2.7f + side * 0.6f + up * glm::mix(0.45f, 0.75f, u);
        at = U + up * glm::mix(1.1f, 1.9f, u);
        break;
    }
    case LabEnding::SPLINTER: {   // close on the new forearm as it opens
        glm::vec3 f = labPartSeat(1);
        eye = f + fwd * 0.9f + side * 0.75f + up * 0.25f;
        at = f + up * 0.12f;
        break;
    }
    case LabEnding::PLATFORM: {   // tracking out along the platform with it, the portal over the end
        float u = sm(0.f, LabEnding::LOOK0 - LabEnding::WALK0, st);
        eye = U - fwd * glm::mix(5.5f, 3.6f, u) + side * glm::mix(-2.4f, -1.4f, u) + up * glm::mix(2.2f, 1.9f, u);
        at = glm::mix(U + up * 1.4f + fwd * 2.f, level.story.labPortal, 0.035f);
        break;
    }
    case LabEnding::LOOKBACK: {   // out over the edge, facing it: it turns and looks back the way it came
        float u = sm(0.f, LabEnding::PULL0 - LabEnding::LOOK0, st);
        eye = U + fwd * glm::mix(3.4f, 2.6f, u) + side * 0.9f + up * 1.45f;
        at = U + up * 1.55f - fwd * glm::mix(0.f, 2.f, u);
        break;
    }
    case LabEnding::PORTAL: {   // behind it on the platform: it goes up into the light
        const glm::vec3 W1 = edge();
        float u = sm(0.f, LabEnding::DURATION - LabEnding::PULL0, st);
        eye = W1 + glm::vec3{2.4f, 1.6f + 2.f * u, 6.5f - 1.5f * u};
        at = glm::mix(U + up * 1.f, level.story.labPortal, 0.25f + 0.5f * u);
        break;
    }
    }
    eye += handheld(t, L.shot() == LabEnding::PART || L.shot() == LabEnding::SPLINTER ? 0.6f : 1.f);
}

// Where each piece sits on the Successor's body (lying on its back), and where it seats on UNIT 24.
// Both come from the last frame the two of them were drawn (their joints), with fallbacks before that.
inline glm::vec3 GameplayState::labPartFrom(int k) const {
    if (labEndHaveJoints) return labEndFrom[k];
    return successorBody() + glm::vec3{0.f, 0.35f, 0.f};
}
inline glm::vec3 GameplayState::labPartSeat(int k) const {
    if (labEndHaveJoints) return labEndSeat[k];
    float yaw; glm::vec3 U = labEndUnit(yaw);
    return U + glm::vec3{0.f, 1.3f, 0.f};
}

inline void GameplayState::updateLabEnding(float dt) {
    LabEnding& L = labEnding;
    const float t0 = L.time;
    const bool done = L.update(dt, storySkipKey);
    const float t = done ? LabEnding::DURATION : L.time;
    auto crossed = [&](float at) { return t0 < at && t >= at; };
    // The player stays where they crossed; the camera is the cutscene's
    player.position = labEndDoor; player.velocity = glm::vec3{0.f};
    if (!done) {
        glm::vec3 eye, at;
        labEndShot(eye, at);
        glm::vec3 d = glm::normalize(at - eye);
        player.camera.position = eye; prevCamPos = eye;
        player.camera.yaw = glm::degrees(std::atan2(d.z, d.x));
        player.camera.pitch = glm::degrees(std::asin(glm::clamp(d.y, -1.f, 1.f)));
    }
    float yaw; const glm::vec3 U = labEndUnit(yaw);
    SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.positional = true; o.pos = U + glm::vec3{0.f, 1.f, 0.f};
    // Its feet
    if (L.walking() > 0.5f) {
        float ph0 = t0 * 3.6f, ph1 = t * 3.6f;
        if (std::floor(ph0) != std::floor(ph1)) { o.pitch = 0.92f + 0.12f * std::fmod(ph1, 2.f); o.volume = 0.7f; audio.playOpts("step_metal", o); }
    }
    if (crossed(LabEnding::KNEEL0 + 0.7f)) { o.pitch = 0.8f; o.volume = 0.9f; audio.playOpts("land_metal", o); }
    for (int k = 0; k < 3; ++k) {
        if (crossed(L.tearT(k))) {   // torn free: metal giving, the body's last red sparks
            glm::vec3 p = labPartFrom(k);
            fx.spawnBurst(p, {1.6f, 0.2f, 0.12f}, 34, 5.f, 0.6f, 9.f);
            o.pos = p; o.pitch = 0.75f - 0.05f * k; o.volume = 1.f; audio.playOpts("sword_break", o);
            o.pitch = 0.55f + 0.1f * k; audio.playOpts("hook_yank", o);
            shake(0.25f, 0.015f);
        }
        if (crossed(L.seatT(k))) {   // seated: a clank, its own light running into it
            glm::vec3 p = labPartSeat(k);
            fx.spawnBurst(p, {0.4f, 1.f, 1.2f}, 30, 4.f, 0.5f, 9.f);
            o.pos = p; o.pitch = 1.2f - 0.1f * k; o.volume = 1.f; audio.playOpts("clank", o);
            o.pitch = 0.9f + 0.15f * k; o.volume = 0.8f; audio.playOpts("cyl_close", o);
        }
    }
    if (crossed(LabEnding::COMPLETE)) { SfxMixer::Opts u; u.group = SoundGroup::UI; u.priority = true; u.pitch = 0.85f; audio.playOpts("upgrade", u); u.pitch = 0.7f; audio.playOpts("overdrive_on", u); }
    if (crossed(LabEnding::STAND0 + 0.5f)) { o.pos = U; o.pitch = 0.9f; o.volume = 0.8f; audio.playOpts("land_metal", o); }
    if (crossed(LabEnding::SPLINTER0 + 0.3f)) {
        o.pos = labPartSeat(1); o.pitch = 1.6f; o.volume = 0.9f; audio.playOpts("parry", o);
        o.pitch = 1.f; audio.playOpts("drive_ready", o);
        fx.spawnBurst(labPartSeat(1) + glm::vec3{0.f, 0.15f, 0.f}, {0.4f, 1.f, 1.3f}, 12, 2.f, 0.4f, 2.f);
    }
    if (crossed(LabEnding::PULL0)) {   // the portal takes hold: the air goes toward it
        SfxMixer::Opts u; u.group = SoundGroup::UI; u.priority = true; u.pitch = 0.45f; u.volume = 1.f;
        audio.playOpts("boost", u); u.pitch = 0.5f; audio.playOpts("ab_pulse", u);
    }
    if (crossed(LabEnding::THROUGH - 0.3f)) { SfxMixer::Opts u; u.group = SoundGroup::UI; u.priority = true; u.pitch = 0.6f; audio.playOpts("explosion", u); }
    if (L.active()) audio.setWind(glm::clamp(L.pull() * 1.2f + 0.15f * L.sm(LabEnding::WALK0, LabEnding::EDGE, t), 0.f, 1.f));
    if (L.time > LabEnding::PULL0) shake(0.1f, 0.01f + 0.03f * L.pull());
    if (done) {   // the chapter's end: its card, then on to Chapter 4
        labEndingDone = true;
        audio.setWind(0.f);
        director.hold = false;
        storyEndT = 0.f; saveStoryProgress(); logRun("end");
        audio.play("wave", 100, SoundGroup::UI, true);
    }
}

// The body, its pieces, UNIT 24 and the portal's light (gatherLab calls it in the final assessment)
inline void GameplayState::gatherLabEnding(std::vector<BoxInstance>& out) {
    using namespace rig; using namespace enemyfile; using glm::vec3;
    if (!storyBossDown) return;
    const LabEnding& L = labEnding;
    const bool cut = L.active();
    const float t = cut ? L.time : 0.f;
    const vec3 cyan{0.2f, 0.85f, 1.f}, ivory{0.66f, 0.68f, 0.68f}, red{1.6f, 0.08f, 0.2f};
    auto origin = [](const mat4& m) { return vec3(m[3]); };
    // The Successor: on its back where it fell, its channels dark, a last red flicker in what is left of it
    const vec3 B = successorBody();
    mat4 Wc[E_COUNT];
    bool body = false;
    if (const auto* M = story::bodyModel("successor")) {
        FilePose P;
        P.motion[E_ARML] = RZ(0.6f) * RX(-0.3f); P.motion[E_ARMR] = RZ(-0.9f) * RX(0.2f);
        P.motion[E_HEAD] = RY(0.5f) * RX(0.2f); P.motion[E_HIPL] = RZ(0.15f); P.motion[E_HIPR] = RZ(-0.1f) * RX(-0.3f);
        P.motion[E_KNEE_R] = RX(0.5f);
        float jerk = 0.f;   // it gives a little with each piece torn out of it
        for (int k = 0; k < 3; ++k) if (cut) jerk += std::exp(-std::max(0.f, t - L.tearT(k)) * 6.f) * (t > L.tearT(k) ? 1.f : 0.f);
        P.motion[E_TORSO] = RX(0.08f * jerk);
        float ember = 0.2f * std::max(0.f, std::sin(gameClock * 7.f)) * (cut ? 1.f - L.spine() : 1.f);
        P.chan[C_CORE] = vec3{1.f, 0.1f, 0.05f} * ember; P.chan[C_EYE] = vec3{0.f};
        Rig rg{out};
        drawFile(rg, T(B + vec3{0.f, 0.18f, 0.f}) * RY(successorFellYaw + 1.5708f) * RX(-1.5708f), *M, P, vec3{0.4f, 0.04f, 0.02f} * ember, Wc);
        body = true;
        labEndFrom[0] = origin(Wc[E_ARML]) + vec3{0.f, 0.12f, 0.f};
        labEndFrom[1] = glm::mix(origin(Wc[E_ELBOW_R]), origin(Wc[E_HAND_R]), 0.45f) + vec3{0.f, 0.08f, 0.f};
        labEndFrom[2] = origin(Wc[E_TORSO]) + vec3{0.f, 0.1f, 0.f};
    }
    // UNIT 24 (only in the cutscene: otherwise it is you)
    mat4 W[E_COUNT];
    float yaw = 0.f;
    bool unit = false;
    if (cut && L.time < LabEnding::THROUGH + 0.3f)
        if (const auto* U = story::bodyModel("unit24")) {
            FilePose P;
            const vec3 at = labEndUnit(yaw);
            float k = L.kneel(), w = L.walking() * (1.f - k), ph = t * 7.2f, sw = std::sin(ph);
            const float pull = L.pull();
            P.motion[E_HIPL] = RX(-1.35f * k - 0.42f * sw * w + 0.3f * pull); P.motion[E_KNEE_L] = RX(1.45f * k + 0.55f * std::max(0.f, -sw) * w + 0.5f * pull);
            P.motion[E_HIPR] = RX(0.15f * k + 0.42f * sw * w - 0.2f * pull); P.motion[E_KNEE_R] = RX(1.75f * k + 0.55f * std::max(0.f, sw) * w + 0.7f * pull);
            P.motion[E_ANKLE_R] = RX(-0.65f * k);
            float lb = L.lookBack();
            P.motion[E_TORSO] = RX(0.38f * k - 0.15f * pull) * RY(0.5f * lb);
            P.motion[E_HEAD] = RX(0.45f * k - 0.1f * lb - 0.3f * pull) * RY(1.2f * lb);
            // The right hand: down into the body, then home with each piece (to the left shoulder, along the left
            // forearm, over its shoulder to its back); the left arm held out for the forearm housing
            float rx = -0.35f * sw * w, rz = -0.1f, ex = -0.2f, fing = 0.f;
            float lrx = 0.35f * sw * w, lrz = 0.12f, lex = -0.15f;
            for (int p = 0; p < 3; ++p) {
                float r = L.reach(p), c = L.carry(p) * (1.f - L.sm(L.seatT(p), L.seatT(p) + 0.5f, t));
                static const float CX[3] = {-1.55f, -1.15f, -2.75f}, CZ[3] = {0.8f, 0.45f, 0.2f}, CE[3] = {-1.4f, -0.9f, -1.3f};
                rx += -1.05f * r * (1.f - L.carry(p)) + CX[p] * c;
                rz += -0.2f * r * (1.f - L.carry(p)) + CZ[p] * c;
                ex += -0.35f * r + CE[p] * c;
                fing += std::max(r, c) * (L.taken(p) ? 0.9f : 0.3f);
                if (p == 1) { float hold = L.sm(L.partStart(1) + LabEnding::REACH, L.partStart(1) + LabEnding::REACH + LabEnding::TEAR, t) * (1.f - L.sm(L.seatT(1) + 0.2f, L.seatT(1) + 0.8f, t)); lrx += -1.25f * hold; lrz += 0.2f * hold; lex += -0.5f * hold; }
            }
            float spl = L.splinter();   // the forearm up to its face, opening
            lrx += -1.4f * spl; lex += -1.2f * spl; lrz += 0.35f * spl;
            rx += -0.6f * pull; rz += -0.5f * pull; lrx += -0.6f * pull; lrz += 0.5f * pull;   // taken: its arms open
            P.motion[E_ARMR] = RX(rx) * RZ(rz); P.motion[E_ELBOW_R] = RX(ex); P.motion[E_FINGERS_R] = RX(fing);
            P.motion[E_ARML] = RX(lrx) * RZ(lrz); P.motion[E_ELBOW_L] = RX(lex);
            float lit = L.spine() + (L.complete() ? 0.3f : 0.f);
            P.chan[C_EYE] = cyan * (0.45f + 0.55f * std::min(lit, 1.f));
            P.chan[C_CORE] = cyan * (0.12f + 1.f * lit);
            Rig rg{out};
            drawFile(rg, T(at) * RY(yaw), *U, P, cyan * (0.2f + 0.8f * std::min(lit, 1.f)), W);
            unit = true;
            const vec3 up{0.f, 1.f, 0.f}, fwd{std::sin(yaw), 0.f, std::cos(yaw)};
            labEndSeat[0] = origin(W[E_ARML]) + up * 0.1f;
            labEndSeat[1] = glm::mix(origin(W[E_ELBOW_L]), origin(W[E_HAND_L]), 0.45f);
            labEndSeat[2] = origin(W[E_TORSO]) + up * 0.3f - fwd * 0.17f;
            labEndHand = origin(W[E_HAND_R]);
            labEndHaveJoints = body;
        }
    // The pieces: on the body (its colours), torn into the hand, carried home, seated (edged in its light)
    const float ball = (float)meshlib::lumpBall();
    for (int p = 0; p < 3; ++p) {
        if (!body) break;
        vec3 at = labEndFrom[p];
        float spin = 0.f, heat = 0.f;
        mat4 R(1.f);
        if (cut && unit && L.taken(p)) {
            float tr = L.tear(p), c = L.carry(p);
            vec3 inHand = labEndHand;
            at = c <= 0.f ? glm::mix(labEndFrom[p], inHand, tr) : glm::mix(inHand, labEndSeat[p], c);
            spin = (1.f - c) * tr * 0.6f;
            heat = L.seated(p) ? 1.f : c;
            R = RY(yaw + spin * std::sin(t * 3.f)) * RX(0.4f * spin);
            if (p == 1 && c > 0.f) R = gleaner::alongZ(vec3{0.f}, origin(W[E_HAND_L]) - origin(W[E_ELBOW_L])) ;   // along the forearm
        } else if (!cut && labEndingDone) continue;   // (taken: gone from the body)
        else R = RY(successorFellYaw) * RX(p == 2 ? 1.5708f : 0.f);
        const vec3 seam = glm::mix(red * 0.8f, cyan * 1.3f, heat);
        mat4 M = T(at) * R * S(vec3{0.6f});   // (sized to UNIT 24: a plate, a sleeve, a coupling, not slabs)
        if (p == 0) {   // the shoulder plate: a pauldron of three overlapping plates
            for (int k = 0; k < 3; ++k) push(out, M * T(vec3{0.f, 0.05f - k * 0.05f, -0.05f + k * 0.06f}) * RX(0.25f * k) * S(vec3{0.34f - 0.04f * k, 0.06f, 0.24f}), ivory * (1.f - 0.08f * k), vec3{0.f});
            push(out, M * T(vec3{0.f, 0.085f, 0.f}) * S(vec3{0.3f, 0.015f, 0.03f}), vec3{0.f}, seam);
        } else if (p == 1) {   // the forearm housing: a sleeve, the splinter's slot along it
            for (int k = 0; k < 4; ++k) {
                float a = k * 1.5708f;
                push(out, M * RZ(a) * T(vec3{0.f, 0.075f, 0.f}) * S(vec3{0.12f, 0.035f, 0.3f}), k == 0 ? ivory : ivory * 0.7f, vec3{0.f});
            }
            float open = cut ? L.splinter() : 0.f;
            push(out, M * T(vec3{0.f, 0.096f + 0.03f * open, 0.f}) * S(vec3{0.03f, 0.012f, 0.26f}), vec3{0.f}, seam * (1.f + 1.5f * open));
            if (open > 0.01f) {   // the SPLINTER, once: a shard of glass out of the slot, glinting
                push(out, M * T(vec3{0.f, 0.12f + 0.11f * open, 0.05f}) * RY(t * 4.f) * RZ(0.5f) * S(vec3{0.03f, 0.13f * open, 0.012f}), vec3{0.5f, 0.8f, 0.9f}, cyan * (1.6f + 1.2f * std::max(0.f, std::sin(t * 9.f))));
            }
        } else {   // the spine coupling: four vertebrae round a core
            for (int k = 0; k < 4; ++k) push(out, M * T(vec3{0.f, -0.13f + k * 0.085f, 0.f}) * S(vec3{0.17f - 0.015f * k, 0.05f, 0.1f}), ivory * 0.9f, vec3{0.f});
            push(out, M * S(vec3{0.05f, 0.33f, 0.05f}), vec3{0.f}, seam * 1.2f);
            out.back().mesh = ball;
        }
    }
    // The portal, taking it: light pouring out of the vortex toward the platform
    if (cut && L.time > LabEnding::PULL0 - 1.f && level.story.labPortalRadius > 0.f) {
        const vec3 P = level.story.labPortal;
        float g = L.sm(LabEnding::PULL0 - 1.f, LabEnding::THROUGH, L.time);
        for (int k = 0; k < 40; ++k) {
            float a = k * 2.399f + gameClock * (0.6f + 0.02f * k), r = level.story.labPortalRadius * (0.15f + 0.8f * std::fmod(k * 0.618f, 1.f));
            float u = std::fmod(gameClock * 0.5f + k * 0.13f, 1.f);
            vec3 q = glm::mix(P + vec3{std::cos(a) * r, std::sin(a) * r, 0.f}, labend::edge() + vec3{0.f, 2.f, 0.f}, u * 0.35f);
            push(out, T(q) * RZ(a) * S(vec3{0.12f, 0.12f, 3.f + 6.f * u}), vec3{0.f}, glm::mix(vec3{1.2f, 0.3f, 1.4f}, vec3{1.8f, 1.7f, 1.9f}, g) * g * (1.f - u));
        }
        push(out, T(P + vec3{0.f, 0.f, 1.f}) * S(vec3{level.story.labPortalRadius * 1.6f * (0.2f + 0.8f * g), level.story.labPortalRadius * 1.6f * (0.2f + 0.8f * g), 0.3f}), vec3{0.f}, vec3{1.6f, 1.4f, 1.8f} * g * 2.f);
        out.back().mesh = ball;
    }
}

// Over the cutscene: the bars, its nameplate, the skip prompt, the white
inline void GameplayState::renderLabEndingOverlay() {
    ui.begin2D(); auto& u = ui.ui;
    const LabEnding& L = labEnding;
    int bar = (int)(70.f * L.bars());
    if (bar > 0) { u.rect(0, 0, SCREEN_W, bar, {0.f, 0.f, 0.02f, 1}); u.rect(0, SCREEN_H - bar, SCREEN_W, bar, {0.f, 0.f, 0.02f, 1}); }
    float a = L.nameplate();
    if (a > 0.01f) {
        bool done = L.complete();
        u.text("UNIT 24  /  PURPOSE: --------  /  STATUS:", SCREEN_W / 2 - 70, SCREEN_H - 52, 2, {0.55f, 0.9f, 1.f, a}, true);
        u.text(done ? "COMPLETE" : "REJECTED", SCREEN_W / 2 + 300, SCREEN_H - 52, 2, done ? glm::vec4{0.4f, 1.f, 1.f, a} : glm::vec4{1.f, 0.3f, 0.25f, a}, true);
    }
    std::string skip = "HOLD " + binds().label(Act::JUMP) + " TO SKIP";
    u.text(skip.c_str(), SCREEN_W / 2, SCREEN_H - 24, 1, {0.5f, 0.75f, 0.85f, 0.8f * L.bars()}, true);
    if (L.skip > 0.f) u.rect(SCREEN_W / 2 - 90, SCREEN_H - 12, (int)(180.f * L.skip / LabEnding::SKIP_HOLD), 3, {0.35f, 0.85f, 1.f, 1});
    float w = L.white();
    if (w > 0.001f) u.rect(0, 0, SCREEN_W, SCREEN_H, {0.97f, 0.96f, 1.f, w});
    ui.end2D();
}
