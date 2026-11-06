#pragma once
// =============================================================================
// Gameplay_Reliquary.h — GameplayState: THE RELIQUARY. The relics drift to the
// next wave's arrangement once a wave is cleared (the fight waits for them to
// settle); a fall puts you back on the last relic you stood on; Revenant souls
// flee and re-form; Weaver wires snare and wake the wave; drawing all of it.
// Included at the end of GameplayState.h.
// =============================================================================

inline void GameplayState::updateReliquary(float dt) {
    const Arena& ar = level.arenas[director.arena];
    if (ar.shift != ArenaShift::DRIFT) { relic.clear(); snareTimer = 0.f; player.speedCap = 0.f; return; }
    auto& fm = level.formation;
    // The drift: started by ArenaShifts on a cleared wave; the fight waits for it
    if (fm.gliding() && !driftWasGliding) {
        relic.clearWires();
        for (auto& p : projSystem.pool) p.alive = false;
        pickups.clear();
        for (int c = 0; c < 4; ++c) audio.playAt("door", fm.top(c), 110, SoundGroup::WORLD);   // stone grinding as each relic lets go
        shake(0.25f, 0.03f);
    }
    if (!fm.gliding() && driftWasGliding) {   // settled: a jolt, and health on two of the relics
        audio.playAt("slam", fm.top(0), 100, SoundGroup::WORLD);
        for (int c : {1, 3}) {
            glm::vec3 p = fm.top(c) + glm::vec3{0.f, 0.6f, 0.f};
            pickups.push_back({p, glm::vec3{0.f}, 1e9f, PickupKind::ORB, p.y - 0.6f, p});
        }
    }
    driftWasGliding = fm.gliding();
    director.hold = fm.gliding();
    // The last relic you stood on: where a fall puts you back
    if (player.onGround) { int c = fm.chunkOfWall(level, player.groundWall); if (c >= 0) lastChunk = c; }
    Arena& mar = level.arenas[director.arena];
    glm::vec3 landing;
    if (relicLanding(level,lastChunk,player.radius,player.height,landing))
        mar.respawn = landing + glm::vec3{0.f, 0.05f, 0.f};
    mar.voidY = fm.lowestTop() - 22.f;
    // Souls: fly; those that get there re-form their Revenant
    for (const Soul& s : relic.arrived(dt)) {
        glm::vec3 at = s.to;
        spawnEnemy(s.type, at, s.hollow);
        Enemy& e = enemies.back();
        e.maxHealth = e.health = s.bodyHealth; e.reforms = s.reforms; e.scale = s.scale;
        audio.playAt("v_revenant_reform", at + glm::vec3{0.f, 1.5f, 0.f}, 128, SoundGroup::ENEMY, true, 0.4f);
        fx.spawnBurst(at + glm::vec3{0.f, 1.f, 0.f}, {0.75f, 0.9f, 1.3f}, 40, 5.f, 0.9f, 6.f);
        ui.feed("IT CAME BACK", {0.75f, 0.9f, 1.3f});
    }
    // Wires: age, snare on touch (once per wire contact)
    relic.age(dt);
    if (snareTimer > 0.f) snareTimer -= dt;
    int wi = relic.touchedWire(player.position, player.height, player.radius);
    if (wi >= 0 && snareTimer <= 0.f) {
        snareTimer = RelicHazards::SNARE_TIME;
        damagePlayer(RelicHazards::SNARE_DAMAGE * level.arenas[director.arena].damageScale * tune().damage, player.position, 0.15f, 0.04f, 0.f);
        audio.playAt("v_weaver_twang", player.position + glm::vec3{0.f, 1.2f, 0.f}, 128, SoundGroup::ENEMY, true, 0.5f);
        ui.feed("SNARED", {0.75f, 0.35f, 1.1f});
        for (auto& e : enemies)   // the whole wave feels it
            if (e.targetable() && glm::length(e.position - player.position) < 30.f) e.attackTimer = e.stats().attackEvery;
        relic.cutWire(wi);   // it snaps
    }
    player.speedCap = snareTimer > 0.f ? 3.f : 0.f;   // snared: a crawl (Player::integrate), and no dash (Gameplay_Tick)
}

inline void GameplayState::onReliquaryEvents(Enemy& e, const EnemyEvents& ev) {
    if (ev.wire) {
        relic.addWire(e.uid, ev.wireA, ev.wireB);
        audio.playAt("v_weaver_twang", (ev.wireA + ev.wireB) * 0.5f, 90, SoundGroup::ENEMY, false, 0.3f);
    }
}

inline void GameplayState::releaseRevenantSoul(const Enemy& e, bool inVoid) {
    if (!revenantSoulEscapes(StyleSource::ENVIRONMENT, inVoid)) return;
    const Arena& ar = level.arenas[director.arena];
    const std::vector<glm::vec3>& spots = director.wave >= 0 && director.wave < (int)ar.waveGround.size() ? ar.waveGround[director.wave] : ar.groundSpawns;
    std::vector<glm::vec3> safe;
    for (auto p : spots)
        if (spawnClearance(level,p,e.radius(),e.height(),true)) safe.push_back(p);
    if (safe.empty()) for (int c=0;c<std::min(4,(int)level.formation.chunks.size());++c) {
        glm::vec3 p;
        if (relicLanding(level,c,e.radius(),e.height(),p)) safe.push_back(p);
    }
    if (safe.empty() || !relic.releaseSoul(e, soulDestination(safe, e.position, 15.f))) return;
    audio.playAt("v_revenant_soul", e.position + glm::vec3{0.f, e.height() * 0.6f, 0.f}, 128, SoundGroup::ENEMY, true, 0.4f);
    ui.feed("ITS SOUL RUNS", {0.75f, 0.9f, 1.3f});
}

// The parry key, aimed at a soul passing within reach: caught for good
inline bool GameplayState::catchSoulWithPunch() {
    glm::vec3 eye = player.camera.position, fwd = player.camera.forward();
    int i = relic.soulNear(eye + fwd * 1.5f, 2.5f);
    if (i < 0) return false;
    glm::vec3 at = relic.souls[i].pos;
    relic.removeSoul(i);
    parryFeedback(at, true);
    styleSystem.addStyle(80.f, StyleSource::PARRY);
    ui.toast("SOUL TAKEN", "BARE-HANDED", {0.75f, 0.9f, 1.3f}, 1.4f);
    return true;
}

inline void GameplayState::gatherReliquaryBoxes(std::vector<BoxInstance>& out) {
    using namespace rig;
    const float t = gameClock;
    // Details follow each slab's interpolated transform, including while it drifts.
    // The amber edge sits below the deck: no decorative lip to catch movement.
    if (!level.formation.chunks.empty() && glm::length(player.position-RELIQUARY_Q) < 220.f) {
        const glm::vec3 amber{1.1f,.55f,.16f}, dark{.13f,.12f,.14f};
        const glm::vec3 accents[]={{1.f,.5f,.17f},{1.2f,.28f,.07f},{.75f,.8f,.95f},{.12f,.8f,1.f}};
        auto box=[&](glm::vec3 p,glm::vec3 size,glm::vec3 col,glm::vec3 glow=glm::vec3{0}) {
            push(out,T(p)*S(size),col,glow);
        };
        auto& fm=level.formation;
        for(int c=0;c<(int)fm.chunks.size();++c) {
            const auto& ch=fm.chunks[c];
            glm::vec3 p=fm.top(c);
            if(!ch.movers.empty())p-=level.movers[ch.movers[0]].delta*(1.f-renderAlpha);
            const auto half=ch.half;
            for(float side:{-1.f,1.f}) {
                box(p+glm::vec3{0,-.12f,side*(half.y-.18f)},{half.x*2-.7f,.055f,.08f},amber*.22f,amber*.6f);
                if(c<4)box(p+glm::vec3{side*(half.x-.2f),-.8f,0},{.3f,.42f,half.y*2-.5f},dark);
            }
            if(c>=4 || ambientDetail()==0)continue;
            // Corner fasteners and broken service ribs make the slabs feel constructed.
            for(float sx:{-1.f,1.f})for(float sz:{-1.f,1.f}) {
                box(p+glm::vec3{sx*(half.x-.5f),-.035f,sz*(half.y-.5f)},{.4f,.04f,.4f},dark);
                box(p+glm::vec3{sx*(half.x-.5f),-.005f,sz*(half.y-.5f)},{.12f,.025f,.12f},accents[c]*.35f,accents[c]*.45f);
            }
            if(c==0) {   // the Yard: fractured brick courses, brass on its familiar dais
                for(int k=0;k<7;++k)box(p+glm::vec3{-8.5f+k*2.5f,1.22f,-9.7f},{1.7f,.09f,.58f},{.55f,.32f,.24f});
                for(float side:{-1.f,1.f})box(p+glm::vec3{side*2.8f,.81f,0},{.055f,.025f,5.5f},amber*.3f,amber*.35f);
                box(p+glm::vec3{8,8.9f,8},{1.5f,.15f,1.5f},dark);
                box(p+glm::vec3{8,8.72f,8},{.75f,.16f,.75f},accents[c]*.3f,accents[c]*1.1f);
            } else if(c==1) {   // the Foundry: a banked furnace behind its surviving grille
                box(p+glm::vec3{-7,1.7f,-3.98f},{3.4f,2.6f,.03f},accents[c]*.18f,accents[c]*.8f);
                for(int k=0;k<9;++k)box(p+glm::vec3{-8.5f+k*.375f,1.7f,-3.91f},{.075f,2.7f,.09f},dark);
                for(int k=0;k<4;++k)box(p+glm::vec3{6.6f,1.5f+k*2.f,-5.98f},{1.1f,.12f,.04f},accents[c]*.3f,accents[c]*.45f);
            } else if(c==2) {   // the Spire: dressed stone and lit relief on its columns
                for(float z:{-4.f,4.f}) {
                    box(p+glm::vec3{4,7.85f,z},{1.25f,.25f,1.25f},{.62f,.59f,.56f});
                    box(p+glm::vec3{3.48f,4.f,z},{.035f,5.8f,.12f},accents[c]*.25f,accents[c]*.6f);
                }
                for(int k=0;k<4;++k)box(p+glm::vec3{-4,2.51f,-5.3f+k*3.5f},{4.7f,.02f,.045f},{.25f,.24f,.27f});
            } else {   // the Core: fractured reactor pillars with recessed cyan cells
                for(float x:{-5.f,5.f})for(int k=0;k<6;++k)
                    box(p+glm::vec3{x,1.f+k*.9f,1.02f},{.8f,.16f,.035f},accents[c]*.22f,accents[c]*.9f);
            }
        }
        if(ambientDetail()>0) {   // an unreachable ruined colonnade frames the void
            for(int k=0;k<20;++k) {
                float a=k*6.2831853f/20.f;
                auto p=RELIQUARY_Q+glm::vec3{std::cos(a)*76.f,-13.f,std::sin(a)*76.f};
                float h=9.f+(k%4)*4.f;
                box(p+glm::vec3{0,h*.5f,0},{2.2f,h,2.2f},{.2f,.16f,.2f});
                box(p+glm::vec3{0,h+.15f,0},{3.f,.3f,3.f},{.3f,.25f,.29f});
            }
        }
        if(level.arenas[director.arena].shift==ArenaShift::DRIFT && director.phase==WaveDirector::Phase::CLEARED) {
            auto p=RELIQUARY_Q+glm::vec3{0,.04f,-56.f};
            for(float side:{-1.f,1.f})
                push(out,T(p+glm::vec3{side*.7f,0,.7f})*RY(side*.785398f)*S({.12f,.04f,2.f}),amber*.3f,amber);
        }
    }
    const glm::vec3 violet{0.75f, 0.35f, 1.1f}, soul{0.75f, 0.9f, 1.3f};
    for (const auto& w : relic.wires) {
        glm::vec3 d = w.b - w.a; float len = glm::length(d);
        float yaw = std::atan2(d.x, d.z), hum = 0.7f + 0.3f * std::sin(t * 30.f + w.a.x);
        push(out, T((w.a + w.b) * 0.5f) * RY(yaw) * S({0.04f, 0.04f, len}), violet * 0.3f, violet * (1.4f * hum));
        for (glm::vec3 n : {w.a, w.b}) push(out, T(n) * S(glm::vec3{0.35f}), violet * 0.3f, violet * 2.f);
    }
    for (const auto& s : relic.souls) {
        float pulse = 0.8f + 0.2f * std::sin(t * 12.f);
        push(out, T(s.pos) * RY(t * 3.f) * S(glm::vec3{0.6f * pulse}), soul * 0.5f, soul * 2.5f);
    }
}
