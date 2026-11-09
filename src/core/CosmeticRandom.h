#pragma once
#include <cstdint>
#include <random>

// Presentation has its own sequence: particle counts, ambient detail, camera
// motion and sound variations must never consume AI, weapon or loot randomness.
// Gameplay retains its existing rand()/srand() sequence and tuning.
namespace cosmetic {
inline std::mt19937& generator() {
    static std::mt19937 rng{0x434f534du};
    return rng;
}
inline void seed(uint32_t value) { generator().seed(value); }
inline uint32_t next() { return generator()(); }
inline int index(int count) { return count > 0 ? int(next() % uint32_t(count)) : 0; }
inline float range(float lo, float hi) {
    return lo + (hi - lo) * float(next() % 10001u) / 10000.f;
}
}
