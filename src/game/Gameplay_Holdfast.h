#pragma once
// =============================================================================
// Gameplay_Holdfast.h — HOLDFAST in the world (the numbers: player/Holdfast.h).
//
// The button fires a mag-spike along your aim. Into a wall: you are held to it
// on a line. Into a small enemy: it is nailed where it stands. Owned in
// Chapter 6 and with --holdfast anywhere; the Drift hands it over once that
// level exists (docs/chapter-6-design.md section 5, "ANCHOR").
// =============================================================================
#include "player/Holdfast.h"

inline bool GameplayState::holdfastOwned() const { return g_devHoldfast || (ring() && (holdfastFound || g_devRingTest)); }   // (the Drift hands it over: Gameplay_Ring.h)

// The SweepShelter a wall is (-1: none)
inline int GameplayState::shelterOfWall(int wall) const {
    if (wall < 0 || wall >= (int)level.walls.size()) return -1;
    for (int i = 0; i < (int)sweepShelters.size(); ++i) if (sweepShelters[i].wall == wall) return i;   // (a drifting one)
    const AABB& b = level.walls[wall].box;
    for (int i = 0; i < (int)sweepShelters.size(); ++i)
        if (glm::length(sweepShelters[i].box.min - b.min) < 0.01f && glm::length(sweepShelters[i].box.max - b.max) < 0.01f) return i;
    return -1;
}

inline void GameplayState::pressHoldfast() {
    const glm::vec3 o = player.camera.position, d = player.camera.forward();
    switch (holdfast.press()) {
    case Holdfast::Press::FIRE: {
        float best = Holdfast::RANGE; int wall = -1;
        for (int i = 0; i < (int)level.walls.size(); ++i) {
            const float t = rayBoxHit(o, d, level.walls[i].box);
            if (t > 0.f && t < best) { best = t; wall = i; }
        }
        // A small enemy nearer than the wall takes the spike instead
        Enemy* pin = nullptr; float pinT = best;
        for (auto& e : enemies) {
            if (!e.targetable() || isBoss(e.type) || e.radius() > 1.1f) continue;
            const glm::vec3 c = e.position + glm::vec3{0.f, e.height() * 0.5f, 0.f};
            const float along = glm::dot(c - o, d);
            if (along < 0.5f || along > pinT) continue;
            if (glm::length(c - (o + d * along)) > std::max(e.radius(), e.height() * 0.5f) + 0.25f) continue;
            pin = &e; pinT = along;
        }
        audio.play("cyl_close", 110, SoundGroup::PLAYER);
        if (pin) {
            holdfast.pin(pin->uid); holdfastPinAt = pin->position;
            pin->stunTimer = std::max(pin->stunTimer, Holdfast::PIN_TIME);
            fx.spawnTracer(o - glm::vec3{0.f, 0.25f, 0.f}, pin->position + glm::vec3{0.f, pin->height() * 0.5f, 0.f}, 0.05f, 0.25f, HOLDFAST_STEEL * 1.6f);
            fx.spawnHitSparks(pin->position + glm::vec3{0.f, pin->height() * 0.5f, 0.f}, HOLDFAST_STEEL);
            audio.playAt("clank", pin->position, 120, SoundGroup::PLAYER, true, 1.3f);
            ui.feed("PINNED", HOLDFAST_STEEL);
        } else if (wall >= 0) {
            const glm::vec3 at = o + d * best;
            holdfast.latch(at, player.position + glm::vec3{0.f, 1.f, 0.f}, wall, shelterOfWall(wall));
            fx.spawnHitSparks(at, HOLDFAST_STEEL);
            audio.playAt("clank", at, 128, SoundGroup::PLAYER, true, 0.8f);
            ui.feed("HELD", HOLDFAST_STEEL);
            if (holdfastHints < 1) { ++holdfastHints; ui.toast("HOLDFAST", "YOU ARE HELD TO IT. PRESS " + binds().label(Act::HOLDFAST) + " AGAIN TO LET GO", HOLDFAST_STEEL, 3.f); }
        } else {
            holdfast.miss();
            fx.spawnTracer(o - glm::vec3{0.f, 0.25f, 0.f}, o + d * Holdfast::RANGE, 0.04f, 0.15f, HOLDFAST_STEEL);
        }
        break;
    }
    case Holdfast::Press::RELEASED:
        audio.play("cyl_close", 80, SoundGroup::PLAYER);
        break;
    case Holdfast::Press::NONE: break;
    }
}

// The rope, straight after the player has moved: the spike rides its body and
// you stay inside the line
inline void GameplayState::holdfastRope() {
    if (!holdfast.held()) return;
    if (const int m = level.moverOfWall(holdfast.body); m >= 0) holdfast.carry(level.movers[m].delta);
    glm::vec3 chest = player.position + glm::vec3{0.f, 1.f, 0.f};
    if (holdfast.constrain(chest, player.velocity)) player.position = chest - glm::vec3{0.f, 1.f, 0.f};
}

inline void GameplayState::tickHoldfast(float dt) {
    if (playerDead) holdfast.cut();
    if (holdfast.held() && holdfast.shelter >= 0 && !sweepShelters[holdfast.shelter].alive) {   // what it was in has broken
        holdfast.cut(); ui.feed("LINE LOST", HOLDFAST_STEEL);
    }
    if (holdfast.pinning()) {
        Enemy* e = enemyByUid(holdfast.pinUid);
        if (!e || !e->targetable()) holdfast.pinGone();
        else {   // nailed where it stands
            e->position = holdfastPinAt; e->velocity = glm::vec3{0.f}; e->knockVel = glm::vec3{0.f};
            e->stunTimer = std::max(e->stunTimer, 0.1f);
        }
    }
    if (holdfast.tick(dt)) audio.play("cyl_close", 70, SoundGroup::PLAYER);
    if (holdfast.held()) {   // the line, from the forearm to the spike
        const glm::vec3 hand = player.camera.position + player.camera.right() * 0.28f - glm::vec3{0.f, 0.32f, 0.f} + player.camera.forward() * 0.35f;
        fx.spawnTracer(hand, holdfast.point, 0.03f, dt * 1.5f, HOLDFAST_STEEL * 1.2f);
    }
}

// The spike where it sits, and the nail over a pinned enemy
inline void GameplayState::gatherHoldfast(std::vector<BoxInstance>& out) {
    using namespace rig;
    if (holdfast.held())
        push(out, T(holdfast.point) * S(glm::vec3{0.14f}), HOLDFAST_STEEL * 0.5f, HOLDFAST_STEEL * 0.8f);
    if (holdfast.pinning())
        if (const Enemy* e = enemyByUid(holdfast.pinUid)) {
            const float h = e->height();
            push(out, T(e->position + glm::vec3{0.f, h + 0.5f, 0.f}) * S(glm::vec3{0.08f, 0.9f * holdfast.pin01() + 0.1f, 0.08f}), HOLDFAST_STEEL * 0.5f, HOLDFAST_STEEL * 1.4f);
        }
}
