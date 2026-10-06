#pragma once
// =============================================================================
// Gameplay_Penitent.h — the Descent's cage and the PENITENT, wired into
// GameplayState: the ride between waves, the chain anchors (shoot them or
// grapple on and rip them out), its hazards turned into damage, drawing and
// sound. Included at the end of GameplayState.h.
// =============================================================================

// The cage: a ride waits for the player to be aboard, the next wave waits for
// the ride, and the fall line follows the cage
inline void GameplayState::updateLift(float dt) {
    (void)dt;
    const Arena& ar = level.arenas[director.arena];
    if (ar.shift != ArenaShift::DESCENT) { director.hold = false; return; }
    auto& lift = level.lift;
    const glm::vec3 C{0.f, lift.y(), -840.f};
    if (lift.pending >= 0 && !lift.riding()) {
        if (player.onGround && level.onLift(player.groundWall)) {
            lift.start();
            liftRiding = true;
            for (auto& p : projSystem.pool) p.alive = false;   // nothing left over from the last floor
            audio.playAt("door_close", C, 128, SoundGroup::WORLD);
            for (int k = 0; k < 4; ++k)   // the chains take the weight, overhead
                audio.playAt("clank", glm::vec3{k < 2 ? -9.f : 9.f, C.y + 8.f, k % 2 ? -831.f : -849.f}, 70, SoundGroup::WORLD);
        } else if (!boardHinted && director.phase != WaveDirector::Phase::ACTIVE) {
            boardHinted = true;
            pushBanner("BOARD THE CAGE", "IT WON'T GO DOWN WITHOUT YOU", {1.f, 0.7f, 0.3f}, 2.5f);
        }
    }
    if (liftRiding && !lift.riding()) {   // arrived: a jolt, and health waiting on the cage
        liftRiding = false;
        boardHinted = false;
        shake(0.35f, 0.05f);
        audio.playAt("slam", C, 110, SoundGroup::WORLD);
        for (float x : {-4.f, 4.f})
            pickups.push_back({C + glm::vec3{x, 0.6f, 0.f}, glm::vec3{0.f}, 1e9f, PickupKind::ORB, C.y, C + glm::vec3{x, 0.6f, 0.f}});
    }
    director.hold = lift.busy();
    // The fall line: 25 m under the cage; a fall puts you back aboard
    Arena& mar = level.arenas[director.arena];
    mar.voidY = lift.y() - 25.f;
    mar.respawn = C;
}
