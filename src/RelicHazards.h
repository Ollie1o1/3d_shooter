#pragma once
// =============================================================================
// RelicHazards.h — the Reliquary's: REVENANT souls in flight (shoot them, punch
// them, or they re-form the body at a spot on another chunk) and WEAVER wires
// (Task 4). No OpenGL or audio: Gameplay_Reliquary.h feeds and draws them.
// =============================================================================
#include "Enemy.h"
#include "StyleSystem.h"
#include <vector>
#include <cmath>
#include <algorithm>

struct Soul {
    int fromUid = 0; EnemyType type = EnemyType::REVENANT; Hollow hollow = Hollow::NONE;
    glm::vec3 pos{0.f}, from{0.f}, to{0.f};
    float t = 0.f, hp = 40.f, bodyHealth = 80.f, scale = 1.f;
    int reforms = 1;
};

// Where a soul goes: the spawn spot furthest from where it died, at least minDist away
inline glm::vec3 soulDestination(const std::vector<glm::vec3>& spawns, glm::vec3 from, float minDist) {
    glm::vec3 best = from; float bd = -1.f;
    for (auto& s : spawns) { float d = glm::length(s - from); if (d >= minDist && (bd < 0.f || d < bd)) { bd = d; best = s; } }
    if (bd < 0.f) for (auto& s : spawns) { float d = glm::length(s - from); if (d > bd) { bd = d; best = s; } }
    return best;
}

struct Wire { int owner = 0; glm::vec3 a{0.f}, b{0.f}; float age = 0.f; };

// Does a dead Revenant's soul flee? Always - unless its body went into the void
inline bool revenantSoulEscapes(StyleSource src, bool inVoid) { (void)src; return !inVoid; }

class RelicHazards {
public:
    static constexpr float SOUL_TIME = 4.f, SOUL_HP = 40.f, SOUL_RADIUS = 0.6f, SOUL_ARC = 6.f;
    std::vector<Soul> souls;

    // A Revenant's body died: its soul flees to `to` (nullptr: that was its last life)
    Soul* releaseSoul(const Enemy& e, glm::vec3 to) {
        if (e.reforms >= 2 || e.splitsOnDeath()) return nullptr;   // last life, or a Twinned one whose twins carry it on
        Soul s; s.fromUid = e.uid; s.type = e.type; s.hollow = e.hollow; s.scale = e.scale;
        s.from = s.pos = e.position + glm::vec3{0.f, e.height() * 0.6f, 0.f};
        s.to = to; s.hp = SOUL_HP; s.bodyHealth = e.maxHealth * 0.5f; s.reforms = e.reforms + 1;
        souls.push_back(s);
        return &souls.back();
    }
    // Ray (o along d, up to maxT) against the souls: the nearest hit, or -1
    int raySoul(glm::vec3 o, glm::vec3 d, float maxT, float& t) const {
        int best = -1; float bt = maxT;
        for (int i = 0; i < (int)souls.size(); ++i) {
            glm::vec3 oc = souls[i].pos - o;
            float along = glm::dot(oc, d);
            if (along < 0.f || along > bt) continue;
            if (glm::length(oc - d * along) < SOUL_RADIUS) { bt = along; best = i; }
        }
        t = bt; return best;
    }
    // True when that killed it (it's removed)
    bool hurtSoul(int i, float dmg) {
        if (i < 0 || i >= (int)souls.size()) return false;
        souls[i].hp -= dmg;
        if (souls[i].hp > 0.f) return false;
        souls.erase(souls.begin() + i);
        return true;
    }
    int soulNear(glm::vec3 p, float r) const {
        for (int i = 0; i < (int)souls.size(); ++i) if (glm::length(souls[i].pos - p) < r) return i;
        return -1;
    }
    void removeSoul(int i) { if (i >= 0 && i < (int)souls.size()) souls.erase(souls.begin() + i); }
    // Fly; returns the souls that reached their spot this tick (removed here: the game re-forms them)
    std::vector<Soul> arrived(float dt) {
        std::vector<Soul> out;
        for (auto& s : souls) {
            s.t += dt;
            float u = std::min(1.f, s.t / SOUL_TIME);
            float e = u * u * (3.f - 2.f * u);
            s.pos = glm::mix(s.from, s.to, e) + glm::vec3{0.f, std::sin(u * 3.1415927f) * SOUL_ARC, 0.f};
            if (u >= 1.f) out.push_back(s);
        }
        souls.erase(std::remove_if(souls.begin(), souls.end(), [](const Soul& s) { return s.t >= SOUL_TIME; }), souls.end());
        return out;
    }
    static constexpr float WIRE_HEIGHT = 1.2f, WIRE_LEN = 10.f, NODE_RADIUS = 0.35f, SNARE_TIME = 1.2f, SNARE_DAMAGE = 10.f, WIRE_ARM = 0.1f;
    static constexpr int   MAX_WIRES = 4;
    std::vector<Wire> wires;

    void addWire(int owner, glm::vec3 a, glm::vec3 b) {
        int n = 0, oldest = -1;
        for (int i = 0; i < (int)wires.size(); ++i) if (wires[i].owner == owner) { ++n; if (oldest < 0) oldest = i; }
        if (n >= MAX_WIRES && oldest >= 0) wires.erase(wires.begin() + oldest);
        wires.push_back({owner, a, b, 0.f});
    }
    // Does the player's body (feet up to feet+height, this thick) cross the wire?
    static bool wireTouches(const Wire& w, glm::vec3 feet, float height, float radius) {
        if (w.a.y < feet.y + 0.05f || w.a.y > feet.y + height) return false;   // under it (sliding) or over it (jumping)
        glm::vec2 a{w.a.x, w.a.z}, b{w.b.x, w.b.z}, p{feet.x, feet.z};
        glm::vec2 ab = b - a;
        float u = glm::clamp(glm::dot(p - a, ab) / std::max(glm::dot(ab, ab), 1e-6f), 0.f, 1.f);
        return glm::length(p - (a + ab * u)) < radius + 0.08f;
    }
    int touchedWire(glm::vec3 feet, float height, float radius) const {
        for (int i = 0; i < (int)wires.size(); ++i) if (wires[i].age >= WIRE_ARM && wireTouches(wires[i], feet, height, radius)) return i;
        return -1;
    }
    int rayNode(glm::vec3 o, glm::vec3 d, float maxT, float& t) const {
        int best = -1; float bt = maxT;
        for (int i = 0; i < (int)wires.size(); ++i)
            for (glm::vec3 n : {wires[i].a, wires[i].b}) {
                glm::vec3 oc = n - o; float along = glm::dot(oc, d);
                if (along < 0.f || along > bt) continue;
                if (glm::length(oc - d * along) < NODE_RADIUS) { bt = along; best = i; }
            }
        t = bt; return best;
    }
    void cutWire(int i) { if (i >= 0 && i < (int)wires.size()) wires.erase(wires.begin() + i); }
    void dropOwner(int owner) { wires.erase(std::remove_if(wires.begin(), wires.end(), [owner](const Wire& w) { return w.owner == owner; }), wires.end()); }
    void clearWires() { wires.clear(); }
    void age(float dt) { for (auto& w : wires) w.age += dt; }
    int  alive() const { return (int)souls.size(); }   // souls in flight: the wave isn't over until they're caught or back
    void clear() { souls.clear(); wires.clear(); }
};
