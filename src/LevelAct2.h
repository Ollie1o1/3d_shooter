#pragma once
// =============================================================================
// LevelAct2.h — ACT II, BENEATH THE ECLIPSE: the arenas under the Sanctum.
//
// When the Sovereign falls the Sanctum cracks and you drop into the machine
// beneath it. Act II is built in its own region, 60 m under Act I and north
// of it (floors at Y -60, everything at Z < -440), so ASCENT can build both
// acts into one world and the fall is a real fall.
//
//                N (-Z)
//        ┌───────────────────┐  Z -622..-652  CHANCEL (+3 m) and the drowned
//        │  ORGAN    (loft)  │                organ: pipes climb to a +12 m loft
//   ┌────┴───┐ CROSSING ┌────┴───┐  Z -592..-622  transepts (+1.5 m) to X ±50,
//   │TRANSEPT│ (light   │TRANSEPT│                rose windows; ring walkway (+9 m)
//   └────┬───┘  shaft)  └───┬────┘
//        │ ║  NAVE  (0) ║   │  Z -473..-592  nave X ±16; aisles (+1.5) to ±29,
//        │ ║═══════════ ║   │                galleries (+6) over them, buttress
//        │ ║  bridges   ║   │                bridges (+9), clerestory (+12)
//        └─┴──┐ door ┌──┴───┘
//             │NARTHEX│     Z -450..-472  the shaft you fall down
//             └───────┘
//                S (+Z)
//
// The water: one volume over the whole floor. FLOOD raises it each wave
// (0.4 m, 2.0 m, 2.6 m over the nave floor).
// Waves: Haloed Husks and Twinned Rippers; Seraphs over the conduits; then an
// Anchor first, with Twinned Shieldbearers, Seraphs and a Haloed Juggernaut.
// =============================================================================
#include "Level.h"

inline void buildDescent(LevelBuilder& B);

inline void buildAct2(LevelBuilder& B) {
    using glm::vec3;
    LevelData& L = B.L;
    auto aabb = &LevelBuilder::aabb;
    auto wall = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { return B.wall(x0,y0,z0,x1,y1,z1,c); };
    auto neon = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { B.neon(x0,y0,z0,x1,y1,z1,c); };

    const float F = -60.f;   // the Nave's floor
    vec3 bone{0.74f,0.70f,0.62f}, boneDark{0.46f,0.44f,0.40f}, slate{0.15f,0.19f,0.21f},
         teal{0.2f,0.9f,0.85f}, red{1.f,0.16f,0.08f}, brass{0.62f,0.5f,0.3f}, light{1.4f,1.25f,1.0f};

    // All of Act II stands in one basin, 60 m down
    L.basins.push_back({aabb(-300, 0, -440, 300, 0, -668), F});          // the Nave
    L.basins.push_back({aabb(-300, 0, -668, 300, 0, -788), -140.f});    // the Orrery: its void bottoms out far below
    L.basins.push_back({aabb(-300, 0, -788, 300, 0, -1000), -300.f});   // the Descent: open below the cage

    Arena a;
    a.name = "THE DROWNED NAVE"; a.space = ReverbSpace::HALL;
    a.subtitle = "THE WATER RISES - SURVIVE 3 WAVES";
    a.bounds = aabb(-49, F, -651, 49, F + 26, -451);
    a.zone   = aabb(-50.5f, F, -652.5f, 50.5f, F + 28, -449.5f);   // ceiling under the dome
    a.extraZones.push_back(aabb(-11.5f, F, -471.5f, 11.5f, -2.f, -450.5f));   // the shaft over the narthex
    a.extraZones.push_back(aabb(-2.9f, F + 3, -662.f, 2.9f, F + 9, -652.f));   // the sealed passage
    a.playerStart = {0.f, -8.f, -460.f};
    a.startYaw = -90.f;

    // ---- floors (drawn) ----
    L.floors.push_back({-300.f, -668.f, 300.f, -440.f, F - 0.3f, {0.05f, 0.06f, 0.07f}});    // seen from wall tops
    L.floors.push_back({-12.f, -472.f, 12.f, -450.f, F, slate});                             // narthex
    L.floors.push_back({-16.f, -622.f, 16.f, -472.f, F, {0.2f, 0.22f, 0.22f}});              // nave + crossing

    // ---- the narthex and its shaft ----
    B.mat = Mat::BRICK;
    wall(-13, F, -472, -12, -2, -450, boneDark);
    wall( 12, F, -472,  13, -2, -450, boneDark);
    wall(-13, F, -450,  13, -2, -449, boneDark);
    for (float y = F + 8.f; y < -4.f; y += 7.f) {                 // rings of light down the shaft
        neon(-11.95f, y, -471.9f, -11.85f, y + 0.25f, -450.1f, teal * 0.7f);
        neon(11.85f, y, -471.9f, 11.95f, y + 0.25f, -450.1f, teal * 0.7f);
    }
    // the great doorway into the nave (open), and the wall above it up to the shaft's top
    B.wallX(-29, 29, -473, -472, F, F + 26, bone, {{-5.f, 5.f, F, F + 8.f}});
    wall(-13, F + 26, -473, 13, -2, -472, bone);
    B.kit().arch({0.f, F + 8.f, -472.5f}, 10.f, 3.f, 0.8f, 1.4f, boneDark);

    // ---- the nave: outer walls, aisles, arcades, galleries ----
    for (int s : {-1, 1}) {
        float sx = (float)s;
        auto lo = [&](float u, float v) { return std::min(sx * u, sx * v); };
        auto hi = [&](float u, float v) { return std::max(sx * u, sx * v); };
        wall(lo(29, 30), F, -592, hi(29, 30), F + 26, -473, bone);                     // outer wall
        wall(lo(16, 29), F, -592, hi(16, 29), F + 1.5f, -473, boneDark);               // aisle floor (+1.5)
        // arcade columns every 12 m; the galleries rest on them
        for (float z = -479.f; z > -592.f; z -= 12.f) {
            wall(sx * 16.f - 0.8f, F, z - 0.8f, sx * 16.f + 0.8f, F + 5.f, z + 0.8f, bone);
            B.kit().column({sx * 16.f, F, z}, 0.95f, 5.f, bone, 12);
        }
        // gallery deck (+6), broken where the arcade collapsed (west Z -528..-536, east -552..-560)
        float g0 = s < 0 ? -528.f : -552.f, g1 = s < 0 ? -536.f : -560.f;
        wall(lo(15.2f, 29), F + 5, g0, hi(15.2f, 29), F + 6, -473, boneDark);
        wall(lo(15.2f, 29), F + 5, -592, hi(15.2f, 29), F + 6, g1, boneDark);
        neon(lo(15.2f, 29), F + 5.9f, g0 - 0.12f, hi(15.2f, 29), F + 6.02f, g0, red * 0.6f);   // broken edges
        neon(lo(15.2f, 29), F + 5.9f, g1, hi(15.2f, 29), F + 6.02f, g1 + 0.12f, red * 0.6f);
        // pointed arches between the columns, except over the collapse
        for (float z = -485.f; z > -592.f; z -= 12.f) {
            if (z < g0 + 6.f && z > g1 - 6.f) continue;
            B.kit().arch({sx * 16.f, F + 1.5f, z}, 10.4f, 3.4f, 0.7f, 1.2f, bone, 1.5707963f);
        }
        // the collapsed span: rubble in the water under it
        for (int k = 0; k < 4; ++k)
            B.kit().rock({sx * (19.f + 2.5f * k), F + 1.5f, (g0 + g1) * 0.5f + (k % 2 ? 1.5f : -1.5f)}, 1.3f, 1.2f,
                         boneDark, 900u + (uint32_t)k + (s > 0 ? 10u : 0u));
        // a pad in the aisle under the gap: up through it onto the gallery beyond
        L.pads.push_back({{sx * 22.f, F + 1.5f, (g0 + g1) * 0.5f}, {1.2f, 1.2f}, {0.f, 17.f, 5.f}});
        // clerestory ledges (+12) and pads up to them from the gallery
        wall(lo(26, 29), F + 11, -589, hi(26, 29), F + 12, -476, boneDark);
        for (float z : {-482.f, -584.f}) L.pads.push_back({{sx * 23.5f, F + 6.f, z}, {1.1f, 1.1f}, {sx * 2.2f, 18.5f, 0.f}});
        // tall windows glowing red from below
        for (float z = -485.f; z > -592.f; z -= 24.f)
            neon(lo(28.9f, 28.95f), F + 13.f, z - 2.f, hi(28.9f, 28.95f), F + 22.f, z + 2.f, red * 0.9f);
    }
    // the buttress bridges (+9) across the nave, with steps up from the galleries
    for (float z : {-500.f, -545.f}) {
        wall(-17, F + 8, z - 1.5f, 17, F + 9, z + 1.5f, bone);
        B.kit().arch({0.f, F + 2.f, z}, 32.f, 6.f, 0.9f, 2.6f, boneDark);    // the flying buttress under it
        for (int s : {-1, 1})
            wall(std::min(s * 17.f, s * 20.f), F + 6, z - 1.5f, std::max(s * 17.f, s * 20.f), F + 7.5f, z + 1.5f, boneDark);
        neon(-17, F + 8.95f, z - 0.15f, 17, F + 9.02f, z + 0.15f, teal * 0.5f);
    }
    // pews: low cover on the nave floor, two blocks of rows
    for (int s : {-1, 1}) for (float z = -486.f; z > -580.f; z -= 9.f) {
        if (((int)(-z) / 9) % 4 == 2) continue;   // gaps where pews were swept away
        wall(std::min(s * 3.f, s * 13.f), F, z - 0.35f, std::max(s * 3.f, s * 13.f), F + 0.9f, z + 0.35f, slate);
    }
    // a fallen column lying across the nave (collision as three blocks under it)
    B.kit().rod({-12.f, F + 0.9f, -560.f}, {8.f, F + 0.9f, -571.f}, 1.0f, bone, 12);
    B.solid(-12.5f, F, -563.5f, -5.f, F + 1.8f, -558.f);
    B.solid(-5.f, F, -567.5f, 2.5f, F + 1.8f, -562.f);
    B.solid(2.5f, F, -571.5f, 8.5f, F + 1.8f, -566.f);

    // ---- the crossing, the transepts, the light ----
    for (int s : {-1, 1}) {
        float sx = (float)s;
        auto lo = [&](float u, float v) { return std::min(sx * u, sx * v); };
        auto hi = [&](float u, float v) { return std::max(sx * u, sx * v); };
        wall(lo(16, 50), F, -622, hi(16, 50), F + 1.5f, -592, boneDark);     // transept floor (+1.5)
        wall(lo(29, 51), F, -593, hi(29, 51), F + 26, -592, bone);           // south wall
        wall(lo(16, 51), F, -623, hi(16, 51), F + 26, -622, bone);           // north wall
        wall(lo(50, 51), F, -622, hi(50, 51), F + 26, -592, bone);           // end wall
        // rose window: eight spokes of red light round a hub
        vec3 c{sx * 49.9f, F + 15.f, -607.f};
        for (int k = 0; k < 8; ++k) {
            float ang = k * 0.7853982f;
            B.kit(true).rod(c, c + vec3{0.f, std::sin(ang) * 6.f, std::cos(ang) * 6.f}, 0.18f, red * 0.9f, 6);
        }
        B.kit(true).column(c - vec3{sx * 0.2f, 0.8f, 0.f}, 0.9f, 1.6f, red, 10);
        // plinth pads up to the ring walkway
        wall(sx * 10.f - 1.3f, F, -601.3f, sx * 10.f + 1.3f, F + 1.5f, -598.7f, slate);
        L.pads.push_back({{sx * 10.f, F + 1.5f, -600.f}, {1.1f, 1.1f}, {sx * 3.5f, 22.f, 0.f}});
    }
    // ring walkway (+9) round the crossing
    wall(-16, F + 8, -594.5f, 16, F + 9, -592, boneDark);
    wall(-16, F + 8, -622, 16, F + 9, -619.5f, boneDark);
    wall(-16, F + 8, -619.5f, -13.5f, F + 9, -594.5f, boneDark);
    wall(13.5f, F + 8, -619.5f, 16, F + 9, -594.5f, boneDark);
    // the crossing piers, and the cracked dome over it (seen through the ceiling's force field)
    for (int sx : {-1, 1}) for (int sz : {-1, 1})
        wall(sx * 16.f - 1.5f, F, -607.f + sz * 15.f - 1.5f, sx * 16.f + 1.5f, F + 26, -607.f + sz * 15.f + 1.5f, bone);
    B.kit().curve({0.f, 0.f, -607.f}, 17.f, 0.f, 6.2831853f, F + 26.f, F + 30.f, 1.2f, boneDark, 24);
    B.kit().dome({0.f, F + 30.f, -607.f}, 17.f, boneDark, 6, 18);
    B.kit(true).rod({-3.f, F + 46.f, -607.f}, {5.f, F + 44.f, -603.f}, 0.4f, light * 0.6f, 6);   // the crack
    // the eclipse light falling through it: thin rays, slightly splayed, round a beam
    for (int k = 0; k < 7; ++k) {
        float ang = k * 0.8975979f;
        vec3 top{std::cos(ang) * 1.2f, F + 44.f, -607.f + std::sin(ang) * 1.2f};
        vec3 foot{std::cos(ang + 0.4f) * 2.8f, F + 0.2f, -607.f + std::sin(ang + 0.4f) * 2.8f};
        B.kit(true).rod(foot, top, 0.07f + 0.03f * (k % 3), light * 0.55f, 5);
    }
    L.gems.push_back({{0.f, F + 0.4f, -607.f}, light, 1.2f, true});

    // ---- the chancel and the drowned organ ----
    wall(-16, F, -652, 16, F + 3, -622, slate);                    // chancel (+3)
    wall(-16, F, -622, 16, F + 1.5f, -619, slate);                 // its step
    wall(-17, F, -652, -16, F + 26, -622, bone);
    wall( 16, F, -652,  17, F + 26, -622, bone);
    // back wall with the sealed way on
    B.wallX(-17, 17, -653, -652, F, F + 26, bone, {{-3.f, 3.f, F + 3.f, F + 8.f}});
    a.exitDoor = B.doorway(true, -3, 3, -653, -652, F + 3, 5.f, red, true);
    // the passage on, north to the shaft down to the Orrery
    wall(-4, F, -668, -3, F + 9, -653, slate); wall(3, F, -668, 4, F + 9, -653, slate);
    wall(-3, F, -668, 3, F + 3, -653, slate);                      // passage floor (+3)
    wall(-4, F + 9, -668, 4, F + 10, -653, slate);
    // organ pipes: a staircase of double jumps from the chancel to the loft (+12)
    B.mat = Mat::METAL;
    const float pipeX[] = {-9.f, -6.f, -3.f, 0.f, 3.f, 6.f};
    for (int k = 0; k < 6; ++k) {
        float h = 4.5f + 1.5f * k;
        wall(pipeX[k] - 0.7f, F + 3, -646.7f, pipeX[k] + 0.7f, F + h, -645.3f, brass);
        B.kit().column({pipeX[k], F + 3.f, -646.f}, 0.75f, h - 3.f, brass, 12);
        B.kit(true).column({pipeX[k], F + h - 0.15f, -646.f}, 0.78f, 0.15f, red, 12);
    }
    for (float x : {-12.f, -10.5f, 9.f, 10.5f, 12.f})
        B.kit().column({x, F + 3.f, -649.f}, 0.55f, 14.f + std::fabs(x) * 0.4f, brass * 0.8f, 10);
    B.mat = Mat::BRICK;
    wall(-14, F + 11, -652, 14, F + 12, -648.5f, boneDark);        // organ loft (+12)

    // ---- the water ----
    L.water.push_back({aabb(-50, F, -652, 50, F, -450), F + 0.4f});

    // ---- spawns, waves ----
    a.groundSpawns = {
        {-8, F, -490}, {8, F, -509}, {-8, F, -538}, {8, F, -578},                                   // nave floor
        {-22, F + 1.5f, -500}, {22, F + 1.5f, -520}, {-22, F + 1.5f, -575}, {22, F + 1.5f, -585},   // aisles
        {-38, F + 1.5f, -607}, {38, F + 1.5f, -607},                                                // transepts
        {-8, F + 3, -634}, {8, F + 3, -634},                                                        // chancel
        {-23, F + 6, -490}, {23, F + 6, -505}, {-23, F + 6, -580}, {23, F + 6, -575},               // galleries
    };
    a.airSpawns = {{0, F + 13, -495}, {0, F + 13, -560}, {-32, F + 11, -607}, {32, F + 11, -607}};
    // Wave 3 opens with the Anchor (first in the queue): kill it before the rest pile in
    a.waves = {
        {{EnemyType::HUSK, 4}, WaveEntry(EnemyType::HUSK, 3).hollow(Hollow::HALOED), {EnemyType::RIPPER, 3},
         WaveEntry(EnemyType::RIPPER, 2).hollow(Hollow::TWINNED), {EnemyType::SENTINEL, 3},
         WaveEntry(EnemyType::SHIELDBEARER, 2).with({EnemyType::HUSK})},
        {{EnemyType::RAPTOR, 3}, {EnemyType::SERAPH, 2}, {EnemyType::BRUTE, 1},
         WaveEntry(EnemyType::BRUTE, 1).hollow(Hollow::ENRAGED), {EnemyType::CONDUCTOR, 2}, {EnemyType::MITE, 6}},
        {{EnemyType::ANCHOR, 1}, {EnemyType::BRUTE, 2},
         WaveEntry(EnemyType::SHIELDBEARER, 2).with({EnemyType::SENTINEL}).hollow(Hollow::TWINNED),
         {EnemyType::CONDUCTOR, 2}, {EnemyType::SERAPH, 2}, WaveEntry(EnemyType::JUGGERNAUT, 1).hollow(Hollow::HALOED),
         {EnemyType::RIPPER, 4}},
    };
    a.goals = {WaveGoal{},
               WaveGoal::conduits("DESTROY THE CONDUITS ON THE GALLERIES",
                                  {{-23.f, F + 6.f, -497.f}, {23.f, F + 6.f, -522.f}, {23.f, F + 6.f, -582.f}}),
               WaveGoal{}};
    a.maxAlive = 11;
    a.damageScale = 1.3f;
    a.shift = ArenaShift::FLOOD;
    a.floodLevels = {F + 0.4f, F + 2.0f, F + 2.6f};
    a.ambient = Ambient::MOTES;
    a.theme = Theme{
        {0.01f,0.03f,0.04f}, {0.06f,0.16f,0.17f}, {0.12f,0.02f,0.02f},
        glm::normalize(vec3{0.f, 0.95f, -0.2f}), {1.6f,1.4f,1.1f}, 0.05f, 0.f,
        {0.02f,0.05f,0.06f}, 0.3f,
        // Pale light falls through the crack; the red comes up from the water
        glm::normalize(vec3{0.15f,-1.f,0.1f}), {0.85f,0.9f,0.95f},
        {0.16f,0.3f,0.32f}, {0.28f,0.06f,0.05f},
        {0.05f,0.12f,0.13f}, 0.012f };
    L.arenas.push_back(std::move(a));

    // =========================================================================
    // The corridor down: the Nave's passage drops down a shaft to an apron at
    // the Orrery's south gate. Lighting blends from teal to gold on the way.
    // =========================================================================
    const float O = -80.f;                  // the Orrery's terrace
    const vec3 C{0.f, O, -732.f};           // its centre (the sun's axis)
    vec3 stone{0.42f,0.36f,0.30f}, stoneDark{0.24f,0.2f,0.17f}, brassO{0.75f,0.58f,0.3f},
         gold{1.f,0.72f,0.3f}, sunCol{1.6f,1.05f,0.45f};
    B.mat = Mat::BRICK;
    wall(-4, O - 2, -669, 4, -57, -668, slate);                    // under the passage's end
    wall(-6, O - 2, -669, -4, -49, -653, slate);                   // no slot beside the passage's end
    wall( 4, O - 2, -669,  6, -49, -653, slate);
    wall(-6, O - 2, -681, -5, -50, -664, slate);                   // the shaft's sides
    wall( 5, O - 2, -681,  6, -50, -664, slate);
    wall(-6, -50, -681, 6, -49, -664, slate);                      // its roof
    wall(-5, O - 1, -680, 5, O, -668, stoneDark);                  // the apron at the bottom
    for (float y = O + 4.f; y < -54.f; y += 5.f) {                 // rings of light down the shaft
        neon(-4.95f, y, -679.9f, -4.85f, y + 0.2f, -668.1f, gold * 0.6f);
        neon(4.85f, y, -679.9f, 4.95f, y + 0.2f, -668.1f, gold * 0.6f);
    }
    L.corridors.push_back(aabb(-6, O, -684, 6, -50, -652));

    // =========================================================================
    // THE ORRERY — a solid terrace round a void; two rings turn over a captive
    // sun, the outer one clockwise, the inner one (2 m higher) against it
    // =========================================================================
    Arena o;
    o.name = "THE ORRERY"; o.space = ReverbSpace::HALL;
    o.subtitle = "MIND THE SUN - SURVIVE 3 WAVES";
    o.bounds = aabb(-52, O, -784, 52, O + 28, -680);
    o.zone   = aabb(-53.5f, O, -785.5f, 53.5f, O + 30, -678.5f);
    o.playerStart = {0.f, O, -686.f};
    o.startYaw = -90.f;
    o.respawn = {0.f, O, -690.f}; o.hasRespawn = true;
    o.voidY = -105.f;
    o.sunPos = {C.x, -92.f, C.z};

    // ---- the terrace: an annulus R 30-52, built of strips. East and west of
    // the pit the strips run along X, north and south along Z, so the pit's
    // edge is never more than a strip's width out of round anywhere. Drawn
    // exactly as you stand on it: what you see is what holds you.
    const float RIN = 30.f, ROUT = 53.f, STRIP = 0.75f;
    auto band = [&](bool alongX, float side, float u0, float u1) {
        // u: across the strip (z for alongX, x otherwise); the strip runs outward along the other axis
        float un = (u0 < 0.f && u1 > 0.f) ? 0.f : std::min(std::fabs(u0), std::fabs(u1));
        float edge = std::sqrt(std::max(0.f, RIN * RIN - un * un));     // the pit's widest point in the strip
        float from = std::max(edge, un), to = std::sqrt(std::max(0.f, ROUT * ROUT - un * un));
        if (from >= to) return;
        float a0 = side * from, a1 = side * to;
        if (alongX) wall(C.x + std::min(a0, a1), O - 1, C.z + u0, C.x + std::max(a0, a1), O, C.z + u1, stone);
        else        wall(C.x + u0, O - 1, C.z + std::min(a0, a1), C.x + u1, O, C.z + std::max(a0, a1), stone);
        if (edge > un + 1e-3f) {   // this end meets the pit: its lit lip
            float e = side * edge;
            if (alongX) neon(C.x + e - 0.06f, O - 0.3f, C.z + u0, C.x + e + 0.06f, O + 0.03f, C.z + u1, gold * 0.7f);
            else        neon(C.x + u0, O - 0.3f, C.z + e - 0.06f, C.x + u1, O + 0.03f, C.z + e + 0.06f, gold * 0.7f);
        }
    };
    for (float u = -ROUT; u < ROUT - 1e-3f; u += STRIP) {
        float u1 = std::min(u + STRIP, ROUT);
        for (float side : {-1.f, 1.f}) { band(true, side, u, u1); band(false, side, u, u1); }
    }

    // ---- the outer wall (posts round R 53) with a gate on the south ----
    for (int k = 0; k < 96; ++k) {
        float ang = k * 6.2831853f / 96.f;
        vec3 p = C + vec3{std::cos(ang) * 54.f, 0.f, std::sin(ang) * 54.f};
        if (std::fabs(p.x) < 5.5f && p.z > C.z) continue;          // the south gate
        if (std::fabs(p.x) < 5.5f && p.z < C.z) continue;          // the north arch, the way down
        B.solid(p.x - 1.8f, O, p.z - 1.8f, p.x + 1.8f, O + 14.f, p.z + 1.8f);
    }
    B.kit().curve({C.x, 0.f, C.z}, 54.f, 0.f, 6.2831853f, O, O + 14.f, 2.f, stoneDark, 64);
    B.kit(true).curve({C.x, 0.f, C.z}, 52.9f, 0.f, 6.2831853f, O + 13.f, O + 13.3f, 0.2f, gold * 0.8f, 64);
    wall(-6, O, -682, -4, O + 14, -678, stoneDark); wall(4, O, -682, 6, O + 14, -678, stoneDark);   // gate posts
    wall(-4, O + 7, -681, 4, O + 14, -679, stoneDark);
    o.entryGate = B.doorway(true, -4, 4, -681, -679, O, 7.f, gold, false);
    wall(-6, O, -790, -4, O + 14, -782, stoneDark); wall(4, O, -790, 6, O + 14, -782, stoneDark);   // north arch posts
    wall(-4, O + 7, -789, 4, O + 14, -783, stoneDark);
    o.exitDoor = B.doorway(true, -4, 4, -787, -785, O, 7.f, gold, true);

    // ---- eight great pillars (cover from the flare, grapple anchors) ----
    for (int k = 0; k < 8; ++k) {
        float ang = (k + 0.5f) * 0.7853982f;
        vec3 p = C + vec3{std::cos(ang) * 41.f, 0.f, std::sin(ang) * 41.f};
        wall(p.x - 1.3f, O, p.z - 1.3f, p.x + 1.3f, O + 18.f, p.z + 1.3f, stone);
        B.kit().column({p.x, O, p.z}, 1.5f, 18.f, stone, 12);
        B.kit(true).column({p.x, O + 17.6f, p.z}, 1.6f, 0.3f, gold, 12);
        B.kit().column({p.x, O + 18.f, p.z}, 2.1f, 1.f, brassO, 12, 0.7f);
    }

    // ---- four spokes reaching in to R 25 (a short jump onto the outer ring) ----
    for (int k = 0; k < 4; ++k) {
        float cx = k == 0 ? 1.f : k == 1 ? -1.f : 0.f, cz = k == 2 ? 1.f : k == 3 ? -1.f : 0.f;
        float x0 = C.x + cx * 25.f, x1 = C.x + cx * 31.f, z0 = C.z + cz * 25.f, z1 = C.z + cz * 31.f;
        if (cx != 0.f) wall(std::min(x0, x1), O - 1, C.z - 1.5f, std::max(x0, x1), O, C.z + 1.5f, stoneDark);
        else           wall(C.x - 1.5f, O - 1, std::min(z0, z1), C.x + 1.5f, O, std::max(z0, z1), stoneDark);
        vec3 tip = C + vec3{cx * 25.1f, 0.f, cz * 25.1f};
        neon(tip.x - 1.5f, O - 0.05f, tip.z - 1.5f, tip.x + 1.5f, O + 0.04f, tip.z + 1.5f, gold * 0.5f);
    }

    // ---- the rings: segments orbiting the sun's axis ----
    vec3 ringGlow{1.f, 0.7f, 0.3f};
    for (int k = 0; k < 26; ++k)       // outer: R 22, tops at the terrace's height, clockwise
        B.mover({C.x, O - 0.5f, C.z}, {2.5f, 0.5f, 2.5f}, Mover::Path::ORBIT,
                {22.f, 0.f, 0.f}, {0.f, 0.f, 22.f}, 62.83f, k / 26.f, ringGlow);
    int firstInner = (int)L.movers.size();
    for (int k = 0; k < 16; ++k)       // inner: R 12, 2 m higher, the other way
        B.mover({C.x, O + 1.5f, C.z}, {2.2f, 0.5f, 2.2f}, Mover::Path::ORBIT,
                {12.f, 0.f, 0.f}, {0.f, 0.f, -12.f}, 34.27f, k / 16.f, ringGlow);

    // ---- the sun, sunk in the pit, and the armillary arcs over everything ----
    for (int k = 0; k < 9; ++k) {
        float y0 = -101.f + k * 2.f, yc = y0 + 1.f;
        float r = std::sqrt(std::max(1.f, 81.f - (yc + 92.f) * (yc + 92.f)));
        B.kit(true).column({C.x, y0, C.z}, r, 2.f, sunCol * (0.7f + 0.05f * k), 20);
    }
    for (float yaw : {0.f, 1.5707963f, 0.7853982f})
        B.kit().arch({C.x, O, C.z}, 96.f, 40.f, 0.7f, 0.7f, brassO, yaw, 24);
    B.kit(true).curve({C.x, 0.f, C.z}, 48.f, 0.f, 6.2831853f, O + 38.f, O + 38.4f, 0.3f, gold * 0.6f, 48);


    // ---- spawns, waves ----
    for (int k = 0; k < 8; ++k) {
        float ang = k * 0.7853982f;
        float r = k % 2 ? 46.f : 36.f;
        o.groundSpawns.push_back(C + vec3{std::cos(ang) * r, 0.f, std::sin(ang) * r});
    }
    o.airSpawns = {C + vec3{0, 12, 18}, C + vec3{0, 12, -18}, C + vec3{18, 14, 0}, C + vec3{-18, 14, 0}};
    o.waves = {
        {{EnemyType::HUSK, 5}, WaveEntry(EnemyType::SENTINEL, 2).hollow(Hollow::HALOED), {EnemyType::RAPTOR, 3},
         {EnemyType::SERAPH, 1}, WaveEntry(EnemyType::SHIELDBEARER, 2).with({EnemyType::HUSK})},
        {{EnemyType::SERAPH, 2}, {EnemyType::RAPTOR, 3}, {EnemyType::CONDUCTOR, 2}, {EnemyType::MITE, 6},
         WaveEntry(EnemyType::RIPPER, 2).hollow(Hollow::TWINNED)},
        {{EnemyType::ANCHOR, 2}, WaveEntry(EnemyType::BRUTE, 2).hollow(Hollow::ENRAGED),
         WaveEntry(EnemyType::SHIELDBEARER, 2).with({EnemyType::SENTINEL}).hollow(Hollow::TWINNED),
         {EnemyType::SERAPH, 2}, {EnemyType::HUSK, 4}},
    };
    o.goals = {WaveGoal{},
               WaveGoal::hold("HOLD THE CIRCLE ON THE INNER RING", {C.x, O + 2.f, C.z}, 3.f, 12.f).onMover(firstInner),
               WaveGoal{}};
    o.maxAlive = 12;
    o.damageScale = 1.35f;
    o.shift = ArenaShift::SOLAR;
    o.ambient = Ambient::EMBERS;
    o.theme = Theme{
        {0.03f,0.02f,0.04f}, {0.32f,0.18f,0.08f}, {0.015f,0.008f,0.004f},   // below: an abyss
        glm::normalize(vec3{0.f, -0.3f, -1.f}), {1.6f,1.1f,0.5f}, 0.08f, 0.f,
        {0.08f,0.05f,0.03f}, 0.5f,
        // The sun is below you: warm light from underneath, cool from the dark above
        glm::normalize(vec3{0.2f, 0.6f, 0.1f}), {1.1f,0.8f,0.45f},
        {0.12f,0.12f,0.18f}, {0.45f,0.28f,0.12f},
        {0.16f,0.09f,0.05f}, 0.009f };
    L.arenas.push_back(std::move(o));

    buildDescent(B);
}

// =============================================================================
// THE DESCENT — a cage that rides down a vast round shaft. It hangs at a
// different floor for each wave (bell galleries, the clamps, the furnace
// ring) and comes to rest in the Penitent's pit, 160 m below the Orrery.
//
//   Y  -80  the landing (the corridor from the Orrery's north arch)
//   Y -120  the bell galleries     arcades and great bells round the shaft
//   Y -160  the clamps             four brake clamps bite the cage's rim
//   Y -200  the furnace ring       catwalks round furnace mouths, red below
//   Y -240  the pit                the Penitent, chained to six anchors
// =============================================================================
inline void buildDescent(LevelBuilder& B) {
    using glm::vec3;
    LevelData& L = B.L;
    auto aabb = &LevelBuilder::aabb;
    auto wall = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { return B.wall(x0,y0,z0,x1,y1,z1,c); };
    auto neon = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { B.neon(x0,y0,z0,x1,y1,z1,c); };

    const vec3 C{0.f, 0.f, -840.f};
    const float TOP = -80.f, PIT = -240.f, RW = 34.f, RCAGE = 12.f, RGAL = 20.f;
    // The cage: four overlapping boxes, a 12-sided platform ~13 m across the radius
    struct Rect { float hx, hz; };
    const Rect CAGE[4] = {{12.f, 5.f}, {5.f, 12.f}, {10.5f, 8.f}, {8.f, 10.5f}};
    const float STOP[5] = {-80.f, -120.f, -160.f, -200.f, -240.f};
    vec3 iron{0.16f,0.15f,0.16f}, ironDark{0.09f,0.085f,0.09f}, bone{0.7f,0.66f,0.58f}, brass{0.62f,0.48f,0.26f},
         ember{1.4f,0.45f,0.12f}, amber{1.3f,0.8f,0.35f}, blood{1.2f,0.12f,0.08f}, pale{0.8f,0.85f,0.9f};
    B.mat = Mat::METAL;

    // ---- the corridor from the Orrery's north arch to the landing ----
    wall(-4, TOP - 1, -808, 4, TOP, -786, iron);
    wall(-5, TOP, -808, -4, TOP + 8, -786, ironDark); wall(4, TOP, -808, 5, TOP + 8, -786, ironDark);
    wall(-5, TOP + 8, -808, 5, TOP + 9, -786, ironDark);
    for (float z = -806.f; z < -787.f; z += 4.f) neon(-3.95f, TOP + 7.6f, z, 3.95f, TOP + 7.75f, z + 0.3f, amber * 0.5f);
    L.corridors.push_back(aabb(-6, TOP, -810, 6, TOP + 14, -784));

    Arena a;
    a.name = "THE DESCENT"; a.space = ReverbSpace::SHAFT;
    a.subtitle = "RIDE THE CAGE DOWN - SURVIVE 3 WAVES";
    a.bounds = aabb(C.x - 33, PIT, C.z - 33, C.x + 33, TOP + 14, C.z + 33);
    a.zone   = aabb(C.x - 34.5f, PIT, C.z - 34.5f, C.x + 34.5f, TOP + 16, C.z + 34.5f);
    a.playerStart = {0.f, TOP, -812.f};
    a.startYaw = -90.f;
    a.respawn = {C.x, TOP, C.z}; a.hasRespawn = true;
    a.voidY = TOP - 25.f;
    a.bossSpawn = {C.x, PIT, C.z - 24.f};

    // ---- the shaft wall: posts round R 35 (collision), a curved wall drawn
    // inside them, ribs of light every 10 m, a gap for the landing's door ----
    for (int k = 0; k < 72; ++k) {
        float ang = k * 6.2831853f / 72.f;
        vec3 p = C + vec3{std::cos(ang) * (RW + 1.2f), 0.f, std::sin(ang) * (RW + 1.2f)};
        bool door = std::fabs(p.x) < 5.5f && p.z > C.z;
        B.solid(p.x - 1.6f, PIT - 6.f, p.z - 1.6f, p.x + 1.6f, door ? TOP - 1.f : TOP + 16.f, p.z + 1.6f);
        if (door) B.solid(p.x - 1.6f, TOP + 8.f, p.z - 1.6f, p.x + 1.6f, TOP + 16.f, p.z + 1.6f);
    }
    B.kit().curve({C.x, 0.f, C.z}, RW + 0.6f, 0.f, 6.2831853f, PIT - 6.f, TOP + 16.f, 1.2f, ironDark, 72);
    for (float y = PIT + 5.f; y < TOP + 14.f; y += 10.f)
        B.kit(true).curve({C.x, 0.f, C.z}, RW - 0.05f, 0.f, 6.2831853f, y, y + 0.18f, 0.1f, amber * 0.35f, 72);
    for (int k = 0; k < 16; ++k) {                                      // ribs: arches climbing the wall
        float yaw = k * 6.2831853f / 16.f;
        vec3 p = C + vec3{std::cos(yaw) * (RW - 0.5f), 0.f, std::sin(yaw) * (RW - 0.5f)};
        B.kit().rod({p.x, PIT, p.z}, {p.x, TOP + 16.f, p.z}, 0.5f, iron, 6);
    }
    a.entryGate = B.doorway(true, -4, 4, -809, -807, TOP, 7.f, amber, false);

    // ---- an annulus floor (galleries, the pit) built as strips, like the
    // Orrery's terrace: drawn exactly where it holds you ----
    auto annulus = [&](float y, float rIn, float rOut, vec3 col, vec3 lip) {
        const float STRIP = 0.75f;
        auto band = [&](bool alongX, float side, float u0, float u1) {
            float un = (u0 < 0.f && u1 > 0.f) ? 0.f : std::min(std::fabs(u0), std::fabs(u1));
            float edge = std::sqrt(std::max(0.f, rIn * rIn - un * un));
            float from = std::max(edge, un), to = std::sqrt(std::max(0.f, rOut * rOut - un * un));
            if (from >= to) return;
            float a0 = side * from, a1 = side * to;
            if (alongX) wall(C.x + std::min(a0, a1), y - 1, C.z + u0, C.x + std::max(a0, a1), y, C.z + u1, col);
            else        wall(C.x + u0, y - 1, C.z + std::min(a0, a1), C.x + u1, y, C.z + std::max(a0, a1), col);
            if (edge > un + 1e-3f) {
                float e = side * edge;
                if (alongX) neon(C.x + e - 0.06f, y - 0.3f, C.z + u0, C.x + e + 0.06f, y + 0.03f, C.z + u1, lip);
                else        neon(C.x + u0, y - 0.3f, C.z + e - 0.06f, C.x + u1, y + 0.03f, C.z + e + 0.06f, lip);
            }
        };
        for (float u = -rOut; u < rOut - 1e-3f; u += STRIP) {
            float u1 = std::min(u + STRIP, rOut);
            for (float side : {-1.f, 1.f}) { band(true, side, u, u1); band(false, side, u, u1); }
        }
    };
    auto bridges = [&](float y, vec3 col, vec3 lip) {
        for (int k = 0; k < 4; ++k) {
            float cx = k == 0 ? 1.f : k == 1 ? -1.f : 0.f, cz = k == 2 ? 1.f : k == 3 ? -1.f : 0.f;
            float r0 = RCAGE + 0.3f, r1 = RGAL + 0.5f;   // RCAGE: the cage's reach along an axis
            if (cx != 0.f) wall(C.x + std::min(cx * r0, cx * r1), y - 0.8f, C.z - 1.5f, C.x + std::max(cx * r0, cx * r1), y, C.z + 1.5f, col);
            else           wall(C.x - 1.5f, y - 0.8f, C.z + std::min(cz * r0, cz * r1), C.x + 1.5f, y, C.z + std::max(cz * r0, cz * r1), col);
            vec3 tip = C + vec3{cx * r0, y, cz * r0};
            neon(tip.x - 1.5f, y - 0.05f, tip.z - 1.5f, tip.x + 1.5f, y + 0.04f, tip.z + 1.5f, lip);
        }
    };

    // ---- the landing: a south apron and a bridge onto the cage (no ring:
    // the cage is the only floor up here, so you're aboard when it drops) ----
    wall(-5, TOP - 1, -810, 5, TOP, C.z + RW - 0.5f, iron);
    wall(-1.5f, TOP - 0.8f, C.z + RCAGE + 0.3f, 1.5f, TOP, C.z + RW - 0.5f, iron);

    // ---- stop 1: the bell galleries ----
    annulus(STOP[1], RGAL, RW, iron, amber * 0.6f);
    bridges(STOP[1], iron, amber * 0.5f);
    for (int k = 0; k < 8; ++k) {
        float yaw = (k + 0.5f) * 0.7853982f;
        vec3 p = C + vec3{std::cos(yaw) * (RW - 1.5f), STOP[1], std::sin(yaw) * (RW - 1.5f)};
        B.kit().arch({p.x, STOP[1], p.z}, 9.f, 7.f, 0.8f, 1.2f, bone * 0.8f, -yaw + 1.5707963f, 12);   // arcades
        vec3 b = C + vec3{std::cos(yaw) * 27.f, 0.f, std::sin(yaw) * 27.f};
        B.kit().dome({b.x, STOP[1] + 5.2f, b.z}, 1.8f, brass, 4, 12);                                 // a bell
        B.kit().column({b.x, STOP[1] + 6.8f, b.z}, 0.12f, 3.f, ironDark, 6);
        B.kit(true).column({b.x, STOP[1] + 4.0f, b.z}, 0.35f, 0.5f, amber, 8);                       // its clapper glows
    }
    // ---- stop 2: the clamps ----
    annulus(STOP[2], RGAL, RW, ironDark, blood * 0.5f);
    bridges(STOP[2], ironDark, blood * 0.4f);
    std::vector<vec3> clamps;
    for (int k = 0; k < 4; ++k) {
        float yaw = k * 1.5707963f + 0.7853982f;                        // between the bridges
        vec3 p = C + vec3{std::cos(yaw) * 17.5f, STOP[2], std::sin(yaw) * 17.5f};
        wall(p.x - 2.f, STOP[2] - 1.f, p.z - 2.f, p.x + 2.f, STOP[2], p.z + 2.f, ironDark);   // the clamp's footing
        B.kit().box({p.x, STOP[2] + 4.5f, p.z}, {1.2f, 9.f, 3.2f}, brass, -yaw, 0.3f, 0.f);    // jaws over the rim
        clamps.push_back(p);
    }
    // ---- stop 3: the furnace ring ----
    annulus(STOP[3], RGAL, RW, iron, ember * 0.6f);
    bridges(STOP[3], iron, ember * 0.5f);
    for (int k = 0; k < 6; ++k) {
        float yaw = k * 1.0471976f;
        vec3 p = C + vec3{std::cos(yaw) * (RW - 0.3f), STOP[3], std::sin(yaw) * (RW - 0.3f)};
        B.kit().arch({p.x, STOP[3], p.z}, 6.f, 5.f, 0.9f, 1.4f, ironDark, -yaw + 1.5707963f, 10);   // furnace mouths
        B.kit(true).dome({p.x, STOP[3] + 1.2f, p.z}, 2.2f, ember, 3, 10);
    }
    B.kit(true).curve({C.x, 0.f, C.z}, RGAL + 0.4f, 0.f, 6.2831853f, STOP[3] + 1.0f, STOP[3] + 1.15f, 0.12f, ember, 48);   // rail light

    // ---- the pit: a round floor fitted round the cage's outline (strips along
    // X, each split where the cage is), so it docks flush; six anchors, rubble ----
    {
        const float STRIP = 0.5f;
        for (float z0 = -RW; z0 < RW - 1e-3f; z0 += STRIP) {
            float z1 = std::min(z0 + STRIP, RW);
            float zf = std::max(std::fabs(z0), std::fabs(z1));
            float xOut = std::sqrt(std::max(0.f, RW * RW - zf * zf));
            if (xOut < 0.3f) continue;
            float xIn = 0.f;
            for (const Rect& r : CAGE) if (z1 > -r.hz + 1e-3f && z0 < r.hz - 1e-3f) xIn = std::max(xIn, r.hx);
            if (xIn <= 0.f) { wall(C.x - xOut, PIT - 1, C.z + z0, C.x + xOut, PIT, C.z + z1, iron); continue; }
            xIn += 0.02f;
            if (xOut <= xIn) continue;
            wall(C.x - xOut, PIT - 1, C.z + z0, C.x - xIn, PIT, C.z + z1, iron);
            wall(C.x + xIn, PIT - 1, C.z + z0, C.x + xOut, PIT, C.z + z1, iron);
        }
    }
    for (int k = 0; k < 6; ++k) {
        static const float ANG[6] = {-170.f, -130.f, -105.f, -75.f, -50.f, -10.f};   // round the north half and the sides
        float yaw = glm::radians(ANG[k]);
        float h = 8.f + 4.f * (k % 3) / 2.f;                                          // 8, 10, 12 m up
        vec3 p = C + vec3{std::cos(yaw) * (RW - 1.0f), PIT + h, std::sin(yaw) * (RW - 1.0f)};
        int w = wall(p.x - 0.9f, p.y - 0.9f, p.z - 0.9f, p.x + 0.9f, p.y + 0.9f, p.z + 0.9f, ironDark);
        L.walls[w].hidden = true;                                                     // drawn live (glow, breaking)
        L.anchors.push_back({w, p, 400.f, true});
    }
    B.kit(true).curve({C.x, 0.f, C.z}, 15.5f, 0.f, 6.2831853f, PIT + 0.02f, PIT + 0.08f, 0.15f, blood, 48);   // a red ring round the dock
    for (int k = 0; k < 9; ++k) {                                                     // rubble round the edge
        float yaw = k * 0.698f + 0.2f;
        vec3 p = C + vec3{std::cos(yaw) * 30.f, PIT, std::sin(yaw) * 30.f};
        B.kit().rock({p.x, PIT, p.z}, 1.6f + 0.4f * (k % 3), 2.2f + 0.5f * (k % 2), iron, (uint32_t)k);
    }
    L.finishPos = {C.x, PIT, C.z + 20.f};

    // ---- the cage: four overlapping driven boxes, top at the stop's height ----
    vec3 cageCol{0.2f, 0.19f, 0.2f}, cageGlow{1.2f, 0.7f, 0.3f};
    vec3 drop{0.f, PIT - TOP, 0.f};
    auto cagePart = [&](float x0, float z0, float x1, float z1) {
        int m = B.mover({C.x + (x0 + x1) * 0.5f, TOP - 0.5f, C.z + (z0 + z1) * 0.5f}, {(x1 - x0) * 0.5f, 0.5f, (z1 - z0) * 0.5f},
                        Mover::Path::DRIVEN, {0.f, 0.f, 0.f}, drop, 1.f, 0.f, cageGlow);
        L.movers[m].color = cageCol;
        L.lift.movers.push_back(m);
    };
    for (const Rect& r : CAGE) cagePart(-r.hx, -r.hz, r.hx, r.hz);
    L.lift.stops.assign(STOP, STOP + 5);

    // ---- spawns per stop: galleries round the shaft + the cage ----
    for (int w = 0; w < 3; ++w) {
        float y = STOP[w + 1];
        std::vector<vec3> g, air;
        for (int k = 0; k < 8; ++k) {
            float yaw = k * 0.7853982f + 0.3927f;                        // between bridges and clamps
            g.push_back(C + vec3{std::cos(yaw) * 27.f, y, std::sin(yaw) * 27.f});
        }
        g.push_back(C + vec3{-6.f, y, -6.f}); g.push_back(C + vec3{6.f, y, 6.f});   // on the cage
        for (int k = 0; k < 4; ++k) {
            float yaw = k * 1.5707963f;
            air.push_back(C + vec3{std::cos(yaw) * 17.f, y + 10.f, std::sin(yaw) * 17.f});
        }
        a.waveGround.push_back(g);
        a.waveAir.push_back(air);
    }
    a.waveGround.push_back({a.bossSpawn});
    a.waveAir.push_back({C + vec3{0.f, PIT + 12.f, 0.f}});
    a.groundSpawns = a.waveGround[0];
    a.airSpawns = a.waveAir[0];

    a.waves = {
        {{EnemyType::HUSK, 4}, {EnemyType::RAPTOR, 3}, {EnemyType::SERAPH, 2},
         WaveEntry(EnemyType::SHIELDBEARER, 2).with({EnemyType::HUSK}), WaveEntry(EnemyType::RIPPER, 2).hollow(Hollow::ENRAGED)},
        {{EnemyType::ANCHOR, 1}, {EnemyType::CONDUCTOR, 2}, {EnemyType::SENTINEL, 3}, {EnemyType::MITE, 6},
         WaveEntry(EnemyType::BRUTE, 2).hollow(Hollow::HALOED)},
        {{EnemyType::ANCHOR, 2}, {EnemyType::JUGGERNAUT, 1},
         WaveEntry(EnemyType::SHIELDBEARER, 2).with({EnemyType::SENTINEL}).hollow(Hollow::TWINNED),
         {EnemyType::SERAPH, 2}, WaveEntry(EnemyType::RAPTOR, 3).hollow(Hollow::ENRAGED), {EnemyType::HUSK, 4}},
        {{EnemyType::SOVEREIGN, 1}},   // replaced by the PENITENT in Task 4
    };
    a.goals = {WaveGoal{}, WaveGoal::conduits("RELEASE THE CLAMPS", clamps), WaveGoal{}, WaveGoal{}};
    a.maxAlive = 12;
    a.damageScale = 1.4f;
    a.ambient = Ambient::ASH;
    a.theme = Theme{
        {0.02f,0.015f,0.02f}, {0.22f,0.08f,0.05f}, {0.01f,0.004f,0.003f},
        glm::normalize(vec3{0.f, -1.f, -0.2f}), {1.2f,0.75f,0.4f}, 0.06f, 0.f,
        {0.05f,0.03f,0.025f}, 0.45f,
        // The light comes from the furnaces below; cold grey from the mouth above
        glm::normalize(vec3{0.1f, 0.8f, 0.2f}), {1.1f,0.5f,0.25f},
        {0.1f,0.1f,0.13f}, {0.4f,0.12f,0.06f},
        {0.12f,0.05f,0.03f}, 0.01f };
    L.arenas.push_back(std::move(a));
}

// ACT II mode's level
inline LevelData buildAct2Level() {
    LevelData L;
    LevelBuilder B{L};
    buildAct2(B);
    return L;
}
