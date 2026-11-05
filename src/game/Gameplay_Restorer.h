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
constexpr float REGROW = 90.f;                // health a second it gets back while a restore point stands
constexpr float TAKE_EVERY = 7.f;             // FACTORY RESET: a gun every this many seconds
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
    rsFinT = -1.f; rsFalterSeen = false; rsHintT = -1.f;
    if (rsFinItem >= 0 && rsFinItem < (int)interactables.size()) interactables[rsFinItem].active = false;
    rsFinItem = -1;
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
        audio.music.rewind();   // the score winds back with you
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
        // dev (--overlay rsfinish): beaten after a few seconds, then straight into the finisher (footage)
        if (g_devOverlay.find("rsfinish") != std::string::npos && !R->rsFalter && elapsedTime > 3.f) R->takeDamage(R->health + 10.f);
        // dev (--overlay rs2 / rs3): straight into its second or third phase (screenshots)
        if ((g_devOverlay == "rs2" || g_devOverlay == "rs3") && rsPhase == 1 && R->health > R->maxHealth * 0.5f)
            R->health = R->maxHealth * (g_devOverlay == "rs2" ? 0.6f : 0.3f);
        // Its phases (health, never back down)
        const float f = R->health / R->maxHealth;
        const int want = f > 0.66f ? 1 : f > 0.33f ? 2 : 3;
        if (want > rsPhase) {
            rsPhase = want; R->rsPhase = want;
            if (want == 1) {
                pushBanner("THE RESTORER", "UNIT ?  /  PURPOSE: RESTORE", {0.8f, 0.9f, 1.f}, 3.f);
                rsLineT = 3.3f; rsLine = deaths > 0 ? 1 : 0;
                rsHintT = 7.2f;   // then what to do about it
            } else if (want == 2) {   // RESTORE POINTS
                pushBanner("RESTORE POINTS", "WHILE ONE STANDS, ITS WOUNDS GROW BACK - BREAK ALL THREE", {0.8f, 0.9f, 1.f}, 3.5f);
                for (int k = 0; k < 3; ++k) {
                    float a = 0.5f + k * 2.094f;
                    spawnEnemy(EnemyType::RS_POINT, cold::vaultC() + glm::vec3{std::cos(a) * 18.f, 0.f, std::sin(a) * 18.f});
                }
                R->rsPending = 0.f;
            } else {   // FACTORY RESET
                pushBanner("\"YOU WERE BETTER BEFORE YOU REMEMBERED.\"", "FACTORY RESET", {0.8f, 0.9f, 1.f}, 4.f);
                rsTakeT = 3.5f;
                rsHintT = 4.2f;   // then: where your guns go
                if (!rsStatues.empty() && rsShatterT < 0.f) rsShatterT = 1.4f;
            }
        }
        if (rsHintT > 0.f && (rsHintT -= dt) <= 0.f) {
            if (rsPhase >= 3) pushBanner("IT IS TAKING YOUR GUNS", "EACH GOES INTO A DRUM ON THE RIM - BREAK THE DRUM, TAKE IT BACK", {1.f, 0.8f, 0.4f}, 4.f);
            else pushBanner("SHOOT ITS GLASS DOME", "GOLD: IT PULLS YOU BACK - KEEP THAT SPOT SAFE OR RECALL  /  RED: GET OUT  /  WHITE: PARRY", {0.8f, 0.9f, 1.f}, 5.f);
        }
        if (R->rsFalter) rsHintT = rsLineT = -1.f;   // (beaten: nothing more to say over the finisher)
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
        if (rsPhase >= 3 && !R->rsFalter) {
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

// Where its dome is, in the world (it kneels for the finisher: the rig's own transforms, EnemyModel.h)
inline glm::vec3 GameplayState::rsDomeWorld(const Enemy& e) const { return rsbody::domeWorld(e); }

// It is down: you climb it and drive the RECALL module into its dome (first person, about four seconds)
//   the camera climbs to its bowed dome; it asks you to wait; your left hand takes the dome's collar, your
//   right draws the module off your hip and drives it into the lens; cracks run out over the glass as you
//   twist it and its lamp turns your colour, ticking backwards; the glass bursts and throws you clear, back on
//   your feet in front of it as it starts to rewind (Gameplay_Cold.h)
namespace rsfin {
constexpr float CLIMB = 0.75f, LINE = 0.45f, GRIP = 1.05f, DRAW0 = 0.8f, DRAW1 = 1.35f, HIT = 1.6f;
constexpr float CYAN = 2.05f, BREAK = 2.75f, END = 3.8f;
constexpr float CLOSE = 1.5f;    // how far in front of the dome's centre the camera holds (its glass in arm's reach)
inline float sm(float a, float b, float x) { float u = std::clamp((x - a) / (b - a), 0.f, 1.f); return u * u * (3.f - 2.f * u); }
}

inline void GameplayState::startRecallFinisher() {
    Enemy* R = restorer();
    if (!R || !R->rsFalter || rsFinT >= 0.f) return;
    rsFinT = 0.f; rsFinFrom = player.camera.position;
    if (rsFinItem >= 0 && rsFinItem < (int)interactables.size()) interactables[rsFinItem].active = false;
    grapple.release();
    banners.clear();   // (no stale "IT FALTERS" over it)
    audio.play("dash", 110, SoundGroup::PLAYER);
    audio.duck(10.f, rsfin::END);
}

// Where the finisher ends: on your feet in front of it, looking up at its dome
inline glm::vec3 GameplayState::rsFinStand(const Enemy& R) const {
    const glm::vec3 fwd{std::sin(R.yaw), 0.f, std::cos(R.yaw)};
    glm::vec3 d = rsDomeWorld(R); d.y = R.position.y;
    return d + fwd * 4.5f;   // (in front of its dome, which hangs well ahead of its feet as it kneels)
}

inline void GameplayState::updateRecallFinisher(float dt) {
    Enemy* R = restorer();
    // It falters: say so, and put the finisher in reach
    if (R && R->rsFalter && !rsFalterSeen) {
        rsFalterSeen = true;
        pushBanner("IT FALTERS", "GET CLOSE  -  " + binds().label(Act::INTERACT) + ": DRIVE THE RECALL MODULE INTO ITS DOME", RECALL_FROST, 4.f);
        audio.playAt("ab_creak", R->position, 128, SoundGroup::ENEMY, true, 0.6f);
        Interactable it;
        it.position = R->position + glm::vec3{0.f, 1.5f, 0.f};
        it.box = {R->position + glm::vec3{-3.5f, 0.f, -3.5f}, R->position + glm::vec3{3.5f, 5.f, 3.5f}};
        it.label = "DRIVE THE RECALL MODULE INTO ITS DOME";
        it.onInteract = [this]() { startRecallFinisher(); };
        rsFinItem = (int)interactables.size();
        interactables.push_back(std::move(it));
    }
    if (g_devOverlay.find("rsfinish") != std::string::npos && R && R->rsFalter && R->rsKneel >= 1.f && rsFinT < 0.f) startRecallFinisher();   // dev: straight into it (footage)
    if (rsFinT < 0.f || !R) return;
    using namespace rsfin;
    const float before = rsFinT;
    rsFinT += dt;
    player.velocity = glm::vec3{0.f};
    const glm::vec3 dome = rsDomeWorld(*R);
    auto crossed = [&](float t) { return before < t && rsFinT >= t; };
    SfxMixer::Opts o; o.group = SoundGroup::UI; o.priority = true; o.unducked = true;
    if (crossed(LINE)) { o.pitch = 0.8f; audio.playOpts("count_tick", o); }
    if (crossed(GRIP)) { audio.playAt("land_metal", dome, 120, SoundGroup::PLAYER, true, 0.8f); camFeel.dip(0.2f); }
    if (crossed(DRAW0 + 0.15f)) { o.pitch = 1.2f; audio.playOpts("cyl_open", o); }
    if (crossed(HIT)) {   // in
        o.pitch = 0.7f; audio.playOpts("cyl_close", o);
        audio.playAt("clank", dome, 128, SoundGroup::ENEMY, true, 1.3f);
        o.pitch = 0.6f; audio.playOpts("finisher", o);
        shake(0.45f, 0.06f); hitStopFrames = std::max(hitStopFrames, 8); fovKick = std::max(fovKick, 5.f);
        fx.spawnBurst(dome, {0.85f, 0.95f, 1.f}, 24, 4.f, 0.5f, 3.f);
    }
    if (crossed(CYAN)) { audio.playAt("ab_zap", dome, 128, SoundGroup::ENEMY, true, 0.8f); fx.spawnBurst(dome, {0.3f, 0.9f, 1.f}, 30, 4.f, 0.5f, 0.f); }
    if (rsFinT > CYAN && rsFinT < BREAK) {   // ticking backwards, quicker and quicker
        const float u = (rsFinT - CYAN) / (BREAK - CYAN), every = 0.2f - 0.15f * u;
        if (std::fmod(rsFinT, every) < dt) { o.pitch = 1.4f - 0.5f * u; audio.playOpts("count_tick", o); }
    }
    if (crossed(BREAK)) {   // the glass goes
        audio.playAt("ab_shatter", dome, 128, SoundGroup::ENEMY, true, 0.7f);
        o.pitch = 0.45f; audio.playOpts("explosion", o);
        fx.spawnBurst(dome, {0.85f, 0.95f, 1.f}, 90, 10.f, 1.f, 8.f);
        fx.spawnBurst(dome, {0.3f, 0.9f, 1.f}, 40, 6.f, 0.8f, 2.f);
        explosionFlashTimer = 0.35f; explosionFlashPos = dome;
        shake(0.8f, 0.09f); hitStopFrames = std::max(hitStopFrames, 10);
    }
    if (rsFinT >= END) {   // and it is over: the rewind (Gameplay_Cold.h) takes it from here
        rsFinT = -1.f;
        R->rsFinished = true;
        const glm::vec3 stand = rsFinStand(*R), domeAt = rsDomeWorld(*R);
        if (R->takeDamage(R->health + 10.f)) onEnemyKilled(*R, StyleSource::PUNCH);
        styleSystem.addStyle(200.f, StyleSource::PUNCH);
        player.position = stand; player.velocity = glm::vec3{0.f};
        player.camera.position = player.position + glm::vec3{0.f, player.eyeHeight, 0.f}; prevCamPos = player.camera.position;
        const glm::vec3 d = glm::normalize(domeAt - player.camera.position);   // (looking where the finisher left you looking)
        player.camera.yaw = glm::degrees(std::atan2(d.z, d.x)); player.camera.pitch = glm::degrees(std::asin(glm::clamp(d.y, -1.f, 1.f)));
    }
}

// The finisher's camera: up onto it, close in front of its dome; jolted in as the module goes in; thrown
// back off it when the glass goes, down to your feet where the fight resumes
inline bool GameplayState::recallFinisherCam(glm::vec3& eye, glm::vec3& look) {
    Enemy* R = restorer();
    if (rsFinT < 0.f || !R) return false;
    using namespace rsfin;
    const float t = rsFinT;
    const glm::vec3 dome = rsDomeWorld(*R), fwd{std::sin(R->yaw), 0.f, std::cos(R->yaw)};
    const glm::vec3 close = dome + fwd * CLOSE + glm::vec3{0.f, 0.42f, 0.f};   // (a little above it, over its collar)
    eye = glm::mix(rsFinFrom, close, sm(0.f, CLIMB, t));
    { const float d = t - HIT; if (d > 0.f && d < 0.25f) eye -= fwd * (0.12f * (1.f - d / 0.25f)); }          // (leaning into the blow)
    const glm::vec3 stand = rsFinStand(*R) + glm::vec3{0.f, player.eyeHeight, 0.f};
    const float off = sm(BREAK, END - 0.1f, t);
    eye = glm::mix(eye, stand, off);
    eye.y += 0.6f * std::sin(off * 3.1416f);   // (an arc: thrown up and back, then down)
    look = dome - glm::vec3{0.f, 0.08f, 0.f};
    return true;
}

// The camera's frame, for things the finisher's hands hold (camera space: x right, y up, z ahead)
inline glm::vec3 GameplayState::rsFinToWorld(glm::vec3 c) const {
    glm::vec3 eye, look;
    if (!const_cast<GameplayState*>(this)->recallFinisherCam(eye, look)) return eye;
    const glm::vec3 f = glm::normalize(look - eye), r = glm::normalize(glm::cross(f, glm::vec3{0.f, 1.f, 0.f})), u = glm::cross(r, f);
    return eye + r * c.x + u * c.y + f * c.z;
}

// Where the right hand holds the module, in camera space (and so where the module is)
inline glm::vec3 GameplayState::rsFinRightHand() const {
    using namespace rsfin; using glm::vec3;
    const float t = rsFinT;
    vec3 at{0.3f, -0.37f, 0.33f};
    at = glm::mix(at, vec3{0.34f, -0.62f, 0.12f}, sm(DRAW0, DRAW0 + 0.2f, t) * (1.f - sm(DRAW0 + 0.2f, DRAW1, t)));   // down to the hip...
    at = glm::mix(at, vec3{0.18f, -0.16f, 0.3f}, sm(DRAW0 + 0.2f, DRAW1, t));                                        // ...up, cocked
    const float thrust = t < HIT ? sm(HIT - 0.22f, HIT, t) : 1.f;
    at = glm::mix(at, vec3{0.03f, -0.13f, 0.7f}, thrust * thrust);                                                    // driven in
    at += vec3{0.f, 0.004f * std::sin(t * 55.f), 0.f} * (t > HIT && t < BREAK ? 1.f : 0.f);                           // (straining)
    at = glm::mix(at, vec3{0.38f, -0.75f, 0.1f}, sm(BREAK, BREAK + 0.35f, t));                                        // thrown off
    return at;
}

// UNIT 24's hands in it
inline int GameplayState::recallFinArms(ViewModel::FreeArm* A) const {
    using namespace rsfin; using glm::vec3;
    const float t = rsFinT;
    int n = 0;
    auto arm = [&](float s, vec3 at, vec3 fingers, vec3 back, armfile::HandPose hand) {
        A[n].shoulder = {s * 0.34f, -0.62f, -0.02f};
        A[n].fist = armfile::handFrame(at, glm::normalize(fingers), glm::normalize(back), s);
        A[n].side = s; A[n].hand = hand;
        ++n;
    };
    {   // the left: up to the dome's collar, held there, flung off when it bursts
        vec3 at{-0.3f, -0.37f, 0.33f};
        const float reach = sm(GRIP - 0.4f, GRIP, t) * (1.f - sm(BREAK, BREAK + 0.3f, t));
        at = glm::mix(at, vec3{-0.27f, -0.36f, 0.66f}, reach);
        at += vec3{0.003f * std::sin(t * 47.f), 0.f, 0.f} * reach;
        armfile::HandPose pose = reach > 0.9f ? armfile::handGrip() : reach > 0.2f ? armfile::handSpread() : armfile::handRelaxed();
        if (t > BREAK) at += vec3{-0.15f, -0.3f, -0.1f} * sm(BREAK, BREAK + 0.3f, t);
        arm(-1.f, at, vec3{0.35f, 0.6f, 1.f}, vec3{-0.8f, 0.4f, -0.1f}, pose);
    }
    {   // the right: the module, drawn, driven in, twisted
        const vec3 at = rsFinRightHand();
        const float twist = sm(HIT + 0.15f, BREAK - 0.1f, t) * 1.2f;
        const vec3 back = glm::normalize(vec3{0.9f * std::cos(twist), 0.3f + 0.9f * std::sin(twist), -0.1f});
        arm(1.f, at, vec3{-0.12f, 0.35f, 1.f}, back, t > DRAW0 + 0.15f && t < BREAK + 0.1f ? armfile::handGrip() : armfile::handRelaxed());
    }
    return n;
}

// Over it: bars, its plea, the white of the blow and of the glass going, your colour taking it
inline void GameplayState::renderRecallFin() {
    using namespace rsfin;
    const float t = rsFinT;
    ui.begin2D(); auto& U = ui.ui;
    const int bar = (int)(58.f * sm(0.f, 0.4f, t) * (1.f - sm(END - 0.35f, END, t)));
    if (bar > 0) { U.rect(0, 0, SCREEN_W, bar, {0.f, 0.01f, 0.02f, 1}); U.rect(0, SCREEN_H - bar, SCREEN_W, bar, {0.f, 0.01f, 0.02f, 1}); }
    const float say = sm(LINE, LINE + 0.25f, t) * (1.f - sm(HIT - 0.25f, HIT, t));
    if (say > 0.01f) U.textShadow("\"WAIT. I CAN STILL SAVE YOU.\"", SCREEN_W / 2.f, SCREEN_H - 110.f, 2, {0.82f, 0.9f, 1.f, say}, true);
    float flash = 0.f;
    { const float d = t - HIT; if (d > 0.f && d < 0.22f) flash = 0.45f * (1.f - d / 0.22f); }
    { const float d = t - BREAK; if (d > 0.f && d < 0.5f) flash = std::max(flash, 0.85f * (1.f - d / 0.5f)); }
    if (flash > 0.f) U.rect(0, 0, SCREEN_W, SCREEN_H, {0.85f, 0.95f, 1.f, flash});
    const float cyan = 0.16f * sm(CYAN, BREAK, t) * (1.f - sm(BREAK, BREAK + 0.2f, t));
    if (cyan > 0.f) for (int k = 0; k < 6; ++k) {   // your colour creeping in at the edges
        const float w = 30.f + k * 20.f, a = cyan * (1.f - k / 6.f);
        U.rect(0, 0, (int)w, SCREEN_H, {0.2f, 0.85f, 1.f, a}); U.rect(SCREEN_W - (int)w, 0, (int)w, SCREEN_H, {0.2f, 0.85f, 1.f, a});
    }
    ui.end2D();
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
    // RESTORE POINTS: a pale tether from each to its chest, bright while it is growing back what you did
    if (R && !R->rsFalter)
        for (const auto& e : enemies) {
            if (!e.alive || e.type != EnemyType::RS_POINT) continue;
            const vec3 a = e.position + vec3{0.f, 5.6f, 0.f}, b = R->position + vec3{0.f, 5.6f - 2.3f * R->rsKneel, 0.f};
            vec3 d = b - a; const float len = glm::length(d);
            if (len < 0.5f) continue;
            d /= len;
            const float live = R->rsPending > 0.f ? 1.f : 0.35f;
            for (int k = 0; k < 3; ++k) {   // three strands, a pulse running along them toward it
                const vec3 off = vec3{std::sin(gameClock * 1.3f + k * 2.1f), std::cos(gameClock * 1.1f + k * 2.1f), 0.f} * 0.12f;
                push(out, T(a + d * (len * 0.5f) + off) * gleaner::alongZ(vec3{0.f}, d) * S(vec3{0.035f, 0.035f, len}), {0.8f, 0.9f, 1.f}, vec3{0.5f, 0.75f, 1.f} * live);
            }
            const float u = std::fmod(gameClock * (R->rsPending > 0.f ? 1.4f : 0.5f) + e.uid * 0.37f, 1.f);
            push(out, T(a + d * (len * u)) * S(vec3{0.3f}), {0.9f, 0.95f, 1.f}, vec3{1.2f, 1.6f, 2.2f} * live);
        }
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
    // The finisher: the RECALL module in your hand, then in its dome; cracks running out over the glass
    if (rsFinT >= 0.f && R) {
        using namespace rsfin;
        const float t = rsFinT;
        const vec3 dome = rsDomeWorld(*R);
        glm::vec3 eye, look; recallFinisherCam(eye, look);
        const vec3 toEye = glm::normalize(eye - dome);
        const vec3 glow = t > CYAN ? vec3{0.3f, 1.2f, 1.6f} * (1.f + std::sin(gameClock * 40.f) * 0.4f) : RECALL_FROST * (0.6f + 0.6f * sm(DRAW0, DRAW1, t));
        if (t > DRAW0 + 0.15f && t < BREAK) {   // the module: in the right hand, then left in the glass when the hand lets go
            const vec3 hand = rsFinRightHand();
            const vec3 base = rsFinToWorld(hand + vec3{-0.02f, 0.03f, 0.04f}), tip = rsFinToWorld(hand + vec3{-0.03f, 0.05f, 0.36f});
            const vec3 d = glm::normalize(tip - base);
            push(out, T((base + tip) * 0.5f) * gleaner::alongZ(vec3{0.f}, d) * S(vec3{0.13f, 0.13f, 0.34f}), RECALL_FROST, glow);
            push(out, T(base) * gleaner::alongZ(vec3{0.f}, d) * S(vec3{0.17f, 0.17f, 0.06f}), vec3{0.3f}, glow * 0.6f);   // its cap, against your palm
            for (int k = 0; k < 3; ++k)   // its bands
                push(out, T(glm::mix(base, tip, 0.25f + 0.25f * k)) * gleaner::alongZ(vec3{0.f}, d) * S(vec3{0.15f, 0.15f, 0.02f}), vec3{0.2f, 0.22f, 0.25f});
        }
        if (t > HIT && t < BREAK) {   // cracks running out over the glass from where it went in
            const vec3 c = dome + toEye * (rsbody::DOME_R * 0.98f);
            const vec3 rx = glm::normalize(glm::cross(toEye, vec3{0.f, 1.f, 0.f})), ry = glm::cross(rx, toEye);
            const float spread = sm(HIT, BREAK - 0.15f, t);
            for (int k = 0; k < 11; ++k) {
                const float a = k * 0.571f + 0.3f, len = (0.2f + 0.5f * ((k * 7) % 5) / 4.f) * spread;
                vec3 dir = rx * std::cos(a) + ry * std::sin(a);
                dir = glm::normalize(dir - toEye * 0.25f);   // (curving back over the dome)
                push(out, T(c + dir * (len * 0.5f)) * gleaner::alongZ(vec3{0.f}, dir) * S(vec3{0.018f, 0.018f, len}), vec3{1.f}, glow * 1.2f);
            }
        }
    }
}
