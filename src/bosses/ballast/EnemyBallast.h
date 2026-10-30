#pragma once
// Ballast uses its working tools. Every targeted attack stops tracking before
// release; its blade is low enough to jump and the blower respects real cover.
inline void Enemy::thinkBallast(float dt,const EnemyWorld& w,bool resolve) {
    ballOpen=std::max(0.f,ballOpen-dt);
    ballRotor+=dt*(ballPhase>=2?8.f:4.5f);
    int phase=health>maxHealth*.55f?1:health>maxHealth*.22f?2:3;
    if(phase>ballPhase) {
        ballPhase=phase;ballPause=phase==1?3.2f:1.2f;ballFollow=AttackKind::NONE;ev.ballPhaseStarted=phase;
        ballDrive=0;telegraphTimer=0;attack=AttackKind::NONE;enraged=phase>1;
    }
    velocity=glm::vec3{0};
    if(ballPause>0){ballPause-=dt;return;}
    if(ballDrive>0) {
        glm::vec3 was=position;
        float distance=glm::length(glm::vec2{ballAim.x-position.x,ballAim.z-position.z});
        float speed=ballPhase==3?32.f:26.f;
        velocity=ballDir*std::min(speed,distance/std::max(dt,.001f));
        integrate(dt,w);ballDrive-=dt;
        glm::vec3 delta=position-was;float n=glm::length(delta);
        float u=n>.001f?glm::clamp(glm::dot(w.playerFeet-was,delta)/(n*n),0.f,1.f):0;
        glm::vec3 near=was+delta*u;
        if(!ballContact && glm::length(glm::vec2{w.playerFeet.x-near.x,w.playerFeet.z-near.z})<radius()+.6f &&
           w.playerFeet.y<floorY+BALL_CUT_HEIGHT && w.playerFeet.y>floorY-.7f) {
            ev.meleeHit=true;ev.meleeDamage=34.f;ballContact=true;
        }
        if(ballDrive<=0 || distance<.3f || n<dt*2.f) {
            ballDrive=0;ballOpen=1.6f;recover(ballPhase==1?1.2f:.7f);
            if(ballPhase==3)ballFollow=AttackKind::BALL_ANCHOR;
        }
        return;
    }
    turnToward(flatTo(w.playerFeet),dt,3.5f);
    glm::vec3 lead=w.playerVel;lead.y=0;float speed=glm::length(lead);
    if(speed>10.f)lead*=10.f/speed;
    glm::vec3 anchorTarget=w.playerFeet+lead*.3f;
    if(telegraphTimer>0) {
        // The last 0.6 seconds always show the final landing point / lane.
        if(telegraphTimer>.6f && attack==AttackKind::BALL_ANCHOR)ballAim=anchorTarget;
        return;
    }
    if(resolve) {
        AttackKind k=attack;attack=AttackKind::NONE;
        if(k==AttackKind::BALL_GRADE) {
            ballFrom=position;glm::vec3 d=ballAim-position;d.y=0;float len=glm::length(d);
            ballDir=len>.01f?d/len:glm::vec3{0,0,1};
            ballAim=position+ballDir*len;
            ballDrive=1.3f;ballContact=false;ev.ballGrade=true;
            ev.ballFrom=position;ev.ballTo=ballAim;
        } else if(k==AttackKind::BALL_GUST) {
            ev.ballGust=true;ev.ballFrom=ballFrom;ev.ballTo=ballAim;
            ballOpen=1.5f;recover(ballPhase>=2?.45f:1.f);
            if(ballPhase>=2){ballFollow=AttackKind::BALL_GRADE;recoverTimer=.5f;}
        } else if(k==AttackKind::BALL_ANCHOR) {
            ev.ballAnchor=true;ev.ballTo=ballAim;ballOpen=1.6f;
            if(glm::length(glm::vec2{w.playerFeet.x-ballAim.x,w.playerFeet.z-ballAim.z})<BALL_ANCHOR_R &&
               std::fabs(w.playerFeet.y-ballAim.y)<2.8f) {ev.meleeHit=true;ev.meleeDamage=32.f;}
            recover(ballPhase==1?1.f:.7f);
        } else if(k==AttackKind::BALL_THROW) {
            fireAt(w.playerEye,ballPhase==3?5:3,.17f,ballPhase==3?29.f:25.f,18.f,1.4f);
            ev.shotHeavy=true;ev.shotParry=320.f;recover(.75f);
        }
        return;
    }
    if(recoverTimer>0)return;
    // Walk into working range; terrain and collision remain authoritative.
    glm::vec3 to=flatTo(w.playerFeet);float distance=glm::length(to);
    if(distance>15.f){velocity=norm2(to)*stats().speed;integrate(dt,w);}
    bool follow=ballFollow!=AttackKind::NONE;
    if(!follow&&!attackReady(dt))return;
    ballAim=w.playerFeet;ballFrom=position;
    int step=follow?-1:ballStep++%5;
    AttackKind k=follow?ballFollow:step==0?AttackKind::BALL_GRADE:step==1?AttackKind::BALL_ANCHOR:
                 step==2?AttackKind::BALL_GUST:step==3?AttackKind::BALL_THROW:AttackKind::BALL_ANCHOR;
    ballFollow=AttackKind::NONE;
    // A perched target is within the cable weight's reach, without invisible damage.
    if(w.playerFeet.y>floorY+3.f&&k==AttackKind::BALL_GRADE)k=AttackKind::BALL_ANCHOR;
    if(k==AttackKind::BALL_ANCHOR)ballAim=anchorTarget;
    ballReverse=k==AttackKind::BALL_GUST && ballPhase==3 && ballStep%2!=0;
    if(ballReverse) {
        // The damaged fan alternates blowing and drawing grit down the road.
        // Both directions are committed and drawn before the motor engages.
        glm::vec3 d=flatTo(ballAim);float n=glm::length(d);
        if(n>.01f) {
            ballFrom=position+d/n*std::min(60.f,n+12.f);
            ballFrom.x=glm::clamp(ballFrom.x,w.bounds.min.x+2.f,w.bounds.max.x-2.f);
            ballFrom.z=glm::clamp(ballFrom.z,w.bounds.min.z+2.f,w.bounds.max.z-2.f);
            ballAim=position;
        }
    }
    if(k==AttackKind::BALL_GRADE) {
        glm::vec3 d=flatTo(ballAim);float n=glm::length(d);
        if(n>.01f)ballAim=position+d/n*std::min(34.f,std::max(16.f,n+6.f));
    }
    float tell=k==AttackKind::BALL_GUST?1.35f:k==AttackKind::BALL_GRADE?1.2f:1.1f;
    startAttack(k,tell*(ballPhase==1?1.f:ballPhase==2?.88f:.76f),follow);
}
