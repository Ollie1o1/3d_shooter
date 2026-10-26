#pragma once
// =============================================================================
// MaterialTex.h — Act I's own surfaces, one look per arena, as RGBA pixels
// (no OpenGL: TextureGen uploads them, and they can be dumped to look at).
//
// Unlike the older textures these wrap: every noise repeats exactly at the
// tile edge, so the 4 m repeat doesn't show as a seam. A texel's alpha below
// 255 marks a part that glows (circuit traces, carved runes): the world shader
// adds uTexGlowColor there, scaled by 1 - alpha.
//
// 256 px a tile = 64 px a metre. Each is scaled to a mean brightness near the
// old brick's, so the arenas' colours (which were tuned on brick) still hold.
// =============================================================================
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace MaterialTex {

constexpr int SIZE = 256;   // a 4 m tile
constexpr int M = SIZE / 4; // pixels a metre

inline float hashI(int x, int y, int seed) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + (uint32_t)seed * 2246822519u + 0x9E3779B9u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFFF) / 16777215.f;
}
inline int wrap(int a, int n) { return ((a % n) + n) % n; }
inline float smooth(float t) { return t * t * (3.f - 2.f * t); }
inline float sstep(float a, float b, float x) { float t = glm::clamp((x - a) / (b - a), 0.f, 1.f); return t * t * (3.f - 2.f * t); }

// Value noise over the tile (u, v in 0..1) with cx by cy cells, wrapping at the edge
inline float noise(float u, float v, int cx, int cy, int seed) {
    float x = u * cx, y = v * cy;
    int ix = (int)std::floor(x), iy = (int)std::floor(y);
    float fx = smooth(x - ix), fy = smooth(y - iy);
    auto H = [&](int a, int b) { return hashI(wrap(a, cx), wrap(b, cy), seed); };
    float a = H(ix, iy), b = H(ix + 1, iy), c = H(ix, iy + 1), d = H(ix + 1, iy + 1);
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}
// Octaves of it, 0..1 (mean 0.5)
inline float fbm(float u, float v, int cells, int octaves, int seed) {
    float s = 0.f, amp = 0.5f, norm = 0.f;
    for (int o = 0; o < octaves; ++o) {
        s += noise(u, v, cells << o, cells << o, seed + o * 17) * amp;
        norm += amp; amp *= 0.5f;
    }
    return s / norm;
}

struct Px { glm::vec3 c; float glow = 0.f; };   // colour 0..1, glow 0..1

// Fill a tile from f(x, y) -> Px, scale it to `mean` brightness (glow left out), pack it
template <class F>
inline std::vector<unsigned char> build(F f, float mean) {
    std::vector<Px> px((size_t)SIZE * SIZE);
    double sum = 0.0;
    for (int y = 0; y < SIZE; ++y)
        for (int x = 0; x < SIZE; ++x) {
            Px p = f(x, y);
            px[(size_t)y * SIZE + x] = p;
            sum += (p.c.r + p.c.g + p.c.b) / 3.f;
        }
    float k = mean / (float)(sum / (SIZE * SIZE));
    std::vector<unsigned char> out((size_t)SIZE * SIZE * 4);
    for (size_t i = 0; i < px.size(); ++i) {
        glm::vec3 c = glm::clamp(px[i].c * k, 0.f, 1.f);
        out[i * 4 + 0] = (unsigned char)(c.r * 255.f + 0.5f);
        out[i * 4 + 1] = (unsigned char)(c.g * 255.f + 0.5f);
        out[i * 4 + 2] = (unsigned char)(c.b * 255.f + 0.5f);
        out[i * 4 + 3] = (unsigned char)((1.f - glm::clamp(px[i].glow, 0.f, 1.f)) * 255.f + 0.5f);
    }
    return out;
}

constexpr float BRICK_MEAN = 0.36f;   // what the old brick averages: the arena colours were tuned on it

// --- SUNSET YARD -------------------------------------------------------------

// Sun-baked plaster: mottled, trowel-streaked, hairline cracks, and here and
// there a patch fallen away to the mud brick under it
inline std::vector<unsigned char> adobe() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        float b = 0.5f + (fbm(u, v, 4, 5, 1) - 0.5f) * 0.30f;
        b += (noise(u, v, 2, 40, 2) - 0.5f) * 0.06f;                 // trowel strokes (long, flat)
        b += (hashI(x, y, 3) - 0.5f) * 0.035f;                       // grit
        float crack = std::fabs(fbm(u, v, 3, 4, 4) - 0.5f);
        float where = sstep(0.5f, 0.68f, noise(u, v, 3, 3, 5));
        if (crack < 0.010f) b *= 1.f - 0.45f * where * (1.f - crack / 0.010f);
        glm::vec3 c = b * glm::vec3{1.04f, 0.97f, 0.90f};
        float spall = fbm(u, v, 4, 3, 6);
        if (spall > 0.735f) {                                         // plaster gone: brick under it
            int row = y / 12, bx = (x + (row & 1) * 12) % 24, by = y % 12;
            bool mortar = bx < 2 || by < 2;
            float br = mortar ? 0.30f : 0.40f + (hashI(x / 24, row, 7) - 0.5f) * 0.10f;
            c = br * glm::vec3{1.15f, 0.82f, 0.66f};
            if (spall < 0.75f) c *= 0.7f;                            // the broken edge
        } else if (spall > 0.71f) c *= 0.9f;                         // the plaster lifting around it
        return Px{c};
    }, BRICK_MEAN * 1.25f);
}

// Terracotta floor tiles, a metre each: grout, tile-to-tile tone, chips, a
// few cracked, sand drifted over it
inline std::vector<unsigned char> terracotta() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        int tx = x / M, ty = y / M, lx = x % M, ly = y % M;
        float tone = 0.9f + 0.22f * hashI(tx, ty, 11);
        float b = 0.55f * tone + (fbm(u, v, 8, 3, 12) - 0.5f) * 0.14f;
        if (lx < 4 || ly < 4) b *= 1.07f;
        if (lx > M - 4 || ly > M - 4) b *= 0.88f;
        glm::vec3 c = b * glm::vec3{1.10f, 0.90f, 0.76f};
        // A chipped corner shows the grout bed
        bool chip = hashI(tx, ty, 13) < 0.3f && (lx - 3) * (lx - 3) + (ly - 3) * (ly - 3) < 40;
        if (lx < 2 || ly < 2 || chip) c = glm::vec3{0.40f, 0.37f, 0.33f};
        else if (hashI(tx, ty, 14) < 0.15f) {                         // a cracked tile
            float d = std::fabs((float)(lx - ly) + (noise(u, v, 16, 16, 15) - 0.5f) * 14.f);
            if (d < 0.9f) c *= 0.55f;
        }
        float sand = sstep(0.5f, 0.72f, fbm(u, v, 2, 4, 16));
        c = glm::mix(c, glm::vec3{0.62f, 0.56f, 0.46f} * (0.9f + 0.2f * hashI(x, y, 17)), sand * 0.65f);
        return Px{c};
    }, BRICK_MEAN * 1.35f);
}

// Rough planks: grain along them, staggered ends, a nail at each
inline std::vector<unsigned char> wood() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        const int PH = 16;                                            // a 25 cm plank
        int row = y / PH, ly = y % PH;
        int off = (int)(hashI(row, 0, 21) * 128.f);
        int lx = (x + off) % 128;
        float tone = 0.85f + 0.3f * hashI(row, ((x + off) / 128) % 2, 22);
        float grain = noise(u, v, 3, 96, 23) * 0.6f + noise(u, v, 8, 160, 24) * 0.4f;
        float b = 0.5f * tone * (0.82f + 0.36f * grain);
        if (ly == 0) b = 0.12f;                                       // gap between planks
        else if (ly == 1) b *= 0.8f;
        if (lx < 1) b = 0.14f;                                        // a plank's end
        if ((lx == 5 || lx == 122) && (ly == 5 || ly == 11)) b = 0.22f;   // nails
        return Px{b * glm::vec3{1.10f, 0.90f, 0.70f}};
    }, BRICK_MEAN * 1.2f);
}

// --- THE FOUNDRY -------------------------------------------------------------

// Riveted iron plates (2 x 1 m), sooty streaks, rust coming through
inline std::vector<unsigned char> plate() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        const int W = 2 * M, H = M;
        int row = y / H, ly = y % H;
        int lx = (x + (row & 1) * M) % W, col = ((x + (row & 1) * M) / W) % (SIZE / W);
        float b = 0.40f + (fbm(u, v, 8, 4, 31) - 0.5f) * 0.14f + (hashI(col, row, 32) - 0.5f) * 0.10f;
        float soot = noise(u, v, 48, 2, 33);
        b *= 1.f - 0.30f * sstep(0.5f, 0.85f, soot);
        if (lx < 4 || ly > H - 4) b *= 1.22f;                         // bevel: lit edge
        if (lx > W - 4 || ly < 4) b *= 0.72f;                         // and shaded
        glm::vec3 c = b * glm::vec3{0.98f, 0.98f, 1.02f};
        float rust = sstep(0.58f, 0.74f, fbm(u, v, 4, 4, 34));
        c = glm::mix(c, (b / 0.4f) * glm::vec3{0.50f, 0.26f, 0.14f}, rust * 0.85f);
        if (lx < 2 || ly < 2) c = glm::vec3{0.08f};                   // seam
        // Rivets along the long edges, every quarter metre
        for (int ry : {7, H - 7}) {
            int rx = ((lx + 8) % 16) - 8;
            float d2 = (float)(rx * rx + (ly - ry) * (ly - ry));
            if (d2 < 6.f) c = glm::vec3{0.62f, 0.60f, 0.58f} * (ly > ry ? 1.f : 0.7f);
            else if (d2 < 10.f && ly < ry) c *= 0.6f;                 // its shadow
        }
        return Px{c};
    }, BRICK_MEAN * 1.05f);
}

// Diamond tread plate in 2 m panels, oil-stained, worn bright where it's walked
inline std::vector<unsigned char> tread() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        float b = 0.38f + (fbm(u, v, 8, 4, 41) - 0.5f) * 0.16f;
        float wear = fbm(u, v, 4, 3, 42);
        const int C = 16;
        int cx = x / C, cy = y / C;
        float lx = (float)(x % C) - C * 0.5f + 0.5f, ly = (float)(y % C) - C * 0.5f + 0.5f;
        float s = ((cx + cy) & 1) ? 1.f : -1.f;                      // alternate the diamonds' lean
        float a = (lx + s * ly) * 0.7071f, d = (lx - s * ly) * 0.7071f;
        if (std::fabs(a) < 5.f && std::fabs(d) < 1.4f) b *= (d * s < 0.f ? 1.30f : 0.85f) + wear * 0.25f;
        float oil = sstep(0.60f, 0.72f, fbm(u, v, 3, 4, 43));
        b *= 1.f - 0.45f * oil;
        int px = x % (2 * M), py = y % (2 * M);
        if (px < 2 || py < 2) b = 0.08f;
        return Px{b * glm::vec3{0.97f, 0.97f, 1.0f}};
    }, BRICK_MEAN * 1.0f);
}

// --- THE SPIRE ---------------------------------------------------------------

// Veined marble ashlar: 1 x 0.5 m blocks, fine joints, each block its own tone
inline std::vector<unsigned char> marble() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        const int W = M, H = M / 2;
        int row = y / H, ly = y % H;
        int lx = (x + (row & 1) * (W / 2)) % W, col = ((x + (row & 1) * (W / 2)) / W) % (SIZE / W);
        float b = 0.64f + (hashI(col, row, 51) - 0.5f) * 0.10f + (fbm(u, v, 4, 4, 52) - 0.5f) * 0.10f;
        float t = u * 4.f + v * 2.f + fbm(u, v, 4, 5, 53) * 3.2f;
        float vein = 1.f - sstep(0.f, 0.05f, std::fabs(std::sin(t * 3.14159f)));
        float t2 = u * 2.f - v * 6.f + fbm(u, v, 8, 3, 54) * 2.f;
        float vein2 = 1.f - sstep(0.f, 0.03f, std::fabs(std::sin(t2 * 3.14159f)));
        b *= 1.f - 0.34f * vein - 0.16f * vein2;
        glm::vec3 c = b * glm::vec3{0.98f, 0.98f, 1.02f};
        if (lx < 1 || ly < 1) c = glm::vec3{0.30f};
        else if (lx < 2 || ly < 2) c *= 1.12f;                        // the arris catches the light
        return Px{c};
    }, BRICK_MEAN * 1.35f);
}

// Flagstones: rows a metre deep cut into 1 or 2 m slabs, faint veining, worn smooth
inline std::vector<unsigned char> flagstone() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        int row = y / M, ly = y % M;
        // Where this row's slabs end: always at the tile edge, else wherever the hash says
        int start = 0;
        for (int k = x / M; k > 0; --k) if (hashI(k, row, 61) < 0.55f) { start = k; break; }
        int lx = x - start * M;
        bool edge = lx < 2 || ly < 2;
        float b = 0.55f + (hashI(start, row, 62) - 0.5f) * 0.14f + (fbm(u, v, 8, 4, 63) - 0.5f) * 0.12f;
        float vein = 1.f - sstep(0.f, 0.04f, std::fabs(std::sin((u * 3.f + fbm(u, v, 4, 4, 64) * 2.5f) * 3.14159f)));
        b *= 1.f - 0.12f * vein;
        if (lx < 4 || ly < 4) b *= 0.92f;
        glm::vec3 c = b * glm::vec3{0.98f, 0.98f, 1.0f};
        if (edge) c = glm::vec3{0.16f};
        return Px{c};
    }, BRICK_MEAN * 1.3f);
}

// --- THE CORE ----------------------------------------------------------------

// Dark 2 m panels laid with circuit traces that glow, pads at their ends
inline std::vector<unsigned char> circuit() {
    const int G = 8;                     // the routing grid
    const int N = SIZE / G;
    // A trace runs along a row (or column) of grid nodes in runs of 4 (or 3)
    auto hEdge = [&](int nx, int ny) { nx = wrap(nx, N); ny = wrap(ny, N); return hashI(nx / 4, ny, 71) < 0.2f && (nx % 16) != 15; };
    auto vEdge = [&](int nx, int ny) { nx = wrap(nx, N); ny = wrap(ny, N); return hashI(nx, ny / 3, 72) < 0.13f && (ny % 16) != 15 && !hEdge(nx, ny); };
    return build([&](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        float b = 0.26f + (fbm(u, v, 8, 3, 73) - 0.5f) * 0.06f;
        int px = x % (2 * M), py = y % (2 * M);
        if (px < 3 || py < 3) b *= 1.25f;
        if (px > 2 * M - 3 || py > 2 * M - 3) b *= 0.6f;
        glm::vec3 c = b * glm::vec3{0.92f, 0.98f, 1.06f};
        float glow = 0.f;
        int nx = (x + G / 2) / G, ny = (y + G / 2) / G;               // nearest node
        int lx = x - nx * G, ly = y - ny * G;                         // from it, -4..3
        bool onH = (ly == 0 || ly == -1) && ((lx >= 0 && hEdge(nx, ny)) || (lx < 0 && hEdge(nx - 1, ny)));
        bool onV = (lx == 0 || lx == -1) && ((ly >= 0 && vEdge(nx, ny)) || (ly < 0 && vEdge(nx, ny - 1)));
        int links = hEdge(nx, ny) + hEdge(nx - 1, ny) + vEdge(nx, ny) + vEdge(nx, ny - 1);
        bool pad = links == 1 && std::abs(lx) <= 1 && std::abs(ly) <= 1;
        if (onH || onV) { c = glm::vec3{0.42f, 0.48f, 0.52f}; glow = 0.35f; }
        if (pad)        { c = glm::vec3{0.55f, 0.60f, 0.62f}; glow = 0.9f; }
        if (px < 2 || py < 2) { c = glm::vec3{0.05f}; glow = 0.f; }
        return Px{c, glow};
    }, BRICK_MEAN * 0.8f);
}

// Floor plates a metre square: a raised middle, vents, and seams that glow
inline std::vector<unsigned char> circuitFloor() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        int lx = x % M, ly = y % M;
        float b = 0.30f + (fbm(u, v, 8, 3, 81) - 0.5f) * 0.08f;
        float glow = 0.f;
        bool inset = lx > 14 && lx < M - 14 && ly > 14 && ly < M - 14;
        if (inset) b *= 0.85f;
        if ((lx == 14 || ly == 14) && lx >= 14 && ly >= 14 && lx <= M - 14 && ly <= M - 14) b *= 1.3f;
        if ((lx == M - 14 || ly == M - 14) && lx >= 14 && ly >= 14 && lx <= M - 14 && ly <= M - 14) b *= 0.6f;
        // Vent slits in two corners
        for (int k = 0; k < 3; ++k) {
            int sy = 5 + k * 3;
            if (ly == sy && lx > 4 && lx < 11) b = 0.08f;
            if (ly == M - 1 - sy && lx > M - 11 && lx < M - 4) b = 0.08f;
        }
        glm::vec3 c = b * glm::vec3{0.92f, 0.98f, 1.06f};
        if (lx < 1 || ly < 1) { c = glm::vec3{0.45f, 0.50f, 0.55f}; glow = 0.55f; }
        else if (lx < 3 || ly < 3 || lx > M - 3 || ly > M - 3) c *= 0.45f;
        return Px{c, glow};
    }, BRICK_MEAN * 0.8f);
}

// --- THE SANCTUM -------------------------------------------------------------

// Rough-cut basalt blocks (2 x 1 m), chisel-pocked, chipped at the joints;
// one block in six carries a carved rune that smoulders
inline std::vector<unsigned char> basalt() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        const int W = 2 * M, H = M;
        int row = y / H, ly = y % H;
        int sx = x + (row & 1) * M, lx = sx % W, col = (sx / W) % (SIZE / W);
        float b = 0.42f + (hashI(col, row, 91) - 0.5f) * 0.12f + (fbm(u, v, 4, 4, 92) - 0.5f) * 0.14f;
        float pock = noise(u, v, 96, 96, 93);
        b *= 0.86f + 0.22f * pock;
        // Joints: ragged, chipped wider in places
        float jw = 2.f + 3.f * sstep(0.55f, 0.8f, noise(u, v, 32, 32, 94));
        bool joint = lx < jw || ly < jw;
        glm::vec3 c = b * glm::vec3{1.0f, 0.96f, 0.98f};
        float glow = 0.f;
        if (joint) c = glm::vec3{0.07f};
        else if (lx < jw + 3 || ly < jw + 3) c *= 0.8f;
        // The rune: strokes between points of a 3 x 3 lattice in the block's middle
        if (!joint && hashI(col, row, 95) < 0.16f) {
            const float S = 13.f;
            float ox = W * 0.5f - S, oy = H * 0.5f - S;
            float fx = (float)lx, fy = (float)ly;
            for (int k = 0; k < 5; ++k) {
                int a = (int)(hashI(col * 7 + k, row, 96) * 9.f), e = (int)(hashI(col * 7 + k, row, 97) * 9.f);
                if (a == e) continue;
                glm::vec2 p0{ox + (a % 3) * S, oy + (a / 3) * S}, p1{ox + (e % 3) * S, oy + (e / 3) * S};
                glm::vec2 d = p1 - p0, q{fx - p0.x, fy - p0.y};
                float t = glm::clamp(glm::dot(q, d) / glm::dot(d, d), 0.f, 1.f);
                float dist = glm::length(q - d * t);
                if (dist < 1.6f) { c = glm::vec3{0.12f, 0.06f, 0.05f}; glow = 0.85f; }
                else if (dist < 2.6f) c *= 0.6f;
            }
        }
        return Px{c, glow};
    }, BRICK_MEAN * 1.0f);
}

} // namespace MaterialTex
