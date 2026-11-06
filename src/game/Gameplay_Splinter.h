#pragma once
// =============================================================================
// Gameplay_Splinter.h — GameplayState: UNIT 24's SPLINTER (player/Splinter.h,
// docs/chapter-4-design.md). Throwing one, moving them, a round that hits one
// being sent on to the next splinter or the best weak point in sight, and
// drawing them.
// Owned with --splinter for now; Story unlocks it after Chapter 3.
// Included at the end of GameplayState.h.
// =============================================================================

// (Story: UNIT 24 takes the forearm that holds it off the Successor at the end of Chapter 3, so Chapter 4 has it)
inline bool GameplayState::splinterOwned() const { return g_devSplinter || plain() || (training() && lessons::usesSplinter(trainingLesson)); }

static const glm::vec3 SPLINTER_GLOW{0.35f, 1.f, 1.15f};
static constexpr int SPLINTER_BROOD = -2;   // splinterTarget: the Gleaner's brood, not an enemy in the list

inline void GameplayState::throwSplinter() {
    int i = splinters.toss(player.camera.position, player.camera.forward(), player.camera.right(), player.velocity);
    if (i < 0) { if (dryCd <= 0.f) { audio.play("dry", 60, SoundGroup::PLAYER); dryCd = 0.25f; } return; }
    viewModel.triggerGrenade();   // the hand flicks forward
    SfxMixer::Opts o; o.group = SoundGroup::PLAYER; o.pitch = 1.7f; o.volume = 0.8f;
    audio.playOpts("casing", o);
    teachSplinterOnce();
}

// A short hint the first two times one is thrown
inline void GameplayState::teachSplinterOnce() {
    if (splinterHints >= 2) return;
    ++splinterHints;
    ui.toast("SPLINTER", "SHOOT IT: THE ROUND FINDS A WEAK POINT", SPLINTER_GLOW, 2.2f);
}

inline void GameplayState::tickSplinters(float dt) {
    // It breaks on a wall or the floor it flies into
    auto solid = [&](glm::vec3 a, glm::vec3 b) {
        glm::vec3 d = b - a;
        float len = glm::length(d);
        if (len < 1e-5f) return false;
        return wallAlong(a, d / len, len + 0.05f) > 0.f;
    };
    unsigned broke = splinters.tick(dt, solid);
    for (int i = 0; i < SplinterSystem::POOL; ++i) {
        Splinter& s = splinters.pool[i];
        if (broke & (1u << i)) {
            fx.spawnBurst(s.pos, SPLINTER_GLOW, 8, 3.f, 0.3f, 8.f);
            audio.playAt("clank", s.pos, 40, SoundGroup::WORLD, false, 2.2f);
            continue;
        }
        bool glint = SplinterSystem::glinting(s);
        if (glint && !splinterGlinted[i]) {   // it has reached the top: the moment it hits hardest
            SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.positional = true; o.pos = s.pos;
            o.pitch = 2.f; o.volume = 0.45f;
            audio.playOpts("dash_ready", o);
        }
        splinterGlinted[i] = s.alive && glint;
    }
}

// Which enemy a round sent on from `from` goes for, and the point it aims at:
// the nearest weak point in sight (a head counts as eight metres nearer).
// -1: nothing in reach.
inline int GameplayState::splinterTarget(glm::vec3 from, glm::vec3& aimAt) const {
    int best = -1;
    float bestScore = 1e9f;
    for (int ei = 0; ei < (int)enemies.size(); ++ei) {
        const Enemy& en = enemies[ei];
        if (!en.targetable()) continue;
        glm::vec3 off = en.hasShown ? en.shownPos - en.position : glm::vec3{0.f};
        if (glm::dot(off, off) > 9.f) off = glm::vec3{0.f};
        AABB body = en.getAABB();
        glm::vec3 point = (body.min + body.max) * 0.5f + off;
        AABB head;
        bool weak = headBox(en, head);
        if (weak) point = (head.min + head.max) * 0.5f + off;
        glm::vec3 to = point - from;
        float d = glm::length(to);
        if (d < 0.3f || d > SplinterSystem::REACH) continue;
        float wall = wallAlong(from, to / d, d);
        if (wall > 0.f && wall < d - 0.3f) continue;   // out of its sight
        float score = d - (weak ? 8.f : 0.f);
        if (score < bestScore) { bestScore = score; best = ei; aimAt = point; }
    }
    glm::vec3 brood;   // the brood's weak point (the Tender's lip, the Sieve's heart, the Caller's lamp) counts as a head
    if (broodSplinterPoint(brood)) {
        glm::vec3 to = brood - from;
        float d = glm::length(to), wall = d > 0.3f ? wallAlong(from, to / d, d) : -1.f;
        if (d > 0.3f && d < SplinterSystem::REACH && !(wall > 0.f && wall < d - 0.3f) && d - 8.f < bestScore) { best = SPLINTER_BROOD; aimAt = brood; }
    }
    return best;
}

// A round (doing `dmg`, from gun `w`) has hit splinter `first`. It breaks and
// the round goes on from where it was: splinter to splinter, then into a weak point.
inline void GameplayState::splinterRicochet(int first, float dmg, int w) {
    WeaponId id = (WeaponId)w;
    const WeaponUpgrades& u = prog.up[w];
    unsigned used = 0;
    int cur = first, link = 0;
    bool landed = false;
    static std::vector<RayHit> hits;
    for (;;) {
        Splinter& s = splinters.pool[cur];
        bool glint = SplinterSystem::glinting(s);
        float mult = SplinterSystem::damageMult(link, glint) * SplinterSystem::heavyRoundScale(weaponDef(id).damage);
        glm::vec3 from = s.pos;
        used |= 1u << cur;
        s.alive = false;
        fx.spawnBurst(from, SPLINTER_GLOW, glint ? 22 : 14, glint ? 7.f : 5.f, 0.35f, 4.f);
        audio.playAt("parry", from, glint ? 110 : 85, SoundGroup::PLAYER, true, 1.35f + 0.12f * (float)link);

        // Another splinter in sight: hop to it (the chain)
        int next = -1;
        if (link + 1 < SplinterSystem::MAX_LINKS) {
            float bestD = SplinterSystem::REACH;
            for (int j = 0; j < SplinterSystem::POOL; ++j) {
                const Splinter& o = splinters.pool[j];
                if (!o.alive || o.age < SplinterSystem::ARMED_AFTER || (used & (1u << j))) continue;
                glm::vec3 to = o.pos - from;
                float d = glm::length(to);
                if (d < 0.1f || d >= bestD) continue;
                float wall = wallAlong(from, to / d, d);
                if (wall > 0.f && wall < d - 0.1f) continue;
                bestD = d; next = j;
            }
        }
        if (next >= 0) {
            fx.spawnTracer(from, splinters.pool[next].pos, 0.07f, 0.35f, SPLINTER_GLOW * 1.4f);
            ++link; cur = next;
            continue;
        }

        // The weak point it can see
        glm::vec3 aimAt;
        int target = splinterTarget(from, aimAt);
        if (target < 0 && target != SPLINTER_BROOD) {   // nothing to go for: it spends itself upward
            fx.spawnTracer(from, from + glm::vec3{0.f, 6.f, 0.f}, 0.05f, 0.25f, SPLINTER_GLOW);
            break;
        }
        glm::vec3 dir = glm::normalize(aimAt - from);
        if (target == SPLINTER_BROOD) {   // into the brood's weak point (the glass finds the gaps)
            float near = SplinterSystem::REACH + 5.f;
            fx.spawnTracer(from, aimAt, 0.08f, 0.35f, SPLINTER_GLOW * 1.6f);
            landed = shootBrood(from, dir, near, dmg * mult, w, true);
            break;
        }
        float wallT = hitscanAll(from, dir, SplinterSystem::REACH + 5.f, hits);
        if (hits.empty()) { fx.spawnTracer(from, from + dir * wallT, 0.07f, 0.3f, SPLINTER_GLOW * 1.4f); break; }
        const RayHit& h = hits[0];
        Enemy& e = enemies[h.enemy];
        glm::vec3 at = from + dir * h.t;
        fx.spawnTracer(from, at, 0.08f, 0.35f, SPLINTER_GLOW * 1.6f);
        if (e.deflects(dir, h.t)) {   // the Sovereign's guard still turns it aside
            fx.spawnHitSparks(at, {1.f, 0.85f, 0.4f});
            audio.playAt("clank", at, 100, SoundGroup::ENEMY, true, 0.6f);
            e.onDeflect();
            break;
        }
        if (!h.head && e.blocks(dir)) { fx.spawnHitSparks(at, {0.4f, 1.f, 0.75f}); break; }
        if (h.head && e.halo) breakHalo(e);
        float m = h.wound ? e.woundMult() : h.head ? weaponHeadMult(id, u) : weaponBodyMult(id, u);
        if (h.zone != LevZone::NONE) {
            m = leviathanZoneMult(e, h.zone);
            if (h.zone == LevZone::THROAT) throatHit(e, dmg * mult * m * e.armorMult());
        }
        fx.spawnHitSparks(at, SPLINTER_GLOW * 1.5f);
        fx.spawnSpray(at, -dir, e.stats().color * 1.6f + glm::vec3{0.4f, 0.3f, 0.2f}, 8, 8.f, 0.22f);
        HitPush hp{dir, hitreact::gunPush(w) * 1.3f, hitreact::gunPoise(w) * 1.5f};
        hurtEnemy(e, dmg * mult * m, at, 14.f + 6.f * (float)link, 2.f, weaponSource(id), true, false, &hp);
        landed = true;
        break;
    }

    // The trick is worth style either way; landing it is worth more
    float style = (landed ? 25.f : 10.f) + 15.f * (float)link;
    styleSystem.addStyle(style, weaponSource(id));
    if (link > 0) {
        char buf[24]; std::snprintf(buf, sizeof(buf), "CHAIN x%d", link + 1);
        ui.feed(buf, SPLINTER_GLOW);
    } else if (landed) ui.feed("SPLINTERED", SPLINTER_GLOW);
    if (landed) {
        trainingSuccess(lessons::SPLINTER);
        if (link > 0) trainingSuccess(lessons::SPLINTER_CHAIN);
        hitStopFrames = glm::max(hitStopFrames, 2 + std::min(link, 2));
    }
}

// The live ones: a long cyan diamond spinning on its own axis; at the top of
// its arc it flares into a four-pointed glint, the moment to shoot it
inline void GameplayState::gatherSplinters(std::vector<BoxInstance>& out) {
    using namespace rig;
    for (int i = 0; i < SplinterSystem::POOL; ++i) {
        const Splinter& s = splinters.pool[i];
        if (!s.alive) continue;
        glm::vec3 at = lerpPos(s.prev, s.pos);
        float spin = s.spin + renderAlpha * (1.f / 60.f) * 15.f;
        bool glint = SplinterSystem::glinting(s);
        float pulse = glint ? 1.6f + 0.6f * std::sin(gameClock * 40.f) : 1.f;
        glm::mat4 base = T(at) * RY(spin) * RX(0.35f) * RZ(0.785f);
        push(out, base * S(glm::vec3{0.16f, 0.16f, 0.55f}), {0.25f, 0.6f, 0.65f}, SPLINTER_GLOW * 2.4f * pulse);
        push(out, T(at) * RY(-spin * 0.6f) * RX(0.785f) * S(glm::vec3{0.09f, 0.09f, 0.09f}), glm::vec3{1.f}, glm::vec3{3.f} * pulse);
        if (glint) {   // the glint: two thin bars across it, facing every way it is seen from
            float L = 1.1f + 0.25f * std::sin(gameClock * 40.f);
            push(out, T(at) * RY(gameClock * 2.f) * S(glm::vec3{L, 0.035f, 0.035f}), glm::vec3{1.f}, SPLINTER_GLOW * 3.f);
            push(out, T(at) * RY(gameClock * 2.f) * S(glm::vec3{0.035f, L, 0.035f}), glm::vec3{1.f}, SPLINTER_GLOW * 3.f);
            push(out, T(at) * RY(gameClock * 2.f) * S(glm::vec3{0.035f, 0.035f, L}), glm::vec3{1.f}, SPLINTER_GLOW * 3.f);
        }
    }
}
