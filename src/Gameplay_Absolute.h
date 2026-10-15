#pragma once
// =============================================================================
// Gameplay_Absolute.h — GameplayState: THE ABSOLUTE's fight in the Zenith.
// His events turned into hazards and damage (AbsoluteHazards.h), parries (the
// punch, or the sword's parry as a white strike lands), the pillars that
// shatter and the shards that fall, his afterimages, his lines on screen, and
// his death on his knees. Included at the end of GameplayState.h.
// =============================================================================

inline Enemy* GameplayState::absolute() {
    for (auto& e : enemies) if (e.alive && e.type == EnemyType::ABSOLUTE) return &e;
    return nullptr;
}

// The Zenith as his mind sees it: its centre, the standing pillars, a shard underfoot
inline void GameplayState::fillZenithWorld(EnemyWorld& w) {
    w.hasZenith = level.zenith.built && act2() && director.arena == ZENITH_ARENA;
    if (!w.hasZenith) return;
    zenPillarBoxes.clear();
    for (size_t i = 0; i < level.zenith.pillarWalls.size(); ++i)
        if (level.zenith.pillarUp[i]) zenPillarBoxes.push_back(level.walls[level.zenith.pillarWalls[i]].box);
    w.zenCentre = level.zenith.centre; w.zenFloor = level.zenith.centre.y;
    w.zenPillars = zenPillarBoxes.data(); w.zenPillarCount = (int)zenPillarBoxes.size();
    w.onShard = level.onZenithShard(player.position);
}

// A white strike parried (the punch, or the sword): he reels, breaks, or his blade shatters
inline bool GameplayState::parryAbsolute(Enemy& e, bool bySword) {
    int r = e.abParried(bySword);
    if (!r) return false;
    viewModel.triggerParry(true);
    parryFeedback(e.position + glm::vec3{0.f, 1.6f, 0.f}, r >= 2);
    styleSystem.addStyle(r >= 2 ? 80.f : 40.f, StyleSource::PARRY);
    styleSystem.heal((bySword ? Sword::HEAL_PARRY : 4.f) * tune().heal);
    gainXp(r >= 2 ? 40 : 15);
    audio.playAt("ab_ting", e.position, 128, SoundGroup::ENEMY, true);
    if (r == 1) ui.feed("REELING", {1.f, 1.f, 1.f});
    if (r == 2) { ui.toast("BROKEN", "THREE SECONDS - DOUBLE DAMAGE", {1.f, 0.95f, 0.7f}, 1.6f); shake(0.4f, 0.06f); }
    if (r == 3) {
        ui.toast("HIS BLADE SHATTERS", "THREE SECONDS - DOUBLE DAMAGE", {1.f, 0.4f, 0.55f}, 1.8f); shake(0.6f, 0.08f);
        fx.spawnBurst(e.position + glm::vec3{0.f, 1.8f, 0.f}, {1.6f, 1.6f, 1.7f}, 60, 14.f, 0.7f, 6.f);
    }
    return true;
}

inline void GameplayState::onAbsoluteEvents(Enemy& e, const EnemyEvents& ev) {
    const float scale = level.arenas[director.arena].damageScale * tune().damage;
    const glm::vec3 white{1.6f, 1.6f, 1.7f}, red{1.5f, 0.2f, 0.15f};
    if (ev.abTell == (int)AbTell::WHITE) audio.playAt("ab_ting", e.position, 110, SoundGroup::ENEMY, true);
    if (ev.abTell == (int)AbTell::GOLD)  audio.playAt("ab_thrum", e.position, 128, SoundGroup::ENEMY, true);
    if (ev.abTell == (int)AbTell::RED)   audio.playAt("telegraph", e.position, 128, SoundGroup::ENEMY, true);
    if (ev.abLine >= 0) { abLineShown = ev.abLine; abLineT = 1.6f; }
    if (ev.blinked) {   // a fading white copy where he was
        Enemy g = e; g.position = g.prevPosition = ev.blinkFrom; g.hitFlashTimer = 0.f;
        abGhosts.push_back({g, 0.f});
        fx.spawnBurst(ev.blinkFrom + glm::vec3{0.f, 1.3f, 0.f}, white, 10, 4.f, 0.3f, 0.f);
        audio.playAt("dash", ev.blinkFrom, 90, SoundGroup::ENEMY);
    }
    if (ev.abPhase == 2) { pushBanner("HE ASCENDS", "STAY CLOSE", {1.f, 0.95f, 0.8f}, 3.f); shake(0.5f, 0.06f); audio.duck(6.f, 0.6f); }
    if (ev.abPhase == 3) { pushBanner("THE BLADE OF LIGHT", "MEET IT", {1.f, 0.5f, 0.6f}, 3.f); shake(0.5f, 0.06f); audio.duck(6.f, 0.6f); }
    if (ev.abHalo) { abh.addHalo(level.zenith.centre, level.zenith.centre.y); audio.play("explosion", 100, SoundGroup::WORLD); }
    if (ev.abShards)
        for (int k = 0; k < (int)level.zenith.shardWalls.size(); ++k)
            if (level.zenith.shardUp[k]) { level.parkZenithPiece(true, k); shardFallT[k] = 0.f; }
    if (ev.abBladeDrawn) { fx.spawnBurst(e.position + glm::vec3{0.f, 2.f, 0.f}, white, 30, 8.f, 0.5f, 0.f); audio.playAt("sword_draw", e.position, 128, SoundGroup::ENEMY, true); }
    if (ev.abRay) { abh.addRay(ev.abFrom, ev.abTo); audio.playAt("explosion", ev.abFrom, 80, SoundGroup::ENEMY, true, 1.6f); }
    if (ev.abStomp) { abh.addStomp(ev.abFrom); fx.spawnShockwave(ev.abFrom, 3.f, red); audio.playAt("slam", ev.abFrom, 128, SoundGroup::ENEMY, true); shake(0.4f, 0.06f); }
    if (ev.abSerpent) {
        glm::vec3 d = player.camera.position - ev.abFrom;
        abh.addSerpent(ev.abFrom, glm::length(d) > 0.1f ? d : glm::vec3{0.f, 0.f, 1.f});
    }
    if (ev.abSeams) abh.addSeams(level.zenith.centre, ev.abSeamFirst);
    if (ev.abZenith) { abh.addZenith(ev.abFrom, ev.abTo); audio.playAt("explosion", ev.abTo, 90, SoundGroup::ENEMY, true, 1.4f); }
    for (int k = 0; k < ev.abMirages; ++k) abh.addMirage(ev.phantomPos[k]);
    if (ev.abJudgeMark) { abh.addJudge(level.zenith.centre, e.position); shake(0.8f, 0.04f); audio.duck(8.f, 2.f); }
    if (ev.abJudge) {
        fx.spawnShockwave(e.position, 44.f, {1.6f, 1.5f, 1.2f});
        shake(1.0f, 0.1f);
        ui.toast("SPENT", "FOUR SECONDS - DOUBLE DAMAGE", {1.f, 0.95f, 0.7f}, 2.f);
    }
    if (ev.meleeHit) {   // his fists, feet or blade: the sword can meet them
        AbTell t = abTellOf(e.lastAbAttack);
        glm::vec3 from = e.position + glm::vec3{0.f, 1.5f, 0.f};
        bool hit = damagePlayerStrike(ev.meleeDamage * scale, from, t == AbTell::GOLD ? Sword::Strike::GOLD : Sword::Strike::WHITE);
        if (hit && lastGuarded == Sword::Guarded::NONE) {
            glm::vec3 away = player.position - e.position; away.y = 0.f;
            if (glm::length(away) > 0.01f) player.velocity += glm::normalize(away) * 7.f + glm::vec3{0.f, 2.5f, 0.f};
            if (ev.abLaunch) { player.velocity.y = 11.f; player.onGround = false; }   // KNEEL: launched for the drop-kick
        }
        audio.playAt("punch", from, 120, SoundGroup::ENEMY, true);
    }
}

inline void GameplayState::updateAbsolute(float dt) {
    for (auto& g : abGhosts) g.t += dt;
    abGhosts.erase(std::remove_if(abGhosts.begin(), abGhosts.end(), [](const AbGhost& g) { return g.t > AB_GHOST_LIFE; }), abGhosts.end());
    for (auto& t : shardFallT) if (t >= 0.f) t += dt;
    for (auto& t : pillarFallT) if (t >= 0.f) t += dt;
    if (abLineT > 0.f) abLineT -= dt;
    if (abHintT > 0.f) abHintT -= dt;
    updateAbsoluteDeath(dt);
    if (!(level.zenith.built && act2() && director.arena == ZENITH_ARENA)) return;
    Enemy* e = absolute();
    const glm::vec3 bp = e ? e->position : level.zenith.centre;
    const float scale = level.arenas[director.arena].damageScale * tune().damage;
    // A ray just fired, met by the sword's parry: it goes back into him
    if (e && swordOut && !abh.rays.empty() && abh.rays.front().t <= AbsoluteHazards::RAY_PARRY && sword.parryOpen()) {
        glm::vec3 to = abh.rays.front().from - player.camera.position;
        if (glm::dot(glm::normalize(to), player.camera.forward()) > 0.3f && sword.tryParry()) {
            abh.rayParry(0);
            hurtEnemy(*e, AbsoluteHazards::RAY_REFLECT, bp + glm::vec3{0.f, 1.6f, 0.f}, 50.f, Sword::HEAL_PARRY, StyleSource::PARRY);
            parryFeedback(player.camera.position + player.camera.forward() * 1.2f, true);
            ui.toast("RAY RETURNED", "", {1.f, 0.9f, 0.9f}, 1.2f);
        }
    }
    float h = player.height;
    for (auto& hit : abh.update(dt, player.position, h, player.onGround, &level, bp))
        damagePlayerStrike(hit.damage * scale, hit.from, hit.kind, hit.tick ? dt : 0.f);
    if (int m = abh.haloShatterMask())
        for (int k = 0; k < (int)level.zenith.pillarWalls.size(); ++k) if (m & (1 << k)) {
            glm::vec3 p = zenithPillar(k);
            level.parkZenithPiece(false, k);
            pillarFallT[k] = 0.f;
            fx.spawnBurst(p + glm::vec3{0.f, 7.f, 0.f}, {0.9f, 0.88f, 0.85f}, 80, 12.f, 1.2f, 14.f);
            audio.playAt("explosion", p, 128, SoundGroup::WORLD);
            shake(0.3f, 0.05f);
        }
    // A serpent within reach of a parry: the punch's, or the sword's window
    int s = abh.serpentNear(player.camera.position, 2.4f);
    if (s >= 0 && ((swordOut && sword.tryParry()) || punchedThisTick)) {
        abh.reflectSerpent(s);
        parryFeedback(abh.serpents[s].pos, false);
        styleSystem.addStyle(30.f, StyleSource::PARRY);
    }
    if (e) for (int n = abh.reflectedHits(bp + glm::vec3{0.f, 1.3f, 0.f}, 1.6f); n > 0; --n) {
        hurtEnemy(*e, AbsoluteHazards::SERPENT_PARRY, bp + glm::vec3{0.f, 1.3f, 0.f}, 40.f, 4.f, StyleSource::PARRY);
        fx.spawnBurst(bp + glm::vec3{0.f, 1.3f, 0.f}, {1.6f, 1.6f, 1.7f}, 30, 8.f, 0.4f, 0.f);
    }
    punchedThisTick = false;
}

// Before his tick: a white strike about to land, met by the sword's parry window
inline void GameplayState::swordParryAbsolute(Enemy& e) {
    if (!swordOut || !sword.parryOpen() || !e.parryWindow()) return;
    glm::vec3 to = e.position - player.position; to.y = 0.f;
    float d = glm::length(to);
    glm::vec3 f = player.camera.forward(); f.y = 0.f;
    if (d > Enemy::AB_REACH + 1.8f || (d > 0.3f && glm::dot(to / d, glm::normalize(f + glm::vec3{1e-4f, 0.f, 0.f})) < 0.2f)) return;
    if (sword.tryParry()) parryAbsolute(e, true);
}

// His death: he kneels, his halo cracks, "...WORTHY.", and he goes up in light
inline void GameplayState::onAbsoluteKilled(const Enemy& e) {
    abh.clear();
    abCorpse = e; abCorpse.alive = true; abCorpse.staggerTimer = 1.f; abCorpse.abReeling = false;
    abCorpse.attack = AttackKind::NONE; abCorpse.telegraphTimer = 0.f; abCorpse.abBlade = false;
    abDeathT = 0.f;
    abLineShown = 4; abLineT = 3.f;
    fx.spawnShockwave(e.position, 20.f, {1.6f, 1.5f, 1.2f});
    explosionFlashTimer = 0.8f; explosionFlashPos = e.position;
    audio.duck(10.f, 2.f);
}
inline void GameplayState::updateAbsoluteDeath(float dt) {
    if (abDeathT >= AB_DEATH) return;
    abDeathT += dt;
    abCorpse.prevPosition = abCorpse.position;
    if (abDeathT > 1.f && std::fmod(abDeathT, 0.12f) < dt)   // the halo cracking, then light pouring up off him
        fx.spawnBurst(abCorpse.position + glm::vec3{frand(-0.5f, 0.5f), frand(0.3f, 2.4f), frand(-0.5f, 0.5f)},
                      {1.6f, 1.5f, 1.2f}, 12, 4.f, 0.8f, -6.f);
    if (abDeathT >= AB_DEATH) fx.spawnBurst(abCorpse.position + glm::vec3{0.f, 1.3f, 0.f}, {1.6f, 1.6f, 1.7f}, 120, 16.f, 1.4f, -4.f);
}

// ---- drawing: pillars, shards, his tells and hazards, mirages, afterimages, his death ----
inline void GameplayState::gatherAbsoluteBoxes(std::vector<BoxInstance>& out) {
    using namespace rig;
    if (!level.zenith.built || !act2() || director.arena != ZENITH_ARENA) return;
    const float t = gameClock;
    const glm::vec3 C = level.zenith.centre;
    const glm::vec3 white{0.88f, 0.87f, 0.84f}, crack{1.4f, 1.3f, 1.05f}, gold{1.5f, 1.1f, 0.45f}, red{2.4f, 0.25f, 0.15f};
    // a long thin box from a to b
    auto along = [&](glm::vec3 a, glm::vec3 b, float w, glm::vec3 col, glm::vec3 emi) {
        glm::vec3 d = b - a; float l = glm::length(d);
        if (l < 1e-3f) return;
        d /= l;
        float yaw = std::atan2(d.x, d.z), pitch = -std::asin(glm::clamp(d.y, -1.f, 1.f));
        push(out, T((a + b) * 0.5f) * RY(yaw) * RX(pitch) * S({w, w, l}), col, emi);
    };
    // pillars: white columns with glowing cracks and a capital; a shattered one falls apart
    for (int k = 0; k < (int)level.zenith.pillarHome.size(); ++k) {
        const AABB& b = level.zenith.pillarHome[k];
        glm::vec3 c{(b.min.x + b.max.x) * 0.5f, b.min.y, (b.min.z + b.max.z) * 0.5f};
        if (level.zenith.pillarUp[k]) {
            push(out, T(c + glm::vec3{0.f, 7.f, 0.f}) * S({2.5f, 14.f, 2.5f}), white);
            push(out, T(c + glm::vec3{0.f, 14.2f, 0.f}) * S({3.1f, 0.5f, 3.1f}), white * 1.05f);
            push(out, T(c + glm::vec3{0.f, 0.25f, 0.f}) * S({3.f, 0.5f, 3.f}), white * 0.9f);
            for (int j = 0; j < 4; ++j)   // cracks of light up its faces
                push(out, T(c + glm::vec3{0.f, 3.f + 2.6f * j, 0.f}) * RY(j * 1.5707963f) * T({0.f, 0.f, 1.26f}) * RZ(0.3f * (j & 1 ? 1.f : -1.f))
                          * S({0.06f, 2.2f, 0.02f}), crack * 0.4f, crack * (1.f + 0.2f * std::sin(t * 2.f + j)));
        } else if (pillarFallT[k] >= 0.f && pillarFallT[k] < 1.6f) {
            float u = pillarFallT[k];
            for (int j = 0; j < 5; ++j) {
                glm::vec3 off{std::sin(j * 2.1f + k) * u * 3.f, 2.f + 2.6f * j - 9.f * u * u, std::cos(j * 1.7f + k) * u * 3.f};
                push(out, T(c + off) * RY(j + u * 2.f) * RX(u * (j + 1)) * S({2.2f, 2.4f, 2.2f}), white, crack * 0.3f * (1.f - u / 1.6f));
            }
        }
    }
    // shards: floating islands, edged with light; at his blade they drop away
    for (int k = 0; k < (int)level.zenith.shardHome.size(); ++k) {
        const AABB& b = level.zenith.shardHome[k];
        glm::vec3 c{(b.min.x + b.max.x) * 0.5f, b.max.y, (b.min.z + b.max.z) * 0.5f};
        float drop = 0.f, fade = 1.f;
        if (!level.zenith.shardUp[k]) {
            if (shardFallT[k] < 0.f || shardFallT[k] > 3.f) continue;
            drop = 4.5f * shardFallT[k] * shardFallT[k]; fade = 1.f - shardFallT[k] / 3.f;
        }
        glm::mat4 m = T(c - glm::vec3{0.f, drop, 0.f}) * RY(k * 0.9f + 0.3f);
        push(out, m * T({0.f, -1.25f, 0.f}) * S({8.f, 2.5f, 8.f}), glm::vec3{0.08f, 0.08f, 0.09f});
        push(out, m * T({0.f, -3.f, 0.f}) * S({5.f, 1.5f, 5.f}), glm::vec3{0.1f, 0.1f, 0.11f});
        push(out, m * T({0.f, 0.02f, 0.f}) * S({7.6f, 0.04f, 7.6f}), white * 0.7f);
        for (int j = 0; j < 4; ++j)
            push(out, m * RY(j * 1.5707963f) * T({0.f, 0.01f, 3.95f}) * S({7.9f, 0.08f, 0.1f}), crack * 0.3f, crack * fade);
    }
    // the floor's seams under the seams attack and the last judgment
    auto cell = [&](float x, float z, glm::vec3 emi) {
        push(out, T(C + glm::vec3{x + 3.f, 0.03f, z + 3.f}) * S({5.6f, 0.02f, 5.6f}), emi * 0.2f, emi);
    };
    for (auto& s : abh.seams) {
        float u0 = glm::clamp(s.t / AbsoluteHazards::SEAM_WARN, 0.f, 1.f);
        float u1 = glm::clamp((s.t - AbsoluteHazards::SEAM_WARN) / AbsoluteHazards::SEAM_GAP, 0.f, 1.f);
        for (float x = -42.f; x < 42.f; x += 6.f)
            for (float z = -42.f; z < 42.f; z += 6.f) {
                if ((x + 3.f) * (x + 3.f) + (z + 3.f) * (z + 3.f) > 43.f * 43.f) continue;
                int par = AbsoluteHazards::seamParity(C, C + glm::vec3{x + 3.f, 0.f, z + 3.f});
                float u = par == s.first ? (s.done0 ? 0.f : u0) : (s.done1 ? 0.f : (s.done0 ? u1 : 0.f));
                if (u > 0.02f) cell(x, z, red * (0.3f + 1.2f * u * u));
            }
    }
    for (auto& j : abh.judges) {
        float u = glm::clamp(j.t / AbsoluteHazards::JUDGE_WARN, 0.f, 1.f);
        for (float x = -42.f; x < 42.f; x += 6.f)
            for (float z = -42.f; z < 42.f; z += 6.f) {
                glm::vec3 p = C + glm::vec3{x + 3.f, 0.f, z + 3.f};
                if ((x + 3.f) * (x + 3.f) + (z + 3.f) * (z + 3.f) > 43.f * 43.f) continue;
                if (glm::length(glm::vec2{p.x - j.boss.x, p.z - j.boss.z}) < AbsoluteHazards::JUDGE_SAFE) continue;
                cell(x, z, red * (0.4f + 1.4f * u));
            }
        for (int k = 0; k < 40; ++k) {   // the safe ring round him, gold
            float a = k * 0.1570796f, R = AbsoluteHazards::JUDGE_SAFE;
            push(out, T(j.boss + glm::vec3{std::cos(a) * R, 0.08f, std::sin(a) * R}) * RY(-a) * S({0.15f, 0.1f, R * 0.17f}), gold * 0.3f, gold * 2.f);
        }
    }
    // rays: the red line of a tell, then the beam
    if (Enemy* e = absolute()) {
        if (e->attack == AttackKind::ARAY && e->telegraphTimer > 0.f) {
            glm::vec3 from = e->position + glm::vec3{0.f, 1.6f, 0.f}, to = e->abAim + glm::vec3{0.f, 1.f, 0.f};
            glm::vec3 d = glm::normalize(to - from + glm::vec3{0.f, 1e-4f, 0.f});
            along(from, from + d * 60.f, 0.04f + 0.05f * e->telegraphProgress(), red * 0.2f, red * (0.6f + 1.2f * e->telegraphProgress()));
        }
        if (e->attack == AttackKind::AZENITH && e->telegraphTimer > 0.f)   // the spot it will burn, marked
            for (int k = 0; k < 24; ++k) {
                float a = k * 0.2617994f, R = AbsoluteHazards::ZRAY_R;
                push(out, T(e->abAim + glm::vec3{std::cos(a) * R, 0.06f, std::sin(a) * R}) * RY(-a) * S({0.12f, 0.06f, R * 0.28f}), red * 0.2f, red * 1.5f);
            }
        if (e->attack == AttackKind::ASTOMP && e->telegraphTimer > 0.f)
            for (int k = 0; k < 24; ++k) {
                float a = k * 0.2617994f, R = 1.2f + 1.5f * e->telegraphProgress();
                push(out, T(e->position + glm::vec3{std::cos(a) * R, 0.06f, std::sin(a) * R}) * RY(-a) * S({0.12f, 0.06f, R * 0.28f}), red * 0.2f, red * 1.5f);
            }
    }
    for (auto& r : abh.rays) {
        glm::vec3 d = glm::normalize(r.at - r.from + glm::vec3{0.f, 1e-4f, 0.f});
        float flick = 0.85f + 0.15f * std::sin(t * 40.f);
        along(r.from, r.from + d * 60.f, 0.5f, red * 0.3f, red * 1.2f * flick);
        along(r.from, r.from + d * 60.f, 0.22f, glm::vec3{1.f}, glm::vec3{2.4f, 2.2f, 2.f} * flick);
    }
    for (auto& z : abh.zrays) {
        glm::vec3 top{z.at.x, z.halo.y, z.at.z};
        along(top, z.at, AbsoluteHazards::ZRAY_R * 1.6f, red * 0.3f, red * 1.3f);
        along(top, z.at, AbsoluteHazards::ZRAY_R * 0.6f, glm::vec3{1.f}, glm::vec3{2.6f, 2.4f, 2.2f});
    }
    for (auto& s : abh.stomps)
        for (int k = 0; k < 48; ++k) {
            float a = k * 0.1308997f;
            push(out, T(s.at + glm::vec3{std::cos(a) * s.r, 0.35f, std::sin(a) * s.r}) * RY(-a) * S({0.2f, 0.7f, s.r * 0.14f}), red * 0.2f, red * 1.4f);
        }
    for (auto& h : abh.halos) {   // the 360 sweep: a bar of light at chest height, turning
        float a = 6.2831853f * std::min(1.f, h.t / AbsoluteHazards::HALO_TIME);
        glm::vec3 d{std::cos(a), 0.f, std::sin(a)}, y{0.f, (AbsoluteHazards::HALO_LO + AbsoluteHazards::HALO_HI) * 0.5f, 0.f};
        along(h.centre + y, h.centre + y + d * AbsoluteHazards::HALO_REACH, 0.5f, red * 0.3f, red * 1.4f);
        along(h.centre + y, h.centre + y + d * AbsoluteHazards::HALO_REACH, 0.2f, glm::vec3{1.f}, glm::vec3{2.4f, 2.2f, 2.f});
    }
    for (auto& s : abh.serpents)   // a coil of light: a head and a writhing tail
        for (int k = 0; k < 7; ++k) {
            glm::vec3 side = glm::normalize(glm::cross(s.dir, glm::vec3{0.f, 1.f, 0.f}) + glm::vec3{1e-4f});
            glm::vec3 p = s.pos - s.dir * (0.35f * k) + side * (0.25f * std::sin(t * 12.f - k * 0.9f));
            float sz = 0.42f - 0.045f * k;
            push(out, T(p) * RY(t * 3.f + k) * S(glm::vec3{sz}), glm::vec3{1.f}, (s.back ? gold : glm::vec3{1.9f, 1.9f, 2.f}) * (1.6f - 0.15f * k));
        }
    // mirages and afterimages: his rig, emissive only, fading
    auto ghost = [&](Enemy pose, float alpha, glm::vec3 tint) {
        pose.hitFlashTimer = 0.f; pose.spawnTimer = 0.f;
        size_t from = out.size();
        buildEnemy(pose, t, out);
        for (size_t j = from; j < out.size(); ++j) { out[j].color = glm::vec3{0.06f}; out[j].emissive = tint * alpha; }
    };
    Enemy* boss = absolute();
    for (auto& m : abh.mirages) {
        Enemy pose = boss ? *boss : abCorpse;
        pose.position = m.at; pose.staggerTimer = 0.f; pose.abBlade = false;
        glm::vec3 to = player.position - m.at; pose.yaw = std::atan2(to.x, to.z);
        pose.attack = AttackKind::AJAB; pose.telegraphDuration = AbsoluteHazards::MIRAGE_TELL; pose.telegraphTimer = std::max(0.01f, AbsoluteHazards::MIRAGE_TELL - m.t);
        ghost(pose, 0.55f + 0.25f * std::sin(t * 30.f + m.at.x), glm::vec3{1.5f, 1.5f, 1.7f});
    }
    for (auto& g : abGhosts) ghost(g.pose, 1.f - g.t / AB_GHOST_LIFE, glm::vec3{1.4f, 1.4f, 1.6f});
    if (abDeathT < AB_DEATH) {   // on his knees, then rising away in light
        Enemy pose = abCorpse;
        float up = std::max(0.f, abDeathT - 1.8f);
        pose.position += glm::vec3{0.f, up * up * 3.f, 0.f};
        size_t from = out.size();
        buildEnemy(pose, t, out);
        float fade = 1.f - glm::clamp((abDeathT - 1.8f) / (AB_DEATH - 1.8f), 0.f, 1.f);
        for (size_t j = from; j < out.size(); ++j) { out[j].color *= fade; out[j].emissive = out[j].emissive * fade + glm::vec3{1.5f} * (1.f - fade); }
    }
}

// His lines, as subtitles; a word on the sword when he arrives
inline void GameplayState::renderAbsoluteHUD() {
    if (abLineT > 0.f && abLineShown >= 0) {
        float a = std::min(1.f, abLineT / 0.4f);
        ui.begin2D();
        ui.ui.textShadow(abLine(abLineShown), SCREEN_W / 2, (int)(SCREEN_H * 0.62f), 5, {1.f, 0.98f, 0.92f, a}, true);
        ui.end2D();
    }
    if (abHintT > 0.f && !swordOut) {
        ui.begin2D();
        ui.ui.textShadow("PRESS 5 - THE SWORD: RMB PARRIES WHITE, BLOCKS GOLD - EVERY CUT HEALS", SCREEN_W / 2, 82, 2,
                         {1.f, 0.55f, 0.7f, std::min(1.f, abHintT)}, true);
        ui.end2D();
    }
}
