#pragma once
// =============================================================================
// FrameTimer.h — where each frame's milliseconds go.
//
// Off unless --bench or --frametimes turns it on; when off, every Scope and
// Lap costs one branch. Zones nest (a parent's time includes its children's);
// "self" is a zone's time minus its children's, which is what the worst-frame
// breakdown ranks by. GPU time comes from GL timer queries (native only) and
// lands a few frames late, so it's written back into history afterwards.
//
//   frametimer::Scope s(frametimer::UPDATE);   // times until the end of the block
//   frametimer::Lap lap;  ...  lap.to(frametimer::SIM_PLAYER);   // time since the last lap
// =============================================================================
#include <SDL2/SDL.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace frametimer {

enum Zone {
    LOAD, EVENTS,
    UPDATE, SIM, SIM_PLAYER, SIM_DIRECTOR, SIM_ENEMIES, SIM_BOSSES, SIM_SHOTS, SIM_REST, EFFECTS,
    RENDER, R_WORLD, R_BOXES, R_FX, R_VIEWMODEL, R_POST, R_HUD,
    PRESENT, GPU_SYNC, SWAP, LIMIT,
    GPU,      // measured on the GPU, not part of the CPU sum
    COUNT
};

struct ZoneInfo { const char* name; int parent; };
inline const ZoneInfo& info(int z) {
    static const ZoneInfo t[COUNT] = {
        {"load", -1}, {"events", -1},
        {"update", -1}, {"sim", UPDATE}, {"player", SIM}, {"director", SIM}, {"enemies", SIM},
        {"bosses", SIM}, {"shots", SIM}, {"pickups+cleanup", SIM}, {"effects", UPDATE},
        {"render", -1}, {"sky+world", RENDER}, {"boxes", RENDER}, {"fx", RENDER},
        {"viewmodel", RENDER}, {"post", RENDER}, {"hud", RENDER},
        {"present", -1}, {"gpu query", -1}, {"swap", -1}, {"limiter", -1},
        {"gpu", -1},
    };
    return t[z];
}
inline int depth(int z) { int d = 0; while (info(z).parent >= 0) { z = info(z).parent; ++d; } return d; }

struct Frame { float ms[COUNT] = {}; float total = 0.f; int ticks = 0; bool gpuKnown = false; };

inline bool& enabled() { static bool on = false; return on; }
inline Frame& cur() { static Frame f; return f; }
inline std::vector<Frame>& history() { static std::vector<Frame> h; return h; }

inline double ms(Uint64 from, Uint64 to) { return (double)(to - from) * 1000.0 / (double)SDL_GetPerformanceFrequency(); }

struct Scope {
    Zone z; Uint64 t0;
    explicit Scope(Zone z) : z(z), t0(enabled() ? SDL_GetPerformanceCounter() : 0) {}
    ~Scope() { if (t0) cur().ms[z] += (float)ms(t0, SDL_GetPerformanceCounter()); }
};

struct Lap {
    Uint64 t = enabled() ? SDL_GetPerformanceCounter() : 0;
    void to(Zone z) {
        if (!t) return;
        Uint64 n = SDL_GetPerformanceCounter();
        cur().ms[z] += (float)ms(t, n);
        t = n;
    }
};

inline void tick() { if (enabled()) ++cur().ticks; }

// The frame's start-to-start time (what the screen sees); returns its index in history
inline int endFrame(double totalMs) {
    if (!enabled()) return -1;
    cur().total = (float)totalMs;
    history().push_back(cur());
    cur() = Frame{};
    return (int)history().size() - 1;
}

inline void setGpu(int frame, double gpuMs) {
    if (frame < 0 || frame >= (int)history().size()) return;
    history()[frame].ms[GPU] = (float)gpuMs;
    history()[frame].gpuKnown = true;
}

inline float self(const Frame& f, int z) {
    float s = f.ms[z];
    for (int c = 0; c < COUNT; ++c) if (info(c).parent == z) s -= f.ms[c];
    return std::max(0.f, s);
}

// --bench's breakdown: every zone's average / 1% high / worst, then the worst
// frames and the zones that made them slow. Skips the first `skip` frames.
inline void report(size_t skip) {
    const auto& h = history();
    if (h.size() <= skip + 10) return;
    std::vector<const Frame*> f;
    for (size_t i = skip; i < h.size(); ++i) f.push_back(&h[i]);
    const size_t n = f.size();
    int gpuN = 0; for (auto* x : f) gpuN += x->gpuKnown;
    double ticks = 0; for (auto* x : f) ticks += x->ticks;

    std::fprintf(stderr, "\nframe breakdown (%zu frames, %.2f sim ticks/frame)\n", n, ticks / n);
    std::fprintf(stderr, "  %-22s %8s %8s %8s %6s\n", "zone", "avg ms", "1% ms", "worst", "share");
    double avgTotal = 0; for (auto* x : f) avgTotal += x->total; avgTotal /= n;
    std::vector<float> v(n);
    for (int z = 0; z < COUNT; ++z) {
        bool gpu = z == GPU;
        size_t m = 0; double sum = 0;
        for (auto* x : f) if (!gpu || x->gpuKnown) { v[m++] = x->ms[z]; sum += x->ms[z]; }
        if (m == 0 || (sum == 0 && !gpu)) continue;
        std::sort(v.begin(), v.begin() + m);
        double avg = sum / m;
        std::string name = std::string(depth(z) * 2, ' ') + info(z).name;
        if (gpu) std::fprintf(stderr, "  %-22s %8.2f %8.2f %8.2f %6s  (%d frames; GPU clock, overlaps the CPU)\n",
                              name.c_str(), avg, v[(size_t)(m * 0.99)], v[m - 1], "", gpuN);
        else std::fprintf(stderr, "  %-22s %8.2f %8.2f %8.2f %5.0f%%\n",
                          name.c_str(), avg, v[(size_t)(m * 0.99)], v[m - 1], 100.0 * avg / avgTotal);
    }
    std::fprintf(stderr, "  %-22s %8.2f\n", "frame (start to start)", avgTotal);

    // The worst frames: which zones (self time) ate them
    std::vector<size_t> idx(n);
    for (size_t i = 0; i < n; ++i) idx[i] = i;
    std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b) { return f[a]->total > f[b]->total; });
    std::fprintf(stderr, "worst frames:\n");
    for (size_t k = 0; k < std::min<size_t>(5, n); ++k) {
        const Frame& x = *f[idx[k]];
        std::vector<std::pair<float, int>> s;
        for (int z = 0; z < GPU; ++z) s.push_back({self(x, z), z});
        std::sort(s.rbegin(), s.rend());
        std::fprintf(stderr, "  #%-5zu %6.2f ms  %d ticks ", skip + idx[k], x.total, x.ticks);
        for (int j = 0; j < 3; ++j) std::fprintf(stderr, " %s %.2f", info(s[j].second).name, s[j].first);
        if (x.gpuKnown) std::fprintf(stderr, "  | gpu %.2f", x.ms[GPU]);
        std::fprintf(stderr, "\n");
    }
}

// --frametimes FILE: one row per frame, one column per zone
inline bool writeCsv(const std::string& path) {
    FILE* out = std::fopen(path.c_str(), "w");
    if (!out) return false;
    std::fprintf(out, "frame,total,ticks");
    for (int z = 0; z < COUNT; ++z) std::fprintf(out, ",%s", info(z).name);
    std::fprintf(out, "\n");
    const auto& h = history();
    for (size_t i = 0; i < h.size(); ++i) {
        std::fprintf(out, "%zu,%.3f,%d", i, h[i].total, h[i].ticks);
        for (int z = 0; z < COUNT; ++z) {
            if (z == GPU && !h[i].gpuKnown) std::fprintf(out, ",");
            else std::fprintf(out, ",%.3f", h[i].ms[z]);
        }
        std::fprintf(out, "\n");
    }
    std::fclose(out);
    return true;
}

} // namespace frametimer
