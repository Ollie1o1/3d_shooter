#pragma once
// =============================================================================
// EnemyRestorer.h — THE RESTORER (Chapter 5's boss, docs/chapter-5-design.md
// section 8): its brain. It walks the Vault on its stilts, keeping a careful
// distance, and handles you like a museum piece. Each attack is a tell here and
// an event when it lands (ev.rsCast); what it does to the world (your past self,
// statues, frost, racks, the hook) is the game's: Gameplay_Restorer.h.
//
//   RS_ROLLBACK  gold: a ghost of you where you were 4 s ago, ticking; on the
//                last tick you are put back there (RECALL then overrides it).
//                Its next attack goes for that spot.
//   RS_SNAPSHOT  red: a camera flash at where you stand; a statue of you is left
//                there. They pile up as cover until it shatters them all.
//   RS_TONG      white: an arm folds back, then a fast grab in front of it.
//                Parry it; caught, you are hung up on a hook.
//   RS_FREEZE    red: grid cells frost over in a pattern, then spikes burst up.
//   RS_SHELVE    gold: racks slam down into the floor, turning it into aisles.
//
// Phases (the game sets rsPhase from its health): 1 rollback, snapshot, tongs;
// 2 adds the freeze and the shelving (and its restore points); 3 everything,
// quicker (and it takes your guns back).
// =============================================================================

#include <glm/gtc/matrix_transform.hpp>

inline void Enemy::thinkRestorer(float dt, const EnemyWorld& w, bool resolve) {
    const glm::vec3 to = flatTo(w.playerFeet);
    const float distance = glm::length(to);
    if (rsFalter) {   // beaten: it sinks to its knees and waits, its lamp low
        rsKneel = std::min(1.f, rsKneel + dt * 1.4f);
        velocity = {0.f, 0.f, 0.f}; attack = AttackKind::NONE; telegraphTimer = 0.f; attackTimer = 0.f;
        return;
    }
    attackPace = rsPhase >= 3 ? 1.3f : rsPhase == 2 ? 1.1f : 1.f;
    animPhase += dt * std::min(moveSpeed, 4.f) * 0.9f;
    if (telegraphTimer > 0.f) {
        velocity.x = velocity.z = 0.f;
        if (attack == AttackKind::RS_TONG) { if (telegraphTimer > 0.3f) rsAim = w.playerFeet; turnToward(flatTo(rsAim), dt, 4.f); }
        else turnToward(to, dt, 1.5f);
        return;
    }
    if (resolve) {
        const AttackKind k = attack; attack = AttackKind::NONE;
        ev.rsCast = k;
        if (k == AttackKind::RS_TONG) {   // the grab: in front of it, within its reach
            glm::vec3 facing{std::sin(yaw), 0.f, std::cos(yaw)};
            glm::vec3 d = flatTo(w.playerFeet);
            float dl = glm::length(d);
            if (dl < 7.5f && std::fabs(w.playerFeet.y - position.y) < 4.f && (dl < 1.5f || glm::dot(facing, d / dl) > 0.45f)) {
                ev.sortGrab = true; ev.meleeDamage = 14.f;
            }
            recover(ev.sortGrab ? 1.2f : 1.4f);
        } else recover(k == AttackKind::RS_SHELVE ? 1.f : 0.8f);
        return;
    }
    turnToward(to, dt, 1.6f);
    rangedMove(dt, w, 9.f, 16.f, stats().speed * (recoverTimer > 0.f ? 0.5f : 1.f));
    if (recoverTimer > 0.f || !attackReady(dt)) return;
    // What next: never the same twice running; after a ROLLBACK, something aimed at the spot it put you
    std::vector<AttackKind> menu{AttackKind::RS_ROLLBACK, AttackKind::RS_SNAPSHOT};
    if (distance < 9.f || rsAimSet) menu.push_back(AttackKind::RS_TONG);
    if (rsPhase >= 2) { menu.push_back(AttackKind::RS_FREEZE); menu.push_back(AttackKind::RS_SHELVE); }
    AttackKind pick;
    if (rsAimSet) pick = distance < 9.f ? AttackKind::RS_TONG : (rsPhase >= 2 ? AttackKind::RS_FREEZE : AttackKind::RS_SNAPSHOT);
    else {
        do pick = menu[(size_t)(rand() % (int)menu.size())]; while (menu.size() > 1 && (int)pick == rsLast);
    }
    rsLast = (int)pick; ++rsCount;
    if (!rsAimSet) rsAim = w.playerFeet;
    rsAimSet = false;
    const float hurry = rsPhase >= 3 ? 0.85f : 1.f;
    switch (pick) {
        case AttackKind::RS_ROLLBACK: startAttack(pick, 2.f); break;   // (the ghost's four ticks: not hurried)
        case AttackKind::RS_SNAPSHOT: startAttack(pick, 1.1f * hurry); break;
        case AttackKind::RS_TONG:     startAttack(pick, 0.7f * hurry); break;
        case AttackKind::RS_FREEZE:   startAttack(pick, 1.4f * hurry); break;
        default:                      startAttack(pick, 1.1f * hurry); break;
    }
}

// ---- Its body, shared by the model (EnemyModel.h), its weak point (headBox) and the finisher
// (Gameplay_Restorer.h): where the body above the stilts is, and the dome on it. Beaten, it kneels:
// the body sinks and bows, bringing the dome down to you.
namespace rsbody {
inline const glm::vec3 DOME{0.f, 7.05f, 1.75f};   // the dome's centre in the body's frame
constexpr float DOME_R = 0.78f;                    // its radius as drawn
inline glm::mat4 bodyFrame(const Enemy& e) {
    const float k = e.rsKneel * e.rsKneel * (3.f - 2.f * e.rsKneel);
    return glm::translate(glm::mat4(1.f), e.position) * glm::rotate(glm::mat4(1.f), e.yaw, glm::vec3{0.f, 1.f, 0.f})
         * glm::scale(glm::mat4(1.f), glm::vec3{e.scale})
         * glm::translate(glm::mat4(1.f), glm::vec3{0.f, -2.55f * k, 0.5f * k}) * glm::rotate(glm::mat4(1.f), 0.3f * k, glm::vec3{1.f, 0.f, 0.f});
}
inline glm::vec3 domeWorld(const Enemy& e) { return glm::vec3(bodyFrame(e) * glm::vec4(DOME, 1.f)); }
}
