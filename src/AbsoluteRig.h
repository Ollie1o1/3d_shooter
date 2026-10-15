#pragma once
// =============================================================================
// AbsoluteRig.h — THE ABSOLUTE's body (included by EnemyModel.h inside
// namespace rig). A 2.6 m figure of white light, built finer than anything
// else in the game: thighs, knee caps, shins, calves and feet; a waist ring
// and hanging wraps; abdomen plates, obliques, ribs, pectorals, collarbones, a
// spine and shoulder blades; a skull with brow, cheekbones and a gold face
// slit; deltoids, biceps, elbows, forearms with wraps trailing light, hands
// with jointed fingers and thumbs; a broken halo turning behind his head; in
// phase 3 a blade of light. Every tell lights the striking limb in its colour
// (white: parry; gold: dodge or block; red: get out).
// Joints: hip / knee / ankle, shoulder / elbow / wrist / knuckle. Right is -X.
// =============================================================================

struct AbsPose {
    float crouch = 0.f, lift = 0.f, torsoYaw = 0.f, torsoPitch = 0.f, headPitch = 0.f;
    float shR = 0.f, shRz = 0.f, elR = 0.f, shL = 0.f, shLz = 0.f, elL = 0.f;   // shoulder swing (- forward/up), splay (+ out), elbow bend
    float hipR = 0.f, kneeR = 0.f, hipL = 0.f, kneeL = 0.f;                      // thigh swing (- forward), knee bend
    float fistR = 1.f, fistL = 1.f;                                              // 0 an open hand .. 1 a fist
    int   glow = -1;   // what lights with the tell: 0 right arm, 1 left arm, 2 right leg, 3 both arms, 4 the blade, 5 all of him
};

inline AbsPose absolutePose(const Enemy& e, float time) {
    AbsPose P;
    const float tp = e.telegraphProgress(), stride = glm::clamp(e.moveSpeed / 11.f, 0.f, 1.2f);
    const float ph = e.animPhase;
    // a boxer's guard: fists by his chin, the lead (left) hand forward, side-on, a bounce on his toes
    const float br = std::sin(time * 2.2f) * 0.03f, bounce = std::fabs(std::sin(time * 5.f)) * 0.035f;
    P.crouch = 0.08f + br - bounce; P.torsoYaw = 0.25f;
    P.shR = -0.6f; P.elR = 1.9f; P.shRz = -0.2f;             // rear hand: elbow tucked in, fist at the jaw
    P.shL = -0.75f; P.elL = 1.6f; P.shLz = -0.35f;           // lead hand: further out in front, at chin height
    P.hipR = -0.05f; P.kneeR = 0.28f; P.hipL = -0.22f; P.kneeL = 0.3f;
    if (stride > 0.1f) {   // closing in: the guard stays up, the feet do the work
        float sw = std::sin(ph) * 0.6f * stride;
        P.hipR = sw - 0.05f; P.hipL = -sw - 0.3f;
        P.kneeR = 0.35f + std::max(0.f, -std::sin(ph)) * 1.1f * stride;
        P.kneeL = 0.35f + std::max(0.f, std::sin(ph)) * 1.1f * stride;
        P.shR = -0.6f; P.shL = -0.75f; P.elR = 1.9f; P.elL = 1.6f;
        P.torsoPitch = 0.18f * stride; P.torsoYaw = 0.25f + std::sin(ph) * 0.08f * stride;
    }
    const bool late = tp > 0.78f;   // the strike itself
    if (e.abAirborne() && !e.abDiving) {   // aloft: arms swept back, legs trailing, the guard gone
        P.shR = P.shL = 0.55f; P.shRz = P.shLz = 0.35f; P.elR = P.elL = 0.35f; P.fistR = P.fistL = 0.6f;
        P.hipR = 0.35f; P.hipL = 0.2f; P.kneeR = 0.6f; P.kneeL = 0.9f; P.torsoPitch = 0.3f; P.crouch = 0.f;
    }
    switch (e.telegraphTimer > 0.f ? e.attack : AttackKind::NONE) {
    case AttackKind::AJAB:
        P.glow = 0; P.torsoYaw = late ? -0.3f : 0.35f;
        P.shR = late ? -1.6f : -0.55f; P.elR = late ? 0.08f : 2.3f; P.crouch = 0.14f;
        break;
    case AttackKind::AHOOK:
        P.glow = 1; P.torsoYaw = late ? 0.65f : -0.55f;
        P.shL = -1.45f; P.shLz = late ? -0.35f : 1.0f; P.elL = 1.35f; P.crouch = 0.16f;
        break;
    case AttackKind::AUPPER:
        P.glow = 0; P.crouch = 0.42f * smooth01(tp * 1.3f); P.torsoPitch = late ? -0.25f : 0.35f;
        P.shR = late ? -2.7f : 0.55f; P.elR = late ? 0.6f : 1.4f; P.kneeR = P.kneeL = 0.9f; P.hipR = P.hipL = -0.5f;
        break;
    case AttackKind::ADROP:
        P.glow = 2; P.lift = 2.6f * smooth01(tp); P.torsoPitch = -0.35f;
        P.hipR = -1.55f; P.kneeR = 0.05f; P.hipL = 0.45f; P.kneeL = 1.7f; P.shR = 0.6f; P.shL = 0.5f; P.shRz = P.shLz = 0.7f;
        break;
    case AttackKind::ARAY: case AttackKind::AZENITH: case AttackKind::ASEAMS: case AttackKind::AHALO:
        P.glow = 0; P.fistR = 0.f;
        if (e.attack == AttackKind::ARAY) { P.shR = -1.57f; P.elR = 0.f; P.torsoYaw = -0.25f; P.shL = -0.3f; P.elL = 0.6f; }
        else { P.shR = -3.0f; P.elR = 0.1f; P.shL = -0.2f; P.shLz = 0.5f; P.headPitch = -0.35f; }   // one hand to the sky
        break;
    case AttackKind::ASTOMP:
        P.glow = 2; P.hipR = -1.45f * smooth01(tp * 1.5f); P.kneeR = 1.7f; P.shRz = P.shLz = 0.9f; P.shR = P.shL = -0.4f; P.elR = P.elL = 0.5f;
        break;
    case AttackKind::ASERPENT:
        P.glow = 3; P.fistR = P.fistL = 0.2f;
        P.shR = P.shL = -1.45f; P.shRz = P.shLz = -0.25f; P.elR = P.elL = 0.25f;
        break;
    case AttackKind::ATELEPORT: case AttackKind::AHOP: case AttackKind::ADASH:
        P.torsoPitch = 0.4f; P.shR = P.shL = 0.7f; P.elR = P.elL = 0.4f; P.crouch = 0.2f;
        break;
    case AttackKind::ASWEEP:
        P.glow = 4; P.torsoYaw = late ? -0.75f : 0.85f; P.shR = -1.35f; P.shRz = late ? -0.2f : 0.75f; P.elR = 0.3f;
        P.shL = -0.8f; P.elL = 1.6f; P.crouch = 0.2f;
        break;
    case AttackKind::ALUNGE:
        P.glow = 4; P.crouch = 0.32f; P.torsoPitch = 0.32f; P.shR = -1.6f; P.elR = 0.05f; P.hipR = -0.9f; P.kneeR = 0.9f; P.hipL = 0.6f;
        break;
    case AttackKind::ACRESCENT:
        P.glow = 4; P.shR = late ? -1.1f : -2.85f; P.elR = 0.2f; P.torsoPitch = late ? 0.25f : -0.2f;
        break;
    case AttackKind::ACROSS:     // the rear hand, straight down the middle, hips turning through it
        P.glow = 0; P.torsoYaw = late ? -0.55f : 0.55f; P.shR = late ? -1.65f : -1.0f; P.elR = late ? 0.05f : 2.4f; P.hipR = late ? 0.2f : -0.1f; P.crouch = 0.15f;
        break;
    case AttackKind::ABACKFIST:  // a spin, the back of the fist
        P.glow = 1; P.torsoYaw = late ? 1.4f : -1.1f; P.shL = -1.5f; P.shLz = late ? 0.2f : 1.3f; P.elL = late ? 0.15f : 1.6f;
        break;
    case AttackKind::AELBOW:     // short, the elbow leading
        P.glow = 0; P.torsoYaw = late ? -0.6f : 0.4f; P.shR = -1.5f; P.shRz = 0.9f; P.elR = 2.6f; P.crouch = 0.12f;
        break;
    case AttackKind::AROUNDHOUSE: // the shin round at your ribs
        P.glow = 2; P.torsoYaw = late ? -1.2f : 0.6f; P.torsoPitch = -0.15f; P.hipR = late ? -1.5f : -0.5f; P.kneeR = late ? 0.15f : 1.6f;
        P.shR = 0.3f; P.shRz = 0.6f; P.shL = -1.3f; P.elL = 2.f;
        break;
    case AttackKind::AHAYMAKER:  // wound right back, then over the top
        P.glow = 0; P.torsoYaw = late ? -0.8f : 1.0f; P.torsoPitch = late ? 0.35f : -0.2f;
        P.shR = late ? -2.2f : 0.9f; P.shRz = late ? 0.1f : 0.7f; P.elR = late ? 0.4f : 1.8f; P.crouch = 0.25f;
        break;
    case AttackKind::ACHARGE:    // head down, shoulder first
        P.glow = 5; P.torsoPitch = 0.75f; P.torsoYaw = 0.6f; P.shL = -0.4f; P.elL = 2.2f; P.shR = 0.5f; P.elR = 1.2f; P.crouch = 0.3f;
        break;
    case AttackKind::AAXE:       // the heel up past his head, then straight down
        P.glow = 2; P.hipR = late ? -0.3f : -2.6f; P.kneeR = late ? 0.1f : 0.05f; P.torsoPitch = late ? 0.25f : -0.3f; P.shR = P.shL = -0.5f; P.shRz = P.shLz = 0.7f;
        break;
    case AttackKind::ACLAP:      // both arms wide, then palms together in front
        P.glow = 3; P.fistR = P.fistL = 0.f; P.shR = P.shL = -1.5f; P.shRz = P.shLz = late ? -0.2f : 1.3f; P.elR = P.elL = 0.15f;
        break;
    case AttackKind::ASKYFALL:   // knees up, both fists raised over his head, then down like a hammer
        P.glow = 3; P.shR = P.shL = late ? -0.4f : -2.9f; P.elR = P.elL = 0.3f; P.hipR = P.hipL = -1.1f; P.kneeR = P.kneeL = 1.6f; P.torsoPitch = late ? 0.4f : -0.2f;
        break;
    case AttackKind::AMETEOR:    // one fist first, the body behind it like a comet
        P.glow = 0; P.shR = -2.9f; P.elR = 0.05f; P.shL = 0.6f; P.elL = 0.4f; P.torsoPitch = 0.9f; P.hipR = P.hipL = 0.4f; P.kneeR = 0.3f; P.kneeL = 0.6f;
        break;
    case AttackKind::ATAKEOFF:   // a crouch, arms flung back, then up
        P.crouch = 0.4f * (1.f - tp); P.shR = P.shL = 0.6f; P.shRz = P.shLz = 0.6f;
        P.kneeR = P.kneeL = 1.1f * (1.f - tp); P.hipR = P.hipL = -0.6f * (1.f - tp);
        break;
    case AttackKind::AJUDGE:
        P.glow = 5; P.lift = 1.2f + 0.6f * tp; P.shR = P.shL = -1.57f; P.shRz = P.shLz = 1.25f; P.elR = P.elL = 0.f;
        P.fistR = P.fistL = 0.f; P.headPitch = -0.3f; P.hipR = P.hipL = 0.15f; P.kneeR = P.kneeL = 0.3f;
        break;
    default: break;
    }
    if (e.staggered()) {
        P = AbsPose{};
        if (e.abReeling) {   // thrown back on his heels
            P.torsoPitch = -0.4f; P.shR = P.shL = 0.25f; P.shRz = P.shLz = 0.55f; P.elR = P.elL = 0.4f; P.headPitch = -0.3f; P.crouch = 0.05f;
        } else {             // broken: down on one knee, head bowed
            P.crouch = 0.62f; P.hipR = -1.45f; P.kneeR = 1.5f; P.hipL = 0.55f; P.kneeL = 2.3f;
            P.torsoPitch = 0.38f; P.headPitch = 0.4f; P.shR = P.shL = 0.15f; P.shRz = P.shLz = 0.2f; P.elR = P.elL = 0.3f;
        }
    }
    return P;
}

// His head's frame (the rig and headBox share it, so the head you see is the head you hit)
inline mat4 absoluteHeadFrame(const Enemy& e, const mat4& root, const AbsPose& P) {
    float bob = std::fabs(std::sin(e.animPhase)) * 0.04f * glm::clamp(e.moveSpeed / 11.f, 0.f, 1.f);
    mat4 torso = root * T({0.f, P.lift - P.crouch + bob + 1.30f, 0.f}) * RY(P.torsoYaw) * RX(P.torsoPitch);
    return torso * T({0.f, 0.95f, 0.f}) * RX(P.headPitch);
}

inline void buildAbsolute(const Enemy& e, float time, Rig& r, const mat4& root) {
    const AbsPose P = absolutePose(e, time);
    const float tp = e.telegraphProgress();
    const bool  hurt = e.staggered() && !e.abReeling;
    const float pulse = 0.85f + 0.15f * std::sin(time * 3.f) * (hurt ? 3.f : 1.f);
    const vec3 skin{0.58f, 0.58f, 0.64f}, plate{0.96f, 0.96f, 0.99f}, dark{0.16f, 0.16f, 0.2f};
    const vec3 light{1.6f, 1.5f, 1.25f}, gold{1.5f, 1.1f, 0.45f};
    const vec3 seam = light * (hurt ? 0.5f : 1.3f) * pulse, soft = light * (hurt ? 0.04f : 0.1f);
    // the strike's colour, blazing as it comes
    const AbTell tell = e.telegraphTimer > 0.f ? abTellOf(e.attack) : AbTell::NONE;
    const vec3 tc = tell == AbTell::GOLD ? vec3{2.2f, 1.72f, 0.7f} : tell == AbTell::RED ? vec3{2.4f, 0.25f, 0.15f} : vec3{1.9f, 1.9f, 2.0f};
    const float tk = 0.5f + 3.f * tp;   // (always the brightest thing on him by the end)
    auto lit = [&](int limb, vec3 base) {
        if (tell == AbTell::NONE || P.glow < 0) return base;
        bool on = P.glow == limb || P.glow == 5 || (P.glow == 3 && (limb == 0 || limb == 1));
        return on ? tc * tk : base;
    };
    const float bob = std::fabs(std::sin(e.animPhase)) * 0.04f * glm::clamp(e.moveSpeed / 11.f, 0.f, 1.f);
    const mat4 body = root * T({0.f, P.lift - P.crouch + bob, 0.f});

    // ---- legs: thigh, its plate and two veins, knee cap, shin, calf, its seam, foot, two toes, heel, an ankle wrap ----
    for (float s : {-1.f, 1.f}) {
        const bool right = s < 0.f;
        const int limb = right ? 2 : 6;
        mat4 hip = body * T({s * 0.15f, 1.15f, 0.f}) * RX(right ? P.hipR : P.hipL) * RZ(s * 0.04f);
        r.box(hip, {0.f, -0.29f, 0.f}, {0.2f, 0.58f, 0.22f}, skin, soft);
        r.box(hip, {0.f, -0.27f, 0.08f}, {0.14f, 0.42f, 0.08f}, plate, soft);
        r.box(hip, {s * 0.105f, -0.3f, 0.f}, {0.02f, 0.4f, 0.03f}, dark, lit(limb, seam));
        r.box(hip, {-s * 0.105f, -0.24f, 0.02f}, {0.02f, 0.3f, 0.03f}, dark, lit(limb, seam * 0.6f));
        mat4 kn = hip * T({0.f, -0.58f, 0.f}) * RX(right ? P.kneeR : P.kneeL);
        r.box(kn, {0.f, 0.f, 0.09f}, {0.13f, 0.12f, 0.06f}, plate, lit(limb, soft));
        r.box(kn, {0.f, -0.27f, 0.f}, {0.15f, 0.52f, 0.16f}, skin, soft);
        r.box(kn, {0.f, -0.22f, -0.06f}, {0.12f, 0.32f, 0.08f}, plate);
        r.box(kn, {0.f, -0.27f, 0.085f}, {0.03f, 0.46f, 0.02f}, dark, lit(limb, seam));
        mat4 an = kn * T({0.f, -0.55f, 0.f}) * RX(-(right ? P.kneeR + P.hipR : P.kneeL + P.hipL) * 0.6f);
        r.box(an, {0.f, -0.03f, 0.05f}, {0.13f, 0.06f, 0.24f}, plate);
        r.box(an, {-0.03f, -0.035f, 0.19f}, {0.06f, 0.05f, 0.07f}, dark);
        r.box(an, {0.035f, -0.035f, 0.185f}, {0.05f, 0.045f, 0.06f}, dark);
        r.box(an, {0.f, 0.02f, -0.04f}, {0.11f, 0.08f, 0.09f}, skin);
        r.box(an, {0.f, 0.08f, 0.f}, {0.16f, 0.03f, 0.16f}, dark, lit(limb, light * 0.6f));
    }
    // ---- the waist: hips, a ring of light, wraps hanging front and back ----
    r.box(body, {0.f, 1.22f, 0.f}, {0.42f, 0.18f, 0.26f}, skin, soft);
    const mat4 waist = body * T({0.f, 1.33f, 0.f}) * RY(P.torsoYaw * 0.5f);
    for (int k = 0; k < 8; ++k)
        r.box(waist * RY(k * 0.7853982f), {0.f, 0.f, 0.17f}, {0.13f, 0.04f, 0.03f}, dark, seam * 0.8f);
    const float sway = std::sin(time * 1.9f) * 0.08f + glm::clamp(e.moveSpeed / 30.f, 0.f, 0.6f);
    for (int k = 0; k < 4; ++k) {
        float x = (k & 1) ? 0.08f : -0.08f, z = k < 2 ? 0.14f : -0.14f;
        r.box(body * T({x, 1.3f, z}) * RX((k < 2 ? -1.f : 1.f) * (sway + 0.05f * k)), {0.f, -0.28f, 0.f}, {0.1f, 0.56f, 0.015f}, dark, light * 0.3f);
    }
    // ---- the torso ----
    const mat4 torso = body * T({0.f, 1.30f, 0.f}) * RY(P.torsoYaw) * RX(P.torsoPitch);
    r.box(torso, {0.f, 0.2f, 0.f}, {0.34f, 0.4f, 0.24f}, skin, soft);                             // abdomen
    for (int row = 0; row < 3; ++row) for (float s : {-1.f, 1.f})                                  // its plates
        r.box(torso, {s * 0.06f, 0.08f + 0.12f * row, 0.125f}, {0.1f, 0.1f, 0.03f}, plate, soft);
    for (float s : {-1.f, 1.f}) {
        r.box(torso, {s * 0.19f, 0.2f, 0.02f}, {0.06f, 0.34f, 0.2f}, skin);                      // obliques
        for (int k = 0; k < 3; ++k)                                                                // ribs
            r.box(torso, {s * 0.22f, 0.42f + 0.08f * k, 0.04f}, {0.08f, 0.04f, 0.22f}, plate);
        r.box(torso, {s * 0.13f, 0.62f, 0.15f}, {0.22f, 0.2f, 0.04f}, plate, soft);              // pectorals
        r.box(torso * T({s * 0.13f, 0.78f, 0.1f}) * RZ(s * 0.15f), {0.f, 0.f, 0.f}, {0.24f, 0.035f, 0.04f}, plate);   // collarbones
        r.box(torso, {s * 0.14f, 0.8f, -0.04f}, {0.16f, 0.1f, 0.18f}, skin);                      // trapezius
        r.box(torso, {s * 0.13f, 0.6f, -0.15f}, {0.16f, 0.2f, 0.03f}, plate);                     // shoulder blades
        r.box(torso * T({s * 0.1f, 0.32f, 0.15f}) * RZ(s * 0.5f), {0.f, 0.f, 0.f}, {0.02f, 0.3f, 0.01f}, dark, seam);   // veins
    }
    r.box(torso, {0.f, 0.6f, 0.f}, {0.56f, 0.3f, 0.3f}, skin, soft);                              // chest
    r.box(torso, {0.f, 0.5f, 0.165f}, {0.025f, 0.5f, 0.015f}, dark, seam * 1.3f);                 // sternum seam
    r.box(torso, {0.f, 0.62f, 0.17f}, {0.08f, 0.08f, 0.02f}, light, seam * (1.2f + 0.4f * std::sin(time * 6.f)));   // the light in his chest
    for (int k = 0; k < 5; ++k)                                                                    // spine
        r.box(torso, {0.f, 0.1f + 0.15f * k, -0.14f}, {0.05f, 0.06f, 0.04f}, plate, seam * 0.4f);
    // ---- neck and head: throat seam, skull, jaw, brow, the gold slit, cheekbones, crown ridge ----
    const mat4 neck = torso * T({0.f, 0.85f, 0.f});
    r.box(neck, {0.f, 0.06f, 0.f}, {0.13f, 0.14f, 0.13f}, skin);
    r.box(neck, {0.f, 0.04f, 0.066f}, {0.03f, 0.1f, 0.01f}, dark, seam);
    const mat4 head = absoluteHeadFrame(e, root, P);
    r.box(head, {0.f, 0.19f, -0.01f}, {0.24f, 0.3f, 0.27f}, plate, soft);
    r.box(head, {0.f, 0.07f, 0.04f}, {0.2f, 0.09f, 0.2f}, skin);
    r.box(head, {0.f, 0.26f, 0.13f}, {0.23f, 0.04f, 0.04f}, plate);
    const vec3 eye{2.8f, 0.18f, 0.12f};                    // two crimson eyes, blazing through a tell
    const float eyeK = (hurt ? 0.5f : 1.f) * (tell != AbTell::NONE ? 1.f + 0.6f * tp : 1.f) * pulse;
    for (float s : {-1.f, 1.f}) {
        r.box(head * T({s * 0.055f, 0.21f, 0.142f}) * RZ(s * 0.18f), {0.f, 0.f, 0.f}, {0.07f, 0.022f, 0.012f}, dark, eye * eyeK);
        r.box(head * T({s * 0.055f, 0.21f, 0.136f}) * RZ(s * 0.18f), {0.f, 0.f, 0.f}, {0.11f, 0.05f, 0.006f}, dark, eye * 0.35f * eyeK);   // their glow
    }
    for (float s : {-1.f, 1.f}) r.box(head, {s * 0.085f, 0.15f, 0.12f}, {0.05f, 0.04f, 0.04f}, plate);
    r.box(head, {0.f, 0.35f, -0.02f}, {0.05f, 0.04f, 0.26f}, plate, soft);
    r.box(head, {0.f, 0.2f, -0.145f}, {0.2f, 0.22f, 0.03f}, skin);
    // ---- arms: deltoid, bicep, tricep, vein, elbow, forearm and its plate, three wraps and two
    // ribbons trailing light, palm, knuckles, four jointed fingers, a jointed thumb ----
    mat4 handR{1.f};
    for (float s : {-1.f, 1.f}) {
        const bool right = s < 0.f;
        const int limb = right ? 0 : 1;
        const float sh = right ? P.shR : P.shL, shz = right ? P.shRz : P.shLz, el = right ? P.elR : P.elL;
        const float fist = right ? P.fistR : P.fistL;
        mat4 shoulder = torso * T({s * 0.31f, 0.76f, 0.f}) * RX(sh) * RZ(s * (0.1f + shz));
        r.box(shoulder, {s * 0.02f, -0.02f, 0.f}, {0.18f, 0.16f, 0.18f}, plate, soft);
        r.box(shoulder, {0.f, -0.25f, 0.03f}, {0.12f, 0.36f, 0.11f}, skin, soft);
        r.box(shoulder, {0.f, -0.24f, -0.04f}, {0.11f, 0.32f, 0.08f}, plate);
        r.box(shoulder, {s * 0.06f, -0.25f, 0.f}, {0.02f, 0.3f, 0.02f}, dark, lit(limb, seam));
        mat4 elbow = shoulder * T({0.f, -0.48f, 0.f}) * RX(-el);
        r.box(elbow, {0.f, 0.f, -0.04f}, {0.1f, 0.08f, 0.06f}, plate);
        r.box(elbow, {0.f, -0.21f, 0.f}, {0.11f, 0.4f, 0.11f}, skin, lit(limb, soft));
        r.box(elbow, {s * 0.03f, -0.2f, 0.03f}, {0.07f, 0.3f, 0.06f}, plate, lit(limb, soft));
        for (int k = 0; k < 3; ++k)
            r.box(elbow, {0.f, -0.3f - 0.05f * k, 0.f}, {0.13f, 0.025f, 0.13f}, dark, lit(limb, light * 0.7f));
        float trail = 0.5f + glm::clamp(e.moveSpeed / 20.f, 0.f, 1.2f) + 0.12f * std::sin(time * 4.f + s);
        for (int k = 0; k < 2; ++k)
            r.box(elbow * T({(k ? 0.03f : -0.03f), -0.36f, -0.06f}) * RX(trail + 0.2f * k - (sh - el)),   // (they hang from the wrist, whichever way the forearm points)
                  {0.f, -0.16f, 0.f}, {0.025f, 0.32f, 0.008f}, dark, light * 0.5f);
        mat4 hand = elbow * T({0.f, -0.44f, 0.f});
        if (right) handR = hand;
        r.box(hand, {0.f, -0.05f, 0.f}, {0.1f, 0.1f, 0.05f}, skin, lit(limb, soft));
        r.box(hand, {0.f, -0.1f, 0.02f}, {0.1f, 0.02f, 0.03f}, plate, lit(limb, soft));
        // the gauntlet: a heavier shell, rune bands at the wrist and knuckles, pale violet-blue
        // (no tell's colour); the striking one flares to its tell
        const vec3 rune{0.6f, 0.7f, 1.7f};
        const vec3 rk = lit(limb, rune * (0.85f + 0.15f * std::sin(time * 3.f + s)));
        r.box(hand, {0.f, -0.06f, 0.f}, {0.125f, 0.12f, 0.075f}, dark * 1.4f, rune * 0.08f);
        r.box(hand, {0.f, 0.02f, 0.f}, {0.135f, 0.022f, 0.085f}, dark, rk);            // the wrist band
        r.box(hand, {0.f, -0.115f, 0.041f}, {0.115f, 0.018f, 0.012f}, dark, rk);       // across the knuckles
        for (int f = 0; f < 4; ++f)
            r.box(hand, {-0.035f + 0.023f * f, -0.09f, 0.043f}, {0.008f, 0.03f, 0.008f}, dark, rk * 0.8f);   // a rune per knuckle
        for (int f = 0; f < 4; ++f) {
            mat4 f1 = hand * T({-0.035f + 0.023f * f, -0.1f, 0.01f}) * RX(-fist * 0.8f);
            r.box(f1, {0.f, -0.03f, 0.f}, {0.02f, 0.06f, 0.022f}, skin, lit(limb, soft));
            mat4 f2 = f1 * T({0.f, -0.06f, 0.f}) * RX(-fist * 1.1f);
            r.box(f2, {0.f, -0.025f, 0.f}, {0.018f, 0.05f, 0.02f}, plate, lit(limb, soft * 1.5f));
        }
        mat4 t1 = hand * T({-s * 0.055f, -0.04f, 0.03f}) * RZ(-s * 0.5f) * RX(-fist * 0.6f);
        r.box(t1, {0.f, -0.025f, 0.f}, {0.022f, 0.05f, 0.022f}, skin, lit(limb, soft));
        r.box(t1 * T({0.f, -0.05f, 0.f}) * RX(-fist * 0.6f), {0.f, -0.02f, 0.f}, {0.02f, 0.04f, 0.02f}, plate, lit(limb, soft));
    }
    // ---- the broken halo: thirteen shards of a ring of sixteen, an inner ring, turning behind his head ----
    const mat4 halo = head * T({0.f, 0.24f, -0.34f}) * RX(0.15f + P.torsoPitch * 0.3f) * RZ(time * 0.35f);
    for (int k = 0; k < 16; ++k) {
        if (k == 3 || k == 4 || k == 11) continue;   // broken out
        float a = k * 0.3926991f, jit = 0.03f * std::sin(k * 7.1f);
        r.box(halo * RZ(a), {0.f, 0.5f + jit, 0.f}, {0.17f, 0.045f, 0.03f}, gold, gold * (hurt ? 0.5f : 1.6f) * pulse);
    }
    for (int k = 0; k < 8; ++k)
        r.box(halo * RZ(k * 0.7853982f + 0.39f), {0.f, 0.34f, 0.f}, {0.16f, 0.02f, 0.02f}, light, seam * 0.9f);
    // ---- the blade of light (phase 3): grip, guard, three lengths of blade, its edge ----
    if (e.abBlade) {
        const mat4 sw = handR * T({0.f, -0.07f, 0.03f}) * RX(1.5707963f);
        const vec3 bl = tell != AbTell::NONE && P.glow == 4 ? tc * tk : vec3{1.9f, 1.75f, 2.1f};
        r.box(sw, {0.f, 0.f, 0.f}, {0.05f, 0.18f, 0.05f}, dark, light * 0.5f);
        r.box(sw, {0.f, -0.1f, 0.f}, {0.32f, 0.04f, 0.07f}, plate, gold);
        for (int k = 0; k < 3; ++k)
            r.box(sw, {0.f, -0.35f - 0.45f * k, 0.f}, {0.09f - 0.02f * k, 0.46f, 0.025f}, plate, bl * (0.9f + 0.1f * k));
        r.box(sw, {0.f, -0.75f, 0.015f}, {0.02f, 1.3f, 0.01f}, light, bl * 1.4f);
    }
}
