#pragma once
// The inspecting gantry commits its attacks to a point. A mark tracks longer,
// but even a marked stamp leaves 0.35 s to move. Nothing targets through cover.
inline void Enemy::thinkSorter(float dt, const EnemyWorld& w, bool resolve) {
    if (!sortHomeSet) { sortHome=position; sortHomeSet=true; }
    const float floor=sortHome.y-2.f;
    const int want=health>maxHealth*.66f?1:health>maxHealth*.30f?2:3;
    if(want>sortPhase) {
        sortPhase=want; sortPause=want==1?1.8f:2.2f;
        ev.sortPhaseStarted=want; enraged=want>=2;
        telegraphTimer=0; attack=AttackKind::NONE; sortScan=0;
    }
    sortMark=std::max(0.f,sortMark-dt); sortOpen=std::max(0.f,sortOpen-dt);
    velocity=glm::vec3{0.f};
    glm::vec3 to=flatTo(w.playerFeet); turnToward(to,dt,2.8f);
    if(sortPause>0) {sortPause-=dt;return;}
    const float quick=sortPhase==1?1.f:sortPhase==2?.85f:.72f;
    if(sortScan>0.f) {
        sortScan-=dt;
        const float a=sortScanYaw+(1.6f-sortScan)*.72f;
        glm::vec3 from=position+glm::vec3{0,3,0}, dir{std::sin(a),sortScanSlope,std::cos(a)};
        dir=glm::normalize(dir);
        glm::vec3 end=from+dir*65.f;
        float nearest=65.f;
        for(int i=0;i<w.wallCount;++i) {float t=rayBoxHit(from,dir,w.walls[i].box);if(t>0.05f) nearest=std::min(nearest,t);}
        end=from+dir*nearest; ev.beamOn=true;ev.beamFrom=from;ev.beamTo=end;
        AABB body{w.playerFeet-glm::vec3{.4f,0,.4f},w.playerFeet+glm::vec3{.4f,1.8f,.4f}};
        if(segmentHitsBox(from,end,body)) {ev.sortMarked=sortMark<=0;sortMark=4.f;}
        if(sortScan<=0) {sortScan=0;recover(.8f);}
        return;
    }
    if(sortMark>0 && !lineOfSight(position+glm::vec3{0,3,0},w)) sortMark=0;
    if(telegraphTimer>0) {
        // Track during preparation, commit before release. The red floor ring is authoritative.
        if(telegraphTimer>(sortMark>0 || glm::length(to)>24.f || std::fabs(w.playerFeet.y-floor)>5.f?.35f:.65f)) sortAim=w.playerFeet;
        return;
    }
    if(resolve) {
        AttackKind k=attack; attack=AttackKind::NONE;
        const float d=glm::length(glm::vec2{w.playerFeet.x-sortAim.x,w.playerFeet.z-sortAim.z});
        if(k==AttackKind::SORT_STAMP) {
            ev.sortStamp=true;ev.sortAt=sortAim;sortOpen=1.7f;
            if(d<SORT_STAMP_R && std::fabs(w.playerFeet.y-sortAim.y)<4.f) {ev.meleeHit=true;ev.meleeDamage=34.f;}
            recover(sortPhase==1?1.3f:1.f);
        } else if(k==AttackKind::SORT_GRAB) {
            ev.sortAt=sortAim;
            if(d<SORT_GRAB_R && std::fabs(w.playerFeet.y-sortAim.y)<3.f) {ev.meleeHit=true;ev.meleeDamage=22.f;ev.sortGrab=true;}
            recover(.85f);
        } else if(k==AttackKind::SORT_SCAN) {
            sortScan=3.2f;sortScanYaw=yaw;
            sortScanSlope=(w.playerEye.y-position.y-3.f)/std::max(4.f,glm::length(flatTo(w.playerFeet)));
        } else if(k==AttackKind::SORT_THROW) {
            fireAt(w.playerEye,sortPhase==3?5:3,.27f,18.f,17.f,1.8f);
            ev.shotHeavy=true;ev.shotParry=260.f;recover(.8f);
        }
        return;
    }
    if(recoverTimer>0)return;
    // Runs on its rails at any height: camping on cover or hooks never removes its attacks.
    glm::vec3 goal=w.playerFeet;goal.y=floor+2.f;
    goal.x=glm::clamp(goal.x,w.bounds.min.x+7.f,w.bounds.max.x-7.f);
    goal.z=glm::clamp(goal.z,w.bounds.min.z+7.f,w.bounds.max.z-7.f);
    glm::vec3 delta=goal-position;float len=glm::length(delta);
    if(len>7.f)position+=delta/len*std::min(len-7.f,dt*(sortPhase==3?9.f:6.f));
    if(!attackReady(dt))return;
    ++sortStep; sortAim=w.playerFeet;
    AttackKind k;
    // Every phase teaches the same four tools, with more stamps in the final phase.
    int n=sortStep%6;
    k=n==1?AttackKind::SORT_SCAN:n==3?AttackKind::SORT_THROW:n==0||n==4?AttackKind::SORT_GRAB:AttackKind::SORT_STAMP;
    float tell=k==AttackKind::SORT_STAMP?1.35f:k==AttackKind::SORT_GRAB?.95f:1.1f;
    startAttack(k,tell*quick);
}
