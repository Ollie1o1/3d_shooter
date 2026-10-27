// Headless physics tests — no window or GL context needed.
// Build + run with `make test`.
#include "player/Player.h"
#include "core/Balance.h"
#include "core/Pacer.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <vector>

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++failures; } \
                              else { std::printf("ok:   %s\n", msg); } } while (0)

static constexpr float DT = 1.f / 60.f;

int main() {
    balanceInit();   // the numbers as assets/balance.txt ships them (core/Balance.h)
    Uint8 keys[SDL_NUM_SCANCODES];
    std::memset(keys, 0, sizeof(keys));

    // 1. Standing on the floor stays grounded every tick.
    {
        Player p({0.f, 0.f, 0.f});
        int airborne = 0;
        for (int i = 0; i < 120; ++i) { p.update(DT, keys, nullptr, 0); if (!p.onGround) ++airborne; }
        CHECK(airborne == 0, "player on floor is grounded every tick");
    }

    // 2. Standing on top of a box stays grounded every tick (no flicker).
    {
        std::vector<Wall> walls{ Wall{ AABB{{-5.f, 0.f, -5.f}, {5.f, 2.f, 5.f}} } };
        SpatialGrid grid; grid.build(walls);
        Player p({0.f, 2.5f, 0.f});
        for (int i = 0; i < 60; ++i) p.update(DT, keys, walls.data(), (int)walls.size(), false, &grid);
        int airborne = 0, groundedToAirTransitions = 0;
        bool prev = p.onGround;
        for (int i = 0; i < 120; ++i) {
            p.update(DT, keys, walls.data(), (int)walls.size(), false, &grid);
            if (!p.onGround) ++airborne;
            if (prev && !p.onGround) ++groundedToAirTransitions;
            prev = p.onGround;
        }
        std::printf("      box top: airborne ticks=%d, ground->air transitions=%d, y=%.4f\n",
                    airborne, groundedToAirTransitions, p.position.y);
        CHECK(airborne == 0, "player standing on a box is grounded every tick");
        CHECK(p.position.y > 1.99f && p.position.y < 2.01f, "player rests on box top");
    }

    // 3. Walking off a box edge becomes airborne and falls to the floor.
    {
        std::vector<Wall> walls{ Wall{ AABB{{-1.f, 0.f, -1.f}, {1.f, 2.f, 1.f}} } };
        Player p({0.f, 2.f, 0.f});
        p.onGround = true;
        p.velocity = {6.f, 0.f, 0.f};
        keys[SDL_SCANCODE_D] = 0;
        bool leftGround = false;
        for (int i = 0; i < 180; ++i) {
            p.velocity.x = 6.f;
            p.update(DT, keys, walls.data(), (int)walls.size());
            if (!p.onGround && p.position.x > 1.5f) leftGround = true;
        }
        CHECK(leftGround, "walking off a box edge leaves the ground");
        CHECK(p.position.y < 0.01f && p.onGround, "player falls to the floor after leaving a box");
    }

    // 4. Jump from the floor goes up and comes back down.
    {
        Player p({0.f, 0.f, 0.f});
        p.update(DT, keys, nullptr, 0);
        keys[SDL_SCANCODE_SPACE] = 1;
        p.update(DT, keys, nullptr, 0);
        keys[SDL_SCANCODE_SPACE] = 0;
        float peak = 0.f;
        for (int i = 0; i < 120; ++i) { p.update(DT, keys, nullptr, 0); peak = std::max(peak, p.position.y); }
        CHECK(peak > 1.05f && peak < 1.35f, "a tap is a hop (the jump cut), not a twitch");
        CHECK(p.onGround && p.position.y == 0.f, "jump lands back on the floor");
    }

    // 4b. The jump cut and the heavier fall
    {
        // airtime / peak of a jump with space held for `hold` seconds, running at 7 m/s
        auto arc = [&](float hold, float& peak, float& air, float& dist) {
            Player p({0.f, 0.f, 0.f});
            Uint8 k[SDL_NUM_SCANCODES]; std::memset(k, 0, sizeof(k));
            p.velocity = {7.f, 0.f, 0.f}; p.update(DT, k, nullptr, 0);
            peak = 0.f; air = 0.f;
            k[SDL_SCANCODE_SPACE] = 1;
            for (int i = 0; i < 240; ++i) {
                if (i * DT >= hold) k[SDL_SCANCODE_SPACE] = 0;
                p.velocity.x = 7.f; p.velocity.z = 0.f;
                p.update(DT, k, nullptr, 0);
                if (!p.onGround) air += DT;
                peak = std::max(peak, p.position.y);
                if (i > 2 && p.onGround) break;
            }
            dist = air * 7.f;
        };
        float pk, air, d, pk2, air2, d2;
        arc(1.f, pk, air, d);
        arc(0.15f, pk2, air2, d2);
        std::printf("      held: peak %.2f m, %.3f s in the air; let go at 0.15 s: peak %.2f m, %.3f s\n", pk, air, pk2, air2);
        // Before the change: 8.5 m/s up at 24 m/s^2 both ways = 1.505 m, 0.708 s in the air
        CHECK(pk > 1.6f && air >= 0.69f, "a held jump goes higher than before and stays up as long (every old jump still makes it)");
        CHECK(pk2 < pk - 0.3f && air2 < air - 0.12f, "letting go early cuts the jump short");
        // Heavier on the way down: falling 3 m off a ledge vs launched from the same height
        Player walk({0.f, 3.f, 0.f}), thrown({0.f, 3.f, 0.f});
        Uint8 k[SDL_NUM_SCANCODES]; std::memset(k, 0, sizeof(k));
        std::vector<Wall> ledge{ Wall{ AABB{{-5.f, 0.f, -5.f}, {0.2f, 3.f, 5.f}} } };
        walk.update(DT, k, ledge.data(), 1);   // standing on it
        float tWalk = 0.f, tThrown = 0.f;
        for (int i = 0; i < 300 && !(i > 5 && walk.onGround && walk.position.y < 0.01f); ++i) {
            walk.velocity.x = 5.f; walk.update(DT, k, ledge.data(), 1); if (walk.position.x > 0.6f) tWalk += DT; }
        thrown.velocity = {0.f, 0.f, 0.f}; thrown.launched();
        for (int i = 0; i < 300 && !thrown.onGround; ++i) { thrown.update(DT, k, nullptr, 0); tThrown += DT; }
        CHECK(walk.onGround && tWalk < tThrown * 0.92f, "stepping off a ledge falls faster than being thrown off one (pads, tubes and the grapple keep their arcs)");
        // A buffered jump (pressed before landing, let go before it fires) still goes full height
        Player b({0.f, 0.15f, 0.f}); b.velocity.y = -2.f;
        k[SDL_SCANCODE_SPACE] = 1; b.update(DT, k, nullptr, 0); k[SDL_SCANCODE_SPACE] = 0;
        float bpk = 0.f;
        for (int i = 0; i < 200; ++i) { b.update(DT, k, nullptr, 0); bpk = std::max(bpk, b.position.y); }
        CHECK(bpk > 1.6f, "a buffered jump is never cut");
    }

    // 4c. Frame pacing (core/Pacer.h) against a simulated GPU that needs G seconds a frame,
    //     with the CPU allowed two frames ahead (main.cpp's fences)
    {
        auto run = [](double gpu, double capPeriod, bool pace, double& fps, double& avgWait) {
            Pacer p; double t = 0.0, gpuFree = 0.0, waitSum = 0.0; int n = 0;
            double done[2] = {0.0, 0.0};   // when the last two frames finish on the GPU
            for (int f = 0; f < 4000; ++f) {
                double start = std::max(t, done[f % 2]);   // wait for the frame two back
                double waited = start - t;
                if (f > 2000) { waitSum += waited; ++n; }
                t = start + 0.001;                          // 1 ms of CPU work
                gpuFree = std::max(gpuFree, t) + gpu;       // the GPU takes it when free
                done[f % 2] = gpuFree;
                if (pace) p.onWait(waited); else p.reset();
                double period = std::max(capPeriod, p.period);
                if (period > 0.0) t = std::max(t, start + period);
            }
            fps = 4000.0 / t; avgWait = waitSum / n;
        };
        double fpsOff, waitOff, fpsOn, waitOn, fpsCap, waitCap;
        run(0.005, 0.0, false, fpsOff, waitOff);      // a 5 ms GPU, unlimited, no pacing
        run(0.005, 0.0, true,  fpsOn,  waitOn);       // ...paced
        run(0.005, 1.0 / 144, true, fpsCap, waitCap); // under the GPU's limit: pacing stays out of it
        std::printf("      unpaced %.0f fps, %.2f ms queued; paced %.0f fps, %.2f ms; 144 cap paced %.0f fps\n",
                    fpsOff, waitOff * 1e3, fpsOn, waitOn * 1e3, fpsCap);
        CHECK(waitOff > 0.003 && waitOn < 0.0005, "pacing: unlimited no longer queues frames behind the GPU (old input on screen)");
        CHECK(fpsOn > fpsOff * 0.93, "pacing: and keeps nearly all the frame rate");
        CHECK(std::fabs(fpsCap - 144.0) < 1.0 && waitCap < 1e-6, "pacing: a cap the GPU can hold is left exactly as it was");
    }

    // 5. Walking into a wall does not pass through it.
    {
        std::vector<Wall> walls{ Wall{ AABB{{2.f, 0.f, -5.f}, {3.f, 4.f, 5.f}} } };
        Player p({0.f, 0.f, 0.f});
        p.camera.yaw = 0.f; // facing +X
        keys[SDL_SCANCODE_W] = 1;
        for (int i = 0; i < 180; ++i) p.update(DT, keys, walls.data(), (int)walls.size());
        keys[SDL_SCANCODE_W] = 0;
        CHECK(p.position.x + p.radius <= 2.001f, "wall blocks forward movement");
    }


    // 6. The spatial grid covers walls far from the origin (Act II sits at Z -450..-660)
    {
        std::vector<Wall> walls{ Wall{ AABB{{-2.f, -61.f, -602.f}, {2.f, -59.f, -598.f}} },
                                 Wall{ AABB{{300.f, 0.f, 0.f}, {302.f, 2.f, 2.f}} } };
        SpatialGrid grid; grid.build(walls);
        std::vector<int> out;
        grid.query(AABB{{-1.f, -61.f, -601.f}, {1.f, -59.f, -599.f}}, out);
        bool foundFar = std::find(out.begin(), out.end(), 0) != out.end();
        bool notOther = std::find(out.begin(), out.end(), 1) == out.end();
        CHECK(foundFar && notOther, "the spatial grid finds a wall at Z -600 and only nearby walls");
        Player p({0.f, -59.f + 0.5f, -600.f});
        p.floorY = -1000.f;
        for (int i = 0; i < 60; ++i) p.update(DT, keys, walls.data(), (int)walls.size(), false, &grid);
        CHECK(std::fabs(p.position.y - (-59.f)) < 0.01f && p.onGround, "the player stands on a wall top far from the origin");
    }

    // 7. The hard floor follows floorY (Act II's floors are at Y -60)
    {
        Player p({0.f, -50.f, 0.f});
        p.floorY = -60.f;
        for (int i = 0; i < 180; ++i) p.update(DT, keys, nullptr, 0);
        CHECK(std::fabs(p.position.y + 60.f) < 0.001f && p.onGround, "the player falls to floorY and stands there");
    }

    // 8. Wading: slower on foot, full speed when sliding
    {
        auto runFor = [&](float depth, bool slide) {
            Player p({0.f, 0.f, 0.f});
            p.wadeDepth = depth;
            Uint8 k[SDL_NUM_SCANCODES]; std::memset(k, 0, sizeof(k));
            k[SDL_SCANCODE_W] = 1;
            for (int i = 0; i < 60; ++i) p.update(DT, k, nullptr, 0);
            float top = 0.f;
            if (slide) {
                k[SDL_SCANCODE_LCTRL] = 1;
                for (int i = 0; i < 6; ++i) { p.update(DT, k, nullptr, 0); top = std::max(top, glm::length(glm::vec2(p.velocity.x, p.velocity.z))); }
            } else top = glm::length(glm::vec2(p.velocity.x, p.velocity.z));
            return top;
        };
        float dry = runFor(0.f, false), ankle = runFor(0.4f, false), deep = runFor(1.2f, false);
        std::printf("      walk speed dry %.2f, 0.4 m %.2f, 1.2 m %.2f\n", dry, ankle, deep);
        CHECK(std::fabs(ankle / dry - 0.8f) < 0.03f && std::fabs(deep / dry - 0.55f) < 0.03f,
              "wading slows walking: -20% ankle-deep, -45% waist-deep and deeper");
        CHECK(deep > 3.f, "even the deepest water leaves you able to move");
        float drySlide = runFor(0.f, true), wetSlide = runFor(1.2f, true);
        std::printf("      slide top speed dry %.2f, 1.2 m %.2f\n", drySlide, wetSlide);
        CHECK(drySlide > 12.f && wetSlide >= drySlide - 0.01f, "a slide skims deep water at full slide speed");
        CHECK(Player::wadeFactor(0.05f) == 1.f, "a puddle doesn't slow you");
    }

    // Fast landings on thin floors: a ground slam (40 m/s and climbing) or a
    // long drop puts the feet more than half a slab deep in one tick. They
    // must land on top, never be pushed out underneath
    {
        int clipped = 0;
        for (float thick : {0.3f, 0.5f, 1.f, 1.5f, 2.f})
            for (float speed : {10.f, 25.f, 40.f, 54.f, 70.f})
                for (float x : {0.f, 1.f}) {   // over the middle of a slab, and over the seam between two
                    std::vector<Wall> walls{ Wall{ AABB{{-5.f, -thick, -5.f}, {1.f, 0.f, 5.f}} },
                                             Wall{ AABB{{1.f, -thick, -5.f}, {5.f, 0.f, 5.f}} } };
                    SpatialGrid grid; grid.build(walls);
                    Player p({x, 3.f, 0.f});
                    p.floorY = -100.f;
                    p.velocity = {0.f, -speed, 0.f};
                    for (int i = 0; i < 90; ++i) p.update(DT, keys, walls.data(), (int)walls.size(), false, &grid);
                    if (p.position.y < -0.01f) { ++clipped; std::printf("      thick %.1f, %.0f m/s at x %.0f: ended at y %.2f\n", thick, speed, x, p.position.y); }
                }
        CHECK(clipped == 0, "landing fast on a thin floor (a slam, a long drop) stands you on top, never under it");
    }

    // ...and the same for walls: a full-speed grapple (45 m/s) into a thin wall stops at it
    {
        int through = 0;
        for (float thick : {0.2f, 0.4f, 0.6f, 1.f})
            for (float speed : {20.f, 28.f, 45.f, 60.f}) {
                std::vector<Wall> walls{ Wall{ AABB{{2.f, 0.f, -5.f}, {2.f + thick, 3.f, 5.f}} } };
                Player p({0.f, 0.f, 0.f}); p.onGround = true;
                for (int i = 0; i < 30; ++i) { p.velocity.x = speed; p.update(DT, keys, walls.data(), (int)walls.size()); }
                if (p.position.x > 2.f) { ++through; std::printf("      thick %.1f at %.0f m/s: x %.2f\n", thick, speed, p.position.x); }
            }
        CHECK(through == 0, "moving fast into a thin wall (a dash, a grapple) stops at it, never passes through");
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
