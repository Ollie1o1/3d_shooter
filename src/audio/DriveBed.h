#pragma once
// =============================================================================
// DriveBed.h — the sound of being in OVERDRIVE: a low, growling power hum that
// lasts as long as it does, so you know you're in it without looking.
//
// Two detuned saws and a sub on the music's root, through a low-pass that
// throbs open on every eighth note of the music (in time and in key, so it
// sits in the track instead of fighting it). In the last 1.5 s (urgency) the
// throb doubles to sixteenths and brightens: it's running out. It fades in
// fast and out a little slower. It plays at the master volume, not the
// music's: it's a cue, and it has to be there with the music turned off.
//
// No SDL: AudioSystem calls render() in the audio callback right after the
// music (whose clock it reads); the tests render it headless.
// =============================================================================
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>

class DriveBed {
public:
    static constexpr float LEVEL = 0.09f;    // peak gain of the hum at full on (well under the guns)

    // ---- game thread ----
    void set(bool on, float urgency) {
        want.store(on ? 1.f : 0.f, std::memory_order_relaxed);
        urgent.store(std::clamp(urgency, 0.f, 1.f), std::memory_order_relaxed);
    }

    // ---- audio thread ----
    // buf: interleaved stereo, frames long (added to). rootMidi: the music's key;
    // stepLen: samples per sixteenth; stepEnd: the music's position at the end of
    // this buffer, in sixteenths (any count: only its fraction and parity matter)
    void render(float* buf, int frames, float rate, float rootMidi, float stepLen, double stepEnd, float gain) {
        const float target = want.load(std::memory_order_relaxed);
        if (target < 0.001f && now < 0.002f) { now = 0.f; return; }   // (-54 dB under its own level: done)
        const float urgWant = urgent.load(std::memory_order_relaxed);
        const float kIn = 1.f - std::exp(-1.f / (0.06f * rate)), kOut = 1.f - std::exp(-1.f / (0.25f * rate));
        const float kUrg = 1.f - std::exp(-1.f / (0.15f * rate));
        const float hz = 440.f * std::pow(2.f, (rootMidi - 24.f - 69.f) / 12.f);   // two octaves under the key
        const float twoPi = 6.2831853f;
        stepLen = std::max(stepLen, 1.f);
        for (int f = 0; f < frames; ++f) {
            now += (target - now) * (target > now ? kIn : kOut);
            urg += (urgWant - urg) * kUrg;
            // Where the music is, in sixteenths, at this sample
            const double pos = stepEnd - (double)(frames - 1 - f) / stepLen;
            const double fl = std::floor(pos);
            const float inStep = (float)(pos - fl);
            const bool odd = ((int64_t)fl & 1) != 0;
            // Seconds since the last eighth (or sixteenth, when urgent)
            const float sinceEighth = ((odd ? 1.f : 0.f) + inStep) * stepLen / rate;
            const float sinceSixteenth = inStep * stepLen / rate;
            const float since = urg > 0.5f ? sinceSixteenth : sinceEighth;
            const float env = std::exp(-since * (9.f + 8.f * urg));
            // The hum
            ph1 += hz * 1.006f / rate; ph1 -= std::floor(ph1);
            ph2 += hz * 0.994f / rate; ph2 -= std::floor(ph2);
            ph3 += hz * 0.5f / rate;   ph3 -= std::floor(ph3);
            const float saws = (2.f * ph1 - 1.f) + (2.f * ph2 - 1.f);
            const float cut = 140.f + (700.f + 900.f * urg) * env;
            const float g = std::tan(3.1415927f * std::min(cut, 0.45f * rate) / rate);
            const float y = lp(saws * 0.5f, g, 0.9f);
            const float sub = std::sin(twoPi * ph3) * 0.5f;
            float v = std::tanh((y * 1.6f + sub) * 1.3f) * (0.55f + 0.45f * env);
            // A thin crackle on top, riding the throb (gives it presence on small speakers)
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            const float n = (float)(int32_t)rng * (1.f / 2147483648.f);
            hiss += 0.35f * (n - hiss);
            const float crackle = (n - hiss) * env * env * (0.10f + 0.12f * urg);
            v = (v + crackle) * LEVEL * now * gain;
            buf[2 * f] += v; buf[2 * f + 1] += v;
        }
    }

    float level() const { return now; }      // 0..1 how far in it is (tests)

private:
    std::atomic<float> want{0.f}, urgent{0.f};
    float now = 0.f, urg = 0.f, ph1 = 0.f, ph2 = 0.37f, ph3 = 0.f, ic1 = 0.f, ic2 = 0.f, hiss = 0.f;
    uint32_t rng = 0xC0FFEEu;
    // A zero-delay-feedback state-variable low-pass (MusicSynth's)
    float lp(float x, float g, float k) {
        float a1 = 1.f / (1.f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.f * v1 - ic1; ic2 = 2.f * v2 - ic2;
        if (std::fabs(ic1) < 1e-15f) ic1 = 0.f;
        if (std::fabs(ic2) < 1e-15f) ic2 = 0.f;
        return v2;
    }
};
