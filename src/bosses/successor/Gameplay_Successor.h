#pragma once
inline void GameplayState::successorEvents(Enemy& e,const EnemyEvents& ev){
    if(successorHazards.epoch!=e.suEpoch){successorHazards.clear();successorHazards.epoch=e.suEpoch;}
    if(ev.suPhaseStarted>=2)openSwordStatue();   // the statue's seal breaks
    if(ev.suPhaseStarted)audio.play("anvil",90,SoundGroup::UI,true);
    if(ev.suMark){fx.spawnShockwave(ev.suAt+glm::vec3{0,.08f,0},2.f,{.45f,.75f,1});audio.playAt("grapple_fire",ev.suAt,110,SoundGroup::ENEMY,true);}
    if(ev.suLock)audio.playAt("count_tick",e.position,100,SoundGroup::ENEMY,true);
    if(ev.suImpact){audio.playAt("punch",e.position,90,SoundGroup::ENEMY,true);}
    if(ev.suSuture){
        auto a=ev.suAt,b=ev.suTo;a.y=b.y=e.floorY;
        auto d=glm::normalize(glm::vec3{b.x-a.x,0,b.z-a.z});
        successorHazards.line(a,a+d*23.f,.8f);successorHazards.line(b,b-d*23.f,.8f);
    }
    if(ev.suRecall)successorHazards.circle(ev.suAt,1.05f,24);
    if(ev.suLattice || ev.suCascade){
        const int room=director.arena;const AABB* box=room>=0&&room<(int)level.arenas.size()?&level.arenas[room].bounds:nullptr;
        auto keep=[&](glm::vec3 p){if(box)p=machineryClamp(p,*box,1.5f);p.y=e.floorY;return p;};
        auto at=ev.suAt;at.y=e.floorY;glm::vec3 d{at.x-ev.suTo.x,0,at.z-ev.suTo.z};
        d=glm::length(d)>.1f?glm::normalize(d):glm::vec3{0,0,1};const glm::vec3 side{-d.z,0,d.x};
        if(ev.suLattice){
            // First the rake along his line to you (step sideways between them), then, from phase 2,
            // the crossing rake (step forward or back): a grid closing on where you stood
            const float quick=ev.suRows==3?.85f:1.f;
            for(float o:{-7.f,0.f,7.f})successorHazards.line(keep(at+side*o-d*26.f),keep(at+side*o+d*24.f),.6f*quick,.85f,20);
            if(ev.suRows>=2)for(float o:{-7.f,0.f,7.f})successorHazards.line(keep(at+d*o-side*25.f),keep(at+d*o+side*25.f),1.25f*quick,.85f,20);
            audio.playAt("anvil",e.position,95,SoundGroup::ENEMY,true);
        }
        if(ev.suCascade){
            // The floor bursts in a line out from him, each a beat after the last: run across it
            static const float fan1[]={0.f},fan2[]={-.2f,.2f},fan3[]={-.34f,0.f,.34f};
            const float* fan=ev.suRows==3?fan3:ev.suRows==2?fan2:fan1;
            glm::vec3 from=ev.suTo;from.y=e.floorY;
            for(int r=0;r<ev.suRows;++r){
                float c=std::cos(fan[r]),s=std::sin(fan[r]);glm::vec3 dir{d.x*c-d.z*s,0,d.x*s+d.z*c};
                for(int k=0;k<11;++k)successorHazards.circle(keep(from+dir*(3.5f+k*3.8f)),.32f+k*.08f,20,0,2.1f,3.f);
            }
            fx.spawnShockwave(from+glm::vec3{0,.08f,0},4.f,{1.6f,.2f,.25f});
            audio.playAt("anvil",e.position,105,SoundGroup::ENEMY,true);shake(.2f,.03f);
        }
    }
    if(ev.suEcho)successorHazards.circle(ev.suAt,1.05f,18,2);
    if(ev.suBreaker){
        successorHazards.circle(ev.suAt,.01f,32,3,4.8f,6.f);
        fx.spawnShockwave(ev.suAt+glm::vec3{0,.08f,0},8.f,{1.7f,.1f,.25f});
        fx.spawnBurst(ev.suAt+glm::vec3{0,.5f,0},{1.5f,.15f,.2f},26,9,.5f,12);
        audio.playAt("anvil",ev.suAt,110,SoundGroup::ENEMY,true);shake(.35f,.05f);
    }
    if(ev.suCore){
        auto origin=e.position+glm::vec3{0,2.65f,0};auto dir=glm::normalize(player.camera.position-origin);
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
    if(laboratory())gatherSwordStatue(out);
    if(laboratory() && level.arenaAt(player.position)==9){
        float heat=0;for(const auto& boss:enemies)if(boss.alive && boss.type==EnemyType::SUCCESSOR)heat=1-boss.health/boss.maxHealth;
        auto centre=lab::centre(9)+vec3{0,32,-12};
        for(int orbit=0;orbit<2;++orbit){
            float radius=94+orbit*15,tilt=orbit?.55f:-.32f,spin=gameClock*(orbit?-.025f:.035f);
            for(int k=0;k<64;++k){if((k+orbit*3)%9==0)continue;
                auto point=[&](float a){a+=spin;return centre+vec3{std::cos(a)*radius,std::sin(a)*radius*std::sin(tilt)+orbit*19,std::sin(a)*radius*std::cos(tilt)};};
                float a=k*6.2831853f/64;
                line(point(a),point(a+.075f),.42f,{.12f,.16f,.22f},glm::mix(vec3{.18f,.32f,.6f},vec3{.95f,.06f,.22f},heat)*.45f);
            }
        }
    }
    for(const auto& m:successorHazards.marks){vec3 col=m.kind==2?vec3{.75f,.35f,1.3f}:vec3{1.5f,.25f,.2f};
        if(m.kind==1)line(m.a+vec3{0,.08f,0},m.b+vec3{0,.08f,0},m.radius*2,col*.4f,col);
        else ring(m.a,m.radius,col);
    }
    for(const auto& e:enemies){if(!e.alive)continue;
        if(e.type==EnemyType::SUCCESSOR){
            if(e.telegraphTimer>0){
                auto color=SuccessorState::white(e.attack)?vec3{2}:e.attack==AttackKind::SU_VECTOR||e.attack==AttackKind::SU_INCISION||e.attack==AttackKind::SU_REAP?abGoldTell(vec3{1.5f,1,.15f}):vec3{1.5f,.2f,.25f};
                if(e.attack==AttackKind::SU_VECTOR){
                    auto d=e.suAim-e.suFrom;d.y=0;float n=glm::length(d);if(n>.01f){vec3 side{-d.z/n,0,d.x/n};
                        for(float sign:{-1.f,1.f})line(e.suFrom+side*1.55f*sign+vec3{0,.06f,0},e.suAim+side*1.55f*sign+vec3{0,.06f,0},.09f,color*.3f,color);}
                }
                else if(e.attack==AttackKind::SU_REAP)ring(e.position,9.f,color);
                else if(e.attack==AttackKind::SU_BREAKER){ring(e.suAim,4.8f,color);line(e.suAim+vec3{0,.1f,0},e.suAim+vec3{0,4,0},.09f,color*.3f,color);}
                else if(e.attack==AttackKind::SU_RECALL)ring(e.suAim,2.8f,color);
                else if(e.attack==AttackKind::SU_LATTICE){
                    auto d=e.suAim-e.position;d.y=0;float n=glm::length(d);if(n>.01f){d/=n;vec3 side{-d.z,0,d.x};auto at=e.suAim;at.y=e.floorY;
                        float u=e.telegraphProgress();for(float o:{-7.f,0.f,7.f})line(at+side*o-d*8.f*u+vec3{0,.06f,0},at+side*o+d*8.f*u+vec3{0,.06f,0},.07f,color*.3f,color);}
                }
                else if(e.attack==AttackKind::SU_CASCADE){
                    auto d=e.suAim-e.position;d.y=0;float n=glm::length(d);if(n>.01f){d/=n;auto a=e.position;a.y=e.floorY+.06f;
                        line(a+d*3.f,a+d*(3.f+38.f*e.telegraphProgress()),.16f,color*.3f,color);}
                    ring(e.position,2.4f,color*.8f);
                }
                else if(e.attack==AttackKind::SU_SEVER){
                    float r=3.2f+e.telegraphProgress()*1.5f;
                    for(int k=0;k<16;++k){float a=e.yaw-.65f+k*.081f,b=a+.081f;line(e.position+vec3{std::sin(a)*r,2.4f,std::cos(a)*r},e.position+vec3{std::sin(b)*r,2.4f,std::cos(b)*r},.08f,color*.3f,color);}
                }
                else{ring(e.position,1.2f,color*.6f);line(e.position+vec3{.6f,2,.2f},e.position+vec3{.6f,3.1f,.2f},.09f,color,color);}
            }
            if(e.suAir>0)ring(e.suAim,4.8f,{1.8f,.1f,.22f});
            if(e.suCross>0){ring(e.suExit,2.f,{.4f,.85f,1.4f});line(e.suFrom+vec3{0,1,0},e.suExit+vec3{0,1,0},.055f,{.3f,.5f,.7f},{.5f,1,1.5f});}
            if(e.suNodes>0)for(int k=0;k<2;++k){line(e.position+vec3{0,2.7f,0},e.suAnchor[k],.065f,{.6f,.3f,.35f},{.8f,.4f,.6f});r.box(T(e.suAnchor[k])*RY(gameClock),{0,0,0},{.7f,.7f,.7f},{.5f,.35f,.4f},{1.1f,.65f,.9f});}
            if(e.suBurst>0 || e.suCross>0){
                for(int k=1;k<=2;++k){Enemy ghost=e;
                    ghost.position=e.suBurst>0?e.position-e.suDir*(k*2.5f):glm::mix(e.suFrom,e.position,1.f-k*.18f);
                    ghost.spawnTimer=0;ghost.finishGlow=ghost.hitFlashTimer=0;size_t first=out.size();buildEnemy(ghost,gameClock,out);
                    for(size_t n=first;n<out.size();++n){out[n].color={.035f,.008f,.018f};out[n].emissive=vec3{.85f,.015f,.07f}*(.5f/k);}
                }
            }
            if(e.suSince<.18f && (e.suLanded==AttackKind::SU_INCISION || e.suLanded==AttackKind::SU_THRUST)){
                float fade=1-e.suSince/.18f;
                if(e.suLanded==AttackKind::SU_THRUST)line(e.position+vec3{0,2.2f,0},e.position+vec3{std::sin(e.yaw)*5,2.2f,std::cos(e.yaw)*5},.09f,{.3f,.05f,.08f},vec3{1.3f,.12f,.2f}*fade);
                else for(int k=0;k<14;++k){float a=e.yaw-1.1f+k*.14f,b=a+.14f;line(e.position+vec3{std::sin(a)*4,2.3f,std::cos(a)*4},e.position+vec3{std::sin(b)*4,2.3f,std::cos(b)*4},.07f,{.3f,.05f,.08f},vec3{1.3f,.12f,.2f}*fade);}
            }
        }
    }
}
