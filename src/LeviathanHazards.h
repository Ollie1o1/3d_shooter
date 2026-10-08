#pragma once
// =============================================================================
// LeviathanHazards.h — what THE LEVIATHAN does to the ground round you: the
// strip its crash comes down along (marked, then struck), the tide its tail
// rolls over the ring (jump it), the spit that bursts where you'll be and
// burns there, the well that boils before it breaches, and the swallow's
// pull and bite. No OpenGL or audio: GameplayState feeds it the Leviathan's
// events and turns hits into damage (Gameplay_Leviathan.h).
// =============================================================================
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

class LeviathanHazards {
public:
    static constexpr float STRIP_HALF = 2.5f, STRIP_PAST = 4.f, CRASH_HIGH = 2.5f, CRASH_DAMAGE = 40.f, STRIP_FADE = 0.6f;
    static constexpr float TIDE_SPEED = 14.f, TIDE_HEIGHT = 1.2f, TIDE_DAMAGE = 25.f;
    static constexpr float SPIT_WARN = 1.f, SPIT_RADIUS = 4.f, SPIT_DAMAGE = 30.f, SPIT_BURN = 4.f, SPIT_DPS = 15.f, SPIT_TICK = 0.25f;
    static constexpr float BOIL_TIME = 1.2f, BREACH_RADIUS = 6.f, BREACH_HIGH = 6.f, BREACH_DAMAGE = 40.f;
    static constexpr float PULL_SPEED = 3.5f, BITE_REACH = 4.5f, BITE_DAMAGE = 45.f;
    struct Strip { glm::vec3 from, to; float t = 0.f; bool landed = false; };   // marked, then struck (and fading)
    struct Tide  { glm::vec3 centre; float radius, prev, maxR; bool hit = false; };
    struct Spit  { glm::vec3 at; float t = 0.f; bool burst = false; };          // a marker, a burst, then it burns
    struct Boil  { glm::vec3 at; float t = 0.f; };
    struct Hit   { float damage; glm::vec3 from; };
    std::vector<Strip> strips; std::vector<Tide> tides; std::vector<Spit> spits; std::vector<Boil> boils;

    // Is a player standing at `feet` in the strip a crash from `from` to `to`
    // comes down along? The ground band (CRASH_HIGH over the floor), from its
    // root out past where its head lands
    static bool inStrip(glm::vec3 from, glm::vec3 to, glm::vec3 feet, float floorY) {
        glm::vec2 d{to.x - from.x, to.z - from.z};
        float len = glm::length(d);
        if (len < 1e-3f) return false;
        d /= len;
        glm::vec2 r{feet.x - from.x, feet.z - from.z};
        float along = glm::dot(r, d), side = std::fabs(r.x * d.y - r.y * d.x);
        return along >= 0.f && along <= len + STRIP_PAST && side <= STRIP_HALF && feet.y < floorY + CRASH_HIGH;
    }
    static bool breachHits(glm::vec3 at, glm::vec3 feet) {
        return glm::length(glm::vec2{feet.x - at.x, feet.z - at.z}) < BREACH_RADIUS && feet.y < at.y + BREACH_HIGH && feet.y > at.y - 2.f;
    }
    // The swallow: how it drags you (a velocity, along the ground, toward its
    // mouth), and whether you've reached the mouth
    static glm::vec3 pull(glm::vec3 mouth, glm::vec3 feet) {
        glm::vec3 d{mouth.x - feet.x, 0.f, mouth.z - feet.z};
        float l = glm::length(d);
        return l > 0.2f ? d / l * PULL_SPEED : glm::vec3{0.f};
    }
    static bool bites(glm::vec3 mouth, glm::vec3 feet) {
        return glm::length(glm::vec2{feet.x - mouth.x, feet.z - mouth.z}) < BITE_REACH && std::fabs(feet.y - mouth.y) < 4.f;
    }

    void markCrash(glm::vec3 from, glm::vec3 to) { strips.push_back({from, to}); }
    // The crash lands along its marked strip (the newest unlanded one)
    void landCrash() {
        for (auto it = strips.rbegin(); it != strips.rend(); ++it)
            if (!it->landed) { it->landed = true; it->t = 0.f; return; }
    }
    void dropMarks() {   // a crash that never came (parried, choked, phase change): its mark goes
        strips.erase(std::remove_if(strips.begin(), strips.end(), [](const Strip& s) { return !s.landed; }), strips.end());
    }
    void addTide(glm::vec3 centre, float startR, float maxR) { tides.push_back({centre, startR, startR, maxR}); }
    void addSpit(glm::vec3 at) { spits.push_back({at}); }
    void addBoil(glm::vec3 at) { boils.push_back({at}); }
    void clear() { strips.clear(); tides.clear(); spits.clear(); boils.clear(); burnTick = 0.f; }

    // Tides passing you, spits bursting and burning; the rest only fades
    std::vector<Hit> update(float dt, glm::vec3 feet, float floorY) {
        std::vector<Hit> hits;
        for (auto& s : strips) if (s.landed) s.t += dt;
        strips.erase(std::remove_if(strips.begin(), strips.end(), [](const Strip& s) { return s.landed && s.t > STRIP_FADE; }), strips.end());
        const float up = feet.y - floorY;
        for (auto& t : tides) {
            t.prev = t.radius; t.radius += TIDE_SPEED * dt;
            float d = glm::length(glm::vec2{feet.x - t.centre.x, feet.z - t.centre.z});
            if (!t.hit && up < TIDE_HEIGHT && d >= t.prev && d < t.radius) { t.hit = true; hits.push_back({TIDE_DAMAGE, t.centre}); }
        }
        tides.erase(std::remove_if(tides.begin(), tides.end(), [](const Tide& t) { return t.radius >= t.maxR; }), tides.end());
        bool burning = false;
        for (auto& s : spits) {
            s.t += dt;
            float d = glm::length(glm::vec2{feet.x - s.at.x, feet.z - s.at.z});
            bool there = d < SPIT_RADIUS && std::fabs(feet.y - s.at.y) < 2.5f;
            if (!s.burst && s.t >= SPIT_WARN) { s.burst = true; if (there) hits.push_back({SPIT_DAMAGE, s.at}); }
            else if (s.burst && there && s.t > SPIT_WARN + 0.25f) burning = true;
        }
        spits.erase(std::remove_if(spits.begin(), spits.end(), [](const Spit& s) { return s.t > SPIT_WARN + SPIT_BURN; }), spits.end());
        if (burning) {
            burnTick += dt;
            while (burnTick >= SPIT_TICK) { burnTick -= SPIT_TICK; hits.push_back({SPIT_DPS * SPIT_TICK, feet}); }
        } else burnTick = 0.f;
        for (auto& b : boils) b.t += dt;
        boils.erase(std::remove_if(boils.begin(), boils.end(), [](const Boil& b) { return b.t > BOIL_TIME + 0.6f; }), boils.end());
        return hits;
    }
private:
    float burnTick = 0.f;
};
