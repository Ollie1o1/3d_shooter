#pragma once
inline void Enemy::thinkLab(float dt,const EnemyWorld& w,bool resolve){
    labOpen=std::max(0.f,labOpen-dt);animPhase+=dt*(2.f+moveSpeed);
    auto to=flatTo(w.playerFeet);float distance=glm::length(to);auto dir=norm2(to);
    if(type==EnemyType::SUSPENDED){
        turnToward(to,dt,2.5f);rangedMove(dt,w,12,22,stats().speed*.6f);
        velocity.y=glm::clamp(floorY+hoverY+std::sin(age*.7f)*.4f-position.y,-2.f,2.f);
        if(labBeam>0){labBeam-=dt;ev.beamOn=true;ev.beamFrom=eyePos();ev.beamTo=labAim;velocity.x=velocity.z=0;return;}
        if(resolve){labBeam=.85f;labOpen=1.4f;recover(1.5f);return;}
        if(telegraphTimer>0){velocity.x=velocity.z=0;return;}
        if(attackReady(dt)&&lineOfSight(eyePos(),w)){labAim=w.playerEye;startAttack(AttackKind::LAB_BEAM,1.05f);}
        return;
    }
    if(type==EnemyType::PHASED && labCross>0){
        labCross-=dt;velocity={0,0,0};
        if(labCross<=0){ev.blinked=true;ev.blinkFrom=position;position=labExit;floorY=position.y;labOpen=1.15f;startAttack(AttackKind::LAB_LUNGE,.7f);labAim=w.playerFeet;}
        return;
    }
    if(dashTimer>0){
        auto from=position;dashTimer-=dt;velocity=labDir*22.f;
        AABB player{w.playerFeet-glm::vec3{.5f,0,.5f},w.playerFeet+glm::vec3{.5f,1.8f,.5f}};
        if(!dashHit && segmentHitsBox(from+glm::vec3{0,1,0},from+velocity*dt+glm::vec3{0,1,0},player)){ev.meleeHit=true;ev.meleeDamage=18;dashHit=true;}
        if(dashTimer<=0){velocity={0,0,0};labOpen=.8f;recover(.8f);}return;
    }
    if(telegraphTimer>0){velocity={0,0,0};return;}
    if(resolve){
        auto kind=attack;attack=AttackKind::NONE;
        if(kind==AttackKind::LAB_CROSS){labCross=.45f;return;}
        if(kind==AttackKind::LAB_LUNGE){dashTimer=.38f;labDir=norm2(labAim-position);dashHit=false;return;}
        if(kind==AttackKind::LAB_PRESS){ev.slam=true;ev.slamRadius=6.f;ev.slamDamage=24;labOpen=1.5f;recover(1.3f);return;}
        if(kind==AttackKind::VOLLEY){fireAt(w.playerEye,3,.45f,19,12,.9f);labOpen=.8f;recover(.9f);return;}
    }
    turnToward(to,dt,type==EnemyType::GRAFTED?1.4f:4.f);
    velocity=distance>3.5f?steer(dir,w)*stats().speed:glm::vec3{0};
    if(recoverTimer>0){velocity*=.2f;return;}
    if(attackReady(dt)){
        labAim=w.playerFeet+leadVel_*.2f;
        if(type==EnemyType::PHASED && distance>7){
            labExit=w.playerFeet+glm::vec3{-dir.z,0,dir.x}*8.f;labExit.x=glm::clamp(labExit.x,w.bounds.min.x+2,w.bounds.max.x-2);labExit.z=glm::clamp(labExit.z,w.bounds.min.z+2,w.bounds.max.z-2);
            if(blockedAt(labExit,w))labExit=position;startAttack(AttackKind::LAB_CROSS,.8f);
        }else if(type==EnemyType::GRAFTED){if(distance<8)startAttack(AttackKind::LAB_PRESS,.95f);else if(lineOfSight(eyePos(),w))startAttack(AttackKind::VOLLEY,.85f);}
        else if(distance<15)startAttack(AttackKind::LAB_LUNGE,.7f);
    }
}
