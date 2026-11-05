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

// The Living Ward, after its concept boards: glazed white tiles in a dark grout
// grid. Walls: 1 m porcelain tiles with a deeper seam every 2 m. Floors: 1 m
// bevelled tiles, each a slightly different white, a few of them cracked.
inline std::vector<unsigned char> clinic() {
    return build([](int x,int y) {
        int tx=x/64,ty=y/64,u=x%64,v=y%64;
        float b=.8f+(hashI(tx,ty,904)-.5f)*.035f+(hashI(x,y,901)-.5f)*.016f;
        if(u<1 || v<1)b=.6f;                         // grout
        else if(u<2 || v<2)b+=.035f;                 // the glaze catching light on the tile's edge
        else if(u>61 || v>61)b-=.025f;
        if(x%128<1 || y%128<1)b=.46f;                // panel seams
        return Px{glm::vec3{b*.985f,b,b*1.015f}};
    },.72f);
}
inline std::vector<unsigned char> lino() {
    return build([](int x,int y) {
        int tx=x/64,ty=y/64,u=x%64,v=y%64;
        float tint=(hashI(tx,ty,905)-.5f)*.05f;
        float b=.8f+tint+(hashI(x,y,902)-.5f)*.02f;
        if(u<2 || v<2)b=.42f;                        // a dark grout line every metre
        else if(u<3 || v<3)b+=.06f;                  // bevel: lit edge
        else if(u>60 || v>60)b-=.05f;                // and its shadowed side
        // One tile in a dozen is cracked: a jagged dark line from a corner
        if(hashI(tx,ty,906)>.9f){
            float k=hashI(tx,ty,907)*2-1;
            float along=(float)u,line=(float)v-(32+k*18)-std::sin(u*.35f+k*4)*3-(u-32)*k*.6f;
            if(along>3 && along<61 && std::fabs(line)<.9f)b-=.28f;
        }
        if(hashI(x,y,903)>.993f)b-=.08f;             // grit
        return Px{glm::vec3{b*.99f,b,b*1.012f}};
    },.68f);
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

// --- THE BLOOD MOON PLAIN (Chapter 4) ----------------------------------------
// Cinder crust: a dry, scorched skin broken into plates (a wrapping Voronoi of
// irregular cells, about 0.7 m), dark cracks between them, and in some cracks
// the heat underneath still showing (glow). Ash settles in the low plates.
inline std::vector<unsigned char> cinder() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        const int N = 6;   // cells across the 4 m tile
        float fx = u * N, fy = v * N;
        int ix = (int)std::floor(fx), iy = (int)std::floor(fy);
        float d1 = 9.f, d2 = 9.f; int best = 0;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                int cx = ix + dx, cy = iy + dy, wx = wrap(cx, N), wy = wrap(cy, N);
                float px = cx + 0.15f + 0.7f * hashI(wx, wy, 501), py = cy + 0.15f + 0.7f * hashI(wx, wy, 502);
                float d = std::hypot(fx - px, fy - py);
                if (d < d1) { d2 = d1; d1 = d; best = wx * 31 + wy; } else if (d < d2) d2 = d;
            }
        float edge = d2 - d1;                                    // 0 on a crack
        float plate = hashI(best, 7, 503);
        float b = 0.34f + (plate - 0.5f) * 0.14f + (fbm(u, v, 8, 4, 504) - 0.5f) * 0.18f;
        b *= 0.85f + 0.3f * noise(u, v, 128, 128, 505);           // grit
        glm::vec3 c = b * glm::vec3{1.f, 0.86f, 0.8f};
        float ash = sstep(0.55f, 0.75f, fbm(u, v, 4, 3, 506));
        c = glm::mix(c, glm::vec3{0.5f, 0.47f, 0.45f}, ash * 0.45f);
        float glow = 0.f;
        float crack = 0.035f + 0.03f * noise(u, v, 24, 24, 507);
        if (edge < crack) {
            c = glm::vec3{0.05f, 0.03f, 0.03f};
            // Heat shows in a few cracks only, and only where the crack runs deep
            float deep = noise(u, v, 12, 12, 509);
            if (hashI(best, 11, 508) < 0.14f && deep > 0.45f && edge < crack * 0.5f) { c = glm::vec3{0.25f, 0.06f, 0.02f}; glow = 0.45f + 0.4f * sstep(0.45f, 0.8f, deep); }
        } else if (edge < crack * 2.2f) c *= 0.7f;               // the plate's edge curls down into it
        return Px{c, glow};
    }, BRICK_MEAN * 0.8f);
}

// --- THE COLD STORE (Chapter 5) ------------------------------------------------
// Packed snow: wind-scoured and bright, rippled into long low sastrugi that run
// one way across the tile, with a faint sparkle of grains.
inline std::vector<unsigned char> snow() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        float ripple = std::sin((v * 7.f + fbm(u, v, 4, 4, 601) * 1.6f) * 6.2832f);   // wind ridges (wrap: whole cycles)
        float b = 0.8f + 0.06f * ripple + (fbm(u, v, 8, 4, 602) - 0.5f) * 0.12f;
        b *= 0.94f + 0.08f * noise(u, v, 128, 128, 603);                               // grain
        if (hashI(x, y, 604) > 0.996f) b = 1.f;                                        // a glint
        glm::vec3 c = b * glm::vec3{0.93f, 0.96f, 1.f};
        float drift = sstep(0.6f, 0.8f, fbm(u, v, 3, 3, 605));                         // blue in the hollows
        c = glm::mix(c, glm::vec3{0.62f, 0.72f, 0.84f}, drift * 0.3f + (ripple < -0.6f ? 0.12f : 0.f));
        return Px{c, 0.f};
    }, BRICK_MEAN * 2.1f);
}

// Clear sea ice: dark (you see into it), cut with white fracture lines and
// trapped bubbles, frost dusted in streaks over the top.
inline std::vector<unsigned char> ice() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        const int N = 3;   // big fracture cells across the 4 m tile
        float fx = u * N, fy = v * N;
        int ix = (int)std::floor(fx), iy = (int)std::floor(fy);
        float d1 = 9.f, d2 = 9.f;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                int cx = ix + dx, cy = iy + dy, wx = wrap(cx, N), wy = wrap(cy, N);
                float px = cx + 0.1f + 0.8f * hashI(wx, wy, 611), py = cy + 0.1f + 0.8f * hashI(wx, wy, 612);
                float d = std::hypot(fx - px, fy - py);
                if (d < d1) { d2 = d1; d1 = d; } else if (d < d2) d2 = d;
            }
        float depth = fbm(u, v, 4, 4, 613);
        glm::vec3 c = glm::mix(glm::vec3{0.08f, 0.16f, 0.24f}, glm::vec3{0.22f, 0.36f, 0.48f}, depth);
        float edge = d2 - d1;
        if (edge < 0.025f) c = glm::mix(c, glm::vec3{0.85f, 0.92f, 1.f}, 0.8f);         // a fracture, white
        else if (edge < 0.06f) c = glm::mix(c, glm::vec3{0.5f, 0.65f, 0.78f}, 0.35f);
        if (hashI(x / 3, y / 3, 614) > 0.992f) c = glm::vec3{0.75f, 0.85f, 0.95f};        // a bubble
        float frost = sstep(0.5f, 0.8f, fbm(u, v, 6, 2, 615));          // streaks of frost
        c = glm::mix(c, glm::vec3{0.82f, 0.88f, 0.94f}, frost * 0.45f);
        return Px{c, 0.f};
    }, BRICK_MEAN * 1.1f);
}

// --- The story chapters' own -------------------------------------------------
// Each chapter restyles the shared surfaces (LevelData::restyle) so its rooms
// are their own place: THE CRADLE's worn factory, THE DEAD SPAN's canyon and
// broken road, THE COLD STORE's frosted steel. Walls: pixel rows run up the
// wall (row 0 is the bottom of the 4 m tile), so drips and rust run toward 0.

// A wrapping Voronoi: the nearest and second-nearest of N x N jittered points
struct Cell { float d1, d2; int id; };
inline Cell voronoi(float u, float v, int N, int seed, float jitter = 0.8f) {
    float fx = u * N, fy = v * N;
    int ix = (int)std::floor(fx), iy = (int)std::floor(fy);
    Cell c{9.f, 9.f, 0};
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx) {
            int cx = ix + dx, cy = iy + dy, wx = wrap(cx, N), wy = wrap(cy, N);
            float px = cx + 0.5f + (hashI(wx, wy, seed) - 0.5f) * jitter, py = cy + 0.5f + (hashI(wx, wy, seed + 1) - 0.5f) * jitter;
            float d = std::hypot(fx - px, fy - py);
            if (d < c.d1) { c.d2 = c.d1; c.d1 = d; c.id = wx * 131 + wy; } else if (d < c.d2) c.d2 = d;
        }
    return c;
}

// THE CRADLE (Chapter 1) ------------------------------------------------------

// Hull steel: 2 m plates in dark blue-grey paint, a weld bead across each, the
// paint worn to bare steel at the edges, grime run down the face and rust
// weeping from some rivet lines
inline std::vector<unsigned char> hull() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        const int W = 2 * M, H = 2 * M;
        int col = x / W, row = y / H, lx = x % W, ly = y % H;
        float b = 0.36f + (hashI(col, row, 701) - 0.5f) * 0.08f + (fbm(u, v, 8, 4, 702) - 0.5f) * 0.12f;
        b *= 1.f - 0.30f * sstep(0.55f, 0.82f, noise(u, v, 64, 3, 703));          // grime run down the face
        b *= 0.94f + 0.12f * noise(u, v, 128, 128, 708);                          // tooth of the paint
        glm::vec3 c = b * glm::vec3{0.86f, 0.93f, 1.0f};
        float edge = std::min(std::min(lx, W - 1 - lx), std::min(ly, H - 1 - ly)) / 12.f;
        float wear = fbm(u, v, 16, 3, 704);
        if (wear > 0.56f + 0.30f * std::min(edge, 1.f))                         // chipped to bare steel
            c = glm::vec3{0.50f, 0.49f, 0.47f} * (0.8f + 0.4f * hashI(x, y, 705));
        int wy = ly - H / 2;                                                      // the weld bead
        if (std::abs(wy) <= 2) c = glm::vec3{0.30f, 0.29f, 0.28f} * (wy > 0 ? 1.3f : 0.8f) * (0.8f + 0.25f * (0.5f + 0.5f * std::sin(lx * 1.4f)));
        else if (std::abs(wy) <= 5) c *= glm::vec3{0.86f, 0.82f, 0.80f};          // its heat tint
        for (int side = 0; side < 2; ++side) {                                    // rivets down each edge
            int rx = side ? W - 7 : 6, dx = lx - rx;
            int ry = (ly / 16) * 16 + 8, dy = ly - ry;
            bool weeps = hashI(col * 2 + side, row, 706) < 0.45f;
            int dyA = ly - (ry > ly ? ry : ry + 16);                              // below the rivet above
            if (weeps && dyA < -2 && ry + 16 < H) {                               // rust weeping down from it
                float t = -dyA / 16.f;
                if (std::abs(dx) <= 1 + (int)(t * 2.f) && noise(u, v, 64, 32, 707) > 0.3f)
                    c = glm::mix(c, (b / 0.36f) * glm::vec3{0.52f, 0.27f, 0.13f}, 0.65f * (1.f - t));
            }
            int d2 = dx * dx + dy * dy;
            if (d2 < 5) c = glm::vec3{0.58f, 0.57f, 0.55f} * (dy > 0 ? 1.f : 0.65f);
            else if (d2 < 9 && dy < 0) c *= 0.55f;
        }
        if (lx < 2 || ly < 2) c = glm::vec3{0.06f};                               // seam
        else if (lx < 4 || ly > H - 4) c *= 1.18f;
        else if (lx > W - 4 || ly < 4) c *= 0.72f;
        return Px{c};
    }, BRICK_MEAN * 1.0f);
}

// Deck plate: the factory's metal floors, 2 m plates of three-bar tread, worn
// bright down the walked lines, oil soaked in, a countersunk bolt in each corner
inline std::vector<unsigned char> deck() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        float b = 0.32f + (fbm(u, v, 8, 4, 711) - 0.5f) * 0.14f;
        float wear = sstep(0.45f, 0.7f, fbm(u, v, 3, 4, 712));
        const int C = 16;
        int cx = x / C, cy = y / C;
        float lx = (float)(x % C) - C * 0.5f + 0.5f, ly = (float)(y % C) - C * 0.5f + 0.5f;
        float s = ((cx + cy) & 1) ? 1.f : -1.f;
        float a = (lx + s * ly) * 0.7071f, d = (lx - s * ly) * 0.7071f;
        for (int k = -1; k <= 1; ++k)
            if (std::fabs(a) < 4.5f && std::fabs(d - k * 3.6f) < 0.9f) b *= (d - k * 3.6f < 0.f ? 1.35f : 0.8f) + wear * 0.45f;
        float oil = fbm(u, v, 5, 4, 713);
        b *= 1.f - 0.32f * sstep(0.64f, 0.74f, oil);                              // oil soaked in
        glm::vec3 c = b * glm::vec3{0.96f, 0.97f, 1.0f};
        c = glm::mix(c, c * glm::vec3{1.25f, 0.85f, 0.65f}, 0.5f * sstep(0.62f, 0.8f, fbm(u, v, 6, 3, 714)));   // rust bloom
        int px = x % (2 * M), py = y % (2 * M);
        for (int bx : {7, 2 * M - 8}) for (int by : {7, 2 * M - 8}) {
            int d2 = (px - bx) * (px - bx) + (py - by) * (py - by);
            if (d2 < 7) c = glm::vec3{0.16f} + glm::vec3{0.2f} * (float)((px + py) % 3 == 0);
            else if (d2 < 12) c *= 1.2f;
        }
        if (px < 2 || py < 2) c = glm::vec3{0.06f};
        return Px{c};
    }, BRICK_MEAN * 0.95f);
}

// Painted panels, a metre by two: recessed joints, paint chipped along the
// edges and the bottom, louvred vents and label plates on some, scuffs
inline std::vector<unsigned char> painted() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        const int W = M, H = 2 * M;
        int col = x / W, row = y / H, lx = x % W, ly = y % H, pl = col + 4 * row;
        float b = 0.42f + (hashI(pl, 0, 721) - 0.5f) * 0.06f + (fbm(u, v, 8, 3, 722) - 0.5f) * 0.08f;
        b += (noise(u, v, 3, 128, 723) - 0.5f) * 0.05f;                           // scuffs, along it
        b *= 1.f - 0.22f * sstep(0.55f, 0.85f, noise(u, v, 48, 4, 724));          // streaks
        glm::vec3 c = b * glm::vec3{0.97f, 1.0f, 0.98f};
        if (hashI(pl, 1, 725) < 0.28f && lx > 10 && lx < W - 10 && ly > 16 && ly < 48) {   // a vent
            int s = (ly - 16) % 5;
            c = s == 0 ? c * 1.3f : s < 3 ? glm::vec3{0.05f} : c * 0.75f;
            if (lx < 12 || lx > W - 12 || ly < 18 || ly > 46) c = glm::vec3{0.3f};
        } else if (ly < 16 && hashI(pl, 1, 725) < 0.28f && lx > 10 && lx < W - 10)
            c *= 1.f - 0.35f * (1.f - ly / 16.f) * noise(u, v, 64, 4, 726);       // grime dribbled down from it
        if (hashI(pl, 2, 727) < 0.3f && lx > 18 && lx < 46 && ly > 92 && ly < 106) {   // a label plate
            c = glm::vec3{0.62f, 0.60f, 0.54f};
            if ((ly == 96 || ly == 101) && lx > 21 && lx < 21 + 4 + (int)(hashI(pl, ly, 728) * 20.f) && lx % 3) c = glm::vec3{0.12f};
            if (lx == 19 || ly == 93) c *= 1.2f;
        }
        float chip = fbm(u, v, 12, 4, 729);
        float edge = std::min(std::min(lx, W - 1 - lx), std::min(ly, H - 1 - ly)) / 10.f;
        float need = 0.6f + 0.25f * std::min(edge, 1.f) - 0.10f * (1.f - std::min(ly / 40.f, 1.f));
        if (chip > need) c = glm::mix(c, glm::vec3{0.36f, 0.27f, 0.20f} * (0.75f + 0.5f * hashI(x, y, 730)), 0.8f);   // to the rust under it
        else if (chip > need - 0.015f) c *= 1.25f;                                // the paint's lifted edge
        if (lx < 2 || ly < 2) c = glm::vec3{0.07f};
        else if (lx < 3 || ly > H - 3) c *= 1.2f;
        else if (lx > W - 3 || ly < 3) c *= 0.7f;
        return Px{c};
    }, 0.362f);
}

// Board-formed concrete: the grain of the shuttering boards, staggered joints,
// tie holes, a darker pour line, damp run down from it. The factory's walls
// and the Dead Span's roadside both
inline std::vector<unsigned char> castwall() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        const int BH = 8;
        int row = y / BH, ly = y % BH;
        int off = (int)(hashI(row, 0, 741) * 128.f);
        int lx = (x + off) % 128, board = ((x + off) / 128) % 2;
        float b = 0.42f + (hashI(row, board, 742) - 0.5f) * 0.06f + (fbm(u, v, 8, 4, 743) - 0.5f) * 0.12f;
        b += (noise(u, v, 6, 96, 744) - 0.5f) * 0.06f;                            // the wood's grain, printed in it
        if (ly == 0) b *= 0.78f; else if (ly == 1) b *= 1.06f;                    // board joints
        if (lx == 0) b *= 0.8f;
        int tx = x % 64, ty = y % 64;                                             // tie holes, every metre
        int d2 = (tx - 32) * (tx - 32) + (ty - 32) * (ty - 32);
        if (d2 < 6) b *= 0.35f; else if (d2 < 14) b *= 0.8f;
        else if (std::abs(tx - 32) < 2 && ty < 32 && ty > 18) b *= 0.9f;          // a stain below it
        if (y % 128 < 3) b *= 0.82f;                                              // the pour line
        if (hashI(x, y, 745) < 0.03f) b *= 0.75f;                                 // air holes
        b *= 1.f - 0.25f * sstep(0.58f, 0.85f, noise(u, v, 40, 3, 746));          // damp, down the face
        glm::vec3 c = b * glm::vec3{1.0f, 0.99f, 0.95f};
        float salt = sstep(0.62f, 0.85f, noise(u, v, 48, 6, 747)) * std::max(0.f, 1.f - (y % 128) / 24.f);   // efflorescence weeping from the pour line
        c = glm::mix(c, glm::vec3{0.70f, 0.69f, 0.66f}, salt * 0.4f);
        return Px{c};
    }, 0.405f);
}

// Factory floor slab: power-trowelled concrete, saw-cut joints every 2 m with
// their arrises chipped, oil stains with dried rims, hairline cracks
inline std::vector<unsigned char> slab() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        float b = 0.34f + (fbm(u, v, 6, 5, 751) - 0.5f) * 0.12f;
        b += (noise(u, v, 32, 32, 752) - 0.5f) * 0.035f;                          // trowel swirl
        b += (hashI(x, y, 753) - 0.5f) * 0.03f;
        float oil = fbm(u, v, 5, 4, 754);
        b *= 1.f - 0.3f * sstep(0.63f, 0.75f, oil);
        Cell cr = voronoi(u, v, 3, 755);                                          // a few shrinkage cracks
        float wob = (noise(u, v, 32, 32, 758) - 0.5f) * 0.03f;
        if (cr.d2 - cr.d1 + wob < 0.012f && noise(u, v, 4, 4, 756) > 0.62f) b *= 0.55f;
        int jx = x % 128, jy = y % 128;
        int dj = std::min(std::min(jx, 128 - jx), std::min(jy, 128 - jy));
        float spall = noise(u, v, 64, 64, 757);
        if (dj == 0) b *= 0.35f;                                                  // the saw cut
        else if (dj < 3 && spall > 0.72f) b *= 0.6f;                              // its chipped edge
        glm::vec3 c = b * glm::vec3{1.0f, 0.97f, 0.92f};
        return Px{c};
    }, 0.330f);
}

// THE DEAD SPAN (Chapter 2) ---------------------------------------------------

// Canyon strata: layers of different tones and hardness, the hard ones
// standing proud with ash settled on their ledges, the soft ones eaten back,
// cracks running up through them
inline std::vector<unsigned char> strata() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        float w = v + (fbm(u, v, 3, 4, 801) - 0.5f) * 0.07f;
        const int N = 11;
        float lf = w * N;
        int layer = wrap((int)std::floor(lf), N);
        float f = lf - std::floor(lf);
        float hard = hashI(layer, 0, 802), hue = hashI(layer, 1, 803);
        glm::vec3 tint = hue < 0.35f ? glm::vec3{1.16f, 0.90f, 0.72f} : hue < 0.7f ? glm::vec3{1.05f, 0.96f, 0.88f} : glm::vec3{1.22f, 0.82f, 0.64f};
        float b = 0.40f + (hard - 0.5f) * 0.12f + (fbm(u, v, 8, 5, 804) - 0.5f) * 0.22f;
        float sub = std::sin((w * 37.f) * 6.2832f);                              // fine bedding inside a layer
        b += sub * 0.025f;
        if (hard > 0.55f) {                                                       // proud: a lit ledge on top, shadow under
            if (f > 0.9f) { b *= 1.15f; tint = glm::mix(tint, glm::vec3{1.f}, 0.3f); }
            else if (f < 0.12f) b *= 0.6f + 0.4f * f / 0.12f;
        } else {
            b *= 0.85f;                                                           // soft: weathered back, pitted
            if (hashI(x / 2, y / 2, 805) < 0.025f) b *= 0.7f;
        }
        int jc = x / 32;                                                          // joints: cracks up through a layer
        if (hashI(jc, layer, 806) < 0.5f) {
            float jx = jc * 32 + 4 + hashI(jc, layer, 809) * 24.f + (noise(u, v, 8, 64, 810) - 0.5f) * 6.f;
            float dj = std::fabs(x - jx);
            if (dj < 0.9f) b *= 0.45f; else if (dj < 2.f) b *= 0.85f;
        }
        b += (hashI(x, y, 807) - 0.5f) * 0.04f;
        return Px{b * tint};
    }, 0.387f);
}

// Ash hardpan: the open ground. Dried mud cracked into plates, ash drifted
// over it in wind-rippled sheets, grit and stones scattered
inline std::vector<unsigned char> ashground() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        Cell cl = voronoi(u, v, 5, 811);
        float edge = cl.d2 - cl.d1;
        float b = 0.40f + (hashI(cl.id, 0, 813) - 0.5f) * 0.06f + (fbm(u, v, 8, 4, 814) - 0.5f) * 0.14f;
        b *= 1.06f - 0.12f * cl.d1;                                               // each plate a little domed
        glm::vec3 c = b * glm::vec3{1.10f, 0.94f, 0.80f};
        float crackW = 0.012f + 0.02f * noise(u, v, 16, 16, 815);
        float deep = 0.45f + 0.4f * noise(u, v, 8, 8, 822);                       // some cracks barely there
        if (edge < crackW) c *= deep;
        else if (edge < crackW * 2.5f) c *= 0.88f;
        Cell fine = voronoi(u, v, 14, 823);                                       // and finer crazing in the plates
        if (fine.d2 - fine.d1 < 0.02f && noise(u, v, 6, 6, 824) > 0.5f) c *= 0.8f;
        float ash = sstep(0.42f, 0.68f, fbm(u, v, 3, 4, 816));
        float ripple = 0.5f + 0.5f * std::sin((u * 9.f + v * 3.f + fbm(u, v, 4, 3, 817) * 1.5f) * 6.2832f);
        glm::vec3 ashC = glm::vec3{0.62f, 0.58f, 0.54f} * (0.92f + 0.12f * ripple) * (0.95f + 0.1f * hashI(x, y, 818));
        c = glm::mix(c, ashC, ash * 0.85f);
        int gx = x / 3, gy = y / 3;                                               // stones
        if (hashI(gx, gy, 819) > 0.985f && ash < 0.6f) c = glm::vec3{0.22f + 0.3f * hashI(gx, gy, 820)} * glm::vec3{1.05f, 0.98f, 0.9f};
        else if (hashI(gx, gy + 1, 819) > 0.985f && ash < 0.6f) c *= 0.6f;        // the stone's shadow
        c *= 0.95f + 0.1f * hashI(x, y, 821);
        return Px{c};
    }, 0.387f);
}

// The broken road: dark asphalt, its stones showing where it's worn, alligator
// cracking in places, tar-sealed cracks, a patch, ash blown across it
inline std::vector<unsigned char> asphalt() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        float b = 0.26f + (fbm(u, v, 6, 4, 831) - 0.5f) * 0.08f;
        float worn = sstep(0.5f, 0.7f, fbm(u, v, 4, 4, 832));
        float h = hashI(x, y, 833), hs = hashI(x / 2, y / 2, 834);
        if (hs > 0.8f - 0.15f * worn) b += 0.12f * (0.5f + hs);                   // aggregate
        else if (h < 0.1f) b *= 0.7f;
        b += worn * 0.04f;
        float tar = sstep(0.62f, 0.7f, fbm(u, v, 2, 3, 840));                     // newer, smoother patches
        b = glm::mix(b, 0.2f + (h - 0.5f) * 0.03f, tar * 0.8f);
        Cell cl = voronoi(u, v, 9, 835);
        if (tar < 0.5f && cl.d2 - cl.d1 < 0.045f && noise(u, v, 3, 3, 836) > 0.58f) b *= 0.45f;   // alligator cracks
        float line = std::fabs(v - 0.37f - 0.06f * std::sin(u * 6.2832f * 2.f) - (fbm(u, v, 4, 3, 837) - 0.5f) * 0.12f);
        if (line < 0.006f && noise(u, v, 3, 1, 838) > 0.55f) b = 0.09f;           // a sealed crack
        glm::vec3 c = b * glm::vec3{0.98f, 0.97f, 0.96f};
        float ash = sstep(0.6f, 0.75f, fbm(u, v, 3, 4, 839));
        c = glm::mix(c, glm::vec3{0.52f, 0.48f, 0.44f} * (0.9f + 0.2f * h), ash * 0.6f);
        return Px{c};
    }, 0.300f);
}

// Corrugated sheet: ribs every 25 cm, 1 m sheets lapped every 2 m with a row
// of fasteners, galvanising gone to rust from the laps and the fasteners down
inline std::vector<unsigned char> corrugated() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        const int SW = M, SH = 2 * M;
        int col = x / SW, row = y / SH, lx = x % SW, ly = y % SH;
        float ph = (x % 16) / 16.f * 6.2832f;
        float b = 0.36f * (0.72f + 0.38f * (0.5f + 0.5f * std::cos(ph)) + 0.12f * std::sin(ph));
        b *= 0.9f + (hashI(col, row, 851) - 0.5f) * 0.16f + (fbm(u, v, 8, 3, 852) - 0.5f) * 0.25f;
        bool painted = hashI(col, row, 853) < 0.35f;
        glm::vec3 c = b * (painted ? glm::vec3{1.12f, 0.92f, 0.82f} : glm::vec3{0.92f, 0.96f, 1.0f});
        float rustBase = fbm(u, v, 6, 4, 854) + 0.35f * (1.f - std::min(ly / 50.f, 1.f));   // most at a sheet's foot
        float rust = sstep(0.6f, 0.8f, rustBase);
        int fy = SH - 6;                                                          // the fastener row at the lap
        bool fastener = (x % 16 == 8 || x % 16 == 9) && (ly == fy || ly == fy + 1);
        if ((x % 16 == 8 || x % 16 == 9) && ly < fy && ly > fy - 40 && hashI(x / 16, row, 855) < 0.5f)
            rust = std::max(rust, (1.f - (fy - ly) / 40.f) * 0.8f * noise(u, v, 64, 16, 856));   // streak below a fastener
        c = glm::mix(c, (b / 0.36f) * glm::vec3{0.48f, 0.25f, 0.12f} * (0.8f + 0.4f * hashI(x, y, 857)), rust * 0.85f);
        if (rustBase > 1.0f && hashI(x / 2, y / 2, 858) < 0.4f) c = glm::vec3{0.03f};   // rusted through
        if (fastener) c = glm::vec3{0.55f};
        if (ly < 2) c = glm::vec3{0.06f};                                         // the lap's shadow
        else if (ly < 4) c *= 0.7f;
        if (lx < 1) c *= 0.6f;
        return Px{c};
    }, 0.260f);
}

// Rusted plate: the tops of the Dead Span's steel, flaking and pitted under ash
inline std::vector<unsigned char> rustplate() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        float n = fbm(u, v, 6, 5, 861);
        float b = 0.30f + (n - 0.5f) * 0.16f;
        glm::vec3 steel = b * glm::vec3{0.9f, 0.93f, 0.96f}, rust = b * glm::vec3{1.55f, 0.85f, 0.5f};
        float r = sstep(0.38f, 0.6f, fbm(u, v, 4, 4, 862));
        glm::vec3 c = glm::mix(steel, rust, r);
        Cell fl = voronoi(u, v, 22, 863);                                         // flakes lifting
        if (r > 0.5f && fl.d2 - fl.d1 < 0.06f) c *= 1.25f;
        if (hashI(x, y, 864) < 0.05f) c *= 0.6f;                                  // pits
        c = glm::mix(c, glm::vec3{0.5f, 0.47f, 0.44f}, 0.55f * sstep(0.6f, 0.78f, fbm(u, v, 3, 3, 865)));   // ash
        int px = x % 128, py = y % 128;
        if (px < 2 || py < 2) c = glm::vec3{0.06f};
        for (int bx : {8, 120}) for (int by : {8, 120})
            if ((px - bx) * (px - bx) + (py - by) * (py - by) < 7) c = glm::vec3{0.4f, 0.3f, 0.22f};
        return Px{c};
    }, 0.260f);
}

// THE COLD STORE (Chapter 5) --------------------------------------------------

// Insulated cold-store panels, a metre wide, fine ribs pressed in, hoar frost
// grown over them in feathery patches, ice hanging from the joints
inline std::vector<unsigned char> froststeel() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        const int W = M, H = 2 * M;
        int col = x / W, row = y / H, lx = x % W, ly = y % H;
        float b = 0.30f + (hashI(col, row, 901) - 0.5f) * 0.05f + (fbm(u, v, 8, 3, 902) - 0.5f) * 0.08f;
        int rib = lx % 8;
        if (rib == 0) b *= 1.2f; else if (rib == 1) b *= 0.8f;                   // the ribs
        glm::vec3 c = b * glm::vec3{0.88f, 0.95f, 1.05f};
        float frost = fbm(u, v, 10, 4, 903) + 0.14f * (noise(u, v, 96, 96, 904) - 0.5f)   // feathery edge
                    + 0.22f * (1.f - std::min(ly / 40.f, 1.f));                   // thickest on the ledge of the joint
        float fr = sstep(0.56f, 0.72f, frost) * 0.72f;
        glm::vec3 rime = glm::vec3{0.80f, 0.88f, 0.96f} * (0.9f + 0.15f * hashI(x, y, 905));
        if (hashI(x, y, 906) > 0.993f && fr > 0.5f) rime = glm::vec3{1.f};       // a glint
        c = glm::mix(c, rime, fr * 0.8f);
        int below = H - ly;                                                       // icicles under the joint above
        int ic = (int)(hashI(x / 3, row, 907) * 30.f);
        if (below < ic && (x % 3 != 0) && hashI(x / 3, row, 908) < 0.4f)
            c = glm::mix(c, glm::vec3{0.75f, 0.86f, 0.95f}, 0.7f * (1.f - below / (float)std::max(ic, 1)));
        if (lx < 2 || ly < 2) c = glm::vec3{0.08f, 0.1f, 0.12f};
        else if (ly < 5) c = glm::mix(c, glm::vec3{0.85f, 0.9f, 0.95f}, 0.6f);      // frost packed along the joint
        return Px{c};
    }, 0.260f);
}

// Frosted deck: steel grating with snow packed white into the gaps, frost
// filming the bars, kicked clear in places
inline std::vector<unsigned char> frostdeck() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        float b = 0.30f + (fbm(u, v, 8, 4, 911) - 0.5f) * 0.1f;
        glm::vec3 c = b * glm::vec3{0.88f, 0.94f, 1.02f};
        int gx = x % 8, gy = y % 32;
        bool gap = gx >= 5 && gy > 2;
        float packed = fbm(u, v, 5, 4, 912);
        if (gap) c = packed > 0.3f ? glm::vec3{0.78f, 0.85f, 0.93f} * (0.9f + 0.15f * hashI(x, y, 913)) : glm::vec3{0.04f, 0.05f, 0.07f};
        else if (gx == 4 || gy <= 2) c *= 0.75f;                                  // cross bars, a step down
        else if (gx == 0) c *= 1.25f;
        float film = sstep(0.55f, 0.75f, fbm(u, v, 4, 3, 914));
        c = glm::mix(c, glm::vec3{0.82f, 0.88f, 0.95f}, film * 0.5f);
        if (x % 128 < 2 || y % 128 < 2) c = glm::vec3{0.06f};
        return Px{c};
    }, 0.300f);
}

// --- The shared surfaces (every mode: the tubes, FAST, ACT II) ---------------
// The original six, redone to wrap and with more to look at, each scaled to
// the old one's brightness so the places lit for them stay as they were.

// Dark concrete floor in half-metre squares: worn, scuffed, stained
inline std::vector<unsigned char> gridFloor() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        const int C = M / 2;
        int cx = x / C, cy = y / C, lx = x % C, ly = y % C;
        float b = 0.30f + (hashI(cx, cy, 101) - 0.5f) * 0.05f + (fbm(u, v, 8, 4, 102) - 0.5f) * 0.12f;
        b += (noise(u, v, 128, 6, 103) - 0.5f) * 0.03f;                 // scuffs, run along it
        b *= 1.f - 0.30f * sstep(0.6f, 0.75f, fbm(u, v, 3, 4, 104));     // stains
        b += (hashI(x, y, 105) - 0.5f) * 0.03f;                          // grit
        if (lx < 2 || ly < 2) b *= 0.62f;                                // the grooves
        else if (lx < 3 || ly < 3) b *= 1.10f;                           // their lit edge
        return Px{b * glm::vec3{1.0f, 0.95f, 0.88f}};
    }, 0.320f);
}

// Brick: half-metre bricks, each its own tone, recessed mortar, chipped
// corners, a few scorched, soot and damp coming through
inline std::vector<unsigned char> brick() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        const int BW = 32, BH = 16;
        int row = y / BH, ly = y % BH;
        int sx = x + (row & 1) * (BW / 2), lx = sx % BW, col = (sx / BW) % (SIZE / BW);
        float tone = 0.85f + 0.3f * hashI(col, row, 111);
        bool burnt = hashI(col, row, 112) < 0.08f;
        float b = 0.42f * tone + (noise(u, v, 64, 64, 113) - 0.5f) * 0.08f + (fbm(u, v, 8, 3, 114) - 0.5f) * 0.08f;
        glm::vec3 c = b * (burnt ? glm::vec3{0.75f, 0.62f, 0.58f} : glm::vec3{1.10f, 0.92f, 0.80f});
        // Mortar, set back: a shadow under each course
        int cornerX = std::min(lx, BW - 1 - lx), cornerY = std::min(ly, BH - 1 - ly);
        bool chip = hashI(col, row, 115) < 0.35f && cornerX + cornerY < 3;
        if (lx < 2 || ly < 2 || chip) c = glm::vec3{0.30f, 0.29f, 0.27f} * (0.9f + 0.2f * hashI(x, y, 116));
        else if (ly == BH - 1) c *= 0.75f;
        float soot = sstep(0.55f, 0.75f, fbm(u, v, 3, 4, 117));
        return Px{c * (1.f - 0.35f * soot)};
    }, 0.374f);
}

// Brushed steel ceiling plates a metre square, bolted at the corners
inline std::vector<unsigned char> metal() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        int px = x % M, py = y % M, pl = (x / M) + 7 * (y / M);
        float b = 0.22f + (noise(u, v, 4, 96, 121) - 0.5f) * 0.10f + (noise(u, v, 2, 256, 122) - 0.5f) * 0.05f;   // brushed
        b += (hashI(pl, 0, 123) - 0.5f) * 0.04f;
        b *= 1.f - 0.3f * sstep(0.62f, 0.78f, fbm(u, v, 4, 3, 124));    // oily smears
        if (px < 2 || py < 2) b *= 0.45f;
        else if (px < 3 || py < 3) b *= 1.25f;
        for (int bx : {6, M - 6}) for (int by : {6, M - 6}) {
            int d2 = (px - bx) * (px - bx) + (py - by) * (py - by);
            if (d2 < 5) b = py > by ? 0.42f : 0.30f;
            else if (d2 < 9 && py < by) b *= 0.6f;
        }
        return Px{b * glm::vec3{0.90f, 0.93f, 1.0f}};
    }, 0.227f);
}

// Sci-fi plates two metres square: bevelled seams, a recessed strip, rivets,
// scratches; now and then a plate with hazard chevrons or a stencilled code
inline std::vector<unsigned char> panel() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        const int P = 2 * M;
        int px = x % P, py = y % P, pl = (x / P) + 3 * (y / P);
        float b = 0.36f + (fbm(u, v, 8, 3, 131) - 0.5f) * 0.08f + (hashI(pl, 1, 132) - 0.5f) * 0.05f;
        // Scratches: short bright hairlines at a slant
        float sc = std::fabs(std::fmod((float)(x + 2 * y) + 400.f, 37.f) - 18.f);
        if (sc < 0.5f && noise(u, v, 32, 32, 133) > 0.72f) b *= 1.18f;
        glm::vec3 c = b * glm::vec3{0.94f, 0.97f, 1.0f};
        // The strip across the middle; on a few plates, hazard chevrons in it
        bool strip = py > P * 0.62f && py < P * 0.70f && px > 10 && px < P - 10;
        if (strip) {
            if (hashI(pl, 2, 134) < 0.25f) c = (((px + py) / 6) % 2) ? glm::vec3{0.62f, 0.50f, 0.16f} : glm::vec3{0.10f};
            else c *= 0.62f;
        }
        // A stencilled block of code on others
        if (hashI(pl, 3, 135) < 0.3f && px > 14 && px < 44 && py > 14 && py < 22 && ((px / 3) % 2 == 0) && hashI(px / 3, pl, 136) < 0.7f)
            c *= 0.55f;
        if (px < 2 || py < 2) c = glm::vec3{0.10f};
        else if (px < 4 || py > P - 4) c *= 1.22f;
        else if (px > P - 4 || py < 4) c *= 0.70f;
        for (int cx : {8, P - 8}) for (int cy : {8, P - 8}) {
            int d2 = (px - cx) * (px - cx) + (py - cy) * (py - cy);
            if (d2 < 6) c = glm::vec3{0.55f} * (py > cy ? 1.f : 0.75f);
        }
        return Px{c};
    }, 0.362f);
}

// Rock: wavy strata, cracks, grain, a few lichen spots
inline std::vector<unsigned char> rock() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        float n = fbm(u, v, 4, 6, 141);
        float band = std::sin((v * 14.f + fbm(u, v, 2, 4, 142) * 3.f) * 6.2831853f / 2.f);
        float b = 0.38f + (n - 0.5f) * 0.32f + band * 0.04f;
        float crack = std::fabs(fbm(u, v, 3, 5, 143) - 0.5f);
        if (crack < 0.007f) b *= 0.55f + 0.45f * crack / 0.007f;
        b += (hashI(x, y, 144) - 0.5f) * 0.04f;
        glm::vec3 c = b * glm::vec3{1.02f, 0.98f, 0.92f};
        float lichen = sstep(0.68f, 0.76f, fbm(u, v, 8, 3, 145));
        c = glm::mix(c, glm::vec3{0.42f, 0.44f, 0.30f} * (0.8f + 0.4f * hashI(x, y, 146)), lichen * 0.6f);
        return Px{c};
    }, 0.387f);
}

// Poured concrete: 2 x 1 m formwork, tie holes weeping rust, run-off, aggregate, hairline cracks
inline std::vector<unsigned char> concrete() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        const int W = 2 * M, H = M;
        int fx = x % W, fy = y % H;
        float b = 0.42f + (fbm(u, v, 8, 4, 151) - 0.5f) * 0.12f + (hashI(x / W, y / H, 152) - 0.5f) * 0.05f;
        b -= (noise(u, v, 24, 2, 153) - 0.5f) * 0.08f;                  // run-off, down the face
        if (hashI(x, y, 154) < 0.04f) b *= 0.8f;                          // aggregate
        float crack = std::fabs(fbm(u, v, 2, 5, 155) - 0.5f);
        if (crack < 0.008f && noise(u, v, 4, 4, 156) > 0.55f) b *= 0.6f;
        glm::vec3 c = b * glm::vec3{1.0f, 1.0f, 0.97f};
        // Tie holes, a streak of rust below each
        for (int hx : {W / 4, 3 * W / 4}) {
            int dx = fx - hx, dy = fy - H / 2;
            if (dx * dx + dy * dy < 7) c *= 0.4f;
            else if (std::abs(dx) <= 1 + (H / 2 - fy) / 12 && dy < 0 && dy > -H / 2 + 4)
                c = glm::mix(c, glm::vec3{0.45f, 0.30f, 0.20f} * (b / 0.42f), 0.35f * (1.f + dy / (float)(H / 2)));
        }
        if (fx < 1 || fy < 1) c *= 0.7f;
        return Px{c};
    }, 0.405f);
}

// --- ACT II and FAST's own ---------------------------------------------------

// Limestone ashlar, pitted and chamfered, with tide lines and dark streaks
// where water has run: the drowned cathedral, the sandstone terraces
inline std::vector<unsigned char> limestone() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        const int W = 2 * M, H = M;                                        // big ashlar: 2 x 1 m
        int row = y / H, ly = y % H;
        int sx = x + (row & 1) * M, lx = sx % W, col = (sx / W) % (SIZE / W);
        float b = 0.55f + (hashI(col, row, 161) - 0.5f) * 0.10f + (fbm(u, v, 8, 4, 162) - 0.5f) * 0.14f;
        b *= 0.9f + 0.1f * noise(u, v, 128, 128, 163);                    // pitting
        float tide = std::fabs(std::sin((v * 6.f + (fbm(u, v, 4, 3, 164) - 0.5f) * 0.4f) * 3.14159f));
        if (tide < 0.03f) b *= 0.82f;
        float streak = noise(u, v, 40, 3, 165);
        b *= 1.f - 0.35f * sstep(0.62f, 0.85f, streak) * sstep(0.45f, 0.7f, fbm(u, v, 2, 3, 166));
        glm::vec3 c = b * glm::vec3{1.04f, 1.0f, 0.92f};
        if (lx < 1 || ly < 1) c *= 0.45f;                                  // a fine joint
        else if (lx < 3 || ly < 3) c *= 0.85f;                             // the chamfer
        else if (lx > W - 3 || ly > H - 3) c *= 1.06f;
        return Px{c};
    }, 0.45f);
}

// Engraved brass: plates ringed like dials, ticks round them, rivets, green
// patina in the hollows. The Orrery's instruments, the Descent's cage
inline std::vector<unsigned char> brass() {
    return build([](int x, int y) {
        float u = (float)x / SIZE, v = (float)y / SIZE;
        const int P = 2 * M;
        int px = x % P, py = y % P;
        float dx = px - P * 0.5f + 0.5f, dy = py - P * 0.5f + 0.5f, r = std::sqrt(dx * dx + dy * dy);
        float b = 0.48f + (noise(u, v, 6, 128, 171) - 0.5f) * 0.10f + (fbm(u, v, 8, 3, 172) - 0.5f) * 0.08f;
        bool groove = (std::fabs(r - 22.f) < 0.8f) || (std::fabs(r - 40.f) < 0.8f) || (std::fabs(r - 54.f) < 0.6f);
        float ang = std::atan2(dy, dx), tick = std::fabs(std::fmod(ang * 24.f / 6.2831853f + 100.f, 1.f) - 0.5f);
        if (r > 42.f && r < 52.f && tick < 0.06f) groove = true;
        if (groove) b *= 0.55f;
        glm::vec3 c = b * glm::vec3{1.18f, 0.92f, 0.55f};
        float patina = sstep(0.6f, 0.74f, fbm(u, v, 4, 4, 173)) + (groove ? 0.4f : 0.f);
        c = glm::mix(c, glm::vec3{0.28f, 0.42f, 0.34f} * (0.8f + 0.4f * hashI(x, y, 174)), std::min(patina, 1.f) * 0.4f);
        if (px < 2 || py < 2) c = glm::vec3{0.12f, 0.09f, 0.05f};
        for (int cx : {6, P - 6}) for (int cy = 6; cy < P; cy += 20) {
            int d2 = (px - cx) * (px - cx) + (py - cy) * (py - cy);
            if (d2 < 5) c = glm::vec3{0.70f, 0.56f, 0.32f};
        }
        return Px{c};
    }, 0.40f);
}

} // namespace MaterialTex
