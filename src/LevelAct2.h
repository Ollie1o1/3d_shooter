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
    L.basins.push_back({aabb(-300, 0, -668, 300, 0, -1000), -140.f});   // the Orrery: its void bottoms out far below

    Arena a;
    a.name = "THE DROWNED NAVE";
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
    wall(-5, O - 2, -669, 5, -57, -668, slate);                    // under the passage's end
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
    o.name = "THE ORRERY";
    o.subtitle = "MIND THE SUN - SURVIVE 3 WAVES";
    o.bounds = aabb(-52, O, -784, 52, O + 28, -680);
    o.zone   = aabb(-53.5f, O, -785.5f, 53.5f, O + 30, -678.5f);
    o.playerStart = {0.f, O, -686.f};
    o.startYaw = -90.f;
    o.respawn = {0.f, O, -690.f}; o.hasRespawn = true;
    o.voidY = -105.f;
    o.sunPos = {C.x, -92.f, C.z};

    // ---- the terrace: an annulus R 30-52, collision as strips of boxes ----
    const float RIN = 30.f, ROUT = 53.f, STRIP = 1.5f;
    for (float z0 = -ROUT; z0 < ROUT - 1e-3f; z0 += STRIP) {
        float z1 = std::min(z0 + STRIP, ROUT);
        float zn = std::min(std::fabs(z0), std::fabs(z1)), zf = std::max(std::fabs(z0), std::fabs(z1));
        if (z0 < 0.f && z1 > 0.f) zn = 0.f;
        float xo = std::sqrt(std::max(0.f, ROUT * ROUT - zn * zn));
        if (zf >= RIN) wall(-xo, O - 1, C.z + z0, xo, O, C.z + z1, stone);
        else {
            float xi = std::sqrt(RIN * RIN - zf * zf);   // the pit's edge (the strip's far side: no floor over the void)
            B.solid(-xo, O - 1, C.z + z0, -xi, O, C.z + z1);
            B.solid(xi, O - 1, C.z + z0, xo, O, C.z + z1);
        }
    }
    B.kit().curve({C.x, 0.f, C.z}, 41.f, 0.f, 6.2831853f, O - 1.f, O, 22.f, stone, 48);   // its face
    B.kit(true).curve({C.x, 0.f, C.z}, 30.3f, 0.f, 6.2831853f, O - 0.2f, O + 0.05f, 0.5f, gold * 0.7f, 48);      // the pit's lit lip

    // ---- the outer wall (posts round R 53) with a gate on the south ----
    for (int k = 0; k < 96; ++k) {
        float ang = k * 6.2831853f / 96.f;
        vec3 p = C + vec3{std::cos(ang) * 54.f, 0.f, std::sin(ang) * 54.f};
        if (std::fabs(p.x) < 5.5f && p.z > C.z) continue;          // the gate
        B.solid(p.x - 1.8f, O, p.z - 1.8f, p.x + 1.8f, O + 14.f, p.z + 1.8f);
    }
    B.kit().curve({C.x, 0.f, C.z}, 54.f, 0.f, 6.2831853f, O, O + 14.f, 2.f, stoneDark, 64);
    B.kit(true).curve({C.x, 0.f, C.z}, 52.9f, 0.f, 6.2831853f, O + 13.f, O + 13.3f, 0.2f, gold * 0.8f, 64);
    wall(-6, O, -682, -4, O + 14, -678, stoneDark); wall(4, O, -682, 6, O + 14, -678, stoneDark);   // gate posts
    wall(-4, O + 7, -681, 4, O + 14, -679, stoneDark);
    o.entryGate = B.doorway(true, -4, 4, -681, -679, O, 7.f, gold, false);

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

    // ---- the finish: a beacon on the north terrace ----
    L.finishPos = {0.f, O, -778.f};

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
}

// ACT II mode's level
inline LevelData buildAct2Level() {
    LevelData L;
    LevelBuilder B{L};
    buildAct2(B);
    return L;
}
