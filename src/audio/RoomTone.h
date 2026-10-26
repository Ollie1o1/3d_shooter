#pragma once
// =============================================================================
// RoomTone.h — each arena's own air, synthesized live under the music: no
// audio files, just noise, filters and a few sine partials.
//
//   YARD     a warm breeze; birds by day, cicadas once night falls (setNight)
//   FOUNDRY  furnace roar and rumble, mains hum, distant clanks that echo
//            down the hall, now and then a burst of steam
//   SPIRE    high wind whistling through the stone, a far-off bell
//   CORE     reactor hum and thrum, cooling fans, chirps of data
//   SANCTUM  a slow beating drone, cold hall air, a distant boom
//
// FAST's rooms and ACT II's arenas are each a recipe over one kit of layers
// (wind, whistles, hum, rumble, drips, running water, clanks, steam, a clock,
// a choir, breathing, bells, a pump, data, a glassy shimmer): see recipeOf.
// A crossfade blends two recipes, so each layer still renders once.
//
// Walking from one arena to the next crossfades over a couple of seconds.
// setLevel ducks it (under a fight, a pause). The game thread sets atomics;
// the audio thread (AudioSystem's callback) renders. Tests render it headless.
// =============================================================================
#include <atomic>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include "audio/AudioTypes.h"

class RoomToneSynth {
public:
    void setSampleRate(float r) { sr = r; }
    void setRoom(RoomTone t)    { target.store((int)t, std::memory_order_relaxed); }
    void setNight(float n)      { night.store(std::clamp(n, 0.f, 1.f), std::memory_order_relaxed); }
    void setLevel(float l)      { level.store(std::max(0.f, l), std::memory_order_relaxed); }
    void setVolume(float v)     { volume.store(std::max(0.f, v), std::memory_order_relaxed); }

    // Audio thread: add the room (stereo, interleaved) into buf
    void render(float* buf, int frames) {
        const int want = target.load(std::memory_order_relaxed);
        const float lvlTarget = level.load(std::memory_order_relaxed) * volume.load(std::memory_order_relaxed);
        const float nightTarget = night.load(std::memory_order_relaxed);
        const float dt = (float)frames / sr;
        bool any = false;
        float g0[ROOMS], g1[ROOMS];
        for (int r = 1; r < ROOMS; ++r) {   // crossfade: ~2.5 s in, ~2.5 s out
            g0[r] = gain[r];
            float goal = r == want ? 1.f : 0.f;
            gain[r] += std::clamp(goal - gain[r], -dt / 2.5f, dt / 2.5f);
            g1[r] = gain[r];
            any = any || g0[r] > 1e-4f || g1[r] > 1e-4f;
        }
        float lv0 = levelNow;
        levelNow += (lvlTarget - levelNow) * std::min(1.f, dt * 2.f);
        nightNow += (nightTarget - nightNow) * std::min(1.f, dt * 0.5f);
        // The kit's layers for the recipes playing now, blended by their gains
        Recipe mix = {};
        float best[RECIPE_PARAMS] = {};
        for (int r = 1; r < ROOMS; ++r) {
            if (g1[r] <= 0.f) continue;
            const Recipe& q = recipeOf((RoomTone)r);
            for (int k = 0; k < RECIPE_LEVELS; ++k) mix.lv[k] += q.lv[k] * g1[r];
            for (int k = 0; k < RECIPE_PARAMS; ++k) {   // pitches and rates: the loudest user's
                float w = g1[r] * q.lv[paramOwner(k)];
                if (w > best[k]) { best[k] = w; mix.pm[k] = q.pm[k]; }
            }
        }
        for (int k = 0; k < RECIPE_PARAMS; ++k) if (best[k] > 0.f) kitPm[k] = mix.pm[k];
        float kit0[RECIPE_LEVELS]; bool kitOn = false;
        for (int k = 0; k < RECIPE_LEVELS; ++k) { kit0[k] = kitLv[k]; kitLv[k] = mix.lv[k]; kitOn = kitOn || kit0[k] > 1e-4f || kitLv[k] > 1e-4f; }
        if (!any) return;
        for (int f = 0; f < frames; ++f) {
            float t = (float)(f + 1) / (float)frames;
            float lv = lv0 + (levelNow - lv0) * t;
            float L = 0.f, R = 0.f;
            for (int r = 1; r < ROOMS; ++r) {
                float g = g0[r] + (g1[r] - g0[r]) * t;
                if (g <= 1e-4f) continue;
                float l = 0.f, rr = 0.f;
                switch ((RoomTone)r) {
                    case RoomTone::YARD:    yard(l, rr);    break;
                    case RoomTone::FOUNDRY: foundry(l, rr); break;
                    case RoomTone::SPIRE:   spire(l, rr);   break;
                    case RoomTone::CORE:    core(l, rr);    break;
                    case RoomTone::SANCTUM: sanctum(l, rr); break;
                    default: break;
                }
                L += l * g; R += rr * g;
            }
            if (kitOn) {
                float lvNow[RECIPE_LEVELS];
                for (int k = 0; k < RECIPE_LEVELS; ++k) lvNow[k] = kit0[k] + (kitLv[k] - kit0[k]) * t;
                kit(lvNow, L, R);
            }
            partials(L, R);
            buf[2 * f]     += L * lv;
            buf[2 * f + 1] += R * lv;
        }
    }

private:
    static constexpr int ROOMS = (int)RoomTone::COUNT;
    static constexpr float TAU = 6.2831853f;
    float sr = 44100.f;
    std::atomic<int>   target{0};
    std::atomic<float> night{0.f}, level{1.f}, volume{1.f};
    float gain[ROOMS] = {};
    float levelNow = 1.f, nightNow = 0.f;
    uint32_t rng = 0x9E3779B9u;

    float white() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (float)(rng & 0xFFFFFF) / 8388607.5f - 1.f; }
    float uni(float a, float b) { return a + (b - a) * (white() * 0.5f + 0.5f); }
    float onePole(float hz) const { return 1.f - std::exp(-TAU * hz / sr); }
    static void pan(float v, float p, float& l, float& r) {   // p -1 (left) .. 1 (right), equal power
        float a = (p + 1.f) * 0.7853982f;
        l += v * std::cos(a); r += v * std::sin(a);
    }

    // A state-variable filter (TPT): band-pass and low-pass from one update
    struct SVF {
        float ic1 = 0.f, ic2 = 0.f, bp = 0.f, lp = 0.f;
        void run(float x, float g, float k) {
            float a1 = 1.f / (1.f + g * (g + k)), a2 = g * a1, a3 = g * a2;
            float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.f * v1 - ic1; ic2 = 2.f * v2 - ic2; bp = v1; lp = v2;
        }
    };
    float svfG(float hz) const { return std::tan(3.14159265f * std::min(hz, sr * 0.45f) / sr); }

    // Struck things (clanks, bells, booms) are sums of decaying sine partials
    struct Partial { float ph = 0.f, hz = 0.f, amp = 0.f, decay = 1.f, glide = 1.f, pan = 0.f; int delay = 0; };
    static constexpr int MAX_PARTIALS = 64;
    Partial part[MAX_PARTIALS];
    void strike(float hz, float amp, float seconds, float p, float delaySec = 0.f, float glideTo = 1.f) {
        for (auto& q : part) if (q.amp < 1e-5f) {
            q.ph = 0.f; q.hz = hz; q.amp = amp; q.pan = p;
            q.decay = std::exp(-6.9f / (seconds * sr));          // -60 dB over `seconds`
            q.glide = std::pow(glideTo, 1.f / (seconds * sr));   // pitch drifts toward hz * glideTo
            q.delay = (int)(delaySec * sr);
            return;
        }
    }
    void partials(float& L, float& R) {
        for (auto& q : part) {
            if (q.amp < 1e-5f) continue;
            if (q.delay > 0) { --q.delay; continue; }
            q.ph += q.hz / sr; q.ph -= std::floor(q.ph);
            q.hz *= q.glide;
            pan(std::sin(TAU * q.ph) * q.amp, q.pan, L, R);
            q.amp *= q.decay;
        }
    }
    // Countdown to the next event, in samples
    struct Timer { float left = 0.f; bool due(float sr_) { left -= 1.f; if (left > 0.f) return false; (void)sr_; return true; } };

    // ---- THE KIT: FAST's rooms and ACT II's arenas ------------------------------
    // A recipe: how loud each layer is (0..1ish), and its pitch or rate
    enum Lv { WIND, WHISTLE, HUM, RUMBLE, DRIPS, WATER, CLANKS, STEAM, TICKS, CHOIR, BREATH, BELLS, PUMP, DATA, SHIMMER, RECIPE_LEVELS };
    enum Pm { BRIGHT, HUM_HZ, CHOIR_ROOT, BELL_ROOT, TICK_HZ, PUMP_PERIOD, RECIPE_PARAMS };
    static int paramOwner(int p) {
        switch (p) { case BRIGHT: return WIND; case HUM_HZ: return HUM; case CHOIR_ROOT: return CHOIR;
                     case BELL_ROOT: return BELLS; case TICK_HZ: return TICKS; default: return PUMP; }
    }
    struct Recipe { float lv[RECIPE_LEVELS] = {}; float pm[RECIPE_PARAMS] = {0.4f, 55.f, 110.f, 220.f, 1.f, 1.2f}; };
    static Recipe make(std::initializer_list<std::pair<int, float>> lv, std::initializer_list<std::pair<int, float>> pm = {}) {
        Recipe r; for (auto& [k, v] : lv) r.lv[k] = v; for (auto& [k, v] : pm) r.pm[k] = v; return r;
    }
    static const Recipe& recipeOf(RoomTone t) {
        static const Recipe none{};
        static const Recipe table[] = {
            // FAST
            make({{WIND, 0.6f}, {WATER, 0.85f}, {DRIPS, 0.3f}, {RUMBLE, 0.3f}}, {{BRIGHT, 0.3f}}),                           // CANAL: water in the lane, a city far off
            make({{WATER, 1.0f}, {PUMP, 0.7f}, {HUM, 0.4f}, {DRIPS, 0.5f}}, {{HUM_HZ, 50.f}, {PUMP_PERIOD, 1.1f}}),            // SLUICE: rushing water, the pump's beat
            make({{WIND, 0.5f}, {CHOIR, 0.3f}, {BELLS, 0.35f}}, {{BRIGHT, 0.5f}, {CHOIR_ROOT, 110.f}, {BELL_ROOT, 330.f}}),  // ASCENT: a warm hall, chimes
            make({{WIND, 1.0f}, {WHISTLE, 0.8f}, {RUMBLE, 0.2f}}, {{BRIGHT, 0.45f}}),                                        // SPAN: canyon wind
            make({{DRIPS, 1.0f}, {HUM, 0.4f}, {WIND, 0.35f}, {RUMBLE, 0.5f}}, {{HUM_HZ, 40.f}, {BRIGHT, 0.1f}}),             // WELL: drips falling a long way
            make({{PUMP, 1.0f}, {CLANKS, 0.6f}, {HUM, 0.5f}, {RUMBLE, 0.5f}, {STEAM, 0.5f}}, {{HUM_HZ, 60.f}, {PUMP_PERIOD, 0.8f}}),   // PUMPWORKS
            make({{WIND, 0.7f}, {WHISTLE, 0.4f}, {HUM, 0.35f}, {DATA, 0.35f}}, {{BRIGHT, 0.6f}, {HUM_HZ, 120.f}}),           // TOWER: wind round the keep, its wiring
            // ACT II
            make({{WATER, 0.4f}, {DRIPS, 0.8f}, {CHOIR, 0.5f}, {RUMBLE, 0.2f}}, {{CHOIR_ROOT, 73.4f}}),                      // NAVE: water and an organ's drone
            make({{TICKS, 1.0f}, {HUM, 0.6f}, {CHOIR, 0.3f}, {WIND, 0.5f}}, {{HUM_HZ, 110.f}, {TICK_HZ, 1.5f}, {CHOIR_ROOT, 98.f}, {BRIGHT, 0.2f}}),   // ORRERY: clockwork, the sun's roar
            make({{WIND, 0.8f}, {CLANKS, 0.5f}, {RUMBLE, 0.5f}, {HUM, 0.3f}}, {{BRIGHT, 0.2f}, {HUM_HZ, 45.f}}),             // DESCENT: the shaft's draught, chains
            make({{CHOIR, 0.6f}, {WIND, 0.4f}, {BREATH, 0.2f}, {BELLS, 0.2f}}, {{CHOIR_ROOT, 87.3f}, {BRIGHT, 0.7f}, {BELL_ROOT, 174.6f}}),   // RELIQUARY: the void
            make({{BREATH, 1.0f}, {RUMBLE, 0.8f}, {DRIPS, 0.4f}, {CHOIR, 0.15f}}, {{CHOIR_ROOT, 55.f}}),                     // MAW: something breathing
            make({{SHIMMER, 1.0f}, {CHOIR, 0.4f}, {WIND, 0.4f}}, {{CHOIR_ROOT, 220.f}, {BRIGHT, 0.8f}}),                     // ZENITH: above the clouds
        };
        int i = (int)t - (int)RoomTone::CANAL;
        return i >= 0 && i < (int)(sizeof(table) / sizeof(table[0])) ? table[i] : none;
    }
    float kitLv[RECIPE_LEVELS] = {};
    float kitPm[RECIPE_PARAMS] = {0.4f, 55.f, 110.f, 220.f, 1.f, 1.2f};

    // Layer state (audio thread)
    float kWL = 0.f, kWR = 0.f, kGust = 0.f, kDrift[2] = {0.3f, 0.7f}, kDriftV[2] = {}; SVF kWh[2];
    float kHumPh = 0.f, kBrown = 0.f, kRumble = 0.f;
    Timer kDrip{44100.f}, kClank{44100.f * 3.f}, kSteam{44100.f * 8.f}, kBell{44100.f * 7.f}, kData{44100.f * 2.f};
    SVF kWater[3]; float kWaterHz[3] = {500.f, 800.f, 1200.f}; int kWaterHold = 0;
    float kSteamT = -1.f, kSteamPan = 0.f, kSteamHP = 0.f;
    float kTickT = 0.f; int kTickN = 0; float kGrind = 0.f;
    float kChoirPh[6] = {}, kChoirSwell = 0.f; SVF kFormant[2];
    float kBreathPh = 0.f, kBreathLP = 0.f, kGrowlPh = 0.f, kGrindLP = 0.f, kPumpLP = 0.f;
    float kPumpT = 0.f;
    int kBlips = 0; float kBlipT = 0.f, kBlipLen = 0.f, kBlipHz = 0.f, kBlipPh = 0.f, kBlipPan = 0.f, kBlipGap = 0.f;
    float kShimL = 0.f, kShimR = 0.f; Timer kSparkle{44100.f};

    void kit(const float* lv, float& L, float& R) {
        const float bright = kitPm[BRIGHT];
        // Wind: low noise opening up with `bright`, slow gusts
        if (lv[WIND] > 1e-4f || lv[WHISTLE] > 1e-4f) {
            kGust += 0.07f / sr;
            float gust = 0.6f + 0.4f * std::sin(TAU * kGust) * std::sin(TAU * kGust * 0.47f + 1.7f);
            float k = onePole(200.f + 1100.f * bright);
            kWL += k * (white() - kWL); kWR += k * (white() - kWR);
            float a = 0.20f * lv[WIND] * gust * (1.2f - 0.6f * bright);
            L += kWL * a; R += kWR * a;
            if (lv[WHISTLE] > 1e-4f) for (int i = 0; i < 2; ++i) {   // narrow bands wandering with it
                kDriftV[i] += (white() * 0.002f - kDriftV[i]) * 0.0005f;
                kDrift[i] = std::clamp(kDrift[i] + kDriftV[i] * 0.02f, 0.f, 1.f);
                float hz = (i ? 900.f : 520.f) + 650.f * kDrift[i] + 150.f * gust;
                kWh[i].run(white(), svfG(hz), 0.08f);
                pan(kWh[i].bp * 0.07f * lv[WHISTLE] * gust * gust, i ? 0.6f : -0.6f, L, R);
            }
        }
        // Hum: a mains-ish tone, harmonics, a little saturation
        if (lv[HUM] > 1e-4f) {
            kHumPh += kitPm[HUM_HZ] / sr; kHumPh -= std::floor(kHumPh);
            float h = std::sin(TAU * kHumPh) + 0.5f * std::sin(2.f * TAU * kHumPh) + 0.3f * std::sin(3.f * TAU * kHumPh) + 0.15f * std::sin(4.f * TAU * kHumPh);
            h = std::tanh(h * 1.3f) * 0.022f * lv[HUM];
            L += h; R += h;
        }
        // Rumble: brown noise, low-passed
        if (lv[RUMBLE] > 1e-4f) {
            kBrown = std::clamp(kBrown * 0.998f + white() * 0.04f, -1.f, 1.f);
            kRumble += onePole(85.f) * (kBrown - kRumble);
            float v = kRumble * 0.09f * lv[RUMBLE];
            L += v; R += v;
        }
        // Drips: a falling plink and its echo
        if (lv[DRIPS] > 1e-4f && kDrip.due(sr)) {
            kDrip.left = uni(0.4f, 2.2f) * sr / std::max(lv[DRIPS], 0.2f);
            float hz = uni(900.f, 1900.f), p = uni(-0.9f, 0.9f), a = 0.02f * std::min(lv[DRIPS], 1.f) * uni(0.5f, 1.f);
            strike(hz, a, 0.12f, p, 0.f, 0.55f);
            strike(hz * 0.98f, a * 0.35f, 0.25f, -p * 0.5f, uni(0.12f, 0.3f), 0.6f);
        }
        // Running water: three bands of noise whose centres jump about (the gurgle)
        if (lv[WATER] > 1e-4f) {
            if (--kWaterHold <= 0) {
                kWaterHold = (int)(uni(0.02f, 0.07f) * sr);
                int i = (int)uni(0.f, 2.99f);
                kWaterHz[i] = uni(i == 0 ? 300.f : i == 1 ? 650.f : 1100.f, i == 0 ? 650.f : i == 1 ? 1100.f : 2200.f);
            }
            float w = 0.f;
            for (int i = 0; i < 3; ++i) { kWater[i].run(white(), svfG(kWaterHz[i]), 0.5f); w += kWater[i].bp; }
            float v = w * 0.035f * lv[WATER];
            L += v * (1.f + 0.2f * white()); R += v * (1.f + 0.2f * white());
        }
        // Clanks: metal struck somewhere, and its echo
        if (lv[CLANKS] > 1e-4f && kClank.due(sr)) {
            kClank.left = uni(2.f, 7.f) * sr / std::max(lv[CLANKS], 0.3f);
            float hz = uni(150.f, 400.f), p = uni(-0.9f, 0.9f), a = uni(0.012f, 0.026f) * std::min(lv[CLANKS], 1.f), d = uni(0.6f, 1.3f);
            const float ratio[4] = {1.f, 2.76f, 5.40f, 8.93f}, amp[4] = {1.f, 0.6f, 0.4f, 0.25f};
            float echo = uni(0.25f, 0.45f);
            for (int k = 0; k < 4; ++k) {
                strike(hz * ratio[k], a * amp[k], d / (1.f + k * 0.5f), p);
                strike(hz * ratio[k], a * amp[k] * 0.3f, d / (1.f + k * 0.5f), -p * 0.6f, echo);
            }
        }
        // Steam: a hiss that swells and dies
        if (lv[STEAM] > 1e-4f) {
            if (kSteam.due(sr)) { kSteam.left = uni(8.f, 20.f) * sr; kSteamT = 0.f; kSteamPan = uni(-0.8f, 0.8f); }
            if (kSteamT >= 0.f) {
                kSteamT += 1.f / sr;
                float env = kSteamT < 0.06f ? kSteamT / 0.06f : kSteamT < 0.9f ? 1.f : std::max(0.f, 1.f - (kSteamT - 0.9f) / 0.8f);
                float w = white(); kSteamHP += onePole(2500.f) * (w - kSteamHP);
                pan((w - kSteamHP) * env * 0.035f * lv[STEAM], kSteamPan, L, R);
                if (kSteamT > 1.7f) kSteamT = -1.f;
            }
        }
        // A clock: tick and tock, a grind of gears under each
        if (lv[TICKS] > 1e-4f) {
            kTickT += kitPm[TICK_HZ] / sr;
            if (kTickT >= 1.f) {
                kTickT -= 1.f; ++kTickN;
                float hz = (kTickN & 1) ? 2600.f : 2100.f, p = (kTickN & 1) ? 0.3f : -0.3f;
                strike(hz, 0.05f * lv[TICKS], 0.05f, p);
                strike(hz * 2.3f, 0.012f * lv[TICKS], 0.03f, p);
                strike(hz * 0.18f, 0.04f * lv[TICKS], 0.14f, p);
                kGrind = 1.f;
            }
            kGrind *= 1.f - 6.f / sr;
            kGrindLP += onePole(500.f) * (white() - kGrindLP);
            float g = kGrindLP * kGrind * 0.03f * lv[TICKS];
            L += g; R += g;
        }
        // Choir: a slow swelling chord (root, fifth, octave, each a pair a hair apart) through an "ah"
        if (lv[CHOIR] > 1e-4f) {
            kChoirSwell += 0.04f / sr;
            float swell = 0.65f + 0.35f * std::sin(TAU * kChoirSwell);
            const float ratio[6] = {1.f, 1.003f, 1.4983f, 1.5021f, 2.f, 2.006f};
            float v = 0.f;
            for (int i = 0; i < 6; ++i) {
                kChoirPh[i] += kitPm[CHOIR_ROOT] * ratio[i] / sr; kChoirPh[i] -= std::floor(kChoirPh[i]);
                v += 2.f * kChoirPh[i] - 1.f;   // saws
            }
            kFormant[0].run(v, svfG(700.f), 1.2f); kFormant[1].run(v, svfG(1150.f), 1.6f);
            float o = (kFormant[0].bp + 0.6f * kFormant[1].bp) * 0.05f * lv[CHOIR] * swell;
            L += o; R += o * 0.9f;
        }
        // Breathing: a long slow in and out, a growl under the out
        if (lv[BREATH] > 1e-4f) {
            kBreathPh += 0.16f / sr; kBreathPh -= std::floor(kBreathPh);
            float cyc = std::sin(TAU * kBreathPh), in = std::max(cyc, 0.f), out = std::max(-cyc, 0.f);
            float n = white();
            kBreathLP += onePole(380.f) * (n - kBreathLP);
            kGrowlPh += 48.f / sr; kGrowlPh -= std::floor(kGrowlPh);
            float v = (kBreathLP * (0.6f * in * in + 1.f * out) * 0.18f + std::sin(TAU * kGrowlPh) * out * out * 0.05f) * lv[BREATH];
            L += v; R += v * 0.95f;
        }
        // Bells: a struck bell, far off
        if (lv[BELLS] > 1e-4f && kBell.due(sr)) {
            kBell.left = uni(9.f, 24.f) * sr / std::max(lv[BELLS], 0.3f);
            const float steps[4] = {1.f, 1.2f, 1.333f, 1.5f};
            float hz = kitPm[BELL_ROOT] * steps[(int)uni(0.f, 3.99f)], p = uni(-0.6f, 0.6f);
            const float ratio[6] = {0.5f, 1.f, 1.19f, 1.5f, 2.f, 2.52f}, amp[6] = {0.5f, 1.f, 0.6f, 0.4f, 0.35f, 0.2f};
            for (int k = 0; k < 6; ++k) strike(hz * ratio[k], 0.018f * amp[k] * std::min(lv[BELLS], 1.f), uni(3.f, 6.f) / (1.f + k * 0.25f), p);
        }
        // A pump: a heavy thump and a breath of air, on its beat
        if (lv[PUMP] > 1e-4f) {
            kPumpT += 1.f / sr;
            if (kPumpT >= kitPm[PUMP_PERIOD]) {
                kPumpT = 0.f;
                strike(58.f, 0.07f * lv[PUMP], 0.35f, 0.f, 0.f, 0.7f);
                strike(116.f, 0.025f * lv[PUMP], 0.2f, 0.f, 0.f, 0.8f);
            }
            float phase = kPumpT / kitPm[PUMP_PERIOD];
            float hiss = (phase > 0.45f && phase < 0.7f) ? std::sin((phase - 0.45f) / 0.25f * 3.14159f) : 0.f;
            float w = white(); kPumpLP += onePole(1800.f) * (w - kPumpLP);
            float v = kPumpLP * hiss * 0.03f * lv[PUMP];
            L += v; R += v;
        }
        // Data: short blips on a pentatonic scale, in bursts
        if (lv[DATA] > 1e-4f) {
            if (kData.due(sr)) { kData.left = uni(1.5f, 5.f) * sr; kBlips = 2 + (int)uni(0.f, 3.99f); kBlipPan = uni(-0.9f, 0.9f); kBlipGap = 0.f; kBlipT = 0.f; }
            if (kBlips > 0) {
                if (kBlipGap > 0.f) kBlipGap -= 1.f / sr;
                else {
                    if (kBlipT == 0.f) {
                        const float scale[5] = {1.f, 1.125f, 1.25f, 1.5f, 1.6875f};
                        kBlipHz = 1318.5f * scale[(int)uni(0.f, 4.99f)]; kBlipLen = uni(0.03f, 0.06f);
                    }
                    kBlipT += 1.f / sr;
                    kBlipPh += kBlipHz / sr; kBlipPh -= std::floor(kBlipPh);
                    float u = kBlipT / kBlipLen, env = u < 0.1f ? u / 0.1f : 1.f - (u - 0.1f) / 0.9f;
                    pan(std::sin(TAU * kBlipPh) * std::max(env, 0.f) * 0.01f * lv[DATA], kBlipPan, L, R);
                    if (u >= 1.f) { --kBlips; kBlipT = 0.f; kBlipGap = uni(0.03f, 0.09f); }
                }
            }
        }
        // Shimmer: thin high air, and now and then a glassy sparkle
        if (lv[SHIMMER] > 1e-4f) {
            float a = white(), b = white();
            kShimL += onePole(7000.f) * (a - kShimL); kShimR += onePole(7000.f) * (b - kShimR);
            float hiL = a - kShimL, hiR = b - kShimR;
            L += hiL * 0.02f * lv[SHIMMER]; R += hiR * 0.02f * lv[SHIMMER];
            if (kSparkle.due(sr)) {
                kSparkle.left = uni(0.3f, 1.4f) * sr;
                const float notes[5] = {1760.f, 1975.5f, 2217.5f, 2637.f, 2960.f};
                float hz = notes[(int)uni(0.f, 4.99f)];
                strike(hz, 0.016f * lv[SHIMMER], 1.6f, uni(-0.9f, 0.9f));
                strike(hz * 2.01f, 0.004f * lv[SHIMMER], 1.0f, uni(-0.9f, 0.9f));
            }
        }
    }

    // ---- SUNSET YARD ----------------------------------------------------------
    float yBreezeL = 0.f, yBreezeR = 0.f, yGust = 0.f;
    Timer yBird{44100.f * 3.f};
    int   yChirps = 0; float yChirpT = -1.f, yChirpLen = 0.f, yChirpPan = 0.f, yChirpHz = 0.f, yChirpPh = 0.f, yChirpGap = 0.f;
    float yCicPh[2] = {}, yCicAM[2] = {}, yCicEnv[2] = {}, yCicOn[2] = {0.f, 0.f}; Timer yCic[2] = {{44100.f}, {44100.f * 2.f}};
    void yard(float& L, float& R) {
        const float n = nightNow, day = 1.f - n;
        // Breeze: soft low noise with slow gusts, a little different in each ear
        yGust += 0.09f / sr;
        float gust = 0.65f + 0.35f * std::sin(TAU * yGust) * std::sin(TAU * yGust * 0.41f + 2.f);
        float k = onePole(420.f);
        yBreezeL += k * (white() - yBreezeL); yBreezeR += k * (white() - yBreezeR);
        float b = 0.27f * gust * (1.f - 0.35f * n);
        L += yBreezeL * b; R += yBreezeR * b;
        // Birds by day: little rising chirps in twos and threes
        if (day > 0.05f && yBird.due(sr)) {
            yBird.left = uni(4.f, 11.f) * sr / std::max(day, 0.2f);
            yChirps = 2 + (int)uni(0.f, 2.99f); yChirpPan = uni(-0.8f, 0.8f); yChirpHz = uni(2400.f, 3300.f);
            yChirpT = 0.f; yChirpLen = uni(0.05f, 0.09f); yChirpGap = 0.f;
        }
        if (yChirps > 0) {
            if (yChirpGap > 0.f) yChirpGap -= 1.f / sr;
            else {
                yChirpT += 1.f / sr;
                float u = yChirpT / yChirpLen;
                float hz = yChirpHz * (1.f + 0.35f * u);
                yChirpPh += hz / sr; yChirpPh -= std::floor(yChirpPh);
                float env = std::sin(3.14159f * std::min(u, 1.f));
                pan(std::sin(TAU * yChirpPh) * env * env * 0.045f * day, yChirpPan, L, R);
                if (u >= 1.f) { --yChirps; yChirpT = 0.f; yChirpGap = uni(0.05f, 0.12f); }
            }
        }
        // Cicadas at night: a high buzz chopped fast, in long rising bursts, one each side
        if (n > 0.02f) for (int i = 0; i < 2; ++i) {
            if (yCic[i].due(sr)) { yCicOn[i] = yCicOn[i] > 0.5f ? 0.f : 1.f; yCic[i].left = sr * (yCicOn[i] > 0.5f ? uni(2.5f, 5.f) : uni(1.f, 3.f)); }
            yCicEnv[i] += (yCicOn[i] - yCicEnv[i]) * onePole(1.2f);
            float hz = i ? 5100.f : 4450.f;
            yCicPh[i] += hz / sr; yCicPh[i] -= std::floor(yCicPh[i]);
            yCicAM[i] += (i ? 52.f : 46.f) / sr; yCicAM[i] -= std::floor(yCicAM[i]);
            float chop = std::sin(3.14159f * yCicAM[i]); chop *= chop; chop *= chop;
            pan(std::sin(TAU * yCicPh[i]) * chop * yCicEnv[i] * 0.026f * n, i ? 0.7f : -0.7f, L, R);
        }
    }

    // ---- THE FOUNDRY ----------------------------------------------------------
    float fBrown = 0.f, fRumble = 0.f, fBreath = 0.f, fHumPh = 0.f; SVF fRoar;
    Timer fClank{44100.f * 2.f}, fSteam{44100.f * 9.f};
    float fSteamT = -1.f, fSteamPan = 0.f, fSteamHP = 0.f;
    void foundry(float& L, float& R) {
        // Rumble: brown noise, low-passed
        fBrown = std::clamp(fBrown * 0.998f + white() * 0.04f, -1.f, 1.f);
        fRumble += onePole(90.f) * (fBrown - fRumble);
        // The furnace breathing: a band of noise that swells and falls
        fBreath += 0.17f / sr;
        fRoar.run(white(), svfG(330.f), 1.3f);
        float roar = fRoar.bp * (0.55f + 0.45f * std::sin(TAU * fBreath)) * 0.10f;
        // Mains hum through the machinery
        fHumPh += 50.f / sr; fHumPh -= std::floor(fHumPh);
        float hum = (std::sin(TAU * fHumPh) + 0.5f * std::sin(2.f * TAU * fHumPh) + 0.25f * std::sin(3.f * TAU * fHumPh)) * 0.008f;
        float c = fRumble * 0.07f + hum;
        L += c + roar * 0.9f; R += c + roar * 1.1f;
        // A clank somewhere in the hall, and its echo off the far wall
        if (fClank.due(sr)) {
            fClank.left = uni(2.5f, 8.f) * sr;
            float hz = uni(160.f, 420.f), p = uni(-0.9f, 0.9f), a = uni(0.012f, 0.03f), d = uni(0.6f, 1.4f);
            const float ratio[4] = {1.f, 2.76f, 5.40f, 8.93f}, amp[4] = {1.f, 0.6f, 0.4f, 0.25f};
            float echo = uni(0.28f, 0.42f);
            for (int k = 0; k < 4; ++k) {
                strike(hz * ratio[k], a * amp[k], d / (1.f + k * 0.5f), p);
                strike(hz * ratio[k], a * amp[k] * 0.3f, d / (1.f + k * 0.5f), -p * 0.6f, echo);
            }
        }
        // A burst of steam: hiss that swells, holds and dies away
        if (fSteam.due(sr)) { fSteam.left = uni(10.f, 22.f) * sr; fSteamT = 0.f; fSteamPan = uni(-0.8f, 0.8f); }
        if (fSteamT >= 0.f) {
            fSteamT += 1.f / sr;
            float env = fSteamT < 0.06f ? fSteamT / 0.06f : fSteamT < 0.9f ? 1.f : std::max(0.f, 1.f - (fSteamT - 0.9f) / 0.8f);
            float w = white();
            fSteamHP += onePole(2500.f) * (w - fSteamHP);
            pan((w - fSteamHP) * env * 0.035f, fSteamPan, L, R);
            if (fSteamT > 1.7f) fSteamT = -1.f;
        }
    }

    // ---- THE SPIRE ------------------------------------------------------------
    float sAirL = 0.f, sAirR = 0.f, sGust = 0.f, sDrift[2] = {0.3f, 0.7f}, sDriftV[2] = {}; SVF sWh[2];
    Timer sBell{44100.f * 6.f};
    void spire(float& L, float& R) {
        sGust += 0.06f / sr;
        float gust = 0.55f + 0.45f * std::sin(TAU * sGust) * std::sin(TAU * sGust * 0.53f + 1.3f);
        float k = onePole(650.f);
        sAirL += k * (white() - sAirL); sAirR += k * (white() - sAirR);
        float a = 0.15f * gust;
        L += sAirL * a; R += sAirR * a;
        // Whistles: narrow bands of noise whose pitch wanders with the gusts
        for (int i = 0; i < 2; ++i) {
            sDriftV[i] += (white() * 0.002f - sDriftV[i]) * 0.0005f;
            sDrift[i] = std::clamp(sDrift[i] + sDriftV[i] * 0.02f, 0.f, 1.f);
            float hz = (i ? 950.f : 620.f) + (i ? 700.f : 600.f) * sDrift[i] + 120.f * gust;
            sWh[i].run(white(), svfG(hz), 0.08f);
            pan(sWh[i].bp * 0.07f * gust * gust, i ? 0.6f : -0.6f, L, R);
        }
        // A bell somewhere far below
        if (sBell.due(sr)) {
            sBell.left = uni(16.f, 34.f) * sr;
            const float notes[4] = {196.f, 220.f, 246.9f, 261.6f};
            float hz = notes[(int)uni(0.f, 3.99f)], p = uni(-0.5f, 0.5f);
            const float ratio[7] = {0.5f, 1.f, 1.19f, 1.5f, 2.f, 2.52f, 3.f}, amp[7] = {0.5f, 1.f, 0.6f, 0.4f, 0.35f, 0.2f, 0.12f};
            for (int k = 0; k < 7; ++k) strike(hz * ratio[k], 0.012f * amp[k], uni(4.f, 7.f) / (1.f + k * 0.25f), p);
        }
    }

    // ---- THE CORE -------------------------------------------------------------
    float cHumPh = 0.f, cThrob = 0.f, cSubPh = 0.f; SVF cFan;
    Timer cData{44100.f};
    int cBlips = 0; float cBlipT = 0.f, cBlipLen = 0.f, cBlipHz = 0.f, cBlipPh = 0.f, cBlipPan = 0.f, cBlipGap = 0.f;
    void core(float& L, float& R) {
        cHumPh += 60.f / sr; cHumPh -= std::floor(cHumPh);
        cThrob += 0.4f / sr; cThrob -= std::floor(cThrob);
        float h = 0.f;
        const float amp[5] = {1.f, 0.5f, 0.35f, 0.2f, 0.12f};
        for (int k = 0; k < 5; ++k) h += amp[k] * std::sin(TAU * cHumPh * (k + 1));
        h = std::tanh(h * 1.4f) * 0.022f * (0.75f + 0.25f * std::sin(TAU * cThrob));
        // The reactor's thrum, felt more than heard
        cSubPh += 45.f / sr; cSubPh -= std::floor(cSubPh);
        float sub = std::sin(TAU * cSubPh) * 0.03f * (0.6f + 0.4f * std::sin(TAU * cThrob * 0.6f));
        cFan.run(white(), svfG(2200.f), 1.8f);
        float fan = cFan.bp * 0.012f;
        L += h + sub + fan; R += h + sub - fan * 0.3f;
        // Data: bursts of short square-ish blips on a pentatonic scale
        if (cData.due(sr)) { cData.left = uni(1.2f, 4.f) * sr; cBlips = 2 + (int)uni(0.f, 4.99f); cBlipPan = uni(-0.9f, 0.9f); cBlipGap = 0.f; cBlipT = 0.f; cBlipLen = 0.f; }
        if (cBlips > 0) {
            if (cBlipGap > 0.f) cBlipGap -= 1.f / sr;
            else {
                if (cBlipT == 0.f) {
                    const float scale[5] = {1.f, 1.125f, 1.25f, 1.5f, 1.6875f};
                    cBlipHz = 1046.5f * scale[(int)uni(0.f, 4.99f)] * (uni(0.f, 1.f) < 0.3f ? 2.f : 1.f);
                    cBlipLen = uni(0.035f, 0.07f);
                }
                cBlipT += 1.f / sr;
                cBlipPh += cBlipHz / sr; cBlipPh -= std::floor(cBlipPh);
                float v = std::sin(TAU * cBlipPh) + 0.3f * std::sin(3.f * TAU * cBlipPh);
                float u = cBlipT / cBlipLen, env = u < 0.1f ? u / 0.1f : 1.f - (u - 0.1f) / 0.9f;
                pan(v * std::max(env, 0.f) * 0.010f, cBlipPan, L, R);
                if (u >= 1.f) { --cBlips; cBlipT = 0.f; cBlipGap = uni(0.03f, 0.09f); }
            }
        }
    }

    // ---- THE SANCTUM ----------------------------------------------------------
    float nPh[4] = {}, nSwell = 0.f, nAir = 0.f;
    Timer nBoom{44100.f * 5.f};
    float nRumbleT = -1.f, nRumbleLP = 0.f, nRumblePan = 0.f;
    void sanctum(float& L, float& R) {
        nSwell += 0.05f / sr;
        float swell = 0.7f + 0.3f * std::sin(TAU * nSwell);
        const float hz[4] = {55.f, 55.45f, 82.4f, 110.3f}, amp[4] = {0.03f, 0.03f, 0.016f, 0.009f};
        float d = 0.f;
        for (int k = 0; k < 4; ++k) { nPh[k] += hz[k] / sr; nPh[k] -= std::floor(nPh[k]); d += std::sin(TAU * nPh[k]) * amp[k]; }
        d *= swell;
        nAir += onePole(280.f) * (white() - nAir);
        float air = nAir * 0.05f;
        L += d + air; R += d - air * 0.6f + white() * 0.0004f;
        // A distant boom: a falling sub thud with a roll of low noise after it
        if (nBoom.due(sr)) {
            nBoom.left = uni(14.f, 30.f) * sr;
            float p = uni(-0.6f, 0.6f);
            strike(40.f, 0.09f, 2.6f, p, 0.f, 0.72f);
            strike(62.f, 0.035f, 1.6f, p, 0.f, 0.8f);
            nRumbleT = 0.f; nRumblePan = p;
        }
        if (nRumbleT >= 0.f) {
            nRumbleT += 1.f / sr;
            nRumbleLP += onePole(160.f) * (white() - nRumbleLP);
            float env = nRumbleT < 0.04f ? nRumbleT / 0.04f : std::exp(-(nRumbleT - 0.04f) * 2.2f);
            pan(nRumbleLP * env * 0.12f, nRumblePan, L, R);
            if (nRumbleT > 2.5f) nRumbleT = -1.f;
        }
    }
};
