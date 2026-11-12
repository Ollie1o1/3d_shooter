#pragma once
// =============================================================================
// Gameplay_RingFoes.h — GameplayState: Chapter 6's DISPATCHED and the sweep
// (their minds: enemies/EnemyRing.h), and the drifting chunks drawn.
//
//   STRAYS   while the posts are lit, each is handed the nearest lit post's
//            spot and runs for it; one that hits you during a countdown shoves
//            you out of the lee, hard (it wants that shelter).
//   HERDERS  during a countdown each takes the loose shelter nearest you that
//            nobody else is shoving, flies to its far side and pushes it along
//            its rail (a DRIVEN mover) and off your line. Kill it and it stops.
//   TALLIES  counted in tickSweep: while one stands, the gap between sweeps halves.
//   THE CROOK  opens by hooking a STRAY out of the reef and reeling it in; then
//            it throws its hook at you (sooner while a countdown runs). Caught,
//            you are dragged to it, out of any shelter, unless HOLDFAST holds
//            you. It breaks off at a third gone (EnemyRing.h).
// =============================================================================

inline void GameplayState::ringFoes(float dt) {
    const bool lit = sweep.active() && sweep.lethal();
    std::vector<int> taken;
    crookTick(dt);
    for (auto& e : enemies) {
        if (!e.alive) continue;
        if (e.type == EnemyType::STRAY) {
            e.ringSeekSet = false; e.ringFaceSet = false;
            if (!lit) continue;
            if (e.uid % 4 == 1) { e.ringFace = sweep.toSource(e.position); e.ringFaceSet = true; continue; }   // it won't run (13.6)
            float best = 60.f;
            for (const SweepShelter& s : sweepShelters) {
                if (!sweep.shelterLit(s)) continue;
                const float d = glm::length(glm::vec2{s.post.x - e.position.x, s.post.z - e.position.z});
                if (d < best && std::fabs(s.post.y - e.position.y) < 2.5f) { best = d; e.ringSeek = s.post - sweep.spec.dir * 0.8f; e.ringSeekSet = true; }
            }
        } else if (e.type == EnemyType::HERDER) {
            e.ringPush = -1;
            if (!lit || e.stunned() || !e.targetable()) continue;
            float best = 45.f; int pick = -1;
            for (int i = 0; i < (int)sweepShelters.size(); ++i) {
                const SweepShelter& s = sweepShelters[i];
                if (s.mover < 0 || !s.alive || std::find(taken.begin(), taken.end(), i) != taken.end()) continue;
                const Mover& m = level.movers[s.mover];
                if (m.drive >= 0.999f) continue;
                const glm::vec3 c = (s.box.min + s.box.max) * 0.5f;
                const float d = glm::length(glm::vec2{c.x - player.position.x, c.z - player.position.z});
                if (d < best) { best = d; pick = i; }
            }
            if (pick < 0) continue;
            taken.push_back(pick);
            const SweepShelter& s = sweepShelters[pick];
            Mover& m = level.movers[s.mover];
            const glm::vec3 c = (s.box.min + s.box.max) * 0.5f, half = (s.box.max - s.box.min) * 0.5f;
            const glm::vec3 dir = glm::normalize(m.b - m.a);
            e.ringPush = pick;
            e.ringPushAt = c - dir * (std::fabs(dir.x) * half.x + std::fabs(dir.z) * half.z + 1.4f);
            e.ringPushAt.y = s.box.min.y;
            if (glm::length(glm::vec2{e.ringPushAt.x - e.position.x, e.ringPushAt.z - e.position.z}) < ringfoe::PUSH_REACH) {
                m.drive = std::min(1.f, m.drive + dt * 0.3f);   // about three seconds to shove it clear
                if (std::fmod(gameClock, 0.1f) < dt) {
                    fx.spawnBurst(glm::vec3{c.x, s.box.min.y + 0.2f, c.z} + dir * half.x, SWEEP_ICE * 0.6f, 3, 3.f, 0.7f, -0.5f);
                    audio.playAt("skim", c, 70, SoundGroup::ENEMY, true, 0.6f);
                }
            }
        }
    }
}

// THE CROOK, each tick: its card, its opening (a stray hooked out of the reef), its line, and you
// on the end of it
inline void GameplayState::crookTick(float dt) {
    const glm::vec3 lineCol = HOLDFAST_STEEL * 1.5f;
    for (auto& c : enemies) {
        if (!c.alive || c.type != EnemyType::CROOK) continue;
        const glm::vec3 hand = c.position + glm::vec3{std::sin(c.yaw) * 0.9f, 1.1f, std::cos(c.yaw) * 0.9f};
        if (c.state == EnemyState::ACTIVE && !crookSeen) {
            crookSeen = true;
            pushBanner("THE CROOK", "IT KEEPS THE FLOCK IN LINE", HOLDFAST_STEEL, 3.f);
            audio.playAt("hook_yank", c.position, 110, SoundGroup::ENEMY, true, 0.6f);
        }
        if (c.crookLeave >= 0.f) {   // a third gone: it breaks off, toward the Shepherd
            if (!crookLeft) {
                crookLeft = true;
                ui.toast("THE CROOK", "IT BREAKS OFF, TOWARD THE SHEPHERD. IT WILL BE BACK", HOLDFAST_STEEL, 3.5f);
                audio.playAt("hook_yank", c.position, 110, SoundGroup::ENEMY, true, 0.6f);
            }
            continue;
        }
        // Its opening: the nearest STRAY hooked and reeled in, in front of you
        if (!c.crookOpened && c.targetable() && c.age > 0.8f) {
            c.crookOpened = true;
            Enemy* best = nullptr; float bd = 45.f;
            for (auto& s : enemies)
                if (s.alive && s.type == EnemyType::STRAY && s.targetable()) {
                    const float d = glm::length(s.position - c.position);
                    if (d < bd) { bd = d; best = &s; }
                }
            if (best) {
                const glm::vec3 fwd = glm::normalize(glm::vec3{best->position.x - c.position.x, 0.f, best->position.z - c.position.z} + glm::vec3{1e-4f});
                best->yank(c.position + fwd * 2.2f);
                c.crookDrag = best->uid; c.attackTimer = std::max(c.attackTimer, 2.f);
                audio.playAt("hook_yank", best->position, 120, SoundGroup::ENEMY, true, 0.6f);
                fx.spawnHitSparks(best->position + glm::vec3{0.f, 1.2f, 0.f}, HOLDFAST_STEEL);
            }
        }
        if (c.crookDrag >= 0) {
            Enemy* s = enemyByUid(c.crookDrag);
            if (!s || !s->alive || s->pullT <= 0.f) c.crookDrag = -1;
            else fx.spawnTracer(hand, s->position + glm::vec3{0.f, 1.2f, 0.f}, 0.035f, dt * 1.5f, lineCol);
        }
        // The throw: its line paying out toward where the hook will land, over the last half second
        if (c.attack == AttackKind::RING_HOOK && c.telegraphTimer > 0.f && c.telegraphTimer < 0.5f) {
            const float k = 1.f - c.telegraphTimer / 0.5f;
            const glm::vec3 at = glm::mix(hand, c.crookAim + glm::vec3{0.f, 1.f, 0.f}, k);
            fx.spawnTracer(hand, at, 0.035f, dt * 1.5f, c.parryWindow() ? glm::vec3{2.4f} : lineCol);
        }
        // During a countdown it wants you out of your shelter: it throws sooner
        if (sweep.active() && sweep.lethal() && sweep.phase == RingSweep::Phase::INBOUND) c.attackTimer = std::min(c.attackTimer, 0.8f);
    }
    // You, hooked: hauled toward it (up and over what is in the way), unless the spike holds you
    if (crookDragT > 0.f) {
        Enemy* c = enemyByUid(crookDragUid);
        if (!c || !c->alive || c->crookLeave >= 0.f || playerDead || holdfast.held()) {
            if (c && holdfast.held() && !playerDead) ui.feed("THE LINE HOLDS", HOLDFAST_STEEL);
            crookDragT = 0.f; crookDragUid = -1;
        } else {
            crookDragT -= dt;
            glm::vec3 d{c->position.x - player.position.x, 0.f, c->position.z - player.position.z};
            const float n = glm::length(d);
            if (n > 2.6f) { player.velocity = d / n * 16.f + glm::vec3{0.f, 3.5f, 0.f}; player.launched(); }
            else crookDragT = 0.f;
            fx.spawnTracer(c->position + glm::vec3{0.f, 1.1f, 0.f}, player.camera.position - glm::vec3{0.f, 0.4f, 0.f}, 0.035f, dt * 1.5f, lineCol);
        }
    }
}

inline void GameplayState::ringFoeEvents(Enemy& e, const EnemyEvents& ev) {
    if (e.type == EnemyType::DROVER && ev.ringCharge && !playerDead) gapLaunch(e);
    if (e.type == EnemyType::CROOK && ev.ringHooked && !playerDead) {   // the hook has you
        if (holdfast.held()) {   // the line holds: it caught, and you didn't move
            ui.feed("THE LINE HOLDS", HOLDFAST_STEEL);
            fx.spawnHitSparks(player.camera.position - glm::vec3{0.f, 0.5f, 0.f}, HOLDFAST_STEEL);
            audio.play("clank", 110, SoundGroup::PLAYER, true);
        } else {
            damagePlayer(9.f, e.position + glm::vec3{0.f, 1.5f, 0.f}, 0.25f, 0.035f, 0.f, {EnemyType::CROOK, DamageKind::MELEE});
            crookDragT = 0.7f; crookDragUid = e.uid;
            grapple.release();
            ui.feed("HOOKED", HOLDFAST_STEEL);
            audio.play("hook_yank", 120, SoundGroup::PLAYER, true);
        }
    }

    // A Stray's blow during a countdown throws you out of its way (low gravity: a long way)
    if (e.type == EnemyType::STRAY && ev.meleeHit && sweep.active() && sweep.lethal() && !playerDead) {
        glm::vec3 side{player.position.x - e.position.x, 0.f, player.position.z - e.position.z};
        if (glm::length(side) < 0.01f) side = {1.f, 0.f, 0.f};
        side = glm::normalize(side);
        player.velocity += side * 9.f + glm::vec3{0.f, 4.f, 0.f};
        player.launched();
        ui.feed("SHOVED OUT", SWEEP_ICE);
    }
    if (e.type == EnemyType::HERDER && ev.dashStarted) audio.playAt("dash", e.position, 110, SoundGroup::ENEMY, true, 0.7f);
}

// The drifting chunks (Mover::look 1): a slab of ice you stand on, lumps under and round it
// so it reads as a broken piece of the ring, not a platform
inline void GameplayState::gatherRing(std::vector<BoxInstance>& out) {
    using namespace rig;
    const float ball = (float)meshlib::lumpBall();
    const glm::vec3 eye = player.camera.position;
    for (const Mover& m : level.movers) {
        if (m.look < 1 || m.gone) continue;
        AABB bx = level.walls[m.wall].box;
        bx.min -= m.delta * (1.f - renderAlpha); bx.max -= m.delta * (1.f - renderAlpha);
        const glm::vec3 c = (bx.min + bx.max) * 0.5f, sz = bx.max - bx.min;
        if (glm::length(c - eye) > 900.f) continue;
        const float k = m.base.min.x * 0.37f + m.base.min.z * 0.11f;
        const glm::vec3 col = m.color * 0.72f;   // (the sun is hard out here: keep the ice off white)
        if (m.look == 2) {   // a boulder or a loose block: a lump of ice that fills its box and never leaves it
            push(out, T(c) * S(sz * 0.96f), col);
            out.back().surface = 6.f;
            for (int i = 0; i < 3; ++i) {   // rounded off with lumps that stay inside (what you see is what you hit)
                BoxInstance bi; bi.color = col * (0.86f + 0.08f * i); bi.mesh = ball;
                const float u = (i - 1) * 0.3f;
                bi.model = T(glm::vec3{c.x + u * sz.x, c.y + 0.05f * sz.y * i, c.z}) * RY(ring::hash(k + i, 6.f) * 6.28f) * S(glm::vec3{sz.x * 0.42f, sz.y * 0.98f, sz.z * 0.98f});
                out.push_back(bi);
            }
            continue;
        }
        push(out, T(c) * S(sz), col);
        out.back().surface = 6.f;   // (distant ground: a grain that travels with it)
        push(out, T(glm::vec3{c.x, bx.max.y + 0.005f, c.z}) * S(glm::vec3{sz.x * 0.98f, 0.01f, sz.z * 0.98f}), glm::mix(col, glm::vec3{0.5f, 0.43f, 0.33f}, 0.4f));
        // Underneath and at the ends: lumps, so the edges are ragged and it hangs over nothing
        const int lumps = 3 + (int)(sz.x / 5.f);
        for (int i = 0; i < lumps; ++i) {
            const float u = (i + 0.5f) / lumps - 0.5f;
            const float r = 0.5f + 0.5f * ring::hash(k + i, 3.f);
            BoxInstance bi; bi.color = col * (0.82f + 0.12f * ring::hash(k + i, 4.f)); bi.mesh = ball;
            bi.model = T(glm::vec3{c.x + u * sz.x, bx.min.y - sz.y * 0.1f * r, c.z + (ring::hash(k + i, 5.f) - 0.5f) * sz.z * 0.4f})
                     * RY(ring::hash(k + i, 6.f) * 6.28f) * S(glm::vec3{sz.x / lumps * 1.7f, sz.y * (1.2f + 1.4f * r), sz.z * (0.9f + 0.3f * r)});
            out.push_back(bi);
        }
        for (int e = -1; e <= 1; e += 2) {   // its two ends, rounded off (pulled in: nothing to stand on that isn't there)
            BoxInstance bi; bi.color = col * 0.92f; bi.mesh = ball;
            bi.model = T(glm::vec3{c.x + e * (sz.x * 0.5f - sz.z * 0.3f), c.y - sz.y * 0.1f, c.z}) * RY(k + e) * S(glm::vec3{sz.z * 0.6f, sz.y * 1.0f, sz.z * 0.98f});
            out.push_back(bi);
        }
    }
    // The HOLDFAST capsule: a glint on its spike until it's taken, so you find it from the lanes
    if (!holdfastFound && !g_devRingTest) {
        const float on = 0.6f + 0.4f * std::sin(gameClock * 3.f);
        push(out, T(ring::holdfastCapsule() + glm::vec3{2.f, 1.5f, 0.f}) * S(glm::vec3{0.25f}), HOLDFAST_STEEL, HOLDFAST_STEEL * 2.5f * on);
        push(out, T(ring::holdfastCapsule() + glm::vec3{2.f, 12.f, 0.f}) * S(glm::vec3{0.05f, 20.f, 0.05f}), glm::vec3{0.f}, HOLDFAST_STEEL * 0.5f * on);
    }
    // The hull's berths: the one still occupied, its lamp counting
    if (!g_devRingTest) {
        const AABB H = ring::quietHull();
        const glm::vec3 lamp{H.min.x + 1.3f, 1.25f, H.min.z + 3.f};
        if (glm::length(lamp - eye) < 120.f) {
            const float beat = std::fmod(gameClock, 1.12f) < 0.15f ? 1.f : 0.15f;
            push(out, T(lamp) * S(glm::vec3{0.18f}), {1.f, 0.7f, 0.3f}, glm::vec3{2.6f, 1.4f, 0.35f} * beat);
            push(out, T(lamp + glm::vec3{0.f, -0.1f, 0.4f}) * S(glm::vec3{0.9f, 0.5f, 1.6f}), glm::vec3{0.75f, 0.77f, 0.77f});   // its berth's lid, shut
        }
    }
}
