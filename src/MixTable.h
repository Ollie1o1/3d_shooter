#pragma once
// =============================================================================
// MixTable.h — the mix in one place. Every sound belongs to a class with a
// target level (its loudest 50 ms, RMS, dBFS); a sound file's trim brings it
// there, and the built voices (VoiceSynth.h) are made at it. Call sites keep
// their volume numbers only as context weights: 128 means "this sound at its
// class's level".
// =============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

enum class MixClass : uint8_t { GUN, FOLEY, PLAYER, ACTION, WORLD, UI, TELL, CHATTER, COUNT };

// Loudest first: guns and the big hits, then tells, the player, UI and the
// world, the mechanical foley, and enemy chatter well below
inline float mixTargetDb(MixClass c) {
    static const float T[(int)MixClass::COUNT] = {-11.f, -18.f, -12.f, -10.f, -13.f, -12.f, -14.f, -22.f};
    return T[(int)c];
}

// The loudest 50 ms of a sound (windows a quarter apart), as RMS in dBFS;
// -120 for silence
inline float shortTermDb(const float* x, size_t n, float rate) {
    if (!x || n == 0) return -120.f;
    const size_t win = std::max<size_t>(1, (size_t)(rate * 0.05f)), hop = std::max<size_t>(1, win / 4);
    double best = 0.0;
    for (size_t s = 0;; s += hop) {
        const size_t e = std::min(n, s + win);
        double sum = 0.0;
        for (size_t i = s; i < e; ++i) sum += (double)x[i] * x[i];
        best = std::max(best, sum / (double)(e - s));
        if (e >= n) break;
    }
    return best > 1e-12 ? 10.f * (float)std::log10(best) : -120.f;
}
