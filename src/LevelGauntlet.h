#pragma once
// =============================================================================
// LevelGauntlet.h — FAST mode's map: THE GAUNTLET.
//
// A time trial along one long route. Each of its six levels is a long, wide
// channel in two parts: a breather (health to pick up, nothing shooting at
// you) and then a fight that starts when you reach the section's trigger.
// Clear the fight and the gate at the far end opens. The route turns and
// climbs, so every level asks something different of your movement:
//
//          N (-Z)                          ┌──────┐ 6 THE TOWER  courtyard fight,
//   3 THE SPAN ┐  islands, bridges and     │  ▲   │   then climb to the beacon
//   (y 23)     │  ferries over a void ─┐   └──┬───┘
//              │                 4 THE WELL ──┴── 5 THE PUMPWORKS ────────┘
//   2 THE ASCENT ◀─ (west, up 20 m)   drop down a tower    east: tunnel → hall
//              └──── 1 THE CANAL (north from the start)
//
//   level            heading   floor        idea
//   1 THE CANAL      north     0 / 3        sunken lane, walkways, bridges
//   2 THE ASCENT     west      3 → 23       five terraces: pads, steps, grapple
//   3 THE SPAN       north     23 (void)    islands, a beam, ferries, anchors
//   4 THE WELL       east      23 → 0       a tower you fall through, floor by floor
//   5 THE PUMPWORKS  east      0            roofed tunnel opening into a hall
//   6 THE TOWER      north     0 → 34       courtyard, then lifts to the beacon
//
// Levels are Arenas run by WaveDirector with fast = true; each has a trigger,
// hand-placed enemies (WaveEntry::at) and its own theme, crossfaded at the
// gates by LevelData::blends.
// =============================================================================
#include "Level.h"

inline LevelData buildGauntlet() {
    using glm::vec3;
    LevelData L;
    L.fast = true;
    LevelBuilder B{L};
    auto aabb = &LevelBuilder::aabb;
    auto wall = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { return B.wall(x0,y0,z0,x1,y1,z1,c); };
    auto neon = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { B.neon(x0,y0,z0,x1,y1,z1,c); };
    auto prop = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { B.prop(x0,y0,z0,x1,y1,z1,c); };
    auto pad  = [&](vec3 at, vec3 launch) { L.pads.push_back({at, {1.3f, 1.3f}, launch}); };
    auto place = [&](vec3 at, int kind) { L.placedPickups.push_back({at, kind}); };
    using W = WaveEntry;

    // Far ground below everything
    L.floors.push_back({-160.f, -400.f, 164.f, 200.f, -0.3f, {0.10f,0.09f,0.11f}});

    auto theme = [](vec3 zen, vec3 hor, vec3 gnd, vec3 sunDir, vec3 sunCol, float sunSize, float stripes,
                    vec3 mtn, float stars, vec3 lightDir, vec3 lightCol, vec3 skyA, vec3 gndA, vec3 fog, float fogD) {
        return Theme{zen, hor, gnd, glm::normalize(sunDir), sunCol, sunSize, stripes, mtn, stars,
                     glm::normalize(lightDir), lightCol, skyA, gndA, fog, fogD};
    };

    auto section = [&](const char* name, const char* sub, AABB zone, AABB bounds, vec3 start, AABB trigger,
                       Ambient amb, Theme th) -> Arena& {
        Arena a;
        a.name = name; a.subtitle = sub;
        a.zone = zone; a.bounds = bounds; a.playerStart = start;
        a.trigger = trigger; a.hasTrigger = true;
        a.maxAlive = 99; a.ambient = amb; a.theme = th;
        a.damageScale = 0.95f + 0.06f * (float)L.arenas.size();   // harder the further in
        L.arenas.push_back(std::move(a));
        return L.arenas.back();
    };

    // =========================================================================
    // 1 THE CANAL — north. A sunken lane between two raised walkways, bridges
    // across it. Start on a raised plaza, drop in, push north, exit west.
    // =========================================================================
    {
        vec3 conc{0.52f,0.54f,0.58f}, dark{0.32f,0.34f,0.39f}, crate{0.45f,0.33f,0.22f}, cyan{0.2f,0.9f,1.f};
        Arena& a = section("THE CANAL", "PUSH UP THE CANAL",
                           aabb(-14.6f, 0, -121.6f, 14.6f, 22, 1.6f), aabb(-15, 0, -122, 15, 16, 2),
                           {0.f, 3.f, -4.f}, aabb(-15, -1, -122, 15, 40, -34), Ambient::DUST,
                           theme({0.10f,0.20f,0.36f}, {0.95f,0.62f,0.42f}, {0.2f,0.18f,0.2f}, {0.3f,0.2f,-1.f}, {1.6f,0.9f,0.5f}, 0.09f, 0.f,
                                 {0.22f,0.2f,0.28f}, 0.f, {-0.3f,-0.6f,0.75f}, {1.1f,0.85f,0.65f},
                                 {0.38f,0.36f,0.46f}, {0.18f,0.15f,0.14f}, {0.62f,0.48f,0.46f}, 0.006f));
        // Perimeter: back, east, north, and the west wall with the exit gate on the walkway
        wall(-16,0,2, 16,9,3, conc);
        wall(15,0,-123, 16,9,2, conc);
        wall(-16,0,-123, 16,9,-122, conc);
        wall(-16,0,-106, -15,9,2, conc);
        wall(-16,0,-125, -15,9,-118, conc);
        wall(-16,0,-118, -15,3,-106, conc);
        a.exitDoor = B.door(-15.9f, -118, -15.1f, -106, 6.f, {0.2f,0.22f,0.26f}, false, 3.f);
        // Start plaza, then the canal with its walkways
        wall(-15,0,-30, 15,3,2, conc);
        wall(-15,0,-122, -9,3,-30, dark);
        wall(9,0,-122, 15,3,-30, dark);
        L.floors.push_back({-9.f, -122.f, 9.f, -30.f, 0.f, {0.18f,0.34f,0.38f}});
        neon(-9.12f,2.75f,-122, -9.0f,2.95f,-30, cyan);
        neon(9.0f,2.75f,-122, 9.12f,2.95f,-30, cyan);
        neon(-15,2.75f,-30.12f, 15,2.95f,-30.0f, cyan);
        // Bridges you can stand on or run under
        for (float z : {-55.f, -90.f}) {
            wall(-9,2.6f,z - 2, 9,3,z + 2, conc);
            neon(-9,2.45f,z - 2.1f, 9,2.6f,z + 2.1f, cyan * 0.6f);
        }
        // Crates to climb out of the canal, pads that throw you onto a walkway
        wall(-9,0,-45, -7,1.5f,-42, crate);  wall(7,0,-75, 9,1.5f,-72, crate);
        wall(-9,0,-104, -7,1.5f,-101, crate); wall(7,0,-112, 9,1.5f,-109, crate);
        pad({-5.f, 0.f, -66.f}, {-7.f, 14.f, 0.f});
        pad({ 5.f, 0.f, -98.f}, { 7.f, 14.f, 0.f});
        // Cover in the lane and on the walkways; pillars to grapple
        wall(-3,0,-64, 3,1.2f,-62, dark); wall(-6,0,-98, -2,1.2f,-96, dark); wall(2,0,-84, 6,1.2f,-82, dark);
        wall(-14,3,-80, -10,4.2f,-79, dark); wall(10,3,-66, 14,4.2f,-65, dark);
        for (float sx : {-12.f, 12.f}) for (float z : {-70.f, -100.f}) {
            wall(sx - 1,3,z - 1, sx + 1,11,z + 1, conc);
            B.ring(sx - 1, z - 1, sx + 1, z + 1, 9.6f, 9.9f, cyan);
        }
        a.waves = {{
            W{EnemyType::HUSK, 0, {{-12,3,-62},{12,3,-84},{12,3,-112},{0,0,-118}}},
            W{EnemyType::SENTINEL, 0, {{-5,3,-90},{5,3,-55}}},
            W{EnemyType::RIPPER, 0, {{0,0,-74},{-4,0,-80},{4,0,-104}}},
            W{EnemyType::RAPTOR, 0, {{-5,11,-80},{5,11,-100}}},
        }};
    }

    // =========================================================================
    // 2 THE ASCENT — west, climbing. Five 4 m terraces from y 3 to 23. Each
    // riser has a jump pad and a step block; gunners hold the terraces above.
    // =========================================================================
    {
        vec3 sand{0.66f,0.52f,0.38f}, sandD{0.48f,0.36f,0.27f}, gold{1.f,0.7f,0.3f};
        Arena& a = section("THE ASCENT", "CLIMB THE TERRACES",
                           aabb(-135.6f, 0, -123.6f, -14.f, 42, -100.4f), aabb(-136, 3, -124, -16, 34, -100),
                           {-24.f, 3.f, -112.f}, aabb(-136, -1, -124, -37, 60, -100), Ambient::DUST,
                           theme({0.14f,0.22f,0.42f}, {1.0f,0.72f,0.40f}, {0.25f,0.2f,0.18f}, {-1.f,0.25f,0.1f}, {1.8f,1.1f,0.5f}, 0.1f, 0.f,
                                 {0.35f,0.26f,0.25f}, 0.f, {0.8f,-0.5f,-0.2f}, {1.2f,0.9f,0.6f},
                                 {0.42f,0.38f,0.44f}, {0.22f,0.16f,0.12f}, {0.75f,0.58f,0.45f}, 0.005f));
        // Walls: north (exit gate at the top), south, west end, and closing the east end above the canal wall
        wall(-114,0,-125, -15,30,-124, sand);
        wall(-137,0,-125, -134,30,-124, sand);
        wall(-134,0,-125, -114,23,-124, sand);
        wall(-134,29,-125, -114,30,-124, sand);
        a.exitDoor = B.door(-134, -124.9f, -114, -124.1f, 6.f, {0.25f,0.2f,0.18f}, false, 23.f);
        wall(-137,0,-100, -15,30,-99, sand);
        wall(-137,0,-125, -136,30,-99, sand);
        wall(-16,9,-124, -15,30,-100, sand);
        // Landing and terraces
        struct T { float x0, x1, top; };
        const T terr[] = {{-36,-16,3}, {-54,-36,7}, {-72,-54,11}, {-90,-72,15}, {-108,-90,19}, {-136,-108,23}};
        for (int i = 0; i < 6; ++i) {
            wall(terr[i].x0, 0, -124, terr[i].x1, terr[i].top, -100, i % 2 ? sandD : sand);
            neon(terr[i].x1 - 0.12f, terr[i].top - 0.3f, -124, terr[i].x1 + 0.02f, terr[i].top - 0.05f, -100, gold);
        }
        for (int i = 0; i < 5; ++i) {
            float xb = terr[i + 1].x1, lo = terr[i].top;      // riser between terrace i and i+1
            pad({xb + 3.f, lo, -106.f}, {-6.f, 15.f, 0.f});
            wall(xb, lo, -122, xb + 2.f, lo + 2.f, -118, sandD);   // step block
        }
        // Cover
        wall(-48,7,-104, -46,8.3f,-101, sandD); wall(-66,11,-118, -63,12.3f,-116, sandD);
        wall(-86,15,-104, -82,17.5f,-102, sandD); wall(-104,19,-112, -101,20.3f,-109, sandD);
        wall(-126,23,-108, -122,24.3f,-106, sandD); wall(-118,23,-120, -116,25,-117, sandD);
        // Tall pylons along the walls to grapple
        for (float x : {-60.f, -96.f}) {
            wall(x - 1, 0, -123.9f, x + 1, 34, -122, sandD);
            neon(x - 1.05f, 31, -123.9f, x + 1.05f, 31.4f, -121.95f, gold);
        }
        place({-28.f, 3.f, -104.f}, 1); place({-30.f, 3.f, -120.f}, 0); place({-20.f, 3.f, -120.f}, 0);
        a.waves = {{
            W{EnemyType::MITE, 0, {{-44,7,-108},{-46,7,-116},{-50,7,-112}}},
            W{EnemyType::HUSK, 0, {{-62,11,-104},{-64,11,-121},{-100,19,-104},{-128,23,-104}}},
            W{EnemyType::SENTINEL, 0, {{-84,15,-112},{-102,19,-120},{-120,23,-112}}},
            W{EnemyType::BRUTE, 0, {{-78,15,-108}}},
        }};
    }

    // =========================================================================
    // 3 THE SPAN — north at y 23 over a void. Landing, twin bridges, an island,
    // a gap (ferries, a beam, or grapple the anchors), a bigger island with a
    // sniper perch, a sweeper, the far landing. Fall and you're back at the start.
    // =========================================================================
    {
        vec3 rock{0.46f,0.48f,0.56f}, rockD{0.30f,0.32f,0.40f}, ice{0.4f,0.9f,1.f};
        Arena& a = section("THE SPAN", "MIND THE GAPS",
                           aabb(-143.6f, 0, -249.6f, -104.8f, 44, -122.8f), aabb(-144, 23, -250, -104, 38, -124),
                           {-124.f, 23.f, -129.f}, aabb(-144, -1, -250, -104, 70, -141), Ambient::WIND,
                           theme({0.12f,0.32f,0.70f}, {0.75f,0.86f,0.97f}, {0.30f,0.32f,0.40f}, {0.4f,0.5f,-1.f}, {1.6f,1.5f,1.3f}, 0.05f, 0.f,
                                 {0.42f,0.48f,0.62f}, 0.f, {-0.3f,-0.8f,0.5f}, {1.15f,1.1f,1.f},
                                 {0.46f,0.52f,0.68f}, {0.2f,0.2f,0.24f}, {0.68f,0.76f,0.88f}, 0.004f));
        a.voidY = 12.f;
        wall(-136,0,-138, -112,23,-125, rock);                       // entry landing (a cliff)
        wall(-134,22.4f,-160, -130,23,-138, rockD);                   // twin bridges
        wall(-118,22.4f,-160, -114,23,-138, rockD);
        wall(-138,18,-176, -110,23,-160, rock);                       // island 1
        wall(-128,23,-170, -120,24.3f,-168, rockD);
        wall(-136,23,-164, -133,28,-161, rockD);
        wall(-131,22.5f,-190, -130,23,-176, rockD);                   // the beam
        B.mover({-122.f, 22.75f, -178.5f}, {2.f, 0.25f, 2.f}, Mover::Path::PINGPONG, {0,0,0}, {0,0,-9.f}, 5.f, 0.f, ice);
        B.mover({-115.f, 22.75f, -178.5f}, {2.f, 0.25f, 2.f}, Mover::Path::PINGPONG, {0,0,0}, {0,0,-9.f}, 5.f, 0.5f, ice);
        wall(-112,29,-184, -110,31,-182, rockD);                      // grapple anchors
        wall(-139,29,-184, -137,31,-182, rockD);
        wall(-142,18,-212, -106,23,-190, rock);                       // island 2
        wall(-128,23,-206, -120,26,-198, rockD);                      // sniper perch
        wall(-120,23,-202, -118,24.5f,-200, rockD);                   // step up to it
        wall(-140,23,-196, -136,24.3f,-194, rockD);
        wall(-112,23,-208, -108,24.3f,-206, rockD);
        B.mover({-124.f, 22.75f, -218.f}, {3.f, 0.25f, 3.f}, Mover::Path::PINGPONG, {-10.f,0,0}, {10.f,0,0}, 7.f, 0.f, ice);
        wall(-110,22.4f,-224, -106,23,-212, rockD);                   // narrow east bridge
        wall(-142,0,-250, -106,23,-224, rock);                        // far landing (cliff)
        // The landing's edge glows; pillars hold the islands up (visual only)
        neon(-136,22.7f,-138.12f, -112,23,-138, ice);
        for (float x : {-134.f, -114.f}) { prop(x - 1.5f, 0, -170, x + 1.5f, 18, -167, rockD); prop(x - 2, 0, -203, x + 2, 18, -199, rockD); }
        // Exit gate in the east wall of the tower beyond
        a.exitDoor = B.door(-105.9f, -248, -105.1f, -238, 6.f, {0.2f,0.22f,0.28f}, false, 23.f);
        place({-124.f, 23.f, -132.f}, 1); place({-130.f, 23.f, -134.f}, 0);
        a.waves = {{
            W{EnemyType::HUSK, 0, {{-130,23,-172},{-114,23,-166},{-136,23,-200},{-112,23,-196}}},
            W{EnemyType::RIPPER, 0, {{-124,23,-164},{-116,23,-172}}},
            W{EnemyType::SENTINEL, 0, {{-124,26,-202},{-136,23,-244}}},
            W{EnemyType::BRUTE, 0, {{-124,23,-194}}},
            W{EnemyType::RAPTOR, 0, {{-124,31,-180},{-118,31,-215},{-130,31,-232}}},
        }};
    }

    // =========================================================================
    // 4 THE WELL — a tower you enter at the top and fall through: a balcony at
    // 23, floors at 16 and 9 each with a hole in a different corner, then the
    // ground and the exit east. Gunners on every floor.
    // =========================================================================
    {
        vec3 brick{0.40f,0.30f,0.30f}, brickD{0.26f,0.20f,0.21f}, steel{0.30f,0.32f,0.36f}, red{1.f,0.35f,0.25f};
        Arena& a = section("THE WELL", "DROP THROUGH THE FLOORS",
                           aabb(-106.2f, 0, -259.6f, -62.6f, 34, -220.4f), aabb(-105, 0, -260, -64, 30, -220),
                           {-101.f, 23.f, -243.f}, aabb(-97, -1, -260, -64, 60, -220), Ambient::MOTES,
                           theme({0.05f,0.03f,0.06f}, {0.30f,0.12f,0.12f}, {0.06f,0.03f,0.04f}, {0.f,0.4f,-1.f}, {0.f,0.f,0.f}, 0.01f, 0.f,
                                 {0.08f,0.04f,0.05f}, 0.f, {0.2f,-1.f,0.2f}, {0.6f,0.45f,0.4f},
                                 {0.18f,0.14f,0.18f}, {0.30f,0.12f,0.08f}, {0.12f,0.05f,0.05f}, 0.012f));
        // Shell: west wall (with the doorway at the top), south, north, east (exit at the bottom), roof
        wall(-106,0,-261, -105,34,-248, brick);
        wall(-106,0,-238, -105,34,-219, brick);
        wall(-106,0,-248, -105,23,-238, brick);
        wall(-106,29,-248, -105,34,-238, brick);
        wall(-106,0,-261, -63,34,-260, brick);
        wall(-106,0,-220, -63,34,-219, brick);
        wall(-64,0,-260, -63,34,-245, brick);
        wall(-64,0,-235, -63,34,-220, brick);
        wall(-64,6,-245, -63,34,-235, brick);
        wall(-106,34,-261, -63,35,-219, brickD);
        a.exitDoor = B.door(-63.9f, -245, -63.1f, -235, 6.f, {0.2f,0.18f,0.2f}, false, 0.f);
        L.floors.push_back({-105.f, -260.f, -64.f, -220.f, 0.f, {0.22f,0.18f,0.18f}});
        wall(-105,22.4f,-260, -97,23,-220, steel);                    // balcony
        wall(-97,15.4f,-260, -64,16,-232, steel);                     // floor 1, hole north-east
        wall(-97,15.4f,-232, -76,16,-220, steel);
        wall(-91,8.4f,-260, -64,9,-246, steel);                       // floor 2, hole south-west
        wall(-105,8.4f,-246, -64,9,-220, steel);
        wall(-86,0,-242, -82,34,-238, brickD);                        // central column
        for (float y : {22.6f, 15.6f, 8.6f}) {                        // floor edges glow
            neon(-105,y,-259.9f, -64,y + 0.25f,-259.75f, red * 0.8f);
            neon(-105,y,-220.25f, -64,y + 0.25f,-220.1f, red * 0.8f);
        }
        neon(-97.12f,22.4f,-260, -97.f,23.f,-220, red);
        wall(-74,16,-254, -70,17.3f,-252, brickD);  wall(-92,9,-232, -88,10.3f,-230, brickD);
        wall(-100,0,-228, -96,1.3f,-226, brickD);   wall(-72,0,-256, -68,1.3f,-254, brickD);
        place({-101.f, 23.f, -232.f}, 1); place({-101.f, 23.f, -254.f}, 0);
        a.waves = {{
            W{EnemyType::HUSK, 0, {{-80,16,-250},{-90,16,-226},{-74,9,-226}}},
            W{EnemyType::SENTINEL, 0, {{-70,16,-246}}},
            W{EnemyType::MITE, 0, {{-80,9,-252},{-70,9,-240},{-96,9,-232}}},
            W{EnemyType::BRUTE, 0, {{-75,0,-240}}},
            W{EnemyType::RIPPER, 0, {{-95,0,-232},{-70,0,-250}}},
        }};
    }

    // =========================================================================
    // 5 THE PUMPWORKS — east under a roof. A 20 m tunnel with a lava strip
    // opens into a 32 m hall with side galleries and a pump block a Brute
    // guards, then narrows again to the exit.
    // =========================================================================
    {
        vec3 iron{0.34f,0.31f,0.30f}, rust{0.50f,0.28f,0.17f}, dark{0.18f,0.17f,0.18f}, orange{1.f,0.4f,0.08f};
        Arena& a = section("THE PUMPWORKS", "THROUGH THE HALL",
                           aabb(-64.4f, 0, -255.6f, 77.6f, 10, -224.4f), aabb(-63, 0, -256, 76, 9, -224),
                           {-58.f, 0.f, -240.f}, aabb(-44, -1, -257, 77, 40, -223), Ambient::EMBERS,
                           theme({0.05f,0.03f,0.02f}, {0.32f,0.14f,0.05f}, {0.05f,0.02f,0.01f}, {0.f,0.3f,-1.f}, {0.f,0.f,0.f}, 0.01f, 0.f,
                                 {0.06f,0.03f,0.02f}, 0.f, {0.2f,-1.f,0.15f}, {0.6f,0.42f,0.3f},
                                 {0.12f,0.10f,0.10f}, {0.42f,0.17f,0.06f}, {0.16f,0.07f,0.03f}, 0.014f));
        L.floors.push_back({-64.f, -256.f, 77.f, -224.f, 0.f, {0.24f,0.22f,0.21f}});
        wall(-64,0,-257, 77,10,-256, iron);  wall(-64,0,-224, 77,10,-223, iron);
        wall(-63,0,-256, -20,10,-250, iron); wall(-63,0,-230, -20,10,-224, iron);   // narrow west part
        wall(36,0,-256, 76,10,-250, iron);   wall(36,0,-230, 76,10,-224, iron);     // narrow east part
        wall(-64,10,-257, 77,11,-223, dark);                                        // roof
        wall(76,0,-250, 77,10,-245, iron); wall(76,0,-235, 77,10,-230, iron); wall(76,6,-245, 77,10,-235, iron);
        a.exitDoor = B.door(76.1f, -245, 76.9f, -235, 6.f, {0.22f,0.2f,0.18f}, false, 0.f);
        // Lava across the tunnel: jump it
        L.hazards.push_back({aabb(-38, 0, -250, -35, 0.06f, -230), 30.f});
        L.hazards.push_back({aabb(20, 0, -252, 24, 0.06f, -228), 30.f});
        for (auto& hz : L.hazards) neon(hz.box.min.x, 0, hz.box.min.z, hz.box.max.x, 0.06f, hz.box.max.z, {1.f,0.24f,0.02f});
        // The hall: galleries both sides, the pump block, pads up
        wall(-20,0,-256, 36,4,-252, rust); wall(-20,0,-228, 36,4,-224, rust);
        neon(-20,3.7f,-252.02f, 36,4,-251.9f, orange); neon(-20,3.7f,-228.1f, 36,4,-227.98f, orange);
        wall(0,0,-246, 16,5,-234, iron);
        B.ring(0, -246, 16, -234, 3.f, 3.3f, orange);
        pad({-12.f, 0.f, -247.f}, {0.f, 15.f, -8.f});
        pad({ 28.f, 0.f, -233.f}, {0.f, 15.f,  8.f});
        // Pillars in the tunnels, crates in the hall
        for (float x : {-28.f, 46.f, 60.f}) { wall(x - 1,0,-249, x + 1,10,-247, iron); wall(x - 1,0,-233, x + 1,10,-231, iron); }
        wall(-8,0,-240, -5,1.4f,-237, rust); wall(26,0,-246, 29,1.4f,-243, rust); wall(52,0,-242, 55,1.4f,-238, rust);
        place({-50.f, 0.f, -246.f}, 1); place({-50.f, 0.f, -234.f}, 0); place({-56.f, 0.f, -234.f}, 0);
        a.waves = {{
            W{EnemyType::RIPPER, 0, {{-30,0,-238},{-26,0,-244},{50,0,-234},{64,0,-244}}},
            W{EnemyType::HUSK, 0, {{-4,4,-254},{20,4,-226},{30,4,-254}}},
            W{EnemyType::BRUTE, 0, {{8,5,-240}}},
            W{EnemyType::JUGGERNAUT, 0, {{64,0,-240}}},
            W{EnemyType::MITE, 0, {{-10,0,-232},{-8,0,-248},{32,0,-240}}},
        }};
    }

    // =========================================================================
    // 6 THE TOWER — the finale. A courtyard with corner bastions and a 34 m
    // tower: fight two waves, then ride the lifts (or grapple) to the beacon.
    // =========================================================================
    {
        vec3 slate{0.26f,0.26f,0.34f}, slateD{0.16f,0.16f,0.22f}, mag{1.f,0.25f,0.75f}, cyan{0.25f,0.9f,1.f};
        const float TX = 111.f, TZ = -238.f;
        Arena& a = section("THE TOWER", "WIN THE YARD - THEN CLIMB",
                           aabb(76.4f, 0, -275.6f, 145.6f, 42, -203.6f), aabb(77, 0, -276, 146, 32, -204),
                           {80.f, 0.f, -240.f}, aabb(83, -1, -277, 147, 60, -203), Ambient::ASH,
                           theme({0.03f,0.01f,0.06f}, {0.36f,0.10f,0.30f}, {0.06f,0.02f,0.06f}, {-0.3f,0.25f,-1.f}, {1.6f,0.4f,1.1f}, 0.12f, 1.f,
                                 {0.10f,0.03f,0.10f}, 0.8f, {0.3f,-0.6f,0.7f}, {0.75f,0.55f,0.85f},
                                 {0.22f,0.14f,0.30f}, {0.10f,0.05f,0.10f}, {0.16f,0.05f,0.16f}, 0.008f));
        L.floors.push_back({76.f, -276.f, 146.f, -204.f, 0.f, {0.24f,0.22f,0.28f}});
        wall(76,0,-277, 77,12,-250, slate); wall(76,0,-230, 77,12,-203, slate); wall(76,10,-250, 77,12,-230, slate);
        wall(146,0,-277, 147,12,-203, slate); wall(76,0,-277, 147,12,-276, slate); wall(76,0,-204, 147,12,-203, slate);
        neon(77,11.4f,-276, 77.1f,11.7f,-204, mag); neon(145.9f,11.4f,-276, 146,11.7f,-204, mag);
        // Corner bastions and pads up to them
        wall(77,0,-276, 90,5,-264, slateD); wall(133,0,-276, 146,5,-264, slateD);
        wall(77,0,-216, 90,5,-204, slateD); wall(133,0,-216, 146,5,-204, slateD);
        pad({93.f, 0.f, -270.f}, {-5.f, 17.f, 0.f}); pad({130.f, 0.f, -270.f}, {5.f, 17.f, 0.f});
        pad({93.f, 0.f, -210.f}, {-5.f, 17.f, 0.f}); pad({130.f, 0.f, -210.f}, {5.f, 17.f, 0.f});
        // The tower, two balcony rings, lifts between them, pads to the top
        wall(TX - 6, 0, TZ - 6, TX + 6, 34, TZ + 6, slateD);
        for (float top : {12.f, 24.f}) {
            wall(TX - 10, top - 0.5f, TZ - 10, TX + 10, top, TZ - 6, slate);
            wall(TX - 10, top - 0.5f, TZ + 6,  TX + 10, top, TZ + 10, slate);
            wall(TX - 10, top - 0.5f, TZ - 6,  TX - 6,  top, TZ + 6, slate);
            wall(TX + 6,  top - 0.5f, TZ - 6,  TX + 10, top, TZ + 6, slate);
            B.ring(TX - 10, TZ - 10, TX + 10, TZ + 10, top - 0.5f, top - 0.2f, cyan);
        }
        B.ring(TX - 6, TZ - 6, TX + 6, TZ + 6, 33.6f, 34.f, mag);
        B.mover({TX - 12.f, 0.25f, TZ}, {2.f, 0.25f, 2.f}, Mover::Path::PINGPONG, {0,0,0}, {0, 11.75f, 0}, 7.f, 0.f, cyan);
        B.mover({TX + 12.f, 11.75f, TZ}, {2.f, 0.25f, 2.f}, Mover::Path::PINGPONG, {0,0,0}, {0, 12.f, 0}, 7.f, 0.5f, cyan);
        pad({TX, 24.f, TZ - 8.f}, {0.f, 26.f,  2.f});   // steep: clear the tower's edge first
        pad({TX, 24.f, TZ + 8.f}, {0.f, 26.f, -2.f});
        B.mover({TX, 18.25f, TZ}, {1.8f, 0.25f, 1.8f}, Mover::Path::ORBIT, {15.f,0,0}, {0,0,15.f}, 14.f, 0.f, mag);
        // Cover in the yard
        wall(95,0,-256, 99,1.3f,-254, slateD); wall(123,0,-222, 127,1.3f,-220, slateD);
        wall(95,0,-222, 97,1.6f,-218, slateD); wall(125,0,-258, 127,1.6f,-254, slateD);
        wall(136,0,-242, 138,1.3f,-234, slateD);
        L.finishPos = {TX, 34.f, TZ};
        L.gems.push_back({{TX, 37.5f, TZ}, {1.8f, 0.5f, 1.4f}, 1.3f, true});
        place({82.f, 0.f, -232.f}, 1); place({82.f, 0.f, -248.f}, 0);
        a.waves = {
            {
                W{EnemyType::HUSK, 0, {{100,0,-260},{122,0,-260},{100,0,-216},{122,0,-216}}},
                W{EnemyType::RIPPER, 0, {{130,0,-238},{136,0,-252},{134,0,-226}}},
                W{EnemyType::SENTINEL, 0, {{140,5,-270},{140,5,-209}}},
                W{EnemyType::RAPTOR, 0, {{111,20,-258},{111,20,-218}}},
            },
            {
                W{EnemyType::BRUTE, 0, {{120,0,-266}}},
                W{EnemyType::JUGGERNAUT, 0, {{130,0,-240}}},
                W{EnemyType::MITE, 0, {{136,0,-258},{136,0,-220},{125,0,-212},{125,0,-268}}},
                W{EnemyType::HUSK, 0, {{TX,12,TZ - 8},{TX,12,TZ + 8}}},
                W{EnemyType::SENTINEL, 0, {{83,5,-270},{83,5,-209}}},
            },
        };
    }

    // Lighting crossfades through each gate
    L.blends = {
        {aabb(-22, 0, -124, -9, 60, -100),     0, 1, 0, true},
        {aabb(-136, 0, -131, -112, 60, -118),  1, 2, 2, true},
        {aabb(-112, 0, -250, -99, 60, -234),   2, 3, 0, false},
        {aabb(-70, 0, -250, -57, 60, -230),    3, 4, 0, false},
        {aabb(70, 0, -250, 84, 60, -230),      4, 5, 0, false},
    };

    // Par times (seconds): S, A, B, C
    L.parTimes[0] = 270.f; L.parTimes[1] = 390.f; L.parTimes[2] = 540.f; L.parTimes[3] = 720.f;
    return L;
}
