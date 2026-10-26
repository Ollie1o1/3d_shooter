#pragma once
// =============================================================================
// CameraFeel.h — how the first-person camera reacts. No GL: GameplayState
// feeds it events, ticks it every frame, and adds what it reads out to the
// render camera only (never to where you aim, except that the trauma shake
// moves the screen a little).
//
//   TRAUMA  shake() adds trauma (0..1); the shake is trauma squared, so small
//           knocks barely move and big ones hit hard. The angles come from
//           smooth noise (not per-frame random jitter), mostly rotation, and
//           look the same at 60 or 240 fps. Trauma drains at a fixed rate
//           after an optional hold (a long rumble).
//   PUNCH   a sprung kick to pitch / yaw / roll (degrees): a shot, a landing,
//           a hit from one side. Kicked as a velocity, so it eases out, peaks
//           and settles with a small overshoot.
//   DIP     the same spring on the eye's height: a landing sinks the head,
//           a jump pushes off.
//   ROLL    a lean into a strafe (Quake's cl_rollangle).
//   STRIDE  a bob driven by distance covered, so each low point is a
//           footstep (the game plays the step on stepTaken()).
// =============================================================================
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace camfeel {

// Smooth 1D value noise in -1..1 (a different curve per seed)
inline float noise1(float x, uint32_t seed) {
    auto hash = [seed](int32_t i) {
        uint32_t h = (uint32_t)i * 0x9E3779B1u ^ seed * 0x85EBCA77u;
        h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12; h *= 0x297A2D39u; h ^= h >> 15;
        return (float)(h & 0xFFFFFF) / (float)0x7FFFFF - 1.f;
    };
    float fl = std::floor(x), f = x - fl;
    int32_t i = (int32_t)fl;
    float u = f * f * (3.f - 2.f * f);
    return hash(i) + (hash(i + 1) - hash(i)) * u;
}

// A damped spring toward zero, ticked in small fixed steps so any frame rate
// gives the same motion
struct Spring3 {
    glm::vec3 x{0.f}, v{0.f};
    float omega = 14.f, zeta = 0.6f;   // stiffness (rad/s) and damping ratio
    void tick(float dt) {
        const float H = 1.f / 480.f;
        while (dt > 1e-6f) {
            float h = std::min(dt, H);
            v += (-omega * omega * x - 2.f * zeta * omega * v) * h;
            x += v * h;
            dt -= h;
        }
    }
};

struct CameraFeel {
    // ---- tuning ----
    static constexpr float SHAKE_PITCH = 2.6f, SHAKE_YAW = 2.6f, SHAKE_ROLL = 3.2f;   // degrees at full trauma
    static constexpr float SHAKE_MOVE  = 0.05f;   // metres at full trauma
    static constexpr float SHAKE_FREQ  = 22.f;    // noise samples a second
    static constexpr float TRAUMA_DRAIN = 1.8f;   // per second, after the hold
    static constexpr float ROLL_MAX    = 1.4f;    // strafe lean (degrees) at ROLL_SPEED
    static constexpr float ROLL_SPEED  = 7.f;     // m/s sideways for the full lean
    static constexpr float STEP_LEN    = 2.6f;    // metres per footstep at a run
    static constexpr float BOB_Y       = 0.028f, BOB_X = 0.016f, BOB_ROLL = 0.35f;

    float trauma = 0.f, hold = 0.f, clock = 0.f;
    Spring3 punchS;                       // degrees: x pitch, y yaw, z roll
    Spring3 dipS{glm::vec3(0.f), glm::vec3(0.f), 12.f, 0.5f};   // metres, y only used
    float roll = 0.f;                     // the strafe lean, smoothed
    float stride = 0.f, stridePrev = 0.f; // radians: one footstep per pi
    float bobAmt = 0.f;                   // 0 standing .. 1 running, smoothed
    int   stepsTaken = 0;
    float kickScale = 1.f;                // punches and dips scale with SCREEN SHAKE (set by the game)

    void reset() { float k = kickScale; *this = CameraFeel{}; kickScale = k; }

    // t: how long it should last (seconds), amount: the old shake's size
    // (metres: 0.01 a pistol, 0.04 a hit, 0.08 a slam, 0.12 a boss)
    void shake(float t, float amount, float scale = 1.f) {
        if (scale <= 0.f) return;
        float a = std::clamp(amount / 0.1f, 0.f, 1.f) * scale;
        trauma = std::min(1.f, std::max(trauma, a));
        hold = std::max(hold, std::min(t - 0.25f, 1.5f));   // a long one rumbles before it drains
    }

    // Velocity kicks (degrees a second): + pitch looks up, + yaw turns right, + roll leans right
    void punch(float pitch, float yaw = 0.f, float rollDeg = 0.f) { punchS.v += glm::vec3{pitch, yaw, rollDeg} * kickScale; }
    // Eye height kick (metres a second): - sinks
    void dip(float v) { dipS.v.y += v * kickScale; }

    // A footstep each time the stride passes pi. Called on the physics tick.
    // Returns true on the tick a foot comes down.
    bool stepTick(float xzSpeed, bool grounded, float dt) {
        stridePrev = stride;
        if (!grounded || xzSpeed < 1.5f) {
            // settle to the nearest foot-down so the next step starts clean
            float down = std::round(stride / 3.14159265f) * 3.14159265f;
            stride += (down - stride) * std::min(1.f, dt * 8.f);
            stridePrev = stride;
            return false;
        }
        int before = (int)std::floor(stride / 3.14159265f);
        // a bit quicker steps when walking than distance alone would give
        float len = STEP_LEN * std::clamp(0.55f + xzSpeed / 15.f, 0.65f, 1.15f);
        stride += xzSpeed * dt / len * 3.14159265f;
        if ((int)std::floor(stride / 3.14159265f) != before) { ++stepsTaken; return true; }
        return false;
    }

    // Every frame: lateral = signed sideways speed (m/s, + right)
    void update(float dt, float lateral, float xzSpeed, bool grounded) {
        clock += dt;
        if (hold > 0.f) hold -= dt;
        else trauma = std::max(0.f, trauma - TRAUMA_DRAIN * dt);
        punchS.tick(dt);
        dipS.tick(dt);
        float target = std::clamp(lateral / ROLL_SPEED, -1.f, 1.f) * ROLL_MAX;
        roll += (target - roll) * (1.f - std::exp(-dt * 9.f));
        float bobTarget = grounded ? std::clamp((xzSpeed - 1.f) / 7.f, 0.f, 1.f) : 0.f;
        bobAmt += (bobTarget - bobAmt) * (1.f - std::exp(-dt * 10.f));
    }

    // ---- read out (render) ----
    float shakeK() const { return trauma * trauma; }
    // pitch, yaw, roll in degrees: shake + punch + lean (+ bob's sway when bob is on)
    glm::vec3 angles(float aim, bool bob, float alpha) const {
        float s = shakeK() * (1.f - 0.7f * aim), t = clock * SHAKE_FREQ;
        glm::vec3 a{noise1(t, 11u) * SHAKE_PITCH, noise1(t, 23u) * SHAKE_YAW, noise1(t, 37u) * SHAKE_ROLL};
        a *= s;
        a += punchS.x * (1.f - 0.5f * aim);
        if (bob) {
            a.z += roll * (1.f - aim);
            a.z += std::sin(strideAt(alpha)) * BOB_ROLL * bobAmt * (1.f - aim);
        }
        return a;
    }
    // A camera-space offset (x right, y up), metres
    glm::vec2 offset(float aim, bool bob, float alpha) const {
        float s = shakeK() * (1.f - 0.7f * aim), t = clock * SHAKE_FREQ;
        glm::vec2 o{noise1(t, 51u), noise1(t, 67u)};
        o *= s * SHAKE_MOVE;
        o.y += dipS.x.y;
        if (bob) {
            float ph = strideAt(alpha), k = bobAmt * (1.f - aim);
            o.x += std::sin(ph) * BOB_X * k;                               // one side per pair of steps
            o.y += (std::fabs(std::sin(ph)) - 0.5f) * 2.f * BOB_Y * k;    // lowest on each foot-down
        }
        return o;
    }
    float strideAt(float alpha) const { return stridePrev + (stride - stridePrev) * alpha; }
};

}  // namespace camfeel
