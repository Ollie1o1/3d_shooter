#pragma once
// =============================================================================
// Holdfast.h — UNIT 24's HOLDFAST (Chapter 6, docs/chapter-6-design.md
// section 5, where it is still called by its working name, ANCHOR. That name
// already belongs to EnemyType::ANCHOR, the Penitent's chains and a Ballast
// attack, so the code uses HOLDFAST).
//
// A mag-spike on a line. Fire it into something solid and you are held to it:
// free to aim, shoot, parry and swing inside the line's length, not free to
// leave. Press again to let go. Fire it into a small enemy and it is nailed
// where it stands for a few seconds (it cannot reach shelter). One spike at a
// time.
//
// It keeps you from drifting. It does not replace shelter: whether a held
// player survives a sweep is decided by what they are held to
// (SweepShelter::holds) and by the shadow rule, in Gameplay_Ring.h.
//
// No OpenGL and no GameplayState: the numbers and the rope live here so the
// tests can check them. Gameplay_Holdfast.h aims, pins and draws.
// =============================================================================
#include <glm/glm.hpp>
#include <algorithm>

inline const glm::vec3 HOLDFAST_STEEL{0.82f, 0.8f, 0.72f};   // the line and its feed text (the world's sand-white, not UNIT 24's cyan)

struct Holdfast {
    static constexpr float RANGE = 30.f;         // how far the spike reaches
    static constexpr float MIN_LINE = 2.5f;      // a point-blank spike still leaves room to move
    static constexpr float COOLDOWN = 0.6f;      // after letting go
    static constexpr float MISS_COOLDOWN = 0.3f;
    static constexpr float PIN_TIME = 4.f;       // how long a pinned enemy stays nailed
    static constexpr float PIN_COOLDOWN = 5.f;   // after a pin ends

    enum class State { IDLE, HELD, PIN };
    enum class Press { NONE, FIRE, RELEASED };

    State     state = State::IDLE;
    glm::vec3 point{0.f};     // HELD: where the spike sits (it rides its body: carry())
    float     length = 0.f;   // HELD: the line
    int       body = -1;      // HELD: the wall it is in (-1: none known)
    int       shelter = -1;   // HELD: the SweepShelter that wall belongs to (-1: not a shelter)
    int       pinUid = -1;    // PIN: the enemy
    float     pinT = 0.f;
    float     cooldown = 0.f;

    void reset() { *this = Holdfast{}; }
    bool held() const { return state == State::HELD; }
    bool pinning() const { return state == State::PIN; }
    bool ready() const { return state == State::IDLE && cooldown <= 0.f; }
    float pin01() const { return pinning() ? std::clamp(pinT / PIN_TIME, 0.f, 1.f) : 0.f; }
    float cooldown01() const { return std::clamp(1.f - cooldown / PIN_COOLDOWN, 0.f, 1.f); }

    // The button. FIRE: the caller casts the ray, then calls latch(), pin() or miss().
    Press press() {
        if (state == State::HELD) { letGo(COOLDOWN); return Press::RELEASED; }
        return ready() ? Press::FIRE : Press::NONE;
    }
    void latch(glm::vec3 at, glm::vec3 from, int wall, int shelterIndex) {
        state = State::HELD; point = at; body = wall; shelter = shelterIndex;
        length = std::clamp(glm::length(at - from), MIN_LINE, RANGE);
    }
    void pin(int uid) { state = State::PIN; pinUid = uid; pinT = PIN_TIME; }
    void miss() { cooldown = MISS_COOLDOWN; }
    // The line is cut, or what it was in is gone
    void cut() { if (state == State::HELD) letGo(COOLDOWN); }
    void letGo(float cd) { state = State::IDLE; body = shelter = pinUid = -1; cooldown = std::max(cooldown, cd); }

    // Its body moved (a drifting chunk, a tine): the spike goes with it
    void carry(glm::vec3 delta) { if (held()) point += delta; }

    // The rope: `pos` (the point the line is tied to on UNIT 24) stays within
    // `length` of the spike, and any speed away from it is lost. True if it
    // pulled.
    bool constrain(glm::vec3& pos, glm::vec3& vel) const {
        if (!held()) return false;
        glm::vec3 d = pos - point;
        const float n = glm::length(d);
        if (n <= length || n < 1e-4f) return false;
        d /= n;
        pos = point + d * length;
        const float out = glm::dot(vel, d);
        if (out > 0.f) vel -= d * out;
        return true;
    }

    // True on the tick a pin runs out (or its enemy died: pinGone())
    bool tick(float dt) {
        cooldown = std::max(0.f, cooldown - dt);
        if (state != State::PIN) return false;
        pinT -= dt;
        if (pinT > 0.f) return false;
        letGo(PIN_COOLDOWN);
        return true;
    }
    void pinGone() { if (state == State::PIN) letGo(PIN_COOLDOWN); }
};
