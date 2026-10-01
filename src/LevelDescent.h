#pragma once
// =============================================================================
// LevelDescent.h — FAST mode's map: THE DESCENT.
//
// A time-trial gauntlet that runs north and down, six sections from a ledge
// 60 m up to the ground. Every section's enemies are hand-placed and appear
// the moment the gate above them opens, so you see the next fight below you
// while you drop into it. Kill everything in a section and its far gate
// sinks; clear the last one and the FINISH beacon lights up. The clock runs
// from the countdown to the beacon, with a split at every section.
//
//   section            floor   z            the idea
//   0 THE LEDGE         60     0 .. -28     snipers on plinths, open lines of fire
//   1 THE FALL          50     -28 .. -62   rushers and cover
//   2 THE CHASM         40     -62 .. -104  ferries + grapple anchors over a void
//   3 THE STAIRS        30→22  -104 .. -140 three terraces stepping down
//   4 THE HALL          10     -140 .. -180 pillars, side galleries, a Brute
//   5 THE PIT            0     -180 .. -232 two waves, then the finish beacon
//
// Each section is an Arena (zone, bounds, waves with hand-placed `at` points)
// so WaveDirector runs it with fast = true. Sections are solid blocks of rock,
// so there's nowhere to fall except the chasm, whose void plane sends you
// back to the start of the section.
// =============================================================================
#include "Level.h"

inline LevelData buildDescent() {
    using glm::vec3;
    LevelData L;
    L.fast = true;
    LevelBuilder B{L};
    auto aabb = &LevelBuilder::aabb;

    // Sky darkens and turns hellish as you go down: lerp between these two
    const Theme TOP{
        {0.12f,0.30f,0.68f}, {0.78f,0.86f,0.96f}, {0.30f,0.30f,0.36f},
        glm::normalize(vec3{0.4f, 0.45f, -1.f}), {1.6f,1.5f,1.25f}, 0.05f, 0.f,
        {0.40f,0.46f,0.60f}, 0.f,
        glm::normalize(vec3{-0.3f,-0.75f,0.6f}), {1.15f,1.08f,0.98f},
        {0.45f,0.52f,0.68f}, {0.20f,0.19f,0.20f},
        {0.66f,0.74f,0.86f}, 0.0045f };
    const Theme BOTTOM{
        {0.05f,0.01f,0.03f}, {0.45f,0.08f,0.05f}, {0.10f,0.02f,0.02f},
        glm::normalize(vec3{0.f, 0.06f, -1.f}), {1.8f,0.35f,0.12f}, 0.16f, 1.f,
        {0.10f,0.02f,0.03f}, 0.4f,
        glm::normalize(vec3{0.2f,-0.9f,0.3f}), {0.8f,0.45f,0.35f},
        {0.18f,0.10f,0.14f}, {0.45f,0.12f,0.05f},
        {0.22f,0.05f,0.04f}, 0.012f };

    // The world floor far below the sections
    L.floors.push_back({-100.f, -320.f, 100.f, 100.f, -0.3f, {0.10f,0.06f,0.07f}});

    struct Spec {
        const char* name; const char* sub;
        float z0, z1, hw, floor;
        vec3 rock, trim;
    };
    const Spec specs[6] = {
        {"THE LEDGE",  "SNIPERS ON THE PLINTHS",       0.f,   -28.f,  12.f, 60.f, {0.55f,0.57f,0.64f}, {0.3f,0.9f,1.0f}},
        {"THE FALL",   "RUSHERS INCOMING",            -28.f,  -62.f,  14.f, 50.f, {0.52f,0.50f,0.56f}, {0.4f,0.95f,0.9f}},
        {"THE CHASM",  "RIDE THE FERRIES OR GRAPPLE", -62.f,  -104.f, 16.f, 40.f, {0.50f,0.44f,0.46f}, {1.0f,0.75f,0.3f}},
        {"THE STAIRS", "THREE TERRACES DOWN",         -104.f, -140.f, 14.f, 22.f, {0.46f,0.36f,0.36f}, {1.0f,0.55f,0.25f}},
        {"THE HALL",   "MIND THE GALLERIES",          -140.f, -180.f, 18.f, 10.f, {0.40f,0.28f,0.28f}, {1.0f,0.35f,0.2f}},
        {"THE PIT",    "LAST STAND - THEN THE BEACON", -180.f, -232.f, 22.f, 0.f,  {0.32f,0.20f,0.20f}, {1.0f,0.2f,0.15f}},
    };

    for (int i = 0; i < 6; ++i) {
        const Spec& s = specs[i];
        float f = s.floor, hw = s.hw;
        Arena a;
        a.name = s.name;
        a.subtitle = s.sub;
        a.bounds = aabb(-hw, f, s.z1, hw, f + 18.f, s.z0);
        a.zone   = aabb(-hw + 0.4f, f, s.z1, hw - 0.4f, f + 24.f, s.z0);    // ceiling 24 m above the section
        a.playerStart = {0.f, (i == 3 ? 30.f : f), s.z0 - 3.f};
        a.damageScale = 0.8f + 0.06f * i;
        a.maxAlive = 99;
        a.voidY = f - 14.f;
        a.ambient = i < 2 ? Ambient::WIND : (i < 4 ? Ambient::ASH : Ambient::EMBERS);
        a.theme = lerpTheme(TOP, BOTTOM, i / 5.f);

        // The rock the section stands on (except the chasm and stairs, built below)
        if (i != 2 && i != 3 && f > 0.f) B.wall(-hw, 0, s.z1, hw, f, s.z0, s.rock * 0.8f);
        if (f == 0.f) L.floors.push_back({-hw, s.z1, hw, s.z0, 0.f, s.rock});
        // Side walls with a band of light, tall enough to feel like a canyon
        for (int sd : {-1, 1}) {
            B.wall(sd * hw, 0, s.z1, sd * (hw + 1.f), f + 7.f, s.z0, s.rock);
            B.neon(sd * (hw - 0.02f), f + 6.2f, s.z1, sd * (hw - 0.12f), f + 6.45f, s.z0, s.trim);
        }
        // Exit gate across the far edge (none after the last section: there's a back wall)
        if (i < 5) {
            a.exitDoor = B.door(-hw, s.z1 + 0.9f, hw, s.z1 + 0.1f, 6.f, {0.18f,0.17f,0.22f}, false, f);
            B.neon(-hw, f, s.z1 + 0.95f, hw, f + 0.12f, s.z1 + 1.05f, s.trim);
        } else {
            B.wall(-hw - 1.f, 0, s.z1 - 1.f, hw + 1.f, 9.f, s.z1, s.rock);
        }
        L.arenas.push_back(std::move(a));

        // Blend the lighting across each boundary (corridor i joins section i and i+1)
        if (i < 5) L.corridors.push_back(aabb(-hw, 0, s.z1 - 4.f, hw, 100, s.z1 + 4.f));
    }
    // Back wall behind the start
    B.wall(-13, 0, 0, 13, 66, 1, specs[0].rock);

    using W = WaveEntry;
    auto& A = L.arenas;

    // ---- 0 THE LEDGE (60) ----------------------------------------------------
    {
        vec3 rock = specs[0].rock, trim = specs[0].trim;
        for (int s : {-1, 1}) {
            B.wall(s * 7.f, 60, -27, s * 11.f, 63, -22, rock * 0.9f);          // sniper plinths
            B.neon(s * 7.f, 62.8f, -22.12f, s * 11.f, 63.f, -21.98f, trim);
        }
        B.wall(-5, 60, -14, -2, 61.3f, -12, rock * 0.7f);
        B.wall( 2, 60, -18,  5, 61.3f, -16, rock * 0.7f);
        A[0].waves = {{
            W{EnemyType::HUSK, 0, {{-6,60,-19},{6,60,-21},{0,60,-24}}},
            W{EnemyType::SENTINEL, 0, {{-9,63.05f,-24.5f},{9,63.05f,-24.5f}}},
        }};
    }
    // ---- 1 THE FALL (50) -----------------------------------------------------
    {
        vec3 rock = specs[1].rock;
        B.wall(-9, 50, -40, -5, 51.4f, -38, rock * 0.7f);
        B.wall( 5, 50, -46,  9, 51.4f, -44, rock * 0.7f);
        B.wall(-2, 50, -52,  2, 53.f,  -50, rock * 0.8f);
        B.wall(-12, 50, -58, -8, 51.4f, -56, rock * 0.7f);
        A[1].waves = {{
            W{EnemyType::RIPPER, 0, {{-8,50,-48},{8,50,-50},{0,50,-56},{-4,50,-58}}},
            W{EnemyType::HUSK, 0, {{-10,50,-54},{10,50,-58},{10,50,-40}}},
            W{EnemyType::SENTINEL, 0, {{0,53.05f,-51}}},
        }};
    }
    // ---- 2 THE CHASM (40) ----------------------------------------------------
    {
        vec3 rock = specs[2].rock, trim = specs[2].trim;
        B.wall(-16, 0, -72, 16, 40, -62, rock * 0.8f);       // near shelf
        B.wall(-16, 0, -104, 16, 40, -90, rock * 0.8f);      // far shelf
        B.neon(-16, 39.7f, -72.12f, 16, 40.f, -71.98f, trim);
        B.neon(-16, 39.7f, -90.02f, 16, 40.f, -89.88f, trim);
        // Ferries shuttle across; anchors hang over the void for the grapple
        B.mover({-6.f, 39.75f, -74.5f}, {2.2f, 0.25f, 2.2f}, Mover::Path::PINGPONG,
                {0, 0, 0}, {0, 0, -13.f}, 6.f, 0.f, trim);
        B.mover({ 6.f, 39.75f, -74.5f}, {2.2f, 0.25f, 2.2f}, Mover::Path::PINGPONG,
                {0, 0, 0}, {0, 0, -13.f}, 6.f, 0.5f, trim);
        B.mover({ 0.f, 45.5f, -81.f}, {1.2f, 1.2f, 1.2f}, Mover::Path::PINGPONG,
                {-9.f, 0, 0}, {9.f, 0, 0}, 5.f, 0.25f, {1.f, 0.4f, 0.9f});
        for (float x : {-11.f, 11.f})
            B.wall(x - 1, 47, -81, x + 1, 49, -79, rock);    // static anchors
        B.wall(-6, 40, -98, -2, 41.4f, -96, rock * 0.7f);
        B.wall( 3, 40, -101, 7, 41.4f, -99, rock * 0.7f);
        A[2].waves = {{
            W{EnemyType::HUSK, 0, {{-8,40,-95},{8,40,-96},{0,40,-102}}},
            W{EnemyType::SENTINEL, 0, {{-13,40,-100},{13,40,-100}}},
            W{EnemyType::RAPTOR, 0, {{-6,48,-84},{6,48,-78}}},
        }};
        A[2].voidY = 30.f;
    }
    // ---- 3 THE STAIRS (30 → 26 → 22) -----------------------------------------
    {
        vec3 rock = specs[3].rock, trim = specs[3].trim;
        B.wall(-14, 0, -114, 14, 30, -104, rock * 0.8f);
        B.wall(-14, 0, -124, 14, 26, -114, rock * 0.75f);
        B.wall(-14, 0, -140, 14, 22, -124, rock * 0.7f);
        B.neon(-14, 29.7f, -114.12f, 14, 30.f, -113.98f, trim);
        B.neon(-14, 25.7f, -124.12f, 14, 26.f, -123.98f, trim);
        B.wall(-3, 22, -134, 3, 23.4f, -132, rock * 0.6f);
        A[3].waves = {{
            W{EnemyType::HUSK, 0, {{-8,26.05f,-119},{8,26.05f,-120},{-10,22,-136},{10,22,-137}}},
            W{EnemyType::MITE, 0, {{-4,26.05f,-122},{4,26.05f,-122},{-6,22,-130},{6,22,-130}}},
            W{EnemyType::SENTINEL, 0, {{0,22,-138}}},
            W{EnemyType::BRUTE, 0, {{0,22,-128}}},
        }};
    }
    // ---- 4 THE HALL (10) -----------------------------------------------------
    {
        vec3 rock = specs[4].rock, trim = specs[4].trim;
        for (int s : {-1, 1}) {
            B.wall(s * 13.f, 10, -175, s * 18.f, 15, -145, rock * 0.85f);   // side galleries
            B.neon(s * 12.88f, 14.7f, -175, s * 12.98f, 15.f, -145, trim);
            L.pads.push_back({{s * 10.5f, 10.f, -160.f}, {1.2f, 1.2f}, {s * 4.f, 17.f, 0.f}});
            for (float z : {-150.f, -168.f}) {
                B.wall(s * 6.f, 10, z, s * 8.f, 24, z + 2.f, rock);          // pillars
                B.ring(std::min(s * 6.f, s * 8.f), z, std::max(s * 6.f, s * 8.f), z + 2.f, 13.f, 13.3f, trim);
            }
        }
        B.mover({0.f, 13.75f, -172.f}, {2.f, 0.25f, 2.f}, Mover::Path::PINGPONG,
                {-9.f, 0, 0}, {9.f, 0, 0}, 7.f, 0.f, trim);
        A[4].waves = {{
            W{EnemyType::BRUTE, 0, {{0,10,-172}}},
            W{EnemyType::HUSK, 0, {{-4,10,-165},{4,10,-165},{-10,10,-176},{10,10,-176}}},
            W{EnemyType::SENTINEL, 0, {{-15.5f,15.05f,-170},{15.5f,15.05f,-150}}},
            W{EnemyType::RIPPER, 0, {{-3,10,-152},{3,10,-150},{0,10,-158}}},
            W{EnemyType::RAPTOR, 0, {{-8,20,-160},{8,20,-170}}},
        }};
    }
    // ---- 5 THE PIT (0) -------------------------------------------------------
    {
        vec3 rock = specs[5].rock, trim = specs[5].trim;
        B.wall(-3, 0, -207, 3, 5, -201, rock * 0.8f);                      // monolith
        B.ring(-3, -207, 3, -201, 2.f, 2.3f, trim);
        for (int s : {-1, 1}) {
            B.wall(s * 10.f, 0, -194, s * 14.f, 1.4f, -192, rock * 0.7f);
            B.wall(s * 12.f, 0, -218, s * 16.f, 1.4f, -216, rock * 0.7f);
        }
        L.hazards.push_back({aabb(-22, 0, -186, -16, 0.06f, -184), 30.f});   // lava cracks
        L.hazards.push_back({aabb( 16, 0, -214,  22, 0.06f, -212), 30.f});
        for (auto& hz : L.hazards) B.neon(hz.box.min.x, 0, hz.box.min.z, hz.box.max.x, 0.06f, hz.box.max.z, {1.f,0.24f,0.02f});
        A[5].waves = {
            {
                W{EnemyType::HUSK, 0, {{-12,0,-205},{12,0,-205},{0,0,-215}}},
                W{EnemyType::MITE, 0, {{-6,0,-212},{6,0,-212},{-10,0,-222},{10,0,-222}}},
                W{EnemyType::RAPTOR, 0, {{-10,9,-200},{10,9,-210}}},
            },
            {
                W{EnemyType::BRUTE, 0, {{-10,0,-222},{10,0,-222}}},
                W{EnemyType::SENTINEL, 0, {{-18,0,-226},{18,0,-226}}},
                W{EnemyType::RAPTOR, 0, {{0,10,-215}}},
            },
        };
        L.finishPos = {0.f, 0.f, -226.f};
        L.gems.push_back({{0.f, 5.f, -226.f}, {1.8f, 0.5f, 0.2f}, 1.3f, true});
    }

    // Spires of rock rising out of the dark either side of the descent
    for (int k = 0; k < 18; ++k) {
        float side = (k % 2) ? 1.f : -1.f;
        float z = 10.f - k * 13.f;
        float x = side * (30.f + (k * 37 % 17));
        float h = 30.f + (k * 53 % 40);
        float w = 4.f + (k * 7 % 5);
        B.prop(x - w, 0, z - w, x + w, h, z + w, {0.18f,0.14f,0.16f});
        B.neon(x - w - 0.05f, h * 0.7f, z - w - 0.05f, x + w + 0.05f, h * 0.7f + 0.3f, z + w + 0.05f,
               glm::mix(vec3{0.3f,0.9f,1.f}, vec3{1.f,0.25f,0.1f}, std::min(1.f, k / 14.f)) * 0.6f);
    }

    // Par times (seconds): S, A, B, C
    L.parTimes[0] = 150.f; L.parTimes[1] = 210.f; L.parTimes[2] = 300.f; L.parTimes[3] = 420.f;
    return L;
}
