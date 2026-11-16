#pragma once
// =============================================================================
// Gameplay_ShepherdBreak.h — THE SHEPHERD, phase 4: THE BREAK (design section
// 6). Out of the Lamp Well onto its surface: it tears the ring apart round
// itself and loses its grip. Low gravity again; three ledges of debris, each
// higher and further toward its crown (LevelShepherd.h, arenas 7-9).
//
//   ledges      break the mooring with its strike; stabilise a relay; board
//               the last Gather under fire. Guards provide finite pressure.
//   the Gather  then it pulls everything in: the sweep reversed, a countdown
//               reading GATHER, the wall coming from behind you. Spike one of
//               the ledge's two RIDE chunks with HOLDFAST (they're marked) and
//               hold on: it throws you and the chunk up to the next ledge. Not
//               held: you're thrown alone, into its hull: hurt, and back.
//   the crown   the third ride lands you on its crown. It is falling.
// =============================================================================

namespace shepbreak {
constexpr float GATHER_WARN = 6.f, GATHER_LEAD = 1.5f, GATHER_AGAIN = 10.f, RIDE_LEN = 3.4f, RIDE_ARC = 45.f, HULL_DAMAGE = 25.f;
constexpr float RELAY_HOLD=2.5f;
}

inline int GameplayState::shepLedge() const {
    const int at = level.arenaAt(player.position);
    return at >= 7 && at <= 9 ? at - 7 : -1;
}

inline void GameplayState::shepBreakReset() {
    shepRideT = -1.f; shepGatherT = -1.f; shepGatherPassed = false; shepLedgeFought = false; shepThrownT = -1.f; shepCrownT = -1.f;
    shepLedgeStrike=5.f;shepLedgeTell=-1.f;
    shepRides = std::clamp(director.arena - 7, 0, 3);
    shepMooringBroken=director.arena>7;shepMooringCalled=false;
    shepRelayDone=director.arena>8;shepRelayCharge=0.f;shepRelayClock=0.f;
    shepLedgeLesson=-1;
}

inline void GameplayState::shepBreak(float dt) {
    using namespace shepbreak;
    const glm::vec3 p = player.position;
    // The ride: you and the chunk, thrown up to the next ledge (or its crown)
    if (shepRideT >= 0.f) {
        shepRideT += dt;
        const float u = glm::clamp(shepRideT / RIDE_LEN, 0.f, 1.f), e = u * u * (3.f - 2.f * u);
        const glm::vec3 mid = (shepRideFrom + shepRideTo) * 0.5f + glm::vec3{0.f, RIDE_ARC, 0.f};
        const glm::vec3 at = (1.f - e) * (1.f - e) * shepRideFrom + 2.f * (1.f - e) * e * mid + e * e * shepRideTo;
        player.position = at; player.velocity = glm::vec3{0.f}; invincFrames = std::max(invincFrames, 0.3f);
        player.gravity = 0.f;
        if (std::fmod(gameClock, 0.03f) < dt) fx.spawnSpray(at - glm::vec3{0.f, 1.5f, 0.f}, glm::vec3{0.f, -0.3f, 1.f}, SWEEP_ICE * 0.8f, 3, 10.f, 0.8f, 0.5f);
        shake(0.08f, 0.012f);
        if (u >= 1.f) {
            shepRideT = -1.f; ++shepRides;
            player.position = shepRideTo; player.velocity = {0.f, -2.f, 0.f};
            fx.spawnBurst(shepRideTo + glm::vec3{0.f, 0.4f, 0.f}, SWEEP_ICE, 40, 10.f, 0.9f, 3.f);
            shake(0.7f, 0.04f);
            audio.play("land_heavy", 128, SoundGroup::PLAYER, true);
            if (shepRides >= shep::LEDGES) {
                shepCrownT = 0.f;
                pushBanner("ITS CROWN", "IT IS FALLING", SWEEP_ICE * 0.8f, 4.f);
            } else {
                char sub[32]; snprintf(sub, sizeof(sub), "LEDGE %d/3", shepRides + 1);
                pushBanner("THE GATHER THREW YOU", sub, SWEEP_ICE * 0.7f, 2.5f);
            }
        }
        return;
    }
    // On its crown: the fall begins (THE FALL is next: the run ends here for now)
    if (shepCrownT >= 0.f) {
        player.velocity.x = player.velocity.z = 0.f;
        const float before = shepCrownT;
        shepCrownT += dt;
        shake(0.1f, 0.01f + 0.01f * std::min(1.f, shepCrownT / 4.f));
        if (before < 2.f && shepCrownT >= 2.f) pushBanner("WITHOUT ME,", "THEY DRIFT.", glm::vec3{0.9f, 0.9f, 0.82f}, 4.f);
        if (before < 5.5f && shepCrownT >= 5.5f) {   // THE FALL (Gameplay_ShepherdFall.h)
            shepCrownT = -1.f;
            storySeenSection = storyCheckpoint = 10;
            enterArena(10); saveStoryProgress();
        }
        return;
    }
    const int L = shepLedge();
    if (L < 0 || playerDead) return;
    player.gravity = -24.f * ring::GRAVITY_SCALE;   // (it has lost its grip: low gravity again)
    if (!shepBreakSeen) {
        shepBreakSeen = true;
        pushBanner("THE BREAK", "IT HAS LOST ITS GRIP. EVERYTHING DRIFTS", SWEEP_ICE * 0.7f, 4.f);
        ui.toast("LOW GRAVITY", "BREAK ITS HOLD ON EACH LEDGE. RIDE THE GATHER UP", SWEEP_ICE * 0.6f, 5.f);
    }
    if(shepLedgeLesson!=L){
        shepLedgeLesson=L;
        if(L==0)ui.toast("THE MOORING","BAIT ITS RED STRIKE ONTO THE CORE, OR HOLDFAST THE CORE TO CALL THE STRIKE. THEN MOVE CLEAR",HOLDFAST_STEEL,7.f);
        if(L==1)ui.toast("THE RELAY","HOLDFAST THE MOVING RELAY FOR 2.5 SECONDS. KEEP FIGHTING WHILE YOU HOLD IT STEADY",HOLDFAST_STEEL,6.f);
        if(L==2)ui.toast("THE LAST RIDE","REACH A RIDE CHUNK. THE GUARDS DO NOT NEED TO DIE",HOLDFAST_STEEL,5.f);
    }
    // Thrown, unheld: into its hull, then back to the ledge
    if (shepThrownT >= 0.f && (shepThrownT += dt) > 1.6f) {
        shepThrownT = -1.f;
        if (!playerDead) {
            damagePlayer(HULL_DAMAGE, p + glm::vec3{0.f, 0.f, -6.f}, 0.5f, 0.07f, 0.f, {EnemyType::COUNT, DamageKind::UNKNOWN});
            resetPlayerPosition(shep::ledgeStart(L));
            ui.toast("INTO ITS HULL", "HOLD ON NEXT TIME: SPIKE A RIDE CHUNK (" + binds().label(Act::HOLDFAST) + ") BEFORE THE GATHER REACHES YOU", SWEEP_RED * 0.8f, 4.f);
        }
    }
    if (g_devOverlay == "breakride") {   // dev: clear each ledge at once and spike a ride chunk when the Gather comes
        for (auto& e : enemies) if (e.targetable() && e.age > 1.f && e.takeDamage(1e6f)) onEnemyKilled(e, StyleSource::ENVIRONMENT);
        if (sweep.active() && !holdfast.held()) { const int w = level.shepRideWalls[2 * L]; holdfast.latch(shep::rideChunk(L, 0), player.camera.position, w, shelterOfWall(w)); }
    }
    // Pinning the mooring calls a strike onto it; baiting a normal strike is
    // equally valid. Both routes use the same delayed, dodgeable impact.
    if(L==0&&!shepMooringBroken&&!shepMooringCalled&&director.fighting()&&level.shepLedgeMooring>=0
        &&holdfast.held()&&holdfast.body==level.movers[level.shepLedgeMooring].wall){
        holdfast.cut();shepMooringCalled=true;shepLedgeTell=0.f;
        shepLedgeAim=shep::ledgeMooring();shepLedgeAim.y=shep::ledge(0).y;
        ui.feed("MOORING HELD. ITS STRIKE IS COMING",HOLDFAST_STEEL);
        shepherdSound("shep_fist_tell",shepLedgeAim);
    }
    if(L==1&&!shepRelayDone&&director.fighting()&&level.shepLedgeRelay>=0){
        const bool pinned=holdfast.held()&&holdfast.body==level.movers[level.shepLedgeRelay].wall;
        shepRelayCharge=pinned?std::min(RELAY_HOLD,shepRelayCharge+dt):0.f;
        if(shepRelayCharge>=RELAY_HOLD){
            shepRelayDone=true;holdfast.cut();director.completeGoal();
            shepherdSound("shep_pin",shep::ledgeRelay());
            fx.spawnShockwave(shep::ledgeRelay(),12.f,POST_LIT);
            ui.feed("RELAY SECURED. THE GATHER CAN TAKE YOU",HOLDFAST_STEEL);
        }
    }
    // The Shepherd hammers the ascent itself. Bait its blow into the flock.
    if(director.fighting()) {
        if(shepLedgeTell>=0.f){shepLedgeTell+=dt;if(shepLedgeTell>=1.8f){
            const auto q=shepLedgeAim;shepLedgeTell=-1.f;wakeRings.push_back({q,.6f,false});fx.spawnShockwave(q,12.f,POST_LIT);fx.spawnBurst(q,POST_LIT,40,14.f,.8f,3.f);shake(.6f,.04f);shepherdSound("shep_contact",q);
            if(L==0&&!shepMooringBroken&&glm::length(glm::vec2{q.x-shep::ledgeMooring().x,q.z-shep::ledgeMooring().z})<8.f){
                shepMooringBroken=true;director.completeGoal();
                if(holdfast.held()&&holdfast.body==level.movers[level.shepLedgeMooring].wall)holdfast.cut();
                shepherdSound("shep_break",shep::ledgeMooring());
                ui.feed("ITS OWN STRIKE BROKE THE MOORING",HOLDFAST_STEEL);
            }
            if(glm::length(glm::vec2{p.x-q.x,p.z-q.z})<6.f&&p.y<q.y+9.f&&invincFrames<=0.f)damagePlayer(28.f,q,.4f,.05f,0.f,{EnemyType::COUNT,DamageKind::SHOCKWAVE});
            for(auto& e:enemies)if(e.targetable()&&glm::length(e.position-q)<8.f&&e.takeDamage(200.f))onEnemyKilled(e,StyleSource::ENVIRONMENT);
        }}else if((shepLedgeStrike-=dt)<=0.f){shepLedgeStrike=6.f;shepLedgeTell=0.f;shepLedgeAim=p;shepLedgeAim.y=shep::ledge(L).y;shepherdSound("shep_fist_tell",shepLedgeAim);}
    }else shepLedgeTell=-1.f;
    // The last ledge is a boarding run: the guards are pressure, not a kill gate.
    if (director.fighting()) shepLedgeFought = true;   // (the director moves its arena on when a wave clears: track the ledge's own fight)
    const bool cleared = shepLedgeFought && !director.fighting();
    const bool boarding=cleared||(L==2&&director.fighting());
    if (boarding && shepRides == L && !sweep.active() && shepThrownT < 0.f) {
        if (shepGatherT < 0.f) {
            shepGatherT = 2.5f;
            ui.toast("THE GATHER", "IT PULLS EVERYTHING IN. SPIKE A RIDE CHUNK (" + binds().label(Act::HOLDFAST) + ") AND HOLD ON", HOLDFAST_STEEL, 5.f);
        }
        if ((shepGatherT -= dt) <= 0.f) {
            shepGatherT = GATHER_AGAIN;
            SweepSpec s = sweeptier::gather(glm::vec3{0.f, 0.f, -1.f}, GATHER_WARN);
            s.flashLead = GATHER_LEAD; s.clocked = true; s.range = 40.f;
            sweep.start(s); ++sweepCount; shepGatherPassed = false;
            shepherdSound("shep_gather",shep::ledge(L));
        }
    }
    if (sweep.active() && sweep.spec.kind == SweepSpec::Kind::GATHER && sweep.passed > 0 && !shepGatherPassed) {
        shepGatherPassed = true;
        int held = -1;
        if (holdfast.held())
            for (int k = 0; k < 2; ++k) if (holdfast.body == level.shepRideWalls[2 * L + k]) held = k;
        if (held >= 0) {   // ride it
            holdfast.cut(); grapple.release();
            if(L==2&&director.fighting())director.completeGoal();
            shepRideT = 0.f; shepRideChunk = held; shepLedgeFought = false; shepGatherT = -1.f;
            shepRideFrom = player.position;
            shepRideTo = shepRides + 1 >= shep::LEDGES ? shep::crown() : shep::ledgeStart(shepRides + 1);
            pushBanner("HOLD ON", "", HOLDFAST_STEEL, 1.5f);
            audio.play("dash", 128, SoundGroup::PLAYER, true);
            audio.duck(10.f, 0.6f);
            for (auto& e : enemies) if (e.targetable() && e.takeDamage(1e6f)) onEnemyKilled(e, StyleSource::ENVIRONMENT);   // (it takes everything loose)
        } else if (shepRides == L && boarding) shepThrownT = 0.f;
    }
}

// Put the player down somewhere without a full retry
inline void GameplayState::resetPlayerPosition(glm::vec3 at) {
    player.position = at; player.velocity = glm::vec3{0.f};
    player.camera.position = at + glm::vec3{0.f, player.eyeHeight, 0.f}; prevCamPos = player.camera.position;
}

// The torn ring all round you, the chunk you ride
inline void GameplayState::gatherBreak(std::vector<BoxInstance>& out) {
    using namespace rig;
    if(shepLedgeTell>=0.f&&shepLedge()>=0){
        for(int j=0;j<40;++j){float a=6.2831853f*j/40.f;push(out,T(shepLedgeAim+glm::vec3{std::cos(a)*6.f,.1f,std::sin(a)*6.f})*RY(-a)*S(glm::vec3{.25f,.1f,1.f}),glm::vec3{.1f},SWEEP_RED*2.f);}
        push(out,T(shepLedgeAim+glm::vec3{0,35.f*(1.f-shepLedgeTell/1.8f)+2.f,0})*S(glm::vec3{9,4,12}),glm::vec3{.2f,.4f,.65f},POST_LIT*.6f);
    }

    if (!shepherd()) return;
    const int L = shepLedge();
    if (L < 0 && shepRideT < 0.f && shepCrownT < 0.f) return;
    const glm::vec3 p = player.position;
    if(L==0&&level.shepLedgeMooring>=0){
        const auto c=shep::ledgeMooring();
        if(!shepMooringBroken){
            push(out,T(c)*S({5.6f,4.f,5.6f}),glm::vec3{.1f});
            push(out,T(c+glm::vec3{0,1.f,2.85f})*S({2.2f,1.3f,.15f}),glm::vec3{.1f},HOLDFAST_STEEL*(shepMooringCalled?1.8f:.65f));
            for(float x:{-2.6f,2.6f})push(out,T(c+glm::vec3{x,0,0})*S({.4f,5.f,6.f}),glm::vec3{.35f,.27f,.14f});
        }else for(int j=0;j<5;++j)push(out,T(c+glm::vec3{float(j-2)*2.7f,-1.5f,float(j%2)*3.f})*RZ(j*.7f)*S({1.8f,.7f,3.f}),glm::vec3{.12f});
    }
    if(L==1&&level.shepLedgeRelay>=0){
        const auto& box=level.walls[level.movers[level.shepLedgeRelay].wall].box;
        const auto c=(box.min+box.max)*.5f;
        push(out,T(c)*S({4.8f,4.8f,4.8f}),glm::vec3{.12f});
        push(out,T(c+glm::vec3{0,0,2.5f})*S({1.7f,1.7f,.15f}),glm::vec3{.1f},HOLDFAST_STEEL*.8f);
        for(int j=0;j<32;++j){float a=j*6.2831853f/32;
            bool charged=shepRelayDone||j<32.f*shepRelayCharge/shepbreak::RELAY_HOLD;
            push(out,T(c+glm::vec3{3.4f*std::cos(a),3.4f*std::sin(a),0})*RZ(a)*S({.3f,.72f,.4f}),glm::vec3{.35f,.27f,.14f},charged?HOLDFAST_STEEL:glm::vec3{0});
        }
    }
    for (int k = 0; k < 40; ++k) {   // debris drifting past, close (the far ones are the level's)
        const float fk = (float)k;
        const glm::vec3 base{(ring::hash(fk, 71.f) - 0.5f) * 150.f, (ring::hash(fk, 72.f) - 0.3f) * 70.f, (ring::hash(fk, 73.f) - 0.5f) * 150.f};
        const glm::vec3 drift = glm::vec3{std::sin(gameClock * 0.05f + fk), 0.3f * std::sin(gameClock * 0.11f + fk * 2.f), std::cos(gameClock * 0.04f + fk)} * 12.f;
        glm::vec3 at = p + base + drift;
        if (glm::length(glm::vec2{at.x - p.x, at.z - p.z}) < 25.f) continue;
        BoxInstance bi; bi.mesh = (float)meshlib::lumpBall(); bi.color = glm::vec3{0.6f, 0.58f, 0.53f} * (0.7f + 0.3f * ring::hash(fk, 74.f));
        bi.model = T(at) * RY(gameClock * 0.2f + fk) * RX(gameClock * 0.13f + fk) * S(glm::vec3{1.f + 4.f * ring::hash(fk, 75.f)});
        out.push_back(bi);
    }
    if (shepRideT >= 0.f) {   // the chunk under you
        BoxInstance bi; bi.mesh = (float)meshlib::lumpBall(); bi.color = glm::vec3{0.66f, 0.64f, 0.6f};
        bi.model = T(p - glm::vec3{0.f, 1.8f, 0.f}) * RY(shepRideT) * S(glm::vec3{6.f, 3.f, 5.f}); out.push_back(bi);
    }
    if (L >= 0 && (sweep.active() || shepGatherT >= 0.f))   // its ride chunks' posts, lit
        for (int k = 0; k < 2; ++k) {
            const glm::vec3 r = shep::rideChunk(L, k);
            push(out, T(r + glm::vec3{0.f, 2.6f, 0.f}) * S(glm::vec3{0.6f}), glm::vec3{0.1f}, HOLDFAST_STEEL * (1.5f + 0.8f * std::sin(gameClock * 8.f)));
        }
}

inline void GameplayState::breakMarkers(const glm::mat4& view, const glm::mat4& proj) {
    const int L = shepLedge();
    if (L < 0 || shepRideT >= 0.f || shepCrownT >= 0.f) return;
    const bool cleared = shepLedgeFought && !director.fighting();
    char buf[64]; float sx, sy;
    if(!cleared&&L<2){
        glm::vec3 at=shep::ledgeMooring()+glm::vec3{0,4.f,0};
        if(L==1&&level.shepLedgeRelay>=0){const auto& box=level.walls[level.movers[level.shepLedgeRelay].wall].box;at=(box.min+box.max)*.5f+glm::vec3{0,5.f,0};}
        const bool on=projectToScreen(at,view,proj,sx,sy);
        if(L==0)snprintf(buf,sizeof(buf),"%s",shepMooringCalled?"STRIKE CALLED. MOVE CLEAR":"MOORING: BAIT ITS STRIKE OR HOLDFAST THE CORE");
        else snprintf(buf,sizeof(buf),"RELAY: HOLDFAST IT  %d%%",int(100.f*shepRelayCharge/shepbreak::RELAY_HOLD));
        ui.renderMarker(sx,sy,on,HOLDFAST_STEEL,buf);return;
    }
    for (int k = 0; k < 2; ++k) {
        const glm::vec3 at = shep::rideChunk(L, k) + glm::vec3{0.f, 3.f, 0.f};
        const bool on = projectToScreen(at, view, proj, sx, sy);
        const bool held = holdfast.held() && holdfast.body == level.shepRideWalls[2 * L + k];
        snprintf(buf, sizeof(buf), held ? "HELD: HOLD ON" : "RIDE  SPIKE IT  %dM", (int)glm::length(at - player.position));
        ui.renderMarker(sx, sy, on, held ? glm::vec3{0.4f, 1.f, 0.6f} : HOLDFAST_STEEL, buf);
    }
}

inline const char* GameplayState::breakObjective() {
    static char line[110];
    const int L = std::max(0, shepLedge());
    const char* step;
    const bool cleared = shepLedgeFought && !director.fighting();
    if (shepRideT >= 0.f) step = "RIDING THE GATHER";
    else if (shepCrownT >= 0.f) step = "ITS CROWN. IT IS FALLING";
    else if (!cleared&&L==0) step=shepMooringCalled?"MOVE CLEAR. ITS STRIKE WILL BREAK THE MOORING":"BREAK THE MOORING: BAIT ITS STRIKE OR HOLDFAST THE CORE";
    else if (!cleared&&L==1) step = "HOLDFAST THE MOVING RELAY FOR 2.5 SECONDS";
    else if (holdfast.held()) step = sweep.active() ? "HOLD ON: LET IT TAKE YOU" : "HOLD ON: THE GATHER IS COMING";
    else step = sweep.active() ? "THE GATHER: SPIKE A RIDE CHUNK NOW" : L==2?"REACH A RIDE CHUNK. LEAVE THE GUARDS":"SPIKE A RIDE CHUNK AND WAIT FOR THE GATHER";
    snprintf(line, sizeof(line), "LEDGE %d/3  /  %s", std::min(L + 1, 3), step);
    return line;
}
