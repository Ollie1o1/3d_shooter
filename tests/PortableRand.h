#pragma once
// One rand() sequence on every platform. The C library's differs between macOS and Linux, so a
// test that seeds it and then expects exact numbers passes on one and fails on the other. Include
// this FIRST in such a test: the game's own rand() calls then draw from this generator instead.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

namespace portable_rand {
inline uint32_t state = 1u;
inline int next() { state = state * 1103515245u + 12345u; return (int)((state >> 16) & 0x7fffu); }
inline void seed(unsigned s) { state = s; }
}
#undef RAND_MAX
#define RAND_MAX 0x7fff
#define rand portable_rand::next
#define srand portable_rand::seed
