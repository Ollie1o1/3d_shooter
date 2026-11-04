#pragma once
inline void Enemy::thinkLab(float dt,const EnemyWorld& w,bool resolve){
    labOpen=std::max(0.f,labOpen-dt);labSwipe=std::max(0.f,labSwipe-dt);
    auto to=flatTo(w.playerFeet);float distance=glm::length(to);auto dir=norm2(to);
    auto side=glm::vec3{-dir.z,0,dir.x}*((uid%2)?1.f:-1.f);
    if(type==EnemyType::SUSPENDED){
        animPhase+=dt*2.f;turnToward(to,dt,2.5f);
        if(labBeam>0){
            labBeam=std::max(0.f,labBeam-dt);velocity={0,0,0};
            auto ray=labBeamDirection();float length=BEAM_RANGE;
            for(int i=0;i<w.wallCount;++i){float hit=rayBoxHit(eyePos(),ray,w.walls[i].box);if(hit>.01f)length=std::min(length,hit);}
            beamEnd=eyePos()+ray*length;ev.beamOn=true;ev.beamFrom=eyePos();ev.beamTo=beamEnd;return;
        }
        if(resolve && attack==AttackKind::LAB_BEAM){
            auto aim=labAim-eyePos();float n=glm::length(aim);
            labDir=n>.01f?aim/n:glm::vec3{0,0,1};labBeam=BEAM_TIME;labLocked=true;
            attack=AttackKind::NONE;labOpen=1.45f;recover(1.5f);velocity={0,0,0};return;
        }
        if(telegraphTimer>0){
            // Track during the first half, then visibly commit before firing.
            if(!labLocked){labAim=w.playerEye;if(telegraphTimer<=.45f)labLocked=true;}
            velocity={0,0,0};return;
        }
        rangedMove(dt,w,13,23,stats().speed*(recoverTimer>0?1.f:.8f));
        velocity.y=glm::clamp(floorY+hoverY+std::sin(age*.7f)*.6f-position.y,-2.f,2.f);
        if(recoverTimer<=0 && attackReady(dt)){
            if(lineOfSight(eyePos(),w)){labAim=w.playerEye;labLocked=false;startAttack(AttackKind::LAB_BEAM,1.1f);}
            else blockedShot();
        }
        return;
    }
    if(type==EnemyType::PHASED && labCross>0){
        labCross=std::max(0.f,labCross-dt);velocity={0,0,0};
        if(labCross<=0){
            // Recheck: a door or platform may have moved since the exit appeared.
            if(blockedAt(labExit,w) || !supportedAt(labExit,w)){attack=AttackKind::NONE;labOpen=1;recover(.9f);return;}
            ev.blinked=true;ev.blinkFrom=position;position=labExit;floorY=position.y;
            labOpen=1.15f;labAim=w.playerFeet;startAttack(AttackKind::LAB_LUNGE,.7f);
        }
        return;
    }
    if(dashTimer>0){
        auto from=position;auto next=from+labDir*(22.f*tune_->moveSpeed*dt);
        if(!canStepTo(next,w)){
            dashTimer=0;velocity={0,0,0};labOpen=1.2f;recover(1.f);return;
        }
        dashTimer=std::max(0.f,dashTimer-dt);velocity=labDir*(22.f*tune_->moveSpeed);
        AABB player{w.playerFeet-glm::vec3{.5f,0,.5f},w.playerFeet+glm::vec3{.5f,1.8f,.5f}};
        if(!dashHit && segmentHitsBox(from+glm::vec3{0,1,0},next+glm::vec3{0,1,0},player)){
            ev.meleeHit=true;ev.meleeDamage=18;dashHit=true;
        }
        if(dashTimer<=0){velocity={0,0,0};labOpen=1.1f;recover(.95f);}return;
    }
    if(telegraphTimer>0){
        velocity={0,0,0};
        // A swipe keeps its eyes on you until the last instant, then commits
        if(attack==AttackKind::LAB_SWIPE && telegraphTimer>.2f)turnToward(to,dt,7.f);
        return;
    }
    if(resolve){
        auto kind=attack;attack=AttackKind::NONE;
        if(kind==AttackKind::LAB_CROSS){labCross=.35f;return;}
        if(kind==AttackKind::LAB_LUNGE){
            dashTimer=.38f;labDir=norm2(labAim-position);dashHit=false;ev.dashStarted=true;return;
        }
        if(kind==AttackKind::LAB_SWIPE){
            // Claws across whatever is in front of it: short reach, a narrow arc
            labSwipe=.32f;++labSwipes;
            glm::vec3 facing{std::sin(yaw),0,std::cos(yaw)};
            if(distance<3.5f && std::fabs(w.playerFeet.y-position.y)<2.f && glm::dot(facing,dir)>.35f){
                ev.meleeHit=true;ev.meleeDamage=9;
            }
            recover(.6f);return;
        }
        if(kind==AttackKind::LAB_PRESS){ev.slam=true;ev.slamRadius=6.f;ev.slamDamage=24;labOpen=1.6f;recover(1.4f);return;}
        if(kind==AttackKind::VOLLEY){
            fireAt(labAim,1,0,14,20,1.2f);ev.shotHeavy=true;ev.shotParry=230;
            labOpen=1.3f;recover(1.2f);return;
        }
    }
    turnToward(to,dt,type==EnemyType::GRAFTED?1.4f:5.f);
    if(recoverTimer>0){setMove(type==EnemyType::GRAFTED?side*.2f:-dir*.45f+side*.65f,stats().speed*.4f,w);return;}
    if(type==EnemyType::GRAFTED){
        // The heavy trudges after you, relentless but never quick
        setMove(distance>4.5f?dir:side*.4f,stats().speed,w);
    }else{
        // The swarm: a weaving sprint from range; up close each claims its own
        // side of you (a golden-angle slot) and prowls there, edging in, so a
        // group closes round you like a ring rather than queueing in a line
        float slot=uid*2.3999632f;
        glm::vec3 ring{std::sin(slot),0,std::cos(slot)},approach;float speed;
        if(distance>8.f){
            approach=norm2(to+side*(std::sin(age*2.1f+uid)*std::min(5.f,distance*.3f))+machineryLead(w.playerVel)*.2f);
            speed=stats().speed*1.1f;
        }else{
            glm::vec3 spot=w.playerFeet+ring*3.f-position;spot.y=0;
            float off=glm::length(spot);
            if(off>1.2f)approach=norm2(spot+dir*std::max(0.f,distance-3.f));
            else approach=norm2(side*.7f+dir*(distance>3.2f?.5f:-.25f));
            speed=stats().speed*(off>1.2f?.9f:.42f);
        }
        setMove(approach,speed,w);
    }
    if(attackReady(dt)){
        labAim=w.playerFeet+machineryLead(leadVel_)*.18f;
        if(type==EnemyType::PHASED && distance>7){
            // Pick a visible side/front exit with a whole body and real support.
            // No unseen rear teleport and no exit hanging over a pit.
            bool found=false;auto toward=norm2(w.playerAim);
            if(glm::length(toward)<.5f)toward=norm2(position-w.playerFeet);
            for(float angle:{1.15f,-1.15f,.65f,-.65f,0.f}){
                auto candidate=w.playerFeet+rotY(toward,angle*((uid%2)?1.f:-1.f))*8.f;
                candidate.y=w.playerFeet.y;
                if(candidate.x<w.bounds.min.x+2 || candidate.x>w.bounds.max.x-2 ||
                   candidate.z<w.bounds.min.z+2 || candidate.z>w.bounds.max.z-2)continue;
                if(!blockedAt(candidate,w) && supportedAt(candidate,w)){labExit=candidate;found=true;break;}
            }
            if(found){labFrom=position;startAttack(AttackKind::LAB_CROSS,.85f);}
            else blockedShot();
        }else if(type==EnemyType::GRAFTED){
            if(distance<8 && std::fabs(w.playerFeet.y-position.y)<3)startAttack(AttackKind::LAB_PRESS,.95f);
            else if(lineOfSight(eyePos(),w))startAttack(AttackKind::VOLLEY,.9f);
            else blockedShot();
        }else if(distance<3.4f && std::fabs(w.playerFeet.y-position.y)<2.f)
            startAttack(AttackKind::LAB_SWIPE,.5f);
        else if(distance<15 && std::fabs(w.playerFeet.y-position.y)<2.5f && lineOfSight(eyePos(),w))
            startAttack(AttackKind::LAB_LUNGE,.7f);
    }
}
