#pragma once
// =============================================================================
// LevelLayout.h — a level's layout made in Blender (assets/levels/<level>.json),
// put in on top of the level the code builds: levels made in C++ keep working,
// and anything placed in Blender (tools/blender/layout_export.py writes it from
// the scene's OD_LAYOUT collection) joins them. No file: the level as it was.
//
//   {"objects": [
//     {"kind": "wall",  "min": [x,y,z], "max": [x,y,z], "mat": "brick", "color": [r,g,b]},   collidable
//     {"kind": "prop",  ... the same},                                                        drawn only
//     {"kind": "neon",  "min", "max", "color"},                                               self-lit
//     {"kind": "model", "name": "brazier", "xf": [16 numbers, column major]},                 a prop file, placed
//     {"kind": "spawn_ground" | "spawn_air", "pos": [x,y,z], "arena": 2},                     where enemies come in
//     {"kind": "destructible", "name": "pillar", "min", "max", "mat", "color", "hp": 300}      breaks (Destructible)
//   ]}
//
// Game space: +Y up, metres. "arena" is optional: a spawn goes to the arena whose
// bounds hold it. New walls go after the code's, so no index the code keeps
// (doors, movers, anchors) moves. No OpenGL.
// =============================================================================
#include "render/GunFile.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

inline bool g_skipLayout = false;   // tools/level_dump: the level as the code builds it

namespace layout {

inline bool vec3Of(const gunkit::JVal& o, const char* key, glm::vec3& out) {
    const auto& a = o.arr(key);
    if (a.size() != 3) return false;
    out = {(float)a[0].n, (float)a[1].n, (float)a[2].n};
    return true;
}
inline bool matOf(const std::string& s, Mat& out) {
    static const char* N[] = {"brick", "panel", "rock", "metal", "concrete", "adobe", "plate", "marble",
                              "circuit", "basalt", "wood", "limestone", "brass"};
    for (int i = 0; i < 13; ++i) if (s == N[i]) { out = (Mat)i; return true; }
    return false;
}
// The arena whose bounds (X and Z) hold p; -1 if none does
inline int arenaAt(const LevelData& L, glm::vec3 p) {
    for (int i = 0; i < (int)L.arenas.size(); ++i) {
        const AABB& b = L.arenas[i].bounds;
        if (p.x >= b.min.x && p.x <= b.max.x && p.z >= b.min.z && p.z <= b.max.z) return i;
    }
    return -1;
}

// The layout in `text` put into L. False (with why in err) if it can't be read: nothing is changed then
inline bool apply(LevelData& L, const std::string& text, std::string& err) {
    gunkit::JVal doc;
    if (!gunkit::parseJson(text, doc, err)) return false;
    LevelData out = L;   // (all or nothing)
    int n = 0;
    for (const auto& o : doc.arr("objects")) {
        ++n;
        const std::string kind = o.str("kind"), at = "object " + std::to_string(n) + " (" + kind + ")";
        if (kind == "wall" || kind == "prop" || kind == "neon" || kind == "destructible") {
            Wall w;
            if (!vec3Of(o, "min", w.box.min) || !vec3Of(o, "max", w.box.max)) { err = at + ": no min/max"; return false; }
            for (int c = 0; c < 3; ++c) if (w.box.min[c] > w.box.max[c]) std::swap(w.box.min[c], w.box.max[c]);
            vec3Of(o, "color", w.color);
            if (std::string m = o.str("mat"); !m.empty() && !matOf(m, w.mat)) { err = at + ": no material '" + m + "'"; return false; }
            if (kind == "wall") out.walls.push_back(w);
            else if (kind == "prop") out.props.push_back(w);
            else if (kind == "neon") out.neon.push_back(w);
            else {
                Destructible d;
                d.name = o.str("name");
                if (d.name.empty()) { err = at + ": no name (its prop: assets/props/<name>.glb)"; return false; }
                d.hp = d.maxHp = (float)o.num("hp", 300.0);
                d.home = w.box;
                w.hidden = true;   // (drawn whole by the game until it breaks, not baked into the world)
                d.wall = (int)out.walls.size();
                out.walls.push_back(w);
                out.destructibles.push_back(d);
            }
        } else if (kind == "model") {
            const auto& xf = o.arr("xf");
            if (xf.size() != 16) { err = at + ": no xf (16 numbers)"; return false; }
            PlacedModel pm;
            pm.name = o.str("name");
            if (pm.name.empty()) { err = at + ": no name"; return false; }
            pm.free = true;
            float* f = &pm.xf[0][0];
            for (int i = 0; i < 16; ++i) f[i] = (float)xf[i].n;
            out.models.push_back(pm);
        } else if (kind == "spawn_ground" || kind == "spawn_air") {
            glm::vec3 p;
            if (!vec3Of(o, "pos", p)) { err = at + ": no pos"; return false; }
            int a = o.get("arena") ? (int)o.num("arena", -1) : arenaAt(out, p);
            if (a < 0 || a >= (int)out.arenas.size()) { err = at + ": not inside any arena"; return false; }
            (kind == "spawn_ground" ? out.arenas[a].groundSpawns : out.arenas[a].airSpawns).push_back(p);
        } else { err = at + ": unknown kind"; return false; }
    }
    L = std::move(out);
    return true;
}

// assets/levels/<name>.json, if there is one (a broken file is reported and left out)
inline void applyFile(LevelData& L, const std::string& name) {
    if (g_skipLayout) return;
    const std::string path = "assets/levels/" + name + ".json";
    std::ifstream f(path, std::ios::binary);
    if (!f) return;
    std::stringstream ss; ss << f.rdbuf();
    std::string err;
    if (!apply(L, ss.str(), err)) std::fprintf(stderr, "%s: %s (left out)\n", path.c_str(), err.c_str());
}

} // namespace layout
