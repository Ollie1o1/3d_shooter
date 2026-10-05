#pragma once
// =============================================================================
// LevelGauntlet.h — FAST mode's map: THE GAUNTLET.
//
// A time trial through seven rooms joined by boost tubes. Every room is
// reached down a ribbed duct (a breather: health to pick up, nothing
// shooting at you) that fires you along at speed; its doors part as you run
// at them. Step into the room and the fight starts: the door behind you
// locks, and the exit stays locked until the room is clear.
//
//                          ┌──────────┐
//                          │ 4 SPAN   │ islands over a canyon (y 20)
//                          │  (void)  ├──▶ 5 WELL ──▶ 6 PUMPWORKS ──▶ 7 TOWER
//                          └────▲─────┘   drop down     hall + control    courtyard,
//                               │          the floors    rooms            climb to the
//   3 ASCENT ◀── 2 SLUICE ◀── 1 CANAL                                     beacon
//   cathedral    pit and       long sunken
//   of terraces  mezzanine     lane, north from the start airlock
//
//   room            size        idea
//   1 THE CANAL     30 x 80     sunken lane, walkways, bridges, beams to grapple
//   2 THE SLUICE    24 x 24     small and close: a pit ringed by a mezzanine
//   3 THE ASCENT    88 x 29     roofed hall of six terraces climbing 20 m west
//   4 THE SPAN      44 x 120    open canyon: islands, ferries, a void
//   5 THE WELL      39 x 39     a tower you fall through, floor by floor
//   6 THE PUMPWORKS 60 x 39     hall with galleries and two control rooms
//   7 THE TOWER     70 x 71     courtyard fight, then a lift shaft to the beacon
//
// Each room has a set piece built from Shapes.h (round, curved and turned
// geometry over box collision): the Canal's dam, aqueduct and skyline, the
// Sluice's turbine, the Ascent's vaulting and rose window, the Span's rock,
// broken viaduct and the ring on the horizon, the Well's lit core, the
// Pumpworks' tanks and pistons, the Tower's octagonal keep.
//
// Rooms are Arenas run by WaveDirector with fast = true; each has a trigger,
// hand-placed enemies (WaveEntry::at), its tube as an extra zone and its own
// theme, crossfaded down the tubes by LevelData::blends.
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
    // Keep a wall's collision but draw a shape in its place
    auto hide = [&](int w) { L.walls[w].hidden = true; };
    auto prop = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { B.prop(x0,y0,z0,x1,y1,z1,c); };
    auto pad  = [&](vec3 at, vec3 launch) { L.pads.push_back({at, {1.3f, 1.3f}, launch}); };
    auto place = [&](vec3 at, int kind) { L.placedPickups.push_back({at, kind}); };
    using W = WaveEntry;
    using Gap = LevelBuilder::Gap;
    const float TW = 5.f, TH = 4.5f;    // every tube: 5 m wide, 4.5 m tall inside
    const float BOOST = 26.f;           // m/s down a boost tube
    // A door in the middle of a tube opens from further off: you arrive fast
    auto tubeDoor = [&](float x, float zc, vec3 trim) {
        int d = B.doorway(false, zc - TW * 0.5f, zc + TW * 0.5f, x - 0.5f, x + 0.5f, 0.f, TH, trim);
        L.doors[d].sense = 12.f;
        return d;
    };

    // Far ground below everything
    L.floors.push_back({-240.f, -320.f, 110.f, 30.f, -0.3f, {0.10f,0.09f,0.11f}});

    auto theme = [](vec3 zen, vec3 hor, vec3 gnd, vec3 sunDir, vec3 sunCol, float sunSize, float stripes,
                    vec3 mtn, float stars, vec3 lightDir, vec3 lightCol, vec3 skyA, vec3 gndA, vec3 fog, float fogD) {
        return Theme{zen, hor, gnd, glm::normalize(sunDir), sunCol, sunSize, stripes, mtn, stars,
                     glm::normalize(lightDir), lightCol, skyA, gndA, fog, fogD};
    };

    auto section = [&](const char* name, const char* sub, AABB zone, AABB bounds, vec3 start, float yaw, AABB trigger,
                       Ambient amb, Theme th) -> Arena& {
        Arena a;
        a.name = name; a.subtitle = sub;
        a.zone = zone; a.bounds = bounds; a.playerStart = start; a.startYaw = yaw;
        a.trigger = trigger; a.hasTrigger = true;
        a.maxAlive = 99; a.ambient = amb; a.theme = th;
        a.damageScale = 1.f + 0.05f * (float)L.arenas.size();   // harder the further in
        L.arenas.push_back(std::move(a));
        return L.arenas.back();
    };

    // =========================================================================
    // 1 THE CANAL — out of the start airlock, a boost tube north into a long
    // sunken lane between raised walkways. Bridges cross it, beams overhead
    // are grapple points. Exit west off the far end of the left walkway.
    // =========================================================================
    {
        vec3 conc{0.52f,0.54f,0.58f}, dark{0.32f,0.34f,0.39f}, crate{0.45f,0.33f,0.22f}, cyan{0.2f,0.9f,1.f},
             panel{0.30f,0.32f,0.38f}, teal{0.2f,1.f,0.8f};
        Arena& a = section("THE CANAL", "PUSH UP THE CANAL",
                           aabb(-15, 0, -125, 15, 22, -46), aabb(-15, 0, -125, 15, 16, -46),
                           {0.f, 3.f, 0.f}, -90.f, aabb(-15, -1, -125, 15, 40, -63), Ambient::DUST,
                           theme({0.10f,0.20f,0.36f}, {0.95f,0.62f,0.42f}, {0.2f,0.18f,0.2f}, {0.3f,0.2f,-1.f}, {1.6f,0.9f,0.5f}, 0.09f, 0.f,
                                 {0.22f,0.2f,0.28f}, 0.f, {-0.3f,-0.6f,0.75f}, {1.1f,0.85f,0.65f},
                                 {0.38f,0.36f,0.46f}, {0.18f,0.15f,0.14f}, {0.62f,0.48f,0.46f}, 0.006f));
        // --- The start airlock (floor 3, roofed), its door, and the tube north ---
        B.mat = Mat::PANEL;
        wall(-5,0,-8, 5,3,4, panel);                                   // floor block
        B.wallX(-6, 6, 4, 5, 0, 9, panel);
        B.wallZ(-9, 5, 5, 6, 0, 9, panel);
        B.wallZ(-9, 5, -6, -5, 0, 9, panel);
        B.wallX(-6, 6, -9, -8, 0, 9, panel, {Gap{-2.5f, 2.5f, 3.f, 3.f + TH}});
        wall(-6,8,-9, 6,9,5, panel * 0.8f);                            // roof
        B.doorway(true, -2.5f, 2.5f, -9, -8, 3.f, TH, cyan);
        for (float x : {-2.f, 2.f}) neon(x - 0.12f, 7.92f, -7, x + 0.12f, 8, 3, vec3{0.7f,0.85f,1.f} * 0.45f);   // ceiling strips
        for (float sx : {-1.f, 1.f}) {                                 // wall screens in dark frames
            float x = sx * 5.f;
            wall(x - sx * 0.25f, 4.2f, -4.6f, x, 6.6f, -1.4f, panel * 0.5f);
            neon(x - sx * 0.3f, 4.4f, -4.4f, x - sx * 0.25f, 6.4f, -1.6f, cyan * 0.18f);
            neon(x - sx * 0.3f, 4.5f, -1.f, x - sx * 0.25f, 4.9f, 1.f, vec3{1.f,0.6f,0.2f} * 0.4f);
        }
        neon(-5,3,3.88f, 5,3.12f,4, cyan);                             // floor edges
        AABB t0 = B.tube(2, -45.f, -9.f, 0.f, 3.f, TW, TH, panel, cyan);
        B.booster(aabb(-2.5f, 3, -39, 2.5f, 3 + TH, -12), {0, 0, -1}, BOOST);
        a.extraZones = {aabb(-5, 3, -8, 5, 8, 4), t0};

        // --- The canal: walls, the plaza you arrive on, walkways, the lane ---
        // The side walls are only parapets over the walkways: past them the
        // city, and ahead, over the sluice gate, the dam the canal ran from
        B.mat = Mat::CONCRETE;
        B.wallX(-16, 16, -46, -45, 0, 11, conc, {Gap{-2.5f, 2.5f, 3.f, 3.f + TH}});
        a.entryGate = B.doorway(true, -2.5f, 2.5f, -46, -45, 3.f, TH, cyan);
        B.wallX(-16, 16, -126, -125, 0, 11, conc);
        B.wallZ(-126, -45, 15, 16, 0, 6, conc);
        B.wallZ(-126, -45, -16, -15, 0, 6, conc, {Gap{-121, -116, 3.f, 3.f + TH}});
        wall(-16, 3.f + TH, -121.5f, -15, 9.f, -115.5f, conc);          // the exit's lintel block
        a.exitDoor = B.doorway(false, -121, -116, -16, -15, 3.f, TH, cyan, true);
        wall(-15,0,-62, 15,3,-46, conc);                               // plaza
        wall(-15,0,-125, -9,3,-62, dark);                              // walkways
        wall(9,0,-125, 15,3,-62, dark);
        L.floors.push_back({-9.f, -125.f, 9.f, -62.f, 0.f, {0.18f,0.34f,0.38f}});
        neon(-9.12f,2.75f,-125, -9.0f,2.95f,-62, cyan);
        neon(9.0f,2.75f,-125, 9.12f,2.95f,-62, cyan);
        neon(-15,2.75f,-62.12f, 15,2.95f,-62.0f, cyan);
        neon(-15,10.8f,-45.98f, 15,11,-45.86f, cyan * 0.7f);           // wall-top trim
        neon(-15,10.8f,-125.14f, 15,11,-125.02f, cyan * 0.7f);
        for (float x : {-15.06f, 15.f}) neon(x, 5.85f, -125, x + 0.06f, 6.f, -46, cyan * 0.5f);
        {
            ShapeKit k = B.kit(), g = B.kit(true);
            // Buttresses leaning on the parapets, every 9 m
            for (float sx : {-1.f, 1.f}) for (float z = -70.f; z > -124.f; z -= 9.f) {
                if (z < -114.f && z > -123.f && sx < 0.f) continue;    // the exit
                k.box({sx * 14.55f, 4.4f, z}, {0.9f, 3.4f, 1.1f}, dark, 0.f, 0.f, sx * 0.22f);
                B.solid(sx > 0 ? 14.1f : -15.f, 3.f, z - 0.55f, sx > 0 ? 15.f : -14.1f, 5.6f, z + 0.55f);
            }
            // The arrival plaza: a round fountain, a lamp on each corner
            k.column({0, 3, -57.f}, 2.6f, 0.6f, conc * 0.9f, 16);
            g.column({0, 3.62f, -57.f}, 2.2f, 0.04f, teal * 0.22f, 16);
            k.column({0, 3, -57.f}, 0.45f, 3.2f, conc, 8, 0.7f);
            g.column({0, 6.2f, -57.f}, 0.5f, 0.5f, teal, 8, 0.2f);
            B.solid(-2.4f, 3.f, -59.4f, 2.4f, 3.6f, -54.6f);
            for (float sx : {-1.f, 1.f}) {
                k.column({sx * 13.5f, 3, -48.5f}, 0.18f, 4.2f, dark, 6);
                g.column({sx * 13.5f, 7.2f, -48.5f}, 0.35f, 0.5f, vec3{1.f, 0.75f, 0.45f}, 6, 0.6f);
            }
            // Stone arches under the bridges
            for (float z : {-82.f, -106.f}) {
                k.arch({0, 0.9f, z}, 18.f, 1.6f, 0.5f, 3.8f, conc * 0.85f);
                g.arch({0, 0.88f, z}, 18.f, 1.6f, 0.12f, 3.9f, cyan * 0.35f);
            }
            // Round columns with capitals in place of the square pillars
            for (float sx : {-12.f, 12.f}) for (float z : {-88.f, -112.f}) {
                B.solid(sx - 1, 3, z - 1, sx + 1, 11, z + 1);
                k.column({sx, 3, z}, 1.25f, 0.5f, conc * 0.8f, 12);
                k.column({sx, 3.5f, z}, 0.95f, 6.6f, conc, 12, 0.9f);
                k.column({sx, 10.1f, z}, 0.9f, 0.9f, conc * 0.9f, 12, 1.5f);
                g.column({sx, 9.55f, z}, 0.92f, 0.25f, cyan, 12);
            }
            // Drums for climbing out of the lane
            const float drums[][2] = {{-8.f, -74.5f}, {8.f, -94.5f}, {-8.f, -112.5f}, {8.f, -120.5f}};
            for (auto& d : drums) {
                B.solid(d[0] - 1.f, 0, d[1] - 1.5f, d[0] + 1.f, 1.5f, d[1] + 1.5f);
                for (float o : {-0.75f, 0.75f}) {
                    k.column({d[0], 0, d[1] + o}, 0.72f, 1.5f, crate, 10);
                    g.column({d[0], 1.1f, d[1] + o}, 0.74f, 0.08f, vec3{1.f, 0.6f, 0.2f} * 0.5f, 10);
                }
            }
        }
        // Arched trusses across the room (grapple their crowns), springing
        // from the parapets
        B.mat = Mat::METAL;
        {
            ShapeKit k = B.kit(), g = B.kit(true);
            for (float z : {-70.f, -94.f, -118.f}) {
                k.arch({0, 6.f, z}, 30.f, 3.6f, 0.55f, 0.7f, {0.26f,0.27f,0.3f}, 0.f, 18);
                g.arch({0, 5.8f, z}, 30.f, 3.6f, 0.1f, 0.2f, teal * 0.6f, 0.f, 18);
                for (float sx : {-1.f, 1.f}) k.rod({sx * 15.f, 6.f, z}, {sx * 7.5f, 9.1f, z}, 0.12f, {0.22f,0.23f,0.26f}, 6);
                B.solid(-8, 9.f, z - 0.4f, 8, 9.6f, z + 0.4f);
            }
        }
        B.mat = Mat::CONCRETE;
        // Cover in the lane and on the walkways
        wall(-3,0,-66, 3,1.2f,-64, dark); wall(-6,0,-100, -2,1.2f,-98, dark); wall(2,0,-112, 6,1.2f,-110, dark);
        wall(-14,3,-96, -11.5f,4.2f,-95, dark); wall(10,3,-74, 14,4.2f,-73, dark);
        pad({-5.f, 0.f, -90.f}, {-7.f, 14.f, 0.f});
        pad({ 5.f, 0.f, -100.f}, { 7.f, 14.f, 0.f});
        // The sluice gate the canal runs out of, at the north end
        B.mat = Mat::METAL;
        prop(-9,0,-125, 9,8.5f,-124.6f, {0.22f,0.23f,0.26f});
        for (float x = -8.f; x <= 8.f; x += 2.f) neon(x - 0.1f, 0.3f, -124.62f, x + 0.1f, 8.2f, -124.5f, teal * 0.45f);
        neon(-9,8.3f,-124.6f, 9,8.5f,-124.45f, teal);
        L.fans.push_back({{-11.5f, 7.f, -124.8f}, 1.4f, 2, teal});
        L.fans.push_back({{ 11.5f, 7.f, -124.8f}, 1.4f, 2, teal});
        // ---- Beyond the walls ----
        {
            B.mat = Mat::CONCRETE;
            ShapeKit k = B.kit(), g = B.kit(true);
            vec3 damC{0.5f,0.49f,0.5f}, water{0.35f,0.95f,1.1f};
            // THE DAM: a curved wall 46 m high bowed toward you, spillways
            // pouring light down its face, a tower at each end
            const vec3 DC{0.f, -0.3f, -55.f};
            const float DR = 92.f, A0 = glm::radians(-90.f - 33.f), A1 = glm::radians(-90.f + 33.f);
            k.curve(DC, DR, A0, A1, -0.3f, 46.f, 5.f, damC, 30);
            k.curve(DC, DR - 3.5f, A0, A1, -0.3f, 7.f, 3.f, damC * 0.8f, 30);          // the toe
            k.curve(DC, DR + 1.5f, A0, A1, 46.f, 48.f, 8.f, damC * 0.7f, 30);         // the crest road
            g.curve(DC, DR - 2.6f, A0, A1, 45.6f, 46.f, 0.2f, water * 0.6f, 30);
            for (int i = 0; i < 5; ++i) {                                              // spillways
                float an = glm::radians(-90.f + (i - 2) * 9.f);
                vec3 p = DC + vec3{std::cos(an) * (DR - 2.7f), 0, std::sin(an) * (DR - 2.7f)};
                g.box(p + vec3{0, 26.f, 0}, {0.3f, 36.f, 3.6f}, water * 0.75f, -an);
                g.box(p + vec3{0, 26.f, 0} + vec3{std::cos(an), 0, std::sin(an)} * -0.2f, {0.2f, 36.f, 1.2f}, water * 1.4f, -an);
                k.box(p + vec3{0, 45.f, 0} + vec3{std::cos(an), 0, std::sin(an)} * -1.2f, {2.f, 2.5f, 5.f}, damC * 0.6f, -an);
                g.column(p + vec3{std::cos(an), 0, std::sin(an)} * -3.f, 4.5f, 0.4f, water * 0.5f, 10);   // foam
            }
            for (float s : {-1.f, 1.f}) {                                              // the dam's towers
                float an = s < 0 ? A0 : A1;
                vec3 p = DC + vec3{std::cos(an) * DR, 0, std::sin(an) * DR};
                k.column(p, 6.5f, 54.f, damC * 0.75f, 12, 0.82f);
                k.dome(p + vec3{0, 54.f, 0}, 5.4f, damC * 0.6f);
                for (float y : {20.f, 34.f, 48.f}) g.column(p + vec3{0, y, 0}, 6.5f * (1.f - 0.18f * y / 54.f) + 0.06f, 0.35f, cyan * 0.6f, 12);
                g.column(p + vec3{0, 59.2f, 0}, 0.25f, 3.f, {1.6f, 0.2f, 0.15f}, 6);
            }
            // THE AQUEDUCT: an arcade crossing over the canal, broken where
            // it spans it, its pieces hanging
            B.mat = Mat::BRICK;
            ShapeKit b = B.kit();
            vec3 brick{0.5f,0.36f,0.3f};
            for (float x = -100.f; x <= 100.f; x += 12.f) {
                if (std::fabs(x) < 13.f) continue;
                b.box({x, 9.f, -99.f}, {2.4f, 18.6f, 3.4f}, brick * 0.85f);
            }
            for (float x = -94.f; x <= 94.f; x += 12.f) {
                if (std::fabs(x) < 18.f) continue;
                b.arch({x, 12.f, -99.f}, 9.6f, 4.8f, 1.f, 3.4f, brick);
                b.box({x, 18.4f, -99.f}, {12.f, 1.6f, 3.6f}, brick * 0.9f);
                g.box({x, 17.5f, -97.25f}, {12.f, 0.16f, 0.1f}, vec3{1.f, 0.7f, 0.4f} * 0.6f);
            }
            for (float s : {-1.f, 1.f}) {   // the broken ends, and what fell
                b.box({s * 15.5f, 18.6f, -99.f}, {7.f, 1.6f, 3.6f}, brick * 0.9f, 0.f, 0.f, s * 0.35f);
                b.box({s * 9.f, 15.2f, -99.6f}, {5.f, 1.3f, 3.2f}, brick * 0.8f, 0.3f, 0.2f, s * 0.9f);
            }
            // THE CITY: towers on both sides and behind you, lit windows
            uint32_t seed = 7;
            auto rnd = [&]() { seed = seed * 1664525u + 1013904223u; return (float)((seed >> 8) & 0xFFFF) / 65535.f; };
            struct Spot { float x, z; };
            std::vector<Spot> spots;
            for (int i = 0; i < 14; ++i) spots.push_back({32.f + rnd() * 100.f, -190.f + rnd() * 200.f});    // east
            for (int i = 0; i < 9; ++i)  spots.push_back({-32.f - rnd() * 90.f, -102.f + rnd() * 120.f});    // west
            for (int i = 0; i < 6; ++i)  spots.push_back({-28.f + rnd() * 56.f, 18.f + rnd() * 60.f});      // south
            B.mat = Mat::PANEL;
            ShapeKit t = B.kit();
            vec3 glass{0.2f,0.22f,0.3f}, win{1.f,0.72f,0.45f};
            for (auto& sp : spots) {
                float h = 25.f + rnd() * 70.f, r = 4.f + rnd() * 6.f;
                vec3 base{sp.x, -0.3f, sp.z};
                int kind = (int)(rnd() * 3.f);
                float taper = kind == 0 ? 0.55f + rnd() * 0.3f : kind == 1 ? 1.f : 0.8f;
                int sides = kind == 1 ? 4 : kind == 2 ? 6 : 10;
                glm::mat4 turn = ShapeKit::R(rnd() * 3.f, {0, 1, 0});
                t.column(base, r, h, glass * (0.7f + 0.5f * rnd()), sides, taper).xf *= turn;
                for (float y = 6.f; y < h - 3.f; y += 5.f + rnd() * 6.f) {
                    float rr = r * (1.f - (1.f - taper) * y / h) + 0.07f;
                    g.column(base + vec3{0, y, 0}, rr, 0.35f, (rnd() < 0.3f ? cyan * 0.5f : win * 0.45f), sides).xf *= turn;
                }
                if (kind == 0) g.column(base + vec3{0, h, 0}, 0.2f, 6.f, {1.6f, 0.2f, 0.15f}, 4);   // a mast
            }
        }
        B.mat = Mat::BRICK;
        place({0.f, 3.f, -50.f}, 0);
        a.waves = {
            {
                W{EnemyType::HUSK, 0, {{-12,3,-80},{12,3,-92},{12,3,-118},{-12,3,-104},{0,0,-122}}},
                W{EnemyType::SENTINEL, 0, {{-5,3,-106},{5,3,-82}}},
                W{EnemyType::RIPPER, 0, {{0,0,-92},{-4,0,-103},{4,0,-116}}},
                W{EnemyType::RAPTOR, 0, {{-5,11,-100},{5,11,-114}}},
            },
            {
                W{EnemyType::BRUTE, 0, {{0,0,-118}}},
                W{EnemyType::MITE, 0, {{-6,0,-86},{6,0,-90},{0,0,-76}}},
                W{EnemyType::HUSK, 0, {{-12,3,-120},{12,3,-66}}},
                W{EnemyType::RAPTOR, 0, {{0,12,-110}}},
            },
        };
    }

    // =========================================================================
    // 2 THE SLUICE — west down a boost tube into a small, roofed pump room:
    // you arrive on a mezzanine ringing a pit, with a pump block in the middle.
    // Close quarters. Exit west at the bottom of the pit.
    // =========================================================================
    {
        vec3 conc{0.40f,0.45f,0.46f}, steel{0.30f,0.33f,0.36f}, rust{0.44f,0.30f,0.20f}, amber{1.f,0.62f,0.15f},
             green{0.35f,1.f,0.55f}, panel{0.30f,0.32f,0.38f};
        Arena& a = section("THE SLUICE", "CLOSE QUARTERS",
                           aabb(-81, 0, -131, -57, 12, -107), aabb(-81, 0, -131, -57, 11, -107),
                           {-18.f, 3.f, -118.5f}, 180.f, aabb(-80, -1, -131, -59, 40, -107), Ambient::STEAM,
                           theme({0.03f,0.06f,0.06f}, {0.12f,0.28f,0.26f}, {0.03f,0.05f,0.05f}, {0.f,0.3f,-1.f}, {0.f,0.f,0.f}, 0.01f, 0.f,
                                 {0.04f,0.08f,0.08f}, 0.f, {0.2f,-1.f,0.1f}, {0.55f,0.72f,0.66f},
                                 {0.15f,0.21f,0.21f}, {0.10f,0.18f,0.15f}, {0.06f,0.14f,0.13f}, 0.02f));
        AABB t1 = B.tube(0, -56.f, -16.f, -118.5f, 3.f, TW, TH, panel, amber);
        B.booster(aabb(-50, 3, -121, -22, 3 + TH, -116), {-1, 0, 0}, BOOST);
        a.extraZones = {t1};
        place({-26.f, 3.f, -118.5f}, 1);

        B.mat = Mat::CONCRETE;
        B.wallZ(-132, -106, -57, -56, 0, 13, conc, {Gap{-121, -116, 3.f, 3.f + TH}});
        a.entryGate = B.doorway(false, -121, -116, -57, -56, 3.f, TH, amber);
        B.wallZ(-132, -106, -82, -81, 0, 13, conc, {Gap{-121, -116, 0.f, TH}});
        a.exitDoor = B.doorway(false, -121, -116, -82, -81, 0.f, TH, amber, true);
        B.wallX(-82, -56, -132, -131, 0, 13, conc);
        B.wallX(-82, -56, -107, -106, 0, 13, conc);
        B.mat = Mat::METAL;
        wall(-82,12,-132, -56,13,-106, steel * 0.7f);                  // roof
        // The mezzanine: east, north and south strips at 3 m, open to the west
        wall(-61,2.5f,-131, -57,3,-107, steel);
        wall(-81,2.5f,-131, -61,3,-127, steel);
        wall(-81,2.5f,-111, -61,3,-107, steel);
        neon(-61.1f,2.55f,-127, -61,2.95f,-111, amber);
        neon(-81,2.55f,-127.1f, -61,2.95f,-127, amber);
        neon(-81,2.55f,-111, -61,2.95f,-110.9f, amber);
        for (float z : {-127.6f, -111.f}) wall(-61.6f,0,z, -61,2.5f,z + 0.6f, steel);   // posts under the corners
        // The pump block, its pipe to the roof, and pads up to the mezzanine and onto it
        // ...a turbine: a round, ribbed housing, its shaft up to the roof,
        // pipes curving away from it into the walls, a ring of light turning
        hide(wall(-73,0,-122, -67,6,-116, rust));
        hide(wall(-70.6f,6,-119.6f, -69.4f,12,-118.4f, steel));
        {
            ShapeKit k = B.kit(), g = B.kit(true);
            const vec3 TC{-70.f, 0.f, -119.f};
            k.column(TC, 3.75f, 0.7f, steel * 0.8f, 16);
            k.column(TC, 3.35f, 5.4f, rust, 16, 0.94f);
            k.column(TC + vec3{0, 5.4f, 0}, 3.6f, 0.6f, steel * 0.8f, 16);
            for (int i = 0; i < 8; ++i) {
                float an = i * 0.7854f;
                k.box(TC + vec3{std::cos(an) * 3.35f, 3.f, std::sin(an) * 3.35f}, {0.5f, 4.6f, 0.4f}, steel * 0.7f, -an);
            }
            for (float y : {1.6f, 4.7f}) g.column(TC + vec3{0, y, 0}, 3.4f - 0.04f * y, 0.3f, green, 16);
            g.column(TC + vec3{0, 6.f, 0}, 1.7f, 0.04f, green * 0.5f, 12);
            k.column(TC + vec3{0, 6.f, 0}, 0.7f, 6.f, steel, 10);
            g.column(TC + vec3{0, 9.f, 0}, 0.74f, 0.3f, green, 10);
            L.spinners.push_back({TC + vec3{0, 3.1f, 0}, 4.3f, 1.4f, 14, green * 1.2f});
            // Pipes out of the housing's top, arching into the four walls
            const vec3 ends[] = {{-57.f, 9.5f, -119.f}, {-81.f, 9.5f, -119.f}, {-70.f, 9.5f, -131.f}, {-70.f, 9.5f, -107.f}};
            for (auto& e : ends) {
                vec3 d = glm::normalize(vec3{e.x - TC.x, 0, e.z - TC.z});
                vec3 p0 = TC + d * 2.6f + vec3{0, 6.f, 0}, p1 = TC + d * 3.6f + vec3{0, 9.5f, 0};
                vec3 prev = p0;
                for (int i = 1; i <= 8; ++i) {
                    float u = i / 8.f;
                    vec3 q = (1 - u) * (1 - u) * p0 + 2 * (1 - u) * u * p1 + u * u * e;
                    k.rod(prev, q, 0.45f, rust * 0.8f, 8);
                    prev = q;
                }
                g.rod(e - d * 1.2f, e - d * 1.0f, 0.5f, green * 0.6f, 8);
            }
            // Vaulted ribs across the roof
            for (float x : {-64.f, -76.f}) k.arch({x, 8.6f, -119.f}, 24.f, 3.2f, 0.45f, 0.6f, steel * 0.8f, 1.5708f);
            k.arch({-69.f, 8.6f, -119.f}, 24.f, 3.2f, 0.45f, 0.6f, steel * 0.8f, 0.f);
        }
        pad({-70.f, 0.f, -113.f}, {0.f, 14.f, 4.f});
        pad({-70.f, 0.f, -125.f}, {0.f, 14.f, -4.f});
        pad({-77.f, 0.f, -119.f}, {4.f, 19.f, 0.f});
        // Pipes along the walls, fans, a grate glowing in the pit floor
        for (float y : {6.5f, 8.5f}) {
            prop(-80.9f, y, -131, -80.3f, y + 0.6f, -122, rust * 0.8f);
            prop(-80.9f, y, -115, -80.3f, y + 0.6f, -107, rust * 0.8f);
        }
        prop(-81, 9.5f, -130.9f, -57, 10.1f, -130.3f, steel);
        L.fans.push_back({{-75.f, 8.f, -107.3f}, 1.5f, 2, green});
        L.fans.push_back({{-63.f, 8.f, -107.3f}, 1.5f, 2, green});
        L.fans.push_back({{-70.f, 11.9f, -112.5f}, 1.6f, 1, amber});
        neon(-79,0,-114, -75,0.03f,-112, green * 0.35f);
        neon(-66,0,-126, -62,0.03f,-124, green * 0.35f);
        B.mat = Mat::BRICK;
        a.waves = {
            {
                W{EnemyType::HUSK, 0, {{-76,3,-129},{-66,3,-129},{-76,3,-109},{-64,3,-109}}},
                W{EnemyType::RIPPER, 0, {{-78,0,-119},{-64,0,-114}}},
                W{EnemyType::MITE, 0, {{-75,0,-113},{-64,0,-124},{-77,0,-124}}},
                W{EnemyType::SENTINEL, 0, {{-71.5f,6,-117}}},
            },
            {
                W{EnemyType::JUGGERNAUT, 0, {{-77,0,-119}}},
                W{EnemyType::RIPPER, 0, {{-63,0,-119},{-64,0,-125}}},
                W{EnemyType::HUSK, 0, {{-59,3,-125},{-59,3,-112}}},
                W{EnemyType::MITE, 0, {{-66,0,-113},{-78,0,-128}}},
            },
        };
    }

    // =========================================================================
    // 3 THE ASCENT — west down another tube into a roofed hall of six 4 m
    // terraces climbing to 20 m. Each riser has a jump pad and a step block;
    // gunners hold the terraces above you. Exit north from the top.
    // =========================================================================
    {
        vec3 sand{0.66f,0.52f,0.38f}, sandD{0.48f,0.36f,0.27f}, gold{1.f,0.7f,0.3f}, panel{0.34f,0.31f,0.28f};
        Arena& a = section("THE ASCENT", "CLIMB THE TERRACES",
                           aabb(-201, 0, -133, -113, 34, -104), aabb(-201, 0, -133, -113, 33, -104),
                           {-84.f, 0.f, -118.5f}, 180.f, aabb(-201, -1, -133, -128, 60, -104), Ambient::DUST,
                           theme({0.14f,0.22f,0.42f}, {1.0f,0.72f,0.40f}, {0.25f,0.2f,0.18f}, {-1.f,0.25f,0.1f}, {1.8f,1.1f,0.5f}, 0.1f, 0.f,
                                 {0.35f,0.26f,0.25f}, 0.f, {0.8f,-0.5f,-0.2f}, {1.2f,0.9f,0.6f},
                                 {0.42f,0.38f,0.44f}, {0.22f,0.16f,0.12f}, {0.75f,0.58f,0.45f}, 0.005f));
        AABB t2 = B.tube(0, -112.f, -82.f, -118.5f, 0.f, TW, TH, panel, gold);
        B.booster(aabb(-108, 0, -121, -88, TH, -116), {-1, 0, 0}, BOOST);
        a.extraZones = {t2};

        // The hall: tall walls with window slits high up, a roof at 35 m
        std::vector<Gap> windows;
        for (float x = -197.f; x < -115.f; x += 14.f) windows.push_back({x, x + 3.f, 23.f, 31.f});
        std::vector<Gap> north = windows;
        north.erase(std::remove_if(north.begin(), north.end(), [](const Gap& g) { return g.a1 > -199.f && g.a0 < -188.f; }), north.end());
        north.push_back({-193, -188, 20.f, 20.f + TH});
        B.wallZ(-134, -103, -113, -112, 0, 36, sand, {Gap{-121, -116, 0.f, TH}});
        a.entryGate = B.doorway(false, -121, -116, -113, -112, 0.f, TH, gold);
        B.wallZ(-134, -103, -202, -201, 0, 36, sand);
        B.wallX(-202, -112, -104, -103, 0, 36, sand, windows);
        B.wallX(-202, -112, -134, -133, 0, 36, sand, north);
        a.exitDoor = B.doorway(true, -193, -188, -134, -133, 20.f, TH, gold, true);
        wall(-202,35,-134, -112,36,-103, sandD * 0.8f);
        for (auto& g : windows) {                                       // sun through the slits
            neon(g.a0, g.y0, -103.95f, g.a1, g.y0 + 0.15f, -103.8f, gold * 0.6f);
            neon(g.a0, g.y0, -133.2f, g.a1, g.y0 + 0.15f, -133.05f, gold * 0.6f);
        }
        // Terraces and the risers between them
        struct T { float x0, x1, top; };
        const T terr[] = {{-127,-113,0}, {-142,-127,4}, {-157,-142,8}, {-172,-157,12}, {-187,-172,16}, {-201,-187,20}};
        for (int i = 1; i < 6; ++i) {
            wall(terr[i].x0, 0, -133, terr[i].x1, terr[i].top, -104, i % 2 ? sandD : sand);
            neon(terr[i].x1 - 0.12f, terr[i].top - 0.3f, -133, terr[i].x1 + 0.02f, terr[i].top - 0.05f, -104, gold);
        }
        for (int i = 0; i < 5; ++i) {
            float xb = terr[i + 1].x1, lo = terr[i].top;      // riser between terrace i and i+1
            pad({xb + 3.f, lo, -110.f}, {-6.f, 15.f, 0.f});
            wall(xb, lo, -131, xb + 2.f, lo + 2.f, -127, sandD);   // step block
        }
        // Columns along both walls, beams across at 28 m (grapple them)
        for (float x : {-120.f, -135.f, -150.f, -165.f, -180.f, -195.f})
            for (float z : {-132.f, -105.f}) {
                hide(wall(x - 1, 0, z - 1, x + 1, 35, z + 1, sandD));
                ShapeKit k = B.kit(), g = B.kit(true);
                k.column({x, 0, z}, 1.45f, 1.4f, sandD, 12);              // plinth
                k.column({x, 1.4f, z}, 1.05f, 31.4f, sandD, 12, 0.92f);
                k.column({x, 32.8f, z}, 0.97f, 2.2f, sand, 12, 1.7f);      // capital
                g.column({x, 26.f, z}, 0.99f, 0.35f, gold, 12);
            }
        B.mat = Mat::METAL;
        for (float x : {-135.f, -165.f, -195.f}) {
            wall(x - 0.5f, 27.5f, -133, x + 0.5f, 28.5f, -104, {0.30f,0.27f,0.25f});
            neon(x - 0.15f, 27.4f, -133, x + 0.15f, 27.5f, -104, gold * 0.7f);
        }
        B.mat = Mat::BRICK;
        {
            ShapeKit k = B.kit(), g = B.kit(true);
            // Ribs vaulting over the hall from each pair of columns
            for (float x : {-120.f, -135.f, -150.f, -165.f, -180.f, -195.f}) {
                k.arch({x, 27.f, -118.5f}, 27.f, 7.4f, 0.8f, 1.2f, sand * 0.9f, 1.5708f, 18);
                g.arch({x, 26.8f, -118.5f}, 27.f, 7.4f, 0.12f, 0.4f, gold * 0.5f, 1.5708f, 18);
            }
            // The rose window over the top terrace: a wheel of tracery, glass glowing behind
            const vec3 RC{-200.8f, 25.5f, -118.5f};
            g.rod(RC - vec3{0.2f, 0, 0}, RC, 7.f, vec3{0.9f, 0.45f, 0.15f} * 0.5f, 24);
            for (float R : {7.f, 2.6f}) {
                k.arch(RC + vec3{0.25f, 0, 0}, 2.f * R, R, 0.5f, 0.45f, sand, 1.5708f, 20);
                k.arch(RC + vec3{0.25f, 0, 0}, 2.f * R, -R, 0.5f, 0.45f, sand, 1.5708f, 20);
            }
            for (int i = 0; i < 12; ++i) {
                float an = i * 0.5236f;
                vec3 d{0.f, std::sin(an), std::cos(an)};
                k.rod(RC + vec3{0.25f, 0, 0} + d * 2.6f, RC + vec3{0.25f, 0, 0} + d * 7.f, 0.18f, sand, 6);
                g.box(RC + vec3{0.1f, 0, 0} + d * 4.8f, {0.08f, 1.4f, 1.4f}, (i % 2 ? vec3{1.f, 0.25f, 0.15f} : vec3{0.3f, 0.5f, 1.f}) * 0.8f, 0.f, an);
            }
            // Two ring chandeliers on chains
            for (auto c : {vec3{-150.f, 22.f, -118.5f}, vec3{-180.f, 27.f, -118.5f}}) {
                B.mat = Mat::METAL;
                ShapeKit m = B.kit();
                m.curve(c, 5.f, 0.f, 6.2832f, 0.f, 0.35f, 0.3f, {0.25f, 0.2f, 0.15f}, 20);
                m.curve(c + vec3{0, -1.6f, 0}, 2.4f, 0.f, 6.2832f, 0.f, 0.3f, 0.25f, {0.25f, 0.2f, 0.15f}, 12);
                for (int i = 0; i < 4; ++i) {
                    float an = i * 1.5708f + 0.785f;
                    m.rod(c + vec3{std::cos(an) * 5.f, 0.3f, std::sin(an) * 5.f}, c + vec3{0, 6.f, 0}, 0.06f, {0.2f, 0.18f, 0.15f}, 4);
                }
                m.rod(c + vec3{0, 6.f, 0}, vec3{c.x, 35.f, c.z}, 0.1f, {0.2f, 0.18f, 0.15f}, 4);
                for (int i = 0; i < 16; ++i) {
                    float an = i * 0.3927f;
                    g.column(c + vec3{std::cos(an) * 5.f, 0.35f, std::sin(an) * 5.f}, 0.1f, 0.5f, vec3{1.f, 0.75f, 0.4f} * 1.2f, 5);
                }
                g.column(c + vec3{0, -3.6f, 0}, 0.5f, 2.f, gold, 6, 0.f);   // a crystal point beneath
                B.mat = Mat::BRICK;
            }
        }
        // Banners down the walls between the columns
        for (float x : {-127.5f, -142.5f, -157.5f, -172.5f, -187.5f}) {
            prop(x - 1.4f, 27.f, -132.9f, x + 1.4f, 34.f, -132.7f, {0.45f,0.12f,0.10f});
            prop(x - 1.4f, 27.f, -104.3f, x + 1.4f, 34.f, -104.1f, {0.45f,0.12f,0.10f});
            neon(x - 1.4f, 27.f, -132.7f, x + 1.4f, 27.2f, -132.6f, gold * 0.5f);
            neon(x - 1.4f, 27.f, -104.4f, x + 1.4f, 27.2f, -104.3f, gold * 0.5f);
        }
        // Cover on the terraces
        wall(-148,8,-112, -146,9.3f,-109, sandD); wall(-136,4,-124, -133,5.3f,-122, sandD);
        wall(-168,12,-122, -164,14.5f,-120, sandD); wall(-180,16,-118, -177,17.3f,-115, sandD);
        wall(-199,20,-121, -196,21.3f,-118, sandD); wall(-192,20,-110, -190,22,-107, sandD);
        place({-118.f, 0.f, -108.f}, 1); place({-118.f, 0.f, -128.f}, 0);
        a.waves = {
            {
                W{EnemyType::MITE, 0, {{-133,4,-110},{-135,4,-128},{-138,4,-117}}},
                W{EnemyType::HUSK, 0, {{-150,8,-109},{-152,8,-127},{-182,16,-110},{-196,20,-114}}},
                W{EnemyType::SENTINEL, 0, {{-161,12,-114},{-184,16,-126},{-198,20,-126}}},
                W{EnemyType::BRUTE, 0, {{-164,12,-109}}},
            },
            {
                W{EnemyType::RAPTOR, 0, {{-150,24,-118},{-180,24,-112}}},
                W{EnemyType::JUGGERNAUT, 0, {{-194,20,-114}}},
                W{EnemyType::HUSK, 0, {{-160,12,-128},{-176,16,-128}}},
                W{EnemyType::MITE, 0, {{-176,16,-110},{-180,16,-122},{-184,16,-114}}},
            },
        };
    }

    // =========================================================================
    // 4 THE SPAN — north at 20 m into an open canyon: an entry cliff, twin
    // bridges, an island, a gap (ferries, a beam, or grapple the canyon wall),
    // a bigger island with a sniper perch, a sweeper, the far cliff. Fall and
    // you're back on the entry cliff. Exit east from the far end.
    // =========================================================================
    {
        vec3 rock{0.46f,0.48f,0.56f}, rockD{0.30f,0.32f,0.40f}, ice{0.4f,0.9f,1.f}, panel{0.32f,0.35f,0.42f};
        Arena& a = section("THE SPAN", "MIND THE GAPS",
                           aabb(-215, 0, -272, -171, 44, -152), aabb(-215, 20, -272, -171, 38, -152),
                           {-190.5f, 20.f, -137.f}, -90.f, aabb(-215, -1, -272, -171, 70, -168), Ambient::WIND,
                           theme({0.12f,0.32f,0.70f}, {0.75f,0.86f,0.97f}, {0.30f,0.32f,0.40f}, {0.4f,0.5f,-1.f}, {1.6f,1.5f,1.3f}, 0.05f, 0.f,
                                 {0.42f,0.48f,0.62f}, 0.f, {-0.3f,-0.8f,0.5f}, {1.15f,1.1f,1.f},
                                 {0.46f,0.52f,0.68f}, {0.2f,0.2f,0.24f}, {0.68f,0.76f,0.88f}, 0.004f));
        a.voidY = 9.f;
        a.respawn = {-190.5f, 20.f, -158.f}; a.hasRespawn = true;
        AABB t3 = B.tube(2, -151.f, -134.f, -190.5f, 20.f, TW, TH, panel, ice);
        B.booster(aabb(-193, 20, -148, -188, 20 + TH, -138), {0, 0, -1}, BOOST);
        a.extraZones = {t3};

        // The canyon: rock walls 40 m high
        B.mat = Mat::ROCK;
        B.wallX(-216, -170, -152, -151, 0, 40, rock, {Gap{-193, -188, 20.f, 20.f + TH}});
        a.entryGate = B.doorway(true, -193, -188, -152, -151, 20.f, TH, ice);
        B.wallZ(-273, -151, -216, -215, 0, 40, rock);
        B.wallZ(-273, -151, -171, -170, 0, 40, rock, {Gap{-266, -261, 20.f, 20.f + TH}});
        a.exitDoor = B.doorway(false, -266, -261, -171, -170, 20.f, TH, ice, true);
        B.wallX(-216, -170, -273, -272, 0, 40, rock);
        // Ledges and outcrops on the canyon walls (grapple points)
        wall(-215,27,-216, -213,29,-212, rockD); wall(-173,27,-216, -171,29,-212, rockD);
        wall(-215,31,-185, -212,33,-180, rockD); wall(-174,31,-245, -171,33,-240, rockD);
        wall(-215,24,-255, -213,26,-250, rockD); wall(-173,24,-178, -171,26,-174, rockD);
        {
            ShapeKit k = B.kit(), g = B.kit(true);
            // Peaks beyond the walls: jagged, snow on top
            const float peaks[][4] = {{-236,-170,18,62},{-245,-215,26,74},{-232,-258,15,56},{-150,-190,17,60},{-146,-232,22,68},{-156,-150,12,50},
                                      {-262,-195,20,80},{-128,-210,18,72}};
            uint32_t sd = 3;
            for (auto& p : peaks) {
                k.rock({p[0], 0, p[1]}, p[2], p[3], {0.30f,0.33f,0.42f}, ++sd, 0.18f, 8);
                k.rock({p[0], p[3] * 0.72f, p[1]}, p[2] * 0.36f, p[3] * 0.32f, {0.85f,0.88f,0.95f}, ++sd, 0.15f, 8);
            }
            // The canyon's own walls: rock lumps clinging to them, spikes along the rim
            // Bold below the islands and above head height; slim where you walk
            for (float side : {-1.f, 1.f}) {
                float wx = side < 0 ? -215.f : -171.f;
                for (float z = -270.f; z < -153.f; z += 4.f) {
                    float v = (float)(((int)(-z * 37.f)) % 100) / 100.f;
                    float r = 3.f + v * 3.f;
                    k.rock({wx - side * r * 0.25f, -0.3f, z}, r, 13.f + v * 5.f, rockD * (0.85f + 0.2f * v), ++sd, 0.5f);
                    bool exit = side > 0 && z > -270.f && z < -257.f;
                    if (!exit) k.rock({wx - side * 0.2f, 18.f + v * 2.f, z + 1.f}, 1.6f + v, 9.f, rock * 0.9f, ++sd, 0.6f, 6);
                    float r2 = 2.5f + (1.f - v) * 3.f;
                    k.rock({wx - side * r2 * 0.3f, 27.f + v * 4.f, z + 2.f}, r2, 10.f + v * 6.f, rock * (0.85f + 0.15f * v), ++sd, 0.35f);
                    k.rock({wx + side * 1.2f, 38.f, z + 2.f}, 2.4f, 6.f + (float)(((int)(-z * 3.f)) % 13), rock, ++sd, 0.1f, 6);
                }
            }
            for (float x = -213.f; x < -172.f; x += 4.5f) {           // the far end wall
                float v = (float)(((int)(-x * 29.f)) % 100) / 100.f;
                k.rock({x, -0.3f, -272.f + 0.8f}, 3.f + 2.f * v, 14.f + 5.f * v, rockD, ++sd, 0.5f);
                k.rock({x + 1.f, 25.f + 4.f * v, -272.f + 0.6f}, 2.f + 2.f * v, 12.f, rock * 0.9f, ++sd, 0.35f);
                for (float z : {-272.f, -152.f}) k.rock({x + 2.f, 38.f, z + (z < -200.f ? -1.2f : 1.2f)}, 2.4f, 6.f + 9.f * v, rock, ++sd, 0.1f, 6);
            }
            // The islands stand on stalks of rock, and hang rock beneath
            for (float x : {-208.f, -178.f}) {
                k.rock({x, 0, -197.f}, 1.6f, 15.2f, rockD, ++sd, 2.1f);
                k.rock({x, 0, -233.5f}, 2.f, 15.2f, rockD, ++sd, 2.f);
            }
            const float under[][3] = {{-200,-196,6},{-186,-192,4},{-194,-202,9},{-205,-232,5},{-190,-238,8},{-182,-228,3},{-198,-226,7},{-212,-240,10}};
            for (auto& u : under) k.rock({u[0], u[2], u[1]}, 1.2f, 15.1f - u[2], rockD * 0.85f, ++sd, 4.2f, 7);
            // THE OLD BRIDGE: a viaduct that once crossed the canyon high
            // above, broken where it spans it; a fallen piece lies below
            B.mat = Mat::CONCRETE;
            ShapeKit b = B.kit();
            vec3 st{0.62f,0.62f,0.66f};
            const float BZ = -228.f, BY = 50.f;
            for (float x : {-330.f, -296.f, -262.f, -228.f, -158.f, -124.f, -90.f, -56.f}) {
                b.column({x, -0.3f, BZ}, 4.2f, BY - 1.f, st * 0.85f, 8, 0.8f);
                g.column({x, BY - 12.f, BZ}, 3.55f, 0.4f, ice * 0.6f, 8);
            }
            for (float x : {-313.f, -279.f, -245.f, -141.f, -107.f, -73.f}) {
                b.arch({x, BY - 18.f, BZ}, 27.f, 15.f, 2.2f, 7.f, st);
                b.box({x, BY, BZ}, {34.f, 2.4f, 8.f}, st * 0.9f);
                g.box({x, BY - 1.25f, BZ + 4.05f}, {34.f, 0.2f, 0.1f}, ice * 0.5f);
            }
            b.box({-213.f, BY, BZ}, {24.f, 2.4f, 8.f}, st * 0.9f, 0.f, 0.f, -0.12f);        // the broken ends
            b.box({-170.f, BY - 1.f, BZ}, {20.f, 2.4f, 8.f}, st * 0.9f, 0.f, 0.f, 0.2f);
            b.box({-196.f, 4.f, -213.f}, {22.f, 2.6f, 8.f}, st * 0.7f, 0.5f, 0.15f, 0.35f);  // fallen into the canyon
            b.box({-184.f, 2.f, -212.f}, {6.f, 6.f, 7.f}, st * 0.6f, 0.9f, 0.4f, 0.2f);
            // THE GATE: a ring the height of a mountain on the northern horizon
            const vec3 GC{-190.f, 60.f, -470.f};
            b.arch(GC, 170.f, 85.f, 9.f, 12.f, st * 0.8f, 0.f, 36);
            b.arch(GC, 170.f, -85.f, 9.f, 12.f, st * 0.8f, 0.f, 36);
            g.arch(GC + vec3{0, 0, 6.2f}, 152.f, 76.f, 1.2f, 0.4f, ice * 1.4f, 0.f, 36);
            g.arch(GC + vec3{0, 0, 6.2f}, 152.f, -76.f, 1.2f, 0.4f, ice * 1.4f, 0.f, 36);
            B.mat = Mat::ROCK;
        }
        // Entry cliff, twin bridges, island 1
        wall(-215,0,-166, -171,20,-152, rock);
        wall(-202,19.4f,-186, -198,20,-166, rockD);
        wall(-188,19.4f,-186, -184,20,-166, rockD);
        wall(-213,15,-206, -173,20,-186, rock);
        wall(-197,20,-198, -189,21.3f,-196, rockD);
        wall(-210,20,-192, -207,25,-189, rockD);
        // The first gap: a beam, two ferries
        wall(-202,19.5f,-221, -201,20,-206, rockD);
        B.mover({-193.f, 19.75f, -208.5f}, {2.f, 0.25f, 2.f}, Mover::Path::PINGPONG, {0,0,0}, {0,0,-10.f}, 5.f, 0.f, ice);
        B.mover({-185.f, 19.75f, -208.5f}, {2.f, 0.25f, 2.f}, Mover::Path::PINGPONG, {0,0,0}, {0,0,-10.f}, 5.f, 0.5f, ice);
        // Island 2 with its sniper perch
        wall(-214,15,-246, -172,20,-221, rock);
        wall(-197,20,-238, -189,23,-230, rockD);
        wall(-189,20,-234, -187,21.5f,-232, rockD);
        wall(-210,20,-228, -206,21.3f,-226, rockD); wall(-180,20,-240, -176,21.3f,-238, rockD);
        // The second gap: a sweeper and a narrow bridge, then the far cliff
        B.mover({-193.f, 19.75f, -252.f}, {3.f, 0.25f, 3.f}, Mover::Path::PINGPONG, {-10.f,0,0}, {10.f,0,0}, 7.f, 0.f, ice);
        wall(-178,19.4f,-258, -174,20,-246, rockD);
        wall(-215,0,-272, -171,20,-258, rock);
        // Pillars holding the islands up, glowing cliff edges
        for (float x : {-208.f, -178.f}) { hide(wall(x - 1.5f, 0, -199, x + 1.5f, 15, -195, rockD)); hide(wall(x - 2, 0, -236, x + 2, 15, -231, rockD)); }
        neon(-215,19.7f,-166.12f, -171,20,-166, ice);
        neon(-213,19.7f,-186.1f, -173,20,-185.98f, ice * 0.7f); neon(-213,19.7f,-206.02f, -173,20,-205.9f, ice * 0.7f);
        neon(-214,19.7f,-221.1f, -172,20,-220.98f, ice * 0.7f); neon(-214,19.7f,-246.02f, -172,20,-245.9f, ice * 0.7f);
        neon(-215,19.7f,-258.02f, -171,20,-257.9f, ice);
        B.mat = Mat::BRICK;
        place({-190.5f, 20.f, -158.f}, 1); place({-200.f, 20.f, -160.f}, 0);
        a.waves = {
            {
                W{EnemyType::HUSK, 0, {{-205,20,-190},{-181,20,-194},{-207,20,-224},{-177,20,-230}}},
                W{EnemyType::RIPPER, 0, {{-193,20,-190},{-185,20,-200}}},
                W{EnemyType::SENTINEL, 0, {{-193,23,-234},{-207,20,-266}}},
                W{EnemyType::BRUTE, 0, {{-193,20,-226}}},
                W{EnemyType::RAPTOR, 0, {{-193,28,-212},{-187,28,-240},{-199,28,-262}}},
            },
            {
                W{EnemyType::JUGGERNAUT, 0, {{-193,20,-266}}},
                W{EnemyType::HUSK, 0, {{-202,20,-240},{-182,20,-226}}},
                W{EnemyType::RAPTOR, 0, {{-200,30,-200},{-184,30,-250}}},
            },
        };
    }

    // =========================================================================
    // 5 THE WELL — a short tube east into a tower you enter near the top and
    // fall through: a balcony at 20, floors at 13 and 6.5 each with a hole in
    // a different corner, then the ground and the exit east.
    // =========================================================================
    {
        vec3 brick{0.40f,0.30f,0.30f}, brickD{0.26f,0.20f,0.21f}, steel{0.30f,0.32f,0.36f}, red{1.f,0.35f,0.25f},
             panel{0.34f,0.28f,0.30f};
        Arena& a = section("THE WELL", "DROP THROUGH THE FLOORS",
                           aabb(-160, 0, -283, -121, 34, -244), aabb(-160, 0, -283, -121, 30, -244),
                           {-167.5f, 20.f, -263.5f}, 0.f, aabb(-152, -1, -283, -121, 60, -244), Ambient::MOTES,
                           theme({0.05f,0.03f,0.06f}, {0.30f,0.12f,0.12f}, {0.06f,0.03f,0.04f}, {0.f,0.4f,-1.f}, {0.f,0.f,0.f}, 0.01f, 0.f,
                                 {0.08f,0.04f,0.05f}, 0.f, {0.3f,-1.f,0.25f}, {0.85f,0.62f,0.52f},
                                 {0.30f,0.22f,0.28f}, {0.40f,0.17f,0.11f}, {0.14f,0.06f,0.06f}, 0.010f));
        AABB t4 = B.tube(0, -170.f, -161.f, -263.5f, 20.f, TW, TH, panel, red);
        a.extraZones = {t4};

        B.wallZ(-284, -243, -161, -160, 0, 35, brick, {Gap{-266, -261, 20.f, 20.f + TH}});
        a.entryGate = B.doorway(false, -266, -261, -161, -160, 20.f, TH, red);
        B.wallZ(-284, -243, -121, -120, 0, 35, brick, {Gap{-266, -261, 0.f, TH}});
        a.exitDoor = B.doorway(false, -266, -261, -121, -120, 0.f, TH, red, true);
        B.wallX(-161, -120, -284, -283, 0, 35, brick);
        B.wallX(-161, -120, -244, -243, 0, 35, brick);
        wall(-161,34,-284, -120,35,-243, brickD);
        L.floors.push_back({-160.f, -283.f, -121.f, -244.f, 0.f, {0.22f,0.18f,0.18f}});
        B.mat = Mat::METAL;
        wall(-160,19.4f,-283, -152,20,-244, steel);                    // balcony
        wall(-152,12.4f,-283, -121,13,-256, steel);                    // floor 1, hole south-east
        wall(-152,12.4f,-256, -133,13,-244, steel);
        wall(-146,5.9f,-283, -121,6.5f,-268, steel);                   // floor 2, hole north-west
        wall(-160,5.9f,-268, -121,6.5f,-244, steel);
        B.mat = Mat::BRICK;
        hide(wall(-142,0,-266, -138,34,-262, brickD));                 // central column...
        {
            ShapeKit k = B.kit(), g = B.kit(true);
            const vec3 CC{-140.f, 0.f, -264.f};
            k.column(CC, 2.75f, 34.f, brickD, 14);                     // ...round, ringed, a stair of light wound up it
            for (float y : {28.f, 21.f, 14.f, 7.f}) g.column(CC + vec3{0, y, 0}, 2.8f, 0.25f, red * 0.8f, 14);
            g.helix(CC + vec3{0, 0.4f, 0}, 3.3f, 33.f, 3.5f, {0.25f, 0.1f, 0.9f}, red * 0.9f, 90);
            L.spinners.push_back({CC + vec3{0, 24.5f, 0}, 4.6f, 0.9f, 18, vec3{1.f, 0.45f, 0.3f}});
            L.spinners.push_back({CC + vec3{0, 9.8f, 0}, 4.6f, -1.2f, 18, vec3{1.f, 0.45f, 0.3f}});
            // The roof: ribs from the corners to an oculus burning over the shaft
            for (int i = 0; i < 8; ++i) {
                float an = i * 0.7854f;
                vec3 out{-140.5f + std::cos(an) * 19.f, 32.f, -263.5f + std::sin(an) * 19.f};
                k.rod(out, vec3{-140.5f, 33.8f, -263.5f} + vec3{std::cos(an), 0, std::sin(an)} * 3.4f, 0.4f, brickD * 0.8f, 6);
            }
            g.column({-140.5f, 33.85f, -263.5f}, 3.2f, 0.12f, vec3{1.f, 0.5f, 0.3f}, 16);
            k.curve({-140.5f, 0.f, -263.5f}, 3.5f, 0.f, 6.2832f, 33.4f, 34.f, 0.5f, brickD * 0.7f, 16);
            // Arched niches high on the long walls, lit from inside
            for (float x : {-151.f, -140.5f, -130.f})
                for (float z : {-282.9f, -244.1f}) {
                    float in = z < -260.f ? 0.06f : -0.06f;
                    g.box({x, 24.5f, z + in}, {2.6f, 6.f, 0.06f}, vec3{1.f, 0.45f, 0.3f} * 0.25f);
                    g.arch({x, 27.5f, z + in * 2.f}, 2.8f, 1.6f, 0.18f, 0.1f, red, 0.f, 10);
                    g.box({x - 1.4f, 24.5f, z + in * 2.f}, {0.18f, 6.f, 0.1f}, red);
                    g.box({x + 1.4f, 24.5f, z + in * 2.f}, {0.18f, 6.f, 0.1f}, red);
                }
        }
        for (float y : {19.6f, 12.6f, 6.1f}) {                         // floor edges glow
            neon(-160,y,-282.9f, -121,y + 0.25f,-282.75f, red * 0.8f);
            neon(-160,y,-244.25f, -121,y + 0.25f,-244.1f, red * 0.8f);
        }
        neon(-152.12f,19.4f,-283, -152.f,20.f,-244, red);
        neon(-133.1f,12.4f,-256, -133,13,-244, red); neon(-133,12.4f,-256.1f, -121,13,-256, red);
        neon(-146,5.9f,-283, -145.9f,6.5f,-268, red); neon(-160,5.9f,-268, -146,6.5f,-267.9f, red);
        for (float y : {3.f, 9.5f, 16.f, 25.f})                       // wall lamps on every floor
            for (float z : {-276.f, -251.f}) {
                neon(-120.95f - 0.07f, y, z - 0.6f, -120.95f, y + 1.4f, z + 0.6f, vec3{1.f,0.55f,0.35f} * 0.8f);
                neon(-160.05f, y, z - 0.6f, -159.98f, y + 1.4f, z + 0.6f, vec3{1.f,0.55f,0.35f} * 0.8f);
            }
        wall(-128,13,-276, -124,14.3f,-274, brickD);  wall(-150,6.5f,-252, -146,7.8f,-250, brickD);
        wall(-155,0,-250, -151,1.3f,-248, brickD);    wall(-127,0,-282, -123,1.3f,-280, brickD);
        // Chains and a hanging cage in the shaft; fans in the roof
        for (float x : {-148.f, -130.f}) prop(x - 0.08f, 22.f, -270.08f, x + 0.08f, 34.f, -269.92f, steel);
        {   // the cage: round, on a chain
            B.mat = Mat::METAL;
            ShapeKit k = B.kit(), g = B.kit(true);
            const vec3 C0{-148.f, 26.f, -256.f};
            for (int i = 0; i < 10; ++i) {
                float an = i * 0.6283f;
                k.rod(C0 + vec3{std::cos(an) * 1.9f, 0, std::sin(an) * 1.9f}, C0 + vec3{std::cos(an) * 1.9f, 4.f, std::sin(an) * 1.9f}, 0.06f, steel, 4);
            }
            k.column(C0 - vec3{0, 0.2f, 0}, 2.05f, 0.3f, steel * 0.6f, 12);
            k.dome(C0 + vec3{0, 4.f, 0}, 2.05f, steel * 0.6f, 3, 12);
            k.rod(C0 + vec3{0, 6.f, 0}, {C0.x, 34.f, C0.z}, 0.08f, steel, 4);
            g.column(C0, 0.6f, 1.2f, red * 0.7f, 6, 0.4f);
            B.mat = Mat::BRICK;
        }
        L.fans.push_back({{-130.f, 33.9f, -275.f}, 2.2f, 1, red});
        L.fans.push_back({{-150.f, 33.9f, -275.f}, 2.2f, 1, red});
        place({-156.f, 20.f, -250.f}, 1); place({-156.f, 20.f, -278.f}, 0);
        a.waves = {
            {
                W{EnemyType::HUSK, 0, {{-140,13,-275},{-146,13,-250},{-128,6.5f,-250}}},
                W{EnemyType::SENTINEL, 0, {{-125,13,-270}}},
                W{EnemyType::MITE, 0, {{-138,6.5f,-274},{-126,6.5f,-260},{-150,6.5f,-256}}},
                W{EnemyType::BRUTE, 0, {{-128,0,-272}}},
                W{EnemyType::RIPPER, 0, {{-150,0,-256},{-130,0,-252}}},
            },
            {
                W{EnemyType::JUGGERNAUT, 0, {{-135,0,-278}}},
                W{EnemyType::HUSK, 0, {{-128,13,-262},{-148,6.5f,-262}}},
                W{EnemyType::RAPTOR, 0, {{-140,24,-255}}},
                W{EnemyType::RIPPER, 0, {{-152,0,-274},{-126,0,-250}}},
            },
        };
    }

    // =========================================================================
    // 6 THE PUMPWORKS — east down a long boost tube (a bulkhead door halfway)
    // into a roofed hall: galleries along both walls, a pump block in the
    // middle, lava across the floor, and a control room behind each gallery
    // whose gunners shoot out through the windows.
    // =========================================================================
    {
        vec3 iron{0.34f,0.31f,0.30f}, rust{0.50f,0.28f,0.17f}, dark{0.18f,0.17f,0.18f}, orange{1.f,0.4f,0.08f},
             panel{0.33f,0.30f,0.28f}, screen{0.3f,1.f,0.6f};
        Arena& a = section("THE PUMPWORKS", "CLEAR THE HALL",
                           aabb(-79, 0, -283, -19, 14, -244), aabb(-79, 0, -296, -19, 13, -231),
                           {-117.5f, 0.f, -263.5f}, 0.f, aabb(-77, -1, -296, -19, 60, -231), Ambient::EMBERS,
                           theme({0.05f,0.03f,0.02f}, {0.32f,0.14f,0.05f}, {0.05f,0.02f,0.01f}, {0.f,0.3f,-1.f}, {0.f,0.f,0.f}, 0.01f, 0.f,
                                 {0.06f,0.03f,0.02f}, 0.f, {0.2f,-1.f,0.15f}, {0.6f,0.42f,0.3f},
                                 {0.12f,0.10f,0.10f}, {0.42f,0.17f,0.06f}, {0.16f,0.07f,0.03f}, 0.014f));
        AABB t5 = B.tube(0, -120.f, -80.f, -263.5f, 0.f, TW, TH, panel, orange);
        B.booster(aabb(-116, 0, -266, -104, TH, -261), {1, 0, 0}, BOOST);
        B.booster(aabb(-96, 0, -266, -84, TH, -261), {1, 0, 0}, BOOST);
        tubeDoor(-100.f, -263.5f, orange);
        place({-110.f, 0.f, -263.5f}, 1);

        // The hall
        B.mat = Mat::METAL;
        B.wallZ(-284, -243, -80, -79, 0, 15, iron, {Gap{-266, -261, 0.f, TH}});
        a.entryGate = B.doorway(false, -266, -261, -80, -79, 0.f, TH, orange);
        B.wallZ(-284, -243, -19, -18, 0, 15, iron, {Gap{-266, -261, 0.f, TH}});
        a.exitDoor = B.doorway(false, -266, -261, -19, -18, 0.f, TH, orange, true);
        std::vector<Gap> side = {{-52, -47, 5.f, 9.f}, {-58, -55, 6.5f, 9.f}, {-44, -41, 6.5f, 9.f}};
        B.wallX(-80, -18, -284, -283, 0, 15, iron, side);
        B.wallX(-80, -18, -244, -243, 0, 15, iron, side);
        B.doorway(true, -52, -47, -284, -283, 5.f, 4.f, screen);
        B.doorway(true, -52, -47, -244, -243, 5.f, 4.f, screen);
        wall(-80,14,-284, -18,15,-243, dark);
        L.floors.push_back({-79.f, -283.f, -19.f, -244.f, 0.f, {0.24f,0.22f,0.21f}});
        // Galleries along both long walls, pads up to them
        wall(-79,4.6f,-283, -19,5,-278, dark); wall(-79,4.6f,-249, -19,5,-244, dark);
        neon(-79,4.6f,-278.02f, -19,5,-277.9f, orange); neon(-79,4.6f,-249.1f, -19,5,-248.98f, orange);
        pad({-30.f, 0.f, -275.f}, {0.f, 17.f, -6.f}); pad({-30.f, 0.f, -252.f}, {0.f, 17.f, 6.f});
        pad({-72.f, 0.f, -275.f}, {0.f, 17.f, -6.f}); pad({-72.f, 0.f, -252.f}, {0.f, 17.f, 6.f});
        // The pump block (a pad onto it), lava across the floor, pillars, crates
        hide(wall(-56,0,-270, -42,6,-257, rust));
        {   // ...two great tanks side by side, a deck across them, saddles beneath
            ShapeKit k = B.kit(), g = B.kit(true);
            for (float z : {-266.75f, -260.25f}) {
                k.rod({-56.f, 2.75f, z}, {-42.f, 2.75f, z}, 3.2f, rust, 14);
                for (float x : {-55.f, -49.f, -43.f}) g.rod({x - 0.2f, 2.75f, z}, {x + 0.2f, 2.75f, z}, 3.25f, orange * 0.7f, 14);
                for (float x : {-53.f, -45.f}) k.box({x, 0.6f, z}, {1.2f, 1.2f, 6.f}, dark);
            }
            k.box({-49.f, 5.85f, -263.5f}, {14.f, 0.3f, 13.f}, dark);
            g.box({-49.f, 5.72f, -263.5f}, {14.1f, 0.06f, 13.1f}, orange * 0.6f);
        }
        pad({-59.f, 0.f, -263.5f}, {4.f, 19.f, 0.f});
        L.hazards.push_back({aabb(-70, 0, -278, -67, 0.06f, -249), 30.f});
        neon(-70, 0, -278, -67, 0.06f, -249, {1.f,0.24f,0.02f});
        for (float x : {-65.f, -33.f}) for (float z : {-272.f, -255.f}) {
            hide(wall(x - 1,0,z - 1, x + 1,14,z + 1, iron));
            ShapeKit k = B.kit(), g = B.kit(true);
            k.column({x, 0, z}, 1.4f, 1.f, dark, 8);
            k.column({x, 1.f, z}, 1.05f, 12.f, iron, 8);
            k.column({x, 13.f, z}, 1.05f, 1.f, dark, 8, 1.4f);
            g.column({x, 9.f, z}, 1.09f, 0.3f, orange, 8);
        }
        {   // Pipes arching over the hall at both ends, and pistons pounding in the roof
            ShapeKit k = B.kit(), g = B.kit(true);
            for (float x : {-76.f, -22.f}) for (float dz : {-0.9f, 0.9f}) {
                k.arch({x + dz, 6.f, -263.5f}, 33.f, 7.f, 0.9f, 0.9f, rust * 0.8f, 1.5708f, 16);
                g.arch({x + dz, 6.f, -263.5f}, 33.f, 7.f, 0.95f, 0.2f, orange * 0.35f, 1.5708f, 16);
            }
            for (float x : {-70.f, -60.f, -38.f, -28.f}) for (float z : {-270.5f, -256.5f}) {
                float ph = (x + z) * 0.037f;
                L.pistons.push_back({{x, 14.f, z}, 0.9f, 0.4f, 3.4f, 2.6f, ph, orange});
            }
        }
        wall(-28,0,-262, -25,1.4f,-259, rust); wall(-38,0,-266, -35,1.4f,-263, rust); wall(-61,0,-251, -58,1.4f,-248, rust);
        // A crane sweeping the length of the hall under the roof
        B.mover({-49.f, 11.25f, -263.5f}, {2.5f, 0.25f, 1.5f}, Mover::Path::PINGPONG, {-24.f,0,0}, {22.f,0,0}, 12.f, 0.f, orange);
        prop(-75,13.4f,-264, -25,14,-263, dark);
        L.fans.push_back({{-78.7f, 10.f, -255.f}, 1.6f, 0, orange});
        L.fans.push_back({{-78.7f, 10.f, -272.f}, 1.6f, 0, orange});
        // Control rooms behind both galleries: a floor at 5, consoles, screens
        for (int s : {-1, 1}) {
            float zin = s < 0 ? -284.f : -243.f, zout = s < 0 ? -296.f : -231.f;
            float lo = std::min(zin, zout), hi = std::max(zin, zout);
            wall(-60, 0, lo, -40, 5, hi, iron);                        // floor block
            B.wallX(-61, -39, s < 0 ? zout - 1 : zout, s < 0 ? zout : zout + 1, 0, 12, iron);
            B.wallZ(lo - (s < 0 ? 1 : 0), hi + (s > 0 ? 1 : 0), -61, -60, 0, 12, iron);
            B.wallZ(lo - (s < 0 ? 1 : 0), hi + (s > 0 ? 1 : 0), -40, -39, 0, 12, iron);
            wall(-61, 11, lo - (s < 0 ? 1 : 0), -39, 12, hi + (s > 0 ? 1 : 0), dark);
            float zc = s < 0 ? zout + 1.f : zout - 2.2f;               // consoles along the back wall
            for (float x : {-57.f, -50.f, -43.f}) {
                wall(x - 2, 5, std::min(zc, zc + 1.2f), x + 2, 6.2f, std::max(zc, zc + 1.2f), dark);
                float zs = s < 0 ? zout + 0.02f : zout - 0.09f;
                neon(x - 1.8f, 6.6f, zs, x + 1.8f, 8.4f, zs + 0.07f, screen * 0.45f);
            }
            neon(-60, 10.9f, lo + 1, -40, 11, hi - 1, vec3{0.8f,1.f,0.9f} * 0.35f);
        }
        B.mat = Mat::BRICK;
        a.extraZones = {t5, aabb(-60, 5, -296, -40, 11, -283), aabb(-60, 5, -244, -40, 11, -231)};
        a.waves = {
            {
                W{EnemyType::RIPPER, 0, {{-60,0,-268},{-64,0,-259},{-30,0,-250},{-26,0,-276}}},
                W{EnemyType::HUSK, 0, {{-50,5,-290},{-45,5,-236},{-35,5,-281},{-35,5,-246}}},
                W{EnemyType::BRUTE, 0, {{-49,6,-263.5f}}},
                W{EnemyType::MITE, 0, {{-40,0,-252},{-38,0,-275},{-24,0,-263}}},
            },
            {
                W{EnemyType::JUGGERNAUT, 0, {{-28,0,-268}}},
                W{EnemyType::SENTINEL, 0, {{-56,5,-290},{-44,5,-236}}},
                W{EnemyType::RAPTOR, 0, {{-62,9,-263}}},
                W{EnemyType::MITE, 0, {{-62,0,-276},{-62,0,-250},{-40,0,-276}}},
                W{EnemyType::HUSK, 0, {{-70,5,-280},{-70,5,-247}}},
            },
        };
    }

    // =========================================================================
    // 7 THE TOWER — the finale. Down the last tube into a courtyard with
    // corner bastions and a 34 m tower: win two waves, then ride the lifts,
    // the lift shaft or the pads up to the beacon on top.
    // =========================================================================
    {
        vec3 slate{0.26f,0.26f,0.34f}, slateD{0.16f,0.16f,0.22f}, mag{1.f,0.25f,0.75f}, cyan{0.25f,0.9f,1.f},
             panel{0.28f,0.27f,0.36f};
        const float TX = 56.f, TZ = -263.5f;
        Arena& a = section("THE TOWER", "WIN THE YARD - THEN CLIMB",
                           aabb(21, 0, -299, 91, 42, -228), aabb(21, 0, -299, 91, 32, -228),
                           {-15.5f, 0.f, -263.5f}, 0.f, aabb(28, -1, -299, 91, 60, -228), Ambient::ASH,
                           theme({0.03f,0.01f,0.06f}, {0.36f,0.10f,0.30f}, {0.06f,0.02f,0.06f}, {-0.3f,0.25f,-1.f}, {1.6f,0.4f,1.1f}, 0.12f, 1.f,
                                 {0.10f,0.03f,0.10f}, 0.8f, {0.3f,-0.6f,0.7f}, {0.75f,0.55f,0.85f},
                                 {0.22f,0.14f,0.30f}, {0.10f,0.05f,0.10f}, {0.16f,0.05f,0.16f}, 0.008f));
        AABB t6 = B.tube(0, -18.f, 21.f, -263.5f, 0.f, TW, TH, panel, mag);
        B.booster(aabb(-13, 0, -266, -3, TH, -261), {1, 0, 0}, BOOST);
        B.booster(aabb(5, 0, -266, 15, TH, -261), {1, 0, 0}, BOOST);
        tubeDoor(1.f, -263.5f, mag);
        place({-9.f, 0.f, -263.5f}, 1);
        a.extraZones = {t6};

        L.floors.push_back({21.f, -299.f, 91.f, -228.f, 0.f, {0.24f,0.22f,0.28f}});
        B.wallZ(-300, -227, 20, 21, 0, 12, slate, {Gap{-266, -261, 0.f, TH}});
        a.entryGate = B.doorway(false, -266, -261, 20, 21, 0.f, TH, mag);
        B.wallZ(-300, -227, 91, 92, 0, 12, slate);
        B.wallX(20, 92, -300, -299, 0, 12, slate);
        B.wallX(20, 92, -228, -227, 0, 12, slate);
        neon(21,11.4f,-299, 21.1f,11.7f,-228, mag); neon(90.9f,11.4f,-299, 91,11.7f,-228, mag);
        neon(21,11.4f,-298.95f, 91,11.7f,-298.85f, mag); neon(21,11.4f,-228.15f, 91,11.7f,-228.05f, mag);
        // Corner bastions and pads up to them
        wall(21,0,-299, 34,5,-287, slateD); wall(78,0,-299, 91,5,-287, slateD);
        wall(21,0,-240, 34,5,-228, slateD); wall(78,0,-240, 91,5,-228, slateD);
        pad({37.f, 0.f, -293.f}, {-5.f, 17.f, 0.f}); pad({75.f, 0.f, -293.f}, {5.f, 17.f, 0.f});
        pad({37.f, 0.f, -234.f}, {-5.f, 17.f, 0.f}); pad({75.f, 0.f, -234.f}, {5.f, 17.f, 0.f});
        // The tower, two balcony rings, lifts between them, pads to the top
        // An octagonal keep (its collision a cross plus four corner blocks)
        hide(wall(TX - 6, 0, TZ - 2.5f, TX + 6, 34, TZ + 2.5f, slateD));
        hide(wall(TX - 2.5f, 0, TZ - 6, TX + 2.5f, 34, TZ + 6, slateD));
        for (float sx : {-1.f, 1.f}) for (float sz : {-1.f, 1.f})
            hide(wall(TX + sx * 2.5f, 0, TZ + sz * 2.5f, TX + sx * 4.2f, 34, TZ + sz * 4.2f, slateD));
        {
            ShapeKit k = B.kit(), g = B.kit(true);
            glm::mat4 oct = ShapeKit::R(0.3927f, {0, 1, 0});
            const float RO = 6.f / std::cos(0.3927f);
            k.column({TX, 0, TZ}, RO + 0.5f, 1.5f, slate, 8).xf *= oct;
            k.column({TX, 0, TZ}, RO, 34.f, slateD, 8).xf *= oct;
            k.column({TX, 32.6f, TZ}, RO + 0.55f, 1.4f, slate, 8).xf *= oct;             // the crown's cornice
            for (float y : {6.f, 18.f, 30.f}) g.column({TX, y, TZ}, RO + 0.06f, 0.2f, mag * 0.6f, 8).xf *= oct;
            g.column({TX, 33.6f, TZ}, RO + 0.6f, 0.4f, mag, 8).xf *= oct;
            for (int i = 0; i < 8; ++i) {                                                // finials
                float an = i * 0.7854f;
                vec3 p{TX + std::cos(an) * (RO + 0.2f), 34.f, TZ + std::sin(an) * (RO + 0.2f)};
                k.column(p, 0.45f, 5.f, slate, 4, 0.1f);
                g.column(p + vec3{0, 4.6f, 0}, 0.2f, 0.9f, mag, 4, 0.f);
                // tall slit windows down each face
                float fa = an + 0.3927f;
                g.box({TX + std::cos(fa) * 6.02f, 20.f, TZ + std::sin(fa) * 6.02f}, {0.06f, 22.f, 0.7f}, cyan * 0.35f, -fa);
            }
            L.spinners.push_back({{TX, 36.5f, TZ}, 4.f, 0.8f, 16, mag * 1.2f});
            // Turrets on the bastions' outer corners, battlements on the walls
            for (float cx : {24.5f, 87.5f}) for (float cz : {-295.5f, -231.5f}) {
                k.column({cx, 5.f, cz}, 2.4f, 5.f, slate, 10);
                k.column({cx, 10.f, cz}, 2.9f, 4.f, slateD, 10, 0.f);
                g.column({cx, 8.f, cz}, 2.45f, 0.25f, mag * 0.8f, 10);
            }
            for (float x = 23.f; x < 90.f; x += 3.f)
                for (float z : {-299.5f, -227.5f}) k.box({x, 12.6f, z}, {1.5f, 1.2f, 1.f}, slate);
            for (float z = -297.f; z < -229.f; z += 3.f)
                for (float x : {20.5f, 91.5f}) if (z < -267.f || z > -260.f || x > 50.f) k.box({x, 12.6f, z}, {1.f, 1.2f, 1.5f}, slate);
        }
        for (float top : {12.f, 24.f}) {
            wall(TX - 10, top - 0.5f, TZ - 10, TX + 10, top, TZ - 6, slate);
            wall(TX - 10, top - 0.5f, TZ + 6,  TX + 10, top, TZ + 10, slate);
            wall(TX - 10, top - 0.5f, TZ - 6,  TX - 6,  top, TZ + 6, slate);
            wall(TX + 6,  top - 0.5f, TZ - 6,  TX + 10, top, TZ + 6, slate);
            B.ring(TX - 10, TZ - 10, TX + 10, TZ + 10, top - 0.5f, top - 0.2f, cyan);
        }
        B.mover({TX - 12.f, 0.25f, TZ}, {2.f, 0.25f, 2.f}, Mover::Path::PINGPONG, {0,0,0}, {0, 11.75f, 0}, 7.f, 0.f, cyan);
        B.mover({TX + 12.f, 11.75f, TZ}, {2.f, 0.25f, 2.f}, Mover::Path::PINGPONG, {0,0,0}, {0, 12.f, 0}, 7.f, 0.5f, cyan);
        pad({TX, 24.f, TZ - 8.f}, {0.f, 26.f,  2.f});   // steep: clear the tower's edge first
        pad({TX, 24.f, TZ + 8.f}, {0.f, 26.f, -2.f});
        B.mover({TX, 18.25f, TZ}, {1.8f, 0.25f, 1.8f}, Mover::Path::ORBIT, {15.f,0,0}, {0,0,15.f}, 14.f, 0.f, mag);
        // A lift shaft from the yard straight up to the top balcony: ride it
        // up, and a gust at the top blows you onto the balcony
        const float SX = TX, SZ = TZ + 20.f;
        B.mat = Mat::PANEL;
        for (float dx : {-2.2f, 1.8f}) for (float dz : {-2.2f, 1.8f})
            wall(SX + dx, 0, SZ + dz, SX + dx + 0.4f, 27.5f, SZ + dz + 0.4f, panel);
        B.mat = Mat::BRICK;
        B.ring(SX - 2.2f, SZ - 2.2f, SX + 2.2f, SZ + 2.2f, 27.3f, 27.5f, cyan);
        B.booster(aabb(SX - 1.8f, 0, SZ - 1.8f, SX + 1.8f, 27.f, SZ + 1.8f), {0, 1, 0}, 20.f);
        B.booster(aabb(SX - 2.5f, 27.f, SZ - 2.5f, SX + 2.5f, 33.f, SZ + 2.5f), {0, 0, -1}, 9.f);
        // Cover in the yard
        wall(40,0,-282, 44,1.3f,-280, slateD); wall(68,0,-247, 72,1.3f,-245, slateD);
        wall(40,0,-247, 42,1.6f,-243, slateD); wall(70,0,-284, 72,1.6f,-280, slateD);
        wall(81,0,-267, 83,1.3f,-259, slateD);
        L.finishPos = {TX, 34.f, TZ};
        L.gems.push_back({{TX, 37.5f, TZ}, {1.8f, 0.5f, 1.4f}, 1.3f, true});
        a.waves = {
            {
                W{EnemyType::HUSK, 0, {{45,0,-293},{67,0,-293},{45,0,-234},{67,0,-234}}},
                W{EnemyType::RIPPER, 0, {{85,0,-255},{80,0,-280},{80,0,-246}}},
                W{EnemyType::SENTINEL, 0, {{85,5,-293},{85,5,-233}}},
                W{EnemyType::RAPTOR, 0, {{56,20,-284},{40,20,-263}}},
            },
            {
                W{EnemyType::BRUTE, 0, {{65,0,-292}}},
                W{EnemyType::JUGGERNAUT, 0, {{85,0,-273}}},
                W{EnemyType::MITE, 0, {{81,0,-283},{81,0,-244},{70,0,-236},{70,0,-291}}},
                W{EnemyType::HUSK, 0, {{TX,12,TZ - 8},{TX,12,TZ + 8}}},
                W{EnemyType::SENTINEL, 0, {{28,5,-293},{28,5,-233}}},
            },
        };
    }

    // Lighting crossfades down each tube
    L.blends = {
        {aabb(-56, 0, -121, -16, 60, -116),   0, 1, 0, true},
        {aabb(-112, 0, -121, -82, 60, -116),  1, 2, 0, true},
        {aabb(-193, 0, -151, -188, 60, -134), 2, 3, 2, true},
        {aabb(-170, 0, -266, -161, 60, -261), 3, 4, 0, false},
        {aabb(-120, 0, -266, -80, 60, -261),  4, 5, 0, false},
        {aabb(-18, 0, -266, 21, 60, -261),    5, 6, 0, false},
    };

    // Par times (seconds): S, A, B, C
    L.parTimes[0] = 300.f; L.parTimes[1] = 420.f; L.parTimes[2] = 570.f; L.parTimes[3] = 780.f;
    return L;
}
