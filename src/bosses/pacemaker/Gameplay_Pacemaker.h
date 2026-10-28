#pragma once
// =============================================================================
// Gameplay_Pacemaker.h — GameplayState: THE PACEMAKER's fight around its mind
// (EnemyPacemaker.h): the leaps up the Tower and what they say, the beacon
// channel (its pillar of light, the HUD line, breaking it), and its death.
// Included at the end of GameplayState.h.
// =============================================================================

inline void GameplayState::onPacemakerEvents(const Enemy& e, const EnemyEvents& ev) {
    if (ev.pmPhaseStarted > 0) {
        static const char* T[] = {"", "IT RUNS FOR THE TOWER", "HIGHER", "OVERCLOCK"};
        static const char* S[] = {"", "AFTER IT - LIFTS, PADS, THE HOOK", "THE SECOND BALCONY", "THE ROOF - IT'S FASTER NOW"};
        pushBanner(T[ev.pmPhaseStarted], S[ev.pmPhaseStarted], ev.pmPhaseStarted == 3 ? glm::vec3{1.f, 0.35f, 0.1f} : glm::vec3{1.f, 0.6f, 0.15f}, 2.4f);
        audio.play("mini_sting", 128, SoundGroup::UI, true);
        audio.duck(5.f, 0.5f);
        shake(0.4f, 0.06f);
    }
    if (ev.pmChannel == 1) {
        pushBanner("IT'S LIGHTING THE BEACON", "HIT IT - BREAK THE CHANNEL", {1.f, 0.85f, 0.4f}, 2.6f);
        audio.play("drive_ready", 128, SoundGroup::UI, true);
    } else if (ev.pmChannel == -1) {
        ui.toast("CHANNEL BROKEN", "IT REELS - DOUBLE DAMAGE", {1.f, 0.75f, 0.2f}, 1.8f);
        parryFeedback(e.position + glm::vec3{0.f, e.height() * 0.6f, 0.f}, true);
        styleSystem.addStyle(60.f, StyleSource::PARRY);
    } else if (ev.pmChannel == 2) {
        pushBanner("THE BEACON BURNS", "IT DRANK IT IN", {1.f, 0.5f, 0.2f}, 2.2f);
        audio.play("overdrive_on", 100, SoundGroup::UI);
        fx.spawnShockwave(e.position, 10.f, {1.4f, 1.f, 0.5f});
    }
    if (e.pmChannel > 0.f) {   // the pillar of light it's drawing up out of the keep
        const glm::vec3 base = e.position + glm::vec3{0.f, 0.2f, 0.f};
        fx.spawnBurst(base + glm::vec3{frand(-0.6f, 0.6f), frand(0.f, 6.f), frand(-0.6f, 0.6f)}, {1.5f, 1.1f, 0.5f}, 2, 3.f, 0.5f, -6.f);
    }
}

inline void GameplayState::onPacemakerKilled(Enemy& e) {
    for (int k = 0; k < 8; ++k)
        fx.spawnBurst(e.position + glm::vec3{frand(-1.f, 1.f), frand(0.5f, 3.f), frand(-1.f, 1.f)}, {1.4f, 0.9f, 0.4f}, 30, 11.f, 0.9f, 2.f);
    fx.spawnShockwave(e.position, 14.f, {1.4f, 1.f, 0.5f});
    pushBanner("THE PACEMAKER STOPS", "", {1.f, 0.75f, 0.3f}, 3.f);
}
