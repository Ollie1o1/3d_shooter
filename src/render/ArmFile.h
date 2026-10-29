#pragma once
// =============================================================================
// ArmFile.h — your arms in first person (both on the sword, the left that
// parries and fans the revolver) as a file: assets/guns/arm.glb, made in
// Blender (tools/blender/arm_build.py), the same format as the guns (GunFile.h).
//
//   joints   upper / fore / fist are framed by the code every frame (the
//            shoulder-elbow-wrist chain buildArm has always worked out):
//              upper  at the shoulder, +Z down the upper arm to the elbow
//              fore   at the elbow, +Z down the forearm to the wrist
//              fist   the hand, framed by what it holds: the grip runs
//                     through its origin along +Z (the index finger's end),
//                     the wrist is at -X, the fingers reach out along +X and
//                     curl round the grip toward -Y; +Y the back of the hand
//            and under fist, five fingers of three joints each (thumb1-3,
//            index1-3, middle1-3, ring1-3, pinky1-3), from the knuckle out:
//            each joint's +Z runs down its bone, +Y the nail's side, and it
//            bends about its own +X (positive: toward the palm). In the file
//            the hand is flat; a HandPose bends it.
//            Parts on upper / fore are laid out for an upper arm UPPER long and
//            a forearm FORE long and stretched to the real length. It's a right
//            arm: the left is the same file, mirrored.
//   glow     "seam" the vambrace's seam; "flare" the gauntlet's studs, "rune"
//            its rune line and cuff ring (both blazing on a parry)
//   show     "gauntlet" the parrying gauntlet / "!gauntlet" the plain glove
//
// With no file (or a broken one) the arms are drawn from code (GunKit.h). No OpenGL.
// =============================================================================
#include "render/GunKit.h"
#include "render/GunFile.h"
#include <string>

namespace armfile {
using glm::vec3; using glm::mat4;

enum Joint { A_ROOT, A_UPPER, A_FORE, A_FIST,
             A_THUMB1, A_THUMB2, A_THUMB3, A_INDEX1, A_INDEX2, A_INDEX3, A_MIDDLE1, A_MIDDLE2, A_MIDDLE3,
             A_RING1, A_RING2, A_RING3, A_PINKY1, A_PINKY2, A_PINKY3, A_COUNT };
constexpr int FINGERS = 5;   // thumb, index, middle, ring, pinky: finger f's joints are A_THUMB1 + 3 f + 0..2
inline const char* jointName(int j) {
    static const char* N[A_COUNT] = {"root", "upper", "fore", "fist",
        "thumb1", "thumb2", "thumb3", "index1", "index2", "index3", "middle1", "middle2", "middle3",
        "ring1", "ring2", "ring3", "pinky1", "pinky2", "pinky3"};
    return N[j];
}
enum Chan { AC_CONST, AC_SEAM, AC_FLARE, AC_RUNE, AC_COUNT };
inline const char* chanName(int c) { static const char* N[AC_COUNT] = {"const", "seam", "flare", "rune"}; return N[c]; }
enum Flag { AF_GAUNTLET, AF_COUNT };
inline const char* flagName(int f) { static const char* N[AF_COUNT] = {"gauntlet"}; return N[f]; }
inline const gunkit::Names& names() { static const gunkit::Names n{A_COUNT, AC_COUNT, AF_COUNT, jointName, chanName, flagName}; return n; }

constexpr float UPPER = 0.4f, FORE = 0.36f;   // buildArm's bones
// Where the wrist is in hand space (the forearm ends here: the glove's cuff)
const vec3 WRIST{-0.062f, 0.0235f, 0.f};
inline const char* path() { return "assets/guns/arm.glb"; }

// ---- the hand's pose ------------------------------------------------------------
// Each finger's three bends (degrees, toward the palm; the thumb's first is its
// swing across the palm), its spread (degrees about the knuckle: + toward the
// index side) and, for the thumb, a roll about its own bone (turning its pad to
// face the fingers)
struct HandPose {
    float curl[FINGERS][3] = {};
    float spread[FINGERS] = {};
    float thumbRoll = 0.f;
};
inline HandPose mixHand(const HandPose& a, const HandPose& b, float t) {
    HandPose o;
    for (int f = 0; f < FINGERS; ++f) {
        for (int k = 0; k < 3; ++k) o.curl[f][k] = a.curl[f][k] + (b.curl[f][k] - a.curl[f][k]) * t;
        o.spread[f] = a.spread[f] + (b.spread[f] - a.spread[f]) * t;
    }
    o.thumbRoll = a.thumbRoll + (b.thumbRoll - a.thumbRoll) * t;
    return o;
}
// A pose from a row per finger: {base, middle, tip, spread}, then the thumb's roll
inline HandPose handOf(const float (&rows)[FINGERS][4], float roll) {
    HandPose h;
    for (int f = 0; f < FINGERS; ++f) { for (int k = 0; k < 3; ++k) h.curl[f][k] = rows[f][k]; h.spread[f] = rows[f][3]; }
    h.thumbRoll = roll;
    return h;
}
// Closed round a sword's grip (or a gun's): the fingers wrapped, the thumb over them
inline HandPose handGrip() {
    static const float R[FINGERS][4] = {{37.f, 55.f, 30.f, -55.f}, {86.f, 96.f, 40.f, 2.f}, {90.f, 98.f, 42.f, 0.f},
                                        {92.f, 98.f, 44.f, -2.f}, {94.f, 96.f, 46.f, -5.f}};
    return handOf(R, 64.f);
}
// A tight fist (the gauntlet's punch): the fingers balled up, the thumb locked across them
inline HandPose handFist() {
    static const float R[FINGERS][4] = {{44.f, 60.f, 38.f, -58.f}, {96.f, 104.f, 68.f, 0.f}, {98.f, 106.f, 70.f, 0.f},
                                        {100.f, 106.f, 70.f, -1.f}, {102.f, 104.f, 72.f, -3.f}};
    return handOf(R, 49.f);
}
// Flat and open (braced on the blade's flat), the fingers together, the thumb laid along them
inline HandPose handFlat() {
    static const float R[FINGERS][4] = {{16.f, 6.f, 6.f, 25.f}, {2.f, 3.f, 2.f, 1.f}, {2.f, 3.f, 2.f, 0.f},
                                        {3.f, 4.f, 3.f, -1.f}, {4.f, 5.f, 4.f, -3.f}};
    return handOf(R, -22.f);
}
// Loose, at rest: a natural curl, a little more toward the little finger
inline HandPose handRelaxed() {
    static const float R[FINGERS][4] = {{8.f, 18.f, 16.f, -16.f}, {22.f, 28.f, 16.f, 2.f}, {28.f, 32.f, 18.f, 0.f},
                                        {34.f, 36.f, 20.f, -3.f}, {40.f, 38.f, 22.f, -6.f}};
    return handOf(R, -9.f);
}
// Splayed wide, a claw (winding up a blow)
inline HandPose handSpread() {
    static const float R[FINGERS][4] = {{-17.f, 0.f, -6.f, -11.f}, {14.f, 26.f, 22.f, 10.f}, {16.f, 28.f, 24.f, 2.f},
                                        {18.f, 30.f, 24.f, -8.f}, {20.f, 30.f, 26.f, -16.f}};
    return handOf(R, -8.f);
}
// Fanning a hammer: the palm flat and stiff, the fingers together and cupped a touch, the thumb tucked in
inline HandPose handFan() {
    static const float R[FINGERS][4] = {{18.f, 8.f, 8.f, 26.f}, {10.f, 12.f, 8.f, 0.f}, {12.f, 12.f, 8.f, 0.f},
                                        {14.f, 14.f, 10.f, -1.f}, {16.f, 16.f, 12.f, -3.f}};
    return handOf(R, -24.f);
}

// A hand that moves like one: every joint chases its target on its own spring, a
// little under-damped (it settles with the smallest overshoot), the fingers out
// of step (the index leads, the little finger trails; each tip trails its
// knuckle), and it's never quite still: a faint flex runs through the fingers
// at rest. step() every frame toward the pose wanted; shown() is what to draw.
struct HandSpring {
    HandPose cur = handRelaxed(), vel{};
    float clock = 0.f;
    bool  primed = false;
    // speed: 1 a natural move (~0.15 s), 2-3 a snap (a fist clenching on a blow); calm: 0..1 how much it idles
    void step(const HandPose& want, float dt, float speed = 1.f) {
        if (!primed) { cur = want; primed = true; }
        clock += dt;
        static const float FING[FINGERS] = {1.0f, 1.18f, 1.06f, 0.94f, 0.82f};   // thumb, index ... little finger
        static const float JOINT[3] = {1.f, 0.86f, 0.74f};
        const float Z = 0.58f, H = 1.f / 240.f;
        for (float left = std::min(dt, 0.1f); left > 1e-6f; left -= H) {
            float h = std::min(left, H);
            for (int f = 0; f < FINGERS; ++f) {
                for (int k = 0; k < 3; ++k) {
                    float w = 30.f * speed * FING[f] * JOINT[k];
                    float& x = cur.curl[f][k]; float& v = vel.curl[f][k];
                    v += (w * w * (want.curl[f][k] - x) - 2.f * Z * w * v) * h; x += v * h;
                }
                float w = 26.f * speed * FING[f];
                vel.spread[f] += (w * w * (want.spread[f] - cur.spread[f]) - 2.f * Z * w * vel.spread[f]) * h;
                cur.spread[f] += vel.spread[f] * h;
            }
            float w = 26.f * speed;
            vel.thumbRoll += (w * w * (want.thumbRoll - cur.thumbRoll) - 2.f * Z * w * vel.thumbRoll) * h;
            cur.thumbRoll += vel.thumbRoll * h;
        }
    }
    // A shove through the fingers (a blow landing, a recoil): they flick open by deg, and spring back
    void jolt(float deg) { for (int f = 0; f < FINGERS; ++f) for (int k = 0; k < 3; ++k) vel.curl[f][k] -= deg * (12.f + 4.f * k) * (0.8f + 0.1f * f); }
    HandPose shown(float calm = 1.f) const {
        HandPose o = cur;
        if (calm <= 0.f) return o;
        for (int f = 0; f < FINGERS; ++f) {   // a slow, uneven flex, each finger on its own beat
            float a = std::sin(clock * (0.9f + 0.17f * f) + f * 1.7f) + 0.5f * std::sin(clock * (2.3f + 0.31f * f) + f * 0.6f);
            for (int k = 0; k < 3; ++k) o.curl[f][k] += calm * a * (1.6f - 0.3f * k);
            o.spread[f] += calm * 0.8f * std::sin(clock * 0.7f + f * 2.1f);
        }
        return o;
    }
};

inline gunkit::GunModel& slot() { static gunkit::GunModel m; return m; }
// Each joint's rest frame relative to its parent's (the fingers' bones; worked out once per load)
inline std::vector<mat4>& restRel() { static std::vector<mat4> r; return r; }
inline std::vector<int>& parentOf() { static std::vector<int> p; return p; }
inline void bindRest() {
    const gunkit::GunModel& m = slot();
    restRel().assign(A_COUNT, mat4(1.f)); parentOf().assign(A_COUNT, A_ROOT);
    mat4 rest[A_COUNT]; for (auto& r : rest) r = mat4(1.f);
    for (const auto& J : m.joints) if (J.id > A_ROOT && J.id < A_COUNT) rest[J.id] = J.frame;
    for (const auto& J : m.joints)
        if (J.id > A_ROOT && J.id < A_COUNT) { parentOf()[J.id] = J.parent; restRel()[J.id] = glm::inverse(rest[J.parent]) * J.frame; }
}
// (Re)load the file; false (and the model's error says why) if it couldn't be. A broken save keeps the last good one.
inline bool load(const std::string& p = path()) {
    gunkit::GunModel m = gunkit::readGunFile(p, names());
    if (m.error.empty())
        for (int j = A_UPPER; j < A_COUNT; ++j)
            if (!m.has(j)) { m.error = p + ": no '" + jointName(j) + "' joint"; break; }
    if (m.error.empty())
        for (const auto& J : m.joints)   // each finger hangs off the hand, or the bone before it
            if (J.id >= A_THUMB1 && J.parent != ((J.id - A_THUMB1) % 3 == 0 ? (int)A_FIST : J.id - 1)) {
                m.error = p + ": '" + jointName(J.id) + "' isn't under '" + jointName((J.id - A_THUMB1) % 3 == 0 ? (int)A_FIST : J.id - 1) + "'";
                break;
            }
    bool ok = m.error.empty();
    if (ok || slot().parts.empty()) { slot() = std::move(m); if (ok) bindRest(); }
    else slot().error = m.error;
    return ok;
}
inline const gunkit::GunModel* model() {
    static bool tried = false;
    if (!tried) { tried = true; load(); }
    return slot().parts.empty() || !slot().error.empty() ? nullptr : &slot();
}

// How one arm looks this frame
struct ArmLook {
    bool  open = false;        // a flat palm (the code-drawn arm, with no file: the file's hand takes `hand`)
    bool  gauntlet = false;    // the parrying gauntlet over the hand
    float seam = 0.8f;         // the vambrace seam's strength (x the sword's glow colour)
    vec3  glow{1.f};           // the gauntlet's colour (the gun's, or the sword's)
    float flare = 0.f;         // 0..1: the gauntlet blazing as it lands a parry
    HandPose hand = handGrip();
    vec3  pole{0.f};           // the way the elbow bends out (zero: out and down)
    bool  wrist = false;       // the forearm follows the hand: held within a wrist's reach of it, and rolled with it
};

// A frame at a, +Z toward c (stretched by |c - a| / rest), +Y as near `up` as it can be (limb()'s frame)
inline mat4 along(vec3 a, vec3 c, float rest, vec3 up = {0.f, 1.f, 0.f}) {
    vec3 z = c - a; float len = glm::length(z);
    z = len > 1e-5f ? z / len : vec3{0.f, 0.f, 1.f};
    vec3 x = glm::cross(up, z);
    if (glm::length(x) < 1e-4f) x = glm::cross(vec3{1.f, 0.f, 0.f}, z);
    x = glm::normalize(x);
    vec3 y = glm::cross(z, x);
    mat4 m(1.f);
    m[0] = glm::vec4(x, 0.f); m[1] = glm::vec4(y, 0.f); m[2] = glm::vec4(z * (len / rest), 0.f); m[3] = glm::vec4(a, 1.f);
    return m;
}
inline mat4 mirrorOf(float side) { return side < 0.f ? glm::scale(mat4(1.f), vec3{-1.f, 1.f, 1.f}) : mat4(1.f); }

// How far a wrist bends (degrees, from the forearm running straight out of the
// back of the hand): toward the thumb / the little finger, back / toward the palm
constexpr float RADIAL = 22.f, ULNAR = 42.f, EXTEND = 55.f, FLEX = 62.f;
// Where the forearm runs from the wrist (hand space, toward the elbow), brought within
// the wrist's reach: hand space -X is straight, +Z the thumb's side, +Y the back of the hand
inline vec3 wristReach(vec3 d) {
    d = glm::normalize(d);
    float th = std::acos(std::clamp(-d.x, -1.f, 1.f));   // how far it's bent
    float rz = d.z > 0.f ? RADIAL : ULNAR, ry = d.y > 0.f ? EXTEND : FLEX;   // (+Z: the forearm on the blade's side, the hand bent back toward the thumb)
    float ph = std::atan2(d.y, d.z), c = std::cos(ph) / glm::radians(rz), s = std::sin(ph) / glm::radians(ry);
    float lim = 1.f / std::sqrt(c * c + s * s);   // the ellipse of reach, that way round
    if (th <= lim) return d;
    return vec3{-std::cos(lim), std::sin(lim) * std::sin(ph), std::sin(lim) * std::cos(ph)};
}
// Holding the sword (ViewModel::drawSword): where the shoulders are as the body turns into the cut
inline vec3 swordShoulder(const SwordPose& P, float side) {
    vec3 twist{0.3f * (P.hand.x - 0.2f), 0.12f * (P.hand.y + 0.22f), 0.1f * (P.hand.z - 0.36f)};
    return vec3{0.34f * side, -0.62f, 0.f} + twist * (side > 0.f ? 1.f : 0.6f);
}
// The pose with its edge rolled about the blade to where the wrists can take it: the
// knuckles face the edge, and they can't face far (`most` degrees) from where the forearm
// points (both forearms, between them, with both hands on the grip). The point doesn't move.
// near: the edge it had last frame. Where the wanted edge could be reached either way round
// (it faces straight back up the forearm), the roll keeps to the side it was on.
inline SwordPose swordHeld(SwordPose P, const vec3* near = nullptr, float most = 50.f) {
    vec3 z = glm::normalize(P.blade);
    auto flat = [&](vec3 v) { v -= z * glm::dot(v, z); return glm::length(v) > 1e-4f ? glm::normalize(v) : v; };   // seen down the blade
    auto forearm = [&](float side) {   // (its length: how square to the blade it is)
        vec3 sh = swordShoulder(P, side);
        vec3 f = glm::normalize(P.hand - gunkit::elbowOf(sh, P.hand, UPPER, FORE, glm::normalize(vec3{side * 1.f, -1.3f, -0.3f})));
        return f - z * glm::dot(f, z);
    };
    vec3 e = flat(P.edge);
    vec3 f = forearm(1.f);
    float square = glm::length(f);
    if (square < 1e-4f || glm::length(e) < 1e-4f) return P;
    f /= square;
    const vec3 ref = near ? flat(*near) : e;
    if (P.twoHand > 0.f) {
        float k = 0.5f * std::min(P.twoHand, 1.f);
        vec3 fl = forearm(-1.f);
        square = square * (1.f - k) + glm::length(fl) * k;
        if (glm::length(fl) > 1e-4f) fl = glm::normalize(fl);
        vec3 g = f * (1.f - k) + fl * k;
        // the forearms near opposite (seen down the blade): between them is either way round, so keep to one
        vec3 side = glm::cross(z, f); if (glm::dot(side, ref) < 0.f) side = -side;
        float l = glm::length(g), w = std::clamp((0.6f - l) / 0.4f, 0.f, 1.f);
        f = glm::normalize(g + side * (w * w * (3.f - 2.f * w)));
        most *= 1.f - 0.8f * std::min(P.twoHand, 1.f);   // (the edge between the forearms, each wrist bent half the gap)
    }
    // with the blade near in line with the forearm (the wrist bent hard toward the little finger)
    // the hand can roll it any way: the limit lets go
    float free = std::clamp((0.6f - square) / 0.35f, 0.f, 1.f);
    most += (180.f - most) * free * free * (3.f - 2.f * free);
    float a = std::atan2(glm::dot(glm::cross(f, e), z), glm::dot(f, e)), lim = glm::radians(most);
    if (std::fabs(a) > lim) {
        auto at = [&](float t) { return f * std::cos(t) + glm::cross(z, f) * std::sin(t); };
        vec3 lo = at(-lim), hi = at(lim);
        if (near && std::fabs(a) > glm::radians(120.f)) e = glm::dot(lo, ref) > glm::dot(hi, ref) ? lo : hi;   // (either way round: stay)
        else e = a > 0.f ? hi : lo;
    }
    P.edge = e;
    return P;
}
// The shoulder-elbow-wrist chain for a hand framed at `fist`: side +1 right, -1 left
// (pole: the way the elbow bends out; zero: out and down, the usual). keepWrist: the
// forearm kept within the wrist's reach of the hand (the elbow moves for it; the upper
// arm, off the bottom of the screen, takes up the difference)
inline void chain(vec3 shoulder, const mat4& fist, float side, vec3& elbow, vec3& wrist, vec3 pole = vec3{0.f}, bool keepWrist = false) {
    const mat4 hand = fist * mirrorOf(side);
    wrist = vec3(hand * glm::vec4(WRIST, 1.f));
    if (glm::length(pole) < 1e-4f) pole = vec3{side * 1.f, -1.3f, -0.3f};
    elbow = gunkit::elbowOf(shoulder, wrist, UPPER, FORE, glm::normalize(pole));
    if (keepWrist) {
        glm::mat3 R{hand};   // (hand space: the mirrored left's is the right's)
        glm::mat3 inv = glm::inverse(R);
        vec3 d = wristReach(inv * (elbow - wrist));
        elbow = wrist + glm::normalize(R * d) * FORE;
    }
}

// A hand frame (for fist) from where its fingers point and where the back of the hand faces;
// side -1 the left (mirrored: its fingers still point `fingers`)
inline mat4 handFrame(vec3 at, vec3 fingers, vec3 back, float side) {
    vec3 x = glm::normalize(fingers) * (side < 0.f ? -1.f : 1.f);
    vec3 y = back - glm::dot(back, x) * x;
    y = glm::length(y) < 1e-4f ? vec3{0.f, 1.f, 0.f} : glm::normalize(y);
    vec3 z = glm::cross(x, y);
    mat4 m(1.f);
    m[0] = glm::vec4(x, 0.f); m[1] = glm::vec4(y, 0.f); m[2] = glm::vec4(z, 0.f); m[3] = glm::vec4(at, 1.f);
    return m;
}

// Every joint's frame this frame: the arm's chain, then the fingers bent down from the hand
inline void pose(vec3 shoulder, const mat4& fist, float side, const HandPose& hp, mat4 W[A_COUNT], vec3 pole = vec3{0.f}, bool keepWrist = false) {
    vec3 elbow, wrist;
    chain(shoulder, fist, side, elbow, wrist, pole, keepWrist);
    const mat4 mirror = mirrorOf(side);
    W[A_ROOT] = mat4(1.f);
    W[A_UPPER] = along(shoulder, elbow, UPPER) * mirror;
    // the forearm's top: up, or (keepWrist) the back of the hand, the forearm turning with it
    W[A_FORE] = along(elbow, wrist, FORE, keepWrist ? vec3(fist[1]) : vec3{0.f, 1.f, 0.f}) * mirror;
    W[A_FIST] = fist * mirror;
    const auto& rel = restRel();
    for (int f = 0; f < FINGERS; ++f)
        for (int k = 0; k < 3; ++k) {
            int j = A_THUMB1 + 3 * f + k;
            mat4 bend = glm::rotate(mat4(1.f), glm::radians(hp.curl[f][k]), vec3{1.f, 0.f, 0.f});
            if (k == 0) {
                bend = glm::rotate(mat4(1.f), glm::radians(hp.spread[f]), vec3{0.f, 1.f, 0.f}) * bend;
                if (f == 0) bend = bend * glm::rotate(mat4(1.f), glm::radians(hp.thumbRoll), vec3{0.f, 0.f, 1.f});
            }
            W[j] = W[k == 0 ? A_FIST : j - 1] * (j < (int)rel.size() ? rel[j] : mat4(1.f)) * bend;
        }
}

// One arm from the file into b; false if there's no file (draw it from code then)
inline bool draw(gunkit::Builder& b, vec3 shoulder, const mat4& fist, float side, const ArmLook& L) {
    const gunkit::GunModel* m = model();
    if (!m) return false;
    mat4 W[A_COUNT];
    pose(shoulder, fist, side, L.hand, W, L.pole, L.wrist);
    vec3 chan[AC_COUNT];
    chan[AC_CONST] = vec3{1.f};
    chan[AC_SEAM] = gunkit::SWORD_GLOW * L.seam;
    chan[AC_FLARE] = L.glow * (0.7f + 3.5f * L.flare) + vec3{1.f, 0.9f, 0.7f} * (2.f * L.flare);
    chan[AC_RUNE] = L.glow * (1.f + 3.f * L.flare);
    const bool flag[AF_COUNT] = {L.gauntlet};
    for (const auto& p : m->parts) {
        if (p.flag >= 0 && p.flag < AF_COUNT && flag[p.flag] != p.want) continue;
        if (p.joint <= A_ROOT || p.joint >= A_COUNT) continue;
        vec3 col = p.glowScale >= 0.f ? L.glow * p.glowScale : p.color;
        vec3 emi = p.chan >= 0 && p.chan < AC_COUNT ? chan[p.chan] * p.k : vec3{0.f};
        b.out.push_back({W[p.joint] * p.rel, col, emi, 0, p.mesh});
    }
    return true;
}

} // namespace armfile

namespace gunkit {
// An arm: from its file, or (no file) from code
inline void drawArm(Builder& b, vec3 shoulder, const mat4& fist, float side, const armfile::ArmLook& L) {
    if (armfile::draw(b, shoulder, fist, side, L)) return;
    buildArm(b, shoulder, fist, side, L.open, L.seam);
    if (L.gauntlet) buildGauntlet(b, fist, L.glow, L.flare);
}
} // namespace gunkit
