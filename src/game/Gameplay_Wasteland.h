#pragma once
// Chapter 2 shares inventory, explicit readable interactions, retries and HUD
// with the Cradle. Its outdoor scripts never run the factory's indexed machinery.
inline void GameplayState::beginWasteland() {
    storyOwned|=level.story.startWeapons;storyArmed=true;storyGrapple=true;
    wakeT=-1;player.eyeHeight=1.7f;clawT=-1;beltBodies.clear();
    if(!wakeSeen && director.arena==0)pushBanner(level.story.title,"CHAPTER 2  /  THE WORLD OUTSIDE",{.85f,.7f,.45f},4.f);
    wakeSeen=true;
    const char* starts[]={"ash","convoy","relay","span","turbines","toll","ballast","road"};
    for(int i=0;i<8;++i)if(g_devOverlay==std::string("waste-")+starts[i] && director.arena!=i) {
        g_devOverlay.clear();enterArena(i);storySeenSection=storyCheckpoint=i;return;
    }
    if(director.arena>level.story.bossSection)storyBossDown=true;
    player.camera.position=player.position+glm::vec3{0,player.eyeHeight,0};prevCamPos=player.camera.position;
}

inline void GameplayState::loadStoryChapter(int chapter) {
    if(chapter!=2)return;
    logRun("chapter_complete");
    g_storyChapter=2;g_startArena=g_startWave=0;g_devOverlay.clear();
    // Release the non-RAII contact buffers before moving in the new world.
    world.contact.release();movingShadows.release();
    level=buildWasteland();world=buildWorldMeshes(level);spatialGrid.build(level.walls);
    bake=&sharedBake("story2",level);applyBake();neonLights=buildNeonLights(level);
    shifts=ArenaShifts{};shifts.capture(level);
    anchorsBuilt=level.anchors;anchorBoxes.clear();
    for(const auto& a:level.anchors)anchorBoxes.push_back(level.walls[a.wall].box);
    storyGlassBoxes.clear();storyTileBoxes.clear();
    director=WaveDirector{};director.level=&level;director.fast=true;
    storyLogsSeen=0;storySeenSection=storyCheckpoint=0;storyBossDown=false;
    storyEndT=-1;wakeSeen=false;wakeT=-1;storyLog=storyTerminal=-1;
    storyOwned|=7;storyGrapple=true;storyArmed=true;
    totalKills=totalShots=totalHits=deaths=0;elapsedTime=0;peakStyle=0;
    bankedStyle=bankedDamage=0;styleSystem=StyleSystem{};
    spaceArena=-1;warmupFrames=2;
    enterArena(0);captureMouse(true);
}

inline void GameplayState::updateWasteland(float dt) {
    const auto& s=level.story;const glm::vec3 p=player.position;
    for(const auto& front:ballastHazards.fronts)if(front.rubble) {
        auto from=front.from+front.dir*front.distance;
        breakStoryCover(from,from+front.dir*(BallastHazards::frontSpeed(front)*dt),2.f);
    }
    for(const auto& hit:ballastHazards.update(dt,p,level.walls))
        if(!playerDead&&!storyBossDown) {
            damagePlayer(hit.damage*level.arenas[s.bossSection].damageScale*tune().damage,hit.at,.15f,.03f);
            if(!g_godMode&&dashMomentumTimer<=0)player.velocity+=hit.push;
        }
    for(int k=0;k<(int)s.turbineBrakes.size();++k)
        if(!level.anchors[s.turbineBrakes[k]].alive)storyTurbineAngle[k]+=dt*.65f;
    if(director.arena==4&&director.fighting()&&!director.goalDone &&
       !s.turbineBrakes.empty() && level.anchorsAlive(LevelData::ChainAnchor::BRAKE)==0)
        director.completeGoal();
    int at=level.arenaAt(p);
    if(at>director.arena&&!director.fighting()&&director.phase!=WaveDirector::Phase::INTRO)director.approach(at);
    if(at>=0 && at>storySeenSection&&!playerDead) {
        storySeenSection=at;pushBanner(level.arenas[at].name,"",{.85f,.72f,.48f},2.5f);
        audio.play("wave",64,SoundGroup::UI,true);
    }
    if(at>=0&&at>storyCheckpoint&&!director.fighting()) {
        storyCheckpoint=at;ui.feed("CHECKPOINT",{.35f,.85f,1.f});
    }
    if(storyLog>=0) {storyLogT+=dt;if(glm::length(s.wrecks[storyLog].pos-p)>6)storyLog=-1;}
    if(storyTerminal>=0&&glm::length(s.terminals[storyTerminal].pos-p)>6)storyTerminal=-1;
    // Subtle arcs from the old transmission equipment; no gameplay hit volume.
    if((at==2||at==4)&&std::floor(gameClock/8.f)!=std::floor((gameClock-dt)/8.f)) {
        glm::vec3 source=at==2?glm::vec3{-72,-1.5f,-27}:glm::vec3{33,3,-119};
        fx.spawnAmbientBurst(source,{.7f,.48f,.2f},5,1.5f,.4f,5.f);
        if(!director.fighting())audio.playAt("clank",source,24,SoundGroup::WORLD);
    }
    const auto& end=s.end;
    if(storyBossDown&&storyEndT<0&&!playerDead&&p.x>end.min.x&&p.x<end.max.x&&p.y>end.min.y&&p.y<end.max.y&&p.z>end.min.z&&p.z<end.max.z) {
        storyEndT=0;logRun("end");audio.play("wave",90,SoundGroup::UI,true);
    }
    if(storyEndT>=0) {
        storyEndT+=dt;player.velocity=glm::vec3{0,std::max(0.f,player.velocity.y),0};
        if(storyEndT>3)captureMouse(false);
    }
}

inline void GameplayState::gatherWasteland(std::vector<BoxInstance>& out) {
    using namespace rig;using glm::vec3;
    Rig r{out};const auto& s=level.story;
    gatherBallast(out);gatherStoryCover(out);
    if(level.arenaAt(player.position)==2) {
        float progress=director.arena>2?1.f:director.arena==2?director.goalProgress():0.f;
        vec3 panel{-74,-2.65f,-48.4f};
        r.box(T(panel),{0,0,0},{2.2f,.26f,.09f},{.08f,.11f,.1f});
        if(progress>0)r.box(T(panel),{-(1.f-progress),0,-.06f},{2.f*progress,.15f,.05f},{.3f,.7f,.5f},{.2f,1.1f,.6f});
        for(int k=0;k<3;++k) {
            vec3 color=progress>(k+1)/3.f-.01f?vec3{.2f,1.2f,.55f}:vec3{.75f,.3f,.08f};
            r.box(T(panel),{-.7f+k*.7f,.5f,-.05f},{.18f,.16f,.12f},color*.4f,color);
        }
    }
    auto beacon=[&](vec3 p,float phase,bool seen){
        if(glm::length(p-player.camera.position)>65)return;
        p.y+=.1f*std::sin(gameClock*2+phase);vec3 glow=seen?vec3{.15f,.45f,.55f}:vec3{.25f,1.7f,2.f};
        auto root=T(p)*RY(gameClock*.65f+phase);
        r.box(root,{0,0,0},{.13f,.22f,.13f},{.7f,.95f,1},glow);
        for(float side:{-1.f,1.f}) {
            r.box(root,{side*.22f,0,0},{.035f,.3f,.035f},{.3f,.7f,.8f},glow*.55f);
            r.box(root,{side*.17f,-.15f,0},{.13f,.035f,.035f},{.3f,.7f,.8f},glow*.55f);
        }
    };
    for(int i=0;i<(int)s.wrecks.size();++i)beacon(s.wrecks[i].pos+vec3{0,1.95f,0},i*.8f,(storyLogsSeen>>i)&1u);
    for(int i=0;i<(int)s.terminals.size();++i)beacon(s.terminals[i].pos+vec3{0,2.65f,.25f},i*.6f,storyTerminal==i);
    // The brake housings are real shot targets. Their visibility is gameplay,
    // so neither they nor their status lamps depend on decorative detail.
    for(int id:s.turbineBrakes) {
        const auto& a=level.anchors[id];
        if(glm::length(a.pos-player.position)>75.f)continue;
        vec3 c=a.alive?vec3{1.5f,.7f,.12f}:vec3{.15f,.75f,.4f};
        r.box(T(a.pos),{0,0,0},{1.35f,1.15f,1.35f},{.18f,.19f,.16f});
        if(a.alive) {
            r.box(T(a.pos)*RZ(.75f),{0,0,.72f},{1.2f,.2f,.08f},{.7f,.6f,.35f},c);
            r.box(T(a.pos)*RZ(-.75f),{0,0,.72f},{1.2f,.2f,.08f},{.7f,.6f,.35f},c);
        }
        r.box(T(a.pos),{0,.75f,0},{.32f,.14f,.32f},c,c);
    }
    const vec3 machines[]={{-20,0,-116},{33,0,-119},{33,0,-82},{-19,0,-68}};
    for(int i=0;i<4;++i) {
        if(glm::length(machines[i]-player.position)>85.f)continue;
        float angle=i<3&&ambientDetail()>0?storyTurbineAngle[i]:0;
        auto hub=T(machines[i])*RY(glm::radians(15.f))*T({0,11.6f,1.65f});
        for(int k=0;k<3;++k) {
            auto blade=hub*RZ(angle+k*2.094395f+.26f);
            r.box(blade,{0,2.8f,0},{.7f,5.2f,.18f},{.52f,.47f,.34f});
            r.box(blade,{0,4.5f,.12f},{.6f,.35f,.05f},{.17f,.2f,.19f});
        }
    }
    if(ambientDetail()>0) {
        // Loose clearance tape moves in the same prevailing wind as the ash.
        const vec3 ties[]={{7,2.7f,46},{-8,2.7f,5},{-35,2.5f,-31},{-72,2.7f,-91},{24,2.7f,-143},{34,2.7f,-161}};
        for(vec3 tie:ties)if(glm::length(tie-player.position)<(ambientDetail()==1?20.f:40.f)) {
            for(int k=0;k<3;++k) {
                vec3 at=tie+vec3{.45f+k*.55f,-k*.12f,.17f*std::sin(gameClock*2.1f+k*.8f)};
                r.box(T(at)*RY(.2f*std::sin(gameClock*2+k)),{0,0,0},{.58f,.16f,.035f},k%2?vec3{.18f,.18f,.14f}:vec3{.62f,.5f,.25f});
            }
        }
    }
    bool live=false;for(const auto& e:enemies)if(e.alive&&e.type==EnemyType::BALLAST)live=true;
    int at=level.arenaAt(player.position);
    if(!live&&!storyBossDown&&(at==5||at==6)) {
        Enemy e{EnemyType::BALLAST,level.arenas[s.bossSection].bossSpawn};e.spawnTimer=0;e.state=EnemyState::ACTIVE;
        e.ballRotor=gameClock*2.f;
        e.yaw=std::atan2(player.position.x-e.position.x,player.position.z-e.position.z);
        buildEnemy(e,gameClock,out);
    }
}
