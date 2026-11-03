#pragma once
// =============================================================================
// Gameplay_ColdEnemies.h — GameplayState: what Chapter 5's enemies do to the
// world (their AI is enemies/EnemyCold.h).
//   PICKER  its snatch lifts you and holds you hanging under it. Shoot it
//           (enough to hurt it) or punch, and it lets go; otherwise it drops
//           you after a few seconds.
//   CLERK   while one is up, an enemy you kill is restored: the Clerk's lamp
//           reaches to where it fell and, a moment later, it stands again.
//           Kill the Clerk in that moment and the restore fails.
// Included at the end of GameplayState.h.
// =============================================================================

inline Enemy* GameplayState::enemyByUid(int uid) {
    for (auto& e : enemies) if (e.alive && e.uid == uid) return &e;
    return nullptr;
}

// A PICKER's drop reached you
inline void GameplayState::pickerGrab(Enemy& e, bool lured) {
    if (lured || pickedBy >= 0 || playerDead) { e.pickT = 0.f; return; }   // (it took RECALL's statue, or you're already held)
    pickedBy = e.uid;
    pickedHealth = e.health;
    grapple.release();
    damagePlayer(e.ev.meleeDamage * level.arenas[director.arena].damageScale * tune().damage, e.position, 0.2f, 0.05f, 0.f);
    audio.playAt("hook_yank", e.position, 120, SoundGroup::ENEMY, true, 0.8f);
    ui.feed("PICKED - SHOOT IT OR PUNCH FREE", {1.f, 0.8f, 0.4f});
    shake(0.25f, 0.05f);
}

inline void GameplayState::pickerLetGo(bool broke) {
    if (Enemy* e = enemyByUid(pickedBy)) {
        e->pickT = 0.f;
        if (broke) e->stun(1.2f);
    }
    pickedBy = -1;
    player.velocity = {0.f, -2.f, 0.f};
    if (broke) {
        audio.play("punch", 110, SoundGroup::PLAYER);
        ui.feed("BROKE FREE", {0.9f, 0.95f, 1.f});
    }
}

// After the player's own move: hung under the claw
inline void GameplayState::holdInPicker(float dt) {
    Enemy* e = enemyByUid(pickedBy);
    if (!e || e->pickT <= 0.f || playerDead) { pickerLetGo(false); return; }
    if (e->health < pickedHealth - 25.f) { pickerLetGo(true); return; }   // shot off you
    player.position = e->position - glm::vec3{0.f, 2.4f, 0.f};
    player.velocity = glm::vec3{0.f};
    pickHurtT -= dt;
    if (pickHurtT <= 0.f) {   // the claw squeezes
        pickHurtT = 0.5f;
        damagePlayer(3.f * tune().damage, e->position, 0.06f, 0.015f, 0.f);
    }
}

// An enemy just died: is a Clerk up to restore it?
inline void GameplayState::clerkNoteDeath(const Enemy& e) {
    if (e.type == EnemyType::CLERK || isBoss(e.type) || isMiniBoss(e.type) || e.type == EnemyType::CONDUIT) return;
    const Enemy* best = nullptr; float bestD = 45.f;
    for (const auto& c : enemies)
        if (c.alive && c.type == EnemyType::CLERK && c.targetable()) {
            float d = glm::length(c.position - e.position);
            if (d < bestD) { bestD = d; best = &c; }
        }
    if (!best) return;
    if (level.baseFloor(e.position.x, e.position.z) < e.position.y - 20.f) return;   // (it fell into a void: nothing to stand back up on)
    pendingRestores.push_back({e.type, e.position, 2.4f, best->uid});
    ui.feed("RESTORING", {1.f, 0.8f, 0.35f});
}

inline void GameplayState::tickRestores(float dt) {
    for (auto& r : pendingRestores) {
        r.t -= dt;
        Enemy* c = enemyByUid(r.clerk);
        if (!c) { r.t = -99.f; fx.spawnBurst(r.at + glm::vec3{0.f, 0.8f, 0.f}, {1.f, 0.7f, 0.3f}, 10, 2.f, 0.4f, 6.f); continue; }   // the Clerk is down: nothing comes back
        // The lamp reaching to where it fell, and the parts gathering back up off the floor
        if (std::fmod(r.t, 0.12f) < dt) fx.spawnTracer(c->position + glm::vec3{0.f, 0.95f, 0.3f}, r.at + glm::vec3{0.f, 0.6f, 0.f}, 0.04f, 0.15f, glm::vec3{1.6f, 1.1f, 0.4f});
        if (std::fmod(r.t, 0.2f) < dt) {
            fx.spawnBurst(r.at + glm::vec3{0.f, 0.3f, 0.f}, statsOf(r.type).color, 3, 1.5f, 0.5f, -5.f);
            audio.playAt("count_tick", r.at, 70, SoundGroup::ENEMY, true, 1.6f);
        }
        if (r.t <= 0.f && r.t > -50.f) {
            spawnEnemy(r.type, r.at);
            audio.playAt("cyl_close", r.at, 100, SoundGroup::ENEMY, true, 1.2f);
            ui.feed("RESTORED", {1.f, 0.8f, 0.35f});
            r.t = -99.f;
        }
    }
    pendingRestores.erase(std::remove_if(pendingRestores.begin(), pendingRestores.end(), [](const PendingRestore& r) { return r.t <= -50.f; }),
                          pendingRestores.end());
}
