#pragma once
// =============================================================================
// SovereignHazards.h — what the SOVEREIGN leaves in the Sanctum besides his
// sword: the blades he calls down (JUDGMENT), the eruptions that run along
// the ground toward you (RUPTURE), the phantoms of himself that dash in from
// the sides, and the last stand's burning edge. No OpenGL or audio:
// GameplayState feeds it his events, draws it and turns its hits into damage
// (Gameplay_Sovereign.h).
// =============================================================================
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

class SovereignHazards {
public:
    // A blade falling from the sky (kind 0) or an eruption out of the floor
    // (kind 1). Marked on the ground for `delay` seconds, then it lands.
    struct Strike {
        glm::vec3 pos;      // on the ground it lands on
        int   kind;
        float delay, total;
        float after = 0.f;  // seconds since it landed (it lingers, fading)
        bool  landed = false;
    };
    // A phantom: glows where it stands for WAIT seconds, then dashes straight
    // through for RUN seconds and is gone
    struct Phantom {
        glm::vec3 pos, dir;
        float t = 0.f;
        bool  hit = false;
    };
    struct Hit { glm::vec3 from; int kind; float damage; };   // kind 2: a phantom

    static constexpr float BLADE_RADIUS = 2.3f, BLADE_DAMAGE = 22.f;
    static constexpr float ERUPT_RADIUS = 1.7f, ERUPT_DAMAGE = 16.f, ERUPT_HEIGHT = 1.4f;
    static constexpr float LINGER = 0.6f;
    static constexpr float PHANTOM_WAIT = 0.55f, PHANTOM_RUN = 0.75f, PHANTOM_SPEED = 34.f, PHANTOM_DAMAGE = 16.f;
    // The last stand: past this far from the seal (or above this height)
    // the Sanctum burns
    static constexpr float SAFE_HALF = 27.f, SAFE_HEIGHT = 8.f, BURN_DPS = 26.f;

    std::vector<Strike>  strikes;
    std::vector<Phantom> phantoms;

    void clear() { strikes.clear(); phantoms.clear(); }

    void addStrike(glm::vec3 pos, int kind, float delay) { strikes.push_back({pos, kind, delay, delay}); }
    void addPhantom(glm::vec3 pos, glm::vec3 dir) { phantoms.push_back({pos, dir}); }

    static glm::vec3 phantomAt(const Phantom& p) {
        float run = std::max(0.f, p.t - PHANTOM_WAIT);
        return p.pos + p.dir * (run * PHANTOM_SPEED);
    }

    // Advance everything; returns what hit the player (feet at `feet`) this tick
    const std::vector<Hit>& update(float dt, glm::vec3 feet) {
        hits.clear();
        for (auto& s : strikes) {
            if (s.landed) { s.after += dt; continue; }
            s.delay -= dt;
            if (s.delay > 0.f) continue;
            s.landed = true;
            landedNow.push_back(s);
            float flat = glm::length(glm::vec2(feet.x - s.pos.x, feet.z - s.pos.z));
            float up = feet.y - s.pos.y;
            bool hit = s.kind == 0 ? flat < BLADE_RADIUS && std::fabs(up) < 2.5f
                                   : flat < ERUPT_RADIUS && up > -0.6f && up < ERUPT_HEIGHT;
            if (hit) hits.push_back({s.pos, s.kind, s.kind == 0 ? BLADE_DAMAGE : ERUPT_DAMAGE});
        }
        for (size_t i = 0; i < strikes.size();)
            if (strikes[i].landed && strikes[i].after > LINGER) strikes.erase(strikes.begin() + i); else ++i;

        for (auto& p : phantoms) {
            p.t += dt;
            if (p.hit || p.t < PHANTOM_WAIT) continue;
            glm::vec3 at = phantomAt(p);
            if (glm::length(glm::vec2(feet.x - at.x, feet.z - at.z)) < 2.4f && std::fabs(feet.y - at.y) < 2.6f) {
                p.hit = true;
                hits.push_back({at, 2, PHANTOM_DAMAGE});
            }
        }
        for (size_t i = 0; i < phantoms.size();)
            if (phantoms[i].t > PHANTOM_WAIT + PHANTOM_RUN) phantoms.erase(phantoms.begin() + i); else ++i;
        return hits;
    }

    // Strikes that landed since the last call (for their sound and sparks)
    std::vector<Strike> takeLanded() { std::vector<Strike> out; out.swap(landedNow); return out; }

    static bool burns(glm::vec3 p, glm::vec3 centre) {
        return std::fabs(p.x - centre.x) > SAFE_HALF || std::fabs(p.z - centre.z) > SAFE_HALF || p.y > SAFE_HEIGHT;
    }

private:
    std::vector<Hit>    hits;
    std::vector<Strike> landedNow;
};
