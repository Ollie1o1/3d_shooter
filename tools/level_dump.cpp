// level_dump: write a level out for Blender (tools/blender/level_import.py builds a scene from it).
// Build and run from the repo root (make level-dump does both, for every level):
//
//   $(CXX) $(CXXFLAGS) tools/level_dump.cpp -o tools/level_dump && tools/level_dump act1 build/levels/act1
//
// Levels: act1 (ARENA), act2 (ACT II), fast (the Gauntlet), story1 (STORY: THE CRADLE). Into the folder go:
//   level.json        the world as the game draws it (WorldGeometry.h: every face, its UVs and its
//                     shading, batched by texture; the neon apart), the collision boxes, and every
//                     arena (bounds, start, spawns, its light). Game space: +Y up, metres.
//   tex_<name>.bmp    each world texture (MaterialTex.h), the 4 m tile the UVs repeat
// The layout file (assets/levels/<level>.json, LevelLayout.h) is left out: the dump is the level as
// the code builds it, and level_import.py loads the layout on top as the collection you edit.
#include "world/Level.h"
#include "world/LevelAct2.h"
#include "world/LevelGauntlet.h"
#include "world/LevelStory.h"
#include "world/LevelWasteland.h"
#include "world/LevelTraining.h"
#include "render/WorldGeometry.h"
#include "render/MaterialTex.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>

static const char* TEX_NAME[TEX_COUNT] = {"grid", "brick", "metal", "panel", "rock", "concrete", "adobe", "terracotta",
                                         "wood", "plate", "tread", "marble", "flagstone", "circuit", "circuit_floor",
                                         "basalt", "limestone", "brass"};
static const char* MAT_NAME[] = {"brick", "panel", "rock", "metal", "concrete", "adobe", "plate", "marble",
                                 "circuit", "basalt", "wood", "limestone", "brass"};

static std::vector<unsigned char> texPixels(int t) {
    using namespace MaterialTex;
    switch (t) {
        case TEX_GRID: return gridFloor();       case TEX_BRICK: return brick();
        case TEX_METAL: return metal();          case TEX_PANEL: return panel();
        case TEX_ROCK: return rock();            case TEX_CONCRETE: return concrete();
        case TEX_ADOBE: return adobe();          case TEX_TERRACOTTA: return terracotta();
        case TEX_WOOD: return wood();            case TEX_PLATE: return plate();
        case TEX_TREAD: return tread();          case TEX_MARBLE: return marble();
        case TEX_FLAG: return flagstone();       case TEX_CIRCUIT: return circuit();
        case TEX_CIRCUIT_FLOOR: return circuitFloor(); case TEX_BASALT: return basalt();
        case TEX_LIMESTONE: return limestone();  case TEX_BRASS: return brass();
        default: return {};
    }
}

// A 24-bit BMP (Blender reads it; no image library needed). RGBA in, rows top first
static bool writeBmp(const std::string& path, const std::vector<unsigned char>& px, int w, int h) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    const int row = (w * 3 + 3) & ~3, size = 54 + row * h;
    auto u32 = [&](uint32_t v) { for (int i = 0; i < 4; ++i) f.put((char)((v >> (8 * i)) & 255)); };
    auto u16 = [&](uint16_t v) { f.put((char)(v & 255)); f.put((char)(v >> 8)); };
    f.put('B'); f.put('M'); u32(size); u32(0); u32(54);
    u32(40); u32(w); u32(h); u16(1); u16(24); u32(0); u32(row * h); u32(2835); u32(2835); u32(0); u32(0);
    std::vector<char> line(row, 0);
    for (int y = h - 1; y >= 0; --y) {   // (BMP rows run bottom up)
        for (int x = 0; x < w; ++x) {
            const unsigned char* p = &px[(y * w + x) * 4];
            line[x * 3] = (char)p[2]; line[x * 3 + 1] = (char)p[1]; line[x * 3 + 2] = (char)p[0];
        }
        f.write(line.data(), row);
    }
    return (bool)f;
}

struct Json {
    std::ostringstream o;
    static std::string num(float v) { char b[32]; std::snprintf(b, sizeof b, "%.4g", v); return b; }
    static std::string v3(glm::vec3 v) { return "[" + num(v.x) + "," + num(v.y) + "," + num(v.z) + "]"; }
    static std::string box(const AABB& b) { return "{\"min\":" + v3(b.min) + ",\"max\":" + v3(b.max) + "}"; }
    static std::string str(const std::string& s) {
        std::string r = "\"";
        for (char c : s) { if (c == '"' || c == '\\') r += '\\'; r += c; }
        return r + "\"";
    }
    static std::string pts(const std::vector<glm::vec3>& p) {
        std::string r = "[";
        for (size_t i = 0; i < p.size(); ++i) r += (i ? "," : "") + v3(p[i]);
        return r + "]";
    }
};

static std::string batch(const char* tex, bool emit, const std::vector<Vertex>& V, const std::vector<unsigned int>& I) {
    std::string pos, nrm, uv, col, idx;
    pos.reserve(V.size() * 24); uv.reserve(V.size() * 16); col.reserve(V.size() * 20);
    for (size_t i = 0; i < V.size(); ++i) {
        const Vertex& v = V[i];
        const char* sep = i ? "," : "";
        pos += sep + Json::num(v.position.x) + "," + Json::num(v.position.y) + "," + Json::num(v.position.z);
        nrm += sep + Json::num(v.normal.x) + "," + Json::num(v.normal.y) + "," + Json::num(v.normal.z);
        uv  += sep + Json::num(v.uv.x) + "," + Json::num(v.uv.y);
        col += sep + Json::num(v.color.x) + "," + Json::num(v.color.y) + "," + Json::num(v.color.z);
    }
    for (size_t i = 0; i < I.size(); ++i) idx += (i ? "," : "") + std::to_string(I[i]);
    return std::string("{\"tex\":") + Json::str(tex) + ",\"emit\":" + (emit ? "true" : "false") +
           ",\"pos\":[" + pos + "],\"nrm\":[" + nrm + "],\"uv\":[" + uv + "],\"col\":[" + col + "],\"idx\":[" + idx + "]}";
}

int main(int argc, char** argv) {
    if (argc < 3) { std::fprintf(stderr, "usage: level_dump act1|act2|fast|story1|story2|training OUTDIR\n"); return 2; }
    const std::string which = argv[1], out = argv[2];
    g_skipLayout = true;   // the level as the code builds it (LevelLayout.h)
    LevelData L = which == "act1" ? buildLevel() : which == "act2" ? buildAct2Level() : which == "fast" ? buildGauntlet() : which == "story1" ? buildCradle() : which == "story2" ? buildWasteland() : which == "training" ? buildTraining() : LevelData{};
    if (L.arenas.empty()) { std::fprintf(stderr, "no level '%s' (act1, act2, fast, story1, story2, training)\n", which.c_str()); return 2; }
    mkdir(out.c_str(), 0755);

    // The world as the game draws it, and the moving walls where they're built (a lift, the Reliquary's
    // drifting relics): the game draws those on their own, but a picture of the level wants them
    LevelData drawn = L;
    for (int w : drawn.moverWalls) { drawn.walls[w].dynamic = false; drawn.walls[w].hidden = false; }
    WorldGeometry G = buildWorldGeometry(drawn);
    std::string j = "{\"level\":" + Json::str(which) + ",\n\"batches\":[\n";
    bool first = true;
    for (int t = 0; t < TEX_COUNT; ++t) {
        if (G.V[t].empty()) continue;
        j += (first ? "" : ",\n") + batch(TEX_NAME[t], false, G.V[t], G.I[t]);
        first = false;
        auto px = texPixels(t);
        if (!writeBmp(out + "/tex_" + TEX_NAME[t] + ".bmp", px, MaterialTex::SIZE, MaterialTex::SIZE)) {
            std::fprintf(stderr, "can't write %s/tex_%s.bmp\n", out.c_str(), TEX_NAME[t]); return 1;
        }
    }
    if (!G.nV.empty()) j += (first ? "" : ",\n") + batch("neon", true, G.nV, G.nI);
    j += "\n],\n\"walls\":[";
    for (size_t i = 0; i < L.walls.size(); ++i) {
        const Wall& w = L.walls[i];
        j += std::string(i ? ",\n" : "\n") + "{\"box\":" + Json::box(w.box) + ",\"mat\":" + Json::str(MAT_NAME[(int)w.mat]) +
             ",\"color\":" + Json::v3(w.color) + ",\"hidden\":" + (w.hidden ? "true" : "false") +
             ",\"dynamic\":" + (w.dynamic ? "true" : "false") + ",\"door\":" + (L.isDoorWall((int)i) ? "true" : "false") + "}";
    }
    j += "],\n\"arenas\":[";
    for (size_t i = 0; i < L.arenas.size(); ++i) {
        const Arena& a = L.arenas[i];
        const Theme& th = a.theme;
        j += std::string(i ? ",\n" : "\n") + "{\"name\":" + Json::str(a.name ? a.name : "") +
             ",\"subtitle\":" + Json::str(a.subtitle ? a.subtitle : "") +
             ",\"bounds\":" + Json::box(a.bounds) + ",\"zone\":" + Json::box(a.zone) +
             ",\"start\":" + Json::v3(a.playerStart) + ",\"yaw\":" + Json::num(a.startYaw) +
             ",\"ground\":" + Json::pts(a.groundSpawns) + ",\"air\":" + Json::pts(a.airSpawns) +
             ",\"boss\":" + Json::v3(a.bossSpawn) +
             ",\"theme\":{\"lightDir\":" + Json::v3(th.lightDir) + ",\"lightColor\":" + Json::v3(th.lightColor) +
             ",\"skyAmb\":" + Json::v3(th.skyAmb) + ",\"groundAmb\":" + Json::v3(th.groundAmb) +
             ",\"fogColor\":" + Json::v3(th.fogColor) + ",\"fogDensity\":" + Json::num(th.fogDensity) +
             ",\"zenith\":" + Json::v3(th.zenith) + ",\"horizon\":" + Json::v3(th.horizon) +
             ",\"sunDir\":" + Json::v3(th.sunDir) + ",\"sunColor\":" + Json::v3(th.sunColor) + "}}";
    }
    j += "]}\n";
    std::ofstream f(out + "/level.json");
    if (!(f << j)) { std::fprintf(stderr, "can't write %s/level.json\n", out.c_str()); return 1; }
    size_t verts = G.nV.size();
    for (int t = 0; t < TEX_COUNT; ++t) verts += G.V[t].size();
    std::printf("%s: %zu vertices, %zu walls, %zu arenas -> %s\n", which.c_str(), verts, L.walls.size(), L.arenas.size(), out.c_str());
    return 0;
}
