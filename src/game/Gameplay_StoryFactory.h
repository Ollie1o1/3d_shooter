#pragma once
// Upper-factory sequences. Real platform walls are driven here; Player carries
// their delta in the normal physics path. No camera teleport is used in play.
inline void GameplayState::resetStoryFactory() {
    auto& S=level.story;
    if(storyGlassBoxes.empty())for(int w:S.podGlass)storyGlassBoxes.push_back(level.walls[w].box);
    if(storyTileBoxes.empty())for(int w:S.sortingTiles)storyTileBoxes.push_back(level.walls[w].box);
    for(size_t i=0;i<S.podGlass.size();++i) {auto& w=level.walls[S.podGlass[i]];w.box=storyGlassBoxes[i];w.hidden=true;}
    for(size_t i=0;i<S.sortingTiles.size();++i) {auto& w=level.walls[S.sortingTiles[i]];w.box=storyTileBoxes[i];w.hidden=true;}
    storyPodsBroken=director.arena>5; storyBossDown=false;storyCollapseT=-1;
    storyScanCooldown=0; storyTerminal=-1;storyLiftStop=0;director.hold=false;
    if(S.elevator>=0)level.movers[S.elevator].drive=director.arena>6?1.f:0.f;
    level.updateMovers(moverClock);spatialGrid.build(level.walls);
    storyCheckpoint=director.arena;
}

inline void GameplayState::updateStoryFactory(float dt) {
    using glm::vec3;
    auto& S=level.story; const vec3 p=player.position;
    const int at=level.arenaAt(p);
    if(at>=0 && at>storyCheckpoint && !director.fighting()) {
        storyCheckpoint=at;
        ui.feed("CHECKPOINT",{.35f,.85f,1.f});
    }
    // A quiet room has no encounter to advance the director. Walking into the
    // next room explicitly approaches it; active fights still own their gates.
    if(at>director.arena && !director.fighting() && director.phase!=WaveDirector::Phase::INTRO) director.approach(at);
    storyTerminal=-1;
    for(int i=0;i<(int)S.terminals.size();++i) {
        vec3 d=S.terminals[i].pos-p;
        if(std::fabs(d.y)<3.f && glm::length(glm::vec2{d.x,d.z})<4.f) storyTerminal=i;
    }
    if(!storyGrapple && at==7 && glm::length(p-S.grappleAt)<3.f) {
        storyGrapple=true;audio.play("pickup",128,SoundGroup::UI,true);
        pushBanner("THE GRAPPLE","Q - ATTACH / RELEASE   SPACE - JUMP OFF",{.35f,.85f,1.f},4.f);
    }
    // The pod fronts become debris at the ambush trigger, leaving no invisible collision.
    if(director.arena==5 && director.fighting() && !storyPodsBroken) {
        storyPodsBroken=true;
        for(int id:S.podGlass) {
            auto& w=level.walls[id];vec3 c=(w.box.min+w.box.max)*.5f;
            w.box.min.y-=500;w.box.max.y-=500;w.hidden=true;
            fx.spawnBurst(c,{.4f,.9f,1.f},14,6.f,.8f,12.f);
        }
        spatialGrid.build(level.walls);
        audio.play("explosion",105,SoundGroup::WORLD,true);shake(.65f,.08f);
    }
    // Board, rise 13 m, hold at each stop until its defenders are dead.
    if(S.elevator>=0) {
        auto& lift=level.movers[S.elevator];
        if(director.arena==6) {
            auto b=level.walls[lift.wall].box;
            auto& arena=level.arenas[6];arena.hasRespawn=true;
            arena.respawn={(b.min.x+b.max.x)*.5f,b.max.y+.05f,(b.min.z+b.max.z)*.5f};
            arena.voidY=b.max.y-7.f;
            bool aboard=player.groundWall==lift.wall;
            if(director.phase==WaveDirector::Phase::APPROACH && aboard) director.startArena(6);
            if(director.phase==WaveDirector::Phase::INTRO || director.fighting()) {
                float target=(director.wave+1)/3.f;
                // A player who stepped off can get back aboard before departure.
                if(aboard || lift.drive>director.wave/3.f+.001f)
                    lift.drive=std::min(target,lift.drive+dt/18.f);
                director.hold=lift.drive<target-.0001f;
                int stop=(int)std::floor(lift.drive*3.f+.001f);
                if(stop>storyLiftStop) {storyLiftStop=stop;audio.play("anvil",100,SoundGroup::WORLD,true);}
            } else director.hold=false;
        } else director.hold=false;
    }
    // Sliding goes under the oscillating inspection strip; otherwise dash through
    // its narrow plane while it is high. The red beam has no hidden hit volume.
    storyScanCooldown=std::max(0.f,storyScanCooldown-dt);
    if(at==7 && storyScanCooldown<=0.f && !playerDead) {
        for(const auto& sc:S.scanners) {
            float y=sc.a.y+1.5f+1.25f*std::sin(gameClock*1.7f+sc.phase);
            if(std::fabs(p.x-sc.a.x)<.45f && p.z>1 && p.z<15 && y>p.y && y<p.y+player.height) {
                storyScanCooldown=5.f;
                audio.play("telegraph",128,SoundGroup::WORLD,true);
                ui.toast("MARKED","THE COLLECTORS ARE COMING",{1.f,.3f,.15f},2.f);
                if(director.phase==WaveDirector::Phase::APPROACH) director.startArena(7);
                else if(director.fighting()) damagePlayer(8.f,sc.a,.2f,.02f);
                break;
            }
        }
    }
    if(storyCollapseT>=0 && !storyBossDown) {
        float before=storyCollapseT;storyCollapseT+=dt;
        if(before<2.2f && storyCollapseT>=2.2f) {
            for(int id:S.sortingTiles) {
                auto& w=level.walls[id];vec3 c=(w.box.min+w.box.max)*.5f;
                fx.spawnBurst(c,{1.f,.45f,.08f},30,8.f,1.2f,16.f);
                w.box.min.y-=8;w.box.max.y-=8;w.hidden=true;
            }
            spatialGrid.build(level.walls);
            audio.play("explosion",128,SoundGroup::WORLD,true);shake(1.f,.12f);
        }
    }
    if(at==8 && p.y<level.arenas[8].playerStart.y-5.f && !storyBossDown) {
        resetToCheckpoint(8);if(!g_godMode)styleSystem.takeDamage(18.f);
    }
    // The exit vista's sky warms as the gate opens; it is visible before the end card.
    if(storyBossDown) {
        auto& th=level.arenas.back().theme;
        th.zenith={.2f,.035f,.055f};th.horizon={.75f,.13f,.06f};th.ground={.16f,.09f,.1f};
        th.mountain={.13f,.05f,.07f};th.sunColor={2.f,.5f,.2f};th.sunSize=.075f;
        th.skyAmb={.38f,.24f,.22f};th.fogColor={.3f,.09f,.065f};th.fogDensity=.004f;
    }
}

inline void GameplayState::storySorterEvents(Enemy& e,const EnemyEvents& ev) {
    using glm::vec3;
    if(ev.telegraphStarted)audio.playAt("telegraph",e.sortAim,120,SoundGroup::ENEMY,true);
    if(ev.sortPhaseStarted) {
        const char* names[]={"","INSPECTION","RECALIBRATION","INCINERATION"};
        pushBanner(names[ev.sortPhaseStarted],ev.sortPhaseStarted==1?"UNIT ?  SORTER  PURPOSE: JUDGE":ev.sortPhaseStarted==2?"REJECT. RETURN TO LINE.":"FLOOR RELEASE - GET TO THE RAILS",{1.f,.5f,.2f},3.5f);
        audio.play("anvil_final",128,SoundGroup::WORLD,true);
        if(ev.sortPhaseStarted==3) storyCollapseT=0;
        const auto& a=level.arenas[8];
        for(auto& cv:level.conveyors) {
            const auto& b=level.walls[cv.wall].box;
            if(b.min.y>a.playerStart.y-1 && b.min.x>=64 && b.max.x<=100)
                cv.speed=ev.sortPhaseStarted==1?1.8f:ev.sortPhaseStarted==2?-3.f:4.2f;
        }
    }
    if(ev.sortMarked) {audio.play("dry",110,SoundGroup::UI,true);ui.feed("MARKED - PARRY OR BREAK SIGHT",{1.f,.3f,.1f});}
    if(ev.sortStamp) {
        fx.spawnShockwave(ev.sortAt,Enemy::SORT_STAMP_R,{1.f,.2f,.05f});
        fx.spawnBurst(ev.sortAt,{.8f,.5f,.2f},30,8,.7f,16);
        audio.playAt("slam",ev.sortAt,128,SoundGroup::ENEMY,true);shake(.7f,.08f);
    }
    if(ev.sortGrab && dashMomentumTimer<=0) {
        vec3 to={82.f,player.position.y,16.f};to-=player.position;to.y=0;
        if(glm::length(to)>.1f)player.velocity=glm::normalize(to)*18.f+vec3{0,7,0};
        audio.play("hook_yank",128,SoundGroup::WORLD,true);
    }
    if(ev.shots)audio.playAt("land_heavy",e.position,110,SoundGroup::ENEMY,true);
}

inline void GameplayState::gatherStoryFactory(std::vector<BoxInstance>& out) {
    using namespace rig;using namespace enemyfile;using glm::vec3;
    const auto& S=level.story;const auto* unit=story::bodyModel();
    if(unit) {
        auto drawUnit=[&](vec3 p,float yaw,bool awake) {
            if(glm::length(p-player.camera.position)>80)return;
            FilePose P;P.chan[C_EYE]=awake?vec3{.2f,.8f,1}:vec3{0};P.chan[C_CORE]=P.chan[C_EYE]*.4f;
            P.motion[E_HEAD]=RY(std::sin(gameClock*.6f+p.x)*.18f);
            P.motion[E_ARML]=RZ(.12f);P.motion[E_ARMR]=RZ(-.12f);
            Rig r{out};drawFile(r,T(p)*RY(yaw),*unit,P,{.15f,.8f,1});
        };
        for(auto p:S.assemblyUnits)drawUnit(p,3.14159f,true);
        if(!storyPodsBroken)for(auto p:S.pods)drawUnit(p,0,false);
    }
    Rig r{out};
    for(int w:S.podGlass) {
        auto b=level.walls[w].box;if(b.max.y< -100)continue;
        vec3 c=(b.min+b.max)*.5f;
        // Frosted edges and fine glass ribs leave the sleeping silhouettes visible.
        for(float sx:{b.min.x,b.max.x})r.box(T({sx,c.y,c.z}),{0,0,0},{.06f,b.max.y-b.min.y,.1f},{.25f,.65f,.7f},{.02f,.09f,.1f});
        for(int k=0;k<6;++k)r.box(T({c.x,b.min.y+k*.7f,c.z}),{0,0,0},{b.max.x-b.min.x,.025f,.04f},{.18f,.45f,.48f});
    }
    for(int w:S.sortingTiles) {auto b=level.walls[w].box;if(b.max.y<level.arenas[8].playerStart.y-1)continue;
        r.box(T((b.min+b.max)*.5f),{0,0,0},b.max-b.min,{.14f,.16f,.18f});
    }
    for(const auto& sc:S.scanners) {
        float y=sc.a.y+1.5f+1.25f*std::sin(gameClock*1.7f+sc.phase);
        r.box(T({sc.a.x,y,8}),{0,0,0},{.08f,.07f,14},{1,.15f,.05f},{2,.2f,.05f});
    }
    if(!storyGrapple) {
        auto p=S.grappleAt+vec3{0,1.4f+.12f*std::sin(gameClock*3),0};
        r.box(T(p)*RY(gameClock),{0,0,0},{.55f,.2f,.25f},{.7f,.75f,.8f},{.1f,.7f,1});
        for(float s:{-1.f,1.f})r.box(T(p)*RY(gameClock),{s*.27f,0,.16f},{.1f,.18f,.3f},{.65f,.7f,.72f});
    }
    auto crane=[&](vec3 p) {
        float top=level.arenas[8].playerStart.y+15;
        r.box(T({82,top,p.z}),{0,0,0},{40,.65f,1.8f},{.48f,.36f,.1f});
        r.box(T({p.x,top-.7f,p.z}),{0,0,0},{6,1,2.4f},{.3f,.33f,.35f});
        for(float s:{-1.f,1.f}) r.box(T({p.x+s*2.3f,(p.y+6.5f+top-1.f)*.5f,p.z}),{0,0,0},{.13f,top-1.f-p.y-6.5f,.13f},{.6f,.63f,.65f});
    };
    // Through the inspection doorway, the Sorter is already working. The real
    // targetable enemy replaces this preview when the fight starts.
    bool live=false;for(const auto& e:enemies)if(e.alive&&e.type==EnemyType::SORTER)live=true;
    if(!live&&!storyBossDown && glm::length(player.position-level.arenas[8].bossSpawn)<95) {
        Enemy e{EnemyType::SORTER,level.arenas[8].bossSpawn};e.spawnTimer=0;e.yaw=1.57f;
        e.yaw=std::atan2(player.position.x-e.position.x,player.position.z-e.position.z);e.sortOpen=.5f*(1+std::sin(gameClock));buildEnemy(e,gameClock,out);crane(e.position);
    }
    for(const auto& e:enemies)if(e.alive&&e.type==EnemyType::SORTER) {
        crane(e.position);
        vec3 col=e.attack==AttackKind::SORT_GRAB?vec3{2}:e.attack==AttackKind::SORT_STAMP?vec3{2,.12f,.05f}:abGoldTell({2,1.2f,.15f});
        if(e.telegraphTimer>0 && (e.attack==AttackKind::SORT_GRAB||e.attack==AttackKind::SORT_STAMP)) {
            float R=e.attack==AttackKind::SORT_GRAB?Enemy::SORT_GRAB_R:Enemy::SORT_STAMP_R;
            for(int k=0;k<48;++k) {float a=k*6.283185f/48;
                r.box(T(e.sortAim+vec3{std::cos(a)*R,.07f,std::sin(a)*R})*RY(-a),{0,0,0},{.3f,.08f,.13f},col,col);
            }
            // The claw/press reaches the marked point, so its white parry tell is at the player's height.
            float h=e.attack==AttackKind::SORT_STAMP?5.f*(1-e.telegraphProgress())+.5f:1.4f;
            r.box(T(e.sortAim+vec3{0,h,0}),{0,0,0},e.attack==AttackKind::SORT_STAMP?vec3{2.6f,.45f,2}:vec3{1.2f,.3f,1},{.24f,.27f,.29f},col*.3f);
        }
        if(e.sortScan>0) {
            vec3 from=e.position+vec3{0,3,0};float a=e.sortScanYaw+(1.6f-e.sortScan)*.72f;
            vec3 to=from+glm::normalize(vec3{std::sin(a),e.sortScanSlope,std::cos(a)})*55.f;
            vec3 d=to-from;float len=glm::length(d);vec3 dir=d/len;
            for(const auto& w:level.walls){float hit=rayBoxHit(from,dir,w.box);if(hit>.05f)len=std::min(len,hit);}
            d=dir*len;
            vec3 side=glm::normalize(glm::cross(glm::normalize(d),vec3{0,1,0}));
            // A row of thin cubes follows the same ray as the collision test.
            for(float t=0;t<len;t+=.16f)r.box(T(from+d*(t/len)),{0,0,0},{.12f,.12f,.12f},{1,.3f,.05f},{2,.4f,.08f});
            (void)side;
        }
    }
    if(storyCollapseT>=0 && storyCollapseT<2.2f)for(int w:S.sortingTiles) {
        auto b=level.walls[w].box;vec3 c=(b.min+b.max)*.5f;c.y=b.max.y+.04f;
        r.box(T(c),{0,0,0},{b.max.x-b.min.x,.04f,b.max.z-b.min.z},{.8f,.1f,.03f},vec3{2.f,.15f,.05f}*float((int)(gameClock*8)%2));
    }
}

inline void GameplayState::gatherStoryProjectile(std::vector<BoxInstance>& out,const Projectile& shot,glm::vec3 at,float spin) {
    using namespace rig;using namespace enemyfile;
    glm::mat4 root=T(at)*RY(spin*.3f)*RX(spin)*S(glm::vec3{shot.parried?1.25f:1.f})*T({0,-.9f,0});
    if(const auto* m=story::bodyModel()) {
        FilePose pose=story::sittingPose(.6f);pose.chan[C_EYE]=glm::vec3{0};pose.chan[C_CORE]=shot.parried?glm::vec3{1,.7f,.1f}:glm::vec3{0};
        Rig r{out};drawFile(r,root,*m,pose,{.1f,.1f,.12f});
    } else {Rig r{out};r.box(root,{0,1.2f,0},{.35f,.55f,.25f},{.7f,.7f,.65f});r.box(root,{0,1.7f,0},{.24f,.24f,.24f},{.7f,.7f,.65f});}
}
