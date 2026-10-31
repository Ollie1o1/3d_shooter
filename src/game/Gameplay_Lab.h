#pragma once

inline void GameplayState::beginLab(){
    audio.stop(labVoice);audio.stop(successorVoice);labVoice=successorVoice=0;labCue=-1;
    labDialogueT=0;labDialogue=-1;
    storyOwned|=15;storyArmed=storyGrapple=true;wakeSeen=true;wakeT=-1;clawT=-1;
    beltBodies.clear();player.eyeHeight=1.7f;
    bool skip=labIntro.seen||g_practice||g_devCam||director.arena>0||g_bossRun>=0;
    labIntro.start(skip);
    if(g_devOverlay.rfind("labintro",0)==0){labIntro.seen=false;labIntro.time=std::clamp(std::atof(g_devOverlay.c_str()+8),0.,22.9);}
    if(labIntro.active()){player.position={0,0,27};player.camera.yaw=-90;player.camera.pitch=0;director.hold=true;}
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
    for(const auto& contact:level.story.labBridges){auto& m=level.movers[contact.mover];
        bool west=(labRoutes&(1u<<(contact.room+3)))!=0;
        m.drive=((labRoutes&(1u<<contact.room)) && contact.side==(west?-1:1))?1.f:0.f;
    }
    level.updateMovers(moverClock);
    player.camera.position=player.position+glm::vec3{0,1.7f,0};prevCamPos=player.camera.position;
}

inline void GameplayState::updateLab(float dt){
    if(labIntro.active()){
        int cue=labIntro.time<4?0:labIntro.time<10?1:labIntro.time<16?2:labIntro.time<20?3:4;
        if(cue!=labCue){labCue=cue;char name[24];std::snprintf(name,sizeof(name),"lab_pa_%d",cue);audio.stop(labVoice);labVoice=audio.play(name,110,SoundGroup::UI,true);}
        if(labIntro.update(dt,storySkipKey)){
            audio.stop(labVoice);labVoice=0;
            director.hold=false;player.position={2.2f,0,20};player.velocity={0,0,0};
            player.camera.yaw=-90;player.camera.pitch=0;saveStoryProgress();
            pushBanner("THE LIVING WARD","CHAPTER 3 / MANUAL ASSESSMENT",{.5f,.9f,1},3.f);
            audio.play("count_go",80,SoundGroup::UI,true);
        }else{
            player.position=labIntro.cameraOffset()-glm::vec3{0,1.7f,0};player.velocity={0,0,0};
            player.camera.yaw=labIntro.yaw();player.camera.pitch=labIntro.pitch();
        }
        player.camera.position=player.position+glm::vec3{0,1.7f,0};prevCamPos=player.camera.position;return;
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
    if(labDialogueT>0)labDialogueT-=dt;
    const auto& e=level.story.end;
    if(storyBossDown && storyEndT<0 && p.x>e.min.x&&p.x<e.max.x&&p.y>e.min.y&&p.y<e.max.y&&p.z>e.min.z&&p.z<e.max.z){
        storyEndT=0;saveStoryProgress();logRun("end");audio.play("wave",100,SoundGroup::UI,true);
    }
    if(storyEndT>=0){storyEndT+=dt;player.velocity={0,0,0};if(storyEndT>3)captureMouse(false);}
}

inline void GameplayState::gatherLab(std::vector<BoxInstance>& out){
    using namespace rig;using glm::vec3;Rig r{out};
    int room=std::max(0,level.arenaAt(player.position));
    auto subject=[&](vec3 p,float phase,bool suspended){
        static const Enemy specimen(EnemyType::PATIENT,{0,0,0});Enemy body=specimen;body.position=p;body.spawnTimer=0;body.state=EnemyState::ACTIVE;
        body.animPhase=phase;body.moveSpeed=suspended?0:3.f;
        if(const auto* model=enemyfile::model(EnemyType::PATIENT)){
            mat4 root;FilePose pose;vec3 glow;filePoseOf(body,gameClock,T(p),root,pose,glow);
            if(suspended){pose.motion[enemyfile::E_ARML]=RZ(.45f);pose.motion[enemyfile::E_ARMR]=RZ(-.45f);pose.motion[enemyfile::E_HIPL]=RX(.08f);}
            // Dormant subjects have extinguished cores, no threat reticle, and no weapons.
            pose.chan[enemyfile::C_EYE]={.04f,.07f,.08f};pose.chan[enemyfile::C_CORE]={0,0,0};
            drawFile(r,root,*model,pose,glow*.08f);
        }
    };
    if(room==0){
        float t=labIntro.active()?labIntro.time:23.f;float walk=std::min(t,10.f);
        subject({std::sin(walk*.75f)*2.f,.6f+(labIntro.active()?labIntro.patientRise():1.35f),13.f},walk*3.f,t>=10);
        // Its projected silhouette keeps walking while the actual subject floats.
        float x=std::sin(t*.75f)*3.f;
        r.box(T({x,0,7.24f}),{0,2,0},{.65f,1.5f,.035f},{.045f,.07f,.08f});
        r.box(T({x,0,7.25f}),{0,3.05f,0},{.38f,.45f,.04f},{.045f,.07f,.08f});
        for(float side:{-1.f,1.f})r.box(T({x+side*.2f,.6f,7.26f})*RZ(side*.15f*std::sin(t*2)),{0,0,0},{.18f,1.35f,.035f},{.045f,.07f,.08f});
        // Narrow mullions keep the observation surface apparent at every angle.
        for(float x:{-10.f,-5.f,5.f,10.f})r.box(T({x,4,18}),{0,0,0},{.065f,8,.065f},{.37f,.6f,.65f},{.03f,.07f,.08f});
    }
    if(room==1)subject(lab::centre(1)+vec3{-27,1.35f,9},1,true);
    if(room>=6)for(int k=0;k<(ambientDetail()>0?9:3);++k){
        auto c=lab::centre(room);vec3 p=c+vec3{(k%2?1.f:-1.f)*(15+k*2.f),5.f+(k%4)*2.f+.25f*std::sin(gameClock*.4f+k),-25.f+k*6};
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
    u.text("THE LIVING WARD / ADMISSIONS",48,22,2,{.65f,.9f,1,1});
    u.text(labIntro.caption(),SCREEN_W/2,SCREEN_H-62,2,{.8f,.94f,.95f,1},true);
    std::string skip="HOLD "+binds().label(Act::JUMP)+" TO SKIP";u.text(skip.c_str(),SCREEN_W/2,SCREEN_H-28,1,{.55f,.7f,.74f,1},true);
    if(labIntro.skip>0)u.rect(SCREEN_W/2-90,SCREEN_H-12,180*labIntro.skip/LabIntro::SKIP_HOLD,3,{.3f,.9f,1,1});
    ui.end2D();
}

inline void GameplayState::renderLabGlass(const glm::mat4& view,const glm::mat4& proj,glm::vec3 camPos){
    using glm::vec3;const int room=level.arenaAt(player.position);if(room<0 || (room<7 && room!=0))return;
    labGlassShader.use();labGlassShader.setMat4("projection",proj);labGlassShader.setMat4("view",view);
    labGlassShader.setVec3("uViewPos",camPos);labGlassShader.setFloat("uTime",gameClock);
    auto c=lab::centre(room);labGlassShader.setVec3("uOrigin",c);
    glDisable(GL_CULL_FACE);glDepthMask(GL_FALSE);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(waterVAO);glBindBuffer(GL_ARRAY_BUFFER,waterVBO);
    auto quad=[&](vec3 a,vec3 b,vec3 d,vec3 e,vec3 normal,float mode,float anomaly){
        const vec3 v[]={a,b,d,a,d,e};glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(v),v);
        labGlassShader.setVec3("uNormal",normal);labGlassShader.setFloat("uMode",mode);labGlassShader.setFloat("uAnomaly",anomaly);
        glDrawArrays(GL_TRIANGLES,0,6);
    };
    if(room==0){
        float anomaly=labIntro.active()?labIntro.windowChange():1.f;
        quad({-9.8f,.6f,7.28f},{9.8f,.6f,7.28f},{9.8f,7.9f,7.28f},{-9.8f,7.9f,7.28f},{0,0,1},1,anomaly);
        quad({-10,.6f,18},{10,.6f,18},{10,8,18},{-10,8,18},{0,0,1},0,anomaly);
    }else{
        for(float side:{-1.f,1.f})quad(c+vec3{side*41.9f,1.6f,-39.5f},c+vec3{side*41.9f,1.6f,39.5f},c+vec3{side*41.9f,18,39.5f},c+vec3{side*41.9f,18,-39.5f},{-side,0,0},2,0);
        quad(c+vec3{-41.5f,23,-39.5f},c+vec3{41.5f,23,-39.5f},c+vec3{41.5f,23,39.5f},c+vec3{-41.5f,23,39.5f},{0,-1,0},2,0);
    }
    glBindVertexArray(0);glDepthMask(GL_TRUE);glDisable(GL_BLEND);glEnable(GL_CULL_FACE);
}
