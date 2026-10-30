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
#include "enemies/Enemy.h"
#include "render/EnemyFile.h"
#include "render/AnimClip.h"

// One unit cube ([-0.5, 0.5]^3) placed by `model`. Layout matches the
// per-instance vertex attributes in BoxRenderer (locations 4-9).
struct BoxInstance {
    glm::mat4 model{1.f};
    glm::vec3 color{1.f};
    float     mesh = 0.f;    // what's drawn: 0 the unit cube, else a mesh from MeshLib.h (centred, at its own size)
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

// Every box a code rig draws, with the frame it hangs off (tools/enemy_export.cpp
// turns a rig into a file from this). Off unless set.
struct TracedBox { mat4 parent, local; vec3 col, emi; };
inline std::vector<TracedBox>*& traceSink() { static std::vector<TracedBox>* t = nullptr; return t; }
// Which joint each part drawn from a file hangs off: (its index in the output, the joint), for a
// death that breaks the body at its joints (Effects.h). Off unless set. A LEVIATHAN segment is 1000 + i.
struct PartJoint { int index, joint; };
inline std::vector<PartJoint>*& jointSink() { static std::vector<PartJoint>* t = nullptr; return t; }

struct Rig {
    std::vector<BoxInstance>& out;
    float flash = 0.f;   // 0..1 white hit flash
    float spawn = 1.f;   // 0..1 materialise progress
    float finish = 0.f;  // 0..1 a punch would finish it: the whole body pulses hot
    float dim = 1.f;     // stunned: its glow gutters
    void box(const mat4& parent, vec3 centre, vec3 size, vec3 col, vec3 emi = vec3{0.f}) {
        if (traceSink()) traceSink()->push_back({parent, T(centre) * S(size), col, emi});
        part(parent * T(centre) * S(size), col, emi);
    }
    void part(const mat4& m, vec3 col, vec3 emi = vec3{0.f}, int mesh = 0) {
        vec3 c = glm::mix(col, vec3{1.f}, flash * 0.85f);
        vec3 e = glm::mix(emi * dim, vec3{0.9f}, flash * 0.6f);
        if (finish > 0.f) { c = glm::mix(c, vec3{1.f, 0.6f, 0.25f}, 0.25f * finish); e += vec3{1.1f, 0.42f, 0.08f} * finish; }
        if (spawn < 1.f) e += vec3{0.5f, 0.85f, 1.f} * (1.f - spawn) * 2.5f;
        push(out, m, c, e);
        out.back().mesh = (float)mesh;
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
    case EnemyType::ANCHOR:     return {0.95f, 0.4f,  0.5f,  0.2f,  1.05f, 1.25f, 0.8f,  0.4f,  1.15f, 0.4f,  {}, {}, {}};
    case EnemyType::PENITENT:   return {4.5f,  1.35f, 1.9f,  0.75f, 4.25f, 4.5f,  2.5f,  1.5f,  5.5f,  1.1f,  {}, {}, {}};
    case EnemyType::REVENANT:   return {1.1f,  0.16f, 0.21f, 0.12f, 0.7f,  0.46f, 0.26f, 0.3f,  0.8f,  0.13f, {}, {}, {}};   // gaunt, long-limbed
    case EnemyType::ABSOLUTE:   return {1.15f, 0.19f, 0.3f,  0.15f, 0.85f, 0.62f, 0.32f, 0.3f,  1.0f,  0.16f, {}, {}, {}};   // a lean athlete's build, broad shoulders
    case EnemyType::PACEMAKER:  return {1.2f,  0.2f,  0.3f,  0.15f, 0.8f,  0.6f,  0.32f, 0.3f,  0.95f, 0.17f, {}, {}, {}};   // a runner's build, long in the leg
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

// A RAPTOR's bob in the air (none in a dive): from its own state, not the clock, so its head
// hitbox (headBox) bobs with the head you see
inline float raptorBob(const Enemy& e) { return e.diveTimer > 0.f ? 0.f : std::sin(e.age * 3.f + e.animPhase * 0.1f) * 0.15f; }
inline float smooth01c(float t) { t = glm::clamp(t, 0.f, 1.f); return t * t * (3.f - 2.f * t); }

// How a humanoid's joints turn this frame (the same for one built in code
// and one from a file): [0] the left side (+X), [1] the right (-X)
struct HumanoidMotion {
    float lift;               // the body's bob, less any crouch
    float hip[2];             // leg swing
    mat4  torso;              // its sway, turn and lean
    float armRx[2], armRz[2];
};
inline HumanoidMotion humanoidMotion(float phase, float stride, ArmPose pose, float amt, const PoseOverride* ov) {
    HumanoidMotion m;
    m.lift = std::fabs(std::sin(phase)) * 0.05f * stride - (ov ? ov->crouch : 0.f);
    m.torso = RY(std::sin(phase) * 0.08f * stride);
    if (ov) m.torso = m.torso * RY(ov->torsoYaw) * RX(ov->torsoPitch);
    for (int i = 0; i < 2; ++i) {
        float s = i == 0 ? 1.f : -1.f;
        float legSwing = std::sin(phase + (s > 0.f ? 0.f : 3.14159f)) * 0.6f * stride;
        m.hip[i] = legSwing;
        float rx = legSwing;                                 // arms swing against the legs
        bool  right = s < 0.f;
        if (pose == ArmPose::AIM_RIGHT && right) rx = -1.5708f * amt + rx * (1.f - amt);
        if (pose == ArmPose::AIM_BOTH)           rx = -1.5708f * amt + rx * (1.f - amt);
        if (pose == ArmPose::RAISED)             rx = -2.9f * amt + rx * (1.f - amt);
        float rz = 0.f;
        if (ov) { rx = right ? ov->rxR : ov->rxL; rz = right ? ov->rzR : ov->rzL; }
        m.armRx[i] = rx; m.armRz[i] = s * 0.08f + rz;
    }
    return m;
}

// Two legs, pelvis, torso with a glowing chest stripe, head with a visor, two
// arms with hands: 13 parts. Right side is local -X (the model faces +Z).
inline HumanoidFrames humanoid(Rig& r, const mat4& root, const HumanoidLook& L,
                               float phase, float stride, ArmPose pose, float amt,
                               bool visor = true, const PoseOverride* ov = nullptr) {
    HumanoidFrames f;
    HumanoidMotion mo = humanoidMotion(phase, stride, pose, amt, ov);
    f.body = root * T({0.f, mo.lift, 0.f});
    float hipY = L.legLen;

    for (float s : {-1.f, 1.f}) {
        mat4 hip = f.body * T({s * L.hipW * 0.5f, hipY, 0.f}) * RX(mo.hip[s > 0.f ? 0 : 1]);
        r.box(hip, {0.f, -L.legLen * 0.5f, 0.f}, {L.legW, L.legLen, L.legW * 1.1f}, L.under);
        r.box(hip, {0.f, -L.legLen + 0.05f, 0.07f}, {L.legW * 1.15f, 0.12f, L.legW * 1.7f}, L.armor);
    }
    r.box(f.body, {0.f, hipY + L.pelvisH * 0.5f, 0.f},
          {L.hipW + L.legW, L.pelvisH, L.torsoD * 0.8f}, L.under);

    f.torso = f.body * T({0.f, hipY + L.pelvisH, 0.f}) * mo.torso;
    r.box(f.torso, {0.f, L.torsoH * 0.5f, 0.f}, {L.torsoW, L.torsoH, L.torsoD}, L.armor);
    r.box(f.torso, {0.f, L.torsoH * 0.62f, L.torsoD * 0.5f + 0.01f},
          {L.torsoW * 0.45f, L.torsoH * 0.12f, 0.05f}, L.glow * 0.3f, L.glow * 1.2f);

    f.head = f.torso * T({0.f, L.torsoH, 0.f});
    r.box(f.head, {0.f, L.headS * 0.5f + 0.02f, 0.f}, vec3{L.headS}, L.armor * 0.85f);
    if (visor)
        r.box(f.head, {0.f, L.headS * 0.58f, L.headS * 0.5f + 0.01f},
              {L.headS * 0.8f, L.headS * 0.2f, 0.05f}, L.glow * 0.3f, L.glow * 2.f);

    for (float s : {-1.f, 1.f}) {
        bool right = s < 0.f;
        int  i = right ? 1 : 0;
        mat4 sh = f.torso * T({s * (L.torsoW * 0.5f + L.armW * 0.5f), L.torsoH - L.armW * 0.5f, 0.f})
                * RX(mo.armRx[i]) * RZ(mo.armRz[i]);
        r.box(sh, {0.f, -L.armLen * 0.5f, 0.f}, {L.armW, L.armLen, L.armW}, L.armor * 0.9f);
        r.box(sh, {0.f, -L.armLen - L.armW * 0.3f, 0.f}, vec3{L.armW * 1.2f}, L.under);
        (right ? f.armR : f.armL) = sh;
    }
    return f;
}

// Where a SHIELDBEARER's shield sits at rest, from its torso (the code rig's numbers)
inline vec3 shieldRest() { HumanoidLook L = humanoidDims(EnemyType::SHIELDBEARER); return {0.12f, -0.15f, L.torsoD * 0.5f + 0.32f}; }

// A rig from its file (EnemyFile.h): the code sets each joint's motion (in the
// joint's own frame, where the file put it) and each glow channel; the parts
// ride their joints. `root` already carries the body's lift.
struct FilePose {
    mat4 motion[enemyfile::E_COUNT];
    vec3 chan[enemyfile::C_COUNT];
    bool flag[enemyfile::F_COUNT] = {};   // a part with a flag shows only while it holds (or, "!flag", doesn't)
    float wave = 0.f;                     // a part with a phase pulses along it: x (0.75 + 0.25 sin(wave + phase))
    float tint = 1.f;                     // every fixed colour x this (MORNINGSTAR's light going out in TOTALITY)
    FilePose() { for (auto& m : motion) m = mat4(1.f); for (auto& c : chan) c = vec3(0.f); chan[enemyfile::C_CONST] = vec3(1.f); }
};
inline void drawFile(Rig& r, const mat4& root, const gunkit::GunModel& m, const FilePose& P, vec3 glow, mat4* Wout = nullptr, int skipJoint = -1) {
    using namespace enemyfile;
    mat4 local[E_COUNT]; int parent[E_COUNT];
    restLocals(m, local, parent);
    mat4 W[E_COUNT];
    for (auto& w : W) w = root;
    for (const auto& J : m.joints) W[J.id] = W[J.parent] * local[J.id] * P.motion[J.id];   // (parents come first)
    for (const auto& p : m.parts) {
        if (p.flag >= 0 && p.flag < F_COUNT && P.flag[p.flag] != p.want) continue;
        if (p.joint == skipJoint) continue;
        vec3 col = p.glowScale >= 0.f ? glow * p.glowScale : p.color * P.tint;
        vec3 emi = p.chan >= 0 && p.chan < C_COUNT ? P.chan[p.chan] * p.k : vec3{0.f};
        if (p.hasPhase) emi *= 0.75f + 0.25f * std::sin(P.wave + p.phase);
        r.part(W[p.joint] * p.rel, col, emi, p.mesh);
        if (jointSink()) jointSink()->push_back({(int)r.out.size() - 1, p.joint});
    }
    if (Wout) for (int j = 0; j < E_COUNT; ++j) Wout[j] = W[j];
}
// A humanoid's motion as a file pose (the same numbers humanoid() turns its boxes by)
inline FilePose humanoidPose(const HumanoidMotion& mo, vec3 glow) {
    using namespace enemyfile;
    FilePose P;
    P.motion[E_HIPL] = RX(mo.hip[0]); P.motion[E_HIPR] = RX(mo.hip[1]);
    P.motion[E_TORSO] = mo.torso;
    P.motion[E_ARML] = RX(mo.armRx[0]) * RZ(mo.armRz[0]);
    P.motion[E_ARMR] = RX(mo.armRx[1]) * RZ(mo.armRz[1]);
    P.chan[C_GLOW] = glow;
    return P;
}

inline PoseOverride sovereignPose(const Enemy& e);   // (below)
inline PoseOverride penitentPose(const Enemy& e);    // (below)
inline bool absoluteFilePose(const Enemy& e, float time, const mat4& root, mat4& fileRoot, FilePose& P, vec3& glowOut);   // (AbsoluteRig.h)
inline bool pacemakerFilePose(const Enemy& e, float time, const mat4& root, mat4& fileRoot, FilePose& P, vec3& glowOut);  // (below)
// The SOVEREIGN's cape (and its hem, which lags it), from how fast he's going
inline void sovereignCape(const Enemy& e, float time, float& cape, float& hem) {
    float lift = glm::clamp(e.moveSpeed / 20.f, 0.f, 1.f);
    float sway = std::sin(time * 1.7f + e.animPhase * 0.2f) * 0.05f;
    cape = 0.12f + sway + lift * 0.9f;
    hem = sway * 1.4f + lift * 0.35f + 0.05f * std::sin(time * 2.3f + 0.8f);
}

// THE WARDEN, from its state (its code rig and its file both): its glow (magenta fed, angrier
// overloaded, white-hot with its core open, burning in meltdown), its arms for each attack, the
// reel when its conduits are cut (broken), its cape, and the four shards orbiting over it
inline vec3 wardenGlow(const Enemy& e, vec3 glow, float tp) {
    return e.ventTimer > 0.f ? vec3{1.6f, 1.5f, 1.3f}
         : e.wardenPhase == 3 ? vec3{1.6f, 0.45f, 0.15f} * (1.f + 1.5f * tp)
         : e.wardenPhase == 2 ? vec3{1.f, 0.1f, 0.25f} * (1.f + 2.5f * tp) : glow;
}
inline void wardenArms(const Enemy& e, float tp, ArmPose& pose, float& amt) {
    pose = ArmPose::SWING; amt = 0.f;
    if (e.attack == AttackKind::LANCE || e.lanceTimer > 0.f) { pose = ArmPose::RAISED; amt = e.lanceTimer > 0.f ? 1.f : smooth01c(tp * 1.5f); }
    if (e.attack == AttackKind::WLUNGE || e.dashTimer > 0.f) { pose = ArmPose::AIM_BOTH; amt = 1.f; }
    if (e.attack == AttackKind::WVENT || e.attack == AttackKind::DETONATE) { pose = ArmPose::RAISED; amt = 0.5f * smooth01c(tp * 2.f); }
    if (e.attack == AttackKind::SLAM)   { pose = ArmPose::RAISED;   amt = smooth01c(tp * 1.5f); }
    if (e.attack == AttackKind::VOLLEY) { pose = ArmPose::AIM_BOTH; amt = smooth01c(tp * 2.f); }
    if (e.attack == AttackKind::SUMMON) { pose = ArmPose::RAISED;   amt = 0.6f * smooth01c(tp * 2.f); }
}
inline mat4 wardenBase(const Enemy& e, const mat4& root) { return e.staggered() ? root * T({0.f, -0.35f, 0.f}) * RX(0.22f) : root; }
inline void wardenCape(const Enemy& e, float time, float& cape, float& hem) {
    float sway = std::sin(time * 1.3f + e.animPhase * 0.15f) * 0.06f;
    cape = 0.1f + sway + 0.12f * glm::clamp(e.moveSpeed / 3.f, 0.f, 1.f);
    hem = 0.05f + sway * 1.3f + 0.04f * std::sin(time * 2.1f + 0.6f);
}
inline void wardenShards(const Enemy& e, float time, float grow, vec3 g, std::vector<BoxInstance>& out) {
    for (int i = 0; i < 4; ++i) {
        float a = time * 1.6f + i * 1.5708f;
        mat4 m = T(e.position + vec3{std::cos(a) * 2.6f, 4.0f + std::sin(time * 2.f + i) * 0.3f, std::sin(a) * 2.6f})
               * RY(a * 2.f) * RX(0.785f) * S(vec3{0.35f * grow});
        push(out, m, g * 0.3f, g * 1.8f);
    }
}

// A clip made in Blender (AnimClip.h) over the code's pose: at `progress` (0..1) through it, eased in over
// its first 15 %. Each joint it keys takes the clip's motion; "root" moves the whole body. False: no clip
inline bool playClip(const char* fileName, const char* clipName, float progress, FilePose& P, mat4& fileRoot) {
    const anim::ClipFile* F = anim::file(fileName, enemyfile::names());
    const anim::Clip* c = F ? F->clip(clipName) : nullptr;
    if (!c) return false;
    const float t = glm::clamp(progress, 0.f, 1.f) * c->length;
    float w = glm::clamp(progress / 0.15f, 0.f, 1.f);
    w = w * w * (3.f - 2.f * w);
    for (const anim::Track& tr : c->tracks) {
        if (tr.joint == anim::ROOT_TRACK) fileRoot = fileRoot * anim::blend(mat4(1.f), anim::motionAt(tr, t), w);
        else if (tr.joint > 0 && tr.joint < enemyfile::E_COUNT) P.motion[tr.joint] = anim::blend(P.motion[tr.joint], anim::motionAt(tr, t), w);
    }
    return true;
}

// Every enemy with a file: where its file is drawn from (fileRoot) and how its joints
// turn and its glow channels shine this frame, from its state: the same numbers its
// code rig uses. false for a type with no file rig. (Also what tools/enemy_export.cpp reads.)
inline bool filePoseOf(const Enemy& e, float time, const mat4& root, mat4& fileRoot, FilePose& P, vec3& glowOut) {
    using namespace enemyfile;
    const EnemyStats& st = e.stats();
    float tp = e.telegraphProgress();
    vec3 glow = st.glow * (1.f + 2.5f * tp);
    float stride = glm::clamp(e.moveSpeed / std::max(0.1f, st.speed), 0.f, 1.2f);
    glowOut = glow;
    auto humanoidOf = [&](float strideH, ArmPose pose, float amt, const mat4& base) {
        HumanoidMotion mo = humanoidMotion(e.animPhase, strideH, pose, amt, nullptr);
        P = humanoidPose(mo, glow);
        fileRoot = base * T({0.f, mo.lift, 0.f});
    };
    if(e.type==EnemyType::BALLAST) {
        fileRoot=root;
        vec3 tell=e.attack==AttackKind::BALL_ANCHOR?vec3{2.f}:e.attack==AttackKind::BALL_GRADE?abGoldTell({2,1.2f,.1f}):vec3{2,.2f,.06f};
        P.chan[C_EYE]=glow;P.chan[C_EDGE]=e.telegraphTimer>0?tell:glow*.25f;
        P.chan[C_CORE]=e.ballOpen>0||e.staggered()?vec3{.3f,1.7f,2.f}:vec3{.5f,.2f,.06f};
        P.motion[E_HALO]=RZ(e.ballReverse?-e.ballRotor:e.ballRotor);
        P.motion[E_HEAD]=RY(std::sin(e.age*.4f)*.08f);
        P.motion[E_BODY]=RX(e.ballDrive>0?-.08f:std::sin(e.age*1.8f)*.012f);
        P.motion[E_SHIELD]=RX(e.ballDrive>0?.1f:e.attack==AttackKind::BALL_GRADE?-tp*.35f:0.f);
        for(int k=0;k<4;++k)P.motion[E_LEG_FL+k]=RX(std::sin(e.animPhase+k*3.14159f)*stride*.18f);
        P.motion[E_ARMR]=RZ(e.attack==AttackKind::BALL_ANCHOR?-tp*.6f:0.f);
        return true;
    }
    if(e.type==EnemyType::SORTER) {
        fileRoot=root;
        vec3 tell=e.attack==AttackKind::SORT_GRAB?vec3{2}:e.attack==AttackKind::SORT_STAMP?vec3{2,.1f,.04f}:abGoldTell({2,1.3f,.2f});
        P.chan[C_EYE]=e.sortOpen>0||e.staggered()?vec3{.2f,1.5f,2}:glow;
        P.chan[C_CORE]=e.sortOpen>0||e.staggered()?vec3{.2f,1.5f,2}:vec3{.35f,.1f,.02f};
        P.chan[C_EDGE]=e.telegraphTimer>0?tell:glow*.4f;
        P.motion[E_HEAD]=RY(std::sin(time*.5f)*.12f);
        float flex=e.telegraphTimer>0?tp*.5f:.08f*std::sin(time*2);
        P.motion[E_ARML]=RZ(flex);P.motion[E_ARMR]=RZ(-flex);
        P.motion[E_ELBOW_L]=RX(-flex);P.motion[E_ELBOW_R]=RX(-flex);
        return true;
    }
    if (e.type == EnemyType::PACEMAKER) return pacemakerFilePose(e, time, root, fileRoot, P, glowOut);
    switch (rigBase(e.type)) {   // (a mini-boss moves as its rig's enemy)
    case EnemyType::HUSK:
        humanoidOf(stride, ArmPose::AIM_RIGHT, e.attack == AttackKind::SHOT ? smooth01c(tp * 3.f) : 0.35f, root);
        P.chan[C_SHOT] = st.shotColor * (0.3f + 3.f * tp);
        return true;
    case EnemyType::SENTINEL:
        humanoidOf(stride, ArmPose::AIM_RIGHT, (e.attack == AttackKind::BURST || e.burstLeft > 0) ? 1.f : 0.55f, root);
        return true;
    case EnemyType::BRUTE: {
        ArmPose pose = ArmPose::SWING; float amt = 0.f;
        if (e.attack == AttackKind::SLAM) { pose = ArmPose::RAISED;   amt = smooth01c(tp * 1.6f); }
        if (e.attack == AttackKind::LOB)  { pose = ArmPose::AIM_BOTH; amt = smooth01c(tp * 2.f); }
        humanoidOf(std::max(stride, 0.4f), pose, amt, root);
        float throb = 0.6f + 0.4f * std::sin(time * 4.f);
        P.chan[C_PULSE] = glow * (1.2f * throb + 2.f * tp);
        return true;
    }
    case EnemyType::JUGGERNAUT: {   // broken (parried): slumps forward, its core strobing
        bool broken = e.staggered();
        vec3 core = broken ? (std::fmod(time * 10.f, 1.f) > 0.5f ? vec3{3.f, 2.6f, 1.2f} : vec3{1.f, 0.5f, 0.1f}) : glow;
        glowOut = core;   // (the glow its visor and core shine with)
        ArmPose pose = ArmPose::SWING; float amt = 0.f;
        if (e.attack == AttackKind::SHELL) { pose = ArmPose::AIM_RIGHT; amt = smooth01c(tp * 2.f); }
        if (e.attack == AttackKind::SMASH) { pose = ArmPose::RAISED;    amt = smooth01c(tp * 1.4f); }
        if (broken) { pose = ArmPose::SWING; amt = 0.f; }
        humanoidOf(broken ? 0.f : std::max(stride, 0.3f), pose, amt, broken ? root * T({0.f, -0.25f, 0.f}) * RX(0.32f) : root);
        P.chan[C_GLOW] = core;
        float throb = 0.6f + 0.4f * std::sin(time * 4.f);
        P.chan[C_PULSE] = core * (1.3f * throb + 2.f * tp);
        P.chan[C_SHOT] = st.shotColor * (e.attack == AttackKind::SHELL ? 0.5f + 4.f * tp : 0.4f);
        P.chan[C_SMASH] = e.attack == AttackKind::SMASH ? glow * (0.3f + 2.f * tp) : vec3{0.f};
        return true;
    }
    case EnemyType::SHIELDBEARER: {   // the tower shield: braced in front, drawn back to bash, knocked aside when broken
        humanoidOf(stride, ArmPose::AIM_RIGHT, e.attack == AttackKind::SHOT ? smooth01c(tp * 2.5f) : 0.3f, root);
        P.chan[C_SHOT] = st.shotColor * (0.3f + 3.f * tp);
        P.chan[C_RIM] = glow * (0.5f + 2.f * tp);   // (the flaring glow, flaring harder)
        float bash = e.attack == AttackKind::BASH ? smooth01c(tp * 1.5f) : 0.f;
        if (e.staggered())   // knocked aside: placed from the torso
            P.motion[E_SHIELD] = T(-shieldRest()) * T({0.55f, -0.5f, 0.3f}) * RY(1.1f) * RZ(0.5f);
        else {
            P.motion[E_SHIELD] = T({0.f, 0.f, -0.22f * bash}) * RX(-0.12f * bash);
        }
        return true;
    }
    case EnemyType::RIPPER: {
        fileRoot = root;
        float crouch = e.attack == AttackKind::LUNGE ? smooth01c(tp * 2.f) : 0.f;
        bool  spring = e.attack == AttackKind::LUNGE && e.telegraphTimer < 0.1f;
        P.motion[E_BODY] = T({0.f, -crouch * 0.22f, 0.f}) * RX(spring ? -0.25f : crouch * 0.12f);
        P.motion[E_HEAD] = RX(-0.12f + crouch * 0.3f);
        float ph = e.animPhase * 1.4f;
        const int legs[4] = {E_LEG_FL, E_LEG_FR, E_LEG_BL, E_LEG_BR};
        const float offs[4] = {0.f, 3.14159f, 3.14159f, 0.f};
        for (int i = 0; i < 4; ++i) P.motion[legs[i]] = RX(std::sin(ph + offs[i]) * 0.7f * stride - crouch * 0.4f);
        P.motion[E_BLADE_L] = RX(-0.35f - crouch * 0.9f) * RY(-0.2f);
        P.motion[E_BLADE_R] = RX(-0.35f - crouch * 0.9f) * RY(0.2f);
        P.chan[C_GLOW] = glow;
        P.chan[C_BLADE] = glow * (0.8f + 1.5f * crouch);
        return true;
    }
    case EnemyType::RAPTOR: {   // its own frame: it pitches into a dive, and bobs in the air
        bool diving = e.diveTimer > 0.f;
        float grow = 0.05f + 0.95f * smooth01c(1.f - e.spawnTimer / Enemy::SPAWN_TIME);
        float bob = raptorBob(e);   // (from its own state, so headBox can follow it)
        fileRoot = T(e.position + vec3{0.f, bob, 0.f}) * RY(e.yaw) * RX(e.pitch) * S(vec3{e.scale}) * S(vec3{grow});
        P.motion[E_TAIL] = RX(0.2f + std::sin(e.animPhase * 0.5f) * 0.1f);
        float flap = diving ? 0.1f : std::sin(e.animPhase) * 0.65f + 0.1f;
        float windFlap = e.attack == AttackKind::DIVE ? -0.5f * smooth01c(tp * 2.f) : 0.f;   // rears up before diving
        for (float s : {-1.f, 1.f}) {
            P.motion[s > 0.f ? E_WING_L : E_WING_R] = (diving ? RY(-s * 0.9f) : mat4(1.f)) * RZ(s * (flap + windFlap));
            P.motion[s > 0.f ? E_TIP_L : E_TIP_R] = RZ(s * flap * 0.6f);
        }
        P.chan[C_GLOW] = glow;
        P.chan[C_DIVE] = diving ? glow : vec3{0.f};
        return true;
    }
    case EnemyType::SOVEREIGN: {   // the greatsword in his right hand, his cape, the halo turning behind his helm
        bool broken = e.staggered();
        vec3 g = e.enraged ? vec3{1.f, 0.12f, 0.08f} * (1.3f + 2.f * tp) : glow;
        glowOut = g;
        PoseOverride ov = sovereignPose(e);
        mat4 base = broken ? root * T({0.f, -0.2f, 0.f}) * RX(0.28f) : root;
        if (broken) { ov.rxR = 0.2f; ov.rxL = 0.1f; ov.grip = -1.3f; ov.torsoYaw = 0.f; ov.torsoPitch = 0.2f; }
        HumanoidMotion mo = humanoidMotion(e.animPhase, stride, ArmPose::SWING, 0.f, &ov);
        P = humanoidPose(mo, g);
        fileRoot = base * T({0.f, mo.lift, 0.f});
        P.motion[E_SWORD] = RX(ov.grip);
        float cape, hem;
        sovereignCape(e, time, cape, hem);
        P.motion[E_CAPE] = RX(cape);
        P.motion[E_CAPE_TIP] = RX(hem);
        P.motion[E_HALO] = RZ(time * 0.8f);
        const vec3 gold{0.86f, 0.64f, 0.22f};
        float pulse = 0.6f + 0.4f * std::sin(time * 4.f);
        P.chan[C_PULSE] = g * (1.2f * pulse + 2.f * tp);
        P.chan[C_EDGE] = (e.enraged ? vec3{2.2f, 0.5f, 0.35f} : vec3{1.7f, 1.35f, 0.75f}) * (1.f + 1.5f * tp);
        P.chan[C_HALO] = g * 0.6f + gold * 0.8f;
        P.chan[C_CAPE] = e.enraged ? vec3{0.32f, 0.04f, 0.05f} * (2.5f + 1.5f * pulse) : vec3{0.f};
        P.flag[F_ENRAGED] = e.enraged;
        P.flag[F_BROKEN] = broken;
        P.wave = time * 3.f;
        // JUDGMENT's wind-up keyed in Blender (assets/enemies/anim/sovereign.glb, "judgment"): played over
        // the wind-up, eased in from whatever he was doing; no file, the code's pose above
        if (!broken && e.telegraphTimer > 0.f && e.attack == AttackKind::JUDGMENT)
            playClip("sovereign", "judgment", tp, P, fileRoot);
        return true;
    }
    case EnemyType::ABSOLUTE: return absoluteFilePose(e, time, root, fileRoot, P, glowOut);
    case EnemyType::WARDEN: {   // its core behind plates that draw back when it vents; reels (broken) when its conduits are cut
        vec3 g = wardenGlow(e, glow, tp);
        bool broken = e.staggered();
        ArmPose pose; float amt;
        wardenArms(e, tp, pose, amt);
        if (broken) { pose = ArmPose::SWING; amt = 0.f; }
        humanoidOf(broken ? 0.f : std::max(stride, 0.35f), pose, amt, wardenBase(e, root));
        glowOut = g;
        P.chan[C_GLOW] = g;
        float pulse = 0.6f + 0.4f * std::sin(time * 5.f);
        P.chan[C_CORE] = g * (1.5f * pulse + 2.f * tp + (e.coreOpen() ? 2.f : 0.f));
        P.chan[C_PULSE] = g * (0.55f + 0.25f * std::sin(time * 3.f) + 1.5f * tp);   // the conduit lines over its plate
        float cape, hem;
        wardenCape(e, time, cape, hem);
        P.motion[E_CAPE] = RX(cape);
        P.motion[E_CAPE_TIP] = RX(hem);
        P.flag[F_OPEN] = e.coreOpen();
        P.flag[F_ENRAGED] = e.wardenPhase == 3;
        P.flag[F_BROKEN] = broken;
        P.wave = time * 4.f;
        return true;
    }
    case EnemyType::REVENANT: {   // its soul in its cracked chest, flaring for a soulbolt; claws up to rake
        bool rake = e.attack == AttackKind::RAKE, bolt = e.attack == AttackKind::SOULBOLT;
        humanoidOf(std::max(stride, 0.25f), rake ? ArmPose::RAISED : ArmPose::SWING, rake ? smooth01c(tp * 3.f) : 0.f, root);
        const vec3 soul{0.75f, 0.9f, 1.3f};
        P.chan[C_CORE] = soul * (1.2f + (bolt ? 4.f * tp : 0.f) + 0.4f * std::sin(time * 6.f));
        P.chan[C_BLADE] = soul * (rake ? 1.5f : 0.3f);
        P.chan[C_EYE] = soul * 2.f;
        return true;
    }
    case EnemyType::ANCHOR: {   // hunched under its ring core; both fists up to fire
        bool firing = e.attack == AttackKind::VOLLEY;
        humanoidOf(std::max(stride, 0.3f), firing ? ArmPose::AIM_BOTH : ArmPose::SWING, firing ? smooth01c(tp * 1.5f) : 0.f, root);
        P.motion[E_TORSO] = P.motion[E_TORSO] * RX(0.18f);   // (bent at the waist: its feet stay flat)
        float pulse = 0.6f + 0.4f * std::sin(time * 2.5f);
        P.chan[C_PULSE] = glow * (0.6f + pulse);
        P.chan[C_CORE] = glow * (2.f * pulse + 2.f * tp);
        return true;
    }
    case EnemyType::WEAVER: {   // six legs, each lifting as it swings forward; it rears up to string a wire
        fileRoot = root;
        float rear = e.attack == AttackKind::STRING ? smooth01c(tp * 2.f) : 0.f;
        P.motion[E_BODY] = T({0.f, 0.25f * rear, 0.f}) * RX(-0.35f * rear);
        float ph = e.animPhase * 1.6f;
        const int hips[2][3] = {{E_LEG_L1, E_LEG_L2, E_LEG_L3}, {E_LEG_R1, E_LEG_R2, E_LEG_R3}};
        const int knees[2][3] = {{E_SHIN_L1, E_SHIN_L2, E_SHIN_L3}, {E_SHIN_R1, E_SHIN_R2, E_SHIN_R3}};
        for (int side = 0; side < 2; ++side)
            for (int k = 0; k < 3; ++k) {
                float s = side == 0 ? 1.f : -1.f, at = ph + (side * 3 + k) * 2.1f;
                float swing = std::sin(at) * 0.45f * stride, lift = std::max(0.f, std::cos(at)) * 0.3f * stride;
                P.motion[hips[side][k]] = RY(swing) * RZ(s * lift);
                P.motion[knees[side][k]] = RZ(-s * lift * 0.8f);
            }
        P.chan[C_GLOW] = glow;
        P.chan[C_CORE] = glow * (0.4f + 3.f * rear);   // the spinner
        return true;
    }
    case EnemyType::SERAPH: {   // its own frame (it bobs from its own state: headBox follows); wings flare to charge and sweep
        bool charging = e.attack == AttackKind::BEAM && e.telegraphTimer > 0.f, sweeping = e.beamTimer > 0.f;
        float grow = 0.05f + 0.95f * smooth01c(1.f - e.spawnTimer / Enemy::SPAWN_TIME);
        float bob = std::sin(e.animPhase * 0.5f) * 0.2f;
        fileRoot = T(e.position + vec3{0.f, bob, 0.f}) * RY(e.yaw) * S(vec3{e.scale}) * S(vec3{grow});
        float flare = charging ? smooth01c(tp) : sweeping ? 1.f : 0.f;
        for (float s : {-1.f, 1.f})
            for (int pair = 0; pair < 2; ++pair)   // (the rest angle is in the file: only the beat and the flare here)
                P.motion[pair == 0 ? (s > 0.f ? E_WING_L : E_WING_R) : (s > 0.f ? E_TIP_L : E_TIP_R)]
                    = RZ(s * (std::sin(e.animPhase + pair) * 0.18f + flare * 0.5f));
        P.chan[C_GLOW] = glow;
        P.chan[C_CORE] = glow * (1.5f + (sweeping ? 3.f : 3.f * tp));   // the chest lens
        P.chan[C_EDGE] = glow * (1.f + 2.f * flare);                     // the wings' edges
        return true;
    }
    case EnemyType::CONDUIT: {   // a pylon: its core beats faster as it's hurt; two rings turn round it, a crystal on top
        fileRoot = root;
        float hurt = 1.f - e.health / e.maxHealth;
        float beat = 0.6f + 0.4f * std::sin(time * (3.f + 6.f * hurt) + e.animPhase);
        for (int i = 0; i < 2; ++i)
            P.motion[i ? E_CROWN : E_HALO] = T({0.f, 0.15f * std::sin(time * 1.3f + i * 1.7f), 0.f}) * RY(time * (i ? -1.1f : 0.8f));
        P.motion[E_HEAD] = T({0.f, 0.08f * std::sin(time * 2.f), 0.f}) * RY(time * 0.6f);
        P.chan[C_GLOW] = glow;
        P.chan[C_CORE] = glow * (1.2f + 1.6f * beat);
        P.chan[C_PULSE] = glow * beat;
        if (e.type == EnemyType::PLUMB) {   // its rings race as it winds up a volley; its core blazes white while open
            for (int i = 0; i < 2; ++i)
                P.motion[i ? E_CROWN : E_HALO] = RY(time * (i ? -1.1f : 0.8f) * (1.f + 4.f * tp));
            if (e.miniOpen > 0.f) { P.chan[C_CORE] = vec3{2.6f, 2.2f, 1.6f} * (0.8f + 0.2f * std::sin(time * 30.f)); glowOut = P.chan[C_CORE]; }
        }
        return true;
    }
    case EnemyType::CONDUCTOR: {   // hovering, its fins spinning; its core brightens while it holds tethers
        fileRoot = root;
        P.motion[E_BODY] = T({0.f, std::sin(time * 2.2f + e.animPhase) * 0.08f, 0.f});
        P.motion[E_HALO] = RY(time * 2.f);
        P.chan[C_GLOW] = glow;
        P.chan[C_CORE] = glow * (e.linkCount > 0 ? 2.4f + 0.6f * std::sin(time * 9.f) : 1.2f);
        return true;
    }
    case EnemyType::LEVIATHAN: {   // its head: the skull (x1.5), the jaw opening, the eye's corona turning (the body: leviathanSegments)
        if (!e.levInit || (e.levHidden() && e.levStage != Enemy::LevStage::RISE)) return false;   // not risen / under the floor
        const vec3 seam{1.3f, 0.3f, 0.12f}, corona = st.glow;
        float rage = e.levPhase == 3 ? 1.6f : 1.f;
        bool crash = e.attack == AttackKind::CRASH, torrent = e.attack == AttackKind::TORRENT,
             swallow = e.attack == AttackKind::SWALLOW || e.levInhale > 0.f;
        vec3 col = crash ? glm::mix(corona, vec3{1.8f, 0.2f, 0.1f}, glm::clamp(tp * 2.f, 0.f, 1.f)) : corona;
        float open = e.levEyeOpen() ? 2.2f : 1.f;
        fileRoot = T(e.position) * RY(e.yaw) * RX(-e.levPitch) * S(vec3{1.5f});
        P.motion[E_JAW] = RX(0.9f * e.levJaw);
        P.motion[E_HALO] = RZ(time * 0.6f);
        glowOut = col;
        P.chan[C_GLOW] = col * (1.4f + 1.5f * tp) * open * rage;                      // the corona
        P.chan[C_EYE] = e.levEyeOpen() ? col * 0.25f : vec3{0.f};                    // the black disc, lit when it opens
        P.chan[C_SEAM] = seam * (0.9f + 0.6f * tp) * rage;                           // seams of light along its skull
        P.chan[C_SHOT] = torrent ? vec3{0.3f, 1.1f, 0.9f} * (0.5f + 3.f * tp)
                       : swallow ? vec3{1.6f, 1.5f, 1.4f} * (0.6f + 2.f * (e.levInhale > 0.f ? 1.f : tp)) : seam * 0.4f;   // its throat
        P.chan[C_LIGHT] = seam * 0.25f * rage;                                       // its fins
        P.chan[C_PULSE] = seam * rage;                                               // (each segment's seam: leviathanSegments)
        P.wave = time * 9.f;                                                         // (the corona's flicker)
        return true;
    }
    case EnemyType::PENITENT: {   // kneeling in its chains (sunk 3.3 m), then risen; censers lit for their sweeps; the wound on its back
        PoseOverride ov = penitentPose(e);
        HumanoidMotion mo = humanoidMotion(e.animPhase, e.risen ? std::max(stride, 0.3f) : 0.f, ArmPose::SWING, 0.f, &ov);
        P = humanoidPose(mo, st.glow);
        fileRoot = root * T({0.f, mo.lift, 0.f});
        glowOut = st.glow;
        P.chan[C_EYE] = st.glow * (1.5f + 2.5f * tp);
        P.chan[C_LIGHT] = st.glow;   // its hood's rim, its hem, the seam down its front
        vec3 hot = e.attack == AttackKind::CENSER_HIGH ? vec3{1.6f, 1.6f, 1.5f} : st.glow;
        bool sweep = e.attack == AttackKind::CENSER_LOW || e.attack == AttackKind::CENSER_HIGH;
        P.chan[C_TELL_R] = hot * (sweep ? 1.f + 3.f * tp : 0.8f) * 1.4f;   // (the right hand's censer sweeps)
        P.chan[C_TELL_L] = hot * 0.8f * 1.4f;
        P.chan[C_CORE] = vec3{2.2f, 0.25f, 0.1f} * (0.6f + 0.4f * std::sin(time * 9.f));
        P.flag[F_OPEN] = e.scourging;
        return true;
    }
    case EnemyType::MITE: {
        fileRoot = root;
        P.motion[E_BODY] = T({0.f, std::fabs(std::sin(e.animPhase)) * 0.04f, 0.f});
        const int L[3] = {E_LEG_L1, E_LEG_L2, E_LEG_L3}, R[3] = {E_LEG_R1, E_LEG_R2, E_LEG_R3};
        for (float s : {-1.f, 1.f})
            for (int i = 0; i < 3; ++i) {
                float lift = std::sin(e.animPhase + i * 2.1f + (s > 0.f ? 0.f : 1.f)) * 0.25f;
                P.motion[s > 0.f ? L[i] : R[i]] = RY(s * (0.5f - i * 0.5f)) * RZ(s * (-0.6f + lift));
            }
        P.chan[C_GLOW] = glow;
        vec3 core = glow * 1.5f;
        if (e.attack == AttackKind::FUSE)
            core = std::fmod(e.age * 14.f, 1.f) > 0.5f ? vec3{3.f, 0.4f, 0.2f} : vec3{0.6f, 0.05f, 0.f};
        P.chan[C_FUSE] = core;
        return true;
    }
    default: return false;
    }
}

// THE PACEMAKER, from its state: a runner's stride, its arm blades ("edge") white-hot through a white
// tell, the clock face in its chest ("core") ticking each second and racing as it winds up, "enraged"
// (its overclock: past 30 %) showing its vents
inline bool pacemakerFilePose(const Enemy& e, float time, const mat4& root, mat4& fileRoot, FilePose& P, vec3& glowOut) {
    using namespace enemyfile;
    const EnemyStats& st = e.stats();
    float tp = e.telegraphProgress();
    float stride = glm::clamp(e.moveSpeed / std::max(0.1f, st.speed), 0.f, 1.3f);
    ArmPose pose = ArmPose::SWING; float amt = 0.f;
    switch (e.attack) {
        case AttackKind::DASH: case AttackKind::THRUST: pose = ArmPose::AIM_BOTH; amt = smooth01c(tp * 1.6f); break;
        case AttackKind::SWEEP: case AttackKind::WHIRL: pose = ArmPose::RAISED; amt = 0.55f * smooth01c(tp * 2.f); break;
        case AttackKind::SLAM: case AttackKind::LEAP:   pose = ArmPose::RAISED; amt = smooth01c(tp * 1.5f); break;
        case AttackKind::VOLLEY: case AttackKind::CRESCENT: pose = ArmPose::AIM_RIGHT; amt = smooth01c(tp * 2.5f); break;
        default: break;
    }
    if (e.dashTimer > 0.f) { pose = ArmPose::AIM_BOTH; amt = 1.f; }
    mat4 base = e.staggered() ? root * T({0.f, -0.3f, 0.f}) * RX(0.3f) : root;
    HumanoidMotion mo = humanoidMotion(e.animPhase, e.staggered() ? 0.f : stride, pose, amt, nullptr);
    const bool over = e.enraged;
    vec3 glow = (over ? vec3{1.6f, 0.35f, 0.1f} : st.glow) * (1.f + 2.f * tp);
    P = humanoidPose(mo, glow);
    fileRoot = base * T({0.f, mo.lift, 0.f});
    glowOut = glow;
    const bool white = e.attack == AttackKind::DASH || e.attack == AttackKind::SWEEP || e.attack == AttackKind::THRUST || e.dashTimer > 0.f;
    P.chan[C_EDGE] = white ? vec3{1.7f, 1.7f, 1.6f} * (0.6f + 1.6f * tp) : glow * 0.6f;
    float tick = std::fmod(time * (1.f + 5.f * tp + (over ? 2.f : 0.f)), 1.f);
    P.chan[C_CORE] = glow * (1.2f + 1.8f * (tick < 0.15f ? 1.f : 0.f));
    P.flag[F_ENRAGED] = over;
    P.flag[F_BROKEN] = e.staggered();
    return true;
}

// Where each file rig's joints sit at rest, in the rig's own space (the frame filePoseOf draws
// from), and their parents: where the code rig turns them (tools/enemy_export.cpp, the tests)
inline std::vector<gunkit::ModelJoint> restJointsOf(EnemyType t) {
    using namespace enemyfile;
    std::vector<gunkit::ModelJoint> J;
    auto add = [&](int id, int parent, vec3 at) { J.push_back({id, parent, T(at)}); };
    t = rigBase(t);   // (a mini-boss: its rig's enemy's)
    switch (t) {
    case EnemyType::BALLAST:
        add(E_BODY,E_ROOT,{0,2.5f,0});add(E_HEAD,E_BODY,{0,4.1f,2.3f});
        add(E_HALO,E_BODY,{0,5.9f,-1.2f});add(E_SHIELD,E_BODY,{0,1.05f,3.7f});
        add(E_ARMR,E_BODY,{-2.7f,3.7f,0});add(E_HAND_R,E_ARMR,{-3.8f,1.5f,.8f});
        add(E_LEG_FL,E_BODY,{2.2f,2.5f,1.5f});add(E_LEG_FR,E_BODY,{-2.2f,2.5f,1.5f});
        add(E_LEG_BL,E_BODY,{2.2f,2.5f,-1.8f});add(E_LEG_BR,E_BODY,{-2.2f,2.5f,-1.8f});
        break;

    case EnemyType::RIPPER: {
        add(E_BODY, E_ROOT, {0.f, 0.62f, 0.f});
        add(E_HEAD, E_BODY, {0.f, 0.72f, 0.6f});
        add(E_LEG_FL, E_BODY, {0.27f, 0.50f, 0.42f});  add(E_LEG_FR, E_BODY, {-0.27f, 0.50f, 0.42f});
        add(E_LEG_BL, E_BODY, {0.27f, 0.50f, -0.42f}); add(E_LEG_BR, E_BODY, {-0.27f, 0.50f, -0.42f});
        add(E_BLADE_L, E_BODY, {0.36f, 0.67f, 0.42f}); add(E_BLADE_R, E_BODY, {-0.36f, 0.67f, 0.42f});
        break;
    }
    case EnemyType::WEAVER:   // hips round its body, knees up and out from them (its feet reach the floor)
        add(E_BODY, E_ROOT, {0.f, 0.85f, 0.f});
        for (int k = 0; k < 3; ++k) {
            const float z = 0.45f - 0.45f * k;   // (1 the front pair)
            add(E_LEG_L1 + k, E_BODY, {0.45f, 0.85f, z});         add(E_LEG_R1 + k, E_BODY, {-0.45f, 0.85f, z});
            add(E_SHIN_L1 + k, E_LEG_L1 + k, {1.15f, 1.45f, z * 1.25f}); add(E_SHIN_R1 + k, E_LEG_R1 + k, {-1.15f, 1.45f, z * 1.25f});
        }
        break;
    case EnemyType::SERAPH:   // (the code rig's frames: its ring for a head, two pairs of wings off its body)
        add(E_HEAD, E_ROOT, {0.f, 1.42f, 0.f});
        add(E_WING_L, E_ROOT, {0.2f, 1.05f, -0.05f}); add(E_WING_R, E_ROOT, {-0.2f, 1.05f, -0.05f});
        add(E_TIP_L, E_ROOT, {0.2f, 0.70f, -0.05f});  add(E_TIP_R, E_ROOT, {-0.2f, 0.70f, -0.05f});
        break;
    case EnemyType::CONDUIT:
        add(E_HALO, E_ROOT, {0.f, 1.2f, 0.f}); add(E_CROWN, E_ROOT, {0.f, 2.3f, 0.f}); add(E_HEAD, E_ROOT, {0.f, 3.1f, 0.f});
        break;
    case EnemyType::CONDUCTOR:
        add(E_BODY, E_ROOT, {0.f, 0.85f, 0.f}); add(E_HALO, E_BODY, {0.f, 0.85f, 0.f});
        break;
    case EnemyType::LEVIATHAN:   // (the head rig's frame: the skull at its origin, x1.5 when drawn)
        add(E_HEAD, E_ROOT, {0.f, 0.f, 0.f});
        add(E_JAW, E_HEAD, {0.f, -0.7f, -1.2f});
        add(E_HALO, E_HEAD, {0.f, 1.25f, 2.2f});
        add(E_BODY, E_ROOT, {0.f, 0.f, -12.f});   // (one body segment, laid out from here: set apart from the head in Blender)
        break;
    case EnemyType::RAPTOR:
        add(E_HEAD, E_ROOT, {0.f, 0.62f, 0.55f});
        add(E_TAIL, E_ROOT, {0.f, 0.5f, -0.5f});
        add(E_WING_L, E_ROOT, {0.25f, 0.55f, 0.05f}); add(E_TIP_L, E_WING_L, {1.25f, 0.55f, 0.05f});
        add(E_WING_R, E_ROOT, {-0.25f, 0.55f, 0.05f}); add(E_TIP_R, E_WING_R, {-1.25f, 0.55f, 0.05f});
        break;
    case EnemyType::MITE:
        add(E_BODY, E_ROOT, {0.f, 0.32f, 0.f});
        for (int i = 0; i < 3; ++i) {
            add(E_LEG_L1 + i, E_BODY, {0.22f, 0.32f, 0.18f - i * 0.2f});
            add(E_LEG_R1 + i, E_BODY, {-0.22f, 0.32f, 0.18f - i * 0.2f});
        }
        break;
    case EnemyType::ABSOLUTE: {   // (MORNINGSTAR's too): AbsoluteRig.h's frames, from his body at the hips
        add(E_TORSO, E_ROOT, {0.f, 1.30f, 0.f});
        add(E_HEAD, E_TORSO, {0.f, 2.25f, 0.f});
        add(E_CROWN, E_HEAD, {0.f, 2.25f, 0.f});
        add(E_HALO, E_HEAD, {0.f, 2.49f, -0.34f});
        add(E_WAIST, E_ROOT, {0.f, 1.33f, 0.f});
        add(E_CLOTH_F, E_ROOT, {0.f, 1.30f, 0.14f}); add(E_CLOTH_B, E_ROOT, {0.f, 1.30f, -0.14f});
        for (float s : {1.f, -1.f}) {
            const bool L = s > 0.f;
            const float x = s * 0.15f, a = s * 0.31f;
            add(L ? E_HIPL : E_HIPR, E_ROOT, {x, 1.15f, 0.f});
            add(L ? E_KNEE_L : E_KNEE_R, L ? E_HIPL : E_HIPR, {x, 0.57f, 0.f});
            add(L ? E_ANKLE_L : E_ANKLE_R, L ? E_KNEE_L : E_KNEE_R, {x, 0.02f, 0.f});
            add(L ? E_ARML : E_ARMR, E_TORSO, {a, 2.06f, 0.f});
            add(L ? E_ELBOW_L : E_ELBOW_R, L ? E_ARML : E_ARMR, {a, 1.58f, 0.f});
            add(L ? E_RIBBON_L : E_RIBBON_R, L ? E_ELBOW_L : E_ELBOW_R, {a, 1.22f, -0.06f});
            add(L ? E_HAND_L : E_HAND_R, L ? E_ELBOW_L : E_ELBOW_R, {a, 1.14f, 0.f});
            add(L ? E_FINGERS_L : E_FINGERS_R, L ? E_HAND_L : E_HAND_R, {a, 1.04f, 0.01f});
            add(L ? E_TIPS_L : E_TIPS_R, L ? E_FINGERS_L : E_FINGERS_R, {a, 0.98f, 0.01f});
            add(L ? E_THUMB_L : E_THUMB_R, L ? E_HAND_L : E_HAND_R, {a - s * 0.055f, 1.10f, 0.03f});
            add(L ? E_THUMB_TIP_L : E_THUMB_TIP_R, L ? E_THUMB_L : E_THUMB_R, {a - s * 0.055f, 1.05f, 0.03f});
        }
        add(E_SWORD, E_HAND_R, {-0.31f, 1.07f, -0.035f});   // (the grip, inside the fist)
        break;
    }
    default: {   // a humanoid
        HumanoidLook L = humanoidDims(t);
        float chest = L.legLen + L.pelvisH, shX = L.torsoW * 0.5f + L.armW * 0.5f, shY = chest + L.torsoH - L.armW * 0.5f;
        add(E_HIPL, E_ROOT, {L.hipW * 0.5f, L.legLen, 0.f}); add(E_HIPR, E_ROOT, {-L.hipW * 0.5f, L.legLen, 0.f});
        add(E_TORSO, E_ROOT, {0.f, chest, 0.f});
        add(E_HEAD, E_TORSO, {0.f, chest + L.torsoH, 0.f});
        add(E_ARML, E_TORSO, {shX, shY, 0.f}); add(E_ARMR, E_TORSO, {-shX, shY, 0.f});
        if (t == EnemyType::SHIELDBEARER) add(E_SHIELD, E_TORSO, vec3{0.f, chest, 0.f} + shieldRest());
        if (t == EnemyType::WARDEN) {   // the cape off its back, its hem lower down; the crown on its helm
            const float capeY = chest + L.torsoH - 0.15f, capeZ = -L.torsoD * 0.5f - 0.08f;
            add(E_CAPE, E_TORSO, {0.f, capeY, capeZ});
            add(E_CAPE_TIP, E_CAPE, {0.f, capeY - 1.7f, capeZ});
            add(E_CROWN, E_HEAD, {0.f, chest + L.torsoH, 0.f});
        }
        if (t == EnemyType::SOVEREIGN) {   // (sovereignPose's frames: the sword in the right fist, cape off his shoulders, halo behind his helm)
            const float head = chest + L.torsoH;
            add(E_SWORD, E_ARMR, {-shX, shY - L.armLen - L.armW * 0.3f, 0.f});
            add(E_CAPE, E_TORSO, {0.f, chest + L.torsoH - 0.05f, -L.torsoD * 0.5f - 0.05f});
            add(E_CAPE_TIP, E_CAPE, {0.f, chest + L.torsoH - 1.15f, -L.torsoD * 0.5f - 0.05f});
            add(E_CROWN, E_HEAD, {0.f, head, 0.f});
            add(E_HALO, E_HEAD, {0.f, head + L.headS * 0.6f, -0.42f});
        }
    }
    }
    return J;
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
// THE PENITENT's stance: kneeling while chained (the body sunk 3.3 m, legs
// folded under the floor), standing once risen; a censer drawn back low or
// raised high for each sweep, both fists up for the slam, one arm thrown back
// over its shoulder for the scourge
inline PoseOverride penitentPose(const Enemy& e) {
    PoseOverride o; o.on = true;
    o.crouch = e.risen ? 0.f : 3.3f;
    if (!e.risen && e.riseTimer > 0.f) o.crouch = 3.3f * (e.riseTimer / Enemy::PEN_RISE_TIME);
    float tp = smooth01(e.telegraphProgress() * 1.3f);
    // at rest both arms hang a little out from its sides, the censers clear of its robe (each move blends from there)
    const float RX0 = 0.15f, RZR0 = -0.25f, RZL0 = 0.25f;
    o.rxR = o.rxL = RX0; o.rzR = RZR0; o.rzL = RZL0;
    auto to = [&](float from, float target) { return from + (target - from) * tp; };
    o.torsoPitch = e.scourging ? 0.25f : 0.1f;
    switch (e.attack) {
        case AttackKind::CENSER_LOW:  o.rxR = to(RX0, 0.9f); o.rzR = to(RZR0, 1.2f); o.torsoYaw = -0.5f * tp; break;   // drawn back low
        case AttackKind::CENSER_HIGH: o.rxR = to(RX0, -2.3f); o.rzR = to(RZR0, 0.6f); o.torsoYaw = -0.4f * tp; break;  // raised high
        case AttackKind::PSLAM:       o.rxR = o.rxL = to(RX0, -2.8f); o.torsoPitch = -0.2f * tp; break;              // both fists up
        case AttackKind::PSTOMP:      o.torsoPitch = 0.3f * tp; break;
        case AttackKind::PLASH:       o.rxL = to(RX0, -1.6f); o.rzL = to(RZL0, -0.4f); break;
        case AttackKind::SCOURGE:     o.rxR = to(RX0, -3.0f); o.rzR = to(RZR0, -0.9f); o.torsoPitch = 0.4f; break;   // over its shoulder
        default: break;
    }
    return o;
}

inline vec3 brassOf() { return {0.6f, 0.45f, 0.22f}; }

#include "bosses/absolute/AbsoluteRig.h"   // THE ABSOLUTE's body

// THE LEVIATHAN's body from its file: the one segment there (on "body") laid along each stretch of its
// curve (leviathanBody), scaled to the stretch's radius and length; each seam pulses a beat after the last
inline void leviathanSegments(const Enemy& e, float time, Rig& r, const gunkit::GunModel& m, FilePose P, vec3 glow) {
    using namespace enemyfile;
    const float RS = 2.5f, LS = 2.5f;   // the file's segment: radius and length
    const vec3 seamCol = P.chan[C_PULSE];
    LevSegment seg[LV_SEGMENTS];
    leviathanBody(e, seg);
    for (int i = 0; i < LV_SEGMENTS; ++i) {
        vec3 a = seg[i].c, b = i + 1 < LV_SEGMENTS ? seg[i + 1].c : e.position;
        vec3 dir = b - a; float len = glm::length(dir);
        if (len < 1e-3f) continue;
        dir /= len;
        float yawS = std::atan2(dir.x, dir.z), pitchS = -std::asin(glm::clamp(dir.y, -1.f, 1.f));
        mat4 f = T(a) * RY(yawS) * RX(pitchS) * S({seg[i].r / RS, seg[i].r / RS, len / LS});
        P.chan[C_PULSE] = seamCol * (0.5f + 0.6f + 0.4f * std::sin(time * 3.f - i * 0.7f));
        for (const auto& p : m.parts) {
            if (p.joint != E_BODY) continue;
            vec3 col = p.glowScale >= 0.f ? glow * p.glowScale : p.color * P.tint;
            vec3 emi = p.chan >= 0 && p.chan < C_COUNT ? P.chan[p.chan] * p.k : vec3{0.f};
            r.part(f * p.rel, col, emi, p.mesh);
            if (jointSink()) jointSink()->push_back({(int)r.out.size() - 1, 1000 + i});
        }
    }
}

// Keep legacy procedural rigs off the file-model drawing stack. In WebAssembly
// this switch otherwise reserves every rig's temporary pose arrays even when a
// Story preview only draws its model file.
[[gnu::noinline]] inline void buildEnemyFallback(const Enemy& e, float time, Rig& r,
                                                const mat4& root, float grow, float tp,
                                                vec3 glow, float stride) {
    const EnemyStats& st=e.stats();
    auto& out=r.out;
    switch (e.type) {
    case EnemyType::BALLAST: {
        r.box(root,{0,2.9f,0},{5.6f,1.8f,5.2f},st.color);
        r.box(root,{0,4.45f,2.3f},{2.5f,1.f,1.45f},{.65f,.62f,.52f});
        r.box(root,{0,4.45f,3.1f},{2.1f,.25f,.1f},glow,glow);
        r.box(root,{0,1.1f,3.7f},{7.1f,1.55f,.55f},st.color);
        for(float x:{-2.4f,2.4f})for(float z:{-1.8f,1.5f})r.box(root,{x,1.2f,z},{.8f,2.4f,1.1f},st.color*.7f);
        for(int k=0;k<6;++k)r.box(root*T({0,5.9f,-1.2f})*RZ(e.ballRotor+k*1.0472f),{0,.95f,0},{.38f,1.7f,.25f},st.color*1.4f);
        break;
    }
    case EnemyType::SORTER: {
        r.box(root,{0,3.4f,0},{5.4f,1.25f,2.6f},st.color);
        r.box(root,{0,4.9f,.5f},{2.4f,1.15f,1.7f},{.7f,.7f,.65f});
        r.box(root,{0,4.92f,1.4f},{2.1f,.6f,.15f},glow,glow);
        for(float s:{-1.f,1.f}) {
            r.box(root,{s*2.5f,5.4f,0},{.5f,2.8f,.5f},st.color);
            r.box(root,{s*3.1f,2.8f,0},{.8f,2.4f,.8f},st.color);
            r.box(root,{s*3.7f,1.f,.5f},{.4f,1.7f,.4f},{.6f,.6f,.6f});
            for(float dx:{-.5f,.5f})r.box(root,{s*3.6f+dx,.1f,1},{.2f,.9f,.7f},{.75f,.74f,.7f});
        }
        r.box(root,{0,1.5f,0},{.4f,2.4f,.4f},{.6f,.6f,.6f});
        r.box(root,{0,.5f,0},{2.5f,.5f,2},st.color);break;
    }
    case EnemyType::HUSK: {
        float aim = e.attack == AttackKind::SHOT ? smooth01(tp * 3.f) : 0.35f;
        HumanoidLook L = humanoidLook(e.type, st.color, st.color * 0.45f, glow);   // (no file: the old rig, in code)
        auto f = humanoid(r, root, L, e.animPhase, stride, ArmPose::AIM_RIGHT, aim);
        vec3 gun{0.16f, 0.15f, 0.17f};
        r.box(f.armR, {0.f, -0.78f, 0.06f}, {0.12f, 0.5f, 0.15f}, gun);
        r.box(f.armR, {0.f, -1.05f, 0.06f}, {0.09f, 0.08f, 0.09f}, gun, st.shotColor * (0.3f + 3.f * tp));
        r.box(f.torso, {0.f, 0.35f, -0.22f}, {0.4f, 0.4f, 0.14f}, st.color * 0.6f);   // backpack
        break;
    }
    case EnemyType::REVENANT: {
        // A gaunt machine-knight; its soul glows white-blue in its cracked chest
        HumanoidLook L = humanoidLook(e.type, st.color, st.color * 0.5f, glow);
        bool rake = e.attack == AttackKind::RAKE, bolt = e.attack == AttackKind::SOULBOLT;
        auto f = humanoid(r, root, L, e.animPhase, std::max(stride, 0.25f), rake ? ArmPose::RAISED : ArmPose::SWING, rake ? smooth01(tp * 3.f) : 0.f);
        vec3 bone{0.72f, 0.7f, 0.66f}, soul{0.75f, 0.9f, 1.3f};
        float flare = 1.2f + (bolt ? 4.f * tp : 0.f) + 0.4f * std::sin(time * 6.f);
        r.box(f.torso, {0.f, 0.42f, 0.14f}, {0.22f, 0.26f, 0.08f}, soul * 0.3f, soul * flare);          // the soul in the crack
        for (float s : {-1.f, 1.f}) {
            r.box(f.torso, {s * 0.16f, 0.42f, 0.13f}, {0.08f, 0.38f, 0.06f}, st.color * 1.2f);        // ribs either side
            r.box(s < 0.f ? f.armR : f.armL, {0.f, -0.98f, 0.08f}, {0.05f, 0.3f, 0.05f}, bone, soul * (rake ? 1.5f : 0.3f));   // claws
            r.box(s < 0.f ? f.armR : f.armL, {s * 0.06f, -0.98f, 0.08f}, {0.04f, 0.26f, 0.04f}, bone);
            r.box(f.torso, {s * 0.28f, 0.7f, 0.f}, {0.18f, 0.08f, 0.26f}, st.color * 1.3f);         // shoulder plates
        }
        r.box(f.head, {0.f, 0.16f, 0.13f}, {0.18f, 0.05f, 0.03f}, soul * 0.2f, soul * 2.f);           // eye slit
        r.box(f.head, {0.f, 0.36f, -0.04f}, {0.24f, 0.08f, 0.28f}, st.color * 0.9f);                 // a cracked crest
        break;
    }
    case EnemyType::WEAVER: {
        // A long-legged spider machine; its violet spinner brightens as it strings a wire
        bool str = e.attack == AttackKind::STRING;
        float rear = str ? smooth01(tp * 2.f) : 0.f;
        mat4 body = root * T({0.f, 0.85f + 0.25f * rear, 0.f}) * RX(-0.35f * rear);
        vec3 shell = st.color, under = st.color * 0.5f, violet = glow;
        r.box(body, {0.f, 0.f, 0.f}, {0.9f, 0.4f, 1.2f}, shell);
        r.box(body, {0.f, 0.05f, 0.75f}, {0.55f, 0.32f, 0.4f}, shell * 0.9f);                          // head
        for (float s : {-1.f, 1.f}) r.box(body, {s * 0.15f, 0.12f, 0.97f}, {0.08f, 0.06f, 0.03f}, violet, violet * 2.5f);
        r.box(body, {0.f, 0.32f, -0.45f}, {0.42f, 0.32f, 0.42f}, violet * 0.3f, violet * (0.4f + 3.f * rear)); // the spinner
        float ph = e.animPhase * 1.6f;
        for (int i = 0; i < 6; ++i) {                                                                   // six legs, two rows
            float s = i < 3 ? -1.f : 1.f, z = -0.45f + 0.45f * (i % 3);
            float swing = std::sin(ph + i * 2.1f) * 0.45f * stride;
            mat4 hip = body * T({s * 0.45f, 0.f, z}) * RY(swing) * RZ(s * 0.9f);
            r.box(hip, {0.f, -0.55f, 0.f}, {0.08f, 1.1f, 0.08f}, under);
            mat4 knee = hip * T({0.f, -1.1f, 0.f}) * RZ(-s * 1.5f);
            r.box(knee, {0.f, -0.5f, 0.f}, {0.06f, 1.0f, 0.06f}, under, violet * 0.3f);
        }
        break;
    }
    case EnemyType::LEVIATHAN: {
        // THE LEVIATHAN: a plated serpent out of its root, a long iron skull, a
        // jaw of turbine teeth and the eclipse eye (a black disc, a burning corona)
        if (!e.levInit) break;                                           // not yet risen: nowhere to grow from
        if (e.levHidden() && e.levStage != Enemy::LevStage::RISE) break;   // under the floor: only its wake shows (Gameplay_Leviathan.h)
        const vec3 plate = st.color, seam = vec3{1.3f, 0.3f, 0.12f}, corona = st.glow, bone{0.5f, 0.47f, 0.42f};
        float rage = e.levPhase == 3 ? 1.6f : 1.f;
        LevSegment seg[LV_SEGMENTS];
        leviathanBody(e, seg);
        for (int i = 0; i < LV_SEGMENTS; ++i) {
            vec3 a = seg[i].c, b = i + 1 < LV_SEGMENTS ? seg[i + 1].c : e.position;
            vec3 dir = b - a; float len = glm::length(dir);
            if (len < 1e-3f) continue;
            dir /= len;
            float yawS = std::atan2(dir.x, dir.z), pitchS = -std::asin(glm::clamp(dir.y, -1.f, 1.f));
            mat4 f = T(a) * RY(yawS) * RX(pitchS);
            float rr = seg[i].r;
            float pulse = 0.6f + 0.4f * std::sin(time * 3.f - i * 0.7f);
            r.box(f, {0.f, 0.f, len * 0.5f}, {rr * 1.7f, rr * 1.55f, len + 0.5f}, plate);                       // the ring of plates
            r.box(f, {0.f, -rr * 0.5f, len * 0.5f}, {rr * 1.3f, rr * 0.7f, len * 0.9f}, plate * 0.6f);          // the belly
            r.box(f, {0.f, 0.f, 0.f}, {rr * 1.78f, rr * 1.6f, 0.25f}, plate * 0.3f, seam * (0.5f + pulse) * rage);   // a glowing seam between plates
            r.box(f, {0.f, rr * 0.85f + 0.4f, len * 0.4f}, {0.25f, 0.9f + 0.5f * std::sin(i * 1.3f), len * 0.7f}, bone, seam * 0.25f * rage);   // a dorsal fin
            for (float sd : {-1.f, 1.f})
                r.box(f, {sd * rr * 0.85f, 0.f, len * 0.5f}, {0.2f, rr * 0.9f, len * 0.6f}, plate * 1.3f);       // side plates
        }
        // The head: its frame turned to face, tilted by levPitch
        mat4 h = T(e.position) * RY(e.yaw) * RX(-e.levPitch) * S(vec3{1.5f});   // drawn half again its rig's size
        float tp = e.telegraphProgress();
        bool crash = e.attack == AttackKind::CRASH, torrent = e.attack == AttackKind::TORRENT, swallow = e.attack == AttackKind::SWALLOW || e.levInhale > 0.f;
        r.box(h, {0.f, 0.3f, -0.3f}, {3.2f, 2.0f, 4.6f}, plate);                       // the skull
        r.box(h, {0.f, 1.1f, 0.3f}, {2.4f, 0.6f, 3.6f}, plate * 1.25f);               // the brow ridge
        r.box(h, {0.f, 0.f, 2.2f}, {2.6f, 1.2f, 1.8f}, plate * 0.9f);                 // the snout
        for (float sd : {-1.f, 1.f}) {
            r.box(h * RY(sd * 0.35f), {sd * 1.6f, 1.2f, -1.8f}, {0.3f, 0.5f, 3.5f}, bone);     // swept-back horns
            r.box(h, {sd * 1.65f, 0.f, 0.2f}, {0.2f, 1.2f, 2.8f}, plate * 1.3f, seam * 0.4f * rage);
        }
        // The jaw: hinged under the skull, opening for the torrent and the swallow
        mat4 jaw = h * T({0.f, -0.7f, -1.2f}) * RX(0.9f * e.levJaw);
        r.box(jaw, {0.f, -0.3f, 2.f}, {2.6f, 0.6f, 4.2f}, plate * 0.8f);
        for (int k = 0; k < 6; ++k) {   // turbine teeth along both jaws
            float x = -1.0f + k * 0.4f;
            r.box(jaw, {x, 0.15f, 3.6f}, {0.18f, 0.5f, 0.18f}, bone);
            r.box(h, {x, -0.75f, 2.9f}, {0.18f, 0.5f, 0.18f}, bone);
        }
        vec3 throat = torrent ? vec3{0.3f, 1.1f, 0.9f} * (0.5f + 3.f * tp) : swallow ? vec3{1.6f, 1.5f, 1.4f} * (0.6f + 2.f * (e.levInhale > 0.f ? 1.f : tp)) : seam * 0.4f;
        r.box(jaw, {0.f, 0.05f, 1.6f}, {1.6f, 0.1f, 2.4f}, vec3{0.05f}, throat);      // the throat's glow
        // The eye: a black disc, its corona burning brighter as it strikes; red when it locks on
        vec3 col = crash ? glm::mix(corona, vec3{1.8f, 0.2f, 0.1f}, glm::clamp(tp * 2.f, 0.f, 1.f)) : corona;
        float open = e.levEyeOpen() ? 2.2f : 1.f;
        r.box(h, {0.f, 1.25f, 2.18f}, {1.3f, 1.3f, 0.12f}, vec3{0.01f}, e.levEyeOpen() ? col * 0.25f : vec3{0.f});   // the black disc
        for (int k = 0; k < 12; ++k) {                                                                    // the corona
            float a = k * 0.5235988f + time * 0.6f, flick = 0.8f + 0.2f * std::sin(time * 9.f + k * 1.7f);
            r.box(h * T({0.f, 1.25f, 2.2f}) * RZ(a), {0.95f, 0.f, 0.f}, {0.6f + 0.15f * (k % 2), 0.13f, 0.1f}, col * 0.3f, col * (1.4f + 1.5f * tp) * open * rage * flick);
        }
        for (float sd : {-1.f, 1.f})   // seams of light along the skull, so its shape reads in the dark
            for (int k = 0; k < 3; ++k)
                r.box(h, {sd * 1.62f, 0.9f - 0.45f * k, -0.6f + 0.3f * k}, {0.06f, 0.06f, 3.2f - 0.6f * k}, plate, seam * (0.9f + 0.6f * tp) * rage);
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
    case EnemyType::ANCHOR: {
        HumanoidLook L = humanoidLook(e.type, st.color, st.color * 0.5f, glow);
        bool firing = e.attack == AttackKind::VOLLEY;
        mat4 hunch = root * RX(0.18f);                                       // bent under its weight
        auto f = humanoid(r, hunch, L, e.animPhase, std::max(stride, 0.3f), firing ? ArmPose::AIM_BOTH : ArmPose::SWING,
                          firing ? smooth01(tp * 1.5f) : 0.f);
        float pulse = 0.6f + 0.4f * std::sin(time * 2.5f);
        mat4 ring = f.torso * T({0.f, 0.75f, -0.62f});                      // the ring core on its back
        for (int k = 0; k < 10; ++k)
            r.box(ring * RZ(k * 0.6283f), {0.f, 0.55f, 0.f}, {0.3f, 0.14f, 0.2f}, st.color * 0.8f, glow * (0.6f + pulse));
        r.box(ring, {0.f, 0.f, 0.f}, {0.36f, 0.36f, 0.18f}, glow * 0.3f, glow * (2.f * pulse + 2.f * tp));
        for (float s : {-1.f, 1.f}) {
            r.box(f.torso, {s * 0.72f, 0.98f, 0.f}, {0.55f, 0.36f, 0.7f}, st.color * 1.2f);   // pauldrons
            r.box(s < 0.f ? f.armR : f.armL, {0.f, -1.3f, 0.f}, {0.5f, 0.45f, 0.5f}, st.color * 0.8f);   // fists
        }
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
        vec3 g = wardenGlow(e, glow, tp);
        HumanoidLook L = humanoidLook(e.type, st.color, st.color * 0.6f, g);
        ArmPose pose; float amt;
        wardenArms(e, tp, pose, amt);
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
        float coreSize = e.coreOpen() ? 1.1f : 0.6f;                               // its core: the plates open when it vents
        r.box(f.torso, {0.f, 0.95f, 0.56f}, {coreSize, coreSize, 0.1f}, g * 0.3f, g * (1.5f * pulse + 2.f * tp + (e.coreOpen() ? 2.f : 0.f)));
        for (int i = 0; i < 5; ++i) {                                                // crown
            float x = -0.24f + 0.12f * i;
            float hgt = (i == 2) ? 0.45f : (i % 2 ? 0.3f : 0.22f);
            r.box(f.head, {x, 0.64f + hgt * 0.5f, 0.f}, {0.07f, hgt, 0.07f}, gold, g * 0.8f);
        }
        wardenShards(e, time, grow, g, out);
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
    case EnemyType::SERAPH: {
        bool charging = e.attack == AttackKind::BEAM && e.telegraphTimer > 0.f, sweeping = e.beamTimer > 0.f;
        float bob = std::sin(e.animPhase * 0.5f) * 0.2f;   // from its own state, so headBox can follow it
        mat4 base = T(e.position + vec3{0.f, bob, 0.f}) * RY(e.yaw) * S(vec3{e.scale}) * S(vec3{grow});
        vec3 bone = st.color, dark = st.color * 0.45f;
        r.box(base, {0.f, 0.8f, 0.f},  {0.42f, 0.9f, 0.3f}, bone);                  // body
        r.box(base, {0.f, 0.32f, 0.f}, {0.2f, 0.5f, 0.2f}, dark);                    // tail spine
        r.box(base, {0.f, 0.95f, 0.17f}, {0.18f, 0.18f, 0.06f}, glow * 0.3f,
              glow * (1.5f + (sweeping ? 3.f : 3.f * tp)));                          // chest lens
        mat4 head = base * T({0.f, 1.42f, 0.f});
        for (int k = 0; k < 8; ++k)                                                 // a ring for a head
            r.box(head * RZ(k * 0.7854f), {0.f, 0.26f, 0.f}, {0.14f, 0.08f, 0.06f}, bone, glow * 0.8f);
        float flare = charging ? smooth01(tp) : sweeping ? 1.f : 0.f;
        for (float s : {-1.f, 1.f}) for (int pair = 0; pair < 2; ++pair) {
            float lift = pair == 0 ? 0.5f : -0.15f;
            mat4 wr = base * T({s * 0.2f, 1.05f - pair * 0.35f, -0.05f})
                    * RZ(s * (lift + std::sin(e.animPhase + pair) * 0.18f + flare * 0.5f)) * RY(-s * 0.25f);
            r.box(wr, {s * 0.6f, 0.f, 0.f}, {1.2f - pair * 0.3f, 0.05f, 0.42f}, bone * (pair ? 0.8f : 1.f));
            r.box(wr, {s * 1.2f, 0.f, 0.2f}, {0.1f, 0.06f, 0.5f}, glow * 0.3f, glow * (1.f + 2.f * flare));
        }
        break;
    }
    case EnemyType::RAPTOR: {
        bool diving = e.diveTimer > 0.f;
        float bob = raptorBob(e);   // (from its own state, so headBox can follow it)
        mat4 base = T(e.position + vec3{0.f, bob, 0.f}) * RY(e.yaw) * RX(e.pitch) * S(vec3{e.scale}) * S(vec3{grow});
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
    case EnemyType::ABSOLUTE: buildAbsolute(e, time, r, root); break;   // the most detailed rig in the game (AbsoluteRig.h)
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
    case EnemyType::PENITENT: {
        // Blackened iron, a hood over a bone mask, two censers on chains
        vec3 iron{0.26f, 0.24f, 0.25f}, cloth{0.15f, 0.1f, 0.11f}, bone{0.86f, 0.8f, 0.68f};
        PoseOverride ov = penitentPose(e);
        HumanoidLook L = humanoidLook(e.type, iron, cloth, st.glow);
        auto f = humanoid(r, root, L, e.animPhase, e.risen ? std::max(stride, 0.3f) : 0.f, ArmPose::SWING, 0.f, false, &ov);
        r.box(f.head, {0.f, 0.8f, -0.1f}, {1.9f, 1.9f, 1.9f}, cloth);                                  // the hood
        r.box(f.head, {0.f, 0.75f, 0.92f}, {1.2f, 1.3f, 0.14f}, bone, bone * 0.12f);                   // the mask, out of the hood
        r.box(f.head, {0.f, 0.98f, 1.0f}, {0.85f, 0.14f, 0.05f}, bone * 0.2f, st.glow * (1.5f + 2.5f * tp));   // its eyes
        r.box(f.head, {0.f, 1.78f, 0.86f}, {1.95f, 0.12f, 0.12f}, cloth, st.glow * 0.5f);              // the hood's lit rim
        r.box(f.torso, {0.f, 2.1f, -1.3f}, {3.6f, 3.8f, 0.2f}, cloth * 1.2f);                         // the robe's back
        r.box(f.torso, {0.f, -0.4f, 0.f}, {4.8f, 1.6f, 2.8f}, cloth);                                  // the skirt
        r.box(f.torso, {0.f, -1.18f, 0.f}, {4.9f, 0.14f, 2.9f}, cloth, st.glow * 0.45f);               // its hem, lit
        r.box(f.torso, {0.f, L.torsoH * 0.5f, L.torsoD * 0.5f + 0.06f}, {0.3f, L.torsoH * 0.9f, 0.08f}, cloth, st.glow * 0.6f);   // a seam of light down its front
        for (int k = 0; k < 6; ++k)                                                                    // its chains' collar
            r.box(f.torso, {std::cos(k * 1.047f) * 2.0f, 4.0f, std::sin(k * 1.047f) * 1.2f}, {0.45f, 0.45f, 0.45f}, iron * 1.6f);
        // Censers: hang from each hand on a chain, glowing amber (white when raised high, hot when lashing)
        vec3 hot = e.attack == AttackKind::CENSER_HIGH ? vec3{1.6f, 1.6f, 1.5f} : st.glow;
        for (int s = 0; s < 2; ++s) {
            const mat4& arm = s ? f.armL : f.armR;
            float glowAmt = (s == 0 && (e.attack == AttackKind::CENSER_LOW || e.attack == AttackKind::CENSER_HIGH)) ? 1.f + 3.f * tp : 0.8f;
            for (int c = 0; c < 4; ++c) r.box(arm, {0.f, -L.armLen - 0.6f - c * 0.6f, 0.f}, {0.22f, 0.5f, 0.22f}, iron * 1.5f);
            r.box(arm, {0.f, -L.armLen - 3.6f, 0.f}, {2.0f, 2.2f, 2.0f}, brassOf(), hot * glowAmt * 1.4f);   // the censer
            r.box(arm, {0.f, -L.armLen - 2.4f, 0.f}, {1.3f, 0.3f, 1.3f}, brassOf(), hot * 0.2f);
            r.box(arm, {0.f, -L.armLen - 4.8f, 0.f}, {1.0f, 0.3f, 1.0f}, brassOf());
        }
        if (e.scourging) {                                                                             // the wound
            float pulse = 0.6f + 0.4f * std::sin(time * 9.f);
            r.box(f.torso, {0.f, L.torsoH * 0.55f, -1.5f}, {2.0f, 2.2f, 0.15f}, vec3{0.3f, 0.02f, 0.02f},   // out past the robe's back
                  vec3{2.2f, 0.25f, 0.1f} * pulse);
        }
        break;
    }
    default: break;
    }
}

inline void buildEnemy(const Enemy& e, float time, std::vector<BoxInstance>& out) {
    const EnemyStats& st = e.stats();
    Rig r{out};
    r.flash = glm::clamp(e.hitFlashTimer / 0.12f, 0.f, 1.f);
    r.spawn = 1.f - e.spawnTimer / Enemy::SPAWN_TIME;
    float grow = 0.05f + 0.95f * smooth01(r.spawn);
    r.finish = e.finishGlow;
    if (e.stunTimer > 0.f) r.dim = 0.25f + 0.75f * (std::sin(time * 55.f) > 0.3f ? 1.f : 0.f);   // its eyes gutter while it's stunned

    float tp   = e.telegraphProgress();
    vec3  glow = st.glow * (1.f + 2.5f * tp);           // eyes flare during a wind-up
    float stride = glm::clamp(e.moveSpeed / std::max(0.1f, st.speed), 0.f, 1.2f);

    mat4 root = T(e.position) * RY(e.yaw) * S(vec3{e.scale}) * S({1.f, grow, 1.f});
    {   // A hit tips it over at the feet, away from the shot, squashed a touch, turned a little; it snaps back (HitReact.h)
        float ang = hitreact::flinchAt(e.flinchAmp, time - e.flinchAt);
        if (ang > 1e-3f) {
            float k = ang / hitreact::FLINCH_MAX;
            vec3 axis = glm::cross(vec3{0.f, 1.f, 0.f}, vec3{e.flinchDir.x, 0.f, e.flinchDir.y});
            if (glm::length(axis) > 1e-4f)
                root = T(e.position) * glm::rotate(mat4(1.f), ang, glm::normalize(axis)) * RY(e.yaw + e.flinchTwist * k)
                     * S(vec3{e.scale}) * S({1.f + 0.07f * k, grow * (1.f - 0.1f * k), 1.f + 0.07f * k});
        }
    }

    if (const auto* m = enemyfile::modelOf(enemyfile::fileOf(e))) {   // made in Blender (assets/enemies/<name>.glb)
        mat4 fileRoot; FilePose P; vec3 g;
        const size_t first = out.size();
        if (filePoseOf(e, time, root, fileRoot, P, g)) {
            drawFile(r, fileRoot, *m, P, g, nullptr, e.type == EnemyType::LEVIATHAN ? (int)enemyfile::E_BODY : -1);
            if (e.type == EnemyType::LEVIATHAN) leviathanSegments(e, time, r, *m, P, g);
            if (e.type == EnemyType::ABSOLUTE) absoluteWhiteHot(e, r, first);
            if (e.type == EnemyType::WARDEN) wardenShards(e, time, grow, g, out);
            return;
        }
    }
    buildEnemyFallback(e,time,r,root,grow,tp,glow,stride);
}
// The head hitbox, world space: the box around the drawn head (helmet, crown,
// beak) a little oversized so a shot that grazes it still counts. It follows
// the same frames the model is built from: walk bob, a broken JUGGERNAUT's
// slump, a RIPPER's crouch, a RAPTOR's dive. False for a type without a head
// (MITE). Uses the physics position, like the rest of the hitscan.
inline bool headBox(const Enemy& e, AABB& out) {
    if(e.type==EnemyType::BALLAST) {
        float tilt=e.ballDrive>0?-.08f:std::sin(e.age*1.8f)*.012f;
        mat4 frame=T(e.position)*RY(e.yaw)*S(vec3{e.scale})*T({0,2.5f,0})*RX(tilt)*T({0,1.6f,2.3f})*RY(std::sin(e.age*.4f)*.08f);
        vec3 c=vec3(frame*glm::vec4(0,.45f,0,1)),half{1.55f,.7f,1.f};
        glm::mat3 m(frame);vec3 ext=glm::abs(m[0])*half.x+glm::abs(m[1])*half.y+glm::abs(m[2])*half.z;
        out={c-ext,c+ext};return true;
    }

    if(e.type==EnemyType::SORTER) {
        mat4 frame=T(e.position)*RY(e.yaw)*T({0,4.4f,0})*RY(std::sin(e.age*.5f)*.12f);
        vec3 c=vec3(frame*glm::vec4(0,.5f,.55f,1)),half{1.25f,.65f,1.05f};
        glm::mat3 m(frame);vec3 ext=glm::abs(m[0])*half.x+glm::abs(m[1])*half.y+glm::abs(m[2])*half.z;
        out={c-ext,c+ext};return true;
    }
    const EnemyStats& st = e.stats();
    float grow   = 0.05f + 0.95f * smooth01(1.f - e.spawnTimer / Enemy::SPAWN_TIME);
    float tp     = e.telegraphProgress();
    float stride = glm::clamp(e.moveSpeed / std::max(0.1f, st.speed), 0.f, 1.2f);
    mat4  root   = T(e.position) * RY(e.yaw) * S(vec3{e.scale}) * S({1.f, grow, 1.f});
    mat4  frame;          // the head's frame
    vec3  centre, half;   // the head box within it

    switch (e.type == EnemyType::PACEMAKER ? EnemyType::HUSK : rigBase(e.type)) {   // (a mini-boss: its rig's enemy's head)
    case EnemyType::HUSK: case EnemyType::SENTINEL: case EnemyType::BRUTE:
    case EnemyType::JUGGERNAUT: case EnemyType::WARDEN: case EnemyType::SOVEREIGN: case EnemyType::SHIELDBEARER: case EnemyType::REVENANT:
    case EnemyType::ANCHOR: case EnemyType::PENITENT: {
        HumanoidLook L = humanoidDims(e.type == EnemyType::PACEMAKER ? e.type : rigBase(e.type));
        mat4 base = root;
        if (e.type == EnemyType::JUGGERNAUT && e.staggered()) base = root * T({0.f, -0.25f, 0.f}) * RX(0.32f);
        if (e.type == EnemyType::WARDEN) base = wardenBase(e, root);
        PoseOverride ov;
        if (e.type == EnemyType::ANCHOR) ov.torsoPitch = 0.18f;   // hunched at the waist, as drawn
        if (e.type == EnemyType::SOVEREIGN) {
            if (e.staggered()) { base = root * T({0.f, -0.2f, 0.f}) * RX(0.28f); ov.torsoPitch = 0.2f; }
            else ov = sovereignPose(e);
        }
        if (e.type == EnemyType::PENITENT) ov = penitentPose(e);
        float bob = std::fabs(std::sin(e.animPhase)) * 0.05f * stride;
        frame = base * T({0.f, bob - ov.crouch + L.legLen + L.pelvisH, 0.f}) * RY(ov.torsoYaw) * RX(ov.torsoPitch)
              * T({0.f, L.torsoH, 0.f});
        float w = L.headS, top = L.headS + 0.02f;
        if (e.type == EnemyType::JUGGERNAUT) { w = 0.62f; top = 0.575f; }   // helmet
        if (e.type == EnemyType::WARDEN)     top = 0.95f;                     // crown
        if (e.type == EnemyType::SOVEREIGN)  top = 0.62f;                     // helm and horn roots
        if (e.type == EnemyType::PENITENT)   { w = 1.9f; top = 1.9f; }        // the hood
        centre = {0.f, top * 0.5f, 0.f};
        half   = {w * 0.5f, top * 0.5f, w * 0.5f};
        break;
    }
    case EnemyType::ABSOLUTE: {   // his skull, as drawn (absoluteHeadFrame)
        frame  = absoluteHeadFrame(e, root, absoluteLivePose(e, 0.f));
        centre = {0.f, 0.19f, -0.01f};
        half   = {0.14f, 0.18f, 0.15f};
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
    case EnemyType::SERAPH: {
        float bob = std::sin(e.animPhase * 0.5f) * 0.2f;   // the model's bob
        frame  = T(e.position + vec3{0.f, bob, 0.f}) * RY(e.yaw) * S(vec3{e.scale}) * S(vec3{grow}) * T({0.f, 1.42f, 0.f});
        centre = {0.f, 0.f, 0.f};
        half   = {0.3f, 0.3f, 0.12f};
        break;
    }
    case EnemyType::RAPTOR: {
        mat4 base = T(e.position + vec3{0.f, raptorBob(e), 0.f}) * RY(e.yaw) * RX(e.pitch) * S(vec3{e.scale}) * S(vec3{grow});
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

// THE WARDEN's core: its chest, open while it vents and all through its meltdown
inline bool coreBox(const Enemy& e, AABB& out) {
    if (!e.coreOpen()) return false;
    HumanoidLook L = humanoidDims(e.type);
    mat4 torso = wardenBase(e, T(e.position) * RY(e.yaw)) * T({0.f, L.legLen + L.pelvisH, 0.f});
    vec3 c = vec3(torso * glm::vec4(0.f, L.torsoH * 0.55f, L.torsoD * 0.5f + 0.25f, 1.f));
    vec3 half{0.9f, 1.0f, 0.9f};
    out = {c - half, c + half};
    return true;
}

// THE PENITENT's wound: its back, open while it scourges itself (x3); the
// WARDEN's core: its chest (coreBox)
inline bool woundBox(const Enemy& e, AABB& out) {
    if (e.type == EnemyType::WARDEN) return coreBox(e, out);
    if (e.type != EnemyType::PENITENT || !e.scourging) return false;
    HumanoidLook L = humanoidDims(e.type);
    PoseOverride ov = penitentPose(e);
    mat4 root = T(e.position) * RY(e.yaw);
    mat4 torso = root * T({0.f, -ov.crouch + L.legLen + L.pelvisH, 0.f}) * RY(ov.torsoYaw) * RX(ov.torsoPitch);
    vec3 c = vec3(torso * glm::vec4(0.f, L.torsoH * 0.55f, -L.torsoD * 0.5f - 0.2f, 1.f));
    vec3 half{1.3f, 1.4f, 1.3f};
    out = {c - half, c + half};
    return true;
}

// A shot along d from o that finds THE PENITENT's wound; t: where
// Only from behind: the shot travels the way it faces, and meets the wound
// no deeper than its back (a chest shot from the front doesn't count)
inline bool woundShot(const Enemy& e, glm::vec3 o, glm::vec3 d, float& t) {
    AABB w;
    if (!woundBox(e, w)) return false;
    float face = glm::dot(glm::vec2{d.x, d.z}, glm::vec2{std::sin(e.yaw), std::cos(e.yaw)});
    if (e.type == EnemyType::WARDEN ? face >= 0.f : face <= 0.f) return false;   // the Penitent's from behind, the Warden's from the front
    t = rayBoxHit(o, d, w);
    if (t <= 0.f) return false;
    float tb = rayBoxHit(o, d, e.getAABB());
    return tb <= 0.f || t <= tb + 1.2f;   // its back: the body box stands ~0.9 m proud of the wound
}

} // namespace rig

using rig::buildEnemy;
using rig::headBox;
using rig::woundBox;
using rig::coreBox;
using rig::woundShot;
