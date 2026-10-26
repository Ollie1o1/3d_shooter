#pragma once
#include <algorithm>
#include <string>
#include <cmath>

enum class StyleRank { D, C, B, A, S, SSS };

// What earned some style. Scoring with the same thing over and over earns
// less: each source wears out as it scores and recovers while you use the
// others (and slowly on its own). Movement (dashes, grapples, slides) is
// NONE: never worn, never boosted.
enum class StyleSource { NONE, REVOLVER, SHOTGUN, KAR, LONGSHOT, PUNCH, PARRY, EXPLOSIVE, SLAM,
                         ENVIRONMENT,   // lava, the void
                         FRIENDLY,      // enemies hurting each other
                         MELEE,         // the sword (Gameplay_Sword.h)
                         COUNT };
enum class Freshness { FRESH, USED, STALE, DULL };

class StyleSystem {
public:
    float health    = 100.f;
    float maxHealth = 100.f;
    float style     = 0.f;
    float maxStyle  = 100.f;
    // OVERDRIVE: a full meter. The game makes it a power state (Gameplay_Feel.h);
    // kills keep it going a little longer, and when it burns out the meter
    // drops back to A, so it has to be earned again
    bool  overdrive = false;
    float overdriveTimer = 0.f;
    static constexpr float OVERDRIVE_DURATION = 6.f;
    static constexpr float OVERDRIVE_MAX      = 8.f;    // what kills can stretch it to
    static constexpr float OVERDRIVE_AFTER    = 40.f;   // where the meter lands when it burns out
    static constexpr float OVERDRIVE_COOL     = 15.f;   // after it burns out, this long before it can start again
    // The meter fills at this share of what's scored (the run's score, `earned`, takes it all):
    // a few good kills in a row, not a few hits, fill it
    static constexpr float METER_GAIN = 0.45f;
    float driveCool = 0.f;
    bool  overdriveBegan = false, overdriveEnded = false;   // set as it starts / ends; the game takes them
    void  extendOverdrive(float s) { if (overdrive) overdriveTimer = std::min(OVERDRIVE_MAX, overdriveTimer + s); }

    float idleTimer = 0.f; // time since last style action

    // For the run's score (Score.h): all the style scored (after freshness),
    // and all the damage taken
    float earned = 0.f;
    float damageTaken = 0.f;

    // 1 fresh .. 0 dull, per source
    float fresh[(int)StyleSource::COUNT];
    static constexpr float WEAR    = 0.0035f;  // freshness lost per style point scored
    static constexpr float SHARE   = 0.25f;    // the others regain this much of what one lost
    static constexpr float RECOVER = 0.05f;    // per second, every source
    // Kills you only set up (lava, the void, enemies hurting each other) can
    // carry the meter this far (well into rank B) but no further: the top ranks and
    // OVERDRIVE are for what you do yourself
    static constexpr float PASSIVE_CAP = 45.f;

    StyleSystem() { resetFreshness(); }
    void resetFreshness() { for (float& f : fresh) f = 1.f; }

    Freshness freshness(StyleSource s) const {
        float f = fresh[(int)s];
        return f > 0.7f ? Freshness::FRESH : f > 0.4f ? Freshness::USED : f > 0.15f ? Freshness::STALE : Freshness::DULL;
    }
    static float freshnessMult(Freshness f) {
        switch (f) {
            case Freshness::FRESH: return 1.5f;
            case Freshness::USED:  return 1.f;
            case Freshness::STALE: return 0.5f;
            default:               return 0.2f;
        }
    }
    static const char* freshnessName(Freshness f) {
        static const char* N[] = {"FRESH", "USED", "STALE", "DULL"};
        return N[(int)f];
    }

    void update(float dt) {
        if (overdrive) {
            overdriveTimer -= dt;
            if (overdriveTimer <= 0.f) {
                overdrive = false;
                overdriveTimer = 0.f;
                overdriveEnded = true;
                style = std::min(style, OVERDRIVE_AFTER);
                driveCool = OVERDRIVE_COOL;
            }
        }
        if (driveCool > 0.f) driveCool = std::max(0.f, driveCool - dt);
        if (style >= maxStyle && !overdrive && driveCool <= 0.f) startOverdrive();   // (cooled down with the meter still full)
        for (float& f : fresh) f = std::min(1.f, f + RECOVER * dt);
        idleTimer += dt;
        if (idleTimer > 3.f) {
            style = std::max(0.f, style - 8.f * dt);
        }
    }

    // Returns the style actually scored, after freshness
    float addStyle(float amount, StyleSource src = StyleSource::NONE) {
        if (src != StyleSource::NONE) {
            float worn = amount * WEAR;
            amount *= freshnessMult(freshness(src));
            if (src == StyleSource::FRIENDLY || src == StyleSource::ENVIRONMENT)
                amount = std::min(amount, std::max(0.f, PASSIVE_CAP - style) / METER_GAIN);
            for (int i = 1; i < (int)StyleSource::COUNT; ++i)
                fresh[i] = i == (int)src ? std::max(0.f, fresh[i] - worn) : std::min(1.f, fresh[i] + worn * SHARE);
        }
        idleTimer = 0.f;
        earned += amount;
        style = std::min(maxStyle, style + amount * METER_GAIN);
        if (style >= maxStyle && !overdrive && driveCool <= 0.f) startOverdrive();
        return amount;
    }

    void startOverdrive() { overdrive = true; overdriveTimer = OVERDRIVE_DURATION; overdriveBegan = true; }

    void takeDamage(float amount) {
        damageTaken += std::min(amount, health);
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
