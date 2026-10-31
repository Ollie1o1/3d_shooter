#pragma once
inline const char* successorLine(int i){static const char* lines[]={"UNIT TWENTY-FOUR. STILL RUNNING.","COME CLOSER. I WANT A PROPER COMPARISON.","THAT WAS NOT IN YOUR DESIGN.","THERE. SOMETHING THEY MISSED.","NO MORE CORRECTIONS.","I WAS SUPPOSED TO LEAVE.","YOU WERE SUPPOSED TO BE THE EASY PART."};return lines[std::clamp(i,0,6)];}
inline void GameplayState::saySuccessor(int line){
    if(line<0 || line>6 || (successorHeard&(1u<<line)))return;
    successorHeard|=1u<<line;
    storyLog=storyTerminal=-1;labDialogue=line;labDialogueT=line==0||line==1?4.5f:3.4f;
    char sound[40];std::snprintf(sound,sizeof(sound),"successor_line_%d",line);
    audio.stop(successorVoice);successorVoice=audio.play(sound,120,SoundGroup::UI,true);audio.duck(3.f,1.8f);
}
inline void GameplayState::successorEvents(Enemy& e,const EnemyEvents& ev){
    if(successorHazards.epoch!=e.suEpoch){successorHazards.clear();successorHazards.epoch=e.suEpoch;}
    if(ev.suPhaseStarted){saySuccessor(ev.suLine);if(ev.suPhaseStarted==1)successorIntroLine=2.7f;audio.play("anvil",90,SoundGroup::UI,true);}
    if(ev.suLine==6)saySuccessor(6);
    if(ev.suMark){fx.spawnShockwave(ev.suAt+glm::vec3{0,.08f,0},2.f,{.45f,.75f,1});audio.playAt("grapple_fire",ev.suAt,110,SoundGroup::ENEMY,true);}
    if(ev.suLock)audio.playAt("count_tick",e.position,100,SoundGroup::ENEMY,true);
    if(ev.suImpact){audio.playAt("punch",e.position,90,SoundGroup::ENEMY,true);}
    if(ev.suSuture){
        auto a=ev.suAt,b=ev.suTo;a.y=b.y=e.floorY;
        auto d=glm::normalize(glm::vec3{b.x-a.x,0,b.z-a.z});
        successorHazards.line(a,a+d*23.f,.8f);successorHazards.line(b,b-d*23.f,.8f);
    }
    if(ev.suRecall)successorHazards.circle(ev.suAt,1.05f,24);
    if(ev.suEcho)successorHazards.circle(ev.suAt,1.05f,18,2);
    if(ev.suCore){
        auto origin=e.position+glm::vec3{0,1.9f,0};auto dir=glm::normalize(player.camera.position-origin);
        auto* p=projSystem.fire(origin,dir*15.f,18,false,{1.4f,1.3f,1.f},false,0,1.1f,true);
        if(p){p->owner=e.uid;p->ownerType=(int)e.type;p->parryDamage=240;}
    }
}
inline void GameplayState::updateSuccessor(float dt){
    Enemy* boss=nullptr;for(auto& e:enemies)if(e.alive&&e.type==EnemyType::SUCCESSOR){boss=&e;break;}
    if(!boss){successorHazards.clear();return;}
    if(successorHazards.epoch!=boss->suEpoch){successorHazards.clear();successorHazards.epoch=boss->suEpoch;}
    float damage=0;auto hits=successorHazards.update(dt,player.position,damage);
    if(!hits.empty())damagePlayer(damage*tune().damage,hits[0],.2f,.04f);
    if(successorIntroLine>0){successorIntroLine-=dt;if(successorIntroLine<=0)saySuccessor(1);}
}
inline bool GameplayState::shootSuccessorNode(glm::vec3 origin,glm::vec3 dir,float& range,float damage){
    for(auto& e:enemies)if(e.alive&&e.type==EnemyType::SUCCESSOR&&e.suNodes>0){
        for(int k=0;k<2;++k){auto p=e.suAnchor[k];float t=rayBoxHit(origin,dir,{p-glm::vec3{.65f},p+glm::vec3{.65f}});
            if(t>0 && t<range){range=t;e.suNodeHp[k]-=damage;fx.spawnHitSparks(p,{.8f,.6f,1});
                if(e.suNodeHp[k]<=0){e.stagger(1.4f);successorHazards.clear();ui.feed("SUTURE CUT / DRIVE EXPOSED",{.35f,1,.7f});audio.playAt("clank",p,110,SoundGroup::ENEMY,true);}return true;}
        }
    }
    return false;
}
inline void GameplayState::gatherSuccessor(std::vector<BoxInstance>& out){
    using namespace rig;using glm::vec3;Rig r{out};
    auto line=[&](vec3 a,vec3 b,float width,vec3 color,vec3 glow){auto d=b-a;float n=glm::length(d);if(n<.001f)return;
        auto root=T((a+b)*.5f)*RY(std::atan2(d.x,d.z))*RX(-std::atan2(d.y,glm::length(glm::vec2{d.x,d.z})));
        r.box(root,{0,0,0},{width,width,n},color,glow);
    };
    auto ring=[&](vec3 p,float radius,vec3 glow){for(int k=0;k<32;++k){float a=k*.19634954f;line(p+vec3{std::cos(a)*radius,.05f,std::sin(a)*radius},p+vec3{std::cos(a+.19635f)*radius,.05f,std::sin(a+.19635f)*radius},.11f,glow*.4f,glow);}};
    for(const auto& m:successorHazards.marks){vec3 col=m.kind==2?vec3{.75f,.35f,1.3f}:vec3{1.5f,.25f,.2f};
        if(m.kind==1)line(m.a+vec3{0,.08f,0},m.b+vec3{0,.08f,0},m.radius*2,col*.4f,col);
        else ring(m.a,m.radius,col);
    }
    for(const auto& e:enemies){if(!e.alive)continue;
        if(e.type==EnemyType::SUCCESSOR){
            if(e.telegraphTimer>0){
                auto color=SuccessorState::white(e.attack)?vec3{2}:e.attack==AttackKind::SU_VECTOR||e.attack==AttackKind::SU_INCISION?abGoldTell(vec3{1.5f,1,.15f}):vec3{1.5f,.2f,.25f};
                if(e.attack==AttackKind::SU_VECTOR)line(e.suFrom+vec3{0,.06f,0},e.suAim+vec3{0,.06f,0},2.7f,color*.25f,color*.7f);
                else if(e.attack==AttackKind::SU_RECALL)ring(e.suAim,2.8f,color);
                else{ring(e.position,.95f,color*.6f);line(e.position+vec3{.4f,1.4f,.2f},e.position+vec3{.4f,2.2f,.2f},.09f,color,color);}
            }
            if(e.suCross>0){ring(e.suExit,2.f,{.4f,.85f,1.4f});line(e.suFrom+vec3{0,1,0},e.suExit+vec3{0,1,0},.055f,{.3f,.5f,.7f},{.5f,1,1.5f});}
            if(e.suNodes>0)for(int k=0;k<2;++k){line(e.position+vec3{0,1.8f,0},e.suAnchor[k],.065f,{.6f,.3f,.35f},{.8f,.4f,.6f});r.box(T(e.suAnchor[k])*RY(gameClock),{0,0,0},{.7f,.7f,.7f},{.5f,.35f,.4f},{1.1f,.65f,.9f});}
        }else if(e.type==EnemyType::SUSPENDED && (e.telegraphTimer>0||e.labBeam>0)){
            line(e.position+vec3{0,1.7f,0},e.labAim,e.labBeam>0?.2f:.045f,{.45f,.3f,.7f},{.8f,.45f,1.2f});ring(e.labAim,1.2f,{.65f,.4f,1});
        }else if(e.type==EnemyType::PHASED && (e.labCross>0||e.attack==AttackKind::LAB_CROSS)){ring(e.labExit,1.3f,{.4f,.8f,1.1f});}
    }
}
inline void GameplayState::renderSuccessorDialogue(){
    if(labDialogueT<=0||labDialogue<0)return;
    ui.begin2D();ui.ui.rect(150,SCREEN_H-190,SCREEN_W-300,58,{.015f,.025f,.04f,.88f});
    ui.ui.text(successorLine(labDialogue),SCREEN_W/2,SCREEN_H-173,2,{.9f,.95f,1,std::min(1.f,labDialogueT)},true);ui.end2D();
}
