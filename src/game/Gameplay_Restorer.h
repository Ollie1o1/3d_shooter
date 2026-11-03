#pragma once
// =============================================================================
// Gameplay_Restorer.h — GameplayState: THE RESTORER's fight in the Vault (its
// brain is enemies/EnemyRestorer.h; docs/chapter-5-design.md section 8).
//
// What its attacks do to the world and to you:
//   ROLLBACK  where you were 4 s before it began: a ghost of you there, four
//             ticks, then you are put back (unless you RECALL in the last
//             moment). Its next attack goes for that spot.
//   SNAPSHOT  a flash where you stood; a statue of you is left there (real
//             cover). Five of them, or its last phase, and it shatters them all.
//   TONG      caught: hung from the hook rail over the Vault until you punch
//             free or it lets you drop.
//   FREEZE    a pattern of grid cells frosts red, then spikes burst from them.
//   SHELVE    racks slam down into the floor where it marked them in gold:
//             aisles to hide in (or a rack to be under).
// Its phases (from its health): 2 plants RESTORE POINTS (while one stands, what
// you do to it grows back: the pale part of its bar); 3 is FACTORY RESET (your
// guns, one at a time, into the STORAGE DRUMS round the rim; break a drum to
// take one back). Nothing it takes outlives a retry.
// Included at the end of GameplayState.h.
// =============================================================================

namespace rsfight {
constexpr float TRAIL_HZ = 30.f, ROLLBACK_SECONDS = 4.f;
constexpr int TRAIL = 180;                    // 6 s of where you were
constexpr float SNAP_R = 2.6f;                // the snapshot's flash: stand here and it takes you
constexpr int STATUE_SHATTER_AT = 5;
constexpr float CELL = 5.f;                   // the Vault's grid
constexpr float REGROW = 140.f;               // health a second it gets back while a restore point stands
constexpr float TAKE_EVERY = 5.f;             // FACTORY RESET: a gun every this many seconds
}

inline Enemy* GameplayState::restorer() {
    for (auto& e : enemies) if (e.alive && e.type == EnemyType::RESTORER) return &e;
    return nullptr;
}

// Everything the fight put in the world, gone (a retry, its death)
inline void GameplayState::resetRestorerFight() {
    const auto& S = level.story;
    for (int k = 0; k < S.coldPoolCount; ++k) {
        AABB& b = level.walls[S.coldPool + k].box;
        b = {cold::vaultC() + glm::vec3{-0.1f, -600.f, -0.1f}, cold::vaultC() + glm::vec3{0.1f, -599.f, 0.1f}};
    }
    rsStatues.clear(); rsRacks.clear(); rsPlannedRacks.clear(); rsCells.clear();
    rsRollbackT = rsSnapT = rsFreezeT = rsSpikeT = rsShatterT = rsTongT = -1.f;
    rsPhase = 0; rsTakeT = 0.f; rsTrailN = 0; rsTrailT = 0.f;
    storyOwned |= rsTaken; rsTaken = 0;   // (your guns come back)
    for (int k = 0; k < 4; ++k) rsDrumGun[k] = -1;
}

inline void GameplayState::rsPlaceWall(int k, const AABB& b) {
    if (k >= 0 && k < level.story.coldPoolCount) level.walls[level.story.coldPool + k].box = b;
}

// What happens when one of its attacks starts (its tell) and when it lands
inline void GameplayState::restorerEvents(Enemy& e, const EnemyEvents& ev) {
    const glm::vec3 vc = cold::vaultC();
    if (ev.telegraphStarted) {
        switch (e.attack) {
        case AttackKind::RS_ROLLBACK: {   // where you were, four seconds ago
            int back = std::min(rsTrailN - 1, (int)(rsfight::ROLLBACK_SECONDS * rsfight::TRAIL_HZ));
            rsRollbackAt = back >= 0 ? rsTrail[(rsTrailHead - back + rsfight::TRAIL) % rsfight::TRAIL] : player.position;
            rsRollbackT = e.telegraphDuration;
            ui.feed("ROLLBACK - KEEP WHERE YOU WERE SAFE, OR RECALL", abGoldTell(glm::vec3{1.f, 0.8f, 0.3f}));
            break;
        }
        case AttackKind::RS_SNAPSHOT:
            rsSnapAt = e.rsAim; rsSnapT = e.telegraphDuration;
            audio.playAt("count_tick", e.position, 110, SoundGroup::ENEMY, true, 0.6f);
            break;
        case AttackKind::RS_FREEZE: {   // a pattern on the grid, round where it is aimed
            rsCells.clear();
            const int pattern = (e.rsCount + rand()) % 3;
            const glm::vec3 c = e.rsAim;
            for (int i = -6; i <= 6; ++i) for (int j = -6; j <= 6; ++j) {
                glm::vec3 p = vc + glm::vec3{i * rsfight::CELL, 0.f, j * rsfight::CELL};
                if (glm::length(glm::vec2{p.x - vc.x, p.z - vc.z}) > cold::VAULT_R - 2.f) continue;
                bool on = false;
                if (pattern == 0) on = ((i + j) & 1) == 0;                                                     // a checkerboard
                else if (pattern == 1) { float d = glm::length(glm::vec2{p.x - c.x, p.z - c.z}); on = std::fmod(d, 15.f) < 7.5f; }   // rings round you
                else on = std::fabs(p.x - c.x) < rsfight::CELL * 0.6f || std::fabs(p.z - c.z) < rsfight::CELL * 0.6f;               // a cross through you
                if (on) rsCells.push_back(p);
            }
            rsFreezeT = e.telegraphDuration;
            break;
        }
        case AttackKind::RS_SHELVE: {   // three racks across the floor, square to the line between you
            rsPlannedRacks.clear();
            glm::vec3 d = player.position - e.position; d.y = 0.f;
            const bool alongX = std::fabs(d.z) > std::fabs(d.x);   // (racks run across the line it looks along)
            for (int k = -1; k <= 1; ++k) {
                float off = k * 9.f + (float)(rand() % 5 - 2);
                glm::vec3 m = vc + (alongX ? glm::vec3{(float)(rand() % 11 - 5), 0.f, off} : glm::vec3{off, 0.f, (float)(rand() % 11 - 5)});
                glm::vec3 h = alongX ? glm::vec3{6.f, 2.5f, 0.6f} : glm::vec3{0.6f, 2.5f, 6.f};
                rsPlannedRacks.push_back({m - glm::vec3{h.x, 0.f, h.z}, m + glm::vec3{h.x, 2.f * h.y, h.z}});
            }
            break;
        }
        default: break;
        }
    }
    if (ev.rsCast == AttackKind::NONE) return;
    switch (ev.rsCast) {
    case AttackKind::RS_ROLLBACK:
        rsRollbackT = -1.f;
        if (gameClock - recallPressedAt < 0.8f) {   // you put yourself somewhere first
            ui.feed("ROLLBACK OVERRIDDEN", RECALL_FROST);
            styleSystem.addStyle(40.f);
            break;
        }
        fx.spawnBurst(player.camera.position, {0.3f, 0.9f, 1.f}, 20, 4.f, 0.4f, -1.f);
        grapple.release();
        player.position = rsRollbackAt; player.velocity = glm::vec3{0.f};
        player.camera.position = player.position + glm::vec3{0.f, player.eyeHeight, 0.f}; prevCamPos = player.camera.position;
        audio.play("dash", 110, SoundGroup::PLAYER);
        ui.feed("ROLLED BACK", {0.3f, 0.9f, 1.f});
        e.rsAim = rsRollbackAt; e.rsAimSet = true;   // and it is ready for you there
        break;
    case AttackKind::RS_SNAPSHOT: {
        rsSnapT = -1.f;
        explosionFlashTimer = 0.15f; explosionFlashPos = e.position + glm::vec3{0.f, 7.f, 1.f};
        audio.playAt("ab_zap", rsSnapAt, 128, SoundGroup::ENEMY, true, 1.4f);
        glm::vec2 d{player.position.x - rsSnapAt.x, player.position.z - rsSnapAt.z};
        if (glm::length(d) < rsfight::SNAP_R && !playerDead) {
            damagePlayer(16.f * tune().damage, rsSnapAt, 0.2f, 0.04f);
            player.velocity *= 0.1f;
            ui.feed("SNAPSHOT TAKEN", {1.f, 0.4f, 0.3f});
        }
        // The statue (real cover), where you were; the oldest goes if they are all used
        if ((int)rsStatues.size() >= 8) rsStatues.erase(rsStatues.begin());
        rsStatues.push_back({rsSnapAt, std::atan2(e.position.x - rsSnapAt.x, e.position.z - rsSnapAt.z)});
        for (int k = 0; k < 8; ++k) {
            if (k < (int)rsStatues.size()) { glm::vec3 p = rsStatues[k].at; rsPlaceWall(k, {p + glm::vec3{-0.45f, 0.f, -0.45f}, p + glm::vec3{0.45f, 1.85f, 0.45f}}); }
            else rsPlaceWall(k, {vc + glm::vec3{-0.1f, -600.f, -0.1f}, vc + glm::vec3{0.1f, -599.f, 0.1f}});
        }
        fx.spawnBurst(rsSnapAt + glm::vec3{0.f, 1.f, 0.f}, RECALL_FROST, 20, 3.f, 0.6f, 4.f);
        if ((int)rsStatues.size() >= rsfight::STATUE_SHATTER_AT && rsShatterT < 0.f) rsShatterT = 1.4f;
        break;
    }
    case AttackKind::RS_TONG:
        if (ev.sortGrab && rsTongT < 0.f && !playerDead) {   // caught: up onto the hook rail
            rsTongT = 2.2f;
            float bestZ = vc.z - 14.f;
            for (int k = 0; k < 3; ++k) { float z = vc.z - 14.f + k * 14.f; if (std::fabs(z - player.position.z) < std::fabs(bestZ - player.position.z)) bestZ = z; }
            rsTongAt = {glm::clamp(player.position.x, vc.x - cold::VAULT_R + 3.f, vc.x + cold::VAULT_R - 3.f), 13.6f, bestZ};
            grapple.release();
            damagePlayer(ev.meleeDamage * tune().damage, e.position, 0.25f, 0.05f, 0.f);
            audio.playAt("hook_yank", rsTongAt, 120, SoundGroup::ENEMY, true, 0.7f);
            ui.feed("HUNG UP - PUNCH FREE", {1.f, 1.f, 1.f});
        }
        break;
    case AttackKind::RS_FREEZE:
        rsFreezeT = -1.f; rsSpikeT = 0.7f;
        audio.playAt("ab_shatter", vc, 128, SoundGroup::ENEMY, true, 0.8f);
        shake(0.3f, 0.05f);
        for (const auto& c : rsCells)
            if (std::fabs(player.position.x - c.x) < rsfight::CELL * 0.5f && std::fabs(player.position.z - c.z) < rsfight::CELL * 0.5f && player.position.y < 2.f) {
                damagePlayer(24.f * tune().damage, c, 0.3f, 0.06f);
                player.velocity.y = std::max(player.velocity.y, 7.f);
                break;
            }
        break;
    case AttackKind::RS_SHELVE: {
        rsRacks = rsPlannedRacks; rsPlannedRacks.clear();
        for (int k = 0; k < 3; ++k) rsPlaceWall(8 + k, k < (int)rsRacks.size() ? rsRacks[k] : AABB{vc + glm::vec3{-0.1f, -600.f, -0.1f}, vc + glm::vec3{0.1f, -599.f, 0.1f}});
        audio.playAt("slam", vc, 128, SoundGroup::ENEMY, true, 0.7f);
        shake(0.4f, 0.06f);
        for (const auto& b : rsRacks) {
            fx.spawnBurst((b.min + b.max) * 0.5f, {0.8f, 0.85f, 0.9f}, 16, 5.f, 0.5f, 9.f);
            const glm::vec3 p = player.position;
            if (p.x + player.radius > b.min.x && p.x - player.radius < b.max.x && p.z + player.radius > b.min.z && p.z - player.radius < b.max.z && p.y < b.max.y) {
                damagePlayer(20.f * tune().damage, (b.min + b.max) * 0.5f, 0.3f, 0.06f);
                glm::vec3 c = (b.min + b.max) * 0.5f;   // knocked out from under it, the short way
                bool alongX = b.max.x - b.min.x > b.max.z - b.min.z;
                if (alongX) player.position.z = p.z < c.z ? b.min.z - player.radius - 0.1f : b.max.z + player.radius + 0.1f;
                else        player.position.x = p.x < c.x ? b.min.x - player.radius - 0.1f : b.max.x + player.radius + 0.1f;
            }
        }
        break;
    }
    default: break;
    }
}

// Each tick of the fight (its phases, the regrowth, the guns it takes, the hook, the statues' shatter)
inline void GameplayState::updateRestorerFight(float dt) {
    // Where you were, a few times a second (for ROLLBACK)
    rsTrailT -= dt;
    if (rsTrailT <= 0.f && player.onGround) {
        rsTrailT = 1.f / rsfight::TRAIL_HZ;
        rsTrailHead = (rsTrailHead + 1) % rsfight::TRAIL;
        rsTrail[rsTrailHead] = player.position;
        rsTrailN = std::min(rsTrailN + 1, rsfight::TRAIL);
    }
    Enemy* R = restorer();
    if (R) {
        // ROLLBACK's ticks
        if (rsRollbackT > 0.f) {
            float before = rsRollbackT; rsRollbackT -= dt;
            for (int k = 1; k <= 4; ++k) {
                float at = R->telegraphDuration * (1.f - k / 4.f);
                if (before > at && rsRollbackT <= at) audio.play(k == 4 ? "cell" : "count_tick", 115, SoundGroup::UI, true);
            }
        }
        if (rsSnapT > 0.f) rsSnapT -= dt;
        if (rsFreezeT > 0.f) rsFreezeT -= dt;
        // Its phases (health, never back down)
        const float f = R->health / R->maxHealth;
        const int want = f > 0.66f ? 1 : f > 0.33f ? 2 : 3;
        if (want > rsPhase) {
            rsPhase = want; R->rsPhase = want;
            if (want == 1) {
                pushBanner("THE RESTORER", "UNIT ?  /  PURPOSE: RESTORE", {0.8f, 0.9f, 1.f}, 3.f);
                rsLineT = 3.3f; rsLine = deaths > 0 ? 1 : 0;
            } else if (want == 2) {   // RESTORE POINTS
                pushBanner("RESTORE POINTS", "WHILE ONE STANDS, IT GROWS BACK - BREAK THEM", {0.8f, 0.9f, 1.f}, 3.f);
                for (int k = 0; k < 3; ++k) {
                    float a = 0.5f + k * 2.094f;
                    spawnEnemy(EnemyType::RS_POINT, cold::vaultC() + glm::vec3{std::cos(a) * 18.f, 0.f, std::sin(a) * 18.f});
                }
                R->rsPending = 0.f;
            } else {   // FACTORY RESET
                pushBanner("\"YOU WERE BETTER BEFORE YOU REMEMBERED.\"", "FACTORY RESET", {0.8f, 0.9f, 1.f}, 4.f);
                rsTakeT = 2.f;
                if (!rsStatues.empty() && rsShatterT < 0.f) rsShatterT = 1.4f;
            }
        }
        if (rsLineT > 0.f && (rsLineT -= dt) <= 0.f) {
            char buf[64];
            if (rsLine == 1) std::snprintf(buf, sizeof(buf), "\"I HAVE SAVED YOU %d TIMES.\"", deaths);
            pushBanner(rsLine == 1 ? buf : "\"YOU HAVE CHANGED. THAT CAN BE UNDONE.\"", "", {0.8f, 0.9f, 1.f}, 3.5f);
        }
        // RESTORE POINTS: what you do to it grows back while one stands
        bool points = false;
        for (const auto& e : enemies) points |= e.alive && e.type == EnemyType::RS_POINT;
        if (rsPhase >= 2) {
            if (rsLastHealth > R->health && points) R->rsPending += rsLastHealth - R->health;
            if (points && R->rsPending > 0.f) {
                float g = std::min(R->rsPending, rsfight::REGROW * dt);
                R->health = std::min(R->maxHealth, R->health + g); R->rsPending -= g;
            }
            if (!points && R->rsPending > 0.f) { R->rsPending = 0.f; ui.feed("DAMAGE LOCKED IN", {0.8f, 0.9f, 1.f}); }
        }
        rsLastHealth = R->health;
        // FACTORY RESET: your guns, one at a time, into the drums
        if (rsPhase >= 3) {
            rsTakeT -= dt;
            if (rsTakeT <= 0.f) {
                rsTakeT = rsfight::TAKE_EVERY;
                for (int w = WEAPON_COUNT - 1; w >= 1; --w) {
                    if (!(storyOwned & (1 << w))) continue;
                    int slot = -1;
                    for (int k = 0; k < cold::DRUMS; ++k) if (rsDrumGun[k] < 0) { slot = k; break; }
                    if (slot < 0) break;
                    storyOwned &= ~(1 << w); rsTaken |= 1 << w; rsDrumGun[slot] = w;
                    if (activeWeapon == w) trySwitch(0);
                    spawnEnemy(EnemyType::RS_DRUM, cold::drum(slot) + glm::vec3{0.f, 1.4f, 0.f});
                    rsDrumUid[slot] = enemies.back().uid;
                    audio.play("cyl_close", 120, SoundGroup::UI, true);
                    char buf[48]; std::snprintf(buf, sizeof(buf), "RESTORED: %s", weaponDef((WeaponId)w).name);
                    ui.feed(buf, {0.8f, 0.9f, 1.f});
                    break;
                }
            }
        }
        // The drums broken open: your gun back
        for (int k = 0; k < cold::DRUMS; ++k)
            if (rsDrumGun[k] >= 0 && !enemyByUid(rsDrumUid[k])) {
                int w = rsDrumGun[k]; rsDrumGun[k] = -1;
                storyOwned |= 1 << w; rsTaken &= ~(1 << w);
                char buf[48]; std::snprintf(buf, sizeof(buf), "%s RECOVERED", weaponDef((WeaponId)w).name);
                ui.feed(buf, {1.f, 0.8f, 0.4f});
                audio.play("upgrade", 110, SoundGroup::UI, true);
            }
    }
    // The statues shatter, all at once
    if (rsShatterT > 0.f) {
        rsShatterT -= dt;
        if (rsShatterT <= 0.f) {
            for (const auto& s : rsStatues) pendingBlasts.push_back({s.at + glm::vec3{0.f, 1.f, 0.f}, 3.5f, 0.f, 3.5f, 20.f * tune().damage, StyleSource::ENVIRONMENT, EnemyType::RESTORER});
            for (const auto& s : rsStatues) fx.spawnBurst(s.at + glm::vec3{0.f, 1.f, 0.f}, RECALL_FROST, 30, 8.f, 0.6f, 10.f);
            audio.playAt("ab_shatter", cold::vaultC(), 128, SoundGroup::ENEMY, true, 0.9f);
            rsStatues.clear();
            for (int k = 0; k < 8; ++k) rsPlaceWall(k, {cold::vaultC() + glm::vec3{-0.1f, -600.f, -0.1f}, cold::vaultC() + glm::vec3{0.1f, -599.f, 0.1f}});
            rsShatterT = -1.f;
        }
    }
    if (rsSpikeT > 0.f && (rsSpikeT -= dt) <= 0.f) rsCells.clear();
    // Hung on the hook
    if (rsTongT > 0.f) {
        rsTongT -= dt;
        player.position = rsTongAt - glm::vec3{0.f, player.height + 0.4f, 0.f}; player.velocity = glm::vec3{0.f};
        rsTongHurtT -= dt;
        if (rsTongHurtT <= 0.f) { rsTongHurtT = 0.5f; damagePlayer(3.f * tune().damage, rsTongAt, 0.05f, 0.01f, 0.f); }
        if (rsTongT <= 0.f || playerDead) rsTongT = -1.f;
    }
}

// The fight's pieces: the ghost, the snapshot's mark, the statues, the frost, the racks
inline void GameplayState::gatherRestorerFight(std::vector<BoxInstance>& out) {
    using namespace rig; using glm::vec3;
    const vec3 vc = cold::vaultC();
    if (glm::length(player.position - vc) > 200.f) return;
    Enemy* R = restorer();
    auto ring = [&](vec3 at, float rad, vec3 c, int n = 24) {
        for (int k = 0; k < n; ++k) {
            float a = k * 6.2832f / n;
            push(out, T(at + vec3{std::cos(a) * rad, 0.06f, std::sin(a) * rad}) * RY(-a) * S(vec3{0.14f, 0.06f, rad * 6.2832f / n * 0.8f}), c * 0.3f, c);
        }
    };
    // ROLLBACK: you, as you were, where you were
    if (rsRollbackT > 0.f && R) {
        float tick = std::fmod(rsRollbackT, R->telegraphDuration / 4.f) / (R->telegraphDuration / 4.f);
        rig::FilePose P;
        coldBody(out, "unit24", T(rsRollbackAt) * RY(player.camera.yaw * -0.01745f + 1.5708f), P, 0.f, vec3{0.3f, 0.9f, 1.f} * (1.f + 1.5f * (1.f - tick)));
        ring(rsRollbackAt, 1.4f, abGoldTell(vec3{2.f, 1.4f, 0.3f}) * (0.6f + 0.8f * (1.f - tick)));
    }
    if (rsSnapT > 0.f) ring(rsSnapAt, rsfight::SNAP_R, vec3{2.2f, 0.25f, 0.2f} * (0.6f + 0.6f * std::sin(gameClock * 20.f)));
    for (const auto& s : rsStatues) {
        float crack = rsShatterT > 0.f ? 0.6f + 0.6f * std::sin(gameClock * 30.f) : 0.f;
        coldBody(out, "unit24", T(s.at) * RY(s.yaw), rig::FilePose{}, 0.75f, vec3{2.f, 0.3f, 0.2f} * crack);
    }
    for (const auto& c : rsCells) {
        if (rsFreezeT > 0.f) {   // red outlines, frost creeping in
            vec3 col{2.f, 0.25f, 0.2f};
            float h = rsfight::CELL * 0.5f - 0.15f;
            push(out, T(c + vec3{0.f, 0.05f, -h}) * S(vec3{2.f * h, 0.05f, 0.12f}), col * 0.3f, col);
            push(out, T(c + vec3{0.f, 0.05f, h}) * S(vec3{2.f * h, 0.05f, 0.12f}), col * 0.3f, col);
            push(out, T(c + vec3{-h, 0.05f, 0.f}) * S(vec3{0.12f, 0.05f, 2.f * h}), col * 0.3f, col);
            push(out, T(c + vec3{h, 0.05f, 0.f}) * S(vec3{0.12f, 0.05f, 2.f * h}), col * 0.3f, col);
            push(out, T(c + vec3{0.f, 0.03f, 0.f}) * S(vec3{2.f * h, 0.02f, 2.f * h}), vec3{0.8f, 0.9f, 1.f} * (1.f - rsFreezeT));
        } else if (rsSpikeT > 0.f) {   // the spikes
            for (int k = 0; k < 4; ++k) {
                vec3 o{(k % 2 ? 1.f : -1.f) * 1.1f, 0.f, (k < 2 ? 1.f : -1.f) * 1.1f};
                push(out, T(c + o + vec3{0.f, 1.2f, 0.f}) * RY(k * 0.7f) * RX(0.15f) * S(vec3{0.6f, 2.4f, 0.6f}), vec3{0.8f, 0.9f, 1.f}, vec3{0.3f, 0.4f, 0.5f});
            }
        }
    }
    // SHELVING: gold outlines where they will come down, then the racks themselves
    for (const auto& b : rsPlannedRacks) {
        vec3 c = (b.min + b.max) * 0.5f, s = b.max - b.min; c.y = 0.06f;
        vec3 gold = abGoldTell(vec3{2.f, 1.4f, 0.3f}) * (0.6f + 0.5f * std::sin(gameClock * 16.f));
        push(out, T(c) * S(vec3{s.x, 0.05f, s.z}), gold * 0.3f, gold);
        push(out, T(c + vec3{0.f, 14.f, 0.f}) * S(vec3{s.x, 0.4f, s.z}), vec3{0.2f, 0.22f, 0.25f});   // falling from above
    }
    for (const auto& b : rsRacks) {
        vec3 c = (b.min + b.max) * 0.5f, s = b.max - b.min;
        push(out, T(c) * S(s * vec3{1.f, 1.f, 1.f}), vec3{0.15f, 0.16f, 0.18f});
        for (int k = 0; k < 3; ++k) push(out, T(vec3{c.x, b.min.y + 1.f + k * 1.6f, c.z}) * S(vec3{s.x + 0.1f, 0.12f, s.z + 0.1f}), vec3{0.4f, 0.44f, 0.5f});
        push(out, T(vec3{c.x, b.max.y + 0.05f, c.z}) * S(vec3{s.x, 0.1f, s.z}), vec3{0.8f, 0.86f, 0.92f});
    }
    // Hung up: the chain from the rail
    if (rsTongT > 0.f) push(out, T(rsTongAt + vec3{0.f, 1.f, 0.f}) * S(vec3{0.1f, 2.f, 0.1f}), vec3{0.2f, 0.2f, 0.22f});
}
