#pragma once
// =============================================================================
// RingDetail.h — Chapter 6's surface detail, worked out once instead of per pixel.
//
// The ring's dust and rock used to build their detail from noise in the world shader every
// frame (~4 ms at 4K). These are the same patterns as two tiling textures (no OpenGL here:
// the pixels; Gameplay_Render.h uploads them, mipmapped). Mipmaps are what make it cheap at a
// distance too: up close a pixel reads the fine pattern, far off it reads the pattern's average,
// which is the plain surface: detail fades out on its own, with nothing to compute.
//
//   ground (TILE_GROUND m a tile)  R: the dust's colour variation    G/B: the ripples' slope (x, z)
//                                  A: fine grain
//   rock   (TILE_ROCK m a tile)    R: colour variation   G: cracks   B/A: the relief's slope (x, y)
//
// Every pattern repeats exactly across its tile (whole periods), so the tiles meet without seams.
// =============================================================================
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace ringdetail {
constexpr int SIZE = 512;
constexpr float TILE_GROUND = 16.f, TILE_ROCK = 8.f;

inline float h2(int x, int y, int seed) {
    uint32_t h = (uint32_t)x * 0x8DA6B343u ^ (uint32_t)y * 0xD8163841u ^ (uint32_t)seed * 0xCB1AB31Fu;
    h ^= h >> 13; h *= 0x5BD1E995u; h ^= h >> 15;
    return (float)(h >> 8) * (1.f / 16777216.f);
}
// Value noise on a lattice of `period` cells across the tile (u, v in 0..1): it repeats with the tile
inline float pnoise(float u, float v, int period, int seed) {
    const float x = u * period, y = v * period;
    const int ix = (int)std::floor(x), iy = (int)std::floor(y);
    float fx = x - ix, fy = y - iy;
    fx = fx * fx * (3.f - 2.f * fx); fy = fy * fy * (3.f - 2.f * fy);
    auto at = [&](int i, int j) { return h2(((i % period) + period) % period, ((j % period) + period) % period, seed); };
    const float a = at(ix, iy), b = at(ix + 1, iy), c = at(ix, iy + 1), d = at(ix + 1, iy + 1);
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}
inline float pfbm(float u, float v, int period, int seed) {
    return pnoise(u, v, period, seed) * 0.5f + pnoise(u, v, period * 2, seed + 1) * 0.3f + pnoise(u, v, period * 4, seed + 2) * 0.2f;
}
// Distance to the nearest cell point and the gap to the second (cell edges, where it is small)
inline void pvoronoi(float u, float v, int period, int seed, float& f1, float& edge) {
    const float x = u * period, y = v * period;
    const int ix = (int)std::floor(x), iy = (int)std::floor(y);
    float d1 = 8.f, d2 = 8.f;
    for (int j = -1; j <= 1; ++j)
        for (int i = -1; i <= 1; ++i) {
            const int cx = ix + i, cy = iy + j;
            const int wx = ((cx % period) + period) % period, wy = ((cy % period) + period) % period;
            const float px = cx + h2(wx, wy, seed), py = cy + h2(wx, wy, seed + 7);
            const float d = std::sqrt((px - x) * (px - x) + (py - y) * (py - y));
            if (d < d1) { d2 = d1; d1 = d; } else if (d < d2) d2 = d;
        }
    f1 = d1; edge = d2 - d1;
}
inline uint8_t u8(float x) { return (uint8_t)std::clamp((int)std::lround(x * 255.f), 0, 255); }

// The dust and ice underfoot
inline std::vector<uint8_t> ground() {
    std::vector<uint8_t> px((size_t)SIZE * SIZE * 4);
    const float TAU = 6.28318530718f;
    for (int y = 0; y < SIZE; ++y)
        for (int x = 0; x < SIZE; ++x) {
            const float u = (x + 0.5f) / SIZE, v = (y + 0.5f) / SIZE;
            const float n = pfbm(u, v, 9, 11);
            // ripples: whole waves across the tile (14 one way, 9 the other), bent by the noise
            const float ph = TAU * (14.f * u + 9.f * v) + n * 7.f;
            const float g = pnoise(u, v, 144, 17);
            uint8_t* p = &px[((size_t)y * SIZE + x) * 4];
            p[0] = u8((0.84f + 0.24f * n + 0.07f * (0.5f + 0.5f * std::sin(ph)) + 0.12f * (g - 0.5f)) / 1.4f);
            const float slope = std::cos(ph);   // (the ripple's slope runs along its wave direction)
            p[1] = u8(0.5f + 0.5f * slope * 0.83f);
            p[2] = u8(0.5f + 0.5f * slope * 0.55f);
            p[3] = u8(g);
        }
    return px;
}

// Rock: colour, cracks along cell edges, and the relief's slope
inline std::vector<uint8_t> rock() {
    std::vector<uint8_t> px((size_t)SIZE * SIZE * 4);
    std::vector<float> height((size_t)SIZE * SIZE), crack((size_t)SIZE * SIZE), col((size_t)SIZE * SIZE);
    for (int y = 0; y < SIZE; ++y)
        for (int x = 0; x < SIZE; ++x) {
            const float u = (x + 0.5f) / SIZE, v = (y + 0.5f) / SIZE;
            float f1, edge; pvoronoi(u, v, 6, 31, f1, edge);
            const float fine = pnoise(u, v, 24, 33);
            const size_t i = (size_t)y * SIZE + x;
            crack[i] = std::clamp(1.f - edge / 0.05f, 0.f, 1.f) * (0.6f + 0.4f * fine);
            col[i] = pfbm(u, v, 13, 37);
            height[i] = pnoise(u, v, 22, 41) + f1 * 0.5f - crack[i] * 0.15f;
        }
    const float cellM = TILE_ROCK / SIZE;
    for (int y = 0; y < SIZE; ++y)
        for (int x = 0; x < SIZE; ++x) {
            const size_t i = (size_t)y * SIZE + x;
            auto H = [&](int xx, int yy) { return height[(size_t)((yy + SIZE) % SIZE) * SIZE + (xx + SIZE) % SIZE]; };
            const float dx = (H(x + 1, y) - H(x - 1, y)) / (2.f * cellM), dy = (H(x, y + 1) - H(x, y - 1)) / (2.f * cellM);
            uint8_t* p = &px[i * 4];
            p[0] = u8(col[i]);
            p[1] = u8(crack[i]);
            p[2] = u8(0.5f + std::clamp(dx * 0.08f, -0.5f, 0.5f));   // (slope per metre, scaled into 0..1)
            p[3] = u8(0.5f + std::clamp(dy * 0.08f, -0.5f, 0.5f));
        }
    return px;
}

// ---- the sky's slow parts, painted once (skybox.frag ringSky reads them) ----
// Value noise that wraps every `px` cells along x (around a sphere), not along y
inline float wnoise(float x, float y, int px, int seed) {
    const int ix = (int)std::floor(x), iy = (int)std::floor(y);
    float fx = x - ix, fy = y - iy;
    fx = fx * fx * (3.f - 2.f * fx); fy = fy * fy * (3.f - 2.f * fy);
    auto at = [&](int i, int j) { return h2(((i % px) + px) % px, j, seed); };
    const float a = at(ix, iy), b = at(ix + 1, iy), c = at(ix, iy + 1), d = at(ix + 1, iy + 1);
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}
inline float wfbm(float x, float y, int px, int seed) {   // four octaves, each still wrapping
    float s = 0.f, a = 0.5f;
    for (int k = 0; k < 4; ++k) { s += a * wnoise(x, y, px, seed + k); x *= 2.f; y *= 2.03f; px *= 2; a *= 0.5f; }
    return s;
}
inline float sstep(float a, float b, float x) { const float t = std::clamp((x - a) / (b - a), 0.f, 1.f); return t * t * (3.f - 2.f * t); }

constexpr int GIANT_W = 2048, GIANT_H = 1024;
// The gas giant's cloud tops, unlit: x longitude (0..1 round it), y latitude angle (south pole to north).
// The belts, their curls, the fine lanes, the rust, the storm and its wake. The shader lights it, drifts
// each latitude at its own pace by where it reads, and adds the pole's hexagon and the rings' shadow
inline std::vector<uint8_t> giant() {
    std::vector<uint8_t> px((size_t)GIANT_W * GIANT_H * 4);
    const float PI = 3.14159265359f, TAU = 6.28318530718f;
    const float butter[3] = {0.92f, 0.8f, 0.56f}, tan_[3] = {0.66f, 0.5f, 0.33f}, cream[3] = {0.97f, 0.93f, 0.82f}, rust[3] = {0.52f, 0.32f, 0.2f};
    for (int j = 0; j < GIANT_H; ++j) {
        const float phi = ((j + 0.5f) / GIANT_H - 0.5f) * PI, lat = std::sin(phi);
        for (int i = 0; i < GIANT_W; ++i) {
            const float u = (i + 0.5f) / GIANT_W, lon = (u - 0.5f) * TAU;
            // (the same recipe as the shader had, on lattices that close round the planet)
            const float bx = u * 19.f, by = lat * 15.f;
            const float w0 = wfbm(bx * 1.3f, by * 1.3f, 25, 101), w1 = wfbm(bx * 1.3f + 5.2f * 0.f, by * 1.3f + 5.2f, 25, 111);
            const float swirl = wfbm(bx + 1.7f * w0 * 3.f, by + 1.7f * w1, 19, 121);
            const float swirl2 = wfbm(u * 69.f + 2.2f * w0 * 3.f, lat * 46.f + 2.2f * w1, 69, 131);
            const float bandsP = 0.5f + 0.5f * std::sin(lat * 34.f + swirl * 4.5f + swirl2 * 1.4f);
            const float belts = 0.5f + 0.5f * std::sin(lat * 11.f + 0.8f + swirl * 0.8f);
            const float hair = 0.5f + 0.5f * std::sin(lat * 130.f + swirl2 * 6.f);
            float c[3];
            for (int k = 0; k < 3; ++k) {
                float v = tan_[k] + (butter[k] - tan_[k]) * bandsP;
                v += (cream[k] - v) * sstep(0.55f, 0.9f, belts) * 0.55f;
                v += (rust[k] - v) * sstep(0.6f, 0.8f, swirl2) * 0.4f * sstep(0.1f, 0.3f, std::fabs(lat));
                c[k] = v * (0.9f + 0.12f * hair);
            }
            // The storm: a pale oval in the northern belts, turning, a dark wake behind it
            float dl = lon - 2.95f; dl -= TAU * std::round(dl / TAU);
            const float sx = dl * 3.2f, sy = (lat - 0.32f) * 7.f, sr = std::sqrt(sx * sx + sy * sy);
            const float storm = sstep(1.f, 0.3f, sr);
            const float swirlS = 0.5f + 0.2f * std::sin(std::atan2(sy, sx) * 3.f + sr * 9.f);
            const float wx = sx - 1.3f, wake = sstep(1.6f, 1.f, std::sqrt(wx * wx + sy * sy)) * (1.f - storm);
            const float pale[3] = {1.f, 0.94f, 0.8f};
            uint8_t* p = &px[((size_t)j * GIANT_W + i) * 4];
            for (int k = 0; k < 3; ++k) {
                float v = c[k] + (pale[k] - c[k]) * storm * swirlS;
                v *= 1.f - 0.25f * wake;
                p[k] = u8(v);
            }
            p[3] = 255;
        }
    }
    return px;
}

constexpr int GALAXY_W = 1024, GALAXY_H = 256;
// The galaxy's dust (the shader's fbm(az * 5, y * 7)): x azimuth round the sky, y the direction's height (-1..1)
inline std::vector<uint8_t> galaxy() {
    std::vector<uint8_t> px((size_t)GALAXY_W * GALAXY_H * 4);
    for (int j = 0; j < GALAXY_H; ++j)
        for (int i = 0; i < GALAXY_W; ++i) {
            const float u = (i + 0.5f) / GALAXY_W, y = ((j + 0.5f) / GALAXY_H) * 2.f - 1.f;
            const float n = wfbm(u * 31.f, y * 7.f, 31, 201);
            uint8_t* p = &px[((size_t)j * GALAXY_W + i) * 4];
            p[0] = p[1] = p[2] = u8(n); p[3] = 255;
        }
    return px;
}
}   // namespace ringdetail
