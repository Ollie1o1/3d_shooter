#pragma once
// =============================================================================
// LevelStory.h — STORY mode's levels. CHAPTER 1: THE CRADLE (the creator's
// factory, climbed from the bottom up and out). Its upper six rooms are in
// LevelStoryChapter.h.
//
// THE FURNACE LINE: where UNIT 24 wakes, on his back on a conveyor carrying
// rejected bodies into a furnace. Three belts run north (-Z) down a long dark
// hall under gantry beams, out of a hatch in the south wall and off a lip into
// the furnace pit; the furnace's mouth fills the north wall. Off the belts, the
// hall's floor; in its east corner a chute drops away (the way on: the Scrap
// Pit).
//
//         z -76 ┌───────────────────────┐
//               │   FURNACE (molten)    │
//         z -63 └──┬─── mouth ──────┬───┘
//               ░░░│░░ the pit ░░░░░│░░░   (falls in: burned)
//         z -56 ═══╪═════╪═════╪════╪═══   the belts end; a rail between them
//               │  ║     ║     ║    │  │
//               │  ║     ║     ║  [chute] x 10..13.5, z -34..-29
//               │  ║     ║     ║    │  │
//               │  ║     ║(you)║    │  │   gantry beams every 4 m overhead
//          z +9 └──╨─────╨─────╨────┘──┘   the hatch the belts come out of
//               x -15   -7    0    7    15
//
// The whole level stands on slabs: the hard floor is far below (a basin at
// -40), so the pit and the chute are real holes. Sections are Arenas run by
// WaveDirector with fast = true (each fight starts at its trigger); the
// furnace line has no fight. No OpenGL.
// =============================================================================
#include "world/Level.h"
#include "core/StoryText.h"
#include "world/LevelStoryChapter.h"

inline LevelData buildCradle() {
    using glm::vec3;
    LevelData L;
    LevelBuilder B{L};
    auto wall = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { return B.wall(x0,y0,z0,x1,y1,z1,c); };
    auto prop = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { return B.prop(x0,y0,z0,x1,y1,z1,c); };
    auto neon = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { B.neon(x0,y0,z0,x1,y1,z1,c); };

    // ---- the palette: dark steel and concrete, hazard yellow, the furnace's light ----
    const vec3 steel{0.22f, 0.23f, 0.26f}, steel2{0.30f, 0.31f, 0.34f}, dark{0.11f, 0.11f, 0.13f}, rubber{0.07f, 0.07f, 0.08f},
               conc{0.34f, 0.33f, 0.32f}, conc2{0.26f, 0.25f, 0.25f}, rust{0.36f, 0.20f, 0.12f}, hazard{0.78f, 0.60f, 0.08f},
               brick{0.30f, 0.17f, 0.12f};
    const vec3 fire{2.4f, 0.85f, 0.18f}, fireHot{3.2f, 1.9f, 0.7f}, ember{1.4f, 0.35f, 0.06f}, lamp{1.2f, 1.0f, 0.75f},
               cyan{0.2f, 0.85f, 1.f}, red{1.6f, 0.12f, 0.08f};

    // ---- the frame of the hall ----
    const float X0 = -15.f, X1 = 15.f, ZS = 9.f, ZN = -56.f, ZF = -63.5f, ZB = -76.f, ROOF = 13.f, PIT = -8.4f;
    const float LANES[3] = {-7.f, 0.f, 7.f}, BW = 1.3f;   // the belts' centres, half their width
    const float BELT_TOP = 1.1f, BELT_S = 12.f, BELT_N = -56.2f, BELT_SPEED = 1.4f;
    const AABB CHUTE = LevelBuilder::aabb(10.f, -36.f, -34.f, 13.5f, 0.f, -29.f);

    // The hard floor, far below everything: every floor here is a slab, so the pit and the chute are holes
    L.basins.push_back({LevelBuilder::aabb(-80.f, 0.f, -140.f, 120.f, 0.f, 105.f), -40.f});

    const LevelBuilder::Mark m0 = B.mark();
    B.mat = Mat::CONCRETE;
    // The floor: slabs round the chute's hole
    wall(X0, -0.6f, ZN, 10.f, 0.f, ZS + 4.f, conc);
    wall(10.f, -0.6f, CHUTE.max.z, X1, 0.f, ZS + 4.f, conc);
    wall(10.f, -0.6f, ZN, X1, 0.f, CHUTE.min.z, conc);
    wall(CHUTE.max.x, -0.6f, CHUTE.min.z, X1, 0.f, CHUTE.max.z, conc);
    // Walls, west and east, from the pit's floor to the roof; the roof
    B.mat = Mat::PANEL;
    wall(X0 - 1.f, PIT, ZB, X0, ROOF, ZS + 5.f, steel);
    wall(X1, PIT, ZB, X1 + 1.f, ROOF, ZS + 5.f, steel);
    B.mat = Mat::METAL;
    wall(X0 - 1.f, ROOF, ZB, X1 + 1.f, ROOF + 0.8f, ZS + 5.f, dark);
    // Pilasters down both walls, every 8 m, and a skirting of rusted plate
    for (float z = 5.f; z > ZN; z -= 8.f)
        for (float sx : {X0, X1}) {
            float in = sx < 0.f ? 1.f : -1.f;
            prop(sx, 0.f, z - 0.5f, sx + in * 0.6f, ROOF, z + 0.5f, steel2);
            prop(sx, 0.f, z - 0.7f, sx + in * 0.8f, 0.5f, z + 0.7f, dark);
        }
    for (float sx : {X0, X1}) prop(sx, 0.f, ZN, sx + (sx < 0.f ? 0.15f : -0.15f), 1.2f, ZS, rust);

    // ---- the south wall: three hatches the belts come out of, a dark tunnel behind ----
    B.mat = Mat::PANEL;
    // (HATCH: tall enough to stand up in on the belt, 1.1 m up: you wake in there and ride out)
    const float HATCH = 3.6f;
    B.wallX(X0, X1, ZS, ZS + 0.6f, 0.f, ROOF, steel, {{LANES[0] - 1.5f, LANES[0] + 1.5f, 0.f, HATCH},
                                                      {LANES[1] - 1.5f, LANES[1] + 1.5f, 0.f, HATCH},
                                                      {LANES[2] - 1.5f, LANES[2] + 1.5f, 0.f, HATCH}});
    wall(X0, 0.f, ZS + 4.f, X1, HATCH + 0.4f, ZS + 4.6f, dark);   // the tunnel's back
    wall(X0, HATCH, ZS + 0.6f, X1, HATCH + 0.4f, ZS + 4.f, dark); // its roof
    for (float lx : LANES) {
        neon(lx - 1.55f, HATCH, ZS - 0.02f, lx + 1.55f, HATCH + 0.15f, ZS, {1.4f, 0.9f, 0.1f});   // a lit lintel over each hatch
        neon(lx - 0.25f, HATCH + 0.5f, ZS - 0.04f, lx + 0.25f, HATCH + 0.8f, ZS, red);                    // and its lamp
        // the flaps of the hatch: strips of rubber hanging in the opening, pushed aside where the belt runs
        for (int k = 0; k < 6; ++k) {
            float fx = lx - 1.45f + k * 0.58f;
            if (std::fabs(fx + 0.24f - lx) < 1.0f) continue;
            prop(fx, 1.3f, ZS + 0.1f, fx + 0.48f, HATCH, ZS + 0.2f, rubber);
        }
    }

    // ---- the belts: a steel frame, the belt itself (a conveyor), rails, the end rollers over the pit ----
    for (int i = 0; i < 3; ++i) {
        float cx = LANES[i];
        B.mat = Mat::METAL;
        wall(cx - BW - 0.1f, 0.f, BELT_N + 0.2f, cx + BW + 0.1f, BELT_TOP - 0.15f, BELT_S, dark);
        B.mat = Mat::PANEL;
        int belt = wall(cx - BW, BELT_TOP - 0.15f, BELT_N, cx + BW, BELT_TOP, BELT_S, rubber);
        L.conveyors.push_back({belt, {0.f, 0.f, -1.f}, BELT_SPEED});
        B.mat = Mat::METAL;
        for (float s : {-1.f, 1.f}) {
            prop(cx + s * (BW + 0.02f), BELT_TOP - 0.1f, BELT_N, cx + s * (BW + 0.16f), BELT_TOP + 0.18f, BELT_S, steel2);   // side rail
            for (float z = BELT_S - 1.f; z > BELT_N; z -= 3.f)                                                                // its feet
                prop(cx + s * (BW - 0.1f), 0.f, z - 0.15f, cx + s * (BW + 0.1f), BELT_TOP - 0.15f, z + 0.15f, steel);
        }
        B.kit().rod({cx - BW, BELT_TOP - 0.25f, BELT_N + 0.05f}, {cx + BW, BELT_TOP - 0.25f, BELT_N + 0.05f}, 0.25f, steel2, 12);   // the end roller
        neon(cx - BW, BELT_TOP - 0.02f, BELT_N - 0.08f, cx + BW, BELT_TOP + 0.02f, BELT_N, {1.3f, 0.8f, 0.1f});              // the lip, lit
        // hazard chevrons painted down the belt's last 4 m (props just over it)
        for (int k = 0; k < 4; ++k) {
            float z = BELT_N + 0.6f + k * 1.0f;
            prop(cx - BW + 0.05f, BELT_TOP, z, cx + BW - 0.05f, BELT_TOP + 0.005f, z + 0.35f, hazard);
        }
    }
    // Between the belts and along the walls: a guard rail at the pit's edge (the only way in is to ride)
    B.mat = Mat::METAL;
    for (auto [a, b] : {std::pair{X0, LANES[0] - BW - 0.1f}, {LANES[0] + BW + 0.1f, LANES[1] - BW - 0.1f},
                        {LANES[1] + BW + 0.1f, LANES[2] - BW - 0.1f}, {LANES[2] + BW + 0.1f, X1}}) {
        wall(a, 0.f, ZN - 0.25f, b, 1.1f, ZN, hazard);
        prop(a, 0.f, ZN, b, 0.02f, ZN + 0.4f, hazard);   // a painted line before it
    }

    // ---- the pit: a sheer drop to molten slag; the furnace's mouth fills the north wall ----
    B.mat = Mat::BASALT;
    wall(X0, PIT - 0.6f, ZB, X1, PIT, ZN, brick);                          // the pit's floor (under the slag)
    wall(X0, PIT, ZN - 0.01f, X1, -0.6f, ZN + 0.6f, brick);                 // its south face, under the floor slab
    neon(X0, PIT, ZF, X1, PIT + 0.15f, ZN, ember);                          // slag across the pit
    for (int k = 0; k < 7; ++k) {                                           // crusts floating on it
        float x = X0 + 2.f + k * 4.2f, z = ZN - 1.5f - (k % 3) * 1.7f;
        prop(x, PIT + 0.1f, z, x + 1.4f + (k % 2), PIT + 0.32f, z + 0.9f, {0.12f, 0.06f, 0.04f});
    }
    L.hazards.push_back({LevelBuilder::aabb(X0, PIT, ZB, X1, -4.5f, ZN), 400.f});
    // The furnace wall: brick, its mouth 20 m wide, a steel lintel and a ring of lit rivets
    B.mat = Mat::BRICK;
    B.wallX(X0, X1, ZF, ZF + 1.5f, PIT, ROOF, brick, {{-10.f, 10.f, PIT, 6.5f}});
    B.mat = Mat::METAL;
    prop(-10.6f, 6.5f, ZF + 1.5f, 10.6f, 7.4f, ZF + 1.8f, dark);
    for (int k = 0; k < 11; ++k) neon(-10.f + k * 2.f - 0.1f, 6.9f, ZF + 1.8f, -10.f + k * 2.f + 0.1f, 7.1f, ZF + 1.85f, ember);
    for (float s : {-1.f, 1.f}) prop(s * 10.f - 0.4f, PIT, ZF + 1.5f, s * 10.f + 0.4f, 6.5f, ZF + 1.8f, dark);   // its jambs
    // Inside the furnace: walls that glow, a white heart at the back, molten floor
    B.mat = Mat::BRICK;
    wall(-11.f, PIT, ZB - 1.f, 11.f, 9.f, ZB, brick);
    wall(-11.f, PIT, ZB, -10.f, 9.f, ZF, brick);
    wall(10.f, PIT, ZB, 11.f, 9.f, ZF, brick);
    wall(-11.f, 8.f, ZB, 11.f, 9.f, ZF, brick);
    neon(-9.8f, PIT, ZB + 0.01f, 9.8f, 7.8f, ZB + 0.15f, fire);
    neon(-5.f, PIT, ZB + 0.15f, 5.f, 3.5f, ZB + 0.3f, fireHot);
    neon(-9.9f, PIT, ZB, -9.8f, 7.8f, ZF, fire * 0.7f);
    neon(9.8f, PIT, ZB, 9.9f, 7.8f, ZF, fire * 0.7f);
    neon(-9.8f, 7.8f, ZB, 9.8f, 7.9f, ZF, ember);

    // ---- overhead: gantry beams every 4 m (you see them pass, lying on the belt), lamps, chains ----
    B.mat = Mat::METAL;
    for (int k = 0; k * 4.f < ZS - ZN - 2.f; ++k) {
        float z = ZS - 3.f - k * 4.f;
        prop(X0, 7.6f, z - 0.25f, X1, 8.2f, z + 0.25f, steel);
        prop(X0, 8.2f, z - 0.08f, X1, 8.9f, z + 0.08f, dark);                 // its web
        if (k % 2 == 0)
            for (float lx : LANES) {                                          // a lamp over each belt
                prop(lx - 0.5f, 7.35f, z - 0.2f, lx + 0.5f, 7.6f, z + 0.2f, dark);
                neon(lx - 0.42f, 7.3f, z - 0.14f, lx + 0.42f, 7.35f, z + 0.14f, lamp * (k % 4 == 0 ? 1.f : 0.6f));
            }
        else if (k % 3 == 1)
            for (float s : {-1.f, 1.f}) prop(s * 11.f - 0.03f, 4.6f, z - 0.03f, s * 11.f + 0.03f, 7.6f, z + 0.03f, rust);   // a hanging chain
    }
    // The gantry rail high over the west belt (something runs on it: you'll see), with its lit track
    const float RAIL_Y = 11.2f;
    prop(LANES[0] - 0.5f, RAIL_Y, ZN - 4.f, LANES[0] + 0.5f, RAIL_Y + 0.6f, ZS, steel2);
    prop(LANES[0] - 0.15f, RAIL_Y + 0.6f, ZN - 4.f, LANES[0] + 0.15f, ROOF, ZS, dark);
    neon(LANES[0] - 0.52f, RAIL_Y - 0.02f, ZN - 4.f, LANES[0] - 0.48f, RAIL_Y + 0.02f, ZS, red * 0.5f);
    neon(LANES[0] + 0.48f, RAIL_Y - 0.02f, ZN - 4.f, LANES[0] + 0.52f, RAIL_Y + 0.02f, ZS, red * 0.5f);

    // ---- along the walls: robot arms at their stations, a cabinet, a creator's window up high ----
    auto arm = [&](float x, float z, float side) {   // a sorting arm's base, column and folded arm, reaching over the belt
        B.mat = Mat::METAL;
        wall(x - 0.6f, 0.f, z - 0.6f, x + 0.6f, 0.5f, z + 0.6f, dark);
        prop(x - 0.25f, 0.5f, z - 0.25f, x + 0.25f, 3.2f, z + 0.25f, steel2);
        prop(x - 0.4f, 3.2f, z - 0.4f, x + 0.4f, 3.7f, z + 0.4f, hazard);
        prop(x + side * 0.2f, 3.4f, z - 0.15f, x + side * 2.4f, 3.7f, z + 0.15f, steel2);          // upper arm, out over the belt
        prop(x + side * 2.2f, 2.2f, z - 0.12f, x + side * 2.5f, 3.5f, z + 0.12f, steel2);          // forearm, down
        prop(x + side * 2.1f, 1.9f, z - 0.3f, x + side * 2.6f, 2.2f, z + 0.3f, dark);              // its gripper
        neon(x - 0.05f, 2.6f, z + (side > 0.f ? 0.26f : -0.26f), x + 0.05f, 2.75f, z + (side > 0.f ? 0.27f : -0.27f), cyan * 0.7f);   // status light
    };
    arm(-10.8f, -8.f, 1.f);  arm(10.8f, -14.f, -1.f);
    arm(-10.8f, -26.f, 1.f); arm(10.8f, -40.f, -1.f);
    arm(-10.8f, -44.f, 1.f);
    arm(-3.5f, -20.f, -1.f); arm(3.5f, -34.f, 1.f);   // between the belts
    B.mat = Mat::PANEL;
    wall(X0, 0.f, -18.f, X0 + 0.8f, 2.2f, -15.6f, steel2);                   // a cabinet on the west wall
    neon(X0 + 0.8f, 1.6f, -17.6f, X0 + 0.82f, 1.8f, -17.3f, {0.2f, 1.2f, 0.4f});
    // The window on the east wall, 9 m up: an observation room, dark but for one lit console
    prop(X1 - 0.05f, 8.6f, -24.f, X1, 11.f, -16.f, {0.03f, 0.04f, 0.05f});
    neon(X1 - 0.06f, 8.6f, -24.f, X1 - 0.05f, 8.7f, -16.f, {0.3f, 0.32f, 0.36f});
    neon(X1 - 0.07f, 9.0f, -21.f, X1 - 0.06f, 9.3f, -20.2f, cyan * 0.5f);
    prop(X1 - 0.6f, 8.4f, -24.4f, X1, 8.6f, -15.6f, steel);                 // its sill

    // ---- the chute: a hole in the east floor, framed in hazard paint, a shaft down into the dark ----
    B.mat = Mat::METAL;
    // The shaft drops 22 m to a short tunnel east: a boost at its foot fires you out through the pit's wall
    const float TUN0 = -23.f, TUN1 = -19.f, PITX = 18.f;
    wall(CHUTE.min.x - 0.4f, TUN0 - 0.6f, CHUTE.min.z - 0.4f, CHUTE.min.x, -0.6f, CHUTE.max.z + 0.4f, steel);   // its west side
    wall(CHUTE.min.x, TUN1, CHUTE.min.z - 0.4f, X1, -0.6f, CHUTE.min.z, steel);                                 // north
    wall(CHUTE.min.x, TUN1, CHUTE.max.z, X1, -0.6f, CHUTE.max.z + 0.4f, steel);                                 // south
    wall(CHUTE.max.x, TUN1, CHUTE.min.z, CHUTE.max.x + 0.4f, -0.6f, CHUTE.max.z, steel);                        // east, above the tunnel
    wall(CHUTE.min.x - 0.4f, TUN0 - 0.6f, CHUTE.min.z - 0.4f, PITX, TUN0, CHUTE.max.z + 0.4f, dark);            // the tunnel's floor
    wall(CHUTE.max.x, TUN1, CHUTE.min.z - 0.4f, PITX, TUN1 + 0.6f, CHUTE.max.z + 0.4f, dark);                   // its roof
    wall(CHUTE.min.x, TUN0, CHUTE.min.z - 0.4f, PITX, TUN1, CHUTE.min.z, steel);                                // its sides
    wall(CHUTE.min.x, TUN0, CHUTE.max.z, PITX, TUN1, CHUTE.max.z + 0.4f, steel);
    L.boosters.push_back({LevelBuilder::aabb(CHUTE.min.x, TUN0, CHUTE.min.z, PITX + 0.5f, TUN1, CHUTE.max.z), {1.f, 0.f, 0.f}, 15.f});
    for (int k = 0; k < 4; ++k)   // ribs of light down the tunnel, pointing the way
        neon(CHUTE.max.x + 0.4f + k * 1.1f, TUN1 - 0.06f, CHUTE.min.z + 0.6f, CHUTE.max.x + 0.7f + k * 1.1f, TUN1, CHUTE.max.z - 0.6f, {1.f, 0.6f, 0.1f});
    for (int k = 0; k < 8; ++k) {   // the frame: hazard stripes round the hole
        float t = k / 8.f;
        prop(CHUTE.min.x + t * 3.5f, 0.f, CHUTE.min.z - 0.4f, CHUTE.min.x + t * 3.5f + 0.25f, 0.02f, CHUTE.min.z, hazard);
        prop(CHUTE.min.x + t * 3.5f, 0.f, CHUTE.max.z, CHUTE.min.x + t * 3.5f + 0.25f, 0.02f, CHUTE.max.z + 0.4f, hazard);
    }
    neon(CHUTE.min.x - 0.05f, -3.f, CHUTE.min.z, CHUTE.min.x, -2.8f, CHUTE.max.z, {1.f, 0.6f, 0.1f});     // a lamp down the shaft
    prop(X1 - 0.1f, 2.f, -32.6f, X1, 3.2f, -30.4f, hazard);                                                // a sign over it
    neon(X1 - 0.12f, 2.3f, -32.2f, X1 - 0.1f, 2.9f, -30.8f, {0.05f, 0.05f, 0.05f});

    // A side-fed reject transfer hatch explains the route to the salvage pit.
    // Its damaged grille is folded against the housing; the opening stays clear.
    B.place("cradle_reject_hatch",{11.75f,0,-31.5f});
    int transfer=wall(7.5f,0,-33.5f,9.8f,.16f,-29.5f,dark);
    L.conveyors.push_back({transfer,{1,0,0},.7f});
    for(int k=0;k<5;++k)
        B.kit().rod({7.7f+k*.45f,.23f,-33.35f},{7.7f+k*.45f,.23f,-29.65f},.1f,steel2,8);
    for(float z:{-34.6f,-28.4f}) {
        prop(10.1f,0,z,10.2f,1.1f,z+.1f,steel2);
        prop(13.4f,0,z,13.5f,1.1f,z+.1f,steel2);
        prop(10.1f,1.05f,z,13.5f,1.15f,z+.1f,hazard);
    }
    B.place("cradle_switchgear",{14.55f,0,-36.5f},-90);
    B.place("cradle_scrap_cluster",{12.5f,.03f,-38.f},25,.55f);

    // Surfaces by colour
    B.paint(m0, Mat::CONCRETE, [&](vec3 c, Mat m) {
        if (LevelBuilder::same(c, conc) || LevelBuilder::same(c, conc2)) return Mat::CONCRETE;
        if (LevelBuilder::same(c, brick)) return Mat::BRICK;
        if (LevelBuilder::same(c, rubber)) return Mat::PANEL;
        if (LevelBuilder::same(c, hazard)) return Mat::PLATE;
        return m;
    });
    L.floors.push_back({X0, ZN, X1, ZS + 4.f, 0.001f, conc2, Mat::CONCRETE});

    // ---- the section ----
    Arena a;
    a.name = "THE FURNACE LINE";
    a.subtitle = "GET OFF THE BELT";
    a.space = ReverbSpace::HALL;
    a.tone = RoomTone::FOUNDRY;
    a.zone = LevelBuilder::aabb(X0, -40.f, ZF, X1, ROOF - 0.2f, ZS + 3.5f);
    a.bounds = LevelBuilder::aabb(X0, 0.f, ZN, X1, ROOF - 0.2f, ZS);
    a.extraZones.push_back(LevelBuilder::aabb(CHUTE.min.x, -40.f, CHUTE.min.z, CHUTE.max.x, ROOF - 0.2f, CHUTE.max.z));
    a.extraZones.push_back(LevelBuilder::aabb(CHUTE.min.x, -40.f, CHUTE.min.z, PITX, TUN1 - 0.1f, CHUTE.max.z));   // the tunnel
    a.playerStart = {3.5f, 0.f, 4.f};   // (a retry: on your feet beside the belt; the first time you wake on it)
    a.startYaw = -90.f;
    a.hasTrigger = false;               // no fight here
    a.maxAlive = 99;
    a.ambient = Ambient::EMBERS;
    a.theme = Theme{{0.03f, 0.025f, 0.03f}, {0.08f, 0.05f, 0.04f}, {0.05f, 0.04f, 0.04f},
                    glm::normalize(vec3{0.f, 0.2f, -1.f}), {1.f, 0.5f, 0.2f}, 0.f, 0.f,
                    {0.05f, 0.04f, 0.04f}, 0.f,
                    glm::normalize(vec3{0.15f, -0.35f, 1.f}), {1.05f, 0.55f, 0.24f},
                    {0.11f, 0.11f, 0.14f}, {0.13f, 0.07f, 0.04f}, {0.09f, 0.05f, 0.035f}, 0.016f};
    L.arenas.push_back(a);
    auto& S = L.story;

    // =========================================================================
    // THE SCRAP PIT: where the rejects end up when the furnace is full. 34 m
    // deep, its floor heaped with thrown-away bodies. The tunnel throws you out
    // of a hatch high in its west wall onto the landing heap. Ahead, under the
    // only lamp, UNIT 01 sits where it stopped, the revolver still in its hand:
    // take it and the scrap wakes. Then climb out up the heaps along the east
    // and south walls to the duct door, 13 m up.
    //
    //   z -62 ┌──────────────────────────────────────┐   (north)
    //         │  heaps          heaps                │
    //    hatch▶ [landing]─[01]                       │
    //  (y -21)│   (y -31)  (y -34)   the floor y -40 │
    //         │                         R1 R2        │
    //         │        R7 R6     R5 R4    R3         │
    //   z  -4 └─────[ledge y -27.4 / door]───────────┘   (south) → the ducts
    //        x 18                                   x 58
    // =========================================================================
    const float PX0 = PITX, PX1 = 58.f, PZN = -62.f, PZS = -4.f, PF = -40.f, PTOP = -4.f;
    const float LEDGE = -27.4f;   // the climb's top: the duct door's sill
    const vec3 slab{0.20f, 0.20f, 0.22f}, slab2{0.27f, 0.26f, 0.26f}, junk{0.30f, 0.26f, 0.22f}, junk2{0.22f, 0.20f, 0.19f},
               crate{0.36f, 0.27f, 0.16f}, ceram{0.62f, 0.61f, 0.58f}, pitLamp{1.6f, 1.35f, 0.95f};
    const LevelBuilder::Mark m1 = B.mark();
    B.mat = Mat::CONCRETE;
    wall(PX0, PF - 0.6f, PZN, PX1, PF, PZS, conc2);                                                   // the floor
    B.mat = Mat::ROCK;
    B.wallZ(PZN, PZS, PX0 - 0.6f, PX0, PF, PTOP, slab, {{CHUTE.min.z, CHUTE.max.z, TUN0, TUN1}});     // west, the hatch in it
    wall(PX1, PF, PZN, PX1 + 0.6f, PTOP, PZS, slab);                                                  // east
    wall(PX0 - 0.6f, PF, PZN - 0.6f, PX1 + 0.6f, PTOP, PZN, slab);                                    // north
    B.wallX(PX0 - 0.6f, PX1 + 0.6f, PZS, PZS + 0.6f, PF, PTOP, slab, {{30.2f, 33.8f, LEDGE, LEDGE + 3.f}});   // south, the duct door
    B.mat = Mat::METAL;
    wall(PX0 - 0.6f, PTOP, PZN - 0.6f, PX1 + 0.6f, PTOP + 0.8f, PZS + 0.6f, dark);                    // the roof, far up
    // The hatch you come out of: a steel frame, hazard paint, a red lamp over it
    prop(PX0, TUN0 - 0.5f, CHUTE.min.z - 0.5f, PX0 + 0.3f, TUN0, CHUTE.max.z + 0.5f, hazard);
    prop(PX0, TUN1, CHUTE.min.z - 0.5f, PX0 + 0.3f, TUN1 + 0.5f, CHUTE.max.z + 0.5f, steel2);
    neon(PX0 + 0.3f, TUN1 + 0.6f, -32.f, PX0 + 0.34f, TUN1 + 0.9f, -31.f, red);
    // Courses of darker stone banding the walls, the pit's depth marked in yellow every 6 m
    for (float y = PF + 6.f; y < PTOP; y += 6.f) {
        prop(PX1 - 0.15f, y, PZN, PX1, y + 0.4f, PZS, slab2);
        prop(PX0, y, PZN, PX0 + 0.15f, y + 0.4f, CHUTE.min.z - 0.6f, slab2);
        prop(PX0, y, CHUTE.max.z + 0.6f, PX0 + 0.15f, y + 0.4f, PZS, slab2);
        prop(PX0, y, PZN, PX1, y + 0.4f, PZN + 0.15f, slab2);
        prop(PX1 - 0.2f, y + 0.4f, -60.f, PX1, y + 0.55f, -58.5f, hazard);
    }

    // ---- the landing heap (where you come down), stepping down east to UNIT 01 ----
    B.mat = Mat::METAL;
    wall(22.f, PF, -37.f, 32.f, -31.f, -26.f, junk);          // the landing: a heap of crushed frames
    wall(32.f, PF, -36.f, 36.f, -34.f, -27.f, junk2);         // a step down: where UNIT 01 sits
    wall(36.f, PF, -35.f, 39.5f, -37.f, -28.f, junk);         // and another, to the floor
    wall(24.f, -31.f, -36.f, 27.f, -30.2f, -33.f, crate);     // a crate on the landing (cover, and you see past it)
    // The heap's top: a crust of plates, ribs and limbs (visual)
    for (int k = 0; k < 14; ++k) {
        float x = 22.6f + (k % 5) * 1.9f, z = -36.4f + (k / 5) * 3.6f + (k % 2) * 0.8f;
        if (x > 28.f && z > -34.f && z < -29.f) continue;   // (kept clear round UNIT 01)
        prop(x, -31.f, z, x + 0.8f + (k % 3) * 0.4f, -30.85f + (k % 2) * 0.1f, z + 0.5f + (k % 4) * 0.25f, k % 3 ? ceram : steel2);
    }
    B.mat = Mat::METAL;
    wall(31.6f, -31.f, -33.2f, 32.f, -29.6f, -30.f, steel2);  // a sheet of plate UNIT 01 leans back against, at the heap's edge
    prop(31.55f, -29.9f, -33.f, 31.6f, -29.7f, -30.2f, hazard);
    // The lamp: a work light hung on a cable from the dark, the only warm light down here, right over UNIT 01
    prop(30.95f, -26.6f, -31.65f, 31.05f, PTOP, -31.55f, dark);
    prop(30.2f, -27.f, -32.4f, 31.8f, -26.6f, -30.8f, steel2);
    neon(30.3f, -27.1f, -32.3f, 31.7f, -27.f, -30.9f, pitLamp * 2.2f);
    S.wrecks.push_back({{30.9f, -31.f, -31.6f}, -1.5708f - 0.45f, 1, (int)0,
                        storytext::cradleLogs[1], 1});

    // ---- the floor: heaps of the rejected; cover; a gantry crane fallen across the middle ----
    B.mat = Mat::METAL;
    auto heap = [&](float x0, float z0, float x1, float z1, float top, uint32_t seed) {   // a heap: a stepped mound of junk
        wall(x0, PF, z0, x1, top - 1.2f, z1, junk2);
        float ix = (x1 - x0) * 0.2f, iz = (z1 - z0) * 0.2f;
        wall(x0 + ix, top - 1.2f, z0 + iz, x1 - ix, top, z1 - iz, seed % 2 ? junk : crate);
        for (int k = 0; k < 4; ++k) {   // ribs and plates on its slopes
            float u = 0.15f + 0.22f * k, x = x0 + (x1 - x0) * u, z = (seed + k) % 2 ? z0 - 0.2f : z1 - 0.3f;
            prop(x, PF, z, x + 0.6f, top - 1.4f - 0.3f * (k % 2), z + 0.5f, k % 2 ? ceram : steel2);
        }
    };
    heap(42.f, -56.f, 48.f, -50.f, -37.6f, 1);
    heap(26.f, -54.f, 32.f, -47.f, -37.0f, 2);
    heap(44.f, -40.f, 47.f, -35.f, -38.4f, 3);
    heap(30.f, -18.f, 34.f, -13.f, -38.2f, 4);
    heap(50.f, -44.f, 55.f, -38.f, -37.2f, 5);
    wall(36.f, PF, -24.f, 37.f, -38.6f, -20.f, steel);        // low cover: a sheet of plate on its edge
    wall(40.f, PF, -47.f, 44.f, -38.8f, -46.f, steel);
    // The fallen gantry crane: a long girder lying askew across the floor, its trolley beside it
    B.kit().box({40.f, -39.4f, -29.f}, {1.2f, 1.2f, 22.f}, {0.45f, 0.35f, 0.08f}, 0.35f, 0.f, 0.08f);
    // Narrow segments follow the visible angled girder; its old enclosing
    // box blocked several metres of apparently empty fighting space.
    for(int k=0;k<11;++k) {
        float z=-39.f+k*2.f, x=40.f+std::tan(.35f)*(z+29.f);
        B.solid(x-.75f,PF,z-1.f,x+.75f,-38.8f,z+1.f);
    }
    wall(46.f, PF, -32.f, 48.f, -38.2f, -29.f, {0.45f, 0.35f, 0.08f});
    // Lamps: a few dead ones, one flickering cyan (a status light on the crane), a slit of the furnace's glow far up
    neon(46.8f, -38.15f, -30.6f, 47.2f, -38.1f, -30.2f, cyan);
    neon(PX0 + 6.f, PTOP - 0.05f, -46.f, PX1 - 6.f, PTOP, -45.4f, fire * 0.35f);       // a grate in the roof: the furnace above
    for (float x = 24.f; x < 56.f; x += 8.f) prop(x, PTOP - 1.2f, -46.f, x + 0.3f, PTOP, -45.4f, dark);   // its bars
    L.fans.push_back({{PX1 - 0.4f, -12.f, -40.f}, 2.2f, 0, {0.6f, 0.25f, 0.08f}});    // an extractor fan high in the east wall
    neon(PX1 - 0.05f, -21.f, -18.f, PX1, -20.6f, -14.f, {0.25f, 0.3f, 0.32f});         // a dim panel

    // ---- the climb out: heaps and crates stacked up the east and south walls to the duct door ----
    struct Step { float x0, z0, x1, z1, top; };
    const Step climb[] = {
        {48.f, -24.f, 52.f, -19.f, -38.6f}, {52.f, -18.f, 56.5f, -14.f, -37.2f}, {52.f, -12.f, 56.5f, -8.f, -35.8f},
        {46.f, -10.f, 50.f, -5.f, -34.4f}, {40.f, -10.f, 44.f, -5.f, -33.f}, {40.f, -17.f, 44.f, -13.f, -31.6f},
        {34.f, -17.f, 38.f, -13.f, -30.2f}, {34.f, -11.f, 37.5f, -8.f, -28.8f}};
    int ci = 0;
    for (const Step& c : climb) {
        B.mat = ci % 3 == 2 ? Mat::WOOD : Mat::METAL;
        wall(c.x0, PF, c.z0, c.x1, c.top, c.z1, ci % 3 == 2 ? crate : ci % 2 ? junk : junk2);
        prop(c.x0 + 0.1f, c.top, c.z0 + 0.1f, c.x1 - 0.1f, c.top + 0.03f, c.z0 + 0.3f, hazard);   // a painted edge to read the jump by
        ++ci;
    }
    B.mat = Mat::CONCRETE;
    wall(26.f, PF, -8.f, 39.f, LEDGE, PZS, slab2);                                     // the ledge at the top, by the door
    prop(26.f, LEDGE, -8.f, 39.f, LEDGE + 0.04f, -7.7f, hazard);
    for (float x = 26.5f; x < 39.f; x += 2.5f) prop(x, LEDGE, -8.1f, x + 0.08f, LEDGE + 1.1f, -8.f, steel2);   // a rail
    prop(26.f, LEDGE + 1.05f, -8.1f, 39.f, LEDGE + 1.12f, -8.f, steel2);
    neon(30.f, LEDGE + 3.3f, PZS - 0.05f, 34.f, LEDGE + 3.45f, PZS, {0.2f, 1.f, 0.5f});   // over the door: a green lamp (the way on)
    const int pitDoor = B.doorway(true, 30.2f, 33.8f, PZS - 0.1f, PZS + 0.7f, LEDGE, 3.f, {0.2f, 0.9f, 0.45f}, true);

    // ---- bodies dumped across the floor and the heaps (drawn by the story's code) ----
    const Step tops[] = {{22.f, -37.f, 32.f, -26.f, -31.f}, {42.f, -56.f, 48.f, -50.f, -37.6f}, {26.f, -54.f, 32.f, -47.f, -37.f},
                         {50.f, -44.f, 55.f, -38.f, -37.2f}, {44.f, -40.f, 47.f, -35.f, -38.4f}};
    for (int k = 0; k < 78; ++k) {
        auto h = [&](float n) { float v = std::sin(n * 12.9898f + k * 78.233f) * 43758.5453f; return v - std::floor(v); };
        LevelData::Story::Dumped d;
        if (k < 32) {   // on the heaps
            const Step& t = tops[k % 5];
            float ix = (t.x1 - t.x0) * 0.2f, iz = (t.z1 - t.z0) * 0.2f;
            d.pos = {glm::mix(t.x0 + ix, t.x1 - ix, h(1.f)), t.top, glm::mix(t.z0 + iz, t.z1 - iz, h(2.f))};
        } else {        // on the floor, mostly round the walls (the middle kept for the fight)
            float a = h(3.f);
            d.pos = a < 0.5f ? vec3{glm::mix(PX0 + 2.f, PX1 - 2.f, h(4.f)), PF, glm::mix(PZN + 1.5f, PZN + 6.f, h(5.f))}
                             : vec3{glm::mix(PX1 - 5.f, PX1 - 1.5f, h(6.f)), PF, glm::mix(-58.f, -26.f, h(7.f))};
        }
        if (k == 0) d.pos = {28.5f, -31.f, -28.f};   // one right where you land: the first thing you see is one of you
        if (glm::length(glm::vec2{d.pos.x - 30.9f, d.pos.z + 31.6f}) < 2.6f) d.pos.x -= 5.f;   // (not on top of UNIT 01)
        d.yaw = h(8.f) * 6.283f; d.face = h(9.f) > 0.6f; d.pose = h(10.f); d.tilt = (h(11.f) - 0.5f) * 0.3f;
        S.dumped.push_back(d);
    }
    // Dense salvage islands around the edges; the central combat lane and
    // the marked climb remain readable. These fragments have no tall collision.
    const vec3 salvage[]={{23,-40,-57},{35,-40,-58},{53,-40,-56},
        {55,-40,-47},{20,-40,-44},{20,-40,-20},{23,-40,-12},
        {28,-37,-50},{45,-37.6f,-53},{52.5f,-37.2f,-41},{45.5f,-38.4f,-37.5f},
        {26,-31,-34},{28,-31,-28},{55,-40,-29},{28,-40,-43}};
    for(int k=0;k<15;++k) {
        B.place("cradle_scrap_cluster",salvage[k],float(k*73),k%3==0?1.2f:.85f);
        // Oil, scrape marks and stripped casing rivets bind each cluster to the floor.
        if(salvage[k].y==PF) {
            vec3 p=salvage[k];
            prop(p.x-1.5f,PF+.006f,p.z-1,p.x+1.5f,PF+.012f,p.z+1,{.075f,.065f,.055f});
        }
    }
    for(vec3 p:{vec3{22,PF,-60},vec3{38,PF,-60},vec3{55,PF,-60},vec3{55,PF,-35}}) {
        int w=wall(p.x-1.35f,p.y,p.z-.95f,p.x+1.35f,p.y+1.65f,p.z+.95f,steel);
        L.walls[w].hidden=true;
        B.place("cradle_salvage_bin",p);
    }
    B.place("cradle_switchgear",{18.3f,PF,-48},90);
    B.place("cradle_switchgear",{57.7f,PF,-25},-90);
    for(float z:{-58.f,-46.f,-18.f}) {
        for(float x:{19.f,57.f}) {
            prop(x-.25f,PF,z-.3f,x+.25f,-5,z+.3f,steel);
            for(int k=0;k<6;++k)
                prop(x-.28f,PF+1+k*5.f,z-.4f,x+.28f,PF+1.15f+k*5.f,z+.4f,rust);
        }
        prop(19,-7,z-.35f,57,-6.4f,z+.35f,steel2);
    }
    for(vec3 p:{vec3{23,PF,-57},vec3{52,PF,-51},vec3{53,PF,-36},vec3{22,PF,-15}}) {
        // Low work lights reveal nearby salvage instead of lighting only the
        // ceiling of this 36 m shaft. Thin supports keep their footprints clear.
        prop(p.x-.08f,p.y,p.z-.08f,p.x+.08f,p.y+3.5f,p.z+.08f,steel2);
        prop(p.x-.65f,p.y+3.5f,p.z-.2f,p.x+.65f,p.y+3.8f,p.z+.2f,dark);
        neon(p.x-.55f,p.y+3.45f,p.z-.15f,p.x+.55f,p.y+3.5f,p.z+.15f,pitLamp*.8f);
    }
    // A suspended broken feeder at the inlet and amber lamps show where the
    // bodies came from; the distinct cyan exit remains visible across the pit.
    for(float y:{-24.f,-26.f,-28.f})
        prop(18.05f,y,-33.8f,18.25f,y+.15f,-29.2f,rust);
    for(float z:{-53.f,-32.f,-12.f}) {
        prop(57.4f,-31,z,57.9f,-30.6f,z+1.4f,dark);
        neon(57.35f,-30.95f,z+.15f,57.4f,-30.65f,z+1.25f,pitLamp*.5f);
    }
    B.paint(m1, Mat::CONCRETE, [&](vec3 c, Mat m) {
        if (LevelBuilder::same(c, slab) || LevelBuilder::same(c, slab2)) return Mat::ROCK;
        if (LevelBuilder::same(c, hazard)) return Mat::PLATE;
        if (LevelBuilder::same(c, conc2)) return Mat::CONCRETE;
        return m;
    });
    L.floors.push_back({PX0, PZN, PX1, PZS, PF + 0.001f, conc2, Mat::CONCRETE});

    {   // the section: no fight until the revolver is taken (STORY's code starts it)
        Arena p;
        p.name = "THE SCRAP PIT";
        p.subtitle = "THE SCRAP IS AWAKE";
        p.space = ReverbSpace::SHAFT;
        p.tone = RoomTone::WELL;
        p.zone = LevelBuilder::aabb(PX0, -45.f, PZN, PX1, PTOP - 0.3f, PZS);
        p.bounds = LevelBuilder::aabb(PX0 + 0.5f, PF, PZN + 0.5f, PX1 - 0.5f, PTOP - 1.f, PZS - 0.5f);
        p.playerStart = {24.5f, -31.f, -30.5f};   // (a retry: on the landing heap, UNIT 01 ahead)
        p.startYaw = 0.f;
        p.scripted = true;
        p.maxAlive = 99;
        p.damageScale = 0.7f;                    // the first fight: forgiving
        p.exitDoor = pitDoor;
        p.ambient = Ambient::DUST;
        using W = WaveEntry;
        p.waves = {{W(EnemyType::HUSK, 0, {{23.f,-31.f,-36.2f},{29.5f,-31.f,-26.6f},{44.f,PF,-30.f}}),
                    W(EnemyType::MITE, 0, {{46.f,PF,-24.f},{36.f,PF,-44.f}})}};
        p.theme = Theme{{0.02f, 0.02f, 0.03f}, {0.04f, 0.04f, 0.05f}, {0.03f, 0.03f, 0.03f},
                        glm::normalize(vec3{0.f, 1.f, 0.f}), {1.f, 0.5f, 0.2f}, 0.f, 0.f, {0.03f, 0.03f, 0.03f}, 0.f,
                        glm::normalize(vec3{0.2f, -1.f, 0.35f}), {0.55f, 0.36f, 0.22f},
                        {0.14f, 0.15f, 0.19f}, {0.09f, 0.075f, 0.07f}, {0.035f, 0.035f, 0.045f}, 0.018f};
        L.arenas.push_back(p);
        S.pitSection = (int)L.arenas.size() - 1;
    }
    // the tunnel's light fades from the furnace's into the pit's
    L.blends.push_back({LevelBuilder::aabb(CHUTE.max.x, TUN0 - 1.f, CHUTE.min.z - 1.f, PITX + 4.f, TUN1 + 1.f, CHUTE.max.z + 1.f), 0, 1, 0, false});

    // =========================================================================
    // THE MAINTENANCE DUCTS (south of the pit, floor y -27.4): a corridor to a
    // junction where something waits, a crawlspace under a pipe run (slide), a
    // small room where UNIT 02 stopped with the Lancer across its knees, and a
    // shaft climbing toward the Assembly Hall. High in the junction's west wall
    // a vent opens onto a room nobody was meant to find: UNIT 03.
    //
    //   z -4 ═[door]═        (the pit)
    //        D1 x 30..34
    //   z 12 ─[gate]──────────────┐
    //        │ JUNCTION  x 24..44 │◀ vent (y +3.6) → UNIT 03's room x 18..24
    //   z 30 └──────────[door]────┘
    //                    D2 x 36..40, the crawlspace at z 38..40.4
    //   z 46 ┌──────────────────────┐
    //        │ UNIT 02   x 30..46   ├─ the shaft x 46..51.5, up 12 m
    //   z 58 └──────────────────────┘
    // =========================================================================
    const float DF = LEDGE;   // the ducts' floor
    const vec3 duct{0.24f, 0.27f, 0.28f}, duct2{0.17f, 0.19f, 0.2f}, pipe{0.36f, 0.30f, 0.20f}, teal{0.15f, 0.9f, 0.75f},
               grime{0.13f, 0.14f, 0.13f};
    const LevelBuilder::Mark m2 = B.mark();
    B.mat = Mat::PANEL;
    auto box3 = [&](float x0, float z0, float x1, float z1, float h, vec3 c) {   // a floor and a roof (the walls are added as they fit)
        wall(x0, DF - 0.6f, z0, x1, DF, z1, grime);
        wall(x0 - 0.4f, DF + h, z0 - 0.4f, x1 + 0.4f, DF + h + 0.5f, z1 + 0.4f, c);
    };
    // D1: from the pit's door south to the junction
    wall(30.f,DF-.6f,PZS,34.f,DF,PZS+.6f,grime); // solid threshold out of the pit
    box3(30.f, PZS + 0.6f, 34.f, 12.f, 3.f, duct2);
    wall(29.6f, DF, PZS + 0.6f, 30.f, DF + 3.f, 12.f, duct);
    wall(34.f, DF, PZS + 0.6f, 34.4f, DF + 3.f, 12.f, duct);
    for (float z = PZS + 2.f; z < 12.f; z += 2.f) {                                        // its ribs, and a strip light every other one
        prop(29.9f, DF, z, 30.f, DF + 3.f, z + 0.2f, duct2); prop(34.f, DF, z, 34.1f, DF + 3.f, z + 0.2f, duct2);
        prop(30.f, DF + 2.9f, z, 34.f, DF + 3.f, z + 0.2f, duct2);
        if ((int)z % 4 == 0) neon(31.4f, DF + 2.88f, z + 0.3f, 32.6f, DF + 2.9f, z + 0.5f, teal * 0.6f);
    }
    B.kit().rod({30.2f, DF + 2.6f, PZS + 0.6f}, {30.2f, DF + 2.6f, 12.f}, 0.12f, pipe, 8);   // a pipe run along it
    // the junction: 20 x 18, 6 m high, pipes and tanks; the gate behind you, the door ahead
    box3(24.f, 12.f, 44.f, 30.f, 6.f, duct2);
    B.wallX(23.6f, 44.4f, 11.6f, 12.f, DF, DF + 6.f, duct, {{30.f, 34.f, DF, DF + 3.f}});
    B.wallX(23.6f, 44.4f, 30.f, 30.4f, DF, DF + 6.f, duct, {{36.f, 40.f, DF, DF + 3.f}});
    wall(44.f, DF, 12.f, 44.4f, DF + 6.f, 30.f, duct);
    B.wallZ(12.f, 30.f, 23.6f, 24.f, DF, DF + 6.f, duct, {{20.f, 23.f, DF + 3.6f, DF + 5.f}});   // west: the vent
    const int jGate = B.doorway(true, 30.2f, 33.8f, 11.5f, 12.1f, DF, 3.f, {0.9f, 0.6f, 0.15f});
    const int jDoor = B.doorway(true, 36.2f, 39.8f, 29.9f, 30.5f, DF, 3.f, {0.9f, 0.25f, 0.15f}, true);
    B.mat = Mat::METAL;
    // tanks (cover), pipe stacks, a gantry walkway along the east wall
    B.kit().column({28.f, DF, 17.f}, 1.4f, 3.6f, {0.30f, 0.33f, 0.3f}, 14);
    B.solid(26.6f, DF, 15.6f, 29.4f, DF + 3.6f, 18.4f);
    B.kit().column({40.f, DF, 24.5f}, 1.2f, 3.2f, {0.30f, 0.33f, 0.3f}, 14);
    B.solid(38.8f, DF, 23.3f, 41.2f, DF + 3.2f, 25.7f);
    wall(33.f, DF, 20.f, 35.f, DF + 1.2f, 22.5f, crate);
    wall(24.f, DF, 24.f, 25.5f, DF + 1.2f, 29.6f, crate);                                 // crates under the vent ...
    wall(24.f, DF + 2.3f, 18.f, 25.4f, DF + 2.5f, 25.f, steel2);                          // ... and a pipe ledge up to it
    wall(24.f,DF+3.3f,20.f,27.f,DF+3.6f,23.f,steel2); // stand here before sliding through the vent
    for (float y : {DF + 4.6f, DF + 5.2f}) B.kit().rod({24.3f, y, 12.f}, {43.7f, y, 12.f}, 0.18f, pipe, 8);
    for (float z : {14.f, 20.f, 26.f}) neon(30.f, DF + 5.95f, z, 38.f, DF + 6.f, z + 0.3f, teal * 0.5f);
    neon(43.95f, DF + 1.f, 16.f, 44.f, DF + 1.3f, 26.f, {0.6f, 0.15f, 0.1f});             // a warning strip on the east wall
    L.fans.push_back({{34.f, DF + 5.9f, 21.f}, 1.3f, 1, teal * 0.4f});                    // a fan in the roof
    // the vent and UNIT 03's room behind it, high in the west wall
    B.mat = Mat::PANEL;
    wall(18.f, DF + 3.f, 18.f, 24.f, DF + 3.6f, 25.f, grime);                             // its floor
    wall(17.6f, DF + 6.f, 17.6f, 24.f, DF + 6.4f, 25.4f, duct2);                          // its roof
    wall(17.6f, DF + 3.f, 17.6f, 18.f, DF + 6.f, 25.4f, duct);
    wall(18.f, DF + 3.f, 17.6f, 24.f, DF + 6.f, 18.f, duct);
    wall(18.f, DF + 3.f, 25.f, 24.f, DF + 6.f, 25.4f, duct);
    B.wallZ(18.f, 25.f, 23.6f, 24.f, DF + 3.f, DF + 6.f, duct, {{20.f, 23.f, DF + 3.6f, DF + 5.f}});
    for (int k = 0; k < 5; ++k) prop(23.5f, DF + 3.65f + k * 0.26f, 20.f, 23.6f, DF + 3.75f + k * 0.26f, 23.f, steel2);   // the vent's louvres (bent open)
    neon(19.f, DF + 5.9f, 21.f, 19.6f, DF + 6.f, 21.6f, {0.4f, 0.45f, 0.5f});
    for (int k = 0; k < 9; ++k)                                                            // tally marks scratched into the wall
        prop(18.f, DF + 4.6f, 19.f + k * 0.12f, 18.05f, DF + 5.0f, 19.04f + k * 0.12f, k % 5 == 4 ? ceram : vec3{0.5f, 0.5f, 0.48f});
    S.wrecks.push_back({{19.f, DF + 3.6f, 22.5f}, 1.5708f, 3, -1,
                        storytext::cradleLogs[3], 2, true});
    // D2: south from the junction's door, a pipe run across it low enough that you slide under
    box3(36.f, 30.4f, 40.f, 46.f, 3.f, duct2);
    wall(35.6f, DF, 30.4f, 36.f, DF + 3.f, 46.f, duct);
    wall(40.f, DF, 30.4f, 40.4f, DF + 3.f, 46.f, duct);
    B.mat = Mat::METAL;
    wall(36.f, DF + 1.2f, 38.f, 40.f, DF + 3.f, 40.4f, pipe);                             // the crawlspace: 1.2 m under the pipes
    for (float y : {DF + 1.45f, DF + 2.05f, DF + 2.65f}) B.kit().rod({36.f, y, 39.2f}, {40.f, y, 39.2f}, 0.28f, pipe, 10);
    neon(36.1f, DF + 1.15f, 37.9f, 39.9f, DF + 1.2f, 38.f, hazard * 1.4f);              // its edge, painted and lit
    prop(36.2f, DF + 2.2f, 37.7f, 39.8f, DF + 2.6f, 37.8f, hazard);
    neon(37.2f, DF + 2.88f, 33.f, 38.8f, DF + 2.9f, 33.3f, teal * 0.6f);
    neon(37.2f, DF + 2.88f, 43.f, 38.8f, DF + 2.9f, 43.3f, teal * 0.6f);
    // UNIT 02's room: 16 x 12, low; UNIT 02 sits against the far wall with the Lancer across its knees
    B.mat = Mat::PANEL;
    box3(30.f, 46.f, 46.f, 58.f, 5.f, duct2);
    B.wallX(29.6f, 46.4f, 45.6f, 46.f, DF, DF + 5.f, duct, {{36.f, 40.f, DF, DF + 3.f}});
    wall(29.6f, DF, 58.f, 46.4f, DF + 5.f, 58.4f, duct);
    wall(29.6f, DF, 46.f, 30.f, DF + 5.f, 58.f, duct);
    B.wallZ(46.f, 58.f, 46.f, 46.4f, DF, DF + 5.f, duct, {{50.f, 56.f, DF, DF + 4.5f}});   // east: into the shaft
    B.mat = Mat::METAL;
    wall(31.f, DF, 54.f, 33.f, DF + 1.f, 57.6f, crate);
    wall(42.f, DF, 47.f, 45.6f, DF + 2.f, 49.f, steel2);                                  // a cabinet, its door hanging open
    prop(45.6f, DF + 0.2f, 49.f, 45.7f, DF + 1.9f, 50.2f, steel2);
    prop(37.4f, DF + 3.4f, 56.4f, 38.6f, DF + 3.6f, 57.6f, steel2);                      // the lamp over UNIT 02
    neon(37.5f, DF + 3.35f, 56.5f, 38.5f, DF + 3.4f, 57.5f, pitLamp * 1.4f);
    prop(37.95f, DF + 3.6f, 56.95f, 38.05f, DF + 5.f, 57.05f, dark);
    S.wrecks.push_back({{38.f, DF, 57.3f}, 3.1416f, 2, (int)2,
                        storytext::cradleLogs[2], 2});
    // The service shaft: a 5.1 x 6 m deck rises to the Assembly Hall hatch
    B.mat = Mat::PANEL;
    const float SX0 = 46.4f, SX1 = 51.5f, SZ0 = 50.f, SZ1 = 56.f, SH = 14.f;
    // Bridge both the physical sill and the room-volume seam. The old
    // 46..46.4 gap let a dash through but clamped walking back to x=46.
    wall(46.f, DF-.6f, SZ0, SX0, DF, SZ1, grime);
    wall(SX0, DF - 1.2f, SZ0, SX1, DF - .65f, SZ1, grime);
    B.wallZ(SZ0 - 0.4f, SZ1 + 0.4f, SX1, SX1 + 0.4f, DF, DF + SH, duct,
            {{SZ0, SZ1, DF + 11.2f, DF + SH}});
    wall(SX0, DF, SZ0 - 0.4f, SX1, DF + SH, SZ0, duct);
    wall(SX0, DF, SZ1, SX1, DF + SH, SZ1 + 0.4f, duct);
    wall(SX0 - 0.4f, DF + 5.f, SZ0 - 0.4f, SX0, DF + SH, SZ1 + 0.4f, duct);
    wall(SX0 - 0.4f, DF + SH, SZ0 - 0.4f, SX1 + 0.4f, DF + SH + 0.5f, SZ1 + 0.4f, duct2);
    B.mat = Mat::METAL;
    // A short recovery beat after the Lancer: board and ride, no dash climb.
    // Full-width deck meets the door on the east, with no lip across the exit.
    S.serviceLift = B.mover({(SX0+SX1)*.5f, DF-.25f, 53.f}, {(SX1-SX0)*.5f,.25f,2.95f},
                           Mover::Path::DRIVEN, {0,0,0}, {0,11.2f,0}, 1, 0, teal);
    for(float z : {50.2f,55.8f}) {
        prop(50.95f,DF,z,51.15f,DF+SH-.2f,z+.1f,steel2);
        neon(51.16f,DF,z,51.2f,DF+11.2f,z+.1f,teal*.5f);
    }
    neon(SX0 + 1.f, DF + SH - 0.1f, SZ0 + 2.6f, SX1 - 1.f, DF + SH - 0.05f, SZ1 - 1.f, {0.85f, 0.9f, 1.f});   // the hatch's light above
    for (int k = 0; k < 4; ++k) neon(SX1 - 0.05f, DF + 2.f + k * 3.f, SZ0 + 1.f, SX1, DF + 2.3f + k * 3.f, SZ0 + 1.6f, teal * 0.7f);
    B.paint(m2, Mat::PANEL, [&](vec3 c, Mat m) {
        if (LevelBuilder::same(c, grime)) return Mat::CONCRETE;
        if (LevelBuilder::same(c, hazard)) return Mat::PLATE;
        if (LevelBuilder::same(c, crate)) return Mat::WOOD;
        return m;
    });
    {
        Arena d;
        d.name = "THE MAINTENANCE DUCTS";
        d.subtitle = "SOMETHING IN THE JUNCTION";
        d.space = ReverbSpace::METAL;
        d.tone = RoomTone::PUMPWORKS;
        d.zone = LevelBuilder::aabb(24.f, DF - 2.f, 12.f, 44.f, DF + 5.8f, 30.f);   // the junction
        d.extraZones = {LevelBuilder::aabb(30.f, DF - 2.f, PZS, 34.f, DF + 2.9f, 12.f),       // D1
                        LevelBuilder::aabb(36.f, DF - 2.f, 30.f, 40.f, DF + 2.9f, 46.f),             // D2
                        LevelBuilder::aabb(30.f, DF - 2.f, 46.f, SX0, DF + 4.9f, 58.f),             // UNIT 02's room
                        LevelBuilder::aabb(SX0, DF - 2.f, SZ0, SX1, DF + SH - 0.1f, SZ1),           // the shaft
                        LevelBuilder::aabb(18.f, DF - 2.f, 18.f, 24.f, DF + 5.9f, 25.f)};           // UNIT 03's
        d.bounds = LevelBuilder::aabb(24.5f, DF, 12.5f, 43.5f, DF + 5.5f, 29.5f);
        d.playerStart = {32.f, DF, PZS + 2.5f};
        d.startYaw = 90.f;
        d.trigger = LevelBuilder::aabb(24.f, DF - 1.f, 14.5f, 44.f, DF + 6.f, 30.f);
        d.hasTrigger = true;
        d.entryGate = jGate; d.exitDoor = jDoor;
        d.maxAlive = 99;
        d.damageScale = 0.8f;
        d.ambient = Ambient::STEAM;
        using W = WaveEntry;
        d.waves = {{W(EnemyType::SHIELDBEARER,0,{{36.f,DF,27.5f}}),
                    W(EnemyType::RIPPER,0,{{42.f,DF,16.f},{29.f,DF,21.f}}),
                    W(EnemyType::MITE,0,{{26.f,DF,14.f},{42.f,DF,21.f},{31.f,DF,25.f}})}};
        d.theme = Theme{{0.02f, 0.03f, 0.03f}, {0.04f, 0.05f, 0.05f}, {0.03f, 0.03f, 0.03f},
                        glm::normalize(vec3{0.f, 1.f, 0.f}), {0.5f, 0.9f, 0.8f}, 0.f, 0.f, {0.03f, 0.03f, 0.03f}, 0.f,
                        glm::normalize(vec3{-0.3f, -1.f, 0.2f}), {0.45f, 0.62f, 0.58f},
                        {0.15f, 0.19f, 0.19f}, {0.08f, 0.08f, 0.07f}, {0.03f, 0.045f, 0.045f}, 0.02f};
        L.arenas.push_back(d);
    }
    L.blends.push_back({LevelBuilder::aabb(30.f, DF - 1.f, PZS - 3.f, 34.f, DF + 4.f, PZS + 6.f), 1, 2, 2, false});
    buildCradleChapter(B, DF + 11.2f);

    // ---- what the story's code needs ----
    S.built = true;
    S.lanes = {LANES[0], LANES[1], LANES[2]};
    S.beltTop = BELT_TOP; S.beltStart = BELT_S - 1.f; S.beltEnd = BELT_N; S.beltSpeed = BELT_SPEED;
    S.wake = {LANES[1], BELT_TOP, 10.f};
    S.furnace = LevelBuilder::aabb(X0, -40.f, ZB, X1, -2.5f, ZN);
    S.pitFloor = PIT + 0.15f;
    S.chute = CHUTE;
    S.clawRail = {LANES[0], RAIL_Y, 0.f};
    return L;
}
