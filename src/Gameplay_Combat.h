#pragma once
// =============================================================================
// Gameplay_Combat.h — GameplayState: Enemies, damage both ways, kills, XP, blasts and pickups, the punch and
// parry, and the guns: hitscan, firing, reloads and their sounds, grenades.
// Included at the end of GameplayState.h.
// =============================================================================

inline int GameplayState::findParryTarget() const {
    glm::vec3 eye = player.camera.position, fwd = player.camera.forward();
    int best = -1; float bestD = 1e9f;
    for (int i = 0; i < ProjectileSystem::POOL_SIZE; ++i) {
        const Projectile& p = projSystem.pool[i];
        if (!p.alive || p.isPlayer) continue;
        glm::vec3 d = p.position - eye;
        float dist = glm::length(d);
        float reach = std::max(2.5f, 1.f + glm::length(p.velocity) * 0.22f) + p.size * 0.3f;
        if (dist < reach && dist > 1e-3f && glm::dot(fwd, d / dist) > 0.25f && dist < bestD) { bestD = dist; best = i; }
    }
    return best;
}

inline void GameplayState::parryFeedback(glm::vec3 at, bool heavy) {
    fx.spawnBurst(at, {1.f, 0.85f, 0.35f}, heavy ? 50 : 28, heavy ? 12.f : 8.f, 0.45f, 4.f);
    fx.spawnBurst(at, {1.f, 1.f, 1.f}, 10, 4.f, 0.2f, 0.f);
    explosionFlashTimer = 0.2f; explosionFlashPos = at;
    audio.play("clank");
    audio.play("parry", 80);
    ui.onParry();
    viewModel.triggerParry(true);
    invincFrames  = std::max(invincFrames, 0.5f);
    hitStopFrames = glm::max(hitStopFrames, heavy ? 9 : 4);
    shake(heavy ? 0.25f : 0.12f, heavy ? 0.06f : 0.03f);
}

inline void GameplayState::punch(int boostable) {
    punchCooldown = 0.3f;
    glm::vec3 eye = player.camera.position, fwd = player.camera.forward();

    int pi = findParryTarget();
    if (pi >= 0) {
        Projectile& p = projSystem.pool[pi];
        float speed = std::max(40.f, glm::length(p.velocity) * 2.f);
        p.isPlayer = true;
        p.parried  = true;
        p.velocity = fwd * speed;                        // it goes where you look
        p.lifetime = 4.f;
        if (p.heavy) {
            p.damage = 400.f; p.size *= 1.3f; p.emissiveColor = {1.6f, 1.1f, 0.3f};
            styleSystem.addStyle(60.f, StyleSource::PARRY);
            ui.toast("HEAVY PARRY", "", {1.f, 0.8f, 0.2f}, 1.2f);
        } else {
            p.damage = 60.f; p.emissiveColor = {1.f, 0.9f, 0.3f};
            styleSystem.addStyle(25.f, StyleSource::PARRY);
            ui.feed("PARRY", {1.f, 0.9f, 0.3f});
        }
        parryFeedback(p.position, p.heavy);
        return;
    }
    for (auto& e : enemies) {
        if (!e.targetable() || !e.parryWindow()) continue;
        glm::vec3 d = e.position + glm::vec3{0, e.height() * 0.5f, 0} - eye;
        float dist = glm::length(d);
        if (dist < 5.5f && glm::dot(fwd, d / dist) > 0.2f) {
            e.stagger(e.staggerTime());
            parryFeedback(eye + fwd * 1.2f, true);
            styleSystem.addStyle(70.f, StyleSource::PARRY);
            gainXp(30);
            if (e.type == EnemyType::SOVEREIGN) ui.toast("PARRIED", "HIS GUARD IS BROKEN - UNLOAD", {1.f, 0.75f, 0.2f}, 1.4f);
            else if (e.type == EnemyType::SHIELDBEARER) ui.toast("SHIELD DOWN", "", {0.4f, 1.f, 0.75f}, 1.2f);
            else ui.toast("BROKEN", "IT TAKES DOUBLE DAMAGE - UNLOAD", {1.f, 0.75f, 0.2f}, 1.8f);
            return;
        }
    }
    if (boostable >= 0) {
        // Projectile boost: detonate your own projectile for a massive explosion.
        auto& p = projSystem.pool[boostable];
        float r = glm::max(p.blastRadius, 4.f) * 2.f;
        pendingBlasts.push_back({p.position, r, p.damage * 3.f, 0.f, 0.f});
        p.alive = false;
        styleSystem.addStyle(40.f, StyleSource::EXPLOSIVE);
        invincFrames  = 0.6f;
        hitStopFrames = glm::max(hitStopFrames, 2);
        audio.play("parry");
        viewModel.triggerParry(true);
        ui.feed("PROJECTILE BOOST", {1.f, 0.6f, 0.2f});
        return;
    }
    viewModel.triggerParry(false);
    audio.play("punch", 90);
    for (auto& e : enemies) {
        if (!e.targetable()) continue;
        glm::vec3 c = e.position + glm::vec3{0, std::min(e.height() * 0.5f, 1.2f), 0};
        glm::vec3 d = c - eye;
        float dist = glm::length(d) - e.radius();
        if (dist < 2.4f && glm::dot(fwd, glm::normalize(d)) > 0.5f) {
            hurtEnemy(e, 25.f, c, 6.f, 1.f, StyleSource::PUNCH);
            glm::vec3 push = glm::normalize(glm::vec3{d.x, 0.f, d.z}) * (e.type == EnemyType::JUGGERNAUT ? 0.3f : 1.4f);
            e.position += push;
            audio.play("clank", 50);
            shake(0.08f, 0.02f);
            break;
        }
    }
}

inline void GameplayState::spawnEnemy(EnemyType t, glm::vec3 pos) {
    enemies.push_back(Enemy(t, pos));
    enemies.back().maxHealth *= tune().health * endlessToughness();
    enemies.back().health = enemies.back().maxHealth;
    glm::vec3 c = statsOf(t).glow;
    fx.spawnBurst(pos + glm::vec3{0, 0.3f, 0}, c, 14, 3.f, 0.7f, -6.f);
    if (level.waterDepthAt(pos) > 0.3f) {   // materialising in the water: a splash and a ring
        fx.spawnBurst(pos + glm::vec3{0, 0.2f, 0}, {0.6f, 0.85f, 0.9f}, 14, 5.f, 0.45f, 9.f);
        fx.spawnShockwave(pos, 2.5f, {0.4f, 0.8f, 0.85f});
    }
    if (spawnSoundThisTick) return;   // a FAST section spawns a dozen at once: one sound
    spawnSoundThisTick = true;
    float d = glm::length(pos - player.position);
    audio.play("spawn", (int)glm::clamp(110.f - d * 2.f, 25.f, 110.f));
}

inline void GameplayState::updateEnemies(float dt) {
    telegraphSoundCd -= dt;
    shieldClankCd -= dt;
    const Arena& ar = level.arenas[director.arena];
    EnemyWorld w;
    w.playerEye  = player.camera.position;
    w.playerFeet = player.position;
    w.walls      = level.walls.data();
    w.wallCount  = (int)level.walls.size();
    w.grid       = &spatialGrid;
    w.bounds     = ar.bounds;
    w.playerVel  = player.velocity;
    w.tune       = &tune();
    const float dmgScale = ar.damageScale * tune().damage;

    // Iterate by index: summons push_back into `enemies` mid-loop
    size_t n = enemies.size();
    for (size_t i = 0; i < n; ++i) {
        Enemy& e = enemies[i];
        if (!e.alive) continue;
        if (g_devOverlay.rfind("pose", 0) == 0 && e.type == EnemyType::SOVEREIGN) { devPose(e); continue; }
        e.floorY = level.floorWithWater(e.position.x, e.position.z, false);
        e.wadeMul = e.stats().flying ? 1.f : 1.f - 0.5f * (1.f - Player::wadeFactor(level.waterDepthAt(e.position)));
        e.update(dt, w);
        const EnemyEvents ev = e.ev;   // copy: spawning below may reallocate
        glm::vec3 epos = e.position;
        float dist = glm::length(epos - player.position);

        // Wind-up tick: a cue for the ones close enough to matter, at most
        // a few a second however many are aiming at you
        if (ev.telegraphStarted && dist < 30.f && telegraphSoundCd <= 0.f) {
            audio.play("telegraph", (int)glm::clamp(70.f - dist * 2.f, 12.f, 70.f));
            telegraphSoundCd = 0.22f;
        }
        for (int k = 0; k < ev.shots; ++k)
            projSystem.fire(ev.shotOrigin, ev.shotDir[k] * ev.shotSpeed, ev.shotDamage * dmgScale, false,
                            enemies[i].stats().shotColor, false, 0.f, ev.shotSize, ev.shotHeavy);
        if (ev.meleeHit) {
            if (damagePlayer(ev.meleeDamage * dmgScale, epos, 0.25f, 0.06f)) {
                glm::vec3 away = player.position - epos; away.y = 0.f;
                if (glm::length(away) > 0.001f)
                    player.velocity += glm::normalize(away) * 8.f + glm::vec3{0, 3.f, 0};
            }
        }
        if (ev.slam) {
            if (level.waterDepthAt(epos) > 0.3f) fx.spawnBurst(epos + glm::vec3{0, 0.2f, 0}, {0.6f, 0.85f, 0.9f}, 24, 7.f, 0.6f, 9.f);
            friendlySlam(enemies[i], ev.slamRadius);
            fx.spawnShockwave(epos, ev.slamRadius, statsOf(enemies[i].type).glow);
            shake(0.35f, 0.07f);
            audio.play("slam");
            glm::vec2 flat{player.position.x - epos.x, player.position.z - epos.z};
            bool grounded = player.position.y < epos.y + 0.9f && player.position.y > epos.y - 1.5f;   // jump it to dodge
            if (ev.slamDamage > 0.f && glm::length(flat) < ev.slamRadius && grounded) {
                if (damagePlayer(ev.slamDamage * dmgScale, epos, 0.3f, 0.08f) && glm::length(flat) > 0.01f)
                    player.velocity += glm::vec3{flat.x, 0.f, flat.y} / glm::length(flat) * 10.f + glm::vec3{0, 6.f, 0};
            }
        }
        if (ev.detonated) {
            // A mite that reached you: hurts you AND its friends
            pendingBlasts.push_back({epos + glm::vec3{0, 0.3f, 0}, 4.f, 30.f, 4.f, 30.f * dmgScale, StyleSource::FRIENDLY});
            spawnDebrisFor(enemies[i]);
        }
        if (ev.summonMites + ev.summonRippers > 0) {
            int total = ev.summonMites + ev.summonRippers;
            for (int k = 0; k < total; ++k) {
                float a = k * 6.2832f / total + gameClock;
                glm::vec3 p = epos + glm::vec3{std::cos(a) * 3.5f, 0.f, std::sin(a) * 3.5f};
                p.x = glm::clamp(p.x, ar.bounds.min.x + 1.f, ar.bounds.max.x - 1.f);
                p.z = glm::clamp(p.z, ar.bounds.min.z + 1.f, ar.bounds.max.z - 1.f);
                p.y = epos.y;
                spawnEnemy(k < ev.summonMites ? EnemyType::MITE : EnemyType::RIPPER, p);
            }
        }
        if (ev.slash >= 0) {   // a sword stroke: its arc, and a whoosh
            fx.slashes.push_back({epos, enemies[i].yaw, ev.slash, 0.f});
            audio.play("dash", ev.slash == 2 ? 120 : 95);
        }
        if (ev.dashStarted) { audio.play("dash", 128); shake(0.12f, 0.03f); }
        if (ev.leapStarted) { audio.play("jump", 128); fx.spawnShockwave(epos, 3.f, statsOf(enemies[i].type).glow); }
        if (enemies[i].type == EnemyType::SOVEREIGN) onSovereignEvents(enemies[i], ev);
        if (ev.enraged) {
            pushBanner(enemies[i].type == EnemyType::SOVEREIGN ? "THE SOVEREIGN IS ENRAGED" : "THE WARDEN IS ENRAGED",
                       "", {1.f, 0.15f, 0.25f}, 2.f);
            shake(0.5f, 0.06f);
            audio.play("wave");
        }
    }

    for (auto& e : enemies) {
        if (!e.targetable()) continue;
        // Fell into the void: counts as your kill
        if (e.position.y < ar.voidY) {
            e.alive = false; e.state = EnemyState::DEAD; e.health = 0.f;
            ui.feed(std::string(e.stats().name) + " FELL", {1.f, 0.7f, 0.3f});
            styleSystem.addStyle(15.f, StyleSource::ENVIRONMENT);
            onEnemyKilled(e, StyleSource::ENVIRONMENT);
            continue;
        }
        // Lava burns enemies too: lure them in
        if (e.stats().flying) continue;
        for (auto& hz : level.hazards)
            if (e.position.y < hz.box.max.y + 0.3f && e.position.y > hz.box.min.y - 0.5f &&
                e.position.x > hz.box.min.x && e.position.x < hz.box.max.x &&
                e.position.z > hz.box.min.z && e.position.z < hz.box.max.z) {
                if (e.takeDamage(hz.dps * 0.5f * dt)) onEnemyKilled(e, StyleSource::ENVIRONMENT);
            }
    }

    // Soft separation so squads don't stack inside each other
    for (size_t i = 0; i < enemies.size(); ++i) {
        for (size_t j = i + 1; j < enemies.size(); ++j) {
            Enemy& a = enemies[i]; Enemy& b = enemies[j];
            if (!a.alive || !b.alive || a.stats().flying != b.stats().flying) continue;
            glm::vec3 d = b.position - a.position;
            if (!a.stats().flying) { if (std::fabs(d.y) > 1.5f) continue; d.y = 0.f; }
            float len = glm::length(d), minD = a.radius() + b.radius();
            if (len >= minD || len < 1e-4f) continue;
            glm::vec3 n = d / len * (minD - len);
            float wa = b.radius() / minD, wb = a.radius() / minD;   // big ones get pushed less
            if (a.type == EnemyType::CONDUIT) { wa = 0.f; wb = 1.f; }   // conduits are rooted
            if (b.type == EnemyType::CONDUIT) { wa = 1.f; wb = 0.f; }
            a.position -= n * wa; b.position += n * wb;
        }
    }
}

inline void GameplayState::friendlySlam(const Enemy& slammer, float radius) {
    float dmg = slammer.type == EnemyType::BRUTE ? FRIENDLY_SLAM_BRUTE
              : slammer.type == EnemyType::JUGGERNAUT ? FRIENDLY_SLAM_JUGGERNAUT : 0.f;
    if (dmg <= 0.f) return;   // the bosses' slams spare their summons
    for (auto& o : enemies) {
        if (&o == &slammer || !o.targetable() || isBoss(o.type) || o.stats().flying) continue;
        glm::vec3 d = o.position - slammer.position;
        if (glm::length(glm::vec2{d.x, d.z}) < radius && std::fabs(d.y) < 1.5f)
            hurtEnemy(o, dmg, o.position + glm::vec3{0, o.height() * 0.5f, 0}, 0.f, 0.f, StyleSource::FRIENDLY);
    }
}

inline bool GameplayState::damagePlayer(float dmg, glm::vec3 from, float shakeT, float shakeAmt) {
    if (invincFrames > 0.f || playerDead || g_godMode || victory ||
        (!fast() && director.phase == WaveDirector::Phase::VICTORY)) return false;
    if (modOn(DailyMod::GLASS_CANNON)) dmg *= 2.f;
    styleSystem.takeDamage(dmg);
    ui.onDamage();
    showDamageFrom(from);
    shake(shakeT, shakeAmt);
    audio.play("player_hit");
    invincFrames = 0.35f;
    grapple.release();
    return true;
}

inline void GameplayState::showDamageFrom(glm::vec3 source) {
    glm::vec3 toSrc = source - player.camera.position;
    float angle = atan2f(toSrc.x, toSrc.z) - glm::radians(player.camera.yaw + 90.f);
    ui.onDamageFrom(angle);
}

inline bool GameplayState::hurtEnemy(Enemy& e, float dmg, glm::vec3 at, float style, float heal, StyleSource src,
                                     bool crit, bool pierceArmor) {
    if (!e.targetable()) return false;
    if (!pierceArmor) dmg *= e.armorMult();
    if (modOn(DailyMod::GLASS_CANNON) && src != StyleSource::FRIENDLY && src != StyleSource::ENVIRONMENT) dmg *= 2.f;
    if (e.shielded) {   // a CONDUCTOR's tether soaks most of it
        dmg *= CONDUCTOR_SHIELD;
        fx.spawnHitSparks(at, {0.3f, 1.f, 0.9f});
    }
    float before = e.health;
    bool killed = e.takeDamage(dmg);
    // Enemies hurting each other: no hitmarker, no style or healing for the
    // hit (a kill still scores, in onEnemyKilled)
    bool friendly = src == StyleSource::FRIENDLY;
    if (!friendly) {
        styleSystem.addStyle(style, src);
        styleSystem.heal(heal * tune().heal);
    }
    if (!friendly) audio.play("hit");
    if (!e.stats().flying) fx.spawnDecal(e.position);
    fx.spawnHitSparks(at, e.stats().color * 1.4f);
    if (!friendly) ui.onHit(killed, crit);
    if (!settings || settings->damageNumbers) ui.spawnDamageNumber(at, std::min(dmg, before), crit);
    if (killed) {
        onEnemyKilled(e, src);
        hitStopFrames = glm::max(hitStopFrames, isBoss(e.type) ? 12 : 2);
    }
    return killed;
}

inline void GameplayState::onEnemyKilled(Enemy& e, StyleSource src) {
    ++totalKills;
    if (e.type == EnemyType::CONDUIT) director.onConduitDestroyed(e.position);
    if (e.type == EnemyType::CONDUCTOR && e.linkCount > 0) ui.feed("TETHERS BROKEN", {0.3f, 1.f, 0.9f});
    // Half for one you only set up (enemies hurting each other, lava, the void)
    styleSystem.addStyle(src == StyleSource::FRIENDLY || src == StyleSource::ENVIRONMENT ? 15.f : 30.f, src);
    styleSystem.heal(5.f * tune().heal);
    audio.play("enemy_death");
    fx.spawnDeathParticles(e.position + glm::vec3{0, e.height() * 0.5f, 0}, e.stats().color);
    spawnDebrisFor(e);
    if (styleSystem.overdrive) dashCharges = 2;
    if (++killsThisCycle >= 2) {
        killsThisCycle = 0;
        if (grenadeCount < grenadeMax) { grenadeCount++; ui.onGrenadeRefill(); }
    }

    // XP, scaled by how stylishly you're playing
    int xp = (int)std::round(xpForKill(e.type) * styleXpMultiplier(styleSystem.getRank()));
    gainXp(xp);
    char buf[64];
    snprintf(buf, sizeof(buf), "%s  +%d XP", e.stats().name, xp);
    ui.feed(buf, {0.8f, 0.85f, 0.9f});
    if (src == StyleSource::FRIENDLY) {
        gainXp(10);
        ui.feed("FRIENDLY FIRE  +10 XP", {1.f, 0.5f, 0.35f});
    }

    // Drops. Health orbs are common and small; potions (18%) and XP shards
    // (10%) are the rare ones, and get a loot beam so you notice them.
    glm::vec3 c = e.position + glm::vec3{0, std::min(e.height() * 0.5f, 1.5f), 0};
    float floorY = groundHeightAt(e.position.x, e.position.z, e.position.y + 0.5f);
    auto drop = [&](PickupKind k) {
        pickups.push_back({c, glm::vec3{frand(-3.f, 3.f), frand(3.f, 6.f), frand(-3.f, 3.f)},
                           k == PickupKind::ORB ? 20.f : 30.f, k, floorY, c});
    };
    switch (e.type) {
        case EnemyType::BRUTE:  for (int i = 0; i < 3; ++i) drop(PickupKind::ORB);
                                if (rand() % 100 < 50) drop(PickupKind::POTION); break;
        case EnemyType::MITE:   if (rand() % 10 == 0) drop(PickupKind::ORB); break;
        case EnemyType::WARDEN: case EnemyType::SOVEREIGN: break;
        case EnemyType::CONDUIT: drop(PickupKind::ORB); drop(PickupKind::ORB); break;
        case EnemyType::CONDUCTOR: drop(PickupKind::ORB); break;
        default:
            if (rand() % 100 < (int)(20 * tune().drops)) drop(PickupKind::ORB);
            if (rand() % 100 < (int)(18 * tune().drops)) drop(PickupKind::POTION);
            if (rand() % 100 < 10) drop(PickupKind::XP);
            break;
    }

    if (e.type == EnemyType::MITE)          // shot mites still pop — but only hurt enemies
        pendingBlasts.push_back({e.position + glm::vec3{0, 0.3f, 0}, 4.f, 30.f, 0.f, 0.f,
                                 src == StyleSource::FRIENDLY || src == StyleSource::ENVIRONMENT ? StyleSource::FRIENDLY
                                                                                                 : StyleSource::EXPLOSIVE});

    if (isBoss(e.type)) {
        // The boss takes his summons with him
        for (auto& o : enemies)
            if (o.alive && &o != &e) {
                o.alive = false; o.state = EnemyState::DEAD;
                spawnDebrisFor(o);
                fx.spawnDeathParticles(o.position + glm::vec3{0, o.height() * 0.5f, 0}, o.stats().color);
            }
        for (int k = 0; k < 4; ++k)
            fx.spawnExplosionParticles(e.position + glm::vec3{frand(-1.5f,1.5f), frand(1.f,4.f), frand(-1.5f,1.5f)}, 4.f);
        explosionFlashTimer = 0.35f; explosionFlashPos = e.position + glm::vec3{0, 2.f, 0};
        shake(1.0f, 0.12f);
        audio.play("explosion");
        if (e.type == EnemyType::SOVEREIGN) {
            for (int k = 0; k < 6; ++k)
                fx.spawnBurst(e.position + glm::vec3{frand(-1.f, 1.f), frand(0.5f, 3.5f), frand(-1.f, 1.f)},
                           {1.f, 0.8f, 0.4f}, 30, 10.f, 0.8f, 2.f);
            pushBanner("THE SOVEREIGN HAS FALLEN", "", {1.f, 0.85f, 0.3f}, 3.f);
            if (!g_godMode && !records.act2Unlocked) {
                records.act2Unlocked = true;
                records.save();
                ui.feed("ACT II UNLOCKED - BENEATH THE ECLIPSE", {0.35f, 0.95f, 0.9f});
            }
        } else {
            pushBanner("WARDEN DESTROYED", "", {1.f, 0.85f, 0.3f}, 2.5f);
        }
    }
}

inline void GameplayState::gainXp(int xp) {
    int lv = prog.addXp(xp);
    if (lv > 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "LEVEL %d", prog.level);
        ui.toast(buf, "UPGRADE READY - PRESS TAB", {0.4f, 0.9f, 1.f}, 2.6f);
        audio.play("levelup");
        styleSystem.heal(15.f);
    }
}

inline void GameplayState::processBlasts() {
    for (int guard = 0; !pendingBlasts.empty() && guard < 64; ++guard) {
        Blast b = pendingBlasts.back();
        pendingBlasts.pop_back();
        fx.spawnExplosionParticles(b.pos, b.radius);
        shake(0.3f, 0.06f);
        explosionFlashTimer = 0.35f; explosionFlashPos = b.pos;
        audio.play("explosion");
        for (auto& e : enemies) {
            if (!e.targetable()) continue;
            float d = glm::length(e.position + glm::vec3{0, e.height() * 0.5f, 0} - b.pos);
            if (d < b.radius)
                hurtEnemy(e, b.damage * (1.f - d / b.radius), e.position + glm::vec3{0, e.height() * 0.6f, 0}, 15.f, 3.f, b.src);
        }
        if (b.playerDamage > 0.f) {
            float pd = glm::length(player.camera.position - b.pos);
            if (pd < b.playerRadius)
                damagePlayer(b.playerDamage * (1.f - pd / b.playerRadius) + 2.f, b.pos, 0.3f, 0.07f);
        }
    }
    pendingBlasts.clear();
}

inline void GameplayState::updatePickups(float dt) {
    glm::vec3 chest = player.position + glm::vec3{0, 0.9f, 0};
    bool hurt = styleSystem.health < styleSystem.maxHealth - 0.5f;
    for (auto& p : pickups) {
        glm::vec3 to = chest - p.pos;
        float d = glm::length(to);
        float magnet = p.kind == PickupKind::ORB ? 5.f : p.kind == PickupKind::XP ? 4.f : (hurt ? 2.5f : 0.f);
        if (d < magnet && d > 1e-3f) {
            p.vel = glm::mix(p.vel, to / d * 16.f, std::min(1.f, dt * 8.f));
        } else {
            p.vel.y -= 20.f * dt;
            p.vel.x *= std::pow(0.2f, dt); p.vel.z *= std::pow(0.2f, dt);
        }
        p.pos += p.vel * dt;
        float rest = p.floorY + (p.kind == PickupKind::ORB ? 0.4f : 0.55f);
        if (p.pos.y < rest) { p.pos.y = rest; p.vel.y = std::fabs(p.vel.y) * 0.3f; }
        p.life -= dt;
        if (d < 1.2f) {
            switch (p.kind) {
            case PickupKind::ORB:
                p.life = 0.f;
                styleSystem.heal(12.f);
                audio.play("pickup");
                fx.spawnBurst(p.pos, {0.3f, 1.f, 0.5f}, 8, 2.f, 0.4f, 0.f);
                break;
            case PickupKind::POTION:
                if (!hurt) break;   // saved for when you need it
                p.life = 0.f;
                styleSystem.heal(40.f);
                audio.play("potion");
                ui.feed("+40 HP  HEALTH POTION", {1.f, 0.35f, 0.4f});
                fx.spawnBurst(p.pos, {1.f, 0.25f, 0.35f}, 20, 3.f, 0.6f, -2.f);
                break;
            case PickupKind::XP: {
                p.life = 0.f;
                int xp = 40 + 5 * prog.level;
                gainXp(xp);
                char buf[32]; snprintf(buf, sizeof(buf), "+%d XP  SHARD", xp);
                ui.feed(buf, {0.6f, 0.5f, 1.f});
                audio.play("pickup");
                fx.spawnBurst(p.pos, {0.6f, 0.4f, 1.f}, 16, 3.f, 0.5f, -2.f);
                break;
            }
            }
        }
    }
    pickups.erase(std::remove_if(pickups.begin(), pickups.end(),
                  [](const Pickup& p){ return p.life <= 0.f; }), pickups.end());
}

inline float GameplayState::hitscanAll(glm::vec3 origin, glm::vec3 dir, float range, std::vector<RayHit>& out) {
    out.clear();
    float wallT = range;
    for (auto& w : level.walls) {
        float t = rayBoxHit(origin, dir, w.box);
        if (t > 0.f && t < wallT) wallT = t;
    }
    for (int ei = 0; ei < (int)enemies.size(); ++ei) {
        if (!enemies[ei].targetable()) continue;
        // A ray through the head box is a headshot, and the head counts
        // even where it pokes out of the body box
        // Test against where it was drawn, not where it has got to since
        const Enemy& en = enemies[ei];
        glm::vec3 off = en.hasShown ? en.shownPos - en.position : glm::vec3{0.f};
        if (glm::dot(off, off) > 9.f) off = glm::vec3{0.f};   // teleported: trust the simulation
        AABB body = en.getAABB();
        body.min += off; body.max += off;
        float t = rayBoxHit(origin, dir, body);
        AABB head;
        bool hasHead = headBox(en, head);
        head.min += off; head.max += off;
        float th = hasHead ? rayBoxHit(origin, dir, head) : -1.f;
        if (th > 0.f && th < wallT) out.push_back({ei, th, true});
        else if (t > 0.f && t < wallT) out.push_back({ei, t, false});
    }
    std::sort(out.begin(), out.end(), [](const RayHit& a, const RayHit& b) { return a.t < b.t; });
    return wallT;
}

inline void GameplayState::reloadSounds() {
    const WeaponState& ws = weapons[activeWeapon];
    if (!ws.reloading || activeWeapon > 2 || pendingWeapon >= 0) return;
    float now = ws.reloadProgress(), before = reloadCueAt;
    reloadCueAt = now;
    auto cue = [&](float at, const char* snd, int vol) { if (before < at && now >= at) audio.play(snd, vol); };
    if (activeWeapon == 0) {
        cue(0.04f, "cyl_open", 110); cue(0.2f, "eject", 100); cue(0.5f, "shell_in", 120); cue(0.78f, "cyl_close", 120);
    } else if (activeWeapon == 2) {   // Kar98: bolt open, the empty, clip in, rounds down, clip out, bolt home
        cue(0.11f, "cyl_open", 100); cue(0.2f, "eject", 90);
        cue(ViewModel::KAR_CLIP1 - 0.02f, "shell_in", 120);
        cue(ViewModel::KAR_PRESS0 + 0.08f, "reload", 90);
        cue(ViewModel::KAR_FLICK0 + 0.02f, "eject", 70);
        cue(0.8f, "bolt", 120);
    } else {
        int n = viewModel.reloadShells;
        for (int i = 0; i < n; ++i) cue(0.13f + (i + 0.55f) * 0.6f / n, "shell_in", 110);
        cue(0.86f, "pump", 120);
    }
}

inline void GameplayState::startReload(int w) {
    WeaponId id = (WeaponId)w;
    weapons[w].startReload(weaponReload(id, prog.up[w]));
    // The revolver, shotgun and Kar98 play their reload over its whole length; the Longshot swings
    int mag = weaponMag(id, prog.up[w]);
    if (w == activeWeapon) viewModel.triggerReload(w <= 2 ? weapons[w].reloadTotal : weapons[w].reloadTotal * 0.6f,
                                                   mag - weapons[w].ammo);
    reloadCueAt = 0.f;
    if (w >= 3) audio.play("reload");
}

inline void GameplayState::fireWeapon(int w) {
    WeaponId id = (WeaponId)w;
    const WeaponDef& d = weaponDef(id);
    const WeaponUpgrades& u = prog.up[w];
    WeaponState& ws = weapons[w];
    --ws.ammo;
    ws.cooldown = weaponCooldown(id, u);

    glm::vec3 origin = player.camera.position;
    glm::vec3 fwd    = player.camera.forward();
    glm::vec3 right  = player.camera.right();
    glm::vec3 up     = glm::cross(right, fwd);

    float aimNow = d.canAim ? aim : 0.f;
    bool  quick  = id == WeaponId::LONGSHOT && aimNow >= 0.98f && aimFullAt >= 0.f && gameClock - aimFullAt < 0.4f;
    bool  noscope = d.canAim && aimNow < 0.15f;
    float spread = weaponSpread(id, u, aimNow);
    // Rifles are wild in the air unless aimed
    if (d.canAim && !player.onGround) spread += 0.03f * (1.f - aimNow);
    int   pellets = weaponPellets(id, u);
    int   pierce  = weaponPierce(id, u);
    float dmg     = weaponDamage(id, u);
    bool  sniper  = id == WeaponId::KAR || id == WeaponId::LONGSHOT;

    bool anyHit = false, headKill = false, kills = 0;
    int  killCount = 0;
    static std::vector<RayHit> hits;
    for (int p = 0; p < pellets; ++p) {
        // Uniform within a disc of radius `spread`
        float ang = frand(0.f, 6.2832f), rad = std::sqrt(frand(0.f, 1.f)) * spread;
        glm::vec3 dir = glm::normalize(fwd + right * (std::cos(ang) * rad) + up * (std::sin(ang) * rad));
        float wallT = hitscanAll(origin, dir, d.range, hits);
        int n = std::min((int)hits.size(), pierce + 1);
        float endT = n > 0 && n == pierce + 1 ? hits[n - 1].t : wallT;
        fx.spawnTracer(origin + dir * 0.25f - up * 0.08f, origin + dir * endT, sniper ? 0.09f : 0.055f, sniper ? 0.35f : 0.22f);
        for (int k = 0; k < n; ++k) {
            Enemy& e = enemies[hits[k].enemy];
            glm::vec3 at = origin + dir * hits[k].t;
            bool head = hits[k].head && d.headMult > 1.f;   // the shotgun has no headshot bonus
            // The Sovereign turns aside shots from range unless he's mid-attack,
            // and answers with a crescent: his openings are up close
            if (e.deflects(dir, hits[k].t)) {
                fx.spawnHitSparks(at, {1.f, 0.85f, 0.4f});
                fx.spawnBurst(at, {1.f, 0.8f, 0.35f}, 10, 6.f, 0.3f, 10.f);
                audio.play("clank", 100);
                e.onDeflect();
                if (deflectHints < 2) { ++deflectHints; ui.toast("DEFLECTED", "GET CLOSE - HIT HIM AS HE STRIKES", {1.f, 0.8f, 0.3f}, 1.8f); }
                anyHit = true;
                break;
            }
            // A Shieldbearer's shield stops body shots from the front (not
            // the head over its rim), and the round with it
            if (!hits[k].head && e.blocks(dir)) {
                fx.spawnHitSparks(at, {0.4f, 1.f, 0.75f});
                if (shieldClankCd <= 0.f) { audio.play("clank", 70); shieldClankCd = 0.12f; }
                anyHit = true;
                break;
            }
            float m = head ? d.headMult : 1.f;
            float falloff = 1.f - 0.15f * k;    // each body it punches through costs a little
            if (head) fx.spawnHitSparks(at, {1.f, 0.9f, 0.3f});
            anyHit = true;
            if (hurtEnemy(e, dmg * m * falloff, at, sniper ? 12.f : (pellets > 1 ? 3.f : 10.f), pellets > 1 ? 0.5f : 2.f,
                          weaponSource(id), head)) {
                ++killCount;
                if (head) headKill = true;
            }
        }
        // Explosive tips: burst where the round stops
        if (id == WeaponId::LONGSHOT && u.mod)
            pendingBlasts.push_back({origin + dir * (n > 0 ? hits[0].t : wallT) - dir * 0.3f, 3.5f, 70.f, 0.f, 0.f, StyleSource::LONGSHOT});
    }
    kills = killCount > 0;

    // Trick-shot bonuses
    if (kills && id == WeaponId::LONGSHOT && quick) {
        styleSystem.addStyle(40.f, StyleSource::LONGSHOT); gainXp(25);
        ui.toast("QUICKSCOPE", "+25 XP", {1.f, 0.85f, 0.2f}, 1.4f);
        audio.play("parry", 90);
    } else if (kills && sniper && noscope) {
        styleSystem.addStyle(60.f, weaponSource(id)); gainXp(40);
        ui.toast("NOSCOPE", "+40 XP", {1.f, 0.4f, 0.8f}, 1.6f);
        audio.play("parry", 90);
    }
    if (killCount >= 2) {
        char buf[32]; snprintf(buf, sizeof(buf), "COLLATERAL x%d", killCount);
        styleSystem.addStyle(25.f * (killCount - 1), weaponSource(id)); gainXp(15 * (killCount - 1));
        ui.toast(buf, "", {1.f, 0.55f, 0.2f}, 1.4f);
    }
    if (headKill && sniper) {
        ui.feed("HEADSHOT", {1.f, 0.85f, 0.25f});
        styleSystem.addStyle(10.f, weaponSource(id));
        // HEADHUNTER: the round comes back and the bolt is skipped
        if (id == WeaponId::KAR && u.mod) { ++ws.ammo; ws.cooldown = 0.1f; }
    }
    if (killCount > 0 && sniper) hitStopFrames = glm::max(hitStopFrames, 3);

    float recoil = d.recoil * (1.f - 0.4f * aimNow);
    recoilPitch = std::min(recoilPitch + recoil, 14.f);
    player.camera.pitch = glm::clamp(player.camera.pitch + recoil, -89.f, 89.f);

    viewModel.triggerFire();
    ui.onShoot();
    muzzleFlashPos   = origin + fwd * 0.6f;
    muzzleFlashTimer = sniper ? 0.08f : pellets > 1 ? 0.07f : 0.04f;
    shake(sniper ? 0.14f : pellets > 1 ? 0.12f : 0.06f, sniper ? 0.03f : pellets > 1 ? 0.025f : 0.012f);
    static const char* SND[] = {"revolver", "shotgun", "kar", "longshot"};
    audio.play(SND[w]);
    ++totalShots;
    if (anyHit) ++totalHits;
    fx.spawnShellCasing(origin, right);
    if (pellets > 1) fx.spawnShellCasing(origin + right * 0.1f, right);

    if (ws.ammo <= 0) {
        if (!sniper) startReload(w);   // rifles reload after the bolt cycle (physicsTick)
    } else if (sniper && ws.cooldown > 0.2f) {
        viewModel.triggerBolt(ws.cooldown * 0.85f);
        boltSoundTimer = ws.cooldown * 0.2f;
    } else if (pellets > 1) {
        viewModel.triggerPump();
        audio.play("pump", 70);
    }
}

inline void GameplayState::throwGrenade() {
    --grenadeCount;
    grenadeTimer = 0.6f;
    glm::vec3 origin = player.camera.position;
    glm::vec3 dir    = player.camera.forward();
    projSystem.fire(origin, dir * 14.f + glm::vec3{0, 5.f, 0}, 80.f, true,
                    {0.3f, 0.9f, 0.1f}, /*grenade=*/true, /*blastRadius=*/5.f);
    viewModel.triggerGrenade();
    shake(0.05f, 0.008f);
    audio.play("jump");
}
