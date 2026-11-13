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
constexpr int   TINES = 3;                       // the Comb's three zones, up one tine (the name stayed)
constexpr float PLAIN_Y = -10.f;                 // the ring's ice, ploughed aside below the tine
constexpr float VOID_Y = -5.f;                   // off the tine: back to the checkpoint
constexpr float FIN_W = 7.f, FIN_D = 1.6f, FIN_H = 5.f;
// PHASE 1, THE COMB: one tine, walked from its tip toward the Shepherd, in three zones.
//   THE PLOUGH     its tip, tearing through a field of capsules that it throws up onto the deck; THE CROOK
//                  hooks from a mast over the middle of it
//   THE STAMPEDE   the tine breaking up behind you (deck plates that fall), THE DROVER rolling its herd
//                  down at you; it makes its stand on the KNUCKLE, where the plates stop
//   THE BELFRY     the root: a tower of hanging bells. THE BELLWETHER rings them; drop one on it
constexpr float HALF = 22.f;                     // the deck's half width
constexpr float TIP = -3300.f, PLOUGH_N = -3445.f;
constexpr float SLAB = 10.f; constexpr int SLABS = 15;
inline float slabZ(int i) { return PLOUGH_N - SLAB * (float)i; }   // a plate's south edge (its north: slabZ(i) - SLAB)
constexpr float KNUCKLE_S = -3595.f, KNUCKLE_N = -3656.f;
constexpr float BEL_S = -3656.f, BEL_N = -3716.f, BEL_HX = 25.f, BALC_Y = 11.f, BALC_D = 6.f, BEL_TOP = 34.f;
inline glm::vec3 perch() { return {0.f, 14.f, -3388.f}; }          // the Crook's mast top
inline glm::vec3 shepherdEye() { return {0.f, 70.f, -3900.f}; }     // where its aimed sweep comes from (its ridge, high and north)
inline std::vector<AABB> fins() {
    std::vector<AABB> out;
    auto fin = [&](float x, float z) { out.push_back({{x - FIN_W * 0.5f, 0.f, z - FIN_D * 0.5f}, {x + FIN_W * 0.5f, FIN_H, z + FIN_D * 0.5f}}); };
    fin(-12.f, -3330.f); fin(12.f, -3330.f); fin(-16.f, -3360.f); fin(16.f, -3360.f);
    fin(-12.f, -3412.f); fin(12.f, -3412.f); fin(0.f, -3436.f);
    return out;
}
constexpr int CAPS = 8;                          // the capsules it throws up onto the deck (a pool of movers)
inline glm::vec3 capHalf() { return {1.2f, 1.1f, 2.4f}; }
struct Bell { glm::vec3 top; float floorY; };
inline std::vector<Bell> bells() {
    return {{{-12.f, 8.5f, -3678.f}, 0.f}, {{12.f, 8.5f, -3678.f}, 0.f}, {{0.f, 8.5f, -3700.f}, 0.f},
            {{-22.f, 19.5f, -3692.f}, BALC_Y}, {{22.f, 19.5f, -3692.f}, BALC_Y}};
}
constexpr float BELL_H = 3.6f, BELL_R = 2.3f;
inline glm::vec3 greatBell() { return {0.f, 30.f, -3690.f}; }

// PHASE 2, THE TURN: a raft of packed ice heaped against its ridge, east of the tines. The ridge's
// face is the north edge; its three vents open after each wave. Each sits further from shelter
constexpr float RX0 = 560.f, RX1 = 700.f, RZ_S = -3296.f, RIDGE_Z = -3458.f;
inline glm::vec3 ridgeStart() { return {630.f, 0.f, RZ_S - 8.f}; }
inline glm::vec3 vent(int i) { const float x[3] = {590.f, 632.f, 674.f}; return {x[i], 2.6f, RIDGE_Z + 0.5f}; }
constexpr float VENT_HX = 3.2f, VENT_HY = 2.2f;
// PHASE 3, THE RESONATOR: inside. Three rooms north from x 800: the Tuning Hall (its blasts run
// down it from the core; the side rooms are shelter; four forks to still), the Ledger (quiet), and
// the Lamp Well (a shaft climbed by grapple, blasts coming up it, alcoves for shelter)
constexpr float HX = 800.f, HHALF = 15.f, HZ_S = -3300.f, HZ_N = -3460.f, H_TOP = 24.f;
constexpr float ALC_DEPTH = 7.f, ALC_LEN = 10.f, ALC_H = 8.f;
inline float alcoveZ(int i) { return -3318.f - 32.f * (float)i; }   // five each side
constexpr int ALCOVES = 5, FORKS = 4;
inline float forkZ(int i) { return -3334.f - 32.f * (float)i; }
inline glm::vec3 forkNode(int i) { return {HX, 4.5f, forkZ(i)}; }
constexpr float LZ_N = -3522.f, L_TOP = 14.f;                         // the Ledger: from the hall's north end to here
constexpr float WX0 = 790.f, WX1 = 810.f, WZ_S = -3524.f, WZ_N = -3544.f, W_TOP = 92.f, W_OUT = 86.f;   // the Well
inline float wellAlcoveY(int i) { return 12.f * (float)(i + 1); }     // seven, alternating west and east
constexpr int WELL_ALCOVES = 7;
inline AABB wellAlcove(int i) {   // the recess (where standing is shelter)
    const bool west = i % 2 == 0; const float y = wellAlcoveY(i);
    return west ? AABB{{WX0 - 4.f, y, -3537.f}, {WX0 + 0.6f, y + 5.f, -3531.f}} : AABB{{WX1 - 0.6f, y, -3537.f}, {WX1 + 4.f, y + 5.f, -3531.f}};
}
inline std::vector<glm::vec2> ridgeShelters() {   // (x, z) of each block's middle
    std::vector<glm::vec2> v = {{280.f, -3335.f}, {320.f, -3335.f}, {360.f, -3335.f}, {388.f, -3335.f}, {300.f, -3368.f}, {345.f, -3368.f}, {380.f, -3368.f},
                                {272.f, -3400.f}, {318.f, -3398.f}, {292.f, -3428.f}, {332.f, -3418.f}, {372.f, -3405.f}};
    for (auto& c : v) c.x += 300.f;   // (moved east, out of sight of the Comb)
    return v;
}
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
        if (std::fabs(x) < shep::HALF + 8.f) continue;   // (off the tine)
        const float r = 1.2f + 3.f * ring::hash(fi, 23.f);
        ringChunk(b, LevelBuilder::aabb(x - r, shep::PLAIN_Y, z - r * 0.7f, x + r, shep::PLAIN_Y + r * 1.1f, z + r * 0.7f), glm::mix(iceLit, iceDark, 0.3f + 0.4f * ring::hash(fi, 24.f)), (uint32_t)(i * 31 + 5));
    }

    auto post = [&](vec3 at) {   // a mooring post (its lamp is drawn live: Gameplay_Ring.h)
        b.mat = Mat::METAL;
        b.prop(at.x - 0.09f, at.y, at.z - 0.09f, at.x + 0.09f, at.y + 3.f, at.z + 0.09f, steel);
        b.prop(at.x - 0.25f, at.y, at.z - 0.25f, at.x + 0.25f, at.y + 0.2f, at.z + 0.25f, steel);
    };

    // =========================================================================
    // PHASE 1: THE COMB. One tine, tip to root
    // =========================================================================
    {
        const float x0 = -shep::HALF, x1 = shep::HALF;
        const vec3 amber{2.4f, 1.3f, 0.35f};
        auto deck = [&](float zS, float zN) {   // solid deck: plating, lit edges, rails
            b.mat = Mat::METAL;
            b.wall(x0, shep::PLAIN_Y - 2.f, zN, x1, 0.f, zS, hull);
            for (float z = zS - 10.f; z > zN; z -= 10.f) {
                if (std::fmod(-z, 20.f) < 1.f) b.neon(x0 + 3.f, 0.f, z - 0.1f, x1 - 3.f, 0.04f, z + 0.1f, lampWhite * 0.22f);
                else b.prop(x0 + 1.f, 0.f, z - 0.08f, x1 - 1.f, 0.03f, z + 0.08f, hullLit);
            }
            for (float xr : {x0, x1 - 0.5f}) b.wall(xr, 0.f, zN, xr + 0.5f, 0.7f, zS, hullLit);
            b.neon(x0 + 0.6f, 0.02f, zN + 1.f, x0 + 0.8f, 0.06f, zS - 1.f, lampWhite * 0.18f);
            b.neon(x1 - 0.8f, 0.02f, zN + 1.f, x1 - 0.6f, 0.06f, zS - 1.f, lampWhite * 0.18f);
        };
        // ---- THE PLOUGH: the tip, and the field of capsules it tears through ----
        deck(shep::TIP, shep::PLOUGH_N);
        for (int i = 0; i < 4; ++i) {   // its point, stepping down into the ice
            const float w = shep::HALF * (0.8f - 0.18f * i), z = shep::TIP + 6.f * i;
            b.prop(-w, shep::PLAIN_Y - 1.f, z, w, -1.5f - 2.f * i, z + 6.f, hull);
        }
        for (int i = 0; i < 150; ++i) {   // the field: capsules lying in the ice both sides, heaped up at its bow
            const float fi = (float)i, side = ring::hash(fi, 41.f) < 0.65f ? -1.f : 1.f;
            const float x = side * (shep::HALF + 4.f + 120.f * ring::hash(fi, 42.f) * ring::hash(fi, 43.f));
            const float z = -3260.f - 520.f * ring::hash(fi, 44.f);
            ringCapsule(b, {x, shep::PLAIN_Y + 0.4f, z}, ring::hash(fi, 45.f) < 0.3f, (int)(ring::hash(fi, 46.f) * 4.f), (ring::hash(fi, 47.f) - 0.5f) * 40.f, false);
        }
        b.mat = Mat::ROCK;
        for (float z = shep::KNUCKLE_N; z < shep::TIP; z += 9.f) {   // the bow wave on its west face, ice and capsules heaped
            const float q = z * 0.13f;
            b.kit().rock({x0 - 2.f - 2.f * ring::hash(q, 1.f), shep::PLAIN_Y - 0.5f, z}, 3.f + 2.f * ring::hash(q, 2.f), 4.f + 4.f * ring::hash(q, 3.f), glm::mix(iceLit, sand, 0.3f), (uint32_t)(q * 7.f), 0.6f, 7);
        }
        int n = 0;
        for (const AABB& f : shep::fins()) {   // fins: shelter from its aim, each with its post in its lee
            b.mat = Mat::METAL;
            b.wall(f.min.x, f.min.y, f.min.z, f.max.x, f.max.y, f.max.z, glm::mix(hull, hullLit, 0.4f + 0.3f * ring::hash((float)n, 5.f)));
            b.neon(f.min.x + 0.4f, f.max.y - 0.5f, f.max.z + 0.01f, f.max.x - 0.4f, f.max.y - 0.36f, f.max.z + 0.06f, lampWhite * 0.25f);
            SweepShelter sh; sh.box = f; sh.section = 0; sh.post = {f.max.x - 0.8f, 0.f, f.max.z + 1.2f};
            post(sh.post); l.sweepShelters.push_back(sh); ++n;
        }
        {   // the Crook's mast: a pillar and its platform, a lamp ring round it
            const vec3 pc = shep::perch();
            b.mat = Mat::METAL;
            b.wall(pc.x - 1.6f, 0.f, pc.z - 1.6f, pc.x + 1.6f, pc.y - 0.6f, pc.z + 1.6f, hullLit);
            b.wall(pc.x - 4.5f, pc.y - 0.6f, pc.z - 4.5f, pc.x + 4.5f, pc.y, pc.z + 4.5f, hull);
            b.neon(pc.x - 4.5f, pc.y - 0.7f, pc.z + 4.45f, pc.x + 4.5f, pc.y - 0.45f, pc.z + 4.55f, amber * 0.6f);
            b.neon(pc.x - 4.5f, pc.y - 0.7f, pc.z - 4.55f, pc.x + 4.5f, pc.y - 0.45f, pc.z - 4.45f, amber * 0.6f);
            SweepShelter sh; sh.box = LevelBuilder::aabb(pc.x - 1.6f, 0.f, pc.z - 1.6f, pc.x + 1.6f, pc.y, pc.z + 1.6f); sh.section = 0; sh.post = {pc.x + 1.f, 0.f, pc.z + 2.8f};
            post(sh.post); l.sweepShelters.push_back(sh);
        }
        for (int i = 0; i < shep::CAPS; ++i) {   // the capsules it throws: parked until thrown (Gameplay_ShepherdComb.h)
            const vec3 c{0.f, shep::capHalf().y, -3380.f};
            const int m = b.mover(c, shep::capHalf(), Mover::Path::DRIVEN, vec3{0.f}, vec3{0.f}, 1.f, 0.f, vec3{0.f});
            l.movers[m].look = 3; l.movers[m].color = vec3{0.62f, 0.64f, 0.64f}; l.walls[l.movers[m].wall].mat = Mat::METAL;
            SweepShelter sh; sh.box = l.walls[l.movers[m].wall].box; sh.wall = l.movers[m].wall; sh.mover = m; sh.fixed = false; sh.section = 0;
            sh.post = c + vec3{0.f, -shep::capHalf().y, shep::capHalf().z + 1.2f};
            l.sweepShelters.push_back(sh);
            l.shepCaps.push_back(m);
        }
        // ---- THE STAMPEDE: plates that fall (movers), then the KNUCKLE (solid) ----
        for (int i = 0; i < shep::SLABS; ++i) {
            const float zS = shep::slabZ(i), zN = zS - shep::SLAB;
            const vec3 c{0.f, -0.75f, (zS + zN) * 0.5f}, h{shep::HALF, 0.75f, shep::SLAB * 0.5f - 0.05f};
            const int m = b.mover(c, h, Mover::Path::DRIVEN, vec3{0.f}, vec3{0.f, -90.f, 0.f}, 1.f, 0.f, vec3{0.f});
            l.movers[m].look = 3; l.movers[m].color = glm::mix(hull, hullLit, 0.25f + 0.2f * ring::hash((float)i, 7.f)); l.walls[l.movers[m].wall].mat = Mat::METAL;
            l.shepSlabs.push_back(m);
        }
        deck(shep::KNUCKLE_S, shep::KNUCKLE_N);
        b.mat = Mat::METAL;
        for (int sd = -1; sd <= 1; sd += 2)   // its knuckle's bosses either side: cover, and the look of a joint
            b.wall(sd * 14.f - 3.f, 0.f, -3630.f, sd * 14.f + 3.f, 4.f, -3622.f, glm::mix(hull, hullLit, 0.5f));
        // The comb's other teeth, far off either side: they grind past (the game's blade, near you)
        for (int sd = -1; sd <= 1; sd += 2) {
            b.wall(sd * 150.f - 18.f, shep::PLAIN_Y, -3900.f, sd * 150.f + 18.f, 3.f, -3240.f, hull);
            b.neon(sd * 150.f - 18.f, 3.f, -3900.f, sd * 150.f - 17.6f, 3.4f, -3240.f, lampWhite * 0.3f);
        }
        // ---- THE BELFRY: the root, a tower of bells ----
        const float bx = shep::BEL_HX, bs = shep::BEL_S, bn = shep::BEL_N;
        b.mat = Mat::METAL;
        b.wall(-bx - 3.f, shep::PLAIN_Y - 2.f, bn - 3.f, bx + 3.f, 0.f, bs, hull);   // floor
        b.wall(-bx - 3.f, 0.f, bn - 3.f, -bx, shep::BEL_TOP, bs, hull);             // west
        b.wall(bx, 0.f, bn - 3.f, bx + 3.f, shep::BEL_TOP, bs, hull);               // east
        b.wall(-bx, 0.f, bn - 3.f, bx, shep::BEL_TOP, bn, hull);                    // north
        b.wall(-bx, 0.f, bs - 3.f, -7.f, shep::BEL_TOP, bs, hull);                  // south, its door in the middle
        b.wall(7.f, 0.f, bs - 3.f, bx, shep::BEL_TOP, bs, hull);
        b.wall(-7.f, 9.f, bs - 3.f, 7.f, shep::BEL_TOP, bs, hull);
        // balconies: west, east and north, at BALC_Y; ramps up to the west and east ones
        b.wall(-bx, shep::BALC_Y - 0.6f, bn, -bx + shep::BALC_D, shep::BALC_Y, bs - 3.f, hullLit);
        b.wall(bx - shep::BALC_D, shep::BALC_Y - 0.6f, bn, bx, shep::BALC_Y, bs - 3.f, hullLit);
        b.wall(-bx, shep::BALC_Y - 0.6f, bn, bx, shep::BALC_Y, bn + shep::BALC_D, hullLit);
        for (int sd = -1; sd <= 1; sd += 2) {
            const float xa = sd < 0 ? -bx + shep::BALC_D : bx - shep::BALC_D - 4.f, xb = xa + 4.f;
            const float zFoot = bs - 6.f, zTop = bs - 6.f - 30.f;
            l.ramps.push_back({LevelBuilder::aabb(xa, 0.f, zTop, xb, shep::BALC_Y, zFoot), 2, shep::BALC_Y, 0.f});
            const float len = std::sqrt(30.f * 30.f + shep::BALC_Y * shep::BALC_Y), ang = ring::datan2(shep::BALC_Y, 30.f);
            Shape& slab = b.kit().box({0.f, 0.f, 0.f}, {4.f, 0.6f, len}, hullLit);
            slab.xf = glm::translate(glm::mat4(1.f), vec3{(xa + xb) * 0.5f, shep::BALC_Y * 0.5f, (zFoot + zTop) * 0.5f}) * ring::drotate(ang, vec3{1.f, 0.f, 0.f})
                    * glm::translate(glm::mat4(1.f), vec3{0.f, -0.3f, 0.f}) * slab.xf;
            for (int k = 1; k < 6; ++k) {   // solid under it
                const float zz = zFoot - 30.f * k / 6.f;
                b.solid(xa, 0.f, zz - 5.f, xb, shep::BALC_Y * k / 6.f - 0.2f, zz);
            }
        }
        for (float y = 4.f; y < shep::BEL_TOP; y += 6.f) {   // its lamps: rows round the tower
            b.neon(-bx + 0.02f, y, bn + 2.f, -bx + 0.25f, y + 0.3f, bs - 5.f, lampWhite * 0.35f);
            b.neon(bx - 0.25f, y, bn + 2.f, bx - 0.02f, y + 0.3f, bs - 5.f, lampWhite * 0.35f);
            b.neon(-bx + 2.f, y, bn + 0.02f, bx - 2.f, y + 0.3f, bn + 0.25f, lampWhite * 0.35f);
        }
        for (const shep::Bell& bl : shep::bells()) {   // the beams the bells hang from (the bells and chains are drawn live)
            b.kit().box({bl.top.x, shep::BEL_TOP - 0.5f, (bl.top.z + bn) * 0.5f}, {1.2f, 1.f, std::fabs(bl.top.z - bn) + 2.f}, steel);
            b.neon(bl.top.x - 1.f, bl.floorY + 0.02f, bl.top.z - 1.f, bl.top.x + 1.f, bl.floorY + 0.06f, bl.top.z + 1.f, amber * 0.25f);   // a lamp under it, on the floor: where it lands
        }
        b.kit().box({0.f, shep::BEL_TOP - 0.5f, (shep::greatBell().z + bn) * 0.5f}, {2.f, 1.4f, std::fabs(shep::greatBell().z - bn) + 2.f}, steel);
        // The root, rising out of the tower's back into the Shepherd
        for (int i = 0; i < 16; ++i) {
            const float fi = (float)i, len = 60.f, ang = 0.42f + 0.025f * fi;
            float y = 20.f, z = bn - 3.f;
            for (int j = 0; j < i; ++j) { y += ring::dsin(0.42f + 0.025f * j) * len; z -= ring::dcos(0.42f + 0.025f * j) * len; }
            const float w = 2.f * shep::HALF + 12.f + 3.f * fi;
            auto tilted = [&](vec3 c, vec3 size, vec3 col) {
                Shape& sh = b.kit().box({0.f, 0.f, 0.f}, size, col);
                sh.xf = glm::translate(glm::mat4(1.f), c) * ring::drotate(ang, vec3{1.f, 0.f, 0.f}) * sh.xf;
            };
            tilted({0.f, y + ring::dsin(ang) * len * 0.5f, z - ring::dcos(ang) * len * 0.5f}, {w, 16.f + 1.5f * fi, len + 2.f}, glm::mix(hull, steel, 0.3f + 0.04f * fi));
            if (i % 2 == 0) tilted({0.f, y + ring::dsin(ang) * len * 0.5f + 8.5f + 0.75f * fi, z - ring::dcos(ang) * len * 0.5f}, {w * 0.7f, 0.4f, 1.f}, lampWhite * 0.4f);
        }
    }

    // ---- PHASE 2: THE TURN. The ice heaped against its ridge, and the ridge ----
    {
        b.mat = Mat::SNOW;
        b.wall(shep::RX0, shep::PLAIN_Y - 2.f, shep::RIDGE_Z, shep::RX1, 0.f, shep::RZ_S, glm::mix(iceLit, sand, 0.35f));
        b.mat = Mat::ROCK;
        for (float x = shep::RX0; x < shep::RX1; x += 8.f)   // its ragged sides and south lip
            for (int sd = 0; sd < 2; ++sd) {
                const float q = x * 0.3f + sd * 77.f;
                b.kit().rock({x + 4.f, shep::PLAIN_Y, sd ? shep::RZ_S + 2.f : shep::RZ_S + 2.f}, 3.f + 2.f * ring::hash(q, 1.f), 8.f + 2.f * ring::hash(q, 2.f), glm::mix(iceLit, iceDark, 0.3f), (uint32_t)(q * 13.f), 0.6f, 7);
            }
        for (float z = shep::RIDGE_Z; z < shep::RZ_S; z += 8.f)
            for (float x : {shep::RX0 - 2.f, shep::RX1 + 2.f}) {
                const float q = z * 0.3f + x;
                b.kit().rock({x, shep::PLAIN_Y, z + 4.f}, 3.f + 2.f * ring::hash(q, 1.f), 8.5f + 2.f * ring::hash(q, 2.f), glm::mix(iceLit, iceDark, 0.3f), (uint32_t)(q * 13.f), 0.6f, 7);
            }
        // Shelter: blocks of the ice the ridge ploughed up, each with its post (the waves come from the north)
        int n = 0;
        for (const glm::vec2& c : shep::ridgeShelters()) {
            const float hx = 3.2f + 0.8f * ring::hash((float)n, 31.f), hz = 1.6f, h = 4.2f + 0.8f * ring::hash((float)n, 32.f);
            b.mat = Mat::ROCK;
            b.wall(c.x - hx, 0.f, c.y - hz, c.x + hx, h, c.y + hz, glm::mix(iceLit, iceDark, 0.25f + 0.3f * ring::hash((float)n, 33.f)));
            SweepShelter sh; sh.box = LevelBuilder::aabb(c.x - hx, 0.f, c.y - hz, c.x + hx, h, c.y + hz); sh.section = 3;
            sh.post = {c.x + hx - 0.7f, 0.f, c.y + hz + 1.2f};
            post(sh.post);
            l.sweepShelters.push_back(sh);
            ++n;
        }
        // The ridge: the Shepherd's face, rolled down to you. Rows of its lamps; its fleece (capsules pressed
        // against it in rows, the ones it kept closest: design 13.8); three vents at its foot (drawn live)
        b.mat = Mat::METAL;
        b.wall(shep::RX0 - 30.f, shep::PLAIN_Y - 2.f, shep::RIDGE_Z - 60.f, shep::RX1 + 30.f, 26.f, shep::RIDGE_Z, hull);
        for (int i = 0; i < 9; ++i) {   // stepping back and up: the ridge's slope
            const float y = 26.f + 22.f * i, z = shep::RIDGE_Z - 6.f - 9.f * i;
            {   // (real walls, not shapes: drawn at any distance, and nothing reaches them)
                const float hw2 = (shep::RX1 - shep::RX0 + 60.f + 8.f * i) * 0.5f, mx2 = (shep::RX0 + shep::RX1) * 0.5f;
                b.mat = Mat::METAL;
                b.wall(mx2 - hw2, y, z - 60.f, mx2 + hw2, y + 22.f, z, glm::mix(hullLit, hull, 0.3f + 0.07f * i));
            }
            const float hw = (shep::RX1 - shep::RX0 + 50.f + 8.f * i) * 0.5f, mx = (shep::RX0 + shep::RX1) * 0.5f;
            for (float lx = mx - hw + 2.f; lx < mx + hw - 4.f; lx += 9.f)   // (in lamps, not one strip: rows of points read as a face of lights)
                { b.neon(lx, y + 12.f, z, lx + 5.f, y + 13.6f, z + 0.5f, lampWhite * (1.6f - 0.08f * i)); if ((int)(lx * 0.11f) % 3 == 0) b.neon(lx + 2.f, y + 5.f, z, lx + 3.f, y + 19.f, z + 0.4f, lampWhite * 0.5f); }   // its lamps, row on row up the ridge
        }
        for (float x = shep::RX0 - 20.f; x < shep::RX1 + 20.f; x += 4.f)
            for (int row = 0; row < 2; ++row) {
                const float y = 9.f + 9.f * row, k2 = x * 0.37f + row * 11.f;
                bool overVent = false;
                for (int v = 0; v < 3; ++v) if (std::fabs(x - shep::vent(v).x) < 6.f && row == 0) overVent = true;
                if (overVent || ring::hash(k2, 4.f) < 0.25f) continue;
                b.prop(x - 1.f, y - 0.3f, shep::RIDGE_Z, x + 1.f, y, shep::RIDGE_Z + 1.4f, steel);   // its ledge
                ringCapsule(b, {x, y, shep::RIDGE_Z + 0.8f}, true, 0, (ring::hash(k2, 5.f) - 0.5f) * 14.f, false);
            }
        b.neon(shep::RX0 - 30.f, 25.2f, shep::RIDGE_Z + 0.02f, shep::RX1 + 30.f, 25.6f, shep::RIDGE_Z + 0.25f, lampWhite * 0.6f);
        for (int v = 0; v < 3; ++v) {   // each vent's throat: a dark recess with a lamp over it
            const glm::vec3 c = shep::vent(v);
            b.prop(c.x - shep::VENT_HX - 0.6f, c.y + shep::VENT_HY, shep::RIDGE_Z, c.x + shep::VENT_HX + 0.6f, c.y + shep::VENT_HY + 0.6f, shep::RIDGE_Z + 0.8f, steel);
            b.neon(c.x - 1.5f, c.y + shep::VENT_HY + 1.4f, shep::RIDGE_Z + 0.02f, c.x + 1.5f, c.y + shep::VENT_HY + 1.7f, shep::RIDGE_Z + 0.2f, lampWhite * 0.8f);
        }
    }

    // ---- PHASE 3: THE RESONATOR (inside) ----
    {
        const vec3 lamp = lampWhite * 0.45f, amber{2.4f, 1.3f, 0.35f};
        const float x0 = shep::HX - shep::HHALF, x1 = shep::HX + shep::HHALF;
        b.mat = Mat::METAL;
        // THE TUNING HALL: floor, ceiling, the south end shut, side walls with their rooms cut in
        b.wall(x0 - 12.f, -2.f, shep::LZ_N, x1 + 12.f, 0.f, shep::HZ_S + 2.f, hull);
        b.wall(x0 - 12.f, shep::H_TOP, shep::HZ_N, x1 + 12.f, shep::H_TOP + 2.f, shep::HZ_S + 2.f, hull);
        b.wall(x0 - 12.f, 0.f, shep::HZ_S, x1 + 12.f, shep::H_TOP, shep::HZ_S + 2.f, hull);
        for (int sd = -1; sd <= 1; sd += 2) {
            const float face = sd < 0 ? x0 : x1, back = face + sd * shep::ALC_DEPTH, outer = back + sd * 3.f;
            const float fa = std::min(face, back), fb = std::max(face, back), oa = std::min(back, outer), ob = std::max(back, outer);
            b.wall(oa, 0.f, shep::HZ_N, ob, shep::H_TOP, shep::HZ_S, hull);   // behind the rooms
            float z = shep::HZ_S;
            for (int i = 0; i <= shep::ALCOVES; ++i) {   // the pieces between the rooms (shelter: a room's north side)
                const float zEnd = i < shep::ALCOVES ? shep::alcoveZ(i) + shep::ALC_LEN * 0.5f : shep::HZ_N;
                if (z - zEnd > 0.1f) {
                    b.wall(fa, 0.f, zEnd, fb, shep::H_TOP, z, glm::mix(hull, hullLit, 0.3f));
                    if (i > 0) {   // the piece north of room i-1 shelters it; its post stands in that room
                        SweepShelter sh; sh.box = LevelBuilder::aabb(fa, 0.f, zEnd, fb, shep::H_TOP, z); sh.section = 4;
                        sh.post = {(fa + fb) * 0.5f, 0.f, z + 2.f};
                        l.sweepShelters.push_back(sh);
                    }
                }
                if (i < shep::ALCOVES) {   // the room: open to the hall, roofed at ALC_H, a lamp in it
                    const float za = shep::alcoveZ(i) - shep::ALC_LEN * 0.5f, zb = shep::alcoveZ(i) + shep::ALC_LEN * 0.5f;
                    b.wall(fa, shep::ALC_H, za, fb, shep::H_TOP, zb, glm::mix(hull, hullLit, 0.3f));
                    b.neon(fa + 0.5f, shep::ALC_H - 0.3f, za + 1.f, fb - 0.5f, shep::ALC_H - 0.05f, zb - 1.f, amber * 0.5f);
                    z = za;
                }
            }
        }
        for (float z = shep::HZ_S - 8.f; z > shep::HZ_N; z -= 16.f) b.neon(x0 + 2.f, shep::H_TOP - 0.3f, z - 0.3f, x1 - 2.f, shep::H_TOP - 0.05f, z + 0.3f, lamp);
        b.neon(x0 + 0.05f, 0.f, shep::HZ_N, x0 + 0.3f, 0.2f, shep::HZ_S, lamp * 0.6f);
        b.neon(x1 - 0.3f, 0.f, shep::HZ_N, x1 - 0.05f, 0.2f, shep::HZ_S, lamp * 0.6f);
        // Its forks: a base (stays) and two prongs (movers: they go when stilled; drawn live)
        for (int i = 0; i < shep::FORKS; ++i) {
            const float z = shep::forkZ(i);
            b.wall(shep::HX - 5.f, 0.f, z - 2.5f, shep::HX + 5.f, 3.f, z + 2.5f, glm::mix(hull, steel, 0.5f));
            for (int sd = -1; sd <= 1; sd += 2) {
                const vec3 c{shep::HX + sd * 2.6f, 3.f + 8.f, z}, h{0.8f, 8.f, 0.8f};
                const int m = b.mover(c, h, Mover::Path::DRIVEN, vec3{0.f}, vec3{0.f}, 1.f, 0.f, vec3{0.f});
                l.movers[m].look = 3; l.movers[m].color = glm::mix(hullLit, steel, 0.2f);
                l.shepForks.push_back(m);
            }
        }
        // The core's membrane across the hall's north end: it parts when the forks are stilled
        {
            const vec3 c{shep::HX, shep::H_TOP * 0.5f, shep::HZ_N + 1.f}, h{shep::HHALF, shep::H_TOP * 0.5f, 1.f};
            l.shepMembrane = b.mover(c, h, Mover::Path::DRIVEN, vec3{0.f}, vec3{0.f}, 1.f, 0.f, vec3{0.f});
            l.movers[l.shepMembrane].look = 3; l.movers[l.shepMembrane].color = glm::vec3{0.3f, 0.12f, 0.06f};
        }
        // THE LEDGER: a long low room, every wall lamps (one a capsule), its terminals
        b.wall(x0, shep::L_TOP, shep::LZ_N, x1, shep::L_TOP + 2.f, shep::HZ_N, hull);
        b.wall(x0 - 3.f, 0.f, shep::LZ_N, x0, shep::L_TOP, shep::HZ_N, hull);
        b.wall(x1, 0.f, shep::LZ_N, x1 + 3.f, shep::L_TOP, shep::HZ_N, hull);
        b.wall(x0, shep::H_TOP * 0.5f + 2.f, shep::HZ_N - 0.5f, x1, shep::H_TOP, shep::HZ_N + 0.2f, hull);   // (over the membrane: the hall is taller)
        for (int sd = -1; sd <= 1; sd += 2)
            for (float z = shep::HZ_N - 3.f; z > shep::LZ_N + 2.f; z -= 1.5f)
                for (float y = 1.5f; y < shep::L_TOP - 1.f; y += 1.2f)
                    if (ring::hash(z * 3.1f + y * 7.f + sd, 9.f) < 0.55f) {
                        const float xf = sd < 0 ? x0 + 0.02f : x1 - 0.12f;
                        b.neon(xf, y, z, xf + 0.1f, y + 0.25f, z + 0.25f, (ring::hash(z + y, 3.f) < 0.03f ? amber : lampWhite * 0.35f));
                    }
        {
            const float zc = (shep::HZ_N + shep::LZ_N) * 0.5f;
            const std::pair<float, float> at[4] = {{-8.f, 14.f}, {8.f, 4.f}, {-8.f, -8.f}, {8.f, -18.f}};
            const char* titles[4] = {"THE COUNT", "UNIT 24", "RETURNED", "UNIT 23"};
            const char* lines[4] = {"40,112 HELD. NONE DRIFTING. NONE ARRIVED.", "DISPATCHED. NOT RECEIVED. NOT RECEIVED. NOT RECEIVED.",
                                    "THE CROOK. THE DROVER. THE BELLWETHER. SENT OUT, BROUGHT BACK, SENT OUT.", "STILL COUNTING."};
            for (int i = 0; i < 4; ++i) {
                const vec3 p{shep::HX + at[i].first, 0.f, zc + at[i].second};
                b.mat = Mat::METAL;
                b.prop(p.x - 0.5f, 0.f, p.z - 0.3f, p.x + 0.5f, 1.2f, p.z + 0.3f, steel);
                b.neon(p.x - 0.4f, 1.2f, p.z - 0.25f, p.x + 0.4f, 1.8f, p.z - 0.2f, lampWhite * 0.6f);
                s.terminals.push_back({p + vec3{0.f, 0.f, 0.f}, titles[i], lines[i]});
            }
        }
        b.mat = Mat::METAL;
        // Its north end, with the way into the well
        b.wall(x0, 0.f, shep::LZ_N - 2.f, shep::HX - 4.f, shep::L_TOP, shep::LZ_N, hull);
        b.wall(shep::HX + 4.f, 0.f, shep::LZ_N - 2.f, x1, shep::L_TOP, shep::LZ_N, hull);
        b.wall(shep::HX - 4.f, 5.f, shep::LZ_N - 2.f, shep::HX + 4.f, shep::L_TOP, shep::LZ_N, hull);
        // THE LAMP WELL: a shaft to its surface. Each side wall is two layers: the back, and a face with
        // the alcoves cut out of it (each with its floor: grapple up to the next)
        b.wall(shep::WX0 - 6.f, -2.f, shep::WZ_N - 2.f, shep::WX1 + 6.f, 0.f, shep::LZ_N - 2.f, hull);
        b.wall(shep::WX0 - 6.f, 0.f, shep::WZ_N - 2.f, shep::WX1 + 6.f, shep::W_TOP, shep::WZ_N, hull);   // north
        b.wall(shep::WX0 - 6.f, 5.f, shep::WZ_S, shep::WX1 + 6.f, shep::W_TOP, shep::LZ_N - 2.f, hull);   // south, over the way in
        b.wall(shep::WX0 - 6.f, 0.f, shep::WZ_S, shep::HX - 4.f, 5.f, shep::LZ_N - 2.f, hull);
        b.wall(shep::HX + 4.f, 0.f, shep::WZ_S, shep::WX1 + 6.f, 5.f, shep::LZ_N - 2.f, hull);
        for (int sd = -1; sd <= 1; sd += 2) {
            const float face = sd < 0 ? shep::WX0 : shep::WX1;
            const float fa = sd < 0 ? face - 4.f : face, fb = sd < 0 ? face : face + 4.f;
            const float ba = sd < 0 ? face - 6.f : face + 4.f, bb = sd < 0 ? face - 4.f : face + 6.f;
            b.wall(ba, 0.f, shep::WZ_N, bb, shep::W_TOP, shep::WZ_S, hull);
            b.wall(fa, 0.f, shep::WZ_N, fb, shep::W_TOP, -3537.f, glm::mix(hull, hullLit, 0.25f));
            b.wall(fa, 0.f, -3531.f, fb, shep::W_TOP, shep::WZ_S, glm::mix(hull, hullLit, 0.25f));
            float y = 0.f;
            for (int i = (sd < 0 ? 0 : 1); i < shep::WELL_ALCOVES; i += 2) {
                const float ay = shep::wellAlcoveY(i);
                b.wall(fa, y, -3537.f, fb, ay, -3531.f, glm::mix(hull, hullLit, 0.25f));
                b.neon(fa + 0.3f, ay + 4.6f, -3536.5f, fb - 0.3f, ay + 4.9f, -3531.5f, amber * 0.6f);   // its lamp
                y = ay + 5.f;
            }
            b.wall(fa, y, -3537.f, fb, shep::W_TOP, -3531.f, glm::mix(hull, hullLit, 0.25f));
        }
        for (float y = 6.f; y < shep::W_TOP; y += 6.f)   // rings of its lamps up the shaft
            for (int sd = 0; sd < 2; ++sd) {
                const float z = sd ? shep::WZ_N + 0.05f : shep::WZ_S - 0.3f;
                b.neon(shep::WX0 + 1.f, y, z, shep::WX1 - 1.f, y + 0.3f, z + 0.25f, lamp * (0.5f + 0.5f * (y / shep::W_TOP)));
            }
        // The top: its surface, a lip to climb out onto
        b.wall(shep::WX0 - 14.f, shep::W_TOP - 2.f, shep::WZ_N - 14.f, shep::WX1 + 14.f, shep::W_TOP, shep::WZ_N - 2.f, hull);
    }

    // ---- the arenas: the Comb's three zones (0-2), then the ridge (3) and inside (4-6) ----
    using W = WaveEntry;
    auto comb = [&](const char* name, const char* sub, AABB zone, vec3 start, AABB trig) {
        Arena a;
        a.name = name; a.subtitle = sub; a.zone = zone; a.bounds = zone; a.bounds.min.y = shep::PLAIN_Y - 2.f; a.bounds.max.y = 40.f;
        a.playerStart = start; a.startYaw = -90.f; a.hasRespawn = true; a.respawn = start;
        a.voidY = shep::VOID_Y;
        a.theme = ring::eclipseTheme(); a.ambient = Ambient::DUST; a.space = ReverbSpace::OPEN; a.tone = RoomTone::NONE;
        a.hasTrigger = true; a.trigger = trig; a.maxAlive = 7; a.waveBreak = 2.f;
        return a;
    };
    {   // 0. THE PLOUGH
        Arena a = comb("THE PLOUGH", "ITS TIP", LevelBuilder::aabb(-shep::HALF - 1.f, -1000.f, -3438.f, shep::HALF + 1.f, 40.f, shep::TIP + 30.f), {0.f, 0.f, shep::TIP - 6.f},
                       LevelBuilder::aabb(-shep::HALF, -2.f, -3438.f, shep::HALF, 20.f, -3326.f));
        a.groundSpawns = {{-14.f, 0.f, -3420.f}, {14.f, 0.f, -3418.f}, {-8.f, 0.f, -3346.f}, {8.f, 0.f, -3350.f}, {0.f, 0.f, -3400.f}};
        a.airSpawns = {{-10.f, 3.f, -3410.f}, {10.f, 3.f, -3410.f}};
        a.waves = {{W(EnemyType::CROOK, 0, {shep::perch()}), W(EnemyType::STRAY, 4)}};
        l.arenas.push_back(a);
    }
    {   // 1. THE STAMPEDE and the KNUCKLE
        Arena a = comb("THE STAMPEDE", "IT IS BREAKING UP", LevelBuilder::aabb(-shep::HALF - 1.f, -1000.f, -3650.f, shep::HALF + 1.f, 40.f, -3438.f), {0.f, 0.f, -3441.f},
                       LevelBuilder::aabb(-shep::HALF, -2.f, -3650.f, shep::HALF, 20.f, shep::PLOUGH_N - 3.f));
        a.groundSpawns = {{-12.f, 0.f, -3612.f}, {12.f, 0.f, -3614.f}, {0.f, 0.f, -3640.f}, {-16.f, 0.f, -3600.f}, {16.f, 0.f, -3602.f}};
        a.airSpawns = {{-8.f, 3.f, -3620.f}, {8.f, 3.f, -3620.f}};
        a.waves = {{W(EnemyType::DROVER, 0, {vec3{0.f, 8.f, -3490.f}})}};
        l.arenas.push_back(a);
    }
    {   // 2. THE BELFRY
        Arena a = comb("THE BELFRY", "WHERE IT RINGS FROM", LevelBuilder::aabb(-shep::BEL_HX, -1000.f, shep::BEL_N, shep::BEL_HX, 40.f, -3650.f), {0.f, 0.f, -3652.f},
                       LevelBuilder::aabb(-shep::BEL_HX, -2.f, shep::BEL_N, shep::BEL_HX, 30.f, shep::BEL_S - 6.f));
        a.voidY = -30.f;
        a.groundSpawns = {{-14.f, 0.f, -3668.f}, {14.f, 0.f, -3668.f}, {-18.f, shep::BALC_Y, -3700.f}, {18.f, shep::BALC_Y, -3700.f}, {0.f, shep::BALC_Y, -3712.f}};
        a.waves = {{W(EnemyType::BELLWETHER, 0, {vec3{0.f, 0.f, -3700.f}}), W(EnemyType::STRAY, 3)}};
        l.arenas.push_back(a);
    }

    {   // 3. THE TURN: the ridge (no waves: its adds and its sweeps are the game's, Gameplay_Shepherd.h)
        Arena a;
        a.name = "THE TURN"; a.subtitle = "ITS RIDGE";
        a.zone = LevelBuilder::aabb(shep::RX0 - 1.f, -1000.f, shep::RIDGE_Z - 1.f, shep::RX1 + 1.f, 40.f, shep::RZ_S + 4.f);
        a.bounds = LevelBuilder::aabb(shep::RX0 + 1.f, shep::PLAIN_Y - 2.f, shep::RIDGE_Z + 1.f, shep::RX1 - 1.f, 40.f, shep::RZ_S);
        a.playerStart = shep::ridgeStart(); a.startYaw = -90.f; a.hasRespawn = true; a.respawn = a.playerStart;
        a.voidY = shep::VOID_Y;
        a.theme = ring::eclipseTheme(); a.ambient = Ambient::DUST; a.space = ReverbSpace::OPEN; a.tone = RoomTone::NONE;
        a.groundSpawns = {{590.f, 0.f, -3445.f}, {632.f, 0.f, -3448.f}, {674.f, 0.f, -3446.f}, {610.f, 0.f, -3440.f}, {654.f, 0.f, -3442.f}};
        a.maxAlive = 8;
        l.arenas.push_back(a);
    }
    auto inside = [&](const char* name, const char* sub, AABB zone, vec3 start) {
        Arena a;
        a.name = name; a.subtitle = sub; a.zone = zone; a.bounds = zone; a.bounds.min.y = -1.f;
        a.playerStart = start; a.startYaw = -90.f; a.hasRespawn = true; a.respawn = start;
        a.voidY = -6.f; a.theme = ring::eclipseTheme(); a.theme.lightDir = glm::normalize(vec3{0.2f, -1.f, 0.1f});
        a.ambient = Ambient::MOTES; a.space = ReverbSpace::METAL; a.tone = RoomTone::NONE; a.maxAlive = 6;
        l.arenas.push_back(a);
    };
    const float hx0 = shep::HX - shep::HHALF - shep::ALC_DEPTH, hx1 = shep::HX + shep::HHALF + shep::ALC_DEPTH;
    inside("THE TUNING HALL", "INSIDE IT", LevelBuilder::aabb(hx0, -1000.f, shep::HZ_N, hx1, shep::H_TOP, shep::HZ_S), {shep::HX, 0.f, shep::HZ_S - 5.f});        // 4
    inside("THE LEDGER", "EVERY ONE, COUNTED", LevelBuilder::aabb(shep::HX - shep::HHALF, -1000.f, shep::LZ_N, shep::HX + shep::HHALF, shep::L_TOP, shep::HZ_N), {shep::HX, 0.f, shep::HZ_N - 4.f});   // 5
    inside("THE LAMP WELL", "UP, TO ITS SURFACE", LevelBuilder::aabb(shep::WX0 - 4.f, -1000.f, shep::WZ_N, shep::WX1 + 4.f, shep::W_TOP + 10.f, shep::LZ_N - 0.5f), {shep::HX, 0.f, shep::WZ_S - 3.f});   // 6
    return l;
}
