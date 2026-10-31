#pragma once
#include "bosses/StoryBossTools.h"
inline void Enemy::thinkSuccessor(float dt,const EnemyWorld& w,bool resolve){
    suOpen=std::max(0.f,suOpen-dt);suNodes=std::max(0.f,suNodes-dt);suEchoCool=std::max(0.f,suEchoCool-dt);
    animPhase+=dt*(2.f+moveSpeed*.9f);
    int phase=health>maxHealth*.65f?1:health>maxHealth*.3f?2:3;
    if(phase!=suPhase){
        suPhase=phase;suPause=phase==1?5.5f:1.1f;suOpen=0;suBurst=suCross=suNodes=0;suStep=0;
        attack=AttackKind::NONE;telegraphTimer=0;velocity={0,0,0};++suEpoch;resolve=false;
        suCentre={(w.bounds.min.x+w.bounds.max.x)*.5f,w.bounds.min.y,(w.bounds.min.z+w.bounds.max.z)*.5f};
        ev.suPhaseStarted=phase;ev.suLine=phase==1?0:phase==2?3:4;
    }
    if(suPause>0){suPause-=dt;velocity={0,0,0};integrate(dt,w);return;}
    auto to=flatTo(w.playerFeet);float distance=glm::length(to);auto dir=norm2(to);
    if(suCross>0){
        suCross-=dt;float t=1-std::max(0.f,suCross)/.24f;position=glm::mix(suFrom,suExit,t);velocity={0,0,0};
        if(suCross<=0){floorY=suExit.y;suAim=w.playerFeet;startAttack(AttackKind::SU_TRANSPOSE,.48f,true);suMove=3;suLocked=true;}
        return;
    }
    if(suBurst>0){
        auto was=position;suBurst-=dt;velocity=suDir*(suPhase==3?37.f:34.f);integrate(dt,w);
        if(!suContact && machinerySweepHits(w.playerFeet,was,position,1.35f,position.y,2.5f)){ev.meleeHit=true;ev.meleeDamage=26;suContact=true;}
        if(suBurst<=0||glm::length(position-was)<dt*2){suBurst=0;suOpen=1.f;recover(.85f);velocity={0,0,0};}
        return;
    }
    if(!suLocked)turnToward(to,dt,6.f);
    if(telegraphTimer>0){
        velocity={0,0,0};
        if(!suLocked && telegraphTimer>.32f && attack!=AttackKind::SU_RECALL){
            float lead=attack==AttackKind::SU_VECTOR?.32f+std::min(.75f,distance/34.f):.18f;
            suAim=w.playerFeet+machineryLead(w.playerVel)*lead;
            if(attack==AttackKind::SU_VECTOR){suFrom=position;suAim=machineryClamp(position+norm2(suAim-position)*std::max(16.f,std::min(distance+7.f,36.f)),w.bounds,3);suAim.y=position.y;}
        }else if(!suLocked){suLocked=true;ev.suLock=true;}
        return;
    }
    if(resolve){
        auto kind=attack;attack=AttackKind::NONE;ev.suImpact=true;ev.suAt=suAim;
        if(kind==AttackKind::SU_VECTOR){suFrom=position;suDir=norm2(suAim-position);suBurst=std::min(.95f,glm::length(suAim-position)/34.f);suContact=false;suStep=0;return;}
        if(kind==AttackKind::SU_INCISION || kind==AttackKind::SU_THRUST || kind==AttackKind::SU_TRANSPOSE){
            float reach=kind==AttackKind::SU_INCISION?4.6f:5.2f;
            if(distance<reach && std::fabs(w.playerFeet.y-position.y)<2.7f && glm::dot(dir,glm::vec3{std::sin(yaw),0,std::cos(yaw)})>-.15f){ev.meleeHit=true;ev.meleeDamage=kind==AttackKind::SU_THRUST?26:18;}
            if(kind==AttackKind::SU_INCISION && suStep<2){++suStep;suLocked=false;suAim=w.playerFeet;startAttack(suStep==2?AttackKind::SU_THRUST:AttackKind::SU_INCISION,suStep==2?.46f:.42f,true);return;}
            if(suPhase==3 && suEchoCool<=0){ev.suEcho=true;ev.suAt=suAim;suEchoCool=4.f;}
            suStep=0;suOpen=1.15f;recover(.85f);
        }else if(kind==AttackKind::SU_SUTURE){
            suAnchor[0]=suCentre+glm::vec3{-26,2,-15};suAnchor[1]=suCentre+glm::vec3{26,2,15};suNodeHp[0]=suNodeHp[1]=35;suNodes=2.f;
            ev.suSuture=true;ev.suAt=suAnchor[0];ev.suTo=suAnchor[1];suOpen=.75f;recover(1.2f);
        }else if(kind==AttackKind::SU_RECALL){ev.suRecall=true;ev.suAt=suAim;suOpen=.85f;recover(.95f);
        }else if(kind==AttackKind::SU_PERFUSION){fireAt(w.playerEye,5,.7f,24,11,.7f);ev.suCore=true;suOpen=1.25f;recover(1.1f);}
        suLocked=false;integrate(dt,w);return;
    }
    if(recoverTimer>0){velocity=steer(dir,w)*2.5f;integrate(dt,w);return;}
    velocity=distance>3.8f?steer(dir,w)*(suPhase==3?14.f:12.5f):glm::vec3{0};
    integrate(dt,w);
    if(!attackReady(dt))return;
    int move=suRotation++%(suPhase==1?3:suPhase==2?5:7);
    if(health<maxHealth*.1f && !suFinal){suFinal=true;suFinalStep=0;ev.suLine=6;}
    if(suFinalStep>=0){static const int sequence[]={1,2,3};move=sequence[suFinalStep++];if(suFinalStep>=3)suFinalStep=-1;}
    if(w.playerFeet.y>floorY+3.f || distance>25.f)move=suPhase>=2?3:5;
    if(move==0 && distance>9.f)move=1;
    if(suPhase==1 && move==2)move=5;
    suMove=move;suAim=w.playerFeet;suFrom=position;suLocked=false;suStep=0;
    if(move==0)startAttack(AttackKind::SU_INCISION,.64f);
    else if(move==1)startAttack(AttackKind::SU_VECTOR,.8f);
    else if(move==2)startAttack(AttackKind::SU_SUTURE,.95f);
    else if(move==3){
        auto side=glm::vec3{-dir.z,0,dir.x};suExit=machineryClamp(w.playerFeet+machineryLead(w.playerVel)*.32f+side*3.2f,w.bounds,3);suExit.y=w.playerFeet.y;
        if(blockedAt(suExit,w))suExit=machineryClamp(w.playerFeet+machineryLead(w.playerVel)*.32f-dir*3.2f,w.bounds,3);
        if(blockedAt(suExit,w)){suExit=position;suExit.y=floorY;}
        suCross=.24f;suFrom=position;ev.suMark=true;ev.suAt=suExit;suLocked=true;
    }else if(move==4 || move==6){suAim=w.playerFeet;suLocked=true;startAttack(AttackKind::SU_RECALL,.85f);}
    else startAttack(AttackKind::SU_PERFUSION,.9f);
}
