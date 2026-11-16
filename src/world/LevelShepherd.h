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
// PHASE 4, THE BREAK: it tears the ring apart round itself. Three ledges of drifting debris west of the
// Comb, each higher and further north, then its crown. Low gravity. Clear a ledge and the Gather comes:
// spike a ride chunk and it throws you up to the next
constexpr float BRK_X = -600.f;
constexpr int LEDGES = 3;
inline glm::vec3 ledge(int i) { return {BRK_X, 20.f + 40.f * (float)i, -3300.f - 220.f * (float)i}; }   // its raft's top centre
inline glm::vec3 ledgeStart(int i) { return ledge(i) + glm::vec3{0.f, 0.f, 12.f}; }
inline glm::vec3 crown() { return {BRK_X, 140.f, -3960.f}; }
inline glm::vec3 rideChunk(int i, int k) { return ledge(i) + glm::vec3{k ? 9.f : -9.f, 1.f, -27.f}; }   // its top centre
// PHASE 5, THE FALL: the court of broken orbits on its crown, falling into the giant.
// The colossus rises beyond the north ridge. Its heart housing takes the final HOLDFAST.
// Legacy lamp locations remain for checkpoint compatibility; the living body owns the fight.
constexpr float FALL_Y = 140.f, FALL_X0 = -690.f, FALL_X1 = -510.f, FALL_ZS = -3890.f, FALL_ZN = -4054.f;
constexpr int LAMPS = 6;
constexpr float FIN_DRAIN = 1.5f;   // the finisher: the last count runs out this fast once it is pinned
inline glm::vec3 fallLamp(int i) { const float x[6] = {-642.f, -628.f, -614.f, -586.f, -572.f, -558.f}; return {x[i], FALL_Y + 9.f, FALL_ZN + 1.f}; }
inline glm::vec3 fallCore() { return {-600.f, FALL_Y + 10.f, FALL_ZN + 1.5f}; }
inline glm::vec3 fallStart() { return {-600.f, FALL_Y, -3942.f}; }
inline std::vector<glm::vec2> fallFins() {
    std::vector<glm::vec2> out;
    // Broken orbital vanes flank a clear central route to the exposed heart.
    for(int row=0;row<4;++row)for(float x:{-666.f,-640.f,-614.f,-586.f,-560.f,-534.f})
        out.push_back({x,-4027.f+33.f*row+(x<-600.f?2.f:-2.f)});
    return out;
}
// After it: UNIT 24's own capsule, drifting, far out
inline glm::vec3 driftStart() { return {-1400.f, 262.f, -3420.f}; }
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

    // ---- PHASE 4: THE BREAK. Ledges of debris, its crown ----
    {
        const vec3 rock = glm::mix(iceLit, iceDark, 0.35f), steelLit{1.2f, 1.25f, 1.35f};
        for (int i = 0; i < shep::LEDGES; ++i) {
            const vec3 c = shep::ledge(i);
            b.mat = Mat::ROCK;
            b.wall(c.x - 18.f, c.y - 5.f, c.z - 18.f, c.x + 18.f, c.y, c.z + 18.f, rock);   // the raft
            for (int k = 0; k < 10; ++k) {   // its ragged edge, and what hangs under it
                const float a = 6.2831853f * k / 10.f, q = c.z * 0.1f + k;
                b.kit().rock({c.x + 19.f * ring::dcos(a), c.y - 9.f, c.z + 19.f * ring::dsin(a)}, 3.f + 2.f * ring::hash(q, 1.f), 8.f, glm::mix(rock, iceLit, 0.2f), (uint32_t)(q * 31.f), 0.5f, 7);
            }
            const float off[5][4] = {{-30.f, 3.f, 0.f, 9.f}, {30.f, 5.f, -8.f, 8.f}, {-22.f, 9.f, -24.f, 7.f}, {24.f, 2.f, 22.f, 8.f}, {0.f, 12.f, 28.f, 6.f}};
            for (const auto& o : off) {   // chunks hanging round it: cover, and high ground in the low gravity
                const vec3 p = c + vec3{o[0], o[1], o[2]};
                b.wall(p.x - o[3] * 0.5f, p.y - o[3] * 0.45f, p.z - o[3] * 0.5f, p.x + o[3] * 0.5f, p.y, p.z + o[3] * 0.5f, glm::mix(rock, iceLit, 0.15f));
            }
            for (int k = 0; k < 2; ++k) {   // two drifting chunks, rising and sinking
                const vec3 mc = c + vec3{k ? 14.f : -14.f, 5.f + 2.f * k, k ? 30.f : -12.f};
                const int m = b.mover(mc, vec3{3.f, 0.6f, 3.f}, Mover::Path::PINGPONG, vec3{0.f, -4.f, 0.f}, vec3{0.f, 4.f, 0.f}, 9.f + 3.f * k, 0.3f * k, vec3{0.f});
                l.movers[m].look = 1; l.movers[m].color = glm::mix(iceLit, sand, 0.3f);
            }
            for (int k = 0; k < 2; ++k) {   // the ride chunks: a gap off its north side, a steel spike-post in each (spike one, ride the Gather)
                const vec3 r = shep::rideChunk(i, k);
                b.mat = Mat::ROCK;
                const int w = b.wall(r.x - 3.f, r.y - 3.f, r.z - 2.5f, r.x + 3.f, r.y, r.z + 2.5f, glm::mix(rock, iceLit, 0.3f));
                l.shepRideWalls.push_back(w);
                b.mat = Mat::METAL;
                b.prop(r.x - 0.15f, r.y, r.z - 0.15f, r.x + 0.15f, r.y + 2.4f, r.z + 0.15f, steel);
                b.neon(r.x - 3.f, r.y - 0.4f, r.z + 2.5f, r.x + 3.f, r.y - 0.2f, r.z + 2.6f, steelLit);
                SweepShelter sh; sh.box = LevelBuilder::aabb(r.x - 3.f, r.y - 3.f, r.z - 2.5f, r.x + 3.f, r.y, r.z + 2.5f); sh.holds = true; sh.section = 7 + i;
                sh.post = r + vec3{2.5f, 0.f, 3.5f};
                l.sweepShelters.push_back(sh);
            }
        }
        // ---- PHASE 5: THE FALL. A broken celestial court cut into the crown. ----
        {
            const float y = shep::FALL_Y;
            const vec3 court{-600.f,0.f,-3972.f};
            b.mat = Mat::HULL;
            b.wall(shep::FALL_X0, y - 6.f, shep::FALL_ZN, shep::FALL_X1, y, shep::FALL_ZS, hull);
            // Individual inset slabs, scored seams and occasional circuit panels.
            // Dressing sits flush above one continuous collision floor.
            for(int ix=0;ix<15;++ix)for(int iz=0;iz<14;++iz){
                float x=shep::FALL_X0+12.f*ix,z=shep::FALL_ZN+12.f*iz;
                float far=std::min(z+11.82f,shep::FALL_ZS-.12f);if(far<=z)continue;
                b.mat=(ix+iz*3)%9==0?Mat::CIRCUIT:Mat::HULL;
                b.prop(x+.12f,y+.006f,z+.12f,x+11.88f,y+.025f,far,
                    glm::mix(hull,hullLit,.14f+.22f*ring::hash(float(ix),float(iz))));
            }
            // Tarnished orbital inlays and broken radial spokes echo his halo.
            b.mat=Mat::BRASS;
            const vec3 inlay{.24f,.19f,.105f};
            for(float radius:{13.f,28.f,45.f,63.f,77.f}){
                for(int q=0;q<8;++q){float a=q*.78539816f;
                    b.kit().curve(court,radius,a+.035f,a+.72f,y+.027f,y+.055f,.19f,inlay,10);
                }
                for(int k=0;k<48;++k){float a=k*.13089969f;
                    b.kit().box(court+vec3{radius*std::cos(a),y+.05f,radius*std::sin(a)},
                        {k%6==0?1.7f:.7f,.045f,.13f},inlay,-a);
                }
            }
            b.mat=Mat::BASALT;
            for(int k=0;k<12;++k){float a=k*.52359878f;
                b.kit().box(court+vec3{38.f*std::cos(a),y+.035f,38.f*std::sin(a)},
                    {51.f,.025f,.38f},hull*.5f,-a);
            }
            // A dark compass rosette is the visual centre, kept level for combat.
            b.kit().column(court+vec3{0,y+.026f,0},11.5f,.04f,hullLit*.65f,24);
            b.mat=Mat::BRASS;
            for(int k=0;k<8;++k)b.kit().box(court+vec3{0,y+.073f,0},{19.f,.025f,.12f},inlay,k*.39269908f);
            b.mat=Mat::HULL;
            for(int sd:{-1,1}){
                const float x=-600.f+sd*82.f;
                // Side galleries with real slopes at both ends form long flank routes.
                b.wall(x-5.f,y,-4028.f,x+5.f,y+3.f,-3928.f,hullLit*.7f);
                auto slope=[&](float za,float zb,float ya,float yb){
                    l.ramps.push_back({LevelBuilder::aabb(x-5.f,y,za,x+5.f,y+3.f,zb),2,ya,yb});
                    float dz=zb-za,dy=ya-yb;
                    auto& slab=b.kit().box({0,0,0},{10.f,.4f,std::sqrt(dz*dz+dy*dy)},hullLit*.7f);
                    slab.xf=glm::translate(glm::mat4{1.f},vec3{x,(ya+yb)*.5f,(za+zb)*.5f})
                        *glm::rotate(glm::mat4{1.f},std::atan2(dy,dz),vec3{1,0,0})
                        *glm::translate(glm::mat4{1.f},vec3{0,-.2f,0})*slab.xf;
                };
                slope(-4040.f,-4028.f,y,y+3.f);slope(-3928.f,-3916.f,y+3.f,y);
                b.neon(x-.12f,y+3.02f,-4028.f,x+.12f,y+3.08f,-3928.f,lampWhite*.22f);
                // Exterior flying ribs and split arches frame the giant, never the firing lanes.
                for(int j=0;j<5;++j){float z=-4032.f+j*31.f;
                    float edge=sd<0?shep::FALL_X0:shep::FALL_X1;
                    b.wall(edge-1.3f,y-8.f,z-2.f,edge+1.3f,y+7.f,z+2.f,hull);
                    b.kit().rod({edge,y+6.f,z},{edge+sd*12.f,y+18.f,z-5.f},2.f,hullLit*.6f,6);
                    b.kit().arch({edge+sd*12.f,y+18.f,z-5.f},19.f,16.f,1.4f,3.f,hullLit*.65f,1.5707963f,10);
                    b.kit(true).rod({edge,y+6.5f,z},{edge+sd*11.5f,y+18.f,z-5.f},.12f,lampWhite*.2f,6);
                }
            }
            for(int sd=0;sd<2;++sd){
                float x=sd?shep::FALL_X1-.6f:shep::FALL_X0+.4f;
                b.neon(x,y+.04f,shep::FALL_ZN+2.f,x+.2f,y+.1f,shep::FALL_ZS-1.f,lampWhite*.3f);
            }
            b.neon(shep::FALL_X0+1.f,y+.04f,shep::FALL_ZS-.6f,shep::FALL_X1-1.f,y+.1f,shep::FALL_ZS-.4f,lampWhite*.3f);
            // its ridge: the north edge, rising; the lamps and the core are drawn live in front of it
            b.wall(shep::FALL_X0 - 10.f, y, shep::FALL_ZN - 14.f, shep::FALL_X1 + 10.f, y + 3.f, shep::FALL_ZN, hull);
            b.wall(shep::FALL_X0 - 20.f, y + 3.f, shep::FALL_ZN - 40.f, shep::FALL_X1 + 20.f, y + 6.f, shep::FALL_ZN - 10.f, glm::mix(hull, steel, 0.3f));
            for (float x = shep::FALL_X0 - 8.f; x < shep::FALL_X1 + 8.f; x += 4.5f)   // its fleece along the top of the ridge, peeling as it falls
                if (ring::hash(x, 81.f) < 0.7f) ringCapsule(b, {x, y + 6.f, shep::FALL_ZN - 3.f - 6.f * ring::hash(x, 82.f)}, true, 0, (ring::hash(x, 83.f) - 0.5f) * 30.f, false);
            {   // the core lamp's housing: HOLDFAST goes into this
                const vec3 c = shep::fallCore();
                l.shepCoreWall = b.wall(c.x - 7.f, y, c.z - 1.5f, c.x + 7.f, y + 20.f, c.z + 1.f, glm::mix(hull, hullLit, 0.4f));
            }
            int n = 0;
            for (const glm::vec2& f : shep::fallFins()) {   // broken counterweight vanes, with HOLDFAST posts in their lee
                const AABB box{{f.x - shep::FIN_W * 0.5f, y, f.y - shep::FIN_D * 0.5f}, {f.x + shep::FIN_W * 0.5f, y + shep::FIN_H, f.y + shep::FIN_D * 0.5f}};
                b.wall(box.min.x, box.min.y, box.min.z, box.max.x, box.max.y, box.max.z, glm::mix(hull, hullLit, 0.4f + 0.3f * ring::hash((float)n, 85.f)));
                // Three tapering crests make each shelter a torn orbital tooth.
                b.mat=Mat::BRASS;
                for(int tooth=0;tooth<3;++tooth)b.kit().column({f.x-2.1f+tooth*2.1f,y+shep::FIN_H,f.y},.72f,1.2f+(tooth==1?1.f:0.f),inlay,5,.15f);
                b.mat=Mat::HULL;
                for(int strut:{-1,1})b.kit().rod({f.x+strut*2.5f,y+.3f,f.y+1.4f},{f.x+strut*2.5f,y+4.4f,f.y+.65f},.24f,hullLit,6);
                b.neon(box.min.x + 0.4f, box.max.y - 0.5f, box.max.z + 0.01f, box.max.x - 0.4f, box.max.y - 0.36f, box.max.z + 0.06f, lampWhite * 0.25f);
                SweepShelter sh; sh.box = box; sh.section = 10; sh.post = {box.max.x - 0.8f, y, box.max.z + 1.2f};
                post(sh.post); l.sweepShelters.push_back(sh); ++n;
            }
        }
        {   // UNIT 24's capsule, far out, drifting (the end)
            const vec3 d = shep::driftStart();
            b.mat = Mat::METAL;
            b.wall(d.x - 3.f, d.y - 2.f, d.z - 5.f, d.x + 3.f, d.y, d.z + 5.f, glm::mix(hull, hullLit, 0.3f));
            ringCapsule(b, d + vec3{0.f, 0.f, -2.f}, false, 0, 0.f, false);
            b.neon(d.x - 0.4f, d.y + 0.02f, d.z + 3.f, d.x + 0.4f, d.y + 0.06f, d.z + 3.6f, vec3{2.4f, 1.3f, 0.35f} * 0.8f);
        }
        b.mat = Mat::ROCK;
        for (int k = 0; k < 60; ++k) {   // the torn ring: chunks hanging all round, far off
            const float fk = (float)k;
            const vec3 p{shep::BRK_X + (ring::hash(fk, 61.f) - 0.5f) * 500.f, -20.f + 220.f * ring::hash(fk, 62.f), -3250.f - 760.f * ring::hash(fk, 63.f)};
            if (std::fabs(p.x - shep::BRK_X) < 60.f) continue;
            const float r = 3.f + 9.f * ring::hash(fk, 64.f);
            ringChunk(b, LevelBuilder::aabb(p.x - r, p.y - r * 0.5f, p.z - r * 0.7f, p.x + r, p.y + r * 0.5f, p.z + r * 0.7f), glm::mix(iceLit, iceDark, 0.3f + 0.4f * ring::hash(fk, 65.f)), (uint32_t)(k * 13 + 7));
        }
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
        a.name = "THE ORRERY"; a.subtitle = "THREE CAPTIVE MOONS";
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
    for (int i = 0; i < shep::LEDGES; ++i) {   // 7-9: THE BREAK
        const vec3 c = shep::ledge(i);
        static const char* subs[3] = {"IT HAS LOST ITS GRIP", "HIGHER", "ITS CROWN ABOVE"};
        Arena a;
        a.name = "THE BREAK"; a.subtitle = subs[i];
        a.zone = LevelBuilder::aabb(c.x - 50.f, -1000.f, c.z - 45.f, c.x + 50.f, c.y + 60.f, c.z + 45.f);
        a.bounds = LevelBuilder::aabb(c.x - 36.f, c.y - 8.f, c.z - 34.f, c.x + 36.f, c.y + 40.f, c.z + 34.f);
        a.playerStart = shep::ledgeStart(i); a.startYaw = -90.f; a.hasRespawn = true; a.respawn = a.playerStart;
        a.voidY = c.y - 25.f;
        a.theme = ring::eclipseTheme(); a.ambient = Ambient::SPECKS; a.space = ReverbSpace::OPEN; a.tone = RoomTone::NONE;
        a.hasTrigger = true; a.trigger = LevelBuilder::aabb(c.x - 18.f, c.y - 2.f, c.z - 18.f, c.x + 18.f, c.y + 20.f, c.z + 18.f);
        a.groundSpawns = {c + vec3{-12.f, 0.f, -12.f}, c + vec3{12.f, 0.f, -10.f}, c + vec3{-14.f, 0.f, 8.f}, c + vec3{14.f, 0.f, 6.f}, c + vec3{0.f, 0.f, -15.f}, c + vec3{-30.f, 3.f, 0.f}, c + vec3{30.f, 5.f, -8.f}};
        a.airSpawns = {c + vec3{-10.f, 5.f, -20.f}, c + vec3{10.f, 5.f, -20.f}, c + vec3{0.f, 8.f, 20.f}};
        a.maxAlive = 8; a.waveBreak = 2.f;
        using W = WaveEntry;
        if (i == 0) a.waves = {{W(EnemyType::STRAY, 3), W(EnemyType::HERDER, 2)}};
        if (i == 1) a.waves = {{W(EnemyType::STRAY, 2), W(EnemyType::TALLY, 2), W(EnemyType::PORTER, 1)}};
        if (i == 2) a.waves = {{W(EnemyType::STRAY, 2), W(EnemyType::HERDER, 2), W(EnemyType::PORTER, 1)}};
        l.arenas.push_back(a);
    }
    {   // 10. THE FALL (its fight is the game's: Gameplay_ShepherdFall.h)
        Arena a;
        a.name = "THE FALL"; a.subtitle = "THE COURT OF BROKEN ORBITS";
        a.zone = LevelBuilder::aabb(shep::FALL_X0, -1000.f, shep::FALL_ZN, shep::FALL_X1, shep::FALL_Y + 60.f, shep::FALL_ZS + 16.f);
        a.bounds = LevelBuilder::aabb(shep::FALL_X0 + 1.f, shep::FALL_Y - 2.f, shep::FALL_ZN + 1.f, shep::FALL_X1 - 1.f, shep::FALL_Y + 40.f, shep::FALL_ZS);
        a.playerStart = shep::fallStart(); a.startYaw = -90.f; a.hasRespawn = true; a.respawn = a.playerStart;
        a.voidY = shep::FALL_Y - 25.f;
        a.theme = ring::eclipseTheme(); a.ambient = Ambient::EMBERS; a.space = ReverbSpace::OPEN; a.tone = RoomTone::NONE;
        a.groundSpawns = {{-640.f, shep::FALL_Y, -4000.f}, {-560.f, shep::FALL_Y, -4002.f}, {-620.f, shep::FALL_Y, -4030.f}, {-580.f, shep::FALL_Y, -4030.f}};
        a.maxAlive = 7;
        l.arenas.push_back(a);
    }
    {   // 11. Drifting: UNIT 24's capsule
        const vec3 d = shep::driftStart();
        Arena a;
        a.name = "THE KEPT RING"; a.subtitle = "";
        a.zone = LevelBuilder::aabb(d.x - 30.f, -1000.f, d.z - 30.f, d.x + 30.f, d.y + 40.f, d.z + 30.f);
        a.bounds = a.zone; a.bounds.min.y = d.y - 3.f;
        a.playerStart = d + vec3{0.f, 0.f, 3.f}; a.startYaw = -90.f; a.hasRespawn = true; a.respawn = a.playerStart;
        a.voidY = d.y - 20.f;
        a.theme = ring::eclipseTheme(); a.ambient = Ambient::SPECKS; a.space = ReverbSpace::OPEN; a.tone = RoomTone::NONE;
        l.arenas.push_back(a);
    }
    return l;
}
