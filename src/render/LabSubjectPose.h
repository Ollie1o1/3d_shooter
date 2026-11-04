#pragma once
// =============================================================================
// LabSubjectPose.h — how the Living Ward's four subjects move (included by
// EnemyModel.h inside namespace rig; the Successor has its own spring rig).
//
//   PATIENT    a feral sprint bent double: long strides, arms pumping against
//              the legs, jaw working, gown and torn straps trailing. Idle it
//              hunches and twitches. It crouches and shrieks before a lunge,
//              flies at you arms-first, and rears back to rake with a claw.
//   GRAFTED    a heavy stomp: wide roll from side to side, the graft arm
//              swinging like dead weight, the press raised over its head.
//   PHASED     the patient's run with arms trailing behind it, and wrong:
//              the pose stutters between frames and limbs skip out of place.
//   SUSPENDED  hangs limp from its hoist, swaying, legs dangling, head lolled,
//              until the beam: it snaps upright, arms flung wide, convulsing.
//
// Everything is angles first (so attack poses blend over the gait), then
// joint matrices. In the angles: hipX > 0 swings a leg forward, kneeX < 0
// bends a knee, torsoX < 0 leans forward, armX < 0 raises an arm forward,
// headX < 0 looks up, RZ(+) takes the left arm out. (The rig's own RX runs
// the other way for hips, knees and torso: "to joints" flips them.)
// =============================================================================

inline float labNoise(float a, float b) { float v = std::sin(a * 12.9898f + b * 78.233f) * 43758.5453f; return v - std::floor(v); }

inline void labSubjectPose(const Enemy& e, float time, const mat4& root, mat4& fileRoot, FilePose& P, vec3& glowOut) {
    using namespace enemyfile;
    const EnemyStats& st = e.stats();
    const bool heavy = e.type == EnemyType::GRAFTED, phased = e.type == EnemyType::PHASED, hung = e.type == EnemyType::SUSPENDED;
    const float id = (float)e.uid;
    float tp = e.telegraphProgress();
    vec3 glow = st.glow * (1.f + 2.5f * tp);
    P = FilePose{};
    fileRoot = root;

    float t = time, ph = e.animPhase;
    if (phased) { t = std::floor(time * 10.f) / 10.f; ph = std::floor(e.animPhase * 2.2f) / 2.2f; }   // it skips frames
    const float run = glm::clamp(e.moveSpeed / std::max(.1f, st.speed), 0.f, 1.3f);
    const float s = std::sin(ph), c = std::cos(ph), breath = std::sin(t * 1.9f + id);
    // A twitch: now and then the head snaps aside and eases back
    float win = std::floor(t * 2.3f + id * .37f), into = t * 2.3f + id * .37f - win;
    float jerk = labNoise(win, id) > .74f ? (labNoise(win, id + 1.f) - .5f) * 2.f * (1.f - smooth01c(into * 1.6f)) : 0.f;

    // ---- the angles ----
    float hipX[2], hipZ[2] = {0, 0}, kneeX[2], ankleX[2], armX[2], armZ[2], elbowX[2], finger[2], ribbon[2];
    float waistY = 0, waistZ = 0, torsoX, torsoY = 0, torsoZ = 0, headX, headY = 0, headZ = 0, jaw, clothF, clothB, lift;
    const float lean = heavy ? .1f + .12f * run : phased ? .25f + .5f * run : .24f + .44f * run;
    const float stride = heavy ? .55f : 1.f;
    for (int k = 0; k < 2; ++k) {
        const float sg = k ? -1.f : 1.f;                // left leg forward when sin > 0, the right opposite
        const float swing = s * sg, rise = std::max(0.f, c * sg);
        hipX[k] = swing * stride * run + .14f * run + (1 - run) * .08f;
        kneeX[k] = -(.16f + (heavy ? .7f : 1.55f) * rise) * run - .14f - (1 - run) * .12f;
        ankleX[k] = .35f * rise * run - .08f * (1 - run);
        // Arms pump against the legs: the left arm back while the left leg is forward
        armX[k] = (heavy ? .5f : 1.05f) * swing * run + (heavy ? 0.f : .12f * run) + lean * .35f * (1 - run);
        armZ[k] = sg * (.14f + .16f * run);
        elbowX[k] = -(.32f + .95f * run) - .3f * std::max(0.f, -swing) * run;
        finger[k] = -.45f - .25f * std::sin(t * 7.f + id + k * 1.7f) * (labNoise(std::floor(t * 4.f), id + k) > .5f ? 1.f : .3f);
        ribbon[k] = .55f * run + .25f * std::sin(ph + 1.2f + k * 3.14f) * run + .1f * std::sin(t * 2.7f + id + k);
    }
    torsoX = -lean + .035f * breath;
    torsoY = -s * .17f * run;
    torsoZ = std::sin(t * .8f + id) * .07f * (1 - run);
    waistY = s * .15f * run; waistZ = c * .05f * run;
    headX = -lean * .85f + .05f * breath;
    headY = jerk * .6f;
    headZ = jerk * .4f + std::sin(t * .7f + id) * .16f * (1 - run);
    jaw = .14f + .06f * breath + .22f * run * std::fabs(s);
    clothF = -(.2f + .25f * std::fabs(s)) * run + .03f * breath;
    clothB = .65f * run + .1f * std::sin(t * 11.f + id) * run;
    lift = run * ((heavy ? .05f : .09f) * std::fabs(c) - (heavy ? .07f : .13f)) - (heavy ? 0.f : .06f) * (1 - run);

    if (heavy) {
        // Weight: a big roll over each planted foot, the graft arm (right) swinging like a sack
        waistZ = s * .13f * run; torsoZ = -s * .09f * run;
        torsoX += .05f * std::sin(t * 2.2f + id);      // breathing hard through the respirator
        headX = .12f - .1f * run; headY = std::sin(t * .45f + id) * .3f * (1 - run);
        armX[1] = -s * .35f * run + .18f; armZ[1] = -.3f; elbowX[1] = -.3f;
        armX[0] = s * .45f * run + .1f; elbowX[0] = -.65f;
        jaw = .05f;
    }
    if (phased) {
        // Arms trailing straight back as it runs, and every so often a limb skips out of place
        for (int k = 0; k < 2; ++k) { float sg = k ? -1.f : 1.f;
            armX[k] = glm::mix(armX[k], 1.15f + .12f * s * sg, run); armZ[k] = sg * (.22f + .1f * run); elbowX[k] = glm::mix(elbowX[k], -.15f, run); }
        float g = std::floor(time * 6.f);
        if (labNoise(g, id) > .78f) {
            headZ += (labNoise(g, id + 2) - .5f) * 1.4f; headY += (labNoise(g, id + 3) - .5f) * 1.2f;
            armZ[(int)(labNoise(g, id + 4) * 2)] += (labNoise(g, id + 5) - .5f) * 1.6f;
            torsoY += (labNoise(g, id + 6) - .5f) * .7f;
            fileRoot = fileRoot * T({(labNoise(g, id + 7) - .5f) * .25f, 0.f, 0.f});
        }
    }
    if (hung) {
        // Limp from its hoist: legs dangling and swinging out of step, toes pointed, head lolled
        for (int k = 0; k < 2; ++k) {
            float w = std::sin(t * 1.3f + id + k * 1.1f);
            hipX[k] = .1f + .13f * w + .2f * run; hipZ[k] = (k ? -1.f : 1.f) * .04f;
            kneeX[k] = -.3f - .16f * std::sin(t * 1.3f + id + k * 1.1f - .6f) - .2f * run;
            ankleX[k] = .75f;
            armX[k] = -.05f + .08f * std::sin(t * .9f + id + k); armZ[k] = (k ? -1.f : 1.f) * .12f; elbowX[k] = -.25f;
            finger[k] = -.3f; ribbon[k] = .2f * std::sin(t * 1.2f + id + k * 2.f) + .3f * run;
        }
        torsoX = -.1f; torsoY = 0; torsoZ = 0; waistY = waistZ = 0;
        headX = .42f + .08f * breath; headY = 0; headZ = .3f * std::sin(t * .5f + id);
        jaw = .1f; clothF = clothB = 0; lift = 0;
    }

    // ---- attacks over the gait ----
    auto blend = [](float& a, float b, float k) { a = glm::mix(a, b, k); };
    const bool tell = e.telegraphTimer > 0.f;
    const float k = smooth01c(tp);
    if (tell && e.attack == AttackKind::LAB_LUNGE) {
        // Crouch, arms drawn back like wings, head up, jaw wide: the shriek before the pounce
        for (int i = 0; i < 2; ++i) { float sg = i ? -1.f : 1.f;
            blend(hipX[i], .9f, k); blend(kneeX[i], -1.65f, k); blend(ankleX[i], -.45f, k);
            blend(armX[i], .95f, k); blend(armZ[i], sg * .75f, k); blend(elbowX[i], -1.3f, k); blend(finger[i], .3f, k); }
        blend(torsoX, -(lean + .35f), k); blend(headX, -(lean * .85f + .45f), k); blend(jaw, 1.f, k);
        torsoZ += std::sin(time * 41.f + id) * .035f * k;
        lift -= .42f * k;
    }
    if (e.dashTimer > 0.f && !heavy) {
        // In flight: arms flung forward to grab, one leg trailing, an arc through the air
        float u = 1.f - glm::clamp(e.dashTimer / .38f, 0.f, 1.f);
        for (int i = 0; i < 2; ++i) { float sg = i ? -1.f : 1.f;
            armX[i] = -1.95f; armZ[i] = sg * .35f; elbowX[i] = -.15f; finger[i] = .35f; }
        hipX[0] = .35f; kneeX[0] = -.5f; hipX[1] = -.55f; kneeX[1] = -1.3f; ankleX[0] = ankleX[1] = .5f;
        torsoX = -.8f; headX = -.55f; jaw = 1.f; clothB = 1.f; clothF = .5f;
        lift = std::sin(3.14159f * u) * .65f;
    }
    if (e.attack == AttackKind::LAB_SWIPE || e.labSwipe > 0.f) {
        const int hand = e.labSwipes % 2 ? 0 : 1;        // right first, then the left
        const float side = hand ? 1.f : -1.f;           // the twist that winds the striking side back
        if (tell && e.attack == AttackKind::LAB_SWIPE) {
            // Rear back, the claw raised high and out, twisted away, jaw open
            blend(armX[hand], -2.5f, k); blend(armZ[hand], -side * .55f, k); blend(elbowX[hand], -1.05f, k);
            blend(armX[1 - hand], .45f, k); blend(finger[hand], .35f, k);
            blend(torsoY, side * .55f, k); blend(torsoX, -lean * .5f + .12f, k); blend(jaw, .75f, k);
            lift -= .1f * k;
        } else {
            // The rake: down and across fast, then settle
            float u = 1.f - e.labSwipe / .32f, a = smooth01c(u / .35f), w = 1.f - smooth01c((u - .35f) / .65f);
            blend(armX[hand], glm::mix(-2.5f, .55f, a), w); blend(armZ[hand], glm::mix(-side * .55f, side * .4f, a), w);
            blend(elbowX[hand], glm::mix(-1.05f, -.2f, a), w); blend(finger[hand], .35f, w);
            blend(torsoY, glm::mix(side * .55f, -side * .6f, a), w); blend(torsoX, -lean - .25f * a, w);
            blend(jaw, .6f, w); lift -= .12f * a * w;
        }
    }
    if (tell && e.attack == AttackKind::LAB_PRESS) {
        // Both fists hauled up over its head, leaning back into it
        for (int i = 0; i < 2; ++i) { float sg = i ? -1.f : 1.f;
            blend(armX[i], -2.75f, k); blend(armZ[i], sg * .32f, k); blend(elbowX[i], -.85f, k); }
        blend(torsoX, .3f, k); blend(headX, -.3f, k); blend(jaw, .5f, k);
        for (int i = 0; i < 2; ++i) blend(kneeX[i], -.5f, k);
        lift += .1f * k - .15f * k * k * k;
    }
    if (tell && e.attack == AttackKind::VOLLEY) {
        // The heavy orb gathered in its open left hand, held out
        blend(armX[0], -1.5f, k); blend(armZ[0], .2f, k); blend(elbowX[0], -.1f, k); blend(finger[0], .4f, k);
        blend(torsoY, .4f, k); blend(armX[1], .3f, k);
    }
    if (phased && ((tell && e.attack == AttackKind::LAB_CROSS) || e.labCross > 0.f)) {
        // Coming apart: arms spread, head thrown back, shaking
        float kk = e.labCross > 0.f ? 1.f : k;
        for (int i = 0; i < 2; ++i) { float sg = i ? -1.f : 1.f;
            blend(armZ[i], sg * 1.35f, kk); blend(armX[i], -.2f, kk); blend(elbowX[i], -.1f, kk); blend(finger[i], .4f, kk); }
        blend(headX, -.65f, kk); blend(torsoX, .22f, kk); blend(jaw, .9f, kk);
        torsoZ += std::sin(time * 47.f) * .08f * kk; headZ += std::sin(time * 39.f + 1) * .15f * kk;
        lift += .15f * kk;
    }
    if (hung && (e.attack == AttackKind::LAB_BEAM || e.labBeam > 0.f)) {
        // It wakes: snaps upright, legs drawn up, arms flung wide, convulsing
        float kk = e.labBeam > 0.f ? 1.f : (tell ? k : 0.f);
        for (int i = 0; i < 2; ++i) { float sg = i ? -1.f : 1.f;
            blend(armZ[i], sg * (.3f + 1.25f), kk); blend(armX[i], -.3f, kk); blend(elbowX[i], -.1f, kk); blend(finger[i], .45f, kk);
            blend(hipX[i], .45f, kk); blend(kneeX[i], -.95f, kk); }
        blend(headX, -.45f, kk); blend(torsoX, .15f, kk); blend(jaw, .45f, kk); headZ *= 1 - kk;
        torsoZ += std::sin(time * 37.f + id) * .05f * kk;
    }
    if (!hung && (e.staggered() || e.stunned())) {
        // Reeling: arched back, head thrown back, arms flung, a shriek
        float q = std::sin(time * 23.f + id) * .06f;
        for (int i = 0; i < 2; ++i) { float sg = i ? -1.f : 1.f;
            armX[i] = -.45f + q; armZ[i] = sg * 1.f; elbowX[i] = -.4f; kneeX[i] = -.55f; hipX[i] = .25f; }
        torsoX = .45f + q; headX = -.5f; jaw = .85f; lift = -.15f;
    } else if (!hung && e.labOpen > 0.f && e.recoverTimer > 0.f && !tell && e.dashTimer <= 0.f && e.labSwipe <= 0.f) {
        // Spent and exposed: slumped, arms hanging, heaving
        torsoX = -.3f + .06f * std::sin(time * 9.f); headX = .45f;
        for (int i = 0; i < 2; ++i) { armX[i] = .1f; elbowX[i] = -.2f; }
        jaw = .4f;
    }

    // ---- to joints ----
    if (hung) {
        // Sway from the hoist: the body trails as it moves and rocks on its cables
        vec3 pivot{0.f, 2.72f, 0.f};
        float pitch = std::sin(t * 1.1f + id) * .07f + .2f * run, roll = std::sin(t * .83f + id * 1.3f) * .06f;
        if (e.attack == AttackKind::LAB_BEAM || e.labBeam > 0.f) pitch *= .3f;
        fileRoot = fileRoot * T(pivot) * RX(pitch) * RZ(roll) * T(-pivot);
    }
    fileRoot = fileRoot * T({0.f, lift, 0.f});
    P.motion[E_WAIST] = RY(waistY) * RZ(waistZ);
    P.motion[E_TORSO] = RX(-torsoX) * RY(torsoY) * RZ(torsoZ);
    P.motion[E_HEAD] = RX(headX) * RY(headY) * RZ(headZ);
    P.motion[E_JAW] = RX(glm::clamp(jaw, 0.f, 1.f) * .5f);
    const int HIP[2] = {E_HIPL, E_HIPR}, KNEE[2] = {E_KNEE_L, E_KNEE_R}, ANKLE[2] = {E_ANKLE_L, E_ANKLE_R},
              ARM[2] = {E_ARML, E_ARMR}, ELBOW[2] = {E_ELBOW_L, E_ELBOW_R}, HAND[2] = {E_HAND_L, E_HAND_R},
              FING[2] = {E_FINGERS_L, E_FINGERS_R}, THUMB[2] = {E_THUMB_L, E_THUMB_R}, RIB[2] = {E_RIBBON_L, E_RIBBON_R};
    for (int i = 0; i < 2; ++i) {
        P.motion[HIP[i]] = RX(-hipX[i]) * RZ(hipZ[i]);
        P.motion[KNEE[i]] = RX(-kneeX[i]);
        P.motion[ANKLE[i]] = RX(ankleX[i]);
        P.motion[ARM[i]] = RX(armX[i]) * RZ(armZ[i]);
        P.motion[ELBOW[i]] = RX(elbowX[i]);
        P.motion[HAND[i]] = RX(-.12f + finger[i] * .2f);
        P.motion[FING[i]] = RX(finger[i]);
        P.motion[THUMB[i]] = RX(finger[i] * .6f);
        P.motion[RIB[i]] = RX(ribbon[i]) * RZ((i ? -1.f : 1.f) * .1f * std::sin(t * 3.1f + id + i));
    }
    P.motion[E_CLOTH_F] = RX(clothF);
    P.motion[E_CLOTH_B] = RX(clothB);

    // ---- light ----
    P.chan[C_EYE] = st.glow; P.chan[C_CORE] = glow; P.chan[C_GLOW] = glow;
    float flicker = phased ? (labNoise(std::floor(time * 14.f), id) > .2f ? 1.f : .25f) : .75f + .25f * std::sin(time * 3.f + id);
    P.chan[C_SEAM] = st.glow * flicker;
    vec3 tellCol = e.parryWindow() ? vec3{2.2f} : e.attack == AttackKind::LAB_PRESS ? abGoldTell(vec3{1.8f, 1.1f, .15f})
                 : e.attack == AttackKind::LAB_BEAM ? vec3{1.3f, .45f, 2.f} : st.glow;
    if (tell || e.dashTimer > 0.f) { P.chan[C_EYE] = tellCol * 1.3f; P.chan[C_CORE] = tellCol * (1 + tp); P.chan[C_SEAM] = tellCol; }
    if (e.labOpen > 0.f || e.staggered()) P.chan[C_CORE] = vec3{.25f, 1.6f, 1.8f} * (.85f + .15f * std::sin(time * 12.f));
    glowOut = glow;
}
