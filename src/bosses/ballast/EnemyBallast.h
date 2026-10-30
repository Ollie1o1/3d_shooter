#pragma once
inline void Enemy::thinkBallast(float dt,const EnemyWorld& w,bool resolve) {
    for(float* timer:{&ballOpen,&ballRushCd,&ballWindCd,&ballWinchCd,&ballRipCd,&ballThrowCd,&ballSweepCd,&ballCableShow,&ballSwing})
        *timer=std::max(0.f,*timer-dt);
    ballRotor+=dt*(ballPhase>=2?9.f:5.f);
    int phase=health>maxHealth*.55f?1:health>maxHealth*.22f?2:3;
    if(phase>ballPhase) {
        ballPhase=phase;ballPause=phase==1?3.2f:1.2f;ballFollow=AttackKind::NONE;ballPasses=0;
        ballDrive=ballTether=0;ballVolley=0;telegraphTimer=0;attack=AttackKind::NONE;enraged=phase>1;ev.ballPhaseStarted=phase;
    }
    velocity=glm::vec3{0};
    if(ballPause>0){ballPause-=dt;return;}
    ballObservedVel=glm::mix(ballObservedVel,machineryLead(w.playerVel),1.f-std::exp(-dt*8.f));
    glm::vec3 lead=ballObservedVel;leadVel_=lead*tune_->lead;
    if(ballTether>0) {
        ballTether=std::max(0.f,ballTether-dt);ballCableAt=w.playerEye;
        if(glm::length(flatTo(w.playerFeet))>5&&lineOfSight(position+glm::vec3{0,3,0},w))ev.ballYank=true;
        else ballTether=0;
    }
    if(ballVolley>0) {
        ballVolleyT-=dt;
        if(ballVolleyT<=0){--ballVolley;fireAt(ballVolleyAim,ballPhase==3?5:3,.24f,34,16,1.4f);ev.shotHeavy=true;ev.shotParry=320;}
    }
    if(ballDrive>0) {
        glm::vec3 was=position;
        float distance=glm::length(glm::vec2{ballAim.x-position.x,ballAim.z-position.z});
        float speed=ballHaul?26.f:ballPhase==3?36.f:31.f;
        velocity=ballDir*std::min(speed,distance/std::max(dt,.001f));
        integrate(dt,w);ballDrive-=dt;yaw=std::atan2(ballDir.x,ballDir.z);
        float moved=glm::length(position-was);
        if(!ballHaul&&!ballContact&&machinerySweepHits(w.playerFeet,was,position,radius()+.7f,floorY,BALL_CUT_HEIGHT)) {
            ev.meleeHit=true;ev.meleeDamage=32;ballContact=true;
        }
        if(ballDrive<=0||distance<.4f||moved<dt*2.f) {
            ballDrive=0;ballOpen=1.5f;recover(ballPhase==1?1.2f:.9f);
            if(!ballHaul&&ballPhase>=2&&ballPasses<1){++ballPasses;ballFollow=AttackKind::BALL_GRADE;}
            else if(!ballHaul&&ballPhase==3)ballFollow=AttackKind::BALL_ANCHOR;
            ballHaul=false;
        }
        return;
    }
    const float distance=glm::length(flatTo(w.playerFeet));
    const bool high=w.playerFeet.y>floorY+3.f;
    auto approach=[&](float speed) {
        if(distance>11.f){velocity=steer(norm2(flatTo(w.playerFeet+lead*.25f)),w)*speed;integrate(dt,w);}
    };
    // Plant the chassis only for committed heavy tools. Other preparations advance.
    if(!(telegraphTimer>0&&ballLocked))turnToward(flatTo(w.playerFeet),dt,4.2f);
    if(telegraphTimer>0) {
        if(telegraphTimer>.55f) {
            if(attack==AttackKind::BALL_ANCHOR)ballAim=w.playerFeet+lead*.30f;
            if(attack==AttackKind::BALL_WINCH)ballAim=w.playerEye+lead*.30f;
            if(attack==AttackKind::BALL_RIP)ballAim=w.playerFeet+lead*glm::clamp(distance/24.f+.45f,.6f,2.f);
            if(attack==AttackKind::BALL_GRADE) {
                approach(6.5f);ballFrom=position;
                glm::vec3 dir=flatTo(w.playerFeet+lead*glm::clamp(distance/31.f+.45f,.6f,1.6f));float n=glm::length(dir);
                if(n>.01f)ballAim=machineryClamp(position+dir/n*std::min(62.f,std::max(18.f,n+10.f)),w.bounds,4.f);
                ballAim.y=position.y;
            } else if(attack==AttackKind::BALL_THROW||attack==AttackKind::BALL_WINCH)approach(6.5f);
            ballSwingDir=norm2(flatTo(w.playerFeet));
        } else if(!ballLocked){ballLocked=true;ev.ballLocked=true;if(attack==AttackKind::BALL_SWEEP)ballSwingDir={std::sin(yaw),0,std::cos(yaw)};}
        if(attack==AttackKind::BALL_SWEEP&&telegraphTimer<=.30f&&!ballContact) {
            glm::vec3 to=flatTo(w.playerFeet);float n=glm::length(to);
            float angle=n>.01f?std::atan2(ballSwingDir.z*to.x-ballSwingDir.x*to.z,glm::dot(ballSwingDir,to)):0;
            float now=-1.05f+2.1f*(1-telegraphTimer/.30f),before=-1.05f+2.1f*(1-(telegraphTimer+dt)/.30f);
            if(n<6.2f&&angle>=before-.2f&&angle<=now+.2f&&w.playerFeet.y<floorY+1.7f) {
                ev.meleeHit=true;ev.meleeDamage=28;ballContact=true;
            }
        }
        return;
    }
    if(resolve) {
        AttackKind k=attack;attack=AttackKind::NONE;
        if(k==AttackKind::BALL_GRADE) {
            ballFrom=position;glm::vec3 d=ballAim-position;d.y=0;float n=glm::length(d);
            ballDir=n>.01f?d/n:glm::vec3{0,0,1};ballDrive=std::min(2.1f,n/31.f+.15f);ballContact=false;ballHaul=false;
            ev.ballGrade=true;ev.ballFrom=position;ev.ballTo=ballAim;
        } else if(k==AttackKind::BALL_GUST) {
            ev.ballGust=true;ev.ballFrom=ballFrom;ev.ballTo=ballAim;
            ballOpen=1.4f;recover(.9f);
            if(ballPhase>=2){ballFollow=AttackKind::BALL_GRADE;ballPasses=0;recoverTimer=.6f;}
        } else if(k==AttackKind::BALL_ANCHOR) {
            ev.ballAnchor=true;ev.ballTo=ballAim;ballOpen=1.4f;
            if(glm::length(glm::vec2{w.playerFeet.x-ballAim.x,w.playerFeet.z-ballAim.z})<BALL_ANCHOR_R&&std::fabs(w.playerFeet.y-ballAim.y)<2.8f) {
                ev.meleeHit=true;ev.meleeDamage=30;
            }
            recover(.9f);
        } else if(k==AttackKind::BALL_WINCH) {
            glm::vec3 from=position+glm::vec3{-1.5f,4.f,1.f};
            AABB body{w.playerFeet-glm::vec3{.45f,0,.45f},w.playerFeet+glm::vec3{.45f,1.85f,.45f}};
            bool blocked=false;glm::vec3 delta=ballAim-from;float n=glm::length(delta);
            if(n>.01f)for(int i=0;i<w.wallCount;++i){float t=rayBoxHit(from,delta/n,w.walls[i].box);if(t>.1f&&t<n-.5f){blocked=true;break;}}
            ballCableAt=ballAim;ballCableShow=.75f;ev.ballWinch=true;
            if(!blocked&&segmentHitsBox(from,ballAim,body)) {
                ballTether=.65f;ev.meleeHit=true;ev.meleeDamage=12;ev.ballYank=true;recover(1.1f);
            } else {
                ballTether=0;ballAim=machineryClamp(ballAim,w.bounds,5.f);ballAim.y=position.y;
                glm::vec3 d=flatTo(ballAim);float length=glm::length(d);ballDir=length>.01f?d/length:glm::vec3{0,0,1};
                ballFrom=position;ballHaul=true;ballDrive=std::min(1.f,length/26.f);recover(.9f);
            }
        } else if(k==AttackKind::BALL_RIP) {
            ev.ballRip=true;ev.ballFrom=position;ev.ballTo=ballAim;ballOpen=1.4f;recover(1.f);ballWindCd=std::max(ballWindCd,4.f);
        } else if(k==AttackKind::BALL_SWEEP) {
            ev.ballSweep=true;ballSwing=.35f;ballOpen=1.2f;recover(.95f);
        } else if(k==AttackKind::BALL_THROW) {
            ballVolleyAim=w.playerEye+lead*.35f;ballVolleyAim.y+=glm::clamp(w.playerVel.y*.15f,-1.5f,1.5f);
            fireAt(ballVolleyAim,ballPhase==3?5:3,.22f,32,17,1.6f);ev.shotHeavy=true;ev.shotParry=320;
            ballVolley=ballPhase>=2?1:0;ballVolleyT=.30f;recover(.85f);
        }
        return;
    }
    if(recoverTimer>0){if(recoverTimer<.4f)approach(8);return;}
    approach(ballPhase==3?12.f:10.f);
    bool follow=ballFollow!=AttackKind::NONE;
    if(!follow&&!attackReady(dt))return;
    AttackKind k=ballFollow;ballFollow=AttackKind::NONE;
    if(k==AttackKind::NONE) {
        ballPasses=0;
        if(high&&ballWinchCd<=0)k=AttackKind::BALL_WINCH;
        else if(high&&ballThrowCd<=0)k=AttackKind::BALL_THROW;
        else if(distance>19&&ballRushCd<=0)k=AttackKind::BALL_GRADE;
        else if(distance<7.2f&&!high&&ballSweepCd<=0)k=AttackKind::BALL_SWEEP;
        else if(ballPhase>=2&&ballRipCd<=0&&ballLast!=AttackKind::BALL_RIP)k=AttackKind::BALL_RIP;
        else if(ballWindCd<=0&&ballLast!=AttackKind::BALL_GUST)k=AttackKind::BALL_GUST;
        else if(distance>12&&ballWinchCd<=0)k=AttackKind::BALL_WINCH;
        else if(ballThrowCd<=0&&ballLast!=AttackKind::BALL_THROW)k=AttackKind::BALL_THROW;
        else k=AttackKind::BALL_ANCHOR;
    }
    if(high&&k==AttackKind::BALL_GRADE)k=AttackKind::BALL_ANCHOR;
    ++ballStep;ballLast=k;ballAim=w.playerFeet;ballFrom=position;ballLocked=false;ballContact=false;
    ballSwingDir=norm2(flatTo(w.playerFeet));
    if(k==AttackKind::BALL_GRADE){ballRushCd=5.f;glm::vec3 d=flatTo(w.playerFeet+lead*glm::clamp(distance/31.f+.45f,.6f,1.6f));float n=glm::length(d);if(n>.01f)ballAim=machineryClamp(position+d/n*std::min(62.f,std::max(18.f,n+10)),w.bounds,4.f);ballAim.y=position.y;}
    if(k==AttackKind::BALL_ANCHOR)ballAim=w.playerFeet+lead*.3f;
    if(k==AttackKind::BALL_WINCH){ballAim=w.playerEye+lead*.3f;ballWinchCd=7;}
    if(k==AttackKind::BALL_RIP){ballAim=w.playerFeet+lead*glm::clamp(distance/24.f+.45f,.6f,2.f);ballRipCd=7;}
    if(k==AttackKind::BALL_SWEEP)ballSweepCd=4;
    if(k==AttackKind::BALL_THROW)ballThrowCd=3.5f;
    ballReverse=k==AttackKind::BALL_GUST&&ballPhase==3&&ballStep%2!=0;
    if(k==AttackKind::BALL_GUST) {
        ballWindCd=7;
        if(ballReverse){glm::vec3 d=norm2(flatTo(ballAim));ballFrom=machineryClamp(position+d*std::min(65.f,distance+14.f),w.bounds,2.f);ballAim=position;}
    }
    float tell=k==AttackKind::BALL_GUST?1.35f:k==AttackKind::BALL_RIP?1.3f:k==AttackKind::BALL_GRADE?1.2f:k==AttackKind::BALL_WINCH?1.25f:1.1f;
    startAttack(k,tell*(ballPhase==1?1.f:ballPhase==2?.92f:.85f),follow);
}
