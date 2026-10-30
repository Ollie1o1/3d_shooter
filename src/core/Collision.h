#pragma once
// =============================================================================
// Collision.h — the shapes everything collides with: AABB, Wall (an AABB with
// a look), and SpatialGrid (walls filed by XZ cell for fast queries). No
// OpenGL and no SDL, so the enemies, levels and tests can use it on their own.
// =============================================================================
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>

// AABB = Axis-Aligned Bounding Box, defined by two corners. Every obstacle in
// the world is one (the levels build them: Level.h, LevelGauntlet.h, LevelAct2.h).
struct AABB {
    glm::vec3 min, max; // min = bottom-left-front, max = top-right-back
};

// Ray vs AABB: distance along the ray to the first hit, or -1 on a miss.
inline float rayBoxHit(glm::vec3 o, glm::vec3 d, const AABB& b) {
    glm::vec3 invD{1.f/(d.x+1e-9f), 1.f/(d.y+1e-9f), 1.f/(d.z+1e-9f)};
    glm::vec3 t0 = (b.min-o)*invD, t1 = (b.max-o)*invD;
    glm::vec3 tMin = glm::min(t0,t1), tMax = glm::max(t0,t1);
    float tEnter = std::max({tMin.x,tMin.y,tMin.z});
    float tExit  = std::min({tMax.x,tMax.y,tMax.z});
    if (tEnter > tExit || tExit < 0.f) return -1.f;
    return tEnter > 0.f ? tEnter : tExit;
}

// Surface material: picks the texture a wall is drawn with (GameplayState).
// The second row are Act I's own: one look per arena (WorldGeometry.h maps
// each to the texture of its sides and of its top).
enum class Mat : unsigned char { BRICK, PANEL, ROCK, METAL, CONCRETE,
                                 ADOBE, PLATE, MARBLE, CIRCUIT, BASALT, WOOD,
                                 LIMESTONE, BRASS };   // ACT II and FAST

struct Wall {
    AABB      box;
    glm::vec3 color{0.28f, 0.28f, 0.32f};
    bool      hidden  = false;  // collides but isn't part of the static world mesh
    bool      dynamic = false;  // moves at runtime (moving platform): kept out of the
                                // SpatialGrid and tested every tick instead
    Mat       mat = Mat::BRICK;
};

// =============================================================================
// SpatialGrid — uniform 2D grid over the XZ plane for fast wall queries.
//
// Walls are static, except doors, which only ever shrink inside the box they
// were built with (so the cells they were filed under stay right). Query with
// a bounding box to get candidate wall indices — then still test actual AABB
// overlap yourself.
//
// build() fits the grid to the walls it's given (12 m cells, one cell of
// margin), so Act I, the Gauntlet and Act II (60 m down, far north) each get
// a grid of their own size.
// Dynamic walls (moving platforms) are left out: they'd be in the wrong
// cells a second later. Callers test those directly every tick.
// =============================================================================
struct SpatialGrid {
    static constexpr float CELL = 12.f;
    // Fitted to the walls in build(), with a cell of margin all round
    float X0 = 0.f, Z0 = 0.f;
    int   NX = 1, NZ = 1;

    std::vector<std::vector<int>> cells = std::vector<std::vector<int>>(1);

    void build(const std::vector<Wall>& walls) {
        float x0 = 1e9f, z0 = 1e9f, x1 = -1e9f, z1 = -1e9f;
        for (auto& w : walls) {
            if (w.dynamic) continue;
            x0 = std::min(x0, w.box.min.x); z0 = std::min(z0, w.box.min.z);
            x1 = std::max(x1, w.box.max.x); z1 = std::max(z1, w.box.max.z);
        }
        if (x0 > x1) { x0 = z0 = -CELL; x1 = z1 = CELL; }
        X0 = x0 - CELL; Z0 = z0 - CELL;
        NX = (int)((x1 - X0) / CELL) + 2;
        NZ = (int)((z1 - Z0) / CELL) + 2;
        cells.assign((size_t)NX * NZ, {});
        for (int i = 0; i < (int)walls.size(); ++i)
            if (!walls[i].dynamic) insertWall(i, walls[i].box);
    }

    // Returns candidate wall indices for a query box.
    // May include false positives — caller must still check intersection.
    void query(const AABB& box, std::vector<int>& out) const {
        out.clear();
        int x0 = cx(box.min.x), x1 = cx(box.max.x);
        int z0 = cz(box.min.z), z1 = cz(box.max.z);
        for (int xi = x0; xi <= x1; ++xi)
            for (int zi = z0; zi <= z1; ++zi)
                for (int idx : cell(xi, zi))
                    out.push_back(idx);
    }

private:
    int cx(float x) const { return glm::clamp((int)((x - X0) / CELL), 0, NX-1); }
    int cz(float z) const { return glm::clamp((int)((z - Z0) / CELL), 0, NZ-1); }
    const std::vector<int>& cell(int x, int z) const { return cells[z * NX + x]; }
          std::vector<int>& cell(int x, int z)       { return cells[z * NX + x]; }

    void insertWall(int idx, const AABB& b) {
        for (int xi = cx(b.min.x); xi <= cx(b.max.x); ++xi)
            for (int zi = cz(b.min.z); zi <= cz(b.max.z); ++zi)
                cell(xi, zi).push_back(idx);
    }
};
