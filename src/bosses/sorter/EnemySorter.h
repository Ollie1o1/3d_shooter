#pragma once
// The gantry leads predictable movement, then commits. Every press leaves
// a fixed 0.55 s lock window and a visible final downstroke.
inline void Enemy::thinkSorter(float dt, const EnemyWorld& w, bool resolve) {
    if (!sortHomeSet) { sortHome=position; sortHomeSet=true; }
    const float floor=sortHome.y-2.f;
    const int want=health>maxHealth*.66f?1:health>maxHealth*.30f?2:3;
    if(want>sortPhase) {
        sortPhase=want; sortPause=want==1?2.8f:want==3?2.2f:1.2f;sortFollowups=0;
        ev.sortPhaseStarted=want; enraged=want>=2;
        telegraphTimer=0; attack=AttackKind::NONE; sortScan=0;
    }
    sortMark=std::max(0.f,sortMark-dt); sortOpen=std::max(0.f,sortOpen-dt);
    velocity=glm::vec3{0.f};
    glm::vec3 to=flatTo(w.playerFeet); turnToward(to,dt,4.2f);
    if(sortPause>0) {sortPause-=dt;return;}
    const float quick=sortPhase==1?1.f:sortPhase==2?.88f:.78f;
    glm::vec3 lead=w.playerVel;lead.y=0;float speed=glm::length(lead);
    if(speed>10.f)lead*=10.f/speed;
    glm::vec3 target=w.playerFeet+lead*(sortMark>0?.4f:.25f);
    if(sortScan>0.f) {
        sortScan-=dt;
        const float a=sortScanYaw+(1.6f-sortScan)*(sortPhase==3?1.1f:.9f);
        glm::vec3 from=position+glm::vec3{0,3,0}, dir{std::sin(a),sortScanSlope,std::cos(a)};
        dir=glm::normalize(dir);
        glm::vec3 end=from+dir*65.f;
        float nearest=65.f;
        for(int i=0;i<w.wallCount;++i) {float t=rayBoxHit(from,dir,w.walls[i].box);if(t>0.05f) nearest=std::min(nearest,t);}
        end=from+dir*nearest; ev.beamOn=true;ev.beamFrom=from;ev.beamTo=end;
        AABB body{w.playerFeet-glm::vec3{.4f,0,.4f},w.playerFeet+glm::vec3{.4f,1.8f,.4f}};
        if(segmentHitsBox(from,end,body)) {ev.sortMarked=sortMark<=0;sortMark=4.f;}
        if(sortScan<=0) {sortScan=0;recover(.45f);}
        return;
    }
    if(sortMark>0 && !lineOfSight(position+glm::vec3{0,3,0},w)) sortMark=0;
    if(telegraphTimer>0) {
        if(attack==AttackKind::SORT_STAMP||attack==AttackKind::SORT_GRAB) {
            if(telegraphTimer>SORT_LOCK)sortAim=target;
            else if(!sortLocked){sortLocked=true;ev.sortLocked=true;}
            if(attack==AttackKind::SORT_STAMP && telegraphTimer<=SORT_DROP && !sortDropCued) {
                sortDropCued=true;ev.sortDrop=true;
            }
        }
        return;
    }
    if(resolve) {
        AttackKind k=attack; attack=AttackKind::NONE;
        const float d=glm::length(glm::vec2{w.playerFeet.x-sortAim.x,w.playerFeet.z-sortAim.z});
        if(k==AttackKind::SORT_STAMP) {
            sortImpactAt=sortAim;sortImpactT=0;sortImpactKind=k;
            ev.sortStamp=true;ev.sortAt=sortAim;sortOpen=1.7f;
            if(d<SORT_STAMP_R && std::fabs(w.playerFeet.y-sortAim.y)<4.f) {ev.meleeHit=true;ev.meleeDamage=sortPhase==3?40.f:36.f;}
            recover(sortPhase==1?.9f:.65f);
            if(sortFollowups>0)recoverTimer=.5f;   // a warned follow-up within the same press string
        } else if(k==AttackKind::SORT_GRAB) {
            sortImpactAt=sortAim;sortImpactT=0;sortImpactKind=k;
            ev.sortAt=sortAim;
            if(d<SORT_GRAB_R && std::fabs(w.playerFeet.y-sortAim.y)<3.f) {ev.meleeHit=true;ev.meleeDamage=26.f;ev.sortGrab=true;}
            recover(.6f);
        } else if(k==AttackKind::SORT_SCAN) {
            sortScan=sortPhase==1?2.5f:2.f;sortScanYaw=yaw;
            sortScanSlope=(w.playerEye.y-position.y-3.f)/std::max(4.f,glm::length(flatTo(w.playerFeet)));
        } else if(k==AttackKind::SORT_THROW) {
            fireAt(w.playerEye,sortPhase==3?5:3,.24f,sortPhase==3?27.f:23.f,19.f,1.8f);
            ev.shotHeavy=true;ev.shotParry=300.f;recover(.55f);
        }
        return;
    }
    if(recoverTimer>0)return;
    if(sortFollowups>0) {
        --sortFollowups;sortAim=target;sortLocked=sortDropCued=false;
        startAttack(AttackKind::SORT_STAMP,sortPhase==3?.95f:1.05f,true);return;
    }
    // Runs on its rails at any height: camping on cover or hooks never removes its attacks.
    glm::vec3 goal=w.playerFeet;goal.y=floor+2.f;
    goal.x=glm::clamp(goal.x,w.bounds.min.x+7.f,w.bounds.max.x-7.f);
    goal.z=glm::clamp(goal.z,w.bounds.min.z+7.f,w.bounds.max.z-7.f);
    glm::vec3 delta=goal-position;float len=glm::length(delta);
    if(len>7.f)position+=delta/len*std::min(len-7.f,dt*(sortPhase==3?14.f:sortPhase==2?11.f:8.5f));
    if(!attackReady(dt))return;
    ++sortStep; sortAim=target;sortLocked=sortDropCued=false;
    AttackKind k;
    // Every phase teaches the same four tools, with more stamps in the final phase.
    int n=sortStep%6;
    k=n==1?AttackKind::SORT_SCAN:n==3?AttackKind::SORT_THROW:n==0||n==4?AttackKind::SORT_GRAB:AttackKind::SORT_STAMP;
    if(k==AttackKind::SORT_STAMP&&n==2&&sortPhase>=2)sortFollowups=sortPhase==3?2:1;
    float tell=k==AttackKind::SORT_STAMP?1.25f:k==AttackKind::SORT_GRAB?.9f:1.f;
    startAttack(k,tell*quick);
}
