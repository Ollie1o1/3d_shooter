#pragma once
#include "bosses/StoryBossTools.h"
inline void Enemy::thinkSuccessor(float dt,const EnemyWorld& w,bool resolve){
    suOpen=std::max(0.f,suOpen-dt);suNodes=std::max(0.f,suNodes-dt);suEchoCool=std::max(0.f,suEchoCool-dt);suSince+=dt;
    int phase=health>maxHealth*.7f?1:health>maxHealth*.35f?2:3;
    if(phase!=suPhase){
        suPhase=phase;suPause=phase==1?1.2f:.65f;suOpen=0;suBurst=suCross=suNodes=suAir=0;suStep=suDashes=suVolley=0;
        attack=AttackKind::NONE;telegraphTimer=recoverTimer=0;velocity={0,0,0};++suEpoch;resolve=false;
        suCentre={(w.bounds.min.x+w.bounds.max.x)*.5f,w.bounds.min.y,(w.bounds.min.z+w.bounds.max.z)*.5f};
        ev.suPhaseStarted=phase;ev.suLine=phase==1?0:phase==2?3:4;
    }
    auto to=flatTo(w.playerFeet);float distance=glm::length(to);auto dir=norm2(to);
    auto move=[&](glm::vec3 direction,float speed){
        auto before=position;auto previousVelocity=velocity;float previousPhase=animPhase;
        setMove(direction,speed,w);auto desired=velocity;
        float blend=1-std::exp(-dt*22.f);
        velocity.x=glm::mix(previousVelocity.x,desired.x,blend);velocity.z=glm::mix(previousVelocity.z,desired.z,blend);
        if(!canStepTo(position+velocity*dt,w)){velocity.x=desired.x;velocity.z=desired.z;
            if(!canStepTo(position+velocity*dt,w))velocity.x=velocity.z=0;}
        integrate(dt,w);
        suGait=glm::mix(suGait,glm::length(glm::vec2{position.x-before.x,position.z-before.z})/std::max(dt,.001f),std::min(1.f,dt*14));
        animPhase=previousPhase+glm::length(glm::vec2{position.x-before.x,position.z-before.z})*1.65f;
    };
    auto target=[&](float lead){return machineryClamp(w.playerFeet+machineryLead(w.playerVel)*lead,w.bounds,3);};
    auto vectorAim=[&](){
        suFrom=position;auto aim=target(.25f+std::min(.65f,distance/52.f));
        suAim=machineryClamp(position+norm2(aim-position)*std::clamp(distance+6.f,16.f,56.f),w.bounds,2.5f);suAim.y=position.y;suLocked=false;
    };
    auto landAt=[&](glm::vec3 p){p.y=w.bounds.min.y;for(int i=0;i<w.wallCount;++i){const auto& b=w.walls[i].box;
        if(p.x>=b.min.x && p.x<=b.max.x && p.z>=b.min.z && p.z<=b.max.z && b.max.y<=w.playerFeet.y+.6f)p.y=std::max(p.y,b.max.y);}
        if(!supportedAt(p,w)){p=machineryClamp(w.playerFeet,w.bounds,3);if(!supportedAt(p,w))p=position;}return p;};
    auto open=[&](float t){suStep=0;suLocked=false;suOpen=t;recoverTimer=std::max(.38f,t*.55f);attackTimer=stats().attackEvery;velocity={0,0,0};};
    if(suPause>0){suPause=std::max(0.f,suPause-dt);velocity={0,0,0};suGait*=std::exp(-dt*12);integrate(dt,w);return;}
    if(suVolley>0){
        suVolleyT-=dt;if(suVolleyT<=0){--suVolley;suVolleyT=.22f;fireAt(w.playerEye,3,.5f,26,12,.65f);}
    }
    if(suCross>0){
        suCross=std::max(0.f,suCross-dt);float t=1-suCross/CROSS_TIME;
        position=glm::mix(suFrom,suExit,t);velocity={0,0,0};suGait=0;
        if(suCross<=0){floorY=suExit.y;suAim=target(.12f);suLocked=false;startAttack(AttackKind::SU_TRANSPOSE,.44f,true);suMove=3;}
        return;
    }
    if(suAir>0){
        suAir=std::max(0.f,suAir-dt);float t=1-suAir/BREAKER_TIME;
        position=glm::mix(suFrom,suAim,t)+glm::vec3{0,5.5f*4*t*(1-t),0};velocity={0,0,0};suGait=0;
        if(suAir<=0){position=suAim;floorY=suAim.y;suLanded=AttackKind::SU_BREAKER;suSince=0;
            ev.suBreaker=ev.suImpact=true;ev.suAt=suAim;open(1.05f);}
        return;
    }
    if(suBurst>0){
        auto was=position;suBurst=std::max(0.f,suBurst-dt);velocity=suDir*((suPhase==3?57.f:50.f)*tune_->moveSpeed);
        if(!canStepTo(position+velocity*dt,w)){suBurst=0;velocity={0,0,0};}
        integrate(dt,w);
        suGait=32;animPhase+=dt*20;
        if(!suContact && machinerySweepHits(w.playerFeet,was,position,1.55f,position.y,3.4f)){ev.meleeHit=true;ev.meleeDamage=30;suContact=true;}
        if(suBurst<=0 || glm::length(position-was)<dt*2){
            suBurst=0;velocity={0,0,0};
            if(suDashes>0){--suDashes;vectorAim();startAttack(AttackKind::SU_VECTOR,suPhase==3?.38f:.44f,true);}
            else if(distance<11){suStep=2;suAim=target(.12f);suLocked=false;startAttack(AttackKind::SU_THRUST,.36f,true);}
            else open(.95f);
        }
        return;
    }
    if(!suLocked)turnToward(to,dt,9.f);
    if(telegraphTimer>0){
        bool melee=attack==AttackKind::SU_INCISION || attack==AttackKind::SU_THRUST || attack==AttackKind::SU_TRANSPOSE;
        if(!suLocked && telegraphTimer>TRACK_LOCK){
            if(attack==AttackKind::SU_VECTOR)vectorAim();
            else if(attack!=AttackKind::SU_RECALL && attack!=AttackKind::SU_SUTURE){suAim=target(attack==AttackKind::SU_BREAKER?.25f:.13f);if(attack==AttackKind::SU_BREAKER)suAim=landAt(suAim);}
        }else if(!suLocked){suLocked=true;ev.suLock=true;}
        // A pursuing guard plants at the lock, then drives the committed strike. No last-frame
        // aim snapping: the marked target and facing then remain committed.
        if(melee && suLocked && telegraphTimer<.16f){
            auto strike=norm2(suAim-position);float gap=glm::length(glm::vec2{suAim.x-position.x,suAim.z-position.z});
            float speed=std::min(24.f,std::max(0.f,gap-2.5f)/std::max(dt,.001f));
            if(canStepTo(position+strike*speed*dt,w))move(strike,speed);else{velocity={0,0,0};integrate(dt,w);}
        }else if(melee && !suLocked && distance>4)move(dir,suPhase==3?13.f:11.f);
        else{velocity={0,0,0};suGait*=std::exp(-dt*14);integrate(dt,w);}
        return;
    }
    if(resolve){
        auto kind=attack;attack=AttackKind::NONE;suLanded=kind;suLandedStep=suStep;suSince=0;ev.suImpact=true;ev.suAt=suAim;
        if(kind==AttackKind::SU_VECTOR){suFrom=position;suDir=norm2(suAim-position);suBurst=std::min(1.2f,glm::length(suAim-position)/((suPhase==3?57.f:50.f)*tune_->moveSpeed));suContact=false;return;}
        if(kind==AttackKind::SU_INCISION || kind==AttackKind::SU_THRUST || kind==AttackKind::SU_TRANSPOSE){
            auto from=position;auto strike=norm2(suAim-position);
            auto end=position+strike*(kind==AttackKind::SU_INCISION?2.6f:3.4f);
            if(machinerySweepHits(w.playerFeet,from,end,kind==AttackKind::SU_INCISION?1.45f:1.25f,position.y,3.5f)){
                ev.meleeHit=true;ev.meleeDamage=kind==AttackKind::SU_INCISION?18:26;
            }
            if(kind==AttackKind::SU_INCISION && suStep<2){++suStep;suLocked=false;suAim=target(.12f);
                startAttack(suStep==2?AttackKind::SU_THRUST:AttackKind::SU_INCISION,suStep==2?.36f:.30f,true);return;}
            if(suPhase==3 && suEchoCool<=0){ev.suEcho=true;ev.suAt=suAim;suEchoCool=3.f;}
            open(suPhase==3?.65f:.8f);
        }else if(kind==AttackKind::SU_REAP){
            ev.suReap=ev.slam=true;ev.slamRadius=9.f;ev.slamDamage=27;open(.95f);
        }else if(kind==AttackKind::SU_BREAKER){
            suFrom=position;suAir=BREAKER_TIME;ev.suImpact=false;suLocked=true;return;
        }else if(kind==AttackKind::SU_SUTURE){
            float half=std::min(36.f,(w.bounds.max.x-w.bounds.min.x)*.32f);
            suAnchor[0]=suCentre+glm::vec3{-half,2,-16};suAnchor[1]=suCentre+glm::vec3{half,2,16};suNodeHp[0]=suNodeHp[1]=35;suNodes=2.3f;
            ev.suSuture=true;ev.suAt=suAnchor[0];ev.suTo=suAnchor[1];suOpen=.8f;recover(.75f);
        }else if(kind==AttackKind::SU_RECALL){ev.suRecall=true;ev.suAt=suAim;open(.8f);
        }else if(kind==AttackKind::SU_PERFUSION){fireAt(w.playerEye,5,.72f,26,12,.7f);ev.suCore=true;
            suVolley=suPhase>=2?2:0;suVolleyT=.22f;open(1.f);}
        suLocked=false;integrate(dt,w);return;
    }
    if(recoverTimer>0){move(dir*(distance>8?1.f:-.15f)+glm::vec3{-dir.z,0,dir.x}*.65f,suPhase==3?13.f:11.f);return;}
    move(distance>5?dir+glm::vec3{-dir.z,0,dir.x}*.23f:glm::vec3{-dir.z,0,dir.x}*.65f,suPhase==3?26.f:22.f);
    if(!attackReady(dt))return;
    static const int first[]={0,1,5,7,1,0,4},second[]={0,1,8,2,3,7,5,4},last[]={1,8,0,2,7,3,5,8,4};
    int moveId=suPhase==1?first[suRotation++%7]:suPhase==2?second[suRotation++%8]:last[suRotation++%9];
    if(health<maxHealth*.12f && !suFinal){suFinal=true;suFinalStep=0;ev.suLine=6;}
    if(suFinalStep>=0){static const int finish[]={1,8,3,7};moveId=finish[suFinalStep++];if(suFinalStep>=4)suFinalStep=-1;}
    if(std::fabs(w.playerFeet.y-position.y)>3)moveId=3;
    else if(distance>42)moveId=suPhase>=2?3:1;
    else if(distance>18 && moveId!=5 && moveId!=2)moveId=1;
    else if(moveId==0 && distance>12)moveId=1;
    suAim=target(.15f);
    // A low gallery cannot accommodate the full airborne silhouette. Cross
    // into that space instead of drawing the body through its solid slab.
    if(moveId==8 && (blockedAt(position+glm::vec3{0,5.5f,0},w) || blockedAt(landAt(suAim)+glm::vec3{0,5.5f,0},w)))moveId=3;
    suMove=moveId;suFrom=position;suLocked=false;suStep=0;
    if(moveId==0)startAttack(AttackKind::SU_INCISION,.44f);
    else if(moveId==1){suDashes=suPhase>=2?1:0;vectorAim();startAttack(AttackKind::SU_VECTOR,.58f);}
    else if(moveId==2)startAttack(AttackKind::SU_SUTURE,.85f);
    else if(moveId==3){
        auto side=glm::vec3{-dir.z,0,dir.x};suExit=target(.25f)+side*3.8f;suExit=machineryClamp(suExit,w.bounds,3);suExit.y=w.playerFeet.y;
        if(blockedAt(suExit,w) || !supportedAt(suExit,w)){suExit=target(.15f)-dir*3.5f;suExit.y=w.playerFeet.y;}
        if(blockedAt(suExit,w) || !supportedAt(suExit,w)){suExit=target(.15f);suExit.y=w.bounds.min.y;}
        if(blockedAt(suExit,w) || !supportedAt(suExit,w)){suExit=position;suExit.y=floorY;}
        // The silhouette is announced before the rapid crossing begins.
        suCross=CROSS_TIME;suFrom=position;ev.suMark=true;ev.suAt=suExit;suLocked=true;
    }else if(moveId==4){suAim=target(.05f);suLocked=true;startAttack(AttackKind::SU_RECALL,.75f);}
    else if(moveId==7)startAttack(AttackKind::SU_REAP,.68f);
    else if(moveId==8){
        suAim=landAt(target(.25f));
        if(blockedAt(suAim,w) || !supportedAt(suAim,w))suAim.y=w.bounds.min.y;
        startAttack(AttackKind::SU_BREAKER,.72f);
    }else startAttack(AttackKind::SU_PERFUSION,.8f);
}
