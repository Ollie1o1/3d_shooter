#pragma once
// =============================================================================
// LevelShepherd.h — THE SHEPHERD (Chapter 6's boss, docs/chapter-6-design.md
// section 6). Its own level, loaded in place when you reach its foot at the end
// of the Shore (Gameplay_Shepherd.h), with its own light bake ("shepherd").
//
// It sits just north of the Shore (z -3300 .. -3520), so everything the ring's
// sky works out from how far north you are (its size, the eclipse, the bent
// rings, full gravity) is already at the Shepherd's foot.
//
// Phase 1, THE COMB: three tines, side by side, kilometres long, ploughing west
// through the ring. You fight on top of them (decks 40 m wide, 200 m long), one
// sheepdog to a tine. The gaps between them are 70 m (out of the grapple's
// reach): when a tine breaks, a spar of it falls across to the next.
//
//   arena 0   tine 1 (west)    THE CROOK
//   arena 1   tine 2           THE DROVER and its herd (moving shelter)
//   arena 2   tine 3 (east)    THE BELLWETHER
//
// The aimed sweep fires from each tine's root horn (its north end), a narrow
// cone down the deck at you: get behind a fin, or out of the cone.
// =============================================================================
#include "world/LevelRing.h"

namespace shep {
constexpr int   TINES = 3;
constexpr float HALF = 20.f;                     // a deck's half width
constexpr float Z_S = -3300.f, Z_N = -3500.f;    // a deck's south tip and its root
constexpr float PLAIN_Y = -10.f;                 // the ring's ice, ploughed aside below the decks
constexpr float VOID_Y = -5.f;                   // off a deck: back to the checkpoint
constexpr float BRIDGE_Z = -3340.f, BRIDGE_HZ = 6.f;
constexpr float FIN_W = 7.f, FIN_D = 1.6f, FIN_H = 5.f;
constexpr float HERD_RUN = 15.f;                 // the Drover's herd slides this far either side of the deck's line
inline float tineX(int k) { return -110.f + 110.f * (float)k; }
inline glm::vec3 horn(int k) { return {tineX(k), 9.f, Z_N - 12.f}; }   // where its sweep fires from
inline glm::vec3 start(int k) { return k == 0 ? glm::vec3{tineX(0), 0.f, Z_S - 6.f} : glm::vec3{tineX(k) - HALF + 4.f, 0.f, BRIDGE_Z}; }
inline AABB bridge(int k) {   // the spar from tine k to tine k + 1 (k = 0, 1)
    return {{tineX(k) + HALF - 0.5f, -1.4f, BRIDGE_Z - BRIDGE_HZ}, {tineX(k + 1) - HALF + 0.5f, 0.f, BRIDGE_Z + BRIDGE_HZ}};
}
inline float rowZ(int r) { return Z_S - 22.f - 30.f * (float)r; }
constexpr int ROWS = 6;
// Its fins: shelter on a tine (each with a mooring post on its lee). Rows of two, then one in the
// middle. The Drover's tine keeps only the rows of two: its herd is the rest
inline std::vector<AABB> fins(int k) {
    std::vector<AABB> out;
    const float cx = tineX(k);
    for (int r = 0; r < ROWS; ++r) {
        const float z = rowZ(r);
        if (r % 2 == 0) {
            for (float sx : {-11.f, 11.f})
                out.push_back({{cx + sx - FIN_W * 0.5f, 0.f, z - FIN_D * 0.5f}, {cx + sx + FIN_W * 0.5f, FIN_H, z + FIN_D * 0.5f}});
        } else if (k != 1) {
            out.push_back({{cx - FIN_W * 0.5f, 0.f, z - FIN_D * 0.5f}, {cx + FIN_W * 0.5f, FIN_H, z + FIN_D * 0.5f}});
        }
    }
    return out;
}
// The Drover's herd: three blocks of ice in the middle rows' places, slid across the deck
inline float herdZ(int i) { return rowZ(1 + 2 * i); }
inline glm::vec3 herdHalf() { return {2.8f, 2.3f, 1.7f}; }
}

inline LevelData buildShepherd() {
    using glm::vec3;
    LevelData l; LevelBuilder b{l}; auto& s = l.story;
    s.built = true; s.chapter = 6; s.part = 1; s.nextChapter = 0; s.title = "THE SHEPHERD"; s.nextName = "TO BE CONTINUED";
    s.startWeapons = 15; s.startGrapple = true; s.bossSection = 0; s.pitSection = -1;
    s.end = LevelBuilder::aabb(9000.f, 0.f, 9000.f, 9001.f, 1.f, 9001.f);   // (the game ends it: Gameplay_Shepherd.h)
    const vec3 iceLit{0.9f, 0.88f, 0.82f}, sand{0.62f, 0.54f, 0.42f}, iceDark{0.42f, 0.4f, 0.37f}, steel{0.06f, 0.06f, 0.065f};
    const vec3 hull{0.34f, 0.33f, 0.31f}, hullLit{0.5f, 0.48f, 0.44f}, lampWhite{2.4f, 2.4f, 2.15f};

    // ---- the ring's ice, ten metres down, ploughed into ridges by the tines ----
    l.basins.push_back({LevelBuilder::aabb(-ring::FAR_GROUND, 0.f, -ring::FAR_GROUND - 3600.f, ring::FAR_GROUND, 0.f, ring::FAR_GROUND), shep::PLAIN_Y});
    for (float x = -700.f; x < 700.f; x += 50.f)
        for (float z = -3900.f; z < -2900.f; z += 50.f) {
            const float n = x * 0.37f + z * 0.11f;
            l.floors.push_back({x, z, x + 50.f, z + 50.f, shep::PLAIN_Y, glm::mix(iceLit, sand, 0.4f + 0.2f * ring::hash(n, 11.f)) * 0.9f, Mat::SNOW});
        }
    b.mat = Mat::ROCK;
    for (int i = 0; i < 70; ++i) {   // chunks lying on the ice between and around the tines (out of reach)
        const float fi = (float)i;
        float x = -260.f + 520.f * ring::hash(fi, 21.f), z = -3560.f + 300.f * ring::hash(fi, 22.f);
        bool onDeck = false;
        for (int k = 0; k < shep::TINES; ++k) if (std::fabs(x - shep::tineX(k)) < shep::HALF + 8.f) onDeck = true;
        if (onDeck) continue;
        const float r = 1.2f + 3.f * ring::hash(fi, 23.f);
        ringChunk(b, LevelBuilder::aabb(x - r, shep::PLAIN_Y, z - r * 0.7f, x + r, shep::PLAIN_Y + r * 1.1f, z + r * 0.7f), glm::mix(iceLit, iceDark, 0.3f + 0.4f * ring::hash(fi, 24.f)), (uint32_t)(i * 31 + 5));
    }

    auto post = [&](vec3 at) {   // a mooring post (its lamp is drawn live: Gameplay_Ring.h)
        b.mat = Mat::METAL;
        b.prop(at.x - 0.09f, at.y, at.z - 0.09f, at.x + 0.09f, at.y + 3.f, at.z + 0.09f, steel);
        b.prop(at.x - 0.25f, at.y, at.z - 0.25f, at.x + 0.25f, at.y + 0.2f, at.z + 0.25f, steel);
    };

    for (int k = 0; k < shep::TINES; ++k) {
        const float cx = shep::tineX(k), x0 = cx - shep::HALF, x1 = cx + shep::HALF;
        // The deck: the top of a tooth ploughing through the ice
        b.mat = Mat::METAL;
        b.wall(x0, shep::PLAIN_Y - 2.f, shep::Z_N, x1, 0.f, shep::Z_S, hull);
        // Its plating: seams across it every 10 m, a lit strip down each edge
        for (float z = shep::Z_S - 10.f; z > shep::Z_N; z -= 10.f) {
            if (std::fmod(-z, 20.f) < 1.f) b.neon(x0 + 3.f, 0.f, z - 0.1f, x1 - 3.f, 0.04f, z + 0.1f, lampWhite * 0.22f);   // its deck lamps, cold, every 20 m
            else b.prop(x0 + 1.f, 0.f, z - 0.08f, x1 - 1.f, 0.03f, z + 0.08f, hullLit);
        }
        b.neon(x0 + 0.6f, 0.02f, shep::Z_N + 4.f, x0 + 0.8f, 0.06f, shep::Z_S - 2.f, lampWhite * 0.18f);
        b.neon(x1 - 0.8f, 0.02f, shep::Z_N + 4.f, x1 - 0.6f, 0.06f, shep::Z_S - 2.f, lampWhite * 0.18f);
        // Rails down both edges (a lip to stop a careless step; jump it if you mean to), open where a spar lands
        for (int sd = -1; sd <= 1; sd += 2) {
            const float xr = sd < 0 ? x0 : x1 - 0.5f;
            const bool gapHere = (sd > 0 && k < shep::TINES - 1) || (sd < 0 && k > 0);
            if (gapHere) {
                b.wall(xr, 0.f, shep::Z_N, xr + 0.5f, 0.7f, shep::BRIDGE_Z - shep::BRIDGE_HZ - 1.f, hullLit);
                b.wall(xr, 0.f, shep::BRIDGE_Z + shep::BRIDGE_HZ + 1.f, xr + 0.5f, 0.7f, shep::Z_S, hullLit);
            } else b.wall(xr, 0.f, shep::Z_N, xr + 0.5f, 0.7f, shep::Z_S, hullLit);
        }
        // Its south point: the tooth's tip, stepping down into the ice
        b.mat = Mat::METAL;
        for (int i = 0; i < 4; ++i) {
            const float w = shep::HALF * (0.8f - 0.18f * i), z = shep::Z_S + 6.f * i;
            b.prop(cx - w, shep::PLAIN_Y - 1.f, z, cx + w, -1.5f - 2.f * i, z + 6.f, hull);
        }
        // The bow wave: the ice it shoves aside, heaped against its west face
        b.mat = Mat::ROCK;
        for (float z = shep::Z_N + 6.f; z < shep::Z_S; z += 9.f) {
            const float q = z * 0.13f + k * 31.f;
            b.kit().rock({x0 - 2.f - 2.f * ring::hash(q, 1.f), shep::PLAIN_Y - 0.5f, z}, 3.f + 2.f * ring::hash(q, 2.f), 4.f + 4.f * ring::hash(q, 3.f), glm::mix(iceLit, sand, 0.3f), (uint32_t)(q * 7.f), 0.6f, 7);
            b.kit().rock({x1 + 1.5f, shep::PLAIN_Y - 0.5f, z + 4.f}, 1.6f + ring::hash(q, 4.f), 1.5f + 2.f * ring::hash(q, 5.f), glm::mix(iceLit, iceDark, 0.4f), (uint32_t)(q * 11.f), 0.6f, 6);
        }
        // Fins: shelter (each with its post in its lee, the south side)
        int n = 0;
        for (const AABB& f : shep::fins(k)) {
            b.mat = Mat::METAL;
            const int w = b.wall(f.min.x, f.min.y, f.min.z, f.max.x, f.max.y, f.max.z, glm::mix(hull, hullLit, 0.4f + 0.3f * ring::hash((float)n + k * 10.f, 5.f)));
            (void)w;
            b.neon(f.min.x + 0.4f, f.max.y - 0.5f, f.max.z + 0.01f, f.max.x - 0.4f, f.max.y - 0.36f, f.max.z + 0.06f, lampWhite * 0.25f);   // a seam of its lamps
            SweepShelter sh; sh.box = f; sh.section = k;
            sh.post = {f.max.x - 0.8f, 0.f, f.max.z + 1.2f};
            post(sh.post);
            l.sweepShelters.push_back(sh);
            ++n;
        }
        // The root: its horn housing across the north end (where the sweep fires from), and the tooth
        // climbing away from it, up into the Shepherd
        b.mat = Mat::METAL;
        b.wall(x0, 0.f, shep::Z_N - 22.f, x1, 16.f, shep::Z_N, hull);
        for (int row = 0; row < 3; ++row) {   // rows of its lamps, which flare with the sweep (skybox's ridge, made near)
            const float y = 4.f + 4.f * row;
            b.neon(x0 + 3.f, y, shep::Z_N + 0.02f, x1 - 3.f, y + 0.35f, shep::Z_N + 0.2f, lampWhite * (0.5f - 0.1f * row));
        }
        b.prop(cx - 4.f, 6.f, shep::Z_N, cx + 4.f, 12.f, shep::Z_N + 1.2f, steel);   // the horn's mouth
        for (int i = 0; i < 16; ++i) {
            const float fi = (float)i, len = 60.f, ang = 0.42f + 0.025f * fi;   // steepening as it rises
            float y = 8.f, z = shep::Z_N - 22.f;
            for (int j = 0; j < i; ++j) { y += std::sin(0.42f + 0.025f * j) * len; z -= std::cos(0.42f + 0.025f * j) * len; }
            const float w = 2.f * shep::HALF + 3.f * fi;
            Shape& seg = b.kit().box({cx, y + std::sin(ang) * len * 0.5f, z - std::cos(ang) * len * 0.5f}, {w, 16.f + 1.5f * fi, len + 2.f}, glm::mix(hull, steel, 0.3f + 0.04f * fi), 0.f, ang);
            (void)seg;
            if (i % 2 == 0) b.kit().box({cx, y + std::sin(ang) * len * 0.5f + 8.5f + 0.75f * fi, z - std::cos(ang) * len * 0.5f}, {w * 0.7f, 0.4f, 1.f}, lampWhite * 0.4f, 0.f, ang);
        }
        // Sheepdog spawn marks don't need geometry; the Drover's herd (tine 2): movers the game slides
        if (k == 1) {
            const vec3 h = shep::herdHalf();
            for (int i = 0; i < 3; ++i) {
                const vec3 c{cx, h.y, shep::herdZ(i)};
                const int m = b.mover(c, h, Mover::Path::DRIVEN, vec3{-shep::HERD_RUN, 0.f, 0.f}, vec3{shep::HERD_RUN, 0.f, 0.f}, 1.f, 0.f, vec3{0.f});
                l.movers[m].look = 2; l.movers[m].color = glm::mix(iceLit, iceDark, 0.2f);
                l.walls[l.movers[m].wall].mat = Mat::ROCK;
                SweepShelter sh; sh.box = l.walls[l.movers[m].wall].box; sh.wall = l.movers[m].wall; sh.mover = m; sh.fixed = false; sh.section = 1;
                sh.post = c + vec3{0.f, -h.y, h.z + 1.3f};
                l.sweepShelters.push_back(sh);
                l.shepHerd.push_back(m);
            }
        }
    }
    // The spars: a broken tine's tooth falls across to the next (DRIVEN: the game drops them)
    for (int k = 0; k + 1 < shep::TINES; ++k) {
        const AABB br = shep::bridge(k);
        const vec3 c = (br.min + br.max) * 0.5f, h = (br.max - br.min) * 0.5f;
        const int m = b.mover(c, h, Mover::Path::DRIVEN, vec3{0.f, 90.f, 0.f}, vec3{0.f}, 1.f, 0.f, vec3{0.f});
        l.movers[m].color = hull; l.movers[m].glow = lampWhite * 0.12f;
        l.walls[l.movers[m].wall].mat = Mat::METAL;
        l.shepBridges.push_back(m);
    }

    // ---- the arenas: one to a tine (its spar in belongs to it) ----
    using W = WaveEntry;
    for (int k = 0; k < shep::TINES; ++k) {
        const float cx = shep::tineX(k);
        static const char* names[3] = {"THE COMB", "THE COMB", "THE COMB"};
        static const char* subs[3] = {"THE FIRST TINE", "THE SECOND TINE", "THE THIRD TINE"};
        Arena a;
        a.name = names[k]; a.subtitle = subs[k];
        a.zone = LevelBuilder::aabb(cx - shep::HALF - 1.f, -1000.f, shep::Z_N - 2.f, cx + shep::HALF + 1.f, 40.f, shep::Z_S + 30.f);
        if (k > 0) { AABB br = shep::bridge(k - 1); br.min.y = -1000.f; br.max.y = 40.f; br.max.x = cx - shep::HALF - 1.f; a.extraZones.push_back(br); }
        a.bounds = LevelBuilder::aabb(cx - shep::HALF + 1.f, shep::PLAIN_Y - 2.f, shep::Z_N + 1.f, cx + shep::HALF - 1.f, 40.f, shep::Z_S);
        a.playerStart = shep::start(k); a.startYaw = -90.f; a.hasRespawn = true; a.respawn = a.playerStart;
        a.voidY = shep::VOID_Y;
        a.theme = ring::eclipseTheme(); a.ambient = Ambient::DUST; a.space = ReverbSpace::OPEN; a.tone = RoomTone::NONE;
        a.hasTrigger = true;
        a.trigger = LevelBuilder::aabb(cx - shep::HALF, -2.f, shep::Z_N, cx + shep::HALF, 20.f, k == 0 ? shep::Z_S - 24.f : shep::Z_S);
        const float zc = (shep::Z_S + shep::Z_N) * 0.5f;
        a.groundSpawns = {{cx - 14.f, 0.f, zc - 40.f}, {cx + 14.f, 0.f, zc - 44.f}, {cx, 0.f, zc - 60.f}, {cx - 15.f, 0.f, zc + 10.f}, {cx + 15.f, 0.f, zc + 6.f}, {cx - 6.f, 0.f, zc - 80.f}, {cx + 6.f, 0.f, zc - 76.f}};
        a.airSpawns = {{cx - 10.f, 3.f, zc - 50.f}, {cx + 10.f, 3.f, zc - 54.f}};
        a.maxAlive = 7; a.waveBreak = 2.f; a.damageScale = 1.f;
        if (k == 0) a.waves = {{W(EnemyType::CROOK, 0, {vec3{cx + 4.f, 0.f, zc - 10.f}}), W(EnemyType::STRAY, 4), W(EnemyType::HERDER, 1)}};
        if (k == 1) a.waves = {{W(EnemyType::DROVER, 0, {vec3{cx - shep::HALF - 10.f, 8.f, zc}}), W(EnemyType::STRAY, 4), W(EnemyType::HERDER, 1)}};
        if (k == 2) a.waves = {{W(EnemyType::BELLWETHER, 0, {vec3{cx, 0.f, zc - 16.f}}), W(EnemyType::STRAY, 5), W(EnemyType::HERDER, 1)}};
        l.arenas.push_back(a);
    }
    return l;
}
