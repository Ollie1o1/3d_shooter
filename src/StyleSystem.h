#pragma once
#include <algorithm>
#include <string>
#include <cmath>

enum class StyleRank { D, C, B, A, S, SSS };

class StyleSystem {
public:
    float health    = 100.f;
    float maxHealth = 100.f;
    float style     = 0.f;
    float maxStyle  = 100.f;
    bool  overdrive = false;
    float overdriveTimer = 0.f;
    static constexpr float OVERDRIVE_DURATION = 5.f;

    float idleTimer = 0.f; // time since last style action

    void update(float dt) {
        if (overdrive) {
            overdriveTimer -= dt;
            if (overdriveTimer <= 0.f) {
                overdrive = false;
                overdriveTimer = 0.f;
            }
        }
        idleTimer += dt;
        if (idleTimer > 3.f) {
            style = std::max(0.f, style - 8.f * dt);
        }
    }

    void addStyle(float amount) {
        idleTimer = 0.f;
        style = std::min(maxStyle, style + amount);
        if (style >= maxStyle && !overdrive) {
            overdrive = true;
            overdriveTimer = OVERDRIVE_DURATION;
        }
    }

    void takeDamage(float amount) {
        health = std::max(0.f, health - amount);
        style  = std::max(0.f, style - 20.f);
        idleTimer = 0.f;
    }

    void heal(float amount) {
        health = std::min(maxHealth, health + amount);
    }

    StyleRank getRank() const {
        if (style < 15.f)  return StyleRank::D;
        if (style < 30.f)  return StyleRank::C;
        if (style < 50.f)  return StyleRank::B;
        if (style < 70.f)  return StyleRank::A;
        if (style < 90.f)  return StyleRank::S;
        return StyleRank::SSS;
    }

    bool isAlive() const { return health > 0.f; }
};
