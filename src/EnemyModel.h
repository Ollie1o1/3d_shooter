#pragma once
// =============================================================================
// EnemyModel.h — builds each enemy as a little rig of boxes.
//
// Every model is a hierarchy of joints (hips, shoulders, wing roots) with box
// "parts" hung off them, posed from the enemy's AI state every frame: legs
// swing with the walk cycle, wings flap, arms come up to aim or slam during a
// telegraph so the player can read the attack before it lands. The output is
// a flat list of BoxInstance (one unit cube each) that BoxRenderer draws in a
// single instanced call. No OpenGL here, so tests can count parts headlessly.
// =============================================================================
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include "Enemy.h"

// One unit cube ([-0.5, 0.5]^3) placed by `model`. Layout matches the
// per-instance vertex attributes in BoxRenderer (locations 4-9).
struct BoxInstance {
    glm::mat4 model{1.f};
    glm::vec3 color{1.f};
    float     _pad0 = 0.f;
    glm::vec3 emissive{0.f};
    float     _pad1 = 0.f;
};

namespace rig {
using glm::mat4; using glm::vec3;

inline mat4 T(vec3 p)            { return glm::translate(mat4(1.f), p); }
inline mat4 RX(float a)          { return glm::rotate(mat4(1.f), a, vec3{1,0,0}); }
inline mat4 RY(float a)          { return glm::rotate(mat4(1.f), a, vec3{0,1,0}); }
inline mat4 RZ(float a)          { return glm::rotate(mat4(1.f), a, vec3{0,0,1}); }
inline mat4 S(vec3 s)            { return glm::scale(mat4(1.f), s); }

inline void push(std::vector<BoxInstance>& out, const mat4& m, vec3 col, vec3 emi = vec3{0.f}) {
    BoxInstance b; b.model = m; b.color = col; b.emissive = emi;
    out.push_back(b);
}

struct Rig {
    std::vector<BoxInstance>& out;
    float flash = 0.f;   // 0..1 white hit flash
    float spawn = 1.f;   // 0..1 materialise progress
    void box(const mat4& parent, vec3 centre, vec3 size, vec3 col, vec3 emi = vec3{0.f}) {
        vec3 c = glm::mix(col, vec3{1.f}, flash * 0.85f);
        vec3 e = glm::mix(emi, vec3{0.9f}, flash * 0.6f);
        if (spawn < 1.f) e += vec3{0.5f, 0.85f, 1.f} * (1.f - spawn) * 2.5f;
        push(out, parent * T(centre) * S(size), c, e);
    }
};

struct HumanoidLook {
    float legLen, legW, hipW, pelvisH, torsoH, torsoW, torsoD, headS, armLen, armW;
    vec3  armor, under, glow;
};
// Body proportions of the humanoid types. buildEnemy adds the colours;
// headBox() reads the same numbers so the head you see is the head you hit.
inline HumanoidLook humanoidDims(EnemyType t) {
    switch (t) {
    case EnemyType::HUSK:       return {0.9f,  0.2f,  0.26f, 0.14f, 0.62f, 0.56f, 0.32f, 0.32f, 0.62f, 0.17f, {}, {}, {}};
    case EnemyType::SENTINEL:   return {1.25f, 0.15f, 0.22f, 0.14f, 0.74f, 0.44f, 0.3f,  0.3f,  0.82f, 0.14f, {}, {}, {}};
    case EnemyType::BRUTE:      return {1.0f,  0.42f, 0.55f, 0.2f,  1.15f, 1.35f, 0.85f, 0.44f, 1.25f, 0.42f, {}, {}, {}};
    case EnemyType::JUGGERNAUT: return {1.15f, 0.5f,  0.65f, 0.25f, 1.3f,  1.5f,  0.95f, 0.42f, 1.3f,  0.45f, {}, {}, {}};
    case EnemyType::WARDEN:     return {1.7f,  0.6f,  0.8f,  0.3f,  1.75f, 1.9f,  1.1f,  0.62f, 1.9f,  0.55f, {}, {}, {}};
    case EnemyType::SOVEREIGN:  return {1.35f, 0.34f, 0.46f, 0.22f, 1.25f, 1.05f, 0.58f, 0.42f, 1.3f,  0.3f,  {}, {}, {}};
    case EnemyType::SHIELDBEARER: return {0.95f, 0.24f, 0.3f, 0.16f, 0.7f, 0.62f, 0.36f, 0.32f, 0.66f, 0.19f, {}, {}, {}};
    default:                    return {};
    }
}
inline HumanoidLook humanoidLook(EnemyType t, vec3 armor, vec3 under, vec3 glow) {
    HumanoidLook L = humanoidDims(t);
    L.armor = armor; L.under = under; L.glow = glow;
    return L;
}

enum class ArmPose { SWING, AIM_RIGHT, AIM_BOTH, RAISED };
struct HumanoidFrames { mat4 body, torso, head, armL, armR; };

// Explicit joint angles instead of the walk cycle's arms (the SOVEREIGN's
// sword work). crouch lowers the whole body; the torso turns and leans.
struct PoseOverride {
    bool  on = false;
    float crouch = 0.f, torsoYaw = 0.f, torsoPitch = 0.f;
    float rxR = 0.f, rzR = 0.f, rxL = 0.f, rzL = 0.f;
    float grip = 0.f;   // the blade's angle in the right hand (0: continues the arm)
};

// Two legs, pelvis, torso with a glowing chest stripe, head with a visor, two
// arms with hands: 13 parts. Right side is local -X (the model faces +Z).
inline HumanoidFrames humanoid(Rig& r, const mat4& root, const HumanoidLook& L,
                               float phase, float stride, ArmPose pose, float amt,
                               bool visor = true, const PoseOverride* ov = nullptr) {
    HumanoidFrames f;
    float bob = std::fabs(std::sin(phase)) * 0.05f * stride;
    f.body = root * T({0.f, bob - (ov ? ov->crouch : 0.f), 0.f});
    float hipY = L.legLen;

    for (float s : {-1.f, 1.f}) {
        float swing = std::sin(phase + (s > 0.f ? 0.f : 3.14159f)) * 0.6f * stride;
        mat4 hip = f.body * T({s * L.hipW * 0.5f, hipY, 0.f}) * RX(swing);
        r.box(hip, {0.f, -L.legLen * 0.5f, 0.f}, {L.legW, L.legLen, L.legW * 1.1f}, L.under);
        r.box(hip, {0.f, -L.legLen + 0.05f, 0.07f}, {L.legW * 1.15f, 0.12f, L.legW * 1.7f}, L.armor);
    }
    r.box(f.body, {0.f, hipY + L.pelvisH * 0.5f, 0.f},
          {L.hipW + L.legW, L.pelvisH, L.torsoD * 0.8f}, L.under);

    f.torso = f.body * T({0.f, hipY + L.pelvisH, 0.f}) * RY(std::sin(phase) * 0.08f * stride);
    if (ov) f.torso = f.torso * RY(ov->torsoYaw) * RX(ov->torsoPitch);
    r.box(f.torso, {0.f, L.torsoH * 0.5f, 0.f}, {L.torsoW, L.torsoH, L.torsoD}, L.armor);
    r.box(f.torso, {0.f, L.torsoH * 0.62f, L.torsoD * 0.5f + 0.01f},
          {L.torsoW * 0.45f, L.torsoH * 0.12f, 0.05f}, L.glow * 0.3f, L.glow * 1.2f);

    f.head = f.torso * T({0.f, L.torsoH, 0.f});
    r.box(f.head, {0.f, L.headS * 0.5f + 0.02f, 0.f}, vec3{L.headS}, L.armor * 0.85f);
    if (visor)
        r.box(f.head, {0.f, L.headS * 0.58f, L.headS * 0.5f + 0.01f},
              {L.headS * 0.8f, L.headS * 0.2f, 0.05f}, L.glow * 0.3f, L.glow * 2.f);

    for (float s : {-1.f, 1.f}) {
        float legSwing = std::sin(phase + (s > 0.f ? 0.f : 3.14159f)) * 0.6f * stride;
        float rx = legSwing;                                 // arms swing against the legs
        bool  right = s < 0.f;
        if (pose == ArmPose::AIM_RIGHT && right) rx = -1.5708f * amt + rx * (1.f - amt);
        if (pose == ArmPose::AIM_BOTH)           rx = -1.5708f * amt + rx * (1.f - amt);
        if (pose == ArmPose::RAISED)             rx = -2.9f * amt + rx * (1.f - amt);
        float rz = 0.f;
        if (ov) { rx = right ? ov->rxR : ov->rxL; rz = right ? ov->rzR : ov->rzL; }
        mat4 sh = f.torso * T({s * (L.torsoW * 0.5f + L.armW * 0.5f), L.torsoH - L.armW * 0.5f, 0.f})
                * RX(rx) * RZ(s * 0.08f + rz);
        r.box(sh, {0.f, -L.armLen * 0.5f, 0.f}, {L.armW, L.armLen, L.armW}, L.armor * 0.9f);
        r.box(sh, {0.f, -L.armLen - L.armW * 0.3f, 0.f}, vec3{L.armW * 1.2f}, L.under);
        (right ? f.armR : f.armL) = sh;
    }
    return f;
}

inline float smooth01(float t) { t = glm::clamp(t, 0.f, 1.f); return t * t * (3.f - 2.f * t); }

// The SOVEREIGN's stance from his attack state: every wind-up is a clear,
// held pose (sword drawn back for a dash, wound to one side for a sweep,
// raised overhead for the cleave), and every stroke follows through.
inline PoseOverride sovereignPose(const Enemy& e) {
    PoseOverride o; o.on = true;
    float walk = std::sin(e.animPhase) * glm::clamp(e.moveSpeed / 6.f, 0.f, 1.f);
    o.rxR = -0.45f + walk * 0.12f; o.rxL = -walk * 0.5f; o.grip = -1.05f;
    float k = smooth01(e.telegraphProgress() * 1.6f);
    auto mixTo = [&](float& v, float to, float t) { v = v + (to - v) * t; };
    bool windup = e.telegraphTimer > 0.f;
    bool follow = e.swingTimer > 0.f && !(windup && e.telegraphProgress() > 0.35f);
    if (follow) {
        float t = 1.f - e.swingTimer / Enemy::SWING_TIME;
        float u = 1.f - (1.f - t) * (1.f - t);   // fast, then settling
        switch (e.lastSwing) {
        case AttackKind::SWEEP: {
            float from = e.lastSwingStep % 2 == 0 ? -1.1f : 1.1f;
            o.torsoYaw = from + (-from * 1.15f - from) * u;
            o.rxR = -1.45f; o.rxL = -0.5f; o.grip = -0.15f; o.crouch = 0.12f;
            break;
        }
        case AttackKind::CLEAVE:
            o.rxR = -2.9f + 2.35f * u; o.rxL = -2.7f + 2.1f * u;
            o.torsoPitch = -0.18f + 0.63f * u; o.grip = -0.1f + 0.3f * u; o.crouch = 0.25f * u;
            break;
        case AttackKind::DASH:
            o.torsoYaw = 1.0f * u; o.torsoPitch = 0.3f; o.rxR = -1.3f; o.rxL = -0.6f; o.grip = -0.2f; o.crouch = 0.2f;
            break;
        case AttackKind::THRUST:
            o.torsoYaw = 0.25f; o.torsoPitch = 0.25f; o.rxR = -1.55f; o.rxL = -0.3f; o.grip = 0.f; o.crouch = 0.25f;
            break;
        default: break;
        }
        return o;
    }
    if (e.whirlTimer > 0.f) {   // arms out, blade level: the spin is his yaw
        o.rxR = -1.5f; o.rxL = -1.4f; o.rzL = 0.4f; o.grip = -0.15f; o.crouch = 0.15f;
        return o;
    }
    if (e.thrustTimer > 0.f) {   // the blade straight out ahead of him
        o.torsoYaw = 0.25f; o.torsoPitch = 0.3f; o.rxR = -1.55f; o.rxL = -0.3f; o.grip = 0.f; o.crouch = 0.3f;
        return o;
    }
    if (e.leapTimer > 0.f) {
        o.rxR = -2.8f; o.rxL = -2.4f; o.grip = -0.1f; o.torsoPitch = -0.12f;
        return o;
    }
    if (e.dashTimer > 0.f) {
        o.crouch = 0.25f; o.torsoPitch = 0.45f; o.rxR = 0.9f; o.rzR = -0.25f; o.rxL = -1.2f; o.grip = -1.6f;
        return o;
    }
    if (!windup) return o;
    switch (e.attack) {
    case AttackKind::DASH:
        o.crouch = 0.3f * k; o.torsoPitch = 0.35f * k;
        mixTo(o.rxR, 0.75f, k); o.rzR = -0.2f * k; mixTo(o.rxL, -1.1f, k); mixTo(o.grip, -1.6f, k);
        break;
    case AttackKind::SWEEP: case AttackKind::CRESCENT: {
        float side = (e.attack == AttackKind::SWEEP && e.comboStep % 2 == 1) ? 1.f : -1.f;
        o.torsoYaw = 1.1f * side * k;
        mixTo(o.rxR, e.attack == AttackKind::CRESCENT ? -1.2f : -1.45f, k);
        mixTo(o.rxL, -0.4f, k); mixTo(o.grip, -0.15f, k); o.crouch = 0.1f * k;
        break;
    }
    case AttackKind::CLEAVE:
        mixTo(o.rxR, -2.9f, k); mixTo(o.rxL, -2.7f, k); o.torsoPitch = -0.18f * k; mixTo(o.grip, -0.1f, k);
        break;
    case AttackKind::LEAP:
        o.crouch = 0.35f * k; mixTo(o.rxR, -2.6f, k); mixTo(o.rxL, -2.2f, k); mixTo(o.grip, -0.2f, k);
        break;
    case AttackKind::BLINK:      // sinking into a crouch, blade trailing
        o.crouch = 0.45f * k; o.torsoPitch = 0.5f * k;
        mixTo(o.rxR, 0.75f, k); mixTo(o.rxL, 0.5f, k); mixTo(o.grip, -1.6f, k);
        break;
    case AttackKind::JUDGMENT:   // the sword held up to the sky
        o.torsoPitch = -0.3f * k; mixTo(o.rxR, -3.05f, k); mixTo(o.rxL, -2.95f, k); mixTo(o.grip, 0.f, k);
        break;
    case AttackKind::WHIRL:      // wound right round to one side
        o.torsoYaw = 1.45f * k; o.crouch = 0.2f * k;
        mixTo(o.rxR, -1.5f, k); mixTo(o.rxL, -0.6f, k); mixTo(o.grip, -0.15f, k);
        break;
    case AttackKind::THRUST:     // drawn back at the hip, point forward
        o.torsoYaw = -0.6f * k; o.crouch = 0.3f * k;
        mixTo(o.rxR, 0.3f, k); mixTo(o.rxL, -0.9f, k); mixTo(o.grip, -1.57f, k);
        break;
    case AttackKind::RUPTURE:    // overhead, to drive it into the floor
        mixTo(o.rxR, -2.9f, k); mixTo(o.rxL, -2.7f, k); o.torsoPitch = -0.25f * k; o.crouch = 0.1f * k; mixTo(o.grip, -0.1f, k);
        break;
    case AttackKind::PHANTOMS:   // pointing the blade at you: they come
        mixTo(o.rxR, -2.1f, k); mixTo(o.rxL, -0.3f, k); mixTo(o.grip, -0.9f, k); o.torsoYaw = -0.3f * k;
        break;
    default: break;
    }
    return o;
}

// Append the parts for one enemy. `time` drives idle animation (orbiting
// shards, blinking fuses).
inline void buildEnemy(const Enemy& e, float time, std::vector<BoxInstance>& out) {
    const EnemyStats& st = e.stats();
    Rig r{out};
    r.flash = glm::clamp(e.hitFlashTimer / 0.12f, 0.f, 1.f);
    r.spawn = 1.f - e.spawnTimer / Enemy::SPAWN_TIME;
    float grow = 0.05f + 0.95f * smooth01(r.spawn);

    float tp   = e.telegraphProgress();
    vec3  glow = st.glow * (1.f + 2.5f * tp);           // eyes flare during a wind-up
    float stride = glm::clamp(e.moveSpeed / std::max(0.1f, st.speed), 0.f, 1.2f);

    mat4 root = T(e.position) * RY(e.yaw) * S({1.f, grow, 1.f});

    switch (e.type) {
    case EnemyType::HUSK: {
        HumanoidLook L = humanoidLook(e.type, st.color, st.color * 0.45f, glow);
        float aim = e.attack == AttackKind::SHOT ? smooth01(tp * 3.f) : 0.35f;
        auto f = humanoid(r, root, L, e.animPhase, stride, ArmPose::AIM_RIGHT, aim);
        vec3 gun{0.16f, 0.15f, 0.17f};
        r.box(f.armR, {0.f, -0.78f, 0.06f}, {0.12f, 0.5f, 0.15f}, gun);
        r.box(f.armR, {0.f, -1.05f, 0.06f}, {0.09f, 0.08f, 0.09f}, gun, st.shotColor * (0.3f + 3.f * tp));
        r.box(f.torso, {0.f, 0.35f, -0.22f}, {0.4f, 0.4f, 0.14f}, st.color * 0.6f);   // backpack
        break;
    }
    case EnemyType::SENTINEL: {
        HumanoidLook L = humanoidLook(e.type, st.color, st.color * 0.4f, glow);
        float aim = (e.attack == AttackKind::BURST || e.burstLeft > 0) ? 1.f : 0.55f;
        auto f = humanoid(r, root, L, e.animPhase, stride, ArmPose::AIM_RIGHT, aim, false);
        vec3 metal{0.12f, 0.13f, 0.16f};
        r.box(f.armR, {0.f, -1.25f, 0.f}, {0.1f, 1.1f, 0.12f}, metal);                 // long rifle
        r.box(f.armR, {0.f, -1.0f, 0.1f}, {0.08f, 0.3f, 0.1f}, metal, glow * 0.6f);   // scope
        r.box(f.head, {0.f, 0.17f, 0.16f}, {0.16f, 0.12f, 0.04f}, glow * 0.2f, glow * 3.f); // cyclops eye
        r.box(f.head, {0.1f, 0.5f, -0.05f}, {0.03f, 0.45f, 0.03f}, metal);            // antenna
        r.box(f.head, {0.1f, 0.74f, -0.05f}, vec3{0.07f}, glow, glow * 2.f);
        for (float s : {-1.f, 1.f})                                                   // pauldrons
            r.box(f.torso, {s * 0.3f, 0.72f, 0.f}, {0.24f, 0.12f, 0.34f}, st.color * 1.25f);
        break;
    }
    case EnemyType::BRUTE: {
        HumanoidLook L = humanoidLook(e.type, st.color, st.color * 0.5f, glow);
        ArmPose pose = ArmPose::SWING; float amt = 0.f;
        if (e.attack == AttackKind::SLAM) { pose = ArmPose::RAISED;   amt = smooth01(tp * 1.6f); }
        if (e.attack == AttackKind::LOB)  { pose = ArmPose::AIM_BOTH; amt = smooth01(tp * 2.f); }
        auto f = humanoid(r, root, L, e.animPhase, std::max(stride, 0.4f), pose, amt);
        for (float s : {-1.f, 1.f}) {
            r.box(f.torso, {s * 0.78f, 1.08f, 0.f}, {0.62f, 0.42f, 0.8f}, st.color * 1.3f); // shoulder pads
            r.box(f.torso, {s * 0.78f, 1.31f, 0.f}, {0.5f, 0.06f, 0.6f}, glow * 0.2f, glow * 0.8f);
            r.box(s < 0.f ? f.armR : f.armL, {0.f, -1.45f, 0.f}, {0.62f, 0.55f, 0.62f}, st.color * 0.8f); // fists
            r.box(f.torso, {s * 0.3f, 0.95f, -0.5f}, {0.18f, 0.5f, 0.18f}, st.color * 0.5f, glow * 0.5f); // exhausts
        }
        float pulse = 0.6f + 0.4f * std::sin(time * 4.f);
        r.box(f.torso, {0.f, 0.55f, 0.44f}, {0.45f, 0.45f, 0.08f}, glow * 0.3f, glow * (1.2f * pulse + 2.f * tp));
        break;
    }
    case EnemyType::JUGGERNAUT: {
        // Broken (parried): slumps forward, core strobing — hit it now
        bool broken = e.staggered();
        mat4 base = broken ? root * T({0.f, -0.25f, 0.f}) * RX(0.32f) : root;
        vec3 core = broken ? (std::fmod(time * 10.f, 1.f) > 0.5f ? vec3{3.f, 2.6f, 1.2f} : vec3{1.f, 0.5f, 0.1f})
                           : glow;
        HumanoidLook L = humanoidLook(e.type, st.color, st.color * 0.45f, core);
        ArmPose pose = ArmPose::SWING; float amt = 0.f;
        if (e.attack == AttackKind::SHELL) { pose = ArmPose::AIM_RIGHT; amt = smooth01(tp * 2.f); }
        if (e.attack == AttackKind::SMASH) { pose = ArmPose::RAISED;    amt = smooth01(tp * 1.4f); }
        if (broken) { pose = ArmPose::SWING; amt = 0.f; }
        auto f = humanoid(r, base, L, e.animPhase, broken ? 0.f : std::max(stride, 0.3f), pose, amt);
        vec3 gun{0.14f, 0.14f, 0.16f}, gold{0.85f, 0.62f, 0.18f};
        // Cannon arm (right) with a muzzle that glows as it charges
        r.box(f.armR, {0.f, -1.45f, 0.f}, {0.62f, 1.0f, 0.62f}, gun);
        r.box(f.armR, {0.f, -1.98f, 0.f}, {0.48f, 0.12f, 0.48f}, gun * 0.6f,
              st.shotColor * (e.attack == AttackKind::SHELL ? 0.5f + 4.f * tp : 0.4f));
        for (float a : {0.f, 1.5708f})
            r.box(f.armR * RY(a), {0.f, -1.2f, 0.f}, {0.72f, 0.1f, 0.18f}, gold * 0.7f);
        // Hammer fist (left)
        r.box(f.armL, {0.f, -1.55f, 0.f}, {0.85f, 0.7f, 0.85f}, gold * 0.75f,
              e.attack == AttackKind::SMASH ? glow * (0.3f + 2.f * tp) : vec3{0.f});
        // Plating: shoulder slabs, chest plate, a helmet with a visor slit
        for (float s : {-1.f, 1.f}) {
            r.box(f.torso, {s * 0.95f, 1.2f, 0.f}, {0.7f, 0.5f, 1.05f}, st.color * 1.3f);
            r.box(f.torso, {s * 0.95f, 1.47f, 0.f}, {0.6f, 0.06f, 0.9f}, gold, gold * 0.2f);
        }
        r.box(f.torso, {0.f, 0.7f, 0.5f}, {1.2f, 0.9f, 0.1f}, st.color * 1.15f);
        float pulse = 0.6f + 0.4f * std::sin(time * 4.f);
        r.box(f.torso, {0.f, 0.72f, 0.56f}, {0.42f, 0.42f, 0.08f}, core * 0.3f, core * (1.3f * pulse + 2.f * tp));
        r.box(f.head, {0.f, 0.3f, 0.f}, {0.62f, 0.55f, 0.62f}, st.color * 1.2f);
        r.box(f.head, {0.f, 0.33f, 0.32f}, {0.46f, 0.07f, 0.02f}, core * 0.3f, core * 2.5f);
        r.box(f.torso, {0.f, 0.9f, -0.58f}, {0.9f, 0.9f, 0.3f}, gun);   // back reactor
        break;
    }
    case EnemyType::WARDEN: {
        vec3 g = e.enraged ? vec3{1.f, 0.1f, 0.25f} * (1.f + 2.5f * tp) : glow;
        HumanoidLook L = humanoidLook(e.type, st.color, st.color * 0.6f, g);
        ArmPose pose = ArmPose::SWING; float amt = 0.f;
        if (e.attack == AttackKind::SLAM)   { pose = ArmPose::RAISED;   amt = smooth01(tp * 1.5f); }
        if (e.attack == AttackKind::VOLLEY) { pose = ArmPose::AIM_BOTH; amt = smooth01(tp * 2.f); }
        if (e.attack == AttackKind::SUMMON) { pose = ArmPose::RAISED;   amt = 0.6f * smooth01(tp * 2.f); }
        auto f = humanoid(r, root, L, e.animPhase, std::max(stride, 0.35f), pose, amt);
        vec3 gold{0.78f, 0.56f, 0.16f};
        for (float s : {-1.f, 1.f}) {
            r.box(f.torso, {s * 1.08f, 1.62f, 0.f}, {0.9f, 0.5f, 1.2f}, st.color * 1.4f);
            r.box(f.torso, {s * 1.08f, 1.9f, 0.f}, {0.95f, 0.08f, 1.25f}, gold, gold * 0.3f);
            r.box(s < 0.f ? f.armR : f.armL, {0.f, -2.1f, 0.f}, {0.8f, 0.7f, 0.8f}, gold * 0.6f);
        }
        float sway = std::sin(time * 1.3f) * 0.08f;
        r.box(f.torso * RX(0.12f + sway), {0.f, -0.6f, -0.62f}, {1.7f, 2.9f, 0.08f}, vec3{0.2f, 0.03f, 0.1f}); // cape
        float pulse = 0.6f + 0.4f * std::sin(time * 5.f);
        r.box(f.torso, {0.f, 0.95f, 0.56f}, {0.6f, 0.6f, 0.1f}, g * 0.3f, g * (1.5f * pulse + 2.f * tp));
        for (int i = 0; i < 5; ++i) {                                                // crown
            float x = -0.24f + 0.12f * i;
            float hgt = (i == 2) ? 0.45f : (i % 2 ? 0.3f : 0.22f);
            r.box(f.head, {x, 0.64f + hgt * 0.5f, 0.f}, {0.07f, hgt, 0.07f}, gold, g * 0.8f);
        }
        for (int i = 0; i < 4; ++i) {                                                // orbiting shards
            float a = time * 1.6f + i * 1.5708f;
            mat4 m = T(e.position + vec3{std::cos(a) * 2.6f, 4.0f + std::sin(time * 2.f + i) * 0.3f, std::sin(a) * 2.6f})
                   * RY(a * 2.f) * RX(0.785f) * S(vec3{0.35f * grow});
            push(out, m, g * 0.3f, g * 1.8f);
        }
        break;
    }
    case EnemyType::RIPPER: {
        float crouch = e.attack == AttackKind::LUNGE ? smooth01(tp * 2.f) : 0.f;
        bool  spring = e.attack == AttackKind::LUNGE && e.telegraphTimer < 0.1f;
        mat4 body = root * T({0.f, 0.62f - crouch * 0.22f, 0.f}) * RX(spring ? -0.25f : crouch * 0.12f);
        vec3 armor = st.color, under = st.color * 0.45f;
        r.box(body, {0.f, 0.f, 0.f},   {0.62f, 0.42f, 1.15f}, armor);
        r.box(body, {0.f, 0.2f, -0.25f}, {0.55f, 0.24f, 0.5f}, armor * 0.85f);
        for (int i = 0; i < 3; ++i)
            r.box(body, {0.f, 0.3f, -0.38f + i * 0.28f}, {0.08f, 0.26f, 0.12f}, under, glow * 0.4f);
        mat4 head = body * T({0.f, 0.1f, 0.6f}) * RX(-0.12f + crouch * 0.3f);
        r.box(head, {0.f, 0.f, 0.22f},    {0.42f, 0.34f, 0.46f}, armor * 0.9f);
        r.box(head, {0.f, -0.17f, 0.28f}, {0.36f, 0.1f, 0.4f},  under);
        for (float s : {-1.f, 1.f})
            r.box(head, {s * 0.12f, 0.06f, 0.46f}, {0.08f, 0.06f, 0.03f}, glow, glow * 2.5f);
        float ph = e.animPhase * 1.4f;
        const vec3 corners[4] = {{0.27f,-0.12f,0.42f},{-0.27f,-0.12f,0.42f},{0.27f,-0.12f,-0.42f},{-0.27f,-0.12f,-0.42f}};
        const float offs[4] = {0.f, 3.14159f, 3.14159f, 0.f};
        for (int i = 0; i < 4; ++i) {
            mat4 leg = body * T(corners[i]) * RX(std::sin(ph + offs[i]) * 0.7f * stride - crouch * 0.4f);
            r.box(leg, {0.f, -0.25f, 0.f}, {0.13f, 0.55f, 0.13f}, under);
        }
        for (float s : {-1.f, 1.f}) {                                                // blades
            mat4 arm = body * T({s * 0.36f, 0.05f, 0.42f}) * RX(-0.35f - crouch * 0.9f) * RY(-s * 0.2f);
            r.box(arm, {0.f, 0.f, 0.36f}, {0.06f, 0.1f, 0.78f}, glow * 0.4f, glow * (0.8f + 1.5f * crouch));
        }
        break;
    }
    case EnemyType::RAPTOR: {
        bool diving = e.diveTimer > 0.f;
        float bob = diving ? 0.f : std::sin(time * 3.f + e.animPhase * 0.1f) * 0.15f;
        mat4 base = T(e.position + vec3{0.f, bob, 0.f}) * RY(e.yaw) * RX(e.pitch) * S(vec3{grow});
        vec3 armor = st.color, under = st.color * 0.5f;
        r.box(base, {0.f, 0.45f, 0.f},    {0.5f, 0.42f, 1.0f}, armor);
        r.box(base, {0.f, 0.38f, 0.32f},  {0.44f, 0.36f, 0.42f}, armor * 1.15f);
        mat4 head = base * T({0.f, 0.62f, 0.55f});
        r.box(head, {0.f, 0.05f, 0.12f},  {0.34f, 0.3f, 0.36f}, armor * 0.9f);
        r.box(head, {0.f, -0.02f, 0.42f}, {0.12f, 0.1f, 0.34f}, {0.9f, 0.6f, 0.15f}, vec3{0.25f, 0.12f, 0.f});
        for (float s : {-1.f, 1.f})
            r.box(head, {s * 0.13f, 0.08f, 0.22f}, {0.05f, 0.07f, 0.07f}, glow, glow * 2.5f);
        r.box(head, {0.f, 0.25f, 0.f}, {0.06f, 0.18f, 0.3f}, under, glow * 0.6f);   // crest
        mat4 tail = base * T({0.f, 0.5f, -0.5f}) * RX(0.2f + std::sin(e.animPhase * 0.5f) * 0.1f);
        r.box(tail, {0.f, 0.f, -0.3f},  {0.45f, 0.06f, 0.6f}, armor * 0.8f);
        r.box(tail, {0.f, 0.f, -0.62f}, {0.52f, 0.05f, 0.08f}, glow * 0.3f, glow * 1.2f);
        float flap = diving ? 0.1f : std::sin(e.animPhase) * 0.65f + 0.1f;
        float windFlap = e.attack == AttackKind::DIVE ? -0.5f * smooth01(tp * 2.f) : 0.f;   // rears up before diving
        for (float s : {-1.f, 1.f}) {
            mat4 w1 = base * T({s * 0.25f, 0.55f, 0.05f}) * (diving ? RY(-s * 0.9f) : mat4(1.f))
                    * RZ(s * (flap + windFlap));
            r.box(w1, {s * 0.5f, 0.f, 0.f},   {1.0f, 0.07f, 0.62f}, armor);
            r.box(w1, {s * 0.5f, 0.f, 0.31f}, {1.0f, 0.08f, 0.06f}, glow * 0.3f, glow * 1.2f);
            mat4 w2 = w1 * T({s * 1.0f, 0.f, 0.f}) * RZ(s * flap * 0.6f);
            r.box(w2, {s * 0.45f, 0.f, -0.08f}, {0.9f, 0.05f, 0.48f}, armor * 0.8f);
            r.box(w2, {s * 0.86f, 0.f, -0.1f},  {0.14f, 0.06f, 0.5f}, glow * 0.4f, glow * 1.5f);
            r.box(base, {s * 0.12f, 0.12f, 0.1f}, {0.07f, 0.25f, 0.07f}, under, diving ? glow : vec3{0.f});
        }
        break;
    }
    case EnemyType::MITE: {
        float hop = std::fabs(std::sin(e.animPhase)) * 0.04f;
        mat4 body = root * T({0.f, 0.32f + hop, 0.f});
        vec3 armor = st.color, under = st.color * 0.5f;
        r.box(body, {0.f, 0.02f, -0.12f}, {0.5f, 0.32f, 0.5f}, armor);
        r.box(body, {0.f, 0.f, 0.22f},    {0.32f, 0.24f, 0.26f}, under);
        for (float s : {-1.f, 1.f})
            r.box(body, {s * 0.08f, 0.04f, 0.36f}, {0.06f, 0.05f, 0.02f}, glow, glow * 2.f);
        vec3 core = glow * 1.5f;
        if (e.attack == AttackKind::FUSE)
            core = std::fmod(e.age * 14.f, 1.f) > 0.5f ? vec3{3.f, 0.4f, 0.2f} : vec3{0.6f, 0.05f, 0.f};
        r.box(body, {0.f, 0.21f, -0.1f}, {0.26f, 0.12f, 0.26f}, glow * 0.3f, core);
        for (float s : {-1.f, 1.f})
            for (int i = 0; i < 3; ++i) {
                float lift = std::sin(e.animPhase + i * 2.1f + (s > 0.f ? 0.f : 1.f)) * 0.25f;
                mat4 leg = body * T({s * 0.22f, 0.f, 0.18f - i * 0.2f})
                         * RY(s * (0.5f - i * 0.5f)) * RZ(s * (-0.6f + lift));
                r.box(leg, {s * 0.25f, 0.f, 0.f}, {0.5f, 0.06f, 0.06f}, under);
            }
        break;
    }
    case EnemyType::SHIELDBEARER: {
        HumanoidLook L = humanoidLook(e.type, st.color, st.color * 0.5f, glow);
        float aim = e.attack == AttackKind::SHOT ? smooth01(tp * 2.5f) : 0.3f;
        auto f = humanoid(r, root, L, e.animPhase, stride, ArmPose::AIM_RIGHT, aim);
        vec3 metal{0.16f, 0.17f, 0.2f}, bronze{0.62f, 0.46f, 0.2f};
        r.box(f.armR, {0.f, -0.74f, 0.05f}, {0.14f, 0.36f, 0.18f}, metal);                         // stubby scattergun
        r.box(f.armR, {0.f, -0.94f, 0.05f}, {0.16f, 0.07f, 0.16f}, metal, st.shotColor * (0.3f + 3.f * tp));
        // The tower shield: braced in front, drawn back for a bash, knocked aside when broken
        bool broken = e.staggered();
        float bash = e.attack == AttackKind::BASH ? smooth01(tp * 1.5f) : 0.f;
        mat4 sh = f.torso * T({0.12f, -0.15f, L.torsoD * 0.5f + 0.32f - 0.22f * bash}) * RX(-0.12f * bash);
        if (broken) sh = f.torso * T({0.55f, -0.5f, 0.3f}) * RY(1.1f) * RZ(0.5f);
        vec3 rim = glow * (0.5f + 2.f * tp);
        r.box(sh, {0.f, 0.f, 0.f},      {1.05f, 1.6f, 0.12f}, st.color * 1.25f);
        r.box(sh, {0.f, 0.f, 0.07f},    {0.86f, 1.38f, 0.03f}, st.color * 0.9f);
        r.box(sh, {0.f, 0.83f, 0.03f},  {1.08f, 0.07f, 0.16f}, bronze, rim);
        r.box(sh, {0.f, -0.83f, 0.03f}, {1.08f, 0.07f, 0.16f}, bronze, rim);
        r.box(sh, {0.f, 0.1f, 0.09f},   {0.1f, 0.6f, 0.03f}, bronze * 0.5f, glow * 1.2f);   // sigil
        r.box(sh, {0.f, 0.1f, 0.09f},   {0.36f, 0.1f, 0.03f}, bronze * 0.5f, glow * 1.2f);
        r.box(f.head, {0.f, L.headS + 0.06f, -0.02f}, {0.06f, 0.14f, 0.3f}, bronze);        // helmet crest
        break;
    }
    case EnemyType::SOVEREIGN: {
        bool broken = e.staggered();
        vec3 g = e.enraged ? vec3{1.f, 0.12f, 0.08f} * (1.3f + 2.f * tp) : glow;
        HumanoidLook L = humanoidLook(e.type, st.color, st.color * 0.45f, g);
        PoseOverride ov = sovereignPose(e);
        mat4 base = broken ? root * T({0.f, -0.2f, 0.f}) * RX(0.28f) : root;
        if (broken) { ov.rxR = 0.2f; ov.rxL = 0.1f; ov.grip = -1.3f; ov.torsoYaw = 0.f; ov.torsoPitch = 0.2f; }
        auto f = humanoid(r, base, L, e.animPhase, stride, ArmPose::SWING, 0.f, true, &ov);
        vec3 gold{0.86f, 0.64f, 0.22f}, steel{0.5f, 0.52f, 0.58f};
        vec3 edge = e.enraged ? vec3{2.2f, 0.5f, 0.35f} : vec3{1.7f, 1.35f, 0.75f};
        // The greatsword, in the right hand: pommel, grip, guard, blade with a lit edge
        mat4 sw = f.armR * T({0.f, -L.armLen - L.armW * 0.3f, 0.f}) * RX(ov.grip);
        const float BL = 2.4f;
        r.box(sw, {0.f, 0.3f, 0.f},  vec3{0.13f}, gold);
        r.box(sw, {0.f, 0.05f, 0.f}, {0.09f, 0.42f, 0.09f}, st.color * 0.6f);
        r.box(sw, {0.f, -0.2f, 0.f}, {0.62f, 0.08f, 0.15f}, gold, gold * 0.15f);
        r.box(sw, {0.f, -0.25f - BL * 0.5f, 0.f}, {0.18f, BL, 0.05f}, steel);
        r.box(sw, {0.f, -0.25f - BL * 0.48f, 0.f}, {0.06f, BL * 0.94f, 0.07f}, edge * 0.3f, edge * (1.f + 1.5f * tp));
        r.box(sw, {0.f, -0.28f - BL, 0.f}, {0.1f, 0.12f, 0.05f}, steel, edge * 0.6f);
        // Plate: pauldrons with gold trim, chest plate and its sigil, belt, tassets
        for (float s : {-1.f, 1.f}) {
            r.box(f.torso, {s * (L.torsoW * 0.5f + 0.12f), L.torsoH - 0.04f, 0.f}, {0.52f, 0.32f, 0.68f}, st.color * 1.5f);
            r.box(f.torso, {s * (L.torsoW * 0.5f + 0.12f), L.torsoH + 0.13f, 0.f}, {0.54f, 0.05f, 0.7f}, gold, gold * 0.2f);
            r.box(f.body, {s * 0.22f, L.legLen - 0.2f, L.torsoD * 0.42f}, {0.34f, 0.5f, 0.06f}, st.color * 1.3f);
        }
        r.box(f.torso, {0.f, L.torsoH * 0.6f, L.torsoD * 0.5f + 0.02f}, {L.torsoW * 0.78f, L.torsoH * 0.5f, 0.06f}, st.color * 1.35f);
        float pulse = 0.6f + 0.4f * std::sin(time * 4.f);
        r.box(f.torso, {0.f, L.torsoH * 0.62f, L.torsoD * 0.5f + 0.06f}, {0.16f, 0.32f, 0.04f}, g * 0.3f, g * (1.2f * pulse + 2.f * tp));
        r.box(f.torso, {0.f, 0.06f, 0.f}, {L.torsoW + 0.05f, 0.12f, L.torsoD + 0.05f}, gold * 0.6f);
        r.box(f.armL, {0.f, -L.armLen - L.armW * 0.3f, 0.f}, vec3{L.armW * 1.45f}, gold * 0.7f);   // gauntlet
        // Helm: three horns swept back
        r.box(f.head * T({0.f, L.headS + 0.1f, -0.06f}) * RX(-0.5f), {0.f, 0.14f, 0.f}, {0.08f, 0.36f, 0.08f}, gold);
        for (float s : {-1.f, 1.f})
            r.box(f.head * T({s * 0.19f, L.headS - 0.02f, -0.02f}) * RZ(-s * 0.45f) * RX(-0.4f),
                  {0.f, 0.2f, 0.f}, {0.07f, 0.44f, 0.07f}, gold * 0.85f);
        // A halo of eight shards turning behind the head
        mat4 halo = f.head * T({0.f, L.headS * 0.6f, -0.42f}) * RZ(time * 0.8f);
        for (int i = 0; i < 8; ++i) {
            float a = i * 0.785f;
            r.box(halo, {std::cos(a) * 0.62f, std::sin(a) * 0.62f, 0.f}, {0.09f, 0.09f, 0.04f}, gold, g * 0.6f + gold * 0.8f);
        }
        // Cape: lifts behind him as he moves, streams out in a dash
        float lift = glm::clamp(e.moveSpeed / 20.f, 0.f, 1.f);
        float sway = std::sin(time * 1.7f + e.animPhase * 0.2f) * 0.05f;
        mat4 cape = f.torso * T({0.f, L.torsoH - 0.05f, -L.torsoD * 0.5f - 0.05f}) * RX(0.12f + sway + lift * 0.9f);
        vec3 capeCol{0.32f, 0.04f, 0.05f};
        r.box(cape, {0.f, -1.05f, 0.f}, {L.torsoW * 0.92f, 2.1f, 0.05f}, capeCol, e.enraged ? capeCol * 1.5f : vec3{0.f});
        r.box(cape, {0.f, -2.08f, 0.f}, {L.torsoW * 0.94f, 0.06f, 0.06f}, gold * 0.5f);
        break;
    }
    case EnemyType::CONDUIT: {
        // A pylon: a plinth, four fins, a pulsing core and two rings turning
        // round it, a crystal on top. The core beats faster as it's hurt.
        vec3 dark = st.color, trim = st.color * 1.8f;
        float hurt = 1.f - e.health / e.maxHealth;
        float beat = 0.6f + 0.4f * std::sin(time * (3.f + 6.f * hurt) + e.animPhase);
        vec3 core = glow * (1.2f + 1.6f * beat);
        r.box(root, {0.f, 0.15f, 0.f}, {1.5f, 0.3f, 1.5f}, dark);
        r.box(root, {0.f, 0.34f, 0.f}, {1.6f, 0.08f, 1.6f}, trim, glow * 0.4f);
        for (int k = 0; k < 4; ++k) {
            mat4 fin = root * RY(k * 1.5708f + 0.785f);
            r.box(fin, {0.f, 1.1f, 0.62f}, {0.14f, 1.6f, 0.32f}, dark);
            r.box(fin, {0.f, 1.1f, 0.79f}, {0.04f, 1.2f, 0.04f}, glow * 0.4f, core * 0.6f);
        }
        r.box(root, {0.f, 1.7f, 0.f}, {0.5f, 2.4f, 0.5f}, glow * 0.3f, core);
        for (int i = 0; i < 2; ++i) {
            float y = 1.2f + i * 1.1f + 0.15f * std::sin(time * 1.3f + i * 1.7f);
            mat4 ring = root * T({0.f, y, 0.f}) * RY(time * (i ? -1.1f : 0.8f));
            for (int k = 0; k < 4; ++k) {
                mat4 seg = ring * RY(k * 1.5708f);
                r.box(seg, {0.f, 0.f, 0.55f}, {0.8f, 0.07f, 0.07f}, trim, glow * 0.8f * beat);
            }
        }
        mat4 top = root * T({0.f, 3.1f + 0.08f * std::sin(time * 2.f), 0.f}) * RY(time * 0.6f) * RX(0.785f) * RZ(0.785f);
        r.box(top, {0.f, 0.f, 0.f}, {0.36f, 0.36f, 0.36f}, glow * 0.5f, core * 1.2f);
        break;
    }
    case EnemyType::CONDUCTOR: {
        // A hovering emitter: a glowing core in a split shell, three fins
        // turning round it, an antenna, and a prong underneath the tethers
        // run from. Brighter while it's holding links.
        float bob = std::sin(time * 2.2f + e.animPhase) * 0.08f;
        mat4 body = root * T({0.f, 0.85f + bob, 0.f}) * S(vec3{1.3f});   // big enough to pick out at range
        vec3 shell = st.color, dark = st.color * 0.5f;
        vec3 core = glow * (e.linkCount > 0 ? 2.4f + 0.6f * std::sin(time * 9.f) : 1.2f);
        r.box(body, {0.f, 0.f, 0.f}, {0.34f, 0.34f, 0.34f}, glow * 0.4f, core);
        for (float s : {-1.f, 1.f}) {
            r.box(body, {s * 0.27f, 0.f, 0.f}, {0.14f, 0.5f, 0.5f}, shell);
            r.box(body, {0.f, s * 0.27f, 0.f}, {0.36f, 0.12f, 0.36f}, dark);
        }
        mat4 spin = body * RY(time * 2.f);
        for (int k = 0; k < 3; ++k) {
            mat4 fin = spin * RY(k * 2.094f) * T({0.f, 0.f, 0.55f});
            r.box(fin, {0.f, 0.f, 0.f}, {0.08f, 0.42f, 0.22f}, shell, glow * 0.5f);
        }
        r.box(body, {0.f, 0.45f, 0.f}, {0.04f, 0.3f, 0.04f}, dark);
        r.box(body, {0.f, 0.62f, 0.f}, {0.09f, 0.09f, 0.09f}, glow * 0.5f, core * 0.8f);
        r.box(body, {0.f, -0.38f, 0.f}, {0.1f, 0.28f, 0.1f}, dark, glow * 0.3f);
        r.box(body, {0.f, -0.56f, 0.f}, {0.16f, 0.06f, 0.16f}, glow * 0.4f, core * 0.7f);
        break;
    }
    default: break;
    }
}
// The head hitbox, world space: the box around the drawn head (helmet, crown,
// beak) a little oversized so a shot that grazes it still counts. It follows
// the same frames the model is built from: walk bob, a broken JUGGERNAUT's
// slump, a RIPPER's crouch, a RAPTOR's dive. False for a type without a head
// (MITE). Uses the physics position, like the rest of the hitscan.
inline bool headBox(const Enemy& e, AABB& out) {
    const EnemyStats& st = e.stats();
    float grow   = 0.05f + 0.95f * smooth01(1.f - e.spawnTimer / Enemy::SPAWN_TIME);
    float tp     = e.telegraphProgress();
    float stride = glm::clamp(e.moveSpeed / std::max(0.1f, st.speed), 0.f, 1.2f);
    mat4  root   = T(e.position) * RY(e.yaw) * S({1.f, grow, 1.f});
    mat4  frame;          // the head's frame
    vec3  centre, half;   // the head box within it

    switch (e.type) {
    case EnemyType::HUSK: case EnemyType::SENTINEL: case EnemyType::BRUTE:
    case EnemyType::JUGGERNAUT: case EnemyType::WARDEN: case EnemyType::SOVEREIGN: case EnemyType::SHIELDBEARER: {
        HumanoidLook L = humanoidDims(e.type);
        mat4 base = root;
        if (e.type == EnemyType::JUGGERNAUT && e.staggered()) base = root * T({0.f, -0.25f, 0.f}) * RX(0.32f);
        PoseOverride ov;
        if (e.type == EnemyType::SOVEREIGN) {
            if (e.staggered()) { base = root * T({0.f, -0.2f, 0.f}) * RX(0.28f); ov.torsoPitch = 0.2f; }
            else ov = sovereignPose(e);
        }
        float bob = std::fabs(std::sin(e.animPhase)) * 0.05f * stride;
        frame = base * T({0.f, bob - ov.crouch + L.legLen + L.pelvisH, 0.f}) * RY(ov.torsoYaw) * RX(ov.torsoPitch)
              * T({0.f, L.torsoH, 0.f});
        float w = L.headS, top = L.headS + 0.02f;
        if (e.type == EnemyType::JUGGERNAUT) { w = 0.62f; top = 0.575f; }   // helmet
        if (e.type == EnemyType::WARDEN)     top = 0.95f;                     // crown
        if (e.type == EnemyType::SOVEREIGN)  top = 0.62f;                     // helm and horn roots
        centre = {0.f, top * 0.5f, 0.f};
        half   = {w * 0.5f, top * 0.5f, w * 0.5f};
        break;
    }
    case EnemyType::RIPPER: {
        float crouch = e.attack == AttackKind::LUNGE ? smooth01(tp * 2.f) : 0.f;
        bool  spring = e.attack == AttackKind::LUNGE && e.telegraphTimer < 0.1f;
        mat4 body = root * T({0.f, 0.62f - crouch * 0.22f, 0.f}) * RX(spring ? -0.25f : crouch * 0.12f);
        frame  = body * T({0.f, 0.1f, 0.6f}) * RX(-0.12f + crouch * 0.3f);
        centre = {0.f, -0.03f, 0.22f};
        half   = {0.21f, 0.2f, 0.23f};
        break;
    }
    case EnemyType::RAPTOR: {
        mat4 base = T(e.position) * RY(e.yaw) * RX(e.pitch) * S(vec3{grow});
        frame  = base * T({0.f, 0.62f, 0.55f});
        centre = {0.f, 0.05f, 0.26f};
        half   = {0.17f, 0.17f, 0.33f};
        break;
    }
    default: return false;
    }

    half = half * 1.15f + vec3{0.04f};
    vec3 c = vec3(frame * glm::vec4(centre, 1.f));
    glm::mat3 m(frame);
    vec3 ext = glm::abs(m[0]) * half.x + glm::abs(m[1]) * half.y + glm::abs(m[2]) * half.z;
    out = {c - ext, c + ext};
    return true;
}

} // namespace rig

using rig::buildEnemy;
using rig::headBox;
