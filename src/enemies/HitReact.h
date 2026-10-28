#pragma once
// =============================================================================
// HitReact.h — how the regular enemies take a hit (no OpenGL, no behaviour):
//
//   MASS   how far a hit shoves it. A gun's push (m/s on a 1-mass body) over
//          its mass is the shove; heavies barely rock, light ones get thrown.
//   POISE  how much damage, inside about a second, breaks its footing. Past
//          that it's STUNNED for a beat: whatever it was winding up is lost,
//          and then it can't be stunned again for a moment (no stunlock).
//   FINISH how low it has to be (or stunned) before a punch finishes it.
//
// Bosses, the Conduit pylons and the Leviathan don't react: they have their
// own answers (parries, staggers, weak points).
// =============================================================================
#include "enemies/EnemyTypes.h"
#include <algorithm>
#include <cmath>

namespace hitreact {

struct Body { float mass, poise, stun; bool heavy; };

inline const Body& bodyOf(EnemyType t) {
    static const Body INF{1e9f, 1e9f, 0.f, true};
    static const Body B[] = {
        {1.0f,   38.f, 0.40f, false},   // HUSK: a magnum round stuns it
        {0.8f,   30.f, 0.40f, false},   // RIPPER: shoot it mid-lunge and the lunge is lost
        {1.3f,   50.f, 0.45f, false},   // SENTINEL
        {0.7f,   30.f, 0.45f, false},   // RAPTOR: knocked out of its dive
        {4.5f,  170.f, 0.70f, true },   // BRUTE: two shotgun blasts (or a rifle headshot) break the slam
        {0.5f,  1e9f, 0.f,   false},    // MITE: nothing to stun; it pops
        {9.0f,  1e9f, 0.f,   true },    // JUGGERNAUT: parry its smash instead
        INF,                            // WARDEN
        INF,                            // SOVEREIGN
        {3.0f,  110.f, 0.60f, true },   // SHIELDBEARER (from the flank or over the rim)
        INF,                            // CONDUIT: a pylon
        {1.2f,   60.f, 0.45f, false},   // CONDUCTOR
        {1.6f,   70.f, 0.50f, false},   // SERAPH
        {6.0f,  150.f, 0.70f, true },   // ANCHOR
        INF,                            // PENITENT
        {1.4f,   70.f, 0.45f, false},   // REVENANT
        {1.6f,   55.f, 0.45f, false},   // WEAVER
        INF,                            // LEVIATHAN
        INF,                            // THE ABSOLUTE
        // The Gauntlet's mini-bosses: heavy (the hook takes you to them), hard to break
        {3.0f,  240.f, 0.60f, true },   // THE COURIER
        {12.f,  1e9f, 0.f,   true },    // THE BULWARK: parry its bash instead
        {4.0f,  280.f, 0.60f, true },   // THE CANTOR
        {3.5f,  220.f, 0.70f, true },   // THE HARRIER
        INF,                            // THE PLUMB LINE: a hanging weight
        {3.5f,  240.f, 0.60f, true },   // THE FOREMAN
    };
    int i = (int)t;
    if (isBoss(t) || i < 0 || i >= (int)(sizeof(B) / sizeof(B[0]))) return INF;
    return B[i];
}

inline bool reacts(EnemyType t) { return !isBoss(t) && t != EnemyType::CONDUIT && t != EnemyType::LEVIATHAN && t != EnemyType::PLUMB; }

// Poise drains this fraction of its bar a second: hits have to come close together
constexpr float POISE_DRAIN = 0.55f;
// After a stun ends: this long before it can be stunned again
constexpr float STUN_GUARD  = 0.9f;
// The visual flinch (radians): how much a shove tips it, a headshot's extra, the most it tips
constexpr float FLINCH_PER_PUSH = 0.034f, FLINCH_HEAD = 0.12f, FLINCH_MAX = 0.42f;
constexpr float FLINCH_TAU = 0.07f;
// A boss doesn't stagger to your gun, but it feels every hit: a smaller flinch, capped low (its tells stay readable)
constexpr float BOSS_FLINCH_PER_PUSH = 0.004f, BOSS_FLINCH_HEAD = 0.02f, BOSS_FLINCH_MAX = 0.08f;
inline bool bossFlinches(EnemyType t) { return isBoss(t) && t != EnemyType::LEVIATHAN; }   // seconds: it snaps over on the hit and settles back critically damped

// How far over it is, `t` seconds after a flinch of `amp`: the snap, then the settle
inline float flinchAt(float amp, float t) {
    if (t < 0.f || amp <= 0.f) return 0.f;
    float u = t / FLINCH_TAU;
    if (u > 12.f) return 0.f;
    return amp * std::exp(-u) * (1.f + u);
}

// Under this share of its health a punch finishes it (heavies a little lower);
// stunned, under 60 %; in OVERDRIVE, under half
inline float finishShare(EnemyType t, bool stunned, bool overdrive) {
    float s = bodyOf(t).heavy ? 0.25f : 0.35f;
    if (stunned) s = std::max(s, 0.6f);
    if (overdrive) s = std::max(s, 0.5f);
    return s;
}

// Each gun's push per round (m/s on a 1-mass body) and how hard it breaks poise
inline float gunPush(int weapon) {
    static const float P[4] = {4.4f, 1.7f, 7.5f, 14.f};   // revolver, shotgun pellet, Lancer, Longshot
    return P[weapon < 0 ? 0 : weapon > 3 ? 3 : weapon];
}
inline float gunPoise(int weapon) {
    static const float P[4] = {1.f, 1.3f, 1.f, 1.f};      // the shotgun's blast staggers more than its damage
    return P[weapon < 0 ? 0 : weapon > 3 ? 3 : weapon];
}

}   // namespace hitreact
