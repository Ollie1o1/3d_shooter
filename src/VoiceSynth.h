#pragma once
// =============================================================================
// VoiceSynth.h — every enemy voice, built in code when the game loads: the
// feel track's "machine + choir", a machine body carrying a hollow voice.
// No files: like MusicSynth and TextureGen, the sound is the recipe.
//
// Blocks: a formant CHOIR (detuned saws through a vowel's three formants),
// SERVO whine, VENT hiss, gear GRIND, METAL ring (damped inharmonic
// partials), CLICK, THUD, HUM, WINGS, SHIMMER. Each enemy type has a profile
// (its choir's pitch and vowel, its body, its size: small is high and quick,
// heavy is low and slow); each kind of voice a recipe over that profile; each
// attack its own tell. Built at RATE, normalised to its mix class's level
// (MixTable.h), deterministic (seeded by name and variant).
// =============================================================================
#include "VoiceTable.h"
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace VoiceSynth {

constexpr float RATE = 22050.f;
constexpr float TAU  = 6.2831853f;
using Buf = std::vector<float>;

enum class Vowel : uint8_t { NONE, AH, OH, OO, EE, UH, EH };
enum class Body  : uint8_t { SERVO, SKITTER, HYDRAULIC, WINGS, HEAVY, TREADS, SCRAPE, CRACKLE, HUM, GRIND, GEARS, ARMOUR, CHAINS };

struct Profile {
    float f0 = 130.f;          // choir pitch, Hz (0: a cold machine, no choir)
    Vowel vowel = Vowel::OH;
    int   voices = 3;          // 1 a solo voice .. 6 a massed choir
    float formant = 1.f;       // formant shift (>1 smaller and brighter)
    Body  body = Body::SERVO;
    float bodyHz = 400.f;      // the machine layer's pitch
    float size = 0.3f;         // 0 small .. 1 colossal
};

inline Profile profileOf(EnemyType t) {
    switch (t) {
        case EnemyType::HUSK:         return {130.f, Vowel::OH,   3, 1.00f, Body::SERVO,      420.f, 0.30f};
        case EnemyType::RIPPER:       return {260.f, Vowel::AH,   2, 1.20f, Body::SKITTER,   1800.f, 0.15f};
        case EnemyType::SENTINEL:     return {  0.f, Vowel::NONE, 0, 1.00f, Body::HYDRAULIC,  300.f, 0.40f};
        case EnemyType::RAPTOR:       return {520.f, Vowel::EE,   2, 1.25f, Body::WINGS,      220.f, 0.20f};
        case EnemyType::BRUTE:        return { 70.f, Vowel::UH,   4, 0.85f, Body::HEAVY,      140.f, 0.70f};
        case EnemyType::MITE:         return {  0.f, Vowel::NONE, 0, 1.00f, Body::SKITTER,   2600.f, 0.05f};
        case EnemyType::JUGGERNAUT:   return { 55.f, Vowel::OO,   4, 0.80f, Body::TREADS,      90.f, 0.85f};
        case EnemyType::WARDEN:       return { 65.f, Vowel::OH,   6, 0.85f, Body::GEARS,      110.f, 0.90f};
        case EnemyType::SOVEREIGN:    return {110.f, Vowel::AH,   1, 0.95f, Body::ARMOUR,     900.f, 0.60f};
        case EnemyType::SHIELDBEARER: return { 98.f, Vowel::AH,   4, 0.95f, Body::SCRAPE,     600.f, 0.50f};
        case EnemyType::CONDUIT:      return {147.f, Vowel::AH,   5, 1.00f, Body::CRACKLE,   3000.f, 0.60f};
        case EnemyType::CONDUCTOR:    return {392.f, Vowel::AH,   3, 1.15f, Body::HUM,        480.f, 0.30f};
        case EnemyType::SERAPH:       return {440.f, Vowel::AH,   4, 1.20f, Body::WINGS,      180.f, 0.40f};
        case EnemyType::ANCHOR:       return { 49.f, Vowel::OO,   3, 0.80f, Body::GRIND,      120.f, 0.75f};
        case EnemyType::PENITENT:     return { 73.f, Vowel::OH,   6, 0.80f, Body::CHAINS,     160.f, 1.00f};
        case EnemyType::REVENANT:     return {165.f, Vowel::OO,   3, 1.05f, Body::ARMOUR,     700.f, 0.35f};
        case EnemyType::WEAVER:       return {  0.f, Vowel::NONE, 0, 1.00f, Body::SKITTER,   1600.f, 0.30f};
        case EnemyType::LEVIATHAN:    return { 41.f, Vowel::OO,   6, 0.70f, Body::GRIND,       70.f, 1.00f};   // a whale's groan through grinding plates
        default:                      return {};
    }
}

// ---- plumbing ------------------------------------------------------------------
struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed) : s(seed ? seed : 1u) {}
    float uni() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (s & 0xFFFFFF) / 16777215.f; }
    float bi()  { return uni() * 2.f - 1.f; }
};
inline uint32_t seedOf(const std::string& name, int variant) {
    uint32_t h = 2166136261u;
    for (char c : name) { h ^= (uint8_t)c; h *= 16777619u; }
    h ^= (uint32_t)(variant + 1) * 0x9E3779B9u;
    return h ? h : 1u;
}
inline int N(float sec) { return std::max(1, (int)(sec * RATE)); }
// Mix b into a, `at` seconds in, scaled by g (a grows to fit)
inline void add(Buf& a, const Buf& b, float g = 1.f, float at = 0.f) {
    const size_t o = (size_t)std::max(0, (int)(at * RATE));
    if (a.size() < o + b.size()) a.resize(o + b.size(), 0.f);
    for (size_t i = 0; i < b.size(); ++i) a[o + i] += g * b[i];
}
// Linear attack, squared release
inline float env(int i, int n, float atk, float rel) {
    const float t = (float)i / RATE, left = (float)(n - i) / RATE;
    const float a = atk > 0.f ? std::min(1.f, t / atk) : 1.f;
    const float r = rel > 0.f ? std::min(1.f, left / rel) : 1.f;
    return a * r * r;
}
struct BandPass {
    float b0 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f, x1 = 0.f, x2 = 0.f, y1 = 0.f, y2 = 0.f;
    BandPass(float hz, float q) {
        const float w = TAU * std::min(hz, RATE * 0.45f) / RATE, al = std::sin(w) / (2.f * q), a0 = 1.f + al;
        b0 = al / a0; b2 = -al / a0; a1 = -2.f * std::cos(w) / a0; a2 = (1.f - al) / a0;
    }
    float operator()(float x) { float y = b0 * x + b2 * x2 - a1 * y1 - a2 * y2; x2 = x1; x1 = x; y2 = y1; y1 = y; return y; }
};
struct LowPass {
    float k, z = 0.f;
    explicit LowPass(float hz) : k(1.f - std::exp(-TAU * hz / RATE)) {}
    float operator()(float x) { z += k * (x - z); return z; }
};

// ---- blocks --------------------------------------------------------------------
inline const float* formantsOf(Vowel v) {
    static const float F[7][3] = {{0, 0, 0}, {730, 1090, 2440}, {570, 840, 2410}, {300, 870, 2240},
                                  {270, 2290, 3010}, {520, 1190, 2390}, {530, 1840, 2480}};
    return F[(int)v];
}
// `voices` detuned saws gliding f0 -> f1 through the vowel's formants
inline Buf choir(float dur, float f0, float f1, Vowel v, int voices, float shift, Rng& r,
                 float atk = 0.05f, float rel = 0.2f, float breath = 0.1f, float vib = 5.f) {
    const int n = N(dur);
    Buf out(n, 0.f);
    if (v == Vowel::NONE || voices <= 0 || f0 <= 0.f) return out;
    const float* F = formantsOf(v);
    BandPass b1(F[0] * shift, 5.f), b2(F[1] * shift, 7.f), b3(F[2] * shift, 9.f);
    std::vector<float> ph(voices), det(voices);
    for (int k = 0; k < voices; ++k) {
        ph[k] = r.uni();
        det[k] = 1.f + (voices > 1 ? 0.012f * (2.f * k / (voices - 1) - 1.f) : 0.f) + 0.003f * r.bi();
    }
    const float vibPh = r.uni() * TAU, ratio = f1 / f0, norm = 1.f / std::sqrt((float)voices);
    for (int i = 0; i < n; ++i) {
        const float f = f0 * std::pow(ratio, (float)i / n) * (1.f + 0.012f * std::sin(vibPh + TAU * vib * i / RATE));
        float src = 0.f;
        for (int k = 0; k < voices; ++k) { ph[k] += f * det[k] / RATE; ph[k] -= std::floor(ph[k]); src += 2.f * ph[k] - 1.f; }
        src = src * norm + breath * r.bi();
        out[i] = (b1(src) + 0.5f * b2(src) + 0.25f * b3(src)) * env(i, n, atk, rel);
    }
    return out;
}
inline Buf servo(float dur, float f0, float f1, Rng& r, float atk = 0.02f, float rel = 0.08f) {
    const int n = N(dur); Buf out(n); float ph = 0.f; LowPass lp(4000.f);
    for (int i = 0; i < n; ++i) {
        const float f = f0 + (f1 - f0) * (float)i / n;
        ph += f / RATE; ph -= std::floor(ph);
        out[i] = lp(0.6f * (ph < 0.5f ? 1.f : -1.f) + 0.4f * (2.f * ph - 1.f) + 0.15f * r.bi()) * env(i, n, atk, rel);
    }
    return out;
}
inline Buf vent(float dur, float hz, Rng& r, float atk = 0.01f, float rel = 0.15f) {
    const int n = N(dur); Buf out(n); BandPass bp(hz, 0.8f);
    for (int i = 0; i < n; ++i) out[i] = bp(r.bi()) * env(i, n, atk, rel);
    return out;
}
// Gear teeth: noise chopped `rate` times a second, rung at hz
inline Buf grind(float dur, float rate, float hz, Rng& r) {
    const int n = N(dur); Buf out(n); BandPass bp(hz, 2.f); float ph = 0.f;
    for (int i = 0; i < n; ++i) {
        ph += rate / RATE; ph -= std::floor(ph);
        out[i] = bp(r.bi() * (ph < 0.3f ? 1.f : 0.15f)) * 2.f * env(i, n, 0.03f, 0.1f);
    }
    return out;
}
inline Buf metal(float dur, float hz, Rng& r, float decay = 6.f) {
    static const float RT[] = {1.f, 2.76f, 5.40f, 8.93f}, GN[] = {1.f, 0.6f, 0.35f, 0.2f};
    const int n = N(dur); Buf out(n, 0.f);
    for (int p = 0; p < 4; ++p) {
        const float f = hz * RT[p] * (1.f + 0.004f * r.bi());
        if (f > RATE * 0.45f) continue;
        const float ph = r.uni() * TAU;
        for (int i = 0; i < n; ++i) { float t = (float)i / RATE; out[i] += GN[p] * std::sin(ph + TAU * f * t) * std::exp(-t * decay * (1.f + p * 0.5f)); }
    }
    for (int i = 0; i < n && i < 20; ++i) out[i] *= i / 20.f;
    return out;
}
inline Buf click(float hz, Rng& r, float dur = 0.03f) {
    const int n = N(dur); Buf out(n); BandPass bp(hz, 3.f);
    for (int i = 0; i < n; ++i) { float t = (float)i / n; out[i] = bp(r.bi()) * 3.f * (1.f - t) * (1.f - t); }
    return out;
}
inline Buf thud(float f0, float f1, float dur) {
    const int n = N(dur); Buf out(n); float ph = 0.f;
    for (int i = 0; i < n; ++i) {
        const float t = (float)i / RATE, f = f1 + (f0 - f1) * std::exp(-t * 18.f);
        ph += TAU * f / RATE;
        out[i] = std::sin(ph) * std::exp(-t * 6.f / dur) * std::min(1.f, i / 30.f);
    }
    return out;
}
inline Buf beep(float hz, float dur) {
    const int n = N(dur); Buf out(n);
    for (int i = 0; i < n; ++i) out[i] = std::sin(TAU * hz * i / RATE) * env(i, n, 0.002f, 0.01f);
    return out;
}
inline Buf hum(float dur, float hz, Rng& r) {
    const int n = N(dur); Buf out(n); const float ph = r.uni() * TAU;
    for (int i = 0; i < n; ++i) {
        const float t = (float)i / RATE;
        out[i] = (std::sin(TAU * hz * t) + 0.5f * std::sin(TAU * 2.f * hz * t + ph)) * (0.75f + 0.25f * std::sin(TAU * 3.f * t + ph)) * env(i, n, 0.05f, 0.1f);
    }
    return out;
}
inline Buf wings(float dur, float hz, Rng& r) {
    const int n = N(dur); Buf out(n); LowPass lp(hz * 4.f);
    for (int i = 0; i < n; ++i) {
        const float beat = 0.5f + 0.5f * std::sin(TAU * 6.f * i / RATE);
        out[i] = lp(r.bi()) * 3.f * beat * beat * env(i, n, 0.02f, 0.05f);
    }
    return out;
}
inline Buf shimmer(float dur, float hz, Rng& r) {
    const int n = N(dur); Buf out(n, 0.f);
    for (int k = 0; k < 4; ++k) {
        const float f = hz * (1.f + 0.13f * k + 0.01f * r.bi()), ph = r.uni() * TAU;
        for (int i = 0; i < n; ++i) out[i] += 0.25f * std::sin(ph + TAU * f * i / RATE) * (0.6f + 0.4f * std::sin(TAU * 9.f * i / RATE + k));
    }
    for (int i = 0; i < n; ++i) out[i] *= env(i, n, 0.05f, dur * 0.4f);
    return out;
}
inline Buf siren(float dur, float f0, float f1) {
    const int n = N(dur); Buf out(n); float ph = 0.f;
    for (int i = 0; i < n; ++i) { ph += (f0 * std::pow(f1 / f0, (float)i / n)) / RATE; ph -= std::floor(ph); out[i] = (2.f * std::fabs(2.f * ph - 1.f) - 1.f) * env(i, n, 0.05f, 0.05f); }
    return out;
}
inline Buf zap(float hz, Rng& r) {
    Buf out = metal(0.15f, hz, r, 25.f);
    add(out, servo(0.12f, hz * 1.5f, hz * 0.5f, r, 0.002f, 0.06f), 0.6f);
    return out;
}
inline Buf sing(float d, float f, float a, float b, Vowel v, const Profile& p, Rng& r, float atk = 0.03f) {
    return choir(d, f * a, f * b, v, p.voices, p.formant, r, atk, d * 0.3f, 0.15f);
}
inline Buf chord(float d, float f, const Profile& p, Rng& r, float r1, float r2, float r3, float atk = 0.1f) {
    const int v = std::max(2, p.voices / 2);
    Buf out = choir(d, f * r1, f * r1, p.vowel, v, p.formant, r, atk, d * 0.3f);
    add(out, choir(d, f * r2, f * r2, p.vowel, v, p.formant, r, atk, d * 0.3f));
    add(out, choir(d, f * r3, f * r3, p.vowel, v, p.formant, r, atk, d * 0.3f));
    return out;
}

// The type's machine layer, `dur` long
inline Buf body(const Profile& p, float dur, Rng& r) {
    const float h = p.bodyHz;
    Buf out(N(dur), 0.f);
    switch (p.body) {
        case Body::SERVO:     add(out, servo(dur, h, h * 1.15f, r)); break;
        case Body::SKITTER:   for (float t = 0.f; t < dur - 0.03f; t += 0.025f + 0.03f * r.uni()) add(out, click(h * (0.8f + 0.4f * r.uni()), r), 0.8f, t); break;
        case Body::HYDRAULIC: add(out, vent(dur, 900.f, r, dur * 0.3f, dur * 0.5f), 0.7f); add(out, servo(dur, h, h * 0.8f, r), 0.4f); break;
        case Body::WINGS:     add(out, wings(dur, h, r)); break;
        case Body::HEAVY:     add(out, servo(dur, h, h * 0.9f, r), 0.6f); add(out, grind(dur, 20.f, h * 3.f, r), 0.5f); break;
        case Body::TREADS:    add(out, grind(dur, 35.f, 300.f, r), 0.7f); add(out, thud(h, h * 0.6f, std::min(dur, 0.3f)), 0.5f); break;
        case Body::SCRAPE:    add(out, vent(dur, 2500.f, r, 0.02f, dur * 0.6f), 0.6f); add(out, metal(dur, h, r, 10.f), 0.3f); break;
        case Body::CRACKLE:   for (float t = 0.f; t < dur - 0.03f; t += 0.02f + 0.08f * r.uni()) add(out, click(h * (0.6f + 0.8f * r.uni()), r, 0.015f), 0.6f, t);
                              add(out, hum(dur, 60.f, r), 0.4f); break;
        case Body::HUM:       add(out, hum(dur, h, r)); break;
        case Body::GRIND:     add(out, grind(dur, 12.f, h * 3.f, r)); break;
        case Body::GEARS:     add(out, grind(dur, 18.f, 400.f, r), 0.7f); add(out, metal(dur, h * 2.f, r, 4.f), 0.4f); break;
        case Body::ARMOUR:    for (float t = 0.f; t < dur - 0.05f; t += 0.06f + 0.06f * r.uni()) add(out, metal(0.08f, h * (0.9f + 0.2f * r.uni()), r, 40.f), 0.4f, t); break;
        case Body::CHAINS:    for (float t = 0.f; t < dur - 0.08f; t += 0.05f + 0.07f * r.uni()) add(out, metal(0.12f, h * (2.f + 2.f * r.uni()), r, 30.f), 0.5f, t); break;
    }
    out.resize(N(dur));
    return out;
}

// ---- kinds ---------------------------------------------------------------------
inline Buf spawnV(const Profile& p, Rng& r) {   // a rising breath and the machine waking
    const float d = 0.45f + 0.35f * p.size;
    Buf out = choir(d, p.f0 * 0.7f, p.f0, p.vowel, p.voices, p.formant, r, d * 0.6f, 0.12f, 0.35f);
    add(out, body(p, d, r), p.f0 > 0.f ? 0.5f : 1.f);
    add(out, metal(0.3f, p.bodyHz * 2.f, r, 12.f), 0.3f, d * 0.6f);
    return out;
}
inline Buf idleV(const Profile& p, Rng& r) {    // a murmur over the hum of the body
    const float d = 0.5f + 0.5f * p.size;
    Buf out = choir(d, p.f0 * (0.98f + 0.04f * r.uni()), p.f0 * 0.95f, p.vowel, p.voices, p.formant * (0.95f + 0.1f * r.uni()), r, 0.15f, 0.3f, 0.2f, 3.f);
    add(out, body(p, d, r), p.f0 > 0.f ? 0.35f : 1.f);
    return out;
}
inline Buf moveV(const Profile& p, Rng& r) {    // one step / wingbeat
    const float d = 0.08f + 0.17f * p.size;
    Buf out(N(d), 0.f);
    switch (p.body) {
        case Body::SKITTER: for (int k = 0; k < 3; ++k) add(out, click(p.bodyHz * (0.8f + 0.4f * r.uni()), r), 0.8f, k * 0.02f); break;
        case Body::WINGS:   add(out, wings(d * 2.f, p.bodyHz, r)); break;
        case Body::HUM: case Body::CRACKLE: add(out, hum(d * 2.f, p.bodyHz, r), 0.6f); break;
        case Body::TREADS: case Body::GRIND: add(out, grind(d * 1.5f, 30.f, 250.f, r), 0.7f); add(out, thud(p.bodyHz, p.bodyHz * 0.6f, d), 0.6f); break;
        case Body::ARMOUR: case Body::CHAINS: add(out, metal(d, p.bodyHz * 1.5f, r, 25.f), 0.5f); add(out, thud(90.f + 60.f * (1.f - p.size), 50.f, d), 0.8f); break;
        default: add(out, servo(d, p.bodyHz, p.bodyHz * 1.3f, r, 0.005f, 0.04f), 0.5f); add(out, thud(110.f - 50.f * p.size, 50.f - 20.f * p.size, d), 0.9f); break;
    }
    return out;
}
inline Buf hurtV(const Profile& p, Rng& r) {    // a short choked note
    const float d = 0.16f + 0.14f * p.size;
    Buf out = choir(d, p.f0 * 1.3f, p.f0 * 0.9f, p.vowel, std::max(1, p.voices / 2), p.formant, r, 0.005f, 0.08f, 0.25f);
    add(out, click(p.bodyHz * 2.f, r), 0.6f);
    if (p.f0 <= 0.f) { add(out, metal(d, p.bodyHz * 2.f, r, 20.f), 0.8f); add(out, servo(d, p.bodyHz * 1.5f, p.bodyHz, r, 0.002f, 0.05f), 0.4f); }
    return out;
}
inline Buf deathV(const Profile& p, Rng& r) {   // the choir cut off, the machine winding down
    const float d = 0.55f + 0.65f * p.size;
    Buf out = choir(d * 0.6f, p.f0, p.f0 * 0.55f, p.vowel, p.voices, p.formant, r, 0.01f, 0.03f, 0.3f);
    add(out, servo(d, p.bodyHz, p.bodyHz * 0.15f, r, 0.01f, d * 0.5f), 0.5f);
    add(out, thud(120.f - 60.f * p.size, 40.f, 0.35f), 0.7f, d * 0.55f);
    add(out, metal(0.4f, p.bodyHz * 2.f, r, 8.f), 0.3f, d * 0.55f);
    return out;
}
inline Buf tellV(EnemyType t, AttackKind a, const Profile& p, Rng& r) {
    const float d = tellDur(t, a), f = p.f0 > 0.f ? p.f0 : 200.f;
    Buf out(N(d), 0.f);
    switch (a) {
        case AttackKind::SHOT:     // a rising hum
            add(out, choir(d, f * 0.8f, f * 1.25f, p.vowel, p.voices, p.formant, r, d * 0.7f, 0.04f, 0.1f));
            add(out, servo(d, p.bodyHz * 0.8f, p.bodyHz * 1.6f, r, d * 0.7f, 0.04f), 0.35f); break;
        case AttackKind::LUNGE:    // a sharp hiss into the lunge
            add(out, vent(d, 3200.f, r, d * 0.8f, 0.03f));
            add(out, choir(d, f, f * 1.2f, p.vowel, 2, p.formant, r, d * 0.5f, 0.03f, 0.6f), 0.5f); break;
        case AttackKind::BURST:    // a whine climbing over the paint, a click at fire
            add(out, servo(d, 500.f, 2400.f, r, d * 0.5f, 0.02f), 0.7f);
            add(out, click(3000.f, r), 1.f, d - 0.03f); break;
        case AttackKind::DIVE:     // a falling shriek
            add(out, choir(d, f * 1.6f, f * 0.8f, Vowel::EE, p.voices, p.formant, r, 0.05f, 0.1f, 0.2f));
            add(out, vent(d, 1500.f, r, d * 0.8f, 0.05f), 0.5f); break;
        case AttackKind::SLAM: case AttackKind::PSLAM:   // a huge inhale, then the gears grind
            add(out, vent(d * 0.6f, 700.f, r, d * 0.55f, 0.05f), 0.9f);
            add(out, choir(d * 0.6f, f * 0.9f, f * 1.1f, p.vowel, p.voices, p.formant * 0.9f, r, d * 0.5f, 0.05f, 0.6f), 0.5f);
            add(out, grind(d * 0.4f, 25.f, p.bodyHz * 3.f, r), 0.8f, d * 0.6f); break;
        case AttackKind::LOB:      // a short hum and a click
            add(out, choir(d, f, f * 1.15f, p.vowel, p.voices, p.formant, r, d * 0.6f, 0.05f));
            add(out, click(800.f, r), 0.8f, d - 0.04f); break;
        case AttackKind::FUSE:     // beeps, faster and faster
            for (float tt = 0.f, gap = 0.12f; tt < d - 0.02f; tt += gap, gap = std::max(0.03f, gap * 0.72f)) add(out, beep(2200.f, 0.025f), 1.f, tt);
            break;
        case AttackKind::SHELL:    // a siren rising over the choir's drone
            add(out, siren(d, 300.f, 900.f));
            add(out, choir(d, f, f, p.vowel, p.voices, p.formant, r, 0.1f, 0.1f), 0.5f); break;
        case AttackKind::SMASH:    // a ratchet winding up
            for (float tt = 0.f, gap = 0.09f; tt < d - 0.02f; tt += gap, gap = std::max(0.025f, gap * 0.85f)) add(out, click(1400.f, r), 0.7f, tt);
            add(out, grind(d, 15.f, 300.f, r), 0.5f); break;
        case AttackKind::BASH:     // a shield clang, then the war cry
            add(out, metal(0.3f, 600.f, r, 10.f), 0.8f);
            add(out, choir(d - 0.12f, f * 1.4f, f * 1.1f, Vowel::AH, p.voices, p.formant, r, 0.02f, 0.05f, 0.3f), 1.f, 0.12f); break;
        case AttackKind::BEAM:     // the chord brightens through the charge
            add(out, choir(d, f, f * 1.5f, Vowel::AH, p.voices, p.formant, r, d * 0.4f, 0.05f, 0.1f, 6.f));
            add(out, shimmer(d, 2000.f, r), 0.4f); break;
        case AttackKind::VOLLEY:
            if (t == EnemyType::ANCHOR) {   // a muffled thump, then the lob's pop
                add(out, thud(60.f, 40.f, 0.35f), 0.8f);
                add(out, choir(d, f, f, p.vowel, p.voices, p.formant, r, d * 0.5f, 0.05f), 0.6f);
                add(out, click(500.f, r), 0.6f, d - 0.04f);
            } else add(out, chord(d, f, p, r, 1.f, 1.2f, 1.5f));   // the Warden: a minor chord
            break;
        case AttackKind::SUMMON:   // a swell from below
            add(out, choir(d, f * 0.5f, f, p.vowel, 6, p.formant, r, d * 0.9f, 0.05f, 0.2f));
            add(out, grind(d, 10.f, 300.f, r), 0.4f); break;
        // THE SOVEREIGN: a sung syllable for every stroke
        case AttackKind::SWEEP:    add(out, sing(d, f, 1.0f, 1.3f, Vowel::AH, p, r)); break;
        case AttackKind::CRESCENT: add(out, sing(d, f, 1.3f, 0.9f, Vowel::AH, p, r)); break;
        case AttackKind::CLEAVE:   add(out, sing(d, f, 0.9f, 1.5f, Vowel::OH, p, r)); break;
        case AttackKind::DASH:     add(out, sing(d, f, 1.2f, 1.0f, Vowel::EH, p, r, 0.005f)); add(out, vent(d, 2000.f, r, d, 0.02f), 0.4f); break;
        case AttackKind::LEAP:     add(out, sing(d, f, 1.0f, 1.8f, Vowel::EE, p, r)); break;
        case AttackKind::BLINK:    add(out, choir(d, f, f, Vowel::EH, 1, p.formant, r, 0.02f, d * 0.4f, 1.5f)); break;   // a whisper
        case AttackKind::WHIRL:    add(out, choir(d, f, f, Vowel::OO, 1, p.formant, r, 0.03f, d * 0.3f, 0.1f, 14.f)); add(out, wings(d, 400.f, r), 0.4f); break;
        case AttackKind::THRUST:   add(out, sing(d * 0.5f, f, 1.1f, 1.0f, Vowel::EH, p, r, 0.003f));
                                   add(out, sing(d * 0.4f, f, 1.3f, 1.3f, Vowel::AH, p, r, 0.003f), 1.f, d * 0.55f); break;
        case AttackKind::RUPTURE:  add(out, sing(d, f, 0.7f, 1.0f, Vowel::UH, p, r)); add(out, grind(d, 20.f, 250.f, r), 0.6f); break;
        case AttackKind::JUDGMENT: add(out, chord(d, f, p, r, 1.f, 1.25f, 1.5f)); break;
        case AttackKind::PHANTOMS: add(out, chord(d, f, p, r, 1.f, 1.19f, 1.41f)); break;
        // THE PENITENT
        case AttackKind::CENSER_LOW:   // a low groan
            add(out, choir(d, f, f * 0.8f, Vowel::OO, p.voices, p.formant, r, d * 0.4f, 0.1f, 0.2f));
            add(out, grind(d, 14.f, 200.f, r), 0.6f); break;
        case AttackKind::CENSER_HIGH:  // a bell
            add(out, metal(d, 660.f, r, 2.f), 0.8f); add(out, metal(d, 990.f, r, 2.5f), 0.5f);
            add(out, choir(d, f * 4.f, f * 4.f, Vowel::EE, 3, 1.2f, r, d * 0.5f, 0.1f), 0.3f); break;
        case AttackKind::PSTOMP:       // two heavy beats
            add(out, thud(65.f, 35.f, 0.3f)); add(out, thud(65.f, 35.f, 0.3f), 1.f, d * 0.5f); break;
        case AttackKind::PLASH:        // the chains rattle, a whine rises
            add(out, body(p, d, r), 0.8f); add(out, servo(d, 300.f, 1500.f, r, d * 0.6f, 0.03f), 0.5f); break;
        case AttackKind::SCOURGE:      // a chant, three syllables
            for (int k = 0; k < 3; ++k)
                add(out, sing(d / 3.f, f, 1.f + 0.1f * k, 1.f + 0.1f * k, k % 2 ? Vowel::AH : Vowel::OH, p, r), 1.f, k * d / 3.f);
            break;
        // THE REVENANT / THE WEAVER
        case AttackKind::RAKE:     add(out, vent(d, 2600.f, r, d * 0.7f, 0.03f)); add(out, metal(0.12f, 900.f, r, 30.f), 0.6f, d - 0.12f); break;
        case AttackKind::SOULBOLT: add(out, choir(d, f * 0.9f, f * 1.6f, Vowel::OO, p.voices, p.formant, r, d * 0.8f, 0.05f, 0.4f)); break;
        case AttackKind::STRING:   add(out, servo(d, 300.f, 1400.f, r, d * 0.8f, 0.03f), 0.8f);
                                   for (float tt = 0.f; tt < d - 0.05f; tt += 0.06f) add(out, click(2200.f, r, 0.02f), 0.4f, tt); break;
        // THE LEVIATHAN
        case AttackKind::CRASH:     // rearing back: a roar climbing out of the deep
            add(out, choir(d, f * 0.8f, f * 1.7f, Vowel::UH, p.voices, p.formant, r, d * 0.7f, 0.05f, 0.3f));
            add(out, grind(d, 18.f, 160.f, r), 0.7f); break;
        case AttackKind::TORRENT:   // a gurgling swell in the throat
            add(out, choir(d, f * 1.2f, f * 1.5f, Vowel::OH, p.voices, p.formant, r, d * 0.6f, 0.05f, 0.2f, 9.f), 0.8f);
            for (float tt = 0.f; tt < d - 0.05f; tt += 0.05f + 0.04f * r.uni()) add(out, thud(120.f + 80.f * r.uni(), 60.f, 0.06f), 0.4f, tt);
            break;
        case AttackKind::TIDE:      // the tail coming up: water churning into a hiss
            add(out, vent(d, 700.f, r, d * 0.8f, 0.05f)); add(out, thud(55.f, 30.f, 0.4f), 0.8f, d - 0.15f); break;
        case AttackKind::SPIT:      // a hacking retch
            add(out, choir(d * 0.6f, f * 2.f, f * 1.2f, Vowel::AH, 3, p.formant, r, 0.02f, 0.1f, 0.6f), 0.8f);
            add(out, vent(d * 0.4f, 1800.f, r, 0.01f, 0.2f), 0.7f, d * 0.6f); break;
        case AttackKind::BREACH:    // from under the floor: grinding, then the roar breaking the surface
            add(out, grind(d, 12.f, 120.f, r), 0.8f);
            add(out, choir(d, f * 0.6f, f * 1.4f, Vowel::UH, 6, p.formant, r, d * 0.9f, 0.05f, 0.3f), 0.8f); break;
        case AttackKind::SWALLOW:   // the jaw unhinging and a long inhale
            add(out, metal(0.25f, p.bodyHz * 4.f, r, 8.f), 0.6f);
            add(out, vent(d, 1400.f, r, d * 0.9f, 0.05f)); add(out, choir(d, f, f * 1.3f, Vowel::OO, p.voices, p.formant, r, d * 0.8f, 0.05f), 0.5f); break;
        case AttackKind::SUBMERGE:  // going under: the groan falling away
            add(out, choir(d, f * 1.3f, f * 0.6f, Vowel::OO, p.voices, p.formant, r, 0.05f, 0.2f, 0.2f));
            add(out, vent(d, 500.f, r, 0.1f, d * 0.5f), 0.5f); break;
        // THE WARDEN
        case AttackKind::WVENT:     // plates opening: a hiss rising, the gears letting go
            add(out, vent(d, 2500.f, r, d * 0.7f, 0.05f)); add(out, metal(0.3f, p.bodyHz * 3.f, r, 10.f), 0.6f, d * 0.5f); break;
        case AttackKind::LANCE:     // the chest gathering a beam: a whine climbing over the choir
            add(out, servo(d, 400.f, 2000.f, r, d * 0.8f, 0.03f), 0.7f);
            add(out, choir(d, f, f * 1.5f, p.vowel, p.voices, p.formant, r, d * 0.6f, 0.05f), 0.6f); break;
        case AttackKind::SEEKER:    // a ping and a low chord: something is coming for you
            add(out, beep(1600.f, 0.06f)); add(out, beep(1600.f, 0.06f), 1.f, 0.25f);
            add(out, chord(d, f, p, r, 1.f, 1.2f, 1.5f), 0.6f); break;
        case AttackKind::WLUNGE:    // a groan dragged forward
            add(out, choir(d, f * 1.1f, f * 0.7f, Vowel::UH, p.voices, p.formant, r, 0.05f, 0.1f, 0.3f));
            add(out, grind(d, 25.f, 250.f, r), 0.6f); break;
        case AttackKind::DETONATE:  // the meltdown: a siren racing up, the choir swelling
            add(out, siren(d, 400.f, 1600.f)); add(out, choir(d, f * 0.7f, f * 1.4f, p.vowel, 6, p.formant, r, d * 0.8f, 0.05f), 0.7f); break;
        default: add(out, choir(d, f, f * 1.2f, p.vowel, p.voices, p.formant, r, d * 0.6f, 0.05f)); break;
    }
    out.resize(N(d));
    return out;
}
inline Buf attackV(AttackKind a, const Profile& p, Rng& r) {
    const float f = p.f0 > 0.f ? p.f0 : 200.f;
    Buf out;
    switch (a) {
        case AttackKind::SHOT:  add(out, zap(p.bodyHz * 2.f, r)); break;
        case AttackKind::BURST: for (int k = 0; k < 3; ++k) add(out, zap(1800.f, r), 0.9f, k * 0.07f); break;
        case AttackKind::LUNGE: case AttackKind::DIVE: case AttackKind::BASH:
            add(out, vent(a == AttackKind::DIVE ? 0.4f : 0.25f, 1200.f, r, 0.04f, 0.15f));
            add(out, choir(0.2f, f * 1.2f, f, p.vowel, std::max(1, p.voices / 2), p.formant, r, 0.005f, 0.1f, 0.4f), 0.7f);
            if (a == AttackKind::BASH) add(out, metal(0.25f, 500.f, r, 14.f), 0.7f);
            break;
        case AttackKind::LOB:   add(out, thud(180.f, 90.f, 0.15f)); add(out, click(700.f, r), 0.6f); break;
        case AttackKind::SHELL: add(out, thud(70.f, 35.f, 0.5f)); add(out, vent(0.4f, 400.f, r, 0.005f, 0.3f), 0.7f); break;
        case AttackKind::VOLLEY:
            for (int k = 0; k < 3; ++k) { add(out, thud(160.f, 80.f, 0.12f), 0.8f, k * 0.06f); add(out, click(900.f, r), 0.5f, k * 0.06f); }
            break;
        case AttackKind::BEAM:  // held while the beam sweeps (3 s), stopped when it ends
            add(out, choir(3.f, f * 1.5f, f * 1.5f, Vowel::AH, p.voices, p.formant * 1.1f, r, 0.08f, 0.3f, 0.1f, 6.f));
            add(out, hum(3.f, 220.f, r), 0.5f); add(out, shimmer(3.f, 2400.f, r), 0.3f); break;
        default: add(out, click(1000.f, r)); break;
    }
    return out;
}
inline Buf specialV(const VoiceSpec& s, Rng& r) {
    const std::string k = s.special;
    if (k == "halo_break") {   // glass: many bright partials and a burst
        Buf out;
        for (int i = 0; i < 8; ++i) add(out, metal(0.5f, 2000.f + 3000.f * r.uni(), r, 8.f + 8.f * r.uni()), 0.4f, 0.01f * i);
        add(out, vent(0.3f, 5000.f, r, 0.002f, 0.25f), 0.6f);
        return out;
    }
    if (k == "halo_shimmer") return shimmer(0.4f, 3000.f, r);
    const Profile p = profileOf(s.type);
    if (k == "enrage") { Buf out = chord(1.5f, p.f0, p, r, 1.f, 1.5f, 2.f, 0.4f); add(out, grind(1.5f, 22.f, 300.f, r), 0.5f); return out; }
    if (k == "rise")   { Buf out = choir(2.f, p.f0 * 0.75f, p.f0, p.vowel, 6, p.formant, r, 1.2f, 0.4f, 0.2f); add(out, body(p, 2.f, r), 0.6f); return out; }
    if (k == "link")   return choir(0.25f, p.f0, p.f0 * 1.5f, Vowel::EE, 2, p.formant, r, 0.02f, 0.08f, 0.05f);
    if (k == "soul")   return choir(0.9f, 300.f, 700.f, Vowel::OO, 3, 1.1f, r, 0.05f, 0.4f, 0.4f, 7.f);   // a soul tearing free: a rising choir
    if (k == "reform") { Buf out = choir(1.f, 140.f, 220.f, Vowel::OH, 4, 1.f, r, 0.8f, 0.15f, 0.2f); add(out, shimmer(1.f, 2400.f, r), 0.3f); return out; }
    if (k == "choke")  { Buf out = choir(0.8f, p.f0 * 2.5f, p.f0 * 1.5f, Vowel::AH, 4, p.formant, r, 0.01f, 0.3f, 0.8f, 11.f);   // gagging on its own breath
                         add(out, metal(0.5f, 300.f, r, 6.f), 0.6f); add(out, grind(0.8f, 30.f, 200.f, r), 0.5f); return out; }
    if (k == "twang")  { Buf out = metal(0.3f, 1300.f, r, 9.f); add(out, servo(0.25f, 900.f, 300.f, r, 0.002f, 0.2f), 0.5f); return out; }
    return Buf(N(0.1f), 0.f);
}

// To the class's level: scale, round off anything over 0.8 (never past 0.95), repeat
inline void normalise(Buf& x, float targetDb) {
    const int fade = std::min((int)x.size(), N(0.005f));   // nothing ends on a click
    for (int i = 0; i < fade; ++i) x[x.size() - 1 - i] *= (float)i / fade;
    for (int pass = 0; pass < 3; ++pass) {
        const float lv = shortTermDb(x.data(), x.size(), RATE);
        if (lv <= -119.f) return;
        const float g = std::pow(10.f, (targetDb - lv) / 20.f);
        for (float& v : x) {
            v *= g;
            const float a = std::fabs(v);
            if (a > 0.8f) v = (v < 0.f ? -1.f : 1.f) * (0.8f + 0.15f * std::tanh((a - 0.8f) / 0.15f));
        }
    }
}

inline Buf build(const VoiceSpec& s, int variant) {
    Rng r(seedOf(s.name, variant));
    Profile p = s.type == EnemyType::COUNT ? Profile{} : profileOf(s.type);
    const float wob = 1.f + 0.03f * r.bi();   // each variant a little different
    p.f0 *= wob; p.bodyHz *= wob;
    Buf out;
    switch (s.kind) {
        case VoiceKind::SPAWN:   out = spawnV(p, r); break;
        case VoiceKind::IDLE:    out = idleV(p, r); break;
        case VoiceKind::MOVE:    out = moveV(p, r); break;
        case VoiceKind::TELL:    out = tellV(s.type, s.attack, p, r); break;
        case VoiceKind::ATTACK:  out = attackV(s.attack, p, r); break;
        case VoiceKind::HURT:    out = hurtV(p, r); break;
        case VoiceKind::DEATH:   out = deathV(p, r); break;
        case VoiceKind::SPECIAL: out = specialV(s, r); break;
    }
    if (variant > 0) {   // every variant sits a little off the first (some recipes use no randomness)
        const float k = 1.f + 0.025f * (variant % 2 ? 1.f : -1.f) * (float)((variant + 1) / 2);
        Buf shifted((size_t)std::max(1.f, out.size() / k));
        for (size_t i = 0; i < shifted.size(); ++i) {
            const float x = i * k; const size_t j = (size_t)x; const float f = x - j;
            shifted[i] = j + 1 < out.size() ? out[j] + (out[j + 1] - out[j]) * f : out.back();
        }
        out.swap(shifted);
    }
    normalise(out, mixTargetDb(s.cls));
    return out;
}

}   // namespace VoiceSynth
