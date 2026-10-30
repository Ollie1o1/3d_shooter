#pragma once
inline void Enemy::tickSorterTools(float dt,const EnemyWorld& w) {
    for(auto& p:sortPresses) {
        if(p.remaining>0) {
            p.remaining-=dt;
            if(p.remaining<=0) {
                p.remaining=0;p.impact=0;ev.sortStamp=true;ev.sortAt=p.at;sortOpen=1.35f;
                if(glm::length(glm::vec2{w.playerFeet.x-p.at.x,w.playerFeet.z-p.at.z})<SORT_STAMP_R&&std::fabs(w.playerFeet.y-p.at.y)<1.1f) {
                    ev.meleeHit=true;ev.meleeDamage=34.f;
                }
            }
        } else p.impact+=dt;
    }
    sortPresses.erase(std::remove_if(sortPresses.begin(),sortPresses.end(),[](const auto& p){return p.impact>SORT_IMPACT;}),sortPresses.end());
    sortFire=std::max(0.f,sortFire-dt);sortFireTick=std::max(0.f,sortFireTick-dt);
    if(sortFire>0&&sortFireTick<=0)for(int i=0;i<sortVentCount;++i) {
        const auto& box=sortVents[i];const auto p=w.playerFeet;
        if(p.x>box.min.x&&p.x<box.max.x&&p.z>box.min.z&&p.z<box.max.z&&p.y<box.max.y&&p.y>box.min.y-.5f) {
            ev.meleeHit=true;ev.meleeDamage=16;sortFireTick=.65f;break;
        }
    }
}

inline void Enemy::thinkSorter(float dt,const EnemyWorld& w,bool resolve) {
    if(!sortHomeSet){sortHome=position;sortHomeSet=true;}
    const float floor=sortHome.y-2.f;
    const int want=health>maxHealth*.66f?1:health>maxHealth*.30f?2:3;
    if(want>sortPhase) {
        sortPhase=want;sortPause=want==1?2.8f:want==3?2.2f:1.2f;
        sortNext=AttackKind::NONE;sortFollowups=0;sortRush=sortFire=0;sortPresses.clear();sortVolley=0;
        ev.sortPhaseStarted=want;enraged=want>=2;telegraphTimer=0;attack=AttackKind::NONE;sortScan=0;
    }
    for(float* timer:{&sortMark,&sortOpen,&sortRushCd,&sortRowCd,&sortPurgeCd,&sortThrowCd})*timer=std::max(0.f,*timer-dt);
    velocity=glm::vec3{0};
    turnToward(flatTo(w.playerFeet),dt,4.8f);
    if(sortPause>0){sortPause-=dt;return;}
    sortObservedVel=glm::mix(sortObservedVel,machineryLead(w.playerVel),1.f-std::exp(-dt*8.f));
    leadVel_=sortObservedVel*tune_->lead;
    glm::vec3 lead=sortObservedVel,target=machineryClamp(w.playerFeet+lead*(sortMark>0?.45f:.30f),w.bounds);
    const float distance=glm::length(flatTo(w.playerFeet));
    const bool high=w.playerFeet.y>floor+4.f;
    auto pressPoint=[&](glm::vec3 at) {
        at.y=floor;
        for(int i=0;i<w.wallCount;++i){const auto& b=w.walls[i].box;
            if(at.x>=b.min.x&&at.x<=b.max.x&&at.z>=b.min.z&&at.z<=b.max.z&&b.max.y<=w.playerFeet.y+.25f&&b.max.y<floor+10)
                at.y=std::max(at.y,b.max.y);
        }
        return at;
    };
    auto chase=[&](float speed) {
        glm::vec3 goal=machineryClamp(w.playerFeet+lead*.4f,w.bounds,6.f);goal.y=floor+2;
        glm::vec3 delta=goal-position;float n=glm::length(delta);
        if(n>6.f){auto step=delta/n*std::min(n-6.f,speed*dt);position+=step;velocity=step/std::max(dt,.001f);}
    };
    if(sortRush>0) {
        glm::vec3 was=position,d=sortRushTo-position;float n=glm::length(d);
        glm::vec3 step=sortRushDir*std::min(n,(sortPhase==3?36.f:31.f)*dt);
        position+=step;velocity=step/std::max(dt,.001f);sortRush-=dt;
        yaw=std::atan2(sortRushDir.x,sortRushDir.z);
        if(!sortContact&&machinerySweepHits(w.playerFeet,was,position,3.4f,floor,3.6f)) {
            sortContact=true;ev.meleeHit=true;ev.meleeDamage=26;
        }
        if(sortRush<=0||n<.5f) {
            sortRush=0;sortOpen=1.25f;recover(.85f);sortNext=AttackKind::SORT_GRAB;
        }
        return;
    }
    // The scanner runs alongside pursuit and attacks rather than suspending the fight.
    if(sortScan>0) {
        sortScan=std::max(0.f,sortScan-dt);
        float a=sortScanYaw+(1.6f-sortScan)*(sortPhase==3?1.1f:.9f);
        glm::vec3 from=position+glm::vec3{0,3,0},dir=glm::normalize(glm::vec3{std::sin(a),sortScanSlope,std::cos(a)});
        float length=65;
        for(int i=0;i<w.wallCount;++i){float t=rayBoxHit(from,dir,w.walls[i].box);if(t>.05f)length=std::min(length,t);}
        ev.beamOn=true;ev.beamFrom=from;ev.beamTo=from+dir*length;
        AABB body{w.playerFeet-glm::vec3{.4f,0,.4f},w.playerFeet+glm::vec3{.4f,1.8f,.4f}};
        if(segmentHitsBox(from,ev.beamTo,body)){ev.sortMarked=sortMark<=0;sortMark=4;}
    }
    if(sortMark>0&&!lineOfSight(position+glm::vec3{0,3,0},w))sortMark=0;
    if(sortVolley>0) {
        sortVolleyT-=dt;
        if(sortVolleyT<=0){--sortVolley;fireAt(sortVolleyAim,sortPhase==3?5:3,.21f,33,17,1.5f);ev.shotHeavy=true;ev.shotParry=300;}
    }
    if(telegraphTimer>0) {
        if(attack==AttackKind::SORT_GRAB||attack==AttackKind::SORT_STAMP||attack==AttackKind::SORT_ROW||attack==AttackKind::SORT_RUSH) {
            if(telegraphTimer>SORT_LOCK) {
                sortAim=(attack==AttackKind::SORT_STAMP||attack==AttackKind::SORT_ROW)?pressPoint(target):target;
                if(attack==AttackKind::SORT_RUSH){sortRushTo=machineryClamp(w.playerFeet+lead*glm::clamp(distance/31.f+.45f,.6f,1.5f),w.bounds);sortRushTo.y=floor+2;}
                if(attack==AttackKind::SORT_ROW)for(int i=0;i<(int)sortPresses.size();++i)
                    sortPresses[i].at=pressPoint(machineryClamp(sortAim+sortRowDir*((i-1)*5.8f),w.bounds));
            } else if(!sortLocked){sortLocked=true;ev.sortLocked=true;}
            if(attack==AttackKind::SORT_STAMP&&telegraphTimer<=SORT_DROP&&!sortDropCued){sortDropCued=true;ev.sortDrop=true;}
        }
        // Preparation can close ground; the final target and downstroke stay fixed.
        if(attack!=AttackKind::SORT_STAMP&&attack!=AttackKind::SORT_ROW&&telegraphTimer>SORT_LOCK)
            chase(sortPhase==3?13.f:10.f);
        return;
    }
    if(resolve) {
        AttackKind k=attack;attack=AttackKind::NONE;
        float d=glm::length(glm::vec2{w.playerFeet.x-sortAim.x,w.playerFeet.z-sortAim.z});
        if(k==AttackKind::SORT_STAMP||k==AttackKind::SORT_GRAB) {
            sortImpactAt=sortAim;sortImpactT=0;sortImpactKind=k;
            float r=k==AttackKind::SORT_STAMP?SORT_STAMP_R:SORT_GRAB_R;
            if(k==AttackKind::SORT_STAMP){ev.sortStamp=true;sortOpen=1.6f;}
            ev.sortAt=sortAim;
            if(d<r&&std::fabs(w.playerFeet.y-sortAim.y)<(k==AttackKind::SORT_STAMP?1.1f:3.f)){ev.sortGrab=k==AttackKind::SORT_GRAB;ev.meleeHit=true;ev.meleeDamage=k==AttackKind::SORT_STAMP?36:26;}
            recover(.85f);if(sortFollowups>0)recoverTimer=.5f;
        } else if(k==AttackKind::SORT_RUSH) {
            sortRushFrom=position;glm::vec3 d=sortRushTo-position;float n=glm::length(d);
            sortRushDir=n>.01f?d/n:glm::vec3{0,0,1};sortRush=std::min(1.6f,n/31.f+.1f);sortContact=false;ev.sortRushStarted=true;
        } else if(k==AttackKind::SORT_ROW){sortOpen=1.2f;recover(1.2f);}
        else if(k==AttackKind::SORT_PURGE){sortFire=2.8f;sortFireTick=0;ev.sortPurge=true;recover(.8f);}
        else if(k==AttackKind::SORT_SCAN) {
            sortScan=2.2f;sortScanYaw=yaw;sortScanSlope=(w.playerEye.y-position.y-3)/std::max(4.f,distance);recover(.8f);
        } else if(k==AttackKind::SORT_THROW) {
            sortVolleyAim=w.playerEye+lead*.3f;sortVolleyAim.y+=glm::clamp(w.playerVel.y*.12f,-1.5f,1.5f);
            fireAt(sortVolleyAim,sortPhase==3?5:3,.23f,31,18,1.7f);ev.shotHeavy=true;ev.shotParry=300;
            sortVolley=sortPhase>=2?1:0;sortVolleyT=.28f;recover(.85f);
        }
        return;
    }
    // Brief heavy-miss opening, then pursue even while the attack cooldown expires.
    if(recoverTimer>0){if(recoverTimer<.35f)chase(10.f);return;}
    chase(sortPhase==3?18.f:sortPhase==2?15.f:12.f);
    bool follow=sortNext!=AttackKind::NONE||sortFollowups>0;
    if(!follow&&!attackReady(dt))return;
    AttackKind k=sortNext;sortNext=AttackKind::NONE;
    if(sortFollowups>0){--sortFollowups;k=AttackKind::SORT_STAMP;}
    if(k==AttackKind::NONE) {
        if(sortStep==0)k=AttackKind::SORT_SCAN;
        else if(high&&sortThrowCd<=0)k=AttackKind::SORT_THROW;
        else if(distance>17&&sortRushCd<=0)k=AttackKind::SORT_RUSH;
        else if(sortPhase>=2&&sortPurgeCd<=0&&sortLast!=AttackKind::SORT_PURGE)k=AttackKind::SORT_PURGE;
        else if(sortRowCd<=0&&sortLast!=AttackKind::SORT_ROW)k=AttackKind::SORT_ROW;
        else if(distance<9&&sortLast!=AttackKind::SORT_GRAB)k=AttackKind::SORT_GRAB;
        else if(sortThrowCd<=0&&sortLast!=AttackKind::SORT_THROW)k=AttackKind::SORT_THROW;
        else k=AttackKind::SORT_STAMP;
    }
    ++sortStep;sortLast=k;sortAim=(k==AttackKind::SORT_STAMP||k==AttackKind::SORT_ROW)?pressPoint(target):target;sortLocked=sortDropCued=false;
    if(k==AttackKind::SORT_RUSH){sortRushCd=5.5f;sortRushTo=machineryClamp(w.playerFeet+lead*glm::clamp(distance/31.f+.45f,.6f,1.5f),w.bounds);sortRushTo.y=floor+2;}
    if(k==AttackKind::SORT_THROW)sortThrowCd=3.8f;
    if(k==AttackKind::SORT_ROW) {
        sortRowCd=6;sortRowDir=glm::length(lead)>1?glm::normalize(lead):norm2(flatTo(target));
        if(glm::length(sortRowDir)<.1f)sortRowDir={1,0,0};
    }
    if(k==AttackKind::SORT_PURGE) {
        sortPurgeCd=9;int closest=0;float best=1e9f;
        for(int i=0;i<3;++i){auto box=sortVentBox(w.bounds,floor,i);float d=glm::length((box.min+box.max)*.5f-w.playerFeet);if(d<best){best=d;closest=i;}}
        sortVentCount=sortPhase==3?2:1;sortVents[0]=sortVentBox(w.bounds,floor,closest);
        sortVents[1]=sortVentBox(w.bounds,floor,(closest+1)%3);
    }
    float quick=sortPhase==1?1.f:sortPhase==2?.91f:.84f;
    float tell=k==AttackKind::SORT_STAMP?1.25f:k==AttackKind::SORT_ROW?1.3f:k==AttackKind::SORT_PURGE?1.4f:k==AttackKind::SORT_RUSH?1.1f:.95f;
    startAttack(k,tell*quick,follow);
    if(k==AttackKind::SORT_ROW)for(int i=0;i<3;++i)
        sortPresses.push_back({pressPoint(machineryClamp(sortAim+sortRowDir*((i-1)*5.8f),w.bounds)),telegraphDuration+.38f*i,-1});
}
