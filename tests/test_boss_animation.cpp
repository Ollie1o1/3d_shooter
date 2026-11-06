#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include "core/Balance.h"
#include "render/EnemyModel.h"
#include <cstdio>
static int checks=0;
static void check(bool ok,const char* what){++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}
int main(){
    balanceInit();
    for(float dt:{1.f/30,1.f/60,1.f/120}){
        Enemy e(EnemyType::ABSOLUTE,{0,0,0});e.state=EnemyState::ACTIVE;e.spawnTimer=0;
        EnemyWorld w;w.playerFeet={0,0,15};w.playerEye={0,1.7f,15};w.bounds={{-50,0,-50},{50,50,50}};
        int impacts=0;
        for(int i=0;i<(int)std::round(Enemy::AB_ARRIVE/dt);++i){
            e.update(dt,w);rig::tickAbsoluteBody(e,dt);
            if(e.ev.abLanded){++impacts;check(e.abAlt<.001f,"entrance impact fires at floor contact");check(e.abArrive>0,"contact precedes the settled guard");}
            auto p=rig::absoluteLivePose(e,e.age);
            for(auto ch:rig::AB_CH)check(std::isfinite(p.*ch),"finite entrance joints");
            if(e.abArrive>0)check(!e.targetable(),"entrance remains invulnerable through recovery");
        }
        check(impacts==1,"one landing impact at every frame rate");
        check(e.abAlt<.001f,"entrance ends on floor");
        for(auto a:{AttackKind::ASKYFALL,AttackKind::MDROP,AttackKind::AMETEOR}){
            e.abArrive=0;e.abAlt=0;e.abBodyLanded=a;e.abBodySince=.04f;e.telegraphTimer=0;
            auto p=rig::absolutePose(e,e.age);check(p.plant>.99f && p.kneeR>1.f,"aerial impacts absorb weight into a planted landing");
        }
        for(auto a:{AttackKind::SU_SUTURE,AttackKind::SU_RECALL,AttackKind::SU_LATTICE,AttackKind::SU_CASCADE,AttackKind::SU_SEVER,AttackKind::SU_PERFUSION}){
            Enemy su(EnemyType::SUCCESSOR,{0,0,0});su.state=EnemyState::ACTIVE;su.spawnTimer=0;su.suPhase=2;
            su.attack=a;su.telegraphDuration=.8f;
            for(int frame=0;frame<(int)(1.6f/dt);++frame){
                const float t=frame*dt;su.age=t;su.telegraphTimer=std::max(0.f,.8f-t);
                if(t>=.8f){su.attack=AttackKind::NONE;su.suLanded=a;su.suSince=t-.8f;}
                rig::tickSuccessorBody(su,dt);
                for(float v:su.suBody)check(std::isfinite(v) && std::fabs(v)<6.f,"bounded ranged-attack springs");
                glm::vec3 r,l;check(rig::successorPalms(su,t,r,l),"palm attachment model loads");
                check(std::isfinite(r.x+r.y+r.z+l.x+l.y+l.z),"finite animated effect attachments");
            }
            auto guard=rig::successorGuard(su,0);
            auto chamber=rig::successorCastPose(guard,a,.65f,.28f),strike=rig::successorCastPose(guard,a,1,0);
            float diff=0;for(auto ch:rig::SU_CH)diff+=std::fabs(chamber.*ch-strike.*ch);
            check(diff>1.f,"each ranged cast has a distinct release stroke");
        }
    }
    std::printf("%d boss animation checks passed (30/60/120 Hz).\n",checks);
}
