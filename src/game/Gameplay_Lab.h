#pragma once

inline void GameplayState::beginLab(){
    audio.stop(labVoice);labVoice=0;labCue=-1;
    storyOwned|=15;storyArmed=storyGrapple=true;wakeSeen=true;wakeT=-1;clawT=-1;
    beltBodies.clear();player.eyeHeight=1.7f;
    bool skip=labIntro.seen||g_devCam||director.arena>0||g_bossRun>=0;
    labIntro.start(skip);
    if(g_devOverlay.rfind("labintro",0)==0){labIntro.seen=false;labIntro.time=std::clamp(std::atof(g_devOverlay.c_str()+8),0.,(double)LabIntro::DURATION-.1);}
    labTheatre=LabTheatre{};labTheatreCue=0;
    if(director.arena>0)labTheatre.time=60;   // (starting further down: it has long since happened)
    if(g_devOverlay.rfind("labtheatre",0)==0)labTheatre.time=std::atof(g_devOverlay.c_str()+10);
    labCue=0;
    if(labIntro.active()){
        int n;const auto* cue=LabIntro::cues(n);while(labCue<n && cue[labCue].at<labIntro.time)++labCue;
        player.position=labIntro.eye()-glm::vec3{0,1.7f,0};player.camera.yaw=labIntro.yaw();player.camera.pitch=labIntro.pitch();director.hold=true;
    }
    if(g_devOverlay=="labending"){   // --overlay labending (with --story3 --arena 10): the Successor dead, you stepping through its doors
        labIntro.time=-1;storyBossDown=true;successorFell=lab::centre(9)+glm::vec3{6,0,-26};successorFellYaw=.7f;successorFellSet=true;
        director.hold=true;player.position=glm::vec3{0,labend::floorY(),labend::doorZ()-1.6f};player.camera.yaw=-90;
    }
    for(int room:{2,6,7,8})for(int side:{-1,1}){
        if(room<7 && side>0)continue;
        Interactable control;control.position=lab::contactPosition(room,side);
        control.box={control.position-glm::vec3{1,.9f,1},control.position+glm::vec3{1,.9f,1}};
        control.label=room>=7?(side<0?"SEAT WEST CONTACT / ROUTE LEFT":"SEAT EAST CONTACT / ROUTE RIGHT"):"RELEASE THE ASSESSMENT LOCK";
        control.onInteract=[this,room,side](){
            if(director.arena!=room || !director.queue.empty())return;
            if(labRoutes&(1u<<room)){ui.feed("CONTACT ALREADY SEATED",{.4f,.8f,1});return;}
            for(const auto& enemy:enemies)if(enemy.alive){ui.feed("CLEAR THE ASSESSMENT FLOOR FIRST",{1,.75f,.3f});return;}
            labRoutes|=1u<<room;if(room>=7 && side<0)labRoutes|=1u<<(room+3);
            director.completeGoal();saveStoryProgress();
            audio.play("clank",110,SoundGroup::WORLD,true);ui.feed("CONTACT SEATED / SERVICE ROUTE OPEN",{.3f,1,.7f});
        };
        interactables.push_back(std::move(control));
    }
    if(g_devOverlay=="swordopen")swordStatueOpen=true;   // --overlay swordopen: the statue's seal already broken (screenshots)
    {   // OATHBREAKER, plunged into its statue in the Successor's arena: sealed until the Successor's second phase
        Interactable it;it.position=lab::swordStatueGrip();
        it.box={it.position-glm::vec3{.45f,1.f,.45f},it.position+glm::vec3{.45f,.5f,.45f}};
        it.label=swordStatueOpen?"PULL OATHBREAKER":"OATHBREAKER / SEALED";it.active=!storySword;
        it.onInteract=[this](){pullStatueSword();};
        swordStatueIt=(int)interactables.size();interactables.push_back(std::move(it));
    }
    for(const auto& contact:level.story.labBridges){auto& m=level.movers[contact.mover];
        bool west=(labRoutes&(1u<<(contact.room+3)))!=0;
        m.drive=((labRoutes&(1u<<contact.room)) && contact.side==(west?-1:1))?1.f:0.f;
    }
    level.updateMovers(moverClock);
    player.camera.position=player.position+glm::vec3{0,1.7f,0};prevCamPos=player.camera.position;
}

inline void GameplayState::updateLab(float dt){
    swordStatueFlash=std::max(0.f,swordStatueFlash-dt*1.5f);
    if(labEnding.active()){holsterSword();updateLabEnding(dt);return;}
    if(storyBossDown)openSwordStatue();   // (it falls before you reach the statue: the seal goes with it)
    if(labIntro.active()){
        int n;const auto* cue=LabIntro::cues(n);
        if(labIntro.update(dt,storySkipKey)){
            audio.stop(labVoice);labVoice=0;
            director.hold=false;player.position=LabIntro::handoff();player.velocity={0,0,0};
            player.camera.yaw=-90;player.camera.pitch=0;saveStoryProgress();
            pushBanner("THE LIVING WARD","CHAPTER 3 / MANUAL ASSESSMENT",{.5f,.9f,1},3.f);
            audio.play("count_go",80,SoundGroup::UI,true);
        }else{
            // The public address cuts off the last announcement; the lift's own sounds overlap
            for(;labCue<n && cue[labCue].at<=labIntro.time;++labCue){
                if(std::strncmp(cue[labCue].sound,"lab_pa",6)==0){audio.stop(labVoice);labVoice=audio.play(cue[labCue].sound,cue[labCue].volume,SoundGroup::UI,true);}
                else audio.play(cue[labCue].sound,cue[labCue].volume,SoundGroup::UI,true);
            }
            player.position=labIntro.eye()-glm::vec3{0,1.7f,0};player.velocity={0,0,0};
            player.camera.yaw=labIntro.yaw();player.camera.pitch=labIntro.pitch();
        }
        player.camera.position=player.position+glm::vec3{0,1.7f,0};prevCamPos=player.camera.position;return;
    }
    // Admissions' theatre runs once you walk up to its glass
    {const auto q=player.position;
        if(!labTheatre.started() && level.arenaAt(q)==0 && q.z<31 && std::abs(q.x)<22)labTheatre.time=0;
        if(labTheatre.started() && labTheatre.time<60){
            labTheatre.time+=dt;
            static const struct{float at;int pa;const char* line;}say[]={{.05f,1,"MOTOR RESPONSE: WITHIN NORMAL RANGE"},
                {6.5f,3,"OBSERVATION RECORD ACCEPTED"},{11.f,4,"PROCEED TO OBSERVATION."}};
            for(;labTheatreCue<3 && say[labTheatreCue].at<=labTheatre.time;++labTheatreCue){
                char name[24];std::snprintf(name,sizeof(name),"lab_pa_%d",say[labTheatreCue].pa);
                audio.stop(labVoice);labVoice=audio.play(name,105,SoundGroup::UI,true);ui.feed(say[labTheatreCue].line,{.55f,.9f,1});
            }
        }
    }
    for(const auto& contact:level.story.labBridges){auto& m=level.movers[contact.mover];
        bool west=(labRoutes&(1u<<(contact.room+3)))!=0;
        float target=((labRoutes&(1u<<contact.room)) && contact.side==(west?-1:1))?1.f:0.f;
        m.drive=std::min(target,m.drive+dt*.45f);
    }
    const auto p=player.position;int at=level.arenaAt(p);
    if(at>director.arena&&!director.fighting()&&director.phase!=WaveDirector::Phase::INTRO)director.approach(at);
    if(at>=0 && at>storyCheckpoint&&!director.fighting()&&!playerDead){storyCheckpoint=at;saveStoryProgress();ui.feed("CHECKPOINT",{.35f,.85f,1});}
    if(at>storySeenSection&&!playerDead){storySeenSection=at;pushBanner(lab::name(at),lab::objective(at),{.5f,.85f,.95f},2.7f);}
    if(storyLog>=0)storyLogT+=dt;
    if(director.arena==4)lockDoor(level.arenas[4].exitDoor,false);
    if(storyLog>=0 && glm::length(level.story.wrecks[storyLog].pos-p)>6)storyLog=-1;
    if(storyTerminal>=0 && glm::length(level.story.terminals[storyTerminal].pos-p)>6)storyTerminal=-1;
    if(director.fighting() && director.goal().kind==WaveGoal::EXTERNAL && (labRoutes&(1u<<director.arena))){
        bool alive=false;for(const auto& e:enemies)alive|=e.alive;
        if(!alive && director.queue.empty())director.completeGoal();
    }
    const auto& e=level.story.end;
    (void)e;
    // Through the doors the Successor held shut: the ending (Gameplay_LabEnding.h)
    if(storyBossDown && storyEndT<0 && !labEndingDone && !playerDead && p.z<labend::doorZ()-1.f && std::abs(p.x)<7.f && p.y>labend::floorY()-2.f && p.y<labend::floorY()+6.f)
        startLabEnding();
    if(storyEndT>=0){storyEndT+=dt;player.velocity={0,0,0};if(storyEndT>3)captureMouse(false);}
}

inline void GameplayState::gatherLab(std::vector<BoxInstance>& out){
    if((storyBossDown && level.arenaAt(player.position)>=8) || labEnding.active())gatherLabEnding(out);
    using namespace rig;using glm::vec3;Rig r{out};
    if(labAshSky()){gatherLabLift(r);return;}   // (the arrival's opening, under Chapter 2's sky)
    int room=std::max(0,level.arenaAt(player.position));
    auto subject=[&](vec3 p,float phase,bool suspended,float yaw=0){
        static const Enemy specimen(EnemyType::PATIENT,{0,0,0});Enemy body=specimen;body.position=p;body.spawnTimer=0;body.state=EnemyState::ACTIVE;
        body.animPhase=phase;body.moveSpeed=suspended?0:3.f;
        if(const auto* model=enemyfile::model(EnemyType::PATIENT)){
            mat4 root;FilePose pose;vec3 glow;filePoseOf(body,gameClock,T(p)*RY(yaw),root,pose,glow);
            if(suspended){pose.motion[enemyfile::E_ARML]=RZ(.45f);pose.motion[enemyfile::E_ARMR]=RZ(-.45f);pose.motion[enemyfile::E_HIPL]=RX(.08f);}
            // Dormant subjects have extinguished cores, no threat reticle, and no weapons.
            pose.chan[enemyfile::C_EYE]={.04f,.07f,.08f};pose.chan[enemyfile::C_CORE]={0,0,0};
            drawFile(r,root,*model,pose,glow*.08f);
        }
    };
    if(room==0){
        gatherLabLift(r);
        // The theatre: the subject walks a line until you come close, then lifts off the floor
        float walk=labTheatre.floating()?labTheatreWalk:gameClock;
        if(!labTheatre.floating())labTheatreWalk=gameClock;
        subject({std::sin(walk*.75f)*2.f,.6f+labTheatre.rise(),13.f},walk*3.f,labTheatre.floating());
        // Its projected silhouette keeps walking while the actual subject floats.
        float t=gameClock,x=std::sin(t*.75f)*3.f;
        r.box(T({x,0,7.24f}),{0,2,0},{.65f,1.5f,.035f},{.045f,.07f,.08f});
        r.box(T({x,0,7.25f}),{0,3.05f,0},{.38f,.45f,.04f},{.045f,.07f,.08f});
        for(float side:{-1.f,1.f})r.box(T({x+side*.2f,.6f,7.26f})*RZ(side*.15f*std::sin(t*2)),{0,0,0},{.18f,1.35f,.035f},{.045f,.07f,.08f});
        // Narrow mullions keep the observation surface apparent at every angle.
        for(float x:{-10.f,-5.f,5.f,10.f})r.box(T({x,4,18}),{0,0,0},{.065f,8,.065f},{.37f,.6f,.65f},{.03f,.07f,.08f});
    }
    if(room==1)subject(lab::centre(1)+vec3{-27,1.35f,9},1,true);
    // Someone pacing a lit ward out in the void, which has no door
    for(const auto& w:level.story.labWards){
        if(std::abs(w.room-room)>1)continue;
        float ph=gameClock*.22f+w.floor.x*.1f,dz=std::sin(ph)*w.span,dir=std::cos(ph)>0?0.f:3.1416f;
        subject(w.floor+vec3{0,0,dz},gameClock*2.6f,false,dir);
    }
    if(room>=6)for(int k=0;k<(ambientDetail()>0?9:3);++k){
        auto c=lab::centre(room);vec3 p=c+(room==9?vec3{(k%2?1.f:-1.f)*(88+k*3.f),15.f+(k%4)*5.f,-40.f+k*9.f}:vec3{(k%2?1.f:-1.f)*(15+k*2.f),5.f+(k%4)*2.f+.25f*std::sin(gameClock*.4f+k),-25.f+k*6});
        subject(p,k*.7f,true);
        if(room==6)r.box(T(p+vec3{0,5,0}),{0,0,0},{.025f,8,.025f},{.3f,.4f,.43f});
    }
    if(room==7)subject(lab::centre(7)+vec3{18,31.f+.1f*std::sin(gameClock*.1f),-15},.4f,true);
    for(int n:{2,6,7,8})if(std::abs(room-n)<=1)for(int side:{-1,1}){
        if(n<7 && side>0)continue;
        auto c=lab::contactPosition(n,side);
        bool used=labRoutes&(1u<<n);bool chosen=used && (n<7 || side==((labRoutes&(1u<<(n+3)))?-1:1));
        r.box(T(c),{0,0,0},{2.1f,2.6f,1.2f},{.16f,.26f,.3f});
        r.box(T(c),{0,.5f,.65f},{1.6f,.5f,.08f},{.3f,.4f,.4f},chosen?vec3{.2f,1,.6f}:used?vec3{.08f,.12f,.14f}:vec3{1,.7f,.2f});
    }
    for(int i=0;i<(int)level.story.wrecks.size();++i){const auto& w=level.story.wrecks[i];
        if(glm::length(w.pos-player.position)>65)continue;
        if(const auto* model=story::bodyModel(story::wreckFile(w.number)))drawFile(r,wreckRoot(i),*model,story::sittingPose(.4f),{.03f,.05f,.06f});
        r.box(T(w.pos+vec3{0,1.75f,.4f}),{0,0,0},{.14f,.08f,.025f},{.2f,.5f,.6f},(storyLogsSeen&(1u<<i))?vec3{.05f,.1f,.1f}:vec3{.3f,1.f,1.2f});
    }
    for(int k=0;k<(int)level.story.terminals.size();++k){auto p=level.story.terminals[k].pos;
        if(glm::length(p-player.position)>65)continue;bool seen=(storyTerminalsSeen>>k)&1u;
        r.box(T(p+vec3{0,2.4f,0})*RY(gameClock*.6f),{0,0,0},{.13f,.22f,.13f},{.4f,.8f,.9f},seen?vec3{.1f,.3f,.35f}:vec3{.2f,1.2f,1.5f});
    }
}

inline void GameplayState::renderLabIntro(){
    ui.begin2D();auto& u=ui.ui;
    u.rect(0,0,SCREEN_W,58,{.015f,.035f,.045f,1});u.rect(0,SCREEN_H-82,SCREEN_W,82,{.015f,.035f,.045f,1});
    u.text(labIntro.header(),48,22,2,labIntro.time<14.5f?glm::vec4{1,.62f,.45f,1}:glm::vec4{.65f,.9f,1,1});
    if(*labIntro.caption())u.text(labIntro.caption(),SCREEN_W/2,SCREEN_H-62,2,{.8f,.94f,.95f,1},true);
    std::string skip="HOLD "+binds().label(Act::JUMP)+" TO SKIP";u.text(skip.c_str(),SCREEN_W/2,SCREEN_H-28,1,{.55f,.7f,.74f,1},true);
    if(labIntro.skip>0)u.rect(SCREEN_W/2-90,SCREEN_H-12,180*labIntro.skip/LabIntro::SKIP_HOLD,3,{.3f,.9f,1,1});
    ui.end2D();
}

inline void GameplayState::renderLabGlass(const glm::mat4& view,const glm::mat4& proj,glm::vec3 camPos){
    using glm::vec3;const int room=labIntro.active()?0:level.arenaAt(player.position);if(room<0 || room==9 || room==4)return;
    labGlassShader.use();labGlassShader.setMat4("projection",proj);labGlassShader.setMat4("view",view);
    labGlassShader.setVec3("uViewPos",camPos);labGlassShader.setFloat("uTime",gameClock);
    auto c=lab::centre(room);labGlassShader.setVec3("uOrigin",c);
    glDisable(GL_CULL_FACE);glDepthMask(GL_FALSE);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(waterVAO);glBindBuffer(GL_ARRAY_BUFFER,waterVBO);
    int slot=0;
    auto quad=[&](vec3 a,vec3 b,vec3 d,vec3 e,vec3 normal,float mode,float anomaly){
        if(slot>=MAX_WATER)return;   // (each quad its own slot: rewriting one the GPU is still drawing would stall it)
        const vec3 v[]={a,b,d,a,d,e};glBufferSubData(GL_ARRAY_BUFFER,slot*sizeof(v),sizeof(v),v);
        labGlassShader.setVec3("uNormal",normal);labGlassShader.setFloat("uMode",mode);labGlassShader.setFloat("uAnomaly",anomaly);
        glDrawArrays(GL_TRIANGLES,slot*6,6);++slot;
    };
    // The enclosed wards' windows onto the void, and Motor Assessment's glass floor
    if(room<7){
        for(const auto& w:lab::windows(room)){vec3 n{w.x<0?1.f:-1.f,0,0};
            quad(c+vec3{w.x,w.y0,w.z0},c+vec3{w.x,w.y0,w.z1},c+vec3{w.x,w.y1,w.z1},c+vec3{w.x,w.y1,w.z0},n,2,0);}
        if(room==2){const AABB g=lab::glassFloor();float y=c.y+.006f;
            quad(c+vec3{g.min.x,0,g.min.z}+vec3{0,.006f,0},c+vec3{g.max.x,0,g.min.z}+vec3{0,.006f,0},
                 vec3{c.x+g.max.x,y,c.z+g.max.z},vec3{c.x+g.min.x,y,c.z+g.max.z},{0,1,0},2,0);}
    }
    if(room==0 && labIntro.active()){
        // The lift car's glass: sides, roof, floor plate and the four door leaves
        const float cy=labIntro.carY();
        for(float s:{-1.f,1.f})quad({s*2.8f,cy+1,40.9f},{s*2.8f,cy+1,46.1f},{s*2.8f,cy+3.3f,46.1f},{s*2.8f,cy+3.3f,40.9f},{-s,0,0},2,0);
        quad({-1.8f,cy+3.72f,41.4f},{1.8f,cy+3.72f,41.4f},{1.8f,cy+3.72f,45.6f},{-1.8f,cy+3.72f,45.6f},{0,-1,0},2,0);
        quad({-1.6f,cy-.02f,42.2f},{1.6f,cy-.02f,42.2f},{1.6f,cy-.02f,45.6f},{-1.6f,cy-.02f,45.6f},{0,1,0},2,0);
        auto leaf=[&](float o,float z,float y0){for(float s:{-1.f,1.f}){float a=s*(o+.05f),b=s*(o+1.95f);
            quad({a,cy+y0,z},{b,cy+y0,z},{b,cy+3.f,z},{a,cy+3.f,z},{0,0,z<43?-1.f:1.f},2,0);}};
        leaf(labIntro.frontDoors()*1.95f,40.67f,1.f);leaf(labIntro.rearDoors()*1.95f,46.33f,.1f);
    }
    if(room==0){
        float anomaly=labTheatre.windowChange();
        quad({-9.8f,.6f,7.28f},{9.8f,.6f,7.28f},{9.8f,7.9f,7.28f},{-9.8f,7.9f,7.28f},{0,0,1},1,anomaly);
        quad({-10,.6f,18},{10,.6f,18},{10,8,18},{-10,8,18},{0,0,1},0,anomaly);
    }else if(room==3){
        quad(c+vec3{41.9f,3,-13.8f},c+vec3{41.9f,3,13.8f},c+vec3{41.9f,14,13.8f},c+vec3{41.9f,14,-13.8f},{-1,0,0},2,0);
        quad(c+vec3{55.9f,6,-6.5f},c+vec3{55.9f,6,6.5f},c+vec3{55.9f,12,6.5f},c+vec3{55.9f,12,-6.5f},{-1,0,0},2,0);
    }else if(room>=7){
        for(const auto& bay:lab::footprint(room))for(float x:{bay.left,bay.right}){
            float side=x<0?-1.f:1.f,inside=x-side*.12f;
            quad(c+vec3{inside,1.4f,bay.farZ},c+vec3{inside,1.4f,bay.nearZ},
                 c+vec3{inside,14,bay.nearZ},c+vec3{inside,14,bay.farZ},{-side,0,0},2,0);
        }
    }
    glBindVertexArray(0);glDepthMask(GL_TRUE);glDisable(GL_BLEND);glEnable(GL_CULL_FACE);
}
