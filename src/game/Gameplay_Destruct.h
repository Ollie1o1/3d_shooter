#pragma once
// =============================================================================
// Gameplay_Destruct.h — GameplayState: the level's pieces that break
// (Destructible, Level.h; placed by a layout file, LevelLayout.h).
//
// One is drawn whole from its prop (assets/props/<name>.glb) fitted to its box
// until shots and blasts take its hp; then its box is parked away (cover gone,
// a way through) and its pieces (<name>_pieces.glb: the same shape cut up in
// Blender, tools/blender/destruct_build.py) fly from where they were, settle,
// and fade. A retry puts every one back.
// Included at the end of GameplayState.h.
// =============================================================================

// The box it fills, as built: its prop's placement (fitted, like a prop that dresses a wall)
inline glm::mat4 GameplayState::destructiblePlacement(const Destructible& d, const char* suffix) const {
    const propfile::Prop* p = propfile::get(d.name + suffix);
    return p ? propfile::placement(*p, false, glm::mat4(1.f), d.home, 0.f) : glm::mat4(1.f);
}

// A round stopped on a wall at wallT: was it one that breaks? (true: it took the shot)
inline bool GameplayState::hitDestructible(glm::vec3 origin, glm::vec3 dir, float wallT, float dmg) {
    for (int i = 0; i < (int)level.destructibles.size(); ++i) {
        const Destructible& d = level.destructibles[i];
        if (d.broken) continue;
        float t = rayBoxHit(origin, dir, level.walls[d.wall].box);
        if (t > 0.f && t <= wallT + 0.05f) { damageDestructible(i, dmg, origin + dir * t, dir); return true; }
    }
    return false;
}

inline void GameplayState::damageDestructible(int i, float dmg, glm::vec3 at, glm::vec3 push) {
    Destructible& d = level.destructibles[i];
    if (d.broken || dmg <= 0.f) return;
    d.hp -= dmg;
    d.hitFlash = 0.1f;
    fx.spawnBurst(at, level.walls[d.wall].color * 0.8f + glm::vec3{0.15f}, 6, 5.f, 0.5f, 18.f);   // chips off it
    if (d.hp <= 0.f) breakDestructible(i, push);
    else if (chipCd <= 0.f) { playBoom("impact", at, 60, SoundGroup::WORLD); chipCd = 0.08f; }
}

inline void GameplayState::breakDestructible(int i, glm::vec3 push) {
    Destructible& d = level.destructibles[i];
    if (d.broken) return;
    d.broken = true;
    d.hp = 0.f;
    AABB& box = level.walls[d.wall].box;   // its box parked far under the level (like a broken anchor)
    const glm::vec3 c = (d.home.min + d.home.max) * 0.5f;
    box.min.y -= 500.f; box.max.y = box.min.y + 0.01f;
    const float floorY = groundHeightAt(c.x, c.z, d.home.min.y + 0.5f) + 0.05f;
    glm::vec3 along = glm::length(push) > 1e-4f ? glm::normalize(push) : glm::vec3{0.f};
    along.y = 0.f;
    // Its pieces where they were in it, thrown out from its middle (more the way the hit went), and up
    const propfile::Prop* pieces = propfile::get(d.name + "_pieces");
    const glm::mat4 M = destructiblePlacement(d, "_pieces");
    if (pieces)
        for (const auto& part : pieces->model.parts) {
            const glm::mat4 w = M * part.rel;
            Effects::Debris b;
            b.shape = glm::mat3(w);
            b.mesh = (float)part.mesh;
            b.pos = glm::vec3(w[3]);
            glm::vec3 out = b.pos - c; out.y = 0.f;
            if (glm::length(out) > 1e-3f) out = glm::normalize(out);
            const float high = glm::clamp((b.pos.y - d.home.min.y) / std::max(0.5f, d.home.max.y - d.home.min.y), 0.f, 1.f);
            b.vel = out * frand(1.5f, 4.f) + along * frand(2.f, 6.f) * (0.4f + high) + glm::vec3{0.f, frand(1.f, 4.f) * high, 0.f};
            b.axis = glm::normalize(glm::vec3{frand(-1.f, 1.f), frand(-1.f, 1.f), frand(-1.f, 1.f)} + glm::vec3{0.01f});
            b.spin = frand(1.f, 5.f);
            b.color = part.color; b.emissive = glm::vec3{0.f};
            b.maxLife = b.life = frand(5.f, 7.f);
            b.fade = 1.2f;   // (rubble: whole until its last moment, then gone)
            b.floorY = floorY;
            b.rest = true;
            fx.debris.push_back(b);
        }
    fx.spawnExplosionParticles(c, 1.5f);   // dust
    fx.spawnBurst(c, level.walls[d.wall].color * 0.7f + glm::vec3{0.2f}, 30, 7.f, 1.1f, 14.f);
    playBoom("ab_boom_stone", c, 115, SoundGroup::WORLD);
    shake(0.18f, 0.04f);
}

// A blast breaks what's in its reach (by how near its box is to the blast)
inline void GameplayState::blastDestructibles(glm::vec3 at, float radius, float dmg) {
    for (int i = 0; i < (int)level.destructibles.size(); ++i) {
        const Destructible& d = level.destructibles[i];
        if (d.broken) continue;
        const AABB& b = level.walls[d.wall].box;
        glm::vec3 q = glm::clamp(at, b.min, b.max);
        float dist = glm::length(q - at);
        if (dist < radius) damageDestructible(i, dmg * (1.f - dist / radius) * 1.5f, q, q - at);
    }
}

// Every one back as built (a retry, a new run)
inline void GameplayState::restoreDestructibles() {
    for (Destructible& d : level.destructibles) {
        d.broken = false; d.hp = d.maxHp; d.hitFlash = 0.f;
        level.walls[d.wall].box = d.home;
    }
}

inline void GameplayState::updateDestructibles(float dt) {
    chipCd -= dt;
    for (Destructible& d : level.destructibles) d.hitFlash = std::max(0.f, d.hitFlash - dt);
}

inline void GameplayState::gatherDestructibleBoxes(std::vector<BoxInstance>& out) {
    for (const Destructible& d : level.destructibles) {
        if (d.broken) continue;
        const glm::mat4 M = destructiblePlacement(d, "");
        const size_t first = out.size();
        if (!propdraw::push(out, d.name.c_str(), M, glm::vec3{1.f})) {   // (no file: its box)
            BoxInstance b;
            b.model = glm::translate(glm::mat4(1.f), (d.home.min + d.home.max) * 0.5f) * glm::scale(glm::mat4(1.f), d.home.max - d.home.min);
            b.color = level.walls[d.wall].color;
            out.push_back(b);
        }
        // hit: a flash; worn down: darker, as it cracks
        const float wear = 1.f - 0.35f * (1.f - glm::clamp(d.hp / std::max(1.f, d.maxHp), 0.f, 1.f));
        for (size_t k = first; k < out.size(); ++k) {
            out[k].color *= wear;
            out[k].emissive += glm::vec3{1.2f, 1.0f, 0.8f} * (d.hitFlash / 0.1f) * 0.5f;
        }
    }
}
