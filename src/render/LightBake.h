#pragma once
// =============================================================================
// LightBake.h — the level's light baked into a grid: for every cell (1 m), how
// open it is (ambient occlusion: corners, under things, between pillars go
// dark) and whether its arena's sun reaches it (soft shadows: the game has no
// shadow maps). The world shader looks it up where it draws (shader.frag).
//
// Baked offline (tools/light_bake, `make light`) into assets/levels/<level>.light,
// with a hash of what it was baked from: a level changed since, and the game
// leaves the bake out (and the tests say to rebake) rather than draw shadows
// of walls that moved.
//
// One region per arena and per corridor between them, packed into one 3D texture: in
// shelves along Z, regions stacked up each shelf as high as the tallest region.
// Each cell is two bytes: R the ambient (1 = as open as flat ground), G the sun.
// No OpenGL.
// =============================================================================
#include "world/Level.h"
#include "render/PropFile.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace lightbake {

constexpr float CELL = 1.f;            // a baked cell, metres
constexpr float VOX = 0.5f;            // the solid grid it's baked against
constexpr float AO_REACH = 4.f;        // how far an occluder still darkens
constexpr float SUN_REACH = 80.f;      // how far a shadow can be cast from
constexpr int   MAX_REGIONS = 16;
constexpr uint32_t MAGIC = 0x4B414C4F;  // "OLAK"
constexpr uint32_t VERSION = 5;

struct Region {
    glm::vec3 min{0.f};       // world corner of cell (0,0,0)
    glm::ivec3 dims{0};       // cells
    glm::ivec3 off{0};        // where its cells start in the atlas
    glm::vec3 sunDir{0.f, -1.f, 0.f};   // the way its sun's light travels, as baked
    bool contains(glm::vec3 p) const {
        glm::vec3 hi = min + glm::vec3(dims) * CELL;
        return p.x >= min.x && p.y >= min.y && p.z >= min.z && p.x < hi.x && p.y < hi.y && p.z < hi.z;
    }
};

struct Bake {
    uint64_t hash = 0;
    std::vector<Region> regions;
    glm::ivec3 atlas{0};          // every region's cells (packAtlas), a cell apart
    std::vector<uint8_t> cells;   // atlas.x * atlas.y * atlas.z * 2 (R ambient, G sun), X fastest
    bool empty() const { return regions.empty(); }
    size_t at(int x, int y, int z) const { return (((size_t)z * atlas.y + y) * atlas.x + x) * 2; }
    // A world point's two values (nearest cell; the shader filters): {1, 1} outside every region
    glm::vec2 sample(glm::vec3 p) const {
        for (const Region& r : regions)
            if (r.contains(p)) {
                glm::ivec3 c = glm::clamp(glm::ivec3(glm::floor((p - r.min) / CELL)), glm::ivec3(0), r.dims - 1);
                size_t i = at(c.x + r.off.x, c.y + r.off.y, c.z + r.off.z);
                return {cells[i] / 255.f, cells[i + 1] / 255.f};
            }
        return {1.f, 1.f};
    }
};

// ---- what casts light and shadow -------------------------------------------
// Every surface the world draws, as boxes and triangles: walls (not hidden, not moving, not doors),
// props, floor patches, shapes, the props made in Blender. (Destructibles are hidden walls: they
// break, so they don't shadow.) The ground under it all is the level's base floor.
inline void forEachOccluder(const LevelData& L, const std::function<void(const AABB&)>& box,
                            const std::function<void(glm::vec3, glm::vec3, glm::vec3)>& tri, bool includeShapes = true, bool includeModels = true) {
    for (int i = 0; i < (int)L.walls.size(); ++i) {
        const Wall& w = L.walls[i];
        if (w.hidden || w.dynamic || L.isDoorWall(i)) continue;
        box(w.box);
    }
    for (const Wall& w : L.props) box(w.box);
    for (const FloorPatch& f : L.floors)
        box({{std::min(f.x0, f.x1), f.y - 0.2f, std::min(f.z0, f.z1)}, {std::max(f.x0, f.x1), f.y, std::max(f.z0, f.z1)}});
    for (const Shape& s : L.shapes)
        if (includeShapes && !s.neon) s.forEachTri([&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3) { tri(a, b, c); });
    if (includeModels) for (const PlacedModel& pm : L.models) {
        const propfile::Prop* pr = propfile::get(pm.name);
        if (!pr) continue;
        const glm::mat4 world = propfile::placement(*pr, pm.free, pm.xf, pm.fit, pm.yaw);
        propfile::forEachTri(*pr, world, [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3, glm::vec3, glm::vec3, glm::vec3, bool glows, int) {
            if (!glows) tri(a, b, c);
        });
    }
}

// What a bake depends on: the occluders, the arenas' boxes and suns. A level changed since: a new hash
inline uint64_t levelHash(const LevelData& L) {
    uint64_t h = 1469598103934665603ull;
    auto mix = [&](const void* p, size_t n) { const uint8_t* b = (const uint8_t*)p; for (size_t i = 0; i < n; ++i) { h ^= b[i]; h *= 1099511628211ull; } };
    auto q = [&](float v) {
        double scaled=(double)v*64.0, half=std::floor(scaled)+0.5;
        // Transform arithmetic can land just above/below an exact half bin.
        // Treat a 16-micrometre neighbourhood as that tie on every compiler.
        if(std::fabs(scaled-half)<1e-3)scaled=half;
        int32_t i=(int32_t)std::lround(scaled);mix(&i,4);
    };
    auto qv = [&](glm::vec3 v) { q(v.x); q(v.y); q(v.z); };
    forEachOccluder(L, [&](const AABB& b) { qv(b.min); qv(b.max); }, [&](glm::vec3 a, glm::vec3 b, glm::vec3 c) { qv(a); qv(b); qv(c); }, false, false);
    // Hash a procedural shape's recipe, not its culled triangle stream. Nearly
    // degenerate dome/cap triangles can cross the area cutoff differently on
    // GCC/x86 and Clang/ARM, even though the source geometry is identical.
    for (const Shape& s : L.shapes) if (!s.neon) {
        int32_t fields[]={(int32_t)s.kind,s.sides,(int32_t)s.seed,s.inward,s.caps};
        mix(fields,sizeof(fields));
        for(int col=0;col<4;++col)for(int row=0;row<4;++row)q(s.xf[col][row]);
        q(s.topScale*100.f);q(s.jitter*100.f);
    }
    // Blender props have the same near-zero triangle issue. Their file-space
    // mesh data and joint transforms are stable; world-space area culling is not.
    auto qm=[&](const glm::mat4& m,float precision){for(int col=0;col<4;++col)for(int row=0;row<4;++row)q(m[col][row]*precision);};
    for(const PlacedModel& pm:L.models) {
        const auto* prop=propfile::get(pm.name);if(!prop)continue;
        qm(propfile::placement(*prop,pm.free,pm.xf,pm.fit,pm.yaw),1.f);
        for(const auto& part:prop->model.parts) if(part.chan<0) {
            qm(part.rel,100.f);
            const auto* mesh=meshlib::get(part.mesh);
            uint32_t counts[]={mesh?(uint32_t)mesh->pos.size():0u,mesh?(uint32_t)mesh->idx.size():0u};mix(counts,sizeof(counts));
            if(mesh){for(auto p:mesh->pos)qv(p*100.f);mix(mesh->idx.data(),mesh->idx.size()*sizeof(uint32_t));}
        }
    }
    for (const Arena& a : L.arenas) { qv(a.bounds.min); qv(a.bounds.max); qv(a.zone.min); qv(a.zone.max); qv(a.theme.lightDir * 100.f); }
    uint32_t v = VERSION; mix(&v, 4);
    return h;
}

// ---- the regions -----------------------------------------------------------------
// One per arena (its bounds and zones, grown a little) with its own sun, then one per corridor between
// two arenas (the first one's sun: the shader fades a baked shadow out as the light turns from it)
inline std::vector<Region> regionsFor(const LevelData& L) {
    std::vector<Region> out;
    auto add = [&](AABB b, glm::vec3 sun) {
        if (out.size() >= (size_t)MAX_REGIONS) return;
        Region r;
        r.min = glm::floor(b.min / CELL) * CELL;
        r.dims = glm::max(glm::ivec3(glm::ceil((b.max - r.min) / CELL)), glm::ivec3(1));
        r.sunDir = glm::normalize(sun);
        out.push_back(r);
    };
    for (const Arena& a : L.arenas) {
        AABB b = a.bounds;
        auto grow = [&](const AABB& z) {
            b.min.x = std::min(b.min.x, z.min.x); b.min.z = std::min(b.min.z, z.min.z);
            b.max.x = std::max(b.max.x, z.max.x); b.max.z = std::max(b.max.z, z.max.z);
            b.max.y = std::max(b.max.y, z.max.y);
        };
        grow(a.zone);
        for (const AABB& z : a.extraZones) grow(z);
        b.min -= glm::vec3(3.f, 2.f, 3.f); b.max += glm::vec3(3.f, 3.f, 3.f);
        add(b, a.theme.lightDir);
    }
    for (size_t i = 0; i < L.corridors.size() && i + 1 < L.arenas.size(); ++i) {
        const Arena &a = L.arenas[i], &n = L.arenas[i + 1];
        AABB b = L.corridors[i];
        b.min.y = std::min(a.bounds.min.y, n.bounds.min.y) - 2.f;
        b.max.y = std::min(std::max(a.bounds.max.y, n.bounds.max.y), b.min.y + 30.f) + 3.f;
        b.min.x -= 2.f; b.min.z -= 2.f; b.max.x += 2.f; b.max.z += 2.f;
        add(b, a.theme.lightDir);
    }
    return out;
}

// Each region a place in the atlas: shelves along Z, each as deep as its deepest region; in a shelf,
// regions stacked up (a cell apart) to the tallest region's height. Returns the atlas's size
inline glm::ivec3 packAtlas(std::vector<Region>& regions) {
    int H = 0, W = 0;
    for (const Region& r : regions) { H = std::max(H, r.dims.y); W = std::max(W, r.dims.x); }
    std::vector<int> order(regions.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = (int)i;
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return regions[a].dims.y > regions[b].dims.y; });
    int z = 0, y = 0, depth = 0;
    for (int i : order) {
        Region& r = regions[i];
        if (y > 0 && y + r.dims.y > H) { z += depth + 1; y = 0; depth = 0; }   // a new shelf
        r.off = {0, y, z};
        y += r.dims.y + 1;
        depth = std::max(depth, r.dims.z);
    }
    return regions.empty() ? glm::ivec3(0) : glm::ivec3(W, H, z + depth);
}

// ---- the solid grid -----------------------------------------------------------------
struct Solid {
    glm::vec3 min{0.f};
    glm::ivec3 dims{0};
    std::vector<uint8_t> v;
    bool in(glm::ivec3 c) const { return c.x >= 0 && c.y >= 0 && c.z >= 0 && c.x < dims.x && c.y < dims.y && c.z < dims.z; }
    uint8_t& at(glm::ivec3 c) { return v[((size_t)c.z * dims.y + c.y) * dims.x + c.x]; }
    uint8_t get(glm::ivec3 c) const { return in(c) ? v[((size_t)c.z * dims.y + c.y) * dims.x + c.x] : 0; }
    glm::ivec3 cellOf(glm::vec3 p) const { return glm::ivec3(glm::floor((p - min) / VOX)); }
    bool solidAt(glm::vec3 p) const { return get(cellOf(p)) != 0; }
};
inline Solid voxelize(const LevelData& L, const AABB& area) {
    Solid S;
    S.min = glm::floor(area.min / VOX) * VOX;
    S.dims = glm::ivec3(glm::ceil((area.max - S.min) / VOX));
    S.v.assign((size_t)S.dims.x * S.dims.y * S.dims.z, 0);
    auto fill = [&](const AABB& b) {
        glm::ivec3 lo = glm::max(S.cellOf(b.min + glm::vec3(1e-3f)), glm::ivec3(0));
        glm::ivec3 hi = glm::min(S.cellOf(b.max - glm::vec3(1e-3f)), S.dims - 1);
        for (int z = lo.z; z <= hi.z; ++z) for (int y = lo.y; y <= hi.y; ++y) for (int x = lo.x; x <= hi.x; ++x) S.at({x, y, z}) = 1;
    };
    auto tri = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c) {   // its surface, sampled finer than a cell
        float len = std::max({glm::length(b - a), glm::length(c - a), glm::length(c - b)});
        int n = std::max(1, (int)std::ceil(len / (VOX * 0.5f)));
        for (int i = 0; i <= n; ++i)
            for (int j = 0; i + j <= n; ++j) {
                glm::vec3 p = a + (b - a) * ((float)i / n) + (c - a) * ((float)j / n);
                glm::ivec3 k = S.cellOf(p);
                if (S.in(k)) S.at(k) = 1;
            }
    };
    forEachOccluder(L, [&](const AABB& b) {
        if (b.max.x < area.min.x || b.min.x > area.max.x || b.max.y < area.min.y || b.min.y > area.max.y ||
            b.max.z < area.min.z || b.min.z > area.max.z) return;
        fill(b);
    }, tri);
    for (int z = 0; z < S.dims.z; ++z)   // the ground: everything under the base floor
        for (int x = 0; x < S.dims.x; ++x) {
            glm::vec3 p = S.min + glm::vec3(x + 0.5f, 0.f, z + 0.5f) * VOX;
            float g = L.baseFloor(p.x, p.z);
            for (int y = 0; y < S.dims.y && S.min.y + (y + 0.5f) * VOX < g; ++y) S.at({x, y, z}) = 1;
        }
    return S;
}

// How far a ray from p along d gets before something solid (up to `reach`): reach if nothing
inline float march(const Solid& S, glm::vec3 p, glm::vec3 d, float reach) {
    const float step = VOX * 0.5f;
    for (float t = step; t < reach; t += step)
        if (S.solidAt(p + d * t)) return t;
    return reach;
}

// Directions over the sphere (a Fibonacci spiral): even, and the same every bake
inline std::vector<glm::vec3> sphereDirs(int n) {
    std::vector<glm::vec3> d;
    const float g = 2.39996323f;
    for (int i = 0; i < n; ++i) {
        float y = 1.f - 2.f * (i + 0.5f) / n, r = std::sqrt(std::max(0.f, 1.f - y * y));
        d.push_back({r * std::cos(g * i), y, r * std::sin(g * i)});
    }
    return d;
}
// How open a point is: each ray counts by how far it gets, squared (an occluder right next to it shuts
// out all of that ray's light, one half AO_REACH off still three quarters, one AO_REACH off none)
inline float rayOpen(float t) { float u = t / AO_REACH; return u * u; }
inline float openness(const Solid& S, glm::vec3 p, const std::vector<glm::vec3>& dirs) {
    float o = 0.f;
    for (glm::vec3 d : dirs) o += rayOpen(march(S, p, d, AO_REACH));
    return o / dirs.size();
}
// The openness of a point half a cell over flat, empty ground: the bake's 1 (a floor in the open is
// neither darkened nor lit up). Worked out the same way the cells are
inline float flatOpenness(const std::vector<glm::vec3>& dirs) {
    float o = 0.f;
    const float h = CELL * 0.5f;
    for (glm::vec3 d : dirs) o += d.y >= -1e-4f ? 1.f : rayOpen(std::min(AO_REACH, h / -d.y));
    return o / dirs.size();
}

// ---- the bake -------------------------------------------------------------------------
// threads: 0 = as many as the machine has
inline Bake bake(const LevelData& L, int threads = 0, const std::function<void(const char*)>& log = nullptr) {
    Bake B;
    B.hash = levelHash(L);
    B.regions = regionsFor(L);
    B.atlas = packAtlas(B.regions);
    B.cells.assign((size_t)B.atlas.x * B.atlas.y * B.atlas.z * 2, 255);
    const std::vector<glm::vec3> dirs = sphereDirs(48);
    const float flat = flatOpenness(dirs);
    // a cell is judged at points spread over it, the ones not inside anything (so a wall face
    // half-way across a cell doesn't make it dark)
    std::vector<glm::vec3> sub;
    for (int i = 0; i < 8; ++i) sub.push_back(glm::vec3((i & 1) ? 0.75f : 0.25f, (i & 2) ? 0.75f : 0.25f, (i & 4) ? 0.75f : 0.25f) * CELL);
    if (threads <= 0) threads = std::max(1u, std::thread::hardware_concurrency());
    for (size_t ri = 0; ri < B.regions.size(); ++ri) {
        const Region& r = B.regions[ri];
        AABB area{r.min - glm::vec3(AO_REACH + 1.f), r.min + glm::vec3(r.dims) * CELL + glm::vec3(AO_REACH + 1.f)};
        area.max.y = std::max(area.max.y, r.min.y + r.dims.y * CELL + SUN_REACH * 0.6f);   // (shadows fall from above it)
        area.min -= glm::vec3(SUN_REACH * 0.6f, 0.f, SUN_REACH * 0.6f);
        area.max += glm::vec3(SUN_REACH * 0.6f, 0.f, SUN_REACH * 0.6f);
        Solid S = voxelize(L, area);
        const glm::vec3 toSun = -glm::normalize(r.sunDir);
        auto work = [&](int z0, int z1) {
            for (int cz = z0; cz < z1; ++cz)
                for (int cy = 0; cy < r.dims.y; ++cy)
                    for (int cx = 0; cx < r.dims.x; ++cx) {
                        glm::vec3 c0 = r.min + glm::vec3(cx, cy, cz) * CELL;
                        float open = 0.f, sun = 0.f; int n = 0;
                        for (glm::vec3 s : sub) {
                            glm::vec3 p = c0 + s;
                            if (S.solidAt(p)) continue;
                            open += openness(S, p, dirs) / flat;
                            sun += march(S, p, toSun, SUN_REACH) >= SUN_REACH ? 1.f : 0.f;
                            ++n;
                        }
                        size_t i = B.at(cx + r.off.x, cy + r.off.y, cz + r.off.z);
                        B.cells[i]     = n ? (uint8_t)std::lround(255.f * glm::clamp(open / n, 0.f, 1.f)) : 0;
                        B.cells[i + 1] = n ? (uint8_t)std::lround(255.f * sun / n) : 0;
                    }
        };
        std::vector<std::thread> pool;
        int per = (r.dims.z + threads - 1) / threads;
        for (int t = 0; t < threads; ++t) {
            int z0 = t * per, z1 = std::min(r.dims.z, z0 + per);
            if (z0 < z1) pool.emplace_back(work, z0, z1);
        }
        for (auto& th : pool) th.join();
        // A roofed arena (its floor mostly out of the sun) was lit by hand for that: no baked sun in it
        size_t lit = 0, free = 0;
        for (int cz = 0; cz < r.dims.z; ++cz)
            for (int cy = 0; cy < std::min(r.dims.y, 8); ++cy)
                for (int cx = 0; cx < r.dims.x; ++cx) {
                    size_t i = B.at(cx + r.off.x, cy + r.off.y, cz + r.off.z);
                    if (B.cells[i] == 0) continue;
                    ++free; lit += B.cells[i + 1] > 127;
                }
        const bool roofed = free > 0 && lit < free / 4;
        if (roofed)
            for (int cz = 0; cz < r.dims.z; ++cz) for (int cy = 0; cy < r.dims.y; ++cy) for (int cx = 0; cx < r.dims.x; ++cx)
                B.cells[B.at(cx + r.off.x, cy + r.off.y, cz + r.off.z) + 1] = 255;
        if (log) {
            char buf[128];
            std::snprintf(buf, sizeof buf, "  region %zu: %d x %d x %d cells%s", ri + 1, r.dims.x, r.dims.y, r.dims.z, roofed ? " (roofed: no sun)" : "");
            log(buf);
        }
    }
    return B;
}

// ---- the file -------------------------------------------------------------------------
// The header, then the cells run-length coded (open air and solid rock are long runs): a byte
// pair and how many times it repeats
inline std::string serialise(const Bake& B) {
    std::string o;
    auto put = [&](const void* p, size_t n) { o.append((const char*)p, n); };
    auto u32 = [&](uint32_t v) { put(&v, 4); };
    auto f32 = [&](float v) { put(&v, 4); };
    u32(MAGIC); u32(VERSION); put(&B.hash, 8);
    u32((uint32_t)B.regions.size());
    for (const Region& r : B.regions) {
        f32(r.min.x); f32(r.min.y); f32(r.min.z);
        u32(r.dims.x); u32(r.dims.y); u32(r.dims.z); u32(r.off.x); u32(r.off.y); u32(r.off.z);
        f32(r.sunDir.x); f32(r.sunDir.y); f32(r.sunDir.z);
    }
    u32(B.atlas.x); u32(B.atlas.y); u32(B.atlas.z);
    for (size_t i = 0; i < B.cells.size();) {
        size_t j = i + 2;
        while (j < B.cells.size() && j - i < 2 * 65535 && B.cells[j] == B.cells[i] && B.cells[j + 1] == B.cells[i + 1]) j += 2;
        uint16_t run = (uint16_t)((j - i) / 2);
        put(&B.cells[i], 2); put(&run, 2);
        i = j;
    }
    return o;
}
inline bool parse(const std::string& s, Bake& B, std::string& err) {
    size_t p = 0;
    auto get = [&](void* d, size_t n) { if (p + n > s.size()) return false; std::memcpy(d, s.data() + p, n); p += n; return true; };
    uint32_t magic = 0, ver = 0, n = 0;
    if (!get(&magic, 4) || magic != MAGIC) { err = "not a light bake"; return false; }
    if (!get(&ver, 4) || ver != VERSION) { err = "an older bake (version " + std::to_string(ver) + ")"; return false; }
    Bake b;
    if (!get(&b.hash, 8) || !get(&n, 4) || n > MAX_REGIONS) { err = "a broken header"; return false; }
    for (uint32_t i = 0; i < n; ++i) {
        Region r; uint32_t d[6];
        if (!get(&r.min, 12) || !get(d, 24) || !get(&r.sunDir, 12)) { err = "a broken region"; return false; }
        r.dims = {(int)d[0], (int)d[1], (int)d[2]}; r.off = {(int)d[3], (int)d[4], (int)d[5]};
        b.regions.push_back(r);
    }
    uint32_t a[3];
    if (!get(a, 12) || (size_t)a[0] * a[1] * a[2] > (size_t)64 << 20) { err = "a broken atlas"; return false; }
    b.atlas = {(int)a[0], (int)a[1], (int)a[2]};
    const size_t total = (size_t)a[0] * a[1] * a[2] * 2;
    b.cells.reserve(total);
    while (b.cells.size() < total) {
        uint8_t v[2]; uint16_t run;
        if (!get(v, 2) || !get(&run, 2) || run == 0 || b.cells.size() + run * 2u > total) { err = "broken cells"; return false; }
        for (uint16_t k = 0; k < run; ++k) { b.cells.push_back(v[0]); b.cells.push_back(v[1]); }
    }
    for (const Region& r : b.regions)
        if (glm::any(glm::lessThan(r.off, glm::ivec3(0))) || glm::any(glm::greaterThan(r.off + r.dims, b.atlas))) { err = "a region outside the atlas"; return false; }
    B = std::move(b);
    return true;
}
inline std::string pathFor(const std::string& level) { return "assets/levels/" + level + ".light"; }
// A level's bake, if there is one and it's of the level as it is now (else empty, and why in err)
inline Bake load(const std::string& level, const LevelData& L, std::string& err) {
    std::ifstream f(pathFor(level), std::ios::binary);
    if (!f) { err = "no bake"; return {}; }
    std::stringstream ss; ss << f.rdbuf();
    Bake B;
    if (!parse(ss.str(), B, err)) return {};
    if (B.hash != levelHash(L)) { err = "baked from an older version of the level (make light)"; return {}; }
    return B;
}

} // namespace lightbake
