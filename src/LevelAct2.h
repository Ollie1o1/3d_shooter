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
    L.basins.push_back({aabb(-300, 0, -440, 300, 0, -1000), F});

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
    L.floors.push_back({-300.f, -1000.f, 300.f, -440.f, F - 0.3f, {0.05f, 0.06f, 0.07f}});   // seen from wall tops
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
    B.kit(true).column({0.f, F, -607.f}, 3.4f, 46.f, light * 0.18f, 18);                          // the shaft of eclipse light
    L.gems.push_back({{0.f, F + 0.4f, -607.f}, light, 1.2f, true});

    // ---- the chancel and the drowned organ ----
    wall(-16, F, -652, 16, F + 3, -622, slate);                    // chancel (+3)
    wall(-16, F, -622, 16, F + 1.5f, -619, slate);                 // its step
    wall(-17, F, -652, -16, F + 26, -622, bone);
    wall( 16, F, -652,  17, F + 26, -622, bone);
    // back wall with the sealed way on
    B.wallX(-17, 17, -653, -652, F, F + 26, bone, {{-3.f, 3.f, F + 3.f, F + 8.f}});
    a.exitDoor = B.doorway(true, -3, 3, -653, -652, F + 3, 5.f, red, true);
    wall(-4, F, -662, -3, F + 9, -653, slate); wall(3, F, -662, 4, F + 9, -653, slate);
    wall(-4, F, -663, 4, F + 9, -662, slate);
    wall(-3, F, -662, 3, F + 3, -653, slate);                      // passage floor (+3)
    wall(-4, F + 9, -663, 4, F + 10, -653, slate);
    L.finishPos = {0.f, F + 3.f, -659.f};
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
    a.waves = {
        {{EnemyType::HUSK, 6}, {EnemyType::RIPPER, 5}, {EnemyType::SENTINEL, 3},
         WaveEntry(EnemyType::SHIELDBEARER, 2).with({EnemyType::HUSK})},
        {{EnemyType::RAPTOR, 4}, {EnemyType::BRUTE, 2}, {EnemyType::CONDUCTOR, 2}, {EnemyType::MITE, 6}},
        {{EnemyType::BRUTE, 3}, WaveEntry(EnemyType::SHIELDBEARER, 3).with({EnemyType::SENTINEL}),
         {EnemyType::CONDUCTOR, 2}, {EnemyType::JUGGERNAUT, 1}, {EnemyType::RIPPER, 4}},
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
}

// ACT II mode's level
inline LevelData buildAct2Level() {
    LevelData L;
    LevelBuilder B{L};
    buildAct2(B);
    return L;
}
