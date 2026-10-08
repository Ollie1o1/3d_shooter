#pragma once
// =============================================================================
// Gameplay_Render.h — GameplayState: Drawing a frame: lighting, the instanced boxes, the world, effects,
// beams and lasers, and the first-frame pipeline warm-up.
// Included at the end of GameplayState.h.
// =============================================================================

inline void GameplayState::applyLighting(ShaderProgram& sh, const Theme& th, glm::vec3 camPos) {
    sh.setVec3("lightDir",    th.lightDir);
    sh.setVec3("lightColor",  th.lightColor);
    sh.setVec3("viewPos",     camPos);
    sh.setVec3("uSkyAmb",     th.skyAmb);
    sh.setVec3("uGroundAmb",  th.groundAmb);
    sh.setVec3("uFogColor",   th.fogColor);
    sh.setFloat("uFogDensity", th.fogDensity);
    sh.setVec3Array("pointLightPos",   pointLightPos,   MAX_POINT_LIGHTS);
    sh.setVec3Array("pointLightColor", pointLightColor, MAX_POINT_LIGHTS);
}

inline void GameplayState::gatherBoxes(std::vector<BoxInstance>& out, const glm::mat4& view) {
    using namespace rig;
    float t = gameClock;

    gatherShiftBoxes(out);
    gatherSovereignBoxes(out);
    gatherPenitentBoxes(out);
    gatherWardenBoxes(out);
    gatherReliquaryBoxes(out);

    // HOLD: a ring of light on the ground; the lit arc is how far it's held.
    // Cyan while you hold it, red when an enemy stands in it.
    if (director.fighting() && director.goal().kind == WaveGoal::HOLD && !director.goalDone) {
        const WaveGoal& g = director.goal();
        const int SEG = 48;
        float held = director.goalProgress();
        glm::vec3 idle{0.9f, 0.85f, 0.6f}, hot = director.zoneContested ? glm::vec3{1.f, 0.2f, 0.15f}
                                                  : director.holding ? glm::vec3{0.3f, 1.f, 0.85f} : glm::vec3{1.f, 0.75f, 0.3f};
        float pulse = 0.7f + 0.3f * std::sin(t * 5.f);
        float segLen = 6.2832f * g.radius / SEG;
        glm::vec3 gp = director.goalPos();   // riding a ring: where it is now
        for (int k = 0; k < SEG; ++k) {
            float a = (k + 0.5f) / SEG * 6.2832f;
            bool lit = (float)k / SEG < held;
            glm::vec3 c = lit ? hot : idle * 0.35f;
            push(out, T(gp + glm::vec3{std::cos(a) * g.radius, 0.06f, std::sin(a) * g.radius}) * RY(-a) *
                      S({0.18f, 0.08f, segLen * 0.8f}), c * 0.4f, c * (lit ? 2.2f * pulse : 0.8f));
        }
    }

    for (auto& e : enemies) {
        if (!e.alive) continue;
        Enemy pose = e;
        pose.position = lerpPos(e.prevPosition, e.position);
        e.shownPos = pose.position; e.hasShown = true;
        pose.yaw = e.prevYaw + std::remainder(e.yaw - e.prevYaw, 6.2831853f) * renderAlpha;
        size_t from = out.size();
        buildEnemy(pose, t, out);
        if (e.hollow == Hollow::ENRAGED)   // everything that glows burns red
            for (size_t j = from; j < out.size(); ++j) {
                float g = std::max({out[j].emissive.x, out[j].emissive.y, out[j].emissive.z});
                if (g > 0.05f) out[j].emissive = glm::vec3{1.6f, 0.22f, 0.12f} * g;
            }
        if (e.hollow == Hollow::TWINNED)   // a seam of light down the middle, front and back
            for (float side : {-1.f, 1.f})
                push(out, T(pose.position + glm::vec3{0, e.height() * 0.5f, 0}) * RY(pose.yaw) * T({0.f, 0.f, side * e.radius() * 0.95f})
                          * S({0.07f, e.height() * 0.85f, 0.06f}),
                     {0.9f, 0.9f, 1.f}, glm::vec3{1.2f, 1.4f, 2.2f});
        if (e.halo) {                      // a thick gold ring turning over its head
            glm::mat4 h = T(pose.position + glm::vec3{0, e.height() + 0.5f, 0}) * RY(-t * 1.8f);
            for (int k = 0; k < 10; ++k)
                push(out, h * RY(k * 0.6283f) * T({0.f, 0.f, 0.5f}) * S({0.34f, 0.1f, 0.1f}),
                     {1.f, 0.8f, 0.35f}, glm::vec3{1.8f, 1.3f, 0.45f});
        }
        if (e.type == EnemyType::ANCHOR && e.targetable()) {   // its field: a ring on the ground and faint ribs
            float R = e.fieldRadius(), pulse = 0.55f + 0.45f * std::sin(t * 2.f + e.uid);
            glm::vec3 c = pose.position + glm::vec3{0, 0.06f, 0};
            glm::vec3 g = e.stats().glow * (0.8f + 0.6f * pulse);
            for (int k = 0; k < 40; ++k) {
                float a = k * 0.15708f;
                push(out, T(c + glm::vec3{std::cos(a) * R, 0.f, std::sin(a) * R}) * RY(-a) * S({0.12f, 0.06f, 1.62f}), g * 0.3f, g);
                if (k % 5 == 0)
                    push(out, T(c + glm::vec3{std::cos(a) * R, 1.5f, std::sin(a) * R}) * S({0.05f, 3.f, 0.05f}), g * 0.1f, g * 0.35f * pulse);
            }
        }
        if (e.shielded) {   // tethered to a CONDUCTOR: a ring of light turning over its head
            glm::mat4 halo = T(pose.position + glm::vec3{0, e.height() + 0.45f, 0}) * RY(t * 2.5f);
            for (int k = 0; k < 6; ++k)
                push(out, halo * RY(k * 1.0472f) * T({0.f, 0.f, 0.45f}) * S({0.28f, 0.06f, 0.06f}),
                     {0.2f, 0.6f, 0.55f}, glm::vec3{0.3f, 1.f, 0.9f} * 1.6f);
        }
        // The SOVEREIGN leaves afterimages down the line of a dash
        if (e.type == EnemyType::SOVEREIGN && e.dashTimer > 0.f) {
            for (int k = 1; k <= 3; ++k) {
                Enemy ghost = pose;
                ghost.position -= e.diveDir * (1.7f * k);
                ghost.hitFlashTimer = 0.f;
                size_t from = out.size();
                buildEnemy(ghost, t, out);
                for (size_t j = from; j < out.size(); ++j) {
                    out[j].color = glm::vec3{0.05f};
                    out[j].emissive = glm::vec3{1.2f, 0.3f, 0.15f} * (0.9f / k);
                }
            }
        }
    }

    // FAST: the ghost of your best run, a glowing figure on its route
    glm::vec3 gFeet; float gYaw;
    if (fast() && (!settings || settings->ghost) && countdown <= 0.f && ghost.at(elapsedTime, gFeet, gYaw) &&
        glm::length(gFeet - player.position) > 1.6f) {
        float r = glm::radians(gYaw);
        glm::mat4 g = T(gFeet) * RY(std::atan2(std::cos(r), std::sin(r)));
        float stride = std::sin(elapsedTime * 11.f) * 0.35f;
        glm::vec3 col{0.03f, 0.06f, 0.08f}, glow{0.25f, 0.9f, 1.2f};
        for (float s : {-1.f, 1.f})
            push(out, g * T({s * 0.14f, 0.85f, 0.f}) * RX(stride * s) * T({0.f, -0.42f, 0.f}) * S({0.16f, 0.84f, 0.18f}), col, glow * 0.6f);
        push(out, g * T({0.f, 1.25f, 0.f}) * S({0.46f, 0.72f, 0.26f}), col, glow * 0.8f);
        push(out, g * T({0.f, 1.78f, 0.f}) * S(glm::vec3{0.26f}), col, glow * 1.2f);
        for (float s : {-1.f, 1.f})
            push(out, g * T({s * 0.31f, 1.5f, 0.f}) * RX(-stride * s) * T({0.f, -0.32f, 0.f}) * S({0.12f, 0.64f, 0.12f}), col, glow * 0.5f);
    }

    // Spawn beams: a column of light while an enemy materialises
    for (auto& e : enemies) {
        if (!e.alive || e.state != EnemyState::SPAWNING) continue;
        float k = e.spawnTimer / Enemy::SPAWN_TIME;
        glm::vec3 c = e.stats().glow;
        glm::vec3 ep = lerpPos(e.prevPosition, e.position);
        float base = ep.y;
        push(out, T({ep.x, base + 8.f, ep.z}) * S({0.25f + 0.6f * k, 16.f, 0.25f + 0.6f * k}),
             c * 0.2f, c * (1.5f + 2.f * k));
        push(out, T({ep.x, base + 0.05f, ep.z}) * RY(t * 3.f) * S({2.2f * k + 0.5f, 0.06f, 2.2f * k + 0.5f}),
             c * 0.2f, c * 2.f);
    }

    // Enemy health bars (only once damaged), facing the camera
    glm::vec3 camRight = glm::normalize(glm::vec3(view[0][0], view[1][0], view[2][0]));
    glm::vec3 camFwdFlat = glm::normalize(glm::cross(camRight, glm::vec3(0,1,0)));
    for (auto& e : enemies) {
        if (!e.targetable() || e.health >= e.maxHealth || isBoss(e.type)) continue;
        float barW = std::max(1.0f, e.radius() * 1.8f), barH = 0.12f;
        float fill = e.health / e.maxHealth;
        glm::vec3 barPos = lerpPos(e.prevPosition, e.position) + glm::vec3{0, e.height() + 0.45f, 0};
        glm::mat4 bg(1.f);
        bg[0] = glm::vec4(camRight * barW, 0); bg[1] = glm::vec4(0, barH, 0, 0);
        bg[2] = glm::vec4(camFwdFlat * 0.01f, 0); bg[3] = glm::vec4(barPos, 1);
        push(out, bg, {0.05f, 0.05f, 0.05f}, {0.02f, 0.02f, 0.02f});
        glm::mat4 fm(1.f);
        fm[0] = glm::vec4(camRight * barW * fill, 0); fm[1] = glm::vec4(0, barH * 0.8f, 0, 0);
        fm[2] = glm::vec4(camFwdFlat * 0.02f, 0);
        fm[3] = glm::vec4(barPos - camRight * (barW * (1.f - fill) * 0.5f) - camFwdFlat * 0.01f, 1);
        push(out, fm, {0.9f, 0.15f, 0.1f}, {1.2f, 0.15f, 0.08f});
    }

    for (auto& d : fx.debris) {
        float k = glm::clamp(d.life / d.maxLife, 0.f, 1.f);
        glm::mat4 m = T(d.pos) * glm::rotate(glm::mat4(1.f), d.angle, d.axis)
                    * glm::mat4(d.shape) * S(glm::vec3{0.3f + 0.7f * k});
        push(out, m, d.color * (0.4f + 0.6f * k), d.emissive * k);
    }

    // Projectiles: a spinning bright core inside a darker shell
    for (auto& p : projSystem.pool) {
        if (!p.alive) continue;
        float s = (p.isGrenade ? 0.22f : 0.26f) * p.size;
        float spin = t * 9.f + p.position.x;
        glm::mat4 base = T(lerpPos(p.prevPosition, p.position)) * RY(spin) * RX(spin * 0.7f);
        push(out, base * S(glm::vec3{s}), p.emissiveColor * 0.3f, p.emissiveColor * 0.8f);
        push(out, base * RZ(0.785f) * S(glm::vec3{s * 0.75f}), p.emissiveColor, p.emissiveColor * 3.f);
    }

    // Drops
    for (auto& p : pickups) {
        float blink = p.life < 5.f && std::fmod(p.life, 0.4f) < 0.15f ? 0.2f : 1.f;
        glm::vec3 pp = lerpPos(p.prev, p.pos);
        glm::vec3 bob = pp + glm::vec3{0, std::sin(t * 3.f + p.pos.x) * 0.1f, 0};
        switch (p.kind) {
        case PickupKind::ORB: {
            float pulse = 1.f + 0.3f * std::sin(t * 8.f);
            push(out, T(bob) * RY(t * 3.f) * RX(0.6f) * S(glm::vec3{0.32f}),
                 {0.2f, 0.9f, 0.4f}, glm::vec3{0.3f, 1.6f, 0.6f} * pulse * blink);
            break;
        }
        case PickupKind::POTION: {
            // A red flask: body, neck, cork, and a loot beam
            glm::mat4 m = T(bob) * RY(t * 1.5f);
            glm::vec3 red{0.9f, 0.1f, 0.15f}, glow = glm::vec3{1.8f, 0.2f, 0.3f} * (0.7f + 0.3f * std::sin(t * 5.f)) * blink;
            push(out, m * T({0, 0.f, 0}) * S({0.42f, 0.42f, 0.42f}), red, glow);
            push(out, m * T({0, 0.3f, 0}) * S({0.16f, 0.2f, 0.16f}), {0.8f, 0.85f, 0.9f}, glm::vec3{0.4f} * blink);
            push(out, m * T({0, 0.45f, 0}) * S({0.18f, 0.1f, 0.18f}), {0.45f, 0.3f, 0.15f});
            push(out, T(p.pos + glm::vec3{0, 3.f, 0}) * S({0.08f, 6.f, 0.08f}), red * 0.2f, glow * 0.6f);
            break;
        }
        case PickupKind::XP: {
            glm::vec3 c{0.55f, 0.4f, 1.f}, g = glm::vec3{1.0f, 0.7f, 2.f} * (0.8f + 0.4f * std::sin(t * 6.f)) * blink;
            push(out, T(bob) * RY(t * 4.f) * RX(0.785f) * RZ(0.785f) * S(glm::vec3{0.36f}), c, g);
            push(out, T(p.pos + glm::vec3{0, 3.f, 0}) * S({0.06f, 6.f, 0.06f}), c * 0.2f, g * 0.5f);
            break;
        }
        }
    }

    for (auto& sl : fx.slashes) {
        float reveal = std::min(1.f, sl.t / 0.1f), fade = 1.f - sl.t / Effects::SLASH_LIFE;
        const int N = 18;
        glm::vec3 hot = glm::vec3{1.9f, 1.4f, 0.8f} * fade * 2.f;
        for (int i = 0; i < N; ++i) {
            float u = (i + 0.5f) / N;
            if (u > reveal) break;
            float w = 1.f - std::fabs(u * 2.f - 1.f);              // thickest mid-arc
            if (sl.kind == 2) {                                    // overhead: a vertical arc down to the floor
                float R = 3.2f, b = -0.4f + 2.1f * u;
                glm::mat4 m = T(sl.pos + glm::vec3{0, 0.3f, 0}) * RY(sl.yaw) * T({0.f, R * std::cos(b) * 0.75f, R * std::sin(b)})
                            * RX(b) * S({0.5f * w + 0.1f, 0.08f, R * 2.1f / N * 1.2f});
                push(out, m, glm::vec3{0.1f}, hot);
            } else {                                               // sweeps: a flat arc at chest height
                float R = sl.kind == 3 ? 3.4f : 4.4f, span = sl.kind == 3 ? 0.9f : 1.35f;
                float a = (sl.kind == 1 ? 1.f - u : u) * 2.f * span - span;
                glm::mat4 m = T(sl.pos + glm::vec3{0, 2.1f, 0}) * RY(sl.yaw + a) * T({0.f, 0.f, R})
                            * S({R * 2.f * span / N * 1.15f, 0.07f, 0.6f * w + 0.12f});
                push(out, m, glm::vec3{0.1f}, hot);
            }
        }
    }

    for (auto& s : fx.shockwaves) {
        float k = s.t / 0.45f;
        float r = s.radius * std::min(1.f, k);
        float fade = 1.f - std::min(1.f, k);
        for (int i = 0; i < 40; ++i) {
            float a = i * 6.2832f / 40.f;
            push(out, T(s.pos + glm::vec3{std::cos(a) * r, 0.25f, std::sin(a) * r}) * RY(-a) * S({0.25f, 0.5f * fade + 0.05f, r * 0.17f}),
                 s.color * 0.3f, s.color * 2.5f * fade);
        }
    }

    // Doors: two halves that part in the middle. Red while locked, cyan
    // when they'll open for you. The meeting edges glow.
    for (auto& d : level.doors) {
        if (d.openAmount >= 0.999f) continue;
        glm::vec3 lit = d.locked ? glm::vec3{1.8f, 0.18f, 0.12f} : glm::vec3{0.25f, 1.3f, 1.6f};
        if (!d.locked && d.open) lit *= 1.4f;
        bool ax = d.alongX();
        for (int h = 0; h < 2; ++h) {
            const AABB& b = level.walls[h ? d.wall2 : d.wall].box;
            glm::vec3 c = (b.min + b.max) * 0.5f, sz = b.max - b.min;
            if ((ax ? sz.x : sz.z) < 0.02f) continue;
            push(out, T(c) * S(sz), {0.17f, 0.17f, 0.21f});
            float edge = ax ? (h ? b.min.x : b.max.x) : (h ? b.min.z : b.max.z);
            float in = h ? 0.08f : -0.08f;
            glm::vec3 e = c;
            (ax ? e.x : e.z) = edge + in;
            glm::vec3 es = ax ? glm::vec3{0.16f, sz.y, sz.z + 0.06f} : glm::vec3{sz.x + 0.06f, sz.y, 0.16f};
            push(out, T(e) * S(es), lit * 0.2f, lit);
            for (float fy : {0.22f, 0.5f, 0.78f}) {        // light bars across each half
                glm::vec3 bc = c; bc.y = b.min.y + sz.y * fy;
                glm::vec3 bs = ax ? glm::vec3{sz.x * 0.75f, 0.09f, sz.z + 0.05f} : glm::vec3{sz.x + 0.05f, 0.09f, sz.z * 0.75f};
                push(out, T(bc) * S(bs), lit * 0.12f, lit * 0.5f);
            }
        }
    }

    // Boost tubes: chevrons streaming along the floor (or up the shaft)
    for (auto& bo : level.boosters) {
        glm::vec3 dir = bo.dir, mn = bo.box.min, mx = bo.box.max;
        glm::vec3 col = glm::vec3{0.3f, 1.4f, 1.9f};
        if (dir.y > 0.5f) {
            // Lift shaft: rings rising up it
            glm::vec3 c = (mn + mx) * 0.5f, sz = mx - mn;
            for (int k = 0; k < 6; ++k) {
                float y = mn.y + std::fmod(t * bo.speed * 0.35f + k * sz.y / 6.f, sz.y);
                float a = 1.f - std::fabs(y - c.y) / (sz.y * 0.5f);
                glm::vec3 g = col * (0.4f + 0.8f * a);
                push(out, T({c.x, y, mn.z + 0.05f}) * S({sz.x, 0.08f, 0.08f}), g * 0.2f, g);
                push(out, T({c.x, y, mx.z - 0.05f}) * S({sz.x, 0.08f, 0.08f}), g * 0.2f, g);
                push(out, T({mn.x + 0.05f, y, c.z}) * S({0.08f, 0.08f, sz.z}), g * 0.2f, g);
                push(out, T({mx.x - 0.05f, y, c.z}) * S({0.08f, 0.08f, sz.z}), g * 0.2f, g);
            }
            continue;
        }
        glm::vec3 side{-dir.z, 0.f, dir.x};
        glm::vec3 c = (mn + mx) * 0.5f;
        float len = std::fabs(glm::dot(mx - mn, dir));
        glm::vec3 start = c - dir * (len * 0.5f);
        start.y = mn.y + 0.04f;
        const float gap = 2.4f;
        float ph = std::fmod(t * bo.speed * 0.45f, gap);
        for (float u = ph; u < len; u += gap) {
            glm::vec3 tip = start + dir * u;
            float fade = std::min({1.f, u / 3.f, (len - u) / 3.f});
            glm::vec3 g = col * fade;
            for (float sgn : {1.f, -1.f}) {
                glm::vec3 arm = glm::normalize(-dir + side * sgn * 1.1f);
                push(out, T(tip + arm * 0.55f) * RY(std::atan2(arm.x, arm.z)) * S({0.16f, 0.03f, 1.1f}), g * 0.2f, g);
            }
        }
    }

    // Fans: four blades spinning in a housing ring
    for (auto& f : level.fans) {
        glm::mat4 base = T(f.pos) * (f.axis == 0 ? RZ(1.5708f) : f.axis == 2 ? RX(1.5708f) : glm::mat4(1.f));
        float spin = t * 7.f + f.pos.x;
        for (int k = 0; k < 4; ++k)
            push(out, base * RY(spin + k * 0.785f) * S({f.radius * 1.9f, 0.06f, f.radius * 0.28f}), {0.2f, 0.2f, 0.23f});
        push(out, base * S({0.5f, 0.18f, 0.5f}), {0.15f, 0.15f, 0.18f}, f.glow * 0.6f);
        for (int k = 0; k < 12; ++k) {
            float a = k * 0.5236f;
            push(out, base * T({std::cos(a) * f.radius, 0.f, std::sin(a) * f.radius}) * RY(-a) * S({0.16f, 0.16f, f.radius * 0.56f}),
                 f.glow * 0.15f, f.glow * 0.7f);
        }
    }

    // Pistons: a hexagonal sleeve and rod (three turned boxes), a heavy head
    for (auto& p : level.pistons) {
        float len = p.length(t);
        auto hex = [&](glm::vec3 c, float r, float h, glm::vec3 col, glm::vec3 emi) {
            for (int k = 0; k < 3; ++k) push(out, T(c) * RY(k * 1.0472f) * S({r * 1.73f, h, r}), col, emi);
        };
        hex(p.top - glm::vec3{0, 1.2f, 0}, p.radius * 1.5f, 2.4f, {0.22f, 0.2f, 0.2f}, glm::vec3{0.f});
        hex(p.top - glm::vec3{0, 2.4f + len * 0.5f, 0}, p.radius * 0.55f, len, {0.55f, 0.55f, 0.6f}, glm::vec3{0.f});
        glm::vec3 head = p.top - glm::vec3{0, 2.4f + len + 0.6f, 0};
        hex(head, p.radius * 1.3f, 1.2f, {0.3f, 0.22f, 0.18f}, glm::vec3{0.f});
        float hot = 1.f - (p.maxLen - len) / std::max(0.01f, p.maxLen - p.minLen);
        hex(head - glm::vec3{0, 0.62f, 0}, p.radius * 1.2f, 0.08f, p.glow * 0.2f, p.glow * (0.3f + 1.5f * hot));
    }
    for (auto& s : level.spinners) {
        float a0 = t * s.speed;
        float segLen = 6.2832f * s.radius / s.segs * 0.55f;
        for (int k = 0; k < s.segs; ++k) {
            float a = a0 + k * 6.2832f / s.segs;
            push(out, T(s.pos + glm::vec3{std::cos(a) * s.radius, 0.f, std::sin(a) * s.radius}) * RY(-a) * S({0.18f, 0.22f, segLen}),
                 s.glow * 0.15f, s.glow);
        }
    }

    // Jump pads: a plate, a glowing core, and a ring that rises off it
    for (auto& pad : level.pads) {
        glm::vec3 c = pad.centre;
        glm::vec2 h = pad.half;
        push(out, T(c + glm::vec3{0, 0.06f, 0}) * S({h.x * 2.f + 0.3f, 0.12f, h.y * 2.f + 0.3f}), {0.15f, 0.16f, 0.2f});
        float pulse = 0.7f + 0.3f * std::sin(t * 6.f);
        push(out, T(c + glm::vec3{0, 0.13f, 0}) * S({h.x * 1.4f, 0.04f, h.y * 1.4f}), {0.2f, 0.8f, 0.7f}, glm::vec3{0.3f, 1.4f, 1.1f} * pulse);
        for (int k = 0; k < 2; ++k) {
            float ph = std::fmod(t * 0.9f + k * 0.5f, 1.f);
            float y = 0.2f + ph * 2.2f, s = 1.f - ph * 0.5f, a = (1.f - ph);
            glm::vec3 e = glm::vec3{0.3f, 1.3f, 1.0f} * a;
            push(out, T(c + glm::vec3{0, y,  h.y * s}) * S({h.x * 2.f * s, 0.05f, 0.08f}), e * 0.2f, e);
            push(out, T(c + glm::vec3{0, y, -h.y * s}) * S({h.x * 2.f * s, 0.05f, 0.08f}), e * 0.2f, e);
            push(out, T(c + glm::vec3{ h.x * s, y, 0}) * S({0.08f, 0.05f, h.y * 2.f * s}), e * 0.2f, e);
            push(out, T(c + glm::vec3{-h.x * s, y, 0}) * S({0.08f, 0.05f, h.y * 2.f * s}), e * 0.2f, e);
        }
    }

    // Moving platforms: a slab with glowing edges and a grapple ring under it
    for (auto& m : level.movers) {
        AABB b = level.walls[m.wall].box;
        b.min -= m.delta * (1.f - renderAlpha); b.max -= m.delta * (1.f - renderAlpha);
        glm::vec3 c = (b.min + b.max) * 0.5f, sz = b.max - b.min;
        float pulse = 0.75f + 0.25f * std::sin(t * 4.f + c.x);
        push(out, T(c) * S(sz), m.color);
        glm::vec3 g = m.glow * pulse;
        float ey = b.max.y - 0.03f;
        push(out, T({c.x, ey, b.min.z}) * S({sz.x + 0.06f, 0.08f, 0.08f}), g * 0.2f, g);
        push(out, T({c.x, ey, b.max.z}) * S({sz.x + 0.06f, 0.08f, 0.08f}), g * 0.2f, g);
        push(out, T({b.min.x, ey, c.z}) * S({0.08f, 0.08f, sz.z + 0.06f}), g * 0.2f, g);
        push(out, T({b.max.x, ey, c.z}) * S({0.08f, 0.08f, sz.z + 0.06f}), g * 0.2f, g);
        float ry = b.min.y - 0.35f, rr = std::min(sz.x, sz.z) * 0.3f;
        for (int k = 0; k < 8; ++k) {
            float a = k * 0.785f + t * 1.5f;
            push(out, T({c.x + std::cos(a) * rr, ry, c.z + std::sin(a) * rr}) * RY(-a) * S({0.1f, 0.1f, rr * 0.8f}), g * 0.2f, g * 1.3f);
        }
    }

    // Lava shimmer over the channels
    for (auto& hz : level.hazards) {
        glm::vec3 c = (hz.box.min + hz.box.max) * 0.5f, sz = hz.box.max - hz.box.min;
        for (int i = 0; i < 8; ++i) {
            float x = hz.box.min.x + sz.x * (i + 0.5f) / 8.f;
            float w = 0.5f + 0.5f * std::sin(t * 2.f + i * 1.7f);
            push(out, T({x, c.y + 0.04f, c.z}) * S({sz.x / 8.f * 0.9f, 0.05f, sz.z * 0.6f}),
                 {1.f, 0.5f, 0.1f}, glm::vec3{1.6f, 0.7f, 0.12f} * (0.6f + 0.6f * w));
        }
    }

    // Spinning gems (the obelisk, the Spire's beacon, the finish)
    for (auto& g : level.gems) {
        float s = g.size;
        glm::vec3 col = g.color;
        push(out, T(g.pos + glm::vec3{0, std::sin(t * 1.5f) * 0.3f, 0}) * RY(t) * RX(0.785f) * RZ(0.785f) * S(glm::vec3{s}),
             col * 0.6f, col);
        if (g.beam) push(out, T(g.pos + glm::vec3{0, 40.f, 0}) * S({0.35f, 80.f, 0.35f}), col * 0.1f, col * 0.5f);
    }

    // FAST / ACT II: the finish beacon lights up once the last fight is won
    if (fast() || act2()) {
        glm::vec3 f = level.finishPos;
        glm::vec3 col = finishOpen ? glm::vec3{1.8f, 0.7f, 0.2f} : glm::vec3{0.3f, 0.12f, 0.1f};
        push(out, T(f + glm::vec3{0, 0.08f, 0}) * S({5.f, 0.16f, 5.f}), {0.15f, 0.12f, 0.12f});
        for (int k = 0; k < 16; ++k) {
            float a = k * 0.3927f + t * (finishOpen ? 1.2f : 0.2f);
            push(out, T(f + glm::vec3{std::cos(a) * 2.6f, 0.25f, std::sin(a) * 2.6f}) * RY(-a) * S({0.18f, 0.18f, 0.8f}), col * 0.2f, col);
        }
        if (finishOpen) push(out, T(f + glm::vec3{0, 30.f, 0}) * S({1.2f, 60.f, 1.2f}), col * 0.1f, col * 0.8f);
    }

    // The Core: the reactor — stacked counter-rotating blocks and orbiting rings
    if (level.hasReactor) {
        bool bossRage = false;
        for (auto& e : enemies) if (e.alive && e.type == EnemyType::WARDEN && e.enraged) bossRage = true;
        glm::vec3 base = level.reactorPos;
        glm::vec3 hot = bossRage ? glm::vec3{1.8f, 0.2f, 0.3f} : glm::vec3{0.3f, 1.4f, 1.8f};
        glm::vec3 alt = bossRage ? glm::vec3{1.6f, 0.4f, 0.1f} : glm::vec3{1.6f, 0.25f, 1.1f};
        // Overloading: it runs red, and flares as each ring winds up
        float flare = 1.f + 2.5f * shifts.warning();
        hot = glm::mix(hot, glm::vec3{1.9f, 0.25f, 0.15f}, shifts.alarm) * flare;
        alt = glm::mix(alt, glm::vec3{1.6f, 0.5f, 0.1f}, shifts.alarm) * flare;
        for (int i = 0; i < 6; ++i) {
            float y = 1.0f + i * 1.55f;
            float s = 2.6f - std::fabs(i - 2.5f) * 0.35f;
            float pulse = 0.6f + 0.4f * std::sin(t * 3.f + i);
            push(out, T(base + glm::vec3{0, y, 0}) * RY(t * (i % 2 ? 0.8f : -0.8f) + i) * S({s, 1.2f, s}),
                 {0.1f, 0.12f, 0.15f}, (i % 2 ? hot : alt) * 0.25f * pulse);
            push(out, T(base + glm::vec3{0, y + 0.66f, 0}) * RY(t * (i % 2 ? 0.8f : -0.8f) + i) * S({s + 0.1f, 0.12f, s + 0.1f}),
                 hot * 0.2f, hot * pulse);
        }
        push(out, T(base + glm::vec3{0, 30.f, 0}) * S({0.7f, 40.f, 0.7f}), hot * 0.2f, hot * 0.9f);   // beam to the sky
        for (int ringI = 0; ringI < 3; ++ringI) {
            float r = 3.6f + ringI * 1.6f, y = 3.5f + ringI * 3.f, spd = (ringI % 2 ? -0.6f : 0.5f);
            for (int k = 0; k < 14; ++k) {
                float a = k * 6.2832f / 14.f + t * spd;
                push(out, T(base + glm::vec3{std::cos(a) * r, y + std::sin(t * 2.f + k) * 0.15f, std::sin(a) * r})
                          * RY(-a) * S({0.2f, 0.2f, 0.7f}), alt * 0.2f, alt * 0.6f);
            }
        }
    }
}

inline void GameplayState::render() {
    applyTextureQuality();
    float alpha = (float)(accumulator / PHYSICS_DT);
    renderAlpha = glm::clamp(alpha, 0.f, 1.f);
    glm::vec3 renderCamPos = glm::mix(prevCamPos, player.camera.position, alpha);
    Camera renderCam = player.camera;
    renderCam.position = renderCamPos;

    if (shakeTimer > 0.f) {
        float s = shakeIntensity * (shakeTimer / 0.2f) * (1.f - 0.7f * aim);
        renderCam.position += glm::vec3{frand(-1.f,1.f) * s, frand(-1.f,1.f) * s, 0.f};
    }
    if (!settings || settings->viewBob)
        renderCam.position += viewModel.getBobOffset(playerXZSpeed, player.onGround, renderCam.right()) * (1.f - aim);
    renderCam.position.y -= landSquash;
    audio.setListener(renderCam.position, renderCam.right(), renderCam.forward());
    const WeaponDef& wd = weaponDef((WeaponId)activeWeapon);
    float baseFov = settings ? settings->fov : 90.f;
    float a = aim * aim * (3.f - 2.f * aim);
    float zoom = wd.canAim ? glm::mix(1.f, wd.aimFov, a) : 1.f;
    renderCam.fov = baseFov * zoom + fovKick * (1.f - a);

    glm::mat4 view = renderCam.viewMatrix();
    glm::mat4 proj = renderCam.projectionMatrix();
    Theme th = level.themeAt(player.position);
    if (rageBlend > 0.f) th = lerpTheme(th, bloodEclipse(th), rageBlend);

    postProcess.beginScene();
    glClearColor(th.fogColor.r, th.fogColor.g, th.fogColor.b, 1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // --- Sky ---
    {
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        skyboxShader.use();
        skyboxShader.setMat4("uInvProj", glm::inverse(proj));
        skyboxShader.setMat4("uInvView", glm::inverse(glm::mat4(glm::mat3(view))));
        skyboxShader.setVec3("uZenith",   th.zenith);
        skyboxShader.setVec3("uHorizon",  th.horizon);
        skyboxShader.setVec3("uGround",   th.ground);
        skyboxShader.setVec3("uSunDir",   th.sunDir);
        skyboxShader.setVec3("uSunColor", th.sunColor);
        skyboxShader.setFloat("uSunSize", th.sunSize);
        skyboxShader.setFloat("uSunStripes", th.sunStripes);
        skyboxShader.setVec3("uMountain", th.mountain);
        skyboxShader.setFloat("uStars",   th.stars);
        glBindVertexArray(postProcess.quadVAO);
        glDrawArrays(GL_TRIANGLES,0,6);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
    }

    // --- Static world ---
    worldShader.use();
    worldShader.setMat4("projection", proj);
    worldShader.setMat4("view",       view);
    worldShader.setMat4("model",      glm::mat4(1.f));
    applyLighting(worldShader, th, renderCamPos);
    worldShader.setVec3 ("emissiveColor", {0.f,0.f,0.f});
    worldShader.setVec3 ("objectColor",   {1.f,1.f,1.f});
    worldShader.setFloat("uPSXStrength",  0.0f);
    worldShader.setFloat("uVertexGlow",   0.0f);
    worldShader.setInt("uTexture",0);
    glActiveTexture(GL_TEXTURE0);
    for (int t = 0; t < TEX_COUNT; ++t) {
        if (world.byTex[t].indexCount == 0) continue;
        glBindTexture(GL_TEXTURE_2D, worldTex[t]);
        world.byTex[t].draw();
    }
    glBindTexture(GL_TEXTURE_2D, whiteTex);
    worldShader.setVec3("objectColor", {0.25f,0.25f,0.25f});
    worldShader.setFloat("uVertexGlow", 1.5f);
    world.neon.draw();
    worldShader.setFloat("uVertexGlow", 0.0f);
    worldShader.setVec3("objectColor", {1.f,1.f,1.f});

    // --- Everything made of boxes: one instanced draw ---
    {
        static std::vector<BoxInstance> boxes;
        boxes.clear();
        gatherBoxes(boxes, view);
        boxRenderer.shader.use();
        boxRenderer.shader.setMat4("projection", proj);
        boxRenderer.shader.setMat4("view",       view);
        boxRenderer.shader.setInt ("uTexture",   0);
        applyLighting(boxRenderer.shader, th, renderCamPos);
        glBindTexture(GL_TEXTURE_2D, whiteTex);
        boxRenderer.draw(boxes);
    }

    renderWater(view, proj, th, renderCamPos);

    worldShader.use();
    worldShader.setMat4("model", glm::mat4(1.f));
    grapple.drawLine(renderCamPos, view, proj);
    renderDecals();

    renderTracers(view, proj);
    renderLasers(view, proj);
    renderFlare(view, proj);
    renderTethers(view, proj);
    renderParticles(view, proj);
    bool warm = warmupFrames > 0;
    if (warm) warmPipelines(renderCamPos, view, proj);

    // --- View model (inside the FBO so it gets bloom; no fog). Hidden
    // behind the Longshot's scope once it's up. ---
    if (!scopedView()) {
        worldShader.use();
        worldShader.setVec3("lightDir",    glm::normalize(glm::vec3{0.4f,-1.f,0.3f}));
        worldShader.setVec3("lightColor",  {1.f,0.9f,0.8f});
        worldShader.setVec3("uSkyAmb",     {0.32f,0.32f,0.38f});
        worldShader.setVec3("uGroundAmb",  {0.12f,0.11f,0.10f});
        worldShader.setFloat("uFogDensity", 0.f);
        worldShader.setVec3("viewPos",     renderCamPos);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, whiteTex);
        worldShader.setInt("uTexture", 0);
        float flash = glm::clamp(muzzleFlashTimer / 0.06f, 0.f, 1.f);
        viewModel.draw(worldShader, renderCam, activeWeapon, flash, wd.canAim ? aim : 0.f,
                       weapons[activeWeapon].ammo, weaponMag((WeaponId)activeWeapon, prog.up[activeWeapon]));
        worldShader.setMat4("projection", proj);
        worldShader.setMat4("view",       view);
    }

    postProcess.crtEnabled = settings ? settings->crtFilter : false;
    postProcess.endScene();

    renderHUD(view, proj);
    if (warm) { ui.warmScope(); --warmupFrames; }
}

inline void GameplayState::renderDecals() {
    struct DVert { float x,y,z, u,v, nx,ny,nz, r,g,b; };
    DVert buf[Effects::MAX_DECALS * 6];
    int count = 0;
    for (auto& d : fx.decals) {
        if (!d.alive) continue;
        float fade = d.life / d.maxLife;
        float r = 0.55f * fade, g = 0.03f * fade, b = 0.03f * fade;
        float s = 0.35f;
        float px = d.pos.x, py = d.pos.y, pz = d.pos.z;
        DVert v0{px-s,py,pz-s, 0,0, 0,1,0, r,g,b};
        DVert v1{px+s,py,pz-s, 1,0, 0,1,0, r,g,b};
        DVert v2{px+s,py,pz+s, 1,1, 0,1,0, r,g,b};
        DVert v3{px-s,py,pz+s, 0,1, 0,1,0, r,g,b};
        buf[count++] = v0; buf[count++] = v1; buf[count++] = v2;
        buf[count++] = v0; buf[count++] = v2; buf[count++] = v3;
    }
    if (count == 0) return;
    glBindBuffer(GL_ARRAY_BUFFER, decalVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(DVert), buf);
    worldShader.use();
    worldShader.setMat4("model",         glm::mat4(1.f));
    worldShader.setVec3("objectColor",   {1.f,1.f,1.f});
    worldShader.setVec3("emissiveColor", {0.f,0.f,0.f});
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.f, -1.f);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(decalVAO);
    glDrawArrays(GL_TRIANGLES, 0, count);
    glBindVertexArray(0);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_CULL_FACE);
}

inline void GameplayState::renderParticles(const glm::mat4& view, const glm::mat4& proj) {
    static PVert buf[Effects::MAX_PARTICLES];
    int count = 0;
    for (auto& p : fx.particles) {
        if (!p.alive) continue;
        float t = p.life / p.maxLife;
        buf[count++] = { p.pos.x, p.pos.y, p.pos.z, p.color.r, p.color.g, p.color.b, t * t };
    }
    drawPoints(buf, count, view, proj);
}

inline void GameplayState::drawPoints(const PVert* buf, int count, const glm::mat4& view, const glm::mat4& proj) {
    if (count == 0) return;
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(PVert), buf);

    particleShader.use();
    particleShader.setMat4("projection", proj);
    particleShader.setMat4("view",       view);
    particleShader.setFloat("uPointScale", 60.f);

    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
#ifndef __EMSCRIPTEN__
    glEnable(GL_PROGRAM_POINT_SIZE);  // always on in GLES/WebGL2
#endif
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBindVertexArray(particleVAO);
    glDrawArrays(GL_POINTS, 0, count);
    glBindVertexArray(0);
#ifndef __EMSCRIPTEN__
    glDisable(GL_PROGRAM_POINT_SIZE);
#endif
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

// Standing water: one translucent quad per volume at its current level, lit
// red from beneath and pale where the eclipse light falls on it
inline void GameplayState::renderWater(const glm::mat4& view, const glm::mat4& proj, const Theme& th, glm::vec3 camPos) {
    if (level.water.empty()) return;
    static float buf[MAX_WATER * 18];
    int n = 0;
    for (auto& w : level.water) {
        if (n + 18 > MAX_WATER * 18) break;
        float x0 = w.box.min.x, x1 = w.box.max.x, z0 = w.box.min.z, z1 = w.box.max.z, y = w.level;
        const float q[18] = {x0,y,z0, x1,y,z1, x1,y,z0,  x0,y,z0, x0,y,z1, x1,y,z1};
        std::memcpy(buf + n, q, sizeof(q)); n += 18;
    }
    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, n * sizeof(float), buf);
    waterShader.use();
    waterShader.setMat4("projection", proj);
    waterShader.setMat4("view", view);
    waterShader.setFloat("uTime", gameClock);
    waterShader.setVec3("uViewPos", camPos);
    waterShader.setVec3("uDeep", {0.03f, 0.1f, 0.11f});
    waterShader.setVec3("uGlow", {0.55f, 0.07f, 0.04f});
    waterShader.setVec3("uFogColor", th.fogColor);
    waterShader.setFloat("uFogDensity", th.fogDensity);
    waterShader.setVec2("uShaftXZ", {0.f, -607.f});   // the Nave's crossing
    waterShader.setFloat("uShaftR", 4.f);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(waterVAO);
    glDrawArrays(GL_TRIANGLES, 0, n / 3);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

inline void GameplayState::warmPipelines(glm::vec3 camPos, const glm::mat4& view, const glm::mat4& proj) {
    glm::vec3 f = camPos + player.camera.forward() * 30.f;
    std::vector<Beam> beam{{glm::vec4(f, 0.f), glm::vec4(f + glm::vec3{0, 1, 0}, 0.f), 0.01f}};
    drawBeams(beam, {0.f, 0.f, 0.f}, view, proj);
    PVert pt{f.x, f.y, f.z, 0.f, 0.f, 0.f, 0.f};
    drawPoints(&pt, 1, view, proj);
    if (!grapple.active) {   // a zero-length rope
        glm::vec3 keep = grapple.target;
        grapple.active = true;
        grapple.target = camPos + glm::vec3{0.f, 1.4f, 0.f};
        grapple.drawLine(camPos, view, proj);
        grapple.active = false;
        grapple.target = keep;
    }
}

inline void GameplayState::drawBeams(const std::vector<Beam>& beams, glm::vec3 color, const glm::mat4& view, const glm::mat4& proj) {
    struct TVert { float x,y,z,a; };
    TVert buf[Effects::MAX_TRACERS * 6];
    int count = 0;
    for (auto& bm : beams) {
        if (count + 6 > Effects::MAX_TRACERS * 6) break;
        glm::vec3 s = glm::vec3(bm.a), e = glm::vec3(bm.b);
        glm::vec3 ray = e - s;
        float len = glm::length(ray);
        if (len < 0.001f) continue;
        glm::vec3 dir = ray / len;
        // Side axis perpendicular to the ray (cross with world up; fall back to X)
        glm::vec3 side = glm::cross(dir, glm::vec3(0.f, 1.f, 0.f));
        if (glm::length(side) < 0.01f) side = glm::cross(dir, glm::vec3(1.f, 0.f, 0.f));
        side = glm::normalize(side) * bm.width;
        float aN = bm.a.w, aF = bm.b.w;
        glm::vec3 s0 = s - side, s1 = s + side, e0 = e - side, e1 = e + side;
        buf[count++] = {s0.x,s0.y,s0.z, aN}; buf[count++] = {s1.x,s1.y,s1.z, aN};
        buf[count++] = {e1.x,e1.y,e1.z, aF}; buf[count++] = {s0.x,s0.y,s0.z, aN};
        buf[count++] = {e1.x,e1.y,e1.z, aF}; buf[count++] = {e0.x,e0.y,e0.z, aF};
    }
    if (count == 0) return;
    glBindBuffer(GL_ARRAY_BUFFER, tracerVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(TVert), buf);
    tracerShader.use();
    tracerShader.setMat4("projection", proj);
    tracerShader.setMat4("view",       view);
    tracerShader.setVec3("uColor",     color);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBindVertexArray(tracerVAO);
    glDrawArrays(GL_TRIANGLES, 0, count);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

inline void GameplayState::renderTracers(const glm::mat4& view, const glm::mat4& proj) {
    // One batch per colour: each gun's tracers take its glow
    static std::vector<Beam> beams;
    static std::vector<glm::vec3> colours;
    colours.clear();
    for (auto& t : fx.tracers)
        if (t.alive && std::find_if(colours.begin(), colours.end(), [&](const glm::vec3& c) { return glm::length(c - t.color) < 1e-3f; }) == colours.end())
            colours.push_back(t.color);
    for (const auto& col : colours) {
        beams.clear();
        for (auto& t : fx.tracers) {
            if (!t.alive || glm::length(t.color - col) >= 1e-3f) continue;
            float fade = t.life / t.maxLife;
            beams.push_back({glm::vec4(t.start, fade), glm::vec4(t.end, fade * 0.08f), t.width});
        }
        drawBeams(beams, col, view, proj);
    }
}

inline void GameplayState::renderLasers(const glm::mat4& view, const glm::mat4& proj) {
    static std::vector<Beam> beams;
    beams.clear();
    glm::vec3 target = player.camera.position - glm::vec3{0, 0.35f, 0};
    for (auto& e : enemies) {
        if (!e.targetable() || e.type != EnemyType::SENTINEL || e.attack != AttackKind::BURST) continue;
        float k = e.telegraphProgress();
        float a = 0.25f + 0.75f * k;
        if (k > 0.8f) a *= 0.5f + 0.5f * std::sin(gameClock * 60.f);
        glm::vec3 eye = e.position + glm::vec3{0, e.height() * 0.85f, 0};
        // Stop a few metres short so the beam doesn't smear across the screen
        glm::vec3 d = target - eye;
        float len = glm::length(d);
        if (len < 4.f) continue;
        beams.push_back({glm::vec4(eye, a), glm::vec4(eye + d * ((len - 3.f) / len), a * 0.5f), 0.03f});
    }
    drawBeams(beams, {0.3f, 0.95f, 1.f}, view, proj);
    // SERAPHS: a thin aim line while charging, then the thick sweeping beam
    static std::vector<Beam> aims, sweeps;
    aims.clear(); sweeps.clear();
    for (auto& e : enemies) {
        if (!e.targetable() || e.type != EnemyType::SERAPH) continue;
        glm::vec3 eye = (e.hasShown ? e.shownPos : e.position) + glm::vec3{0, e.height() * 0.6f, 0};
        if (e.attack == AttackKind::BEAM && e.telegraphTimer > 0.f) {
            float a = 0.3f + 0.6f * e.telegraphProgress();
            aims.push_back({glm::vec4(eye, a), glm::vec4(e.beamPoint, a * 0.6f), 0.025f});
        } else if (e.beamTimer > 0.f) {
            float f = 0.85f + 0.15f * std::sin(gameClock * 40.f);
            sweeps.push_back({glm::vec4(eye, f), glm::vec4(e.beamEnd, f), 0.22f});
            sweeps.push_back({glm::vec4(eye, f * 0.5f), glm::vec4(e.beamEnd, f * 0.5f), 0.5f});
        }
    }
    drawBeams(aims, {1.f, 0.85f, 0.5f}, view, proj);
    drawBeams(sweeps, {1.4f, 1.15f, 0.7f}, view, proj);
}

// SOLAR: each arm of the sun's flare as a fan of beams across its wedge, from
// the sun out over the terrace: faint while it gathers, blazing while it burns
inline void GameplayState::renderFlare(const glm::mat4& view, const glm::mat4& proj) {
    const Arena& ar = level.arenas[director.arena];
    if (ar.shift != ArenaShift::SOLAR || !director.fighting()) return;
    FlarePhase ph = shifts.flarePhase();
    if (ph == FlarePhase::OFF) return;
    static std::vector<Beam> fan;
    fan.clear();
    bool burn = ph == FlarePhase::BURN;
    float a = burn ? 0.5f : 0.18f + 0.12f * std::sin(gameClock * 12.f);
    glm::vec3 c{ar.sunPos.x, ar.playerStart.y + 0.6f, ar.sunPos.z};   // just over the terrace
    for (int arm = 0; arm < shifts.flareArms; ++arm)
        for (int k = 0; k < 7; ++k) {
            float ang = shifts.flareAngle + arm * 6.2831853f / shifts.flareArms
                      + (k - 3) / 3.f * ArenaShifts::FLARE_HALF;
            glm::vec3 dir{std::cos(ang), 0.f, std::sin(ang)};
            fan.push_back({glm::vec4(c + dir * 9.f, a), glm::vec4(c + dir * 52.f, a * 0.6f), burn ? 1.1f : 0.8f});
        }
    drawBeams(fan, burn ? glm::vec3{1.6f, 1.25f, 0.7f} : glm::vec3{1.f, 0.7f, 0.3f}, view, proj);
}

// CONDUCTOR tethers: a flickering line from each conductor to every ally it shields
inline void GameplayState::renderTethers(const glm::mat4& view, const glm::mat4& proj) {
    static std::vector<Beam> beams;
    beams.clear();
    for (auto& c : enemies) {
        if (!c.alive || c.type != EnemyType::CONDUCTOR) continue;
        glm::vec3 from = (c.hasShown ? c.shownPos : c.position) + glm::vec3{0, 0.3f, 0};
        for (int k = 0; k < c.linkCount; ++k) {
            const Enemy& o = enemies[c.links[k]];
            if (!o.alive) continue;
            glm::vec3 to = (o.hasShown ? o.shownPos : o.position) + glm::vec3{0, o.height() * 0.6f, 0};
            float a = 0.75f + 0.25f * std::sin(gameClock * 13.f + k * 2.f);
            beams.push_back({glm::vec4(from, a), glm::vec4(to, a * 0.85f), 0.1f});
        }
    }
    drawBeams(beams, {0.3f, 1.f, 0.9f}, view, proj);
}

