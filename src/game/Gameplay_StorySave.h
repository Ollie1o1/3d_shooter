#pragma once

inline void GameplayState::saveStoryProgress() {
    if (!story() || g_practice || g_godMode || g_devCam || g_devNoMouse || g_bossRun >= 0
        || !g_devOverlay.empty() || !level.story.built) return;
    StorySave s;
    s.chapter = level.story.chapter; s.checkpoint = storyCheckpoint;
    s.owned = storyOwned; s.logs = storyLogsSeen; s.terminals = storyTerminalsSeen;
    s.relayIsolated = storyRelayIsolated; s.grapple = storyGrapple; s.bossDown = storyBossDown;
    s.completed = storyEndT >= 0; s.wakeSeen = wakeSeen;
    s.prog = prog; s.elapsed = elapsedTime;
    s.style = bankedStyle + styleSystem.earned; s.damage = bankedDamage + styleSystem.damageTaken;
    s.peak = peakStyle; s.kills = totalKills; s.shots = totalShots; s.hits = totalHits; s.deaths = deaths;
    s.save();
}

inline void GameplayState::restoreStoryProgress() {
    const auto& s = resumedStory;
    storySeenSection = storyCheckpoint = s.checkpoint;
    storyBossDown = s.bossDown;
    elapsedTime = s.elapsed; bankedStyle = s.style; bankedDamage = s.damage;
    styleSystem.earned = styleSystem.damageTaken = 0;
    peakStyle = s.peak; totalKills = s.kills; totalShots = s.shots; totalHits = s.hits; deaths = s.deaths;
    if (storyBossDown) {
        const int boss = level.story.bossSection;
        if (boss >= 0 && level.arenas[boss].exitDoor >= 0)
            level.doors[level.arenas[boss].exitDoor].locked = false;
        if (director.arena == boss) {
            director.queue.clear(); director.events.clear(); director.phase = WaveDirector::Phase::CLEARED;
        }
    }
    ui.feed("STORY RESTORED AT CHECKPOINT", {.35f,.85f,1.f});
}
