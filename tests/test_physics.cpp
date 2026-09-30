// Headless physics tests — no window or GL context needed.
// Build + run with `make test`.
#include "../src/Player.h"
#include <cstdio>
#include <cstring>

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++failures; } \
                              else { std::printf("ok:   %s\n", msg); } } while (0)

static constexpr float DT = 1.f / 60.f;

int main() {
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
        CHECK(peak > 1.2f, "jump reaches a reasonable height");
        CHECK(p.onGround && p.position.y == 0.f, "jump lands back on the floor");
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

    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
