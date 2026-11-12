#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include "PortableRand.h"   // (first: one rand() sequence on macOS and Linux)
#include "core/Balance.h"
#include "render/EnemyModel.h"
#include <cstdio>
static int checks=0;
static void check(bool ok,const char* what){++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}
// ---- the motion pass (docs/fable-plan.md): planted feet, bounded springs at every frame rate, hit
// recoil that comes home, and the fights' tells exactly as they were ----------------------------------
static void absMotion(){
    using namespace rig;
    // Planted feet stay planted. Walked round a 9 m circle at three paces, both sizes, four frame rates:
    // an ankle that is down doesn't move over the floor, never sinks into it, and the leg always reaches it.
    for(int form=0;form<2;++form)for(float pace:{.6f,1.2f,1.8f})for(float dt:{1.f/30,1.f/60,1.f/120,1.f/240}){
        Enemy e(EnemyType::ABSOLUTE,{0,0,0});e.state=EnemyState::ACTIVE;e.spawnTimer=0;e.abArrive=0;e.abAlt=e.abAltWant=0;
        if(form){e.abForm=1;e.scale=1.65f;}
        double slide=0,travel=0;float under=0,shortBy=0;glm::vec3 prev[2];bool had[2]={false,false};int steps=0;
        const float v=pace*e.scale;float t=0;
        for(int i=0;i<(int)(14.f/dt);++i,t+=dt){
            const float ang=v*t/9.f;e.position={9.f*std::sin(ang),0,9.f*(1.f-std::cos(ang))};e.yaw=ang;e.age+=dt;
            tickAbsoluteBody(e,dt);steps+=(e.abStepped&1)+((e.abStepped>>1)&1);
            if(t<2.f)continue;
            AbsPose P=absoluteLivePose(e,e.age);
            for(int k=0;k<2;++k){
                glm::vec3 a=glm::vec3(absRootOf(e)*glm::vec4(absAnkleLocal(e,P,k==0),1.f));
                under=std::min(under,a.y-AB_SOLE*e.scale);
                shortBy=std::max(shortBy,glm::length(glm::vec2{a.x-e.abFoot[k].x,a.z-e.abFoot[k].z}));
                const bool down=e.abFootS[k]>=1.f;
                if(down && had[k])slide+=glm::length(glm::vec2{a.x-prev[k].x,a.z-prev[k].z});
                prev[k]=a;had[k]=down;
            }
            travel+=v*dt;
        }
        check(slide<travel*.005+.01,"a planted foot stays where it was put (under 0.5% of the ground covered)");
        check(under>-.01f*e.scale,"no ankle sinks into the floor");
        check(shortBy<.01f*e.scale,"the leg reaches its planted foot");
        check(steps>8,"he steps to cover ground rather than sliding");
        const double stride=travel*14./12./steps;
        check(stride>.1*e.scale && stride<1.1*e.scale,"a stride is the ground covered, within a leg's reach");
    }
    // Every move at every frame rate: joints finite and in range, nothing snaps, and the same pose at the
    // same moment whatever the tick length (the springs are solved exactly).
    const AttackKind moves[]={AttackKind::AJAB,AttackKind::ACROSS,AttackKind::AHOOK,AttackKind::AUPPER,AttackKind::ASTOMP,AttackKind::ASWEEP,
        AttackKind::ALUNGE,AttackKind::ACRESCENT,AttackKind::ABACKFIST,AttackKind::AELBOW,AttackKind::AROUNDHOUSE,AttackKind::AHAYMAKER,
        AttackKind::ACHARGE,AttackKind::AAXE,AttackKind::ACLAP,AttackKind::MJAB,AttackKind::MBACKHAND,AttackKind::MGRASP,AttackKind::MSUNDER,AttackKind::MFISSURE,AttackKind::MFLARES};
    for(AttackKind a:moves)for(int form=0;form<2;++form){
        AbsPose at[4];int r=0;
        for(float dt:{1.f/30,1.f/60,1.f/120,1.f/240}){
            Enemy e(EnemyType::ABSOLUTE,{0,0,0});e.state=EnemyState::ACTIVE;e.spawnTimer=0;e.abArrive=0;e.abAlt=e.abAltWant=0;
            e.abBlade=a==AttackKind::ASWEEP||a==AttackKind::ALUNGE||a==AttackKind::ACRESCENT;
            if(form){e.abForm=1;e.scale=1.65f;}
            const float tell=.5f;AbsPose prev{};bool have=false;
            const int n=(int)std::lround(2.4f/dt);
            for(int i=1;i<=n;++i){
                const float t=i*dt,into=t-1.f;e.age=t;
                if(into>=0 && into<tell-1e-4f){e.attack=a;e.telegraphDuration=tell;e.telegraphTimer=tell-into;}
                else{e.telegraphTimer=0;if(into>=tell)e.attack=AttackKind::NONE;}
                tickAbsoluteBody(e,dt);
                AbsPose P=absoluteLivePose(e,e.age);
                for(auto ch:AB_CH)check(std::isfinite(P.*ch) && std::fabs(P.*ch)<7.f,"finite, bounded joints through every move");
                if(have)for(auto ch:AB_CH)check(std::fabs(P.*ch-prev.*ch)<3.4f*dt*120.f*.4f+.05f,"no joint snaps between ticks");
                prev=P;have=true;
            }
            at[r++]=absoluteSprungPose(e,e.age);
        }
        // (settled back in his guard 0.9 s after the blow: the same guard at 30 Hz as at 240)
        for(int k=0;k<AB_NCH;++k)check(std::fabs(at[0].*AB_CH[k]-at[3].*AB_CH[k])<.06f && std::fabs(at[1].*AB_CH[k]-at[3].*AB_CH[k])<.04f,
            "the same motion at 30, 60 and 240 Hz");
    }
    // MORNINGSTAR is heavier, not a scaled copy: sent to the same pose, his trunk takes longer to get there
    // and to come to rest.
    {
        float settle[2];
        for(int form=0;form<2;++form){
            Enemy e(EnemyType::ABSOLUTE,{0,0,0});e.state=EnemyState::ACTIVE;e.spawnTimer=0;e.abArrive=0;e.abAlt=e.abAltWant=0;
            if(form){e.abForm=1;e.scale=1.65f;}
            const float dt=1.f/120;for(int i=0;i<240;++i){e.age=i*dt;tickAbsoluteBody(e,dt);}
            e.abBody[3]-=.3f;e.abBodyV[3]=0;   // (both tipped back by the same amount, and let go)
            float last=0;for(int i=0;i<360;++i){e.age=2.f+i*dt;tickAbsoluteBody(e,dt);
                if(std::fabs(e.abBody[3]-absolutePose(e,e.age).torsoPitch)>.012f)last=i*dt;}
            settle[form]=last;
        }
        check(settle[1]>settle[0]*1.25f,"MORNINGSTAR's trunk takes longer to come to rest than the Absolute's");
    }
    // A hit goes into the body and the body comes home: struck from in front he rocks back (pitch falls),
    // and a second later he is in his guard again. Never across a tell.
    for(int form=0;form<2;++form){
        Enemy e(EnemyType::ABSOLUTE,{0,0,0});e.state=EnemyState::ACTIVE;e.spawnTimer=0;e.abArrive=0;e.abAlt=e.abAltWant=0;e.yaw=0;
        if(form){e.abForm=1;e.scale=1.65f;}
        const float dt=1.f/120;for(int i=0;i<240;++i){e.age=i*dt;tickAbsoluteBody(e,dt);}
        const float before=e.abBody[3];absoluteHit(e,{0,0,-1},1.f);   // (he faces +Z: this one comes from in front)
        float least=before;for(int i=0;i<180;++i){e.age=2.f+i*dt;tickAbsoluteBody(e,dt);least=std::min(least,e.abBody[3]);}
        check(least<before-(form?.03f:.08f),"a hit from the front rocks him back");
        check(std::fabs(e.abBody[3]-absolutePose(e,e.age).torsoPitch)<.03f,"and he is back in his guard within a second and a half");
        e.attack=AttackKind::ACROSS;e.telegraphDuration=e.telegraphTimer=.4f;const float v=e.abBodyV[3];
        absoluteHit(e,{0,0,-1},1.f);check(e.abBodyV[3]==v,"a hit never bends a tell");
    }
}
static void sovMotion(){
    using namespace rig;
    const AttackKind moves[]={AttackKind::DASH,AttackKind::SWEEP,AttackKind::CRESCENT,AttackKind::CLEAVE,AttackKind::LEAP,AttackKind::BLINK,
        AttackKind::JUDGMENT,AttackKind::WHIRL,AttackKind::THRUST,AttackKind::RUPTURE,AttackKind::PHANTOMS};
    for(AttackKind a:moves){
        PoseOverride at[4];int r=0;
        for(float dt:{1.f/30,1.f/60,1.f/120,1.f/240}){
            Enemy e(EnemyType::SOVEREIGN,{0,0,0});e.state=EnemyState::ACTIVE;e.spawnTimer=0;
            const float tell=.55f;PoseOverride prev{};bool have=false;float snapped=0;
            const int n=(int)std::lround(2.6f/dt);
            for(int i=1;i<=n;++i){
                const float t=i*dt,into=t-1.f;e.age=t;
                if(into>=0 && into<tell-1e-4f){e.attack=a;e.telegraphDuration=tell;e.telegraphTimer=tell-into;}
                else{e.telegraphTimer=0;if(into>=tell){e.attack=AttackKind::NONE;e.lastSwing=a;e.swingTimer=std::max(0.f,Enemy::SWING_TIME-(into-tell));}}
                tickSovereignBody(e,dt);
                PoseOverride P=sovereignPose(e);
                for(auto ch:SOV_CH){check(std::isfinite(P.*ch) && std::fabs(P.*ch)<6.f,"the Sovereign: finite, bounded joints");
                    if(have)snapped=std::max(snapped,std::fabs(P.*ch-prev.*ch)/(dt*120.f));}
                prev=P;have=true;
            }
            // (his stance used to be cut to: the sweep's follow-through started 2.2 rad from its wind-up in one frame)
            check(snapped<.55f,"the Sovereign moves into and out of every stance; none is cut to");
            at[r++]=sovereignPose(e);
        }
        for(auto ch:SOV_CH)check(std::fabs(at[0].*ch-at[3].*ch)<.06f && std::fabs(at[1].*ch-at[3].*ch)<.04f,"the Sovereign: the same motion at 30, 60 and 240 Hz");
    }
    {   // idle life: over ten seconds standing, his sword hand and his shoulders move
        Enemy e(EnemyType::SOVEREIGN,{0,0,0});e.state=EnemyState::ACTIVE;e.spawnTimer=0;
        float lo=9,hi=-9,ylo=9,yhi=-9;const float dt=1.f/60;
        for(int i=0;i<600;++i){e.age=i*dt;tickSovereignBody(e,dt);auto P=sovereignPose(e);lo=std::min(lo,P.grip);hi=std::max(hi,P.grip);ylo=std::min(ylo,P.torsoYaw);yhi=std::max(yhi,P.torsoYaw);}
        check(hi-lo>.1f && yhi-ylo>.08f,"the Sovereign's guard lives: the blade drifts, his shoulders turn");
        const float before=e.sovBody[2];sovereignHit(e,{0,0,-1},1.f);float least=before;
        for(int i=0;i<120;++i){e.age=10.f+i*dt;tickSovereignBody(e,dt);least=std::min(least,e.sovBody[2]);}
        check(least<before-.03f,"a hit from the front rocks the Sovereign back");
    }
}
static void suMotion(){
    using namespace rig;
    Enemy e(EnemyType::SUCCESSOR,{0,0,0});e.state=EnemyState::ACTIVE;e.spawnTimer=0;e.suPhase=2;
    const float dt=1.f/60;float lo=9,hi=-9;
    for(int i=0;i<900;++i){e.age=i*dt;tickSuccessorBody(e,dt);lo=std::min(lo,e.suBody[15]);hi=std::max(hi,e.suBody[15]);}
    check(hi-lo>.1f,"the Successor's guard lives: his hand works while he waits");
    const float before=e.suBody[2];successorHit(e,{0,0,-1},1.f);float least=before;
    for(int i=0;i<90;++i){e.age=15.f+i*dt;tickSuccessorBody(e,dt);least=std::min(least,e.suBody[2]);for(float v:e.suBody)check(std::isfinite(v)&&std::fabs(v)<6.f,"bounded springs through a hit");}
    check(least<before-.04f,"a hit from the front rocks the Successor back");
    check(std::fabs(e.suBody[2]-successorGoal(e,e.age).lean)<.04f,"and he is back in his guard");
}
// The tells each boss's real mind starts over two minutes against a player circling it (which attack, how
// long its wind-up, in order), against the same run on main before this pass: animation must not move them.
static void tellsUnchanged(){
    struct Want{EnemyType t;const char* name;int tells;float total;uint32_t hash;};
    const Want want[]={{EnemyType::ABSOLUTE,"the Absolute",160,68.693f,0xe379e830u},{EnemyType::SUCCESSOR,"the Successor",127,67.410f,0xeec73058u},
                       {EnemyType::SOVEREIGN,"the Sovereign",88,36.790f,0x65637f65u}};   // (on tests/PortableRand.h's sequence: the same on every platform)
    // (the Absolute's and the Successor's hashes were re-recorded when the Revenant's and Weaver's three attack kinds left the enum before theirs:
    // the hash takes each attack's number. Their tell counts and total lengths did not move.)
    for(const Want& wnt:want){
        srand(1234);const float dt=1.f/60;
        Enemy e(wnt.t,{0,0,0});e.state=EnemyState::ACTIVE;e.spawnTimer=0;
        if(wnt.t==EnemyType::ABSOLUTE){e.abArrive=0;e.abAlt=e.abAltWant=0;}
        if(wnt.t==EnemyType::SUCCESSOR)e.suPhase=2;
        EnemyWorld w;w.bounds={{-60,0,-60},{60,50,60}};
        uint32_t h=2166136261u;int tells=0;float total=0;
        for(int i=0;i<60*120;++i){
            const float tt=i*dt;
            w.playerFeet={7*std::cos(tt*.4f),0,7*std::sin(tt*.4f)};w.playerEye=w.playerFeet+glm::vec3{0,1.7f,0};
            e.mayAttack=true;e.health=e.maxHealth;e.floorY=0;
            e.update(dt,w);
            // (the bodies tick as they do in the game: nothing they do may reach back into the fight)
            if(wnt.t==EnemyType::ABSOLUTE)rig::tickAbsoluteBody(e,dt);
            if(wnt.t==EnemyType::SUCCESSOR)rig::tickSuccessorBody(e,dt);
            if(wnt.t==EnemyType::SOVEREIGN)rig::tickSovereignBody(e,dt);
            if(e.ev.telegraphStarted){++tells;total+=e.telegraphDuration;
                for(uint32_t v:{(uint32_t)e.attack,(uint32_t)std::lround(e.telegraphDuration*1000.f)})h=(h^v)*16777619u;}
        }
        if(tells!=wnt.tells || h!=wnt.hash)std::fprintf(stderr,"%s: %d tells, %.3f s, 0x%08x (was %d, %.3f, 0x%08x)\n",wnt.name,tells,total,h,wnt.tells,wnt.total,wnt.hash);
        check(tells==wnt.tells && h==wnt.hash && std::fabs(total-wnt.total)<.01f,"every tell, and its length, exactly as on main");
    }
}
int main(){
    balanceInit();
    absMotion();sovMotion();suMotion();tellsUnchanged();
    for(float dt:{1.f/30,1.f/60,1.f/120,1.f/240}){
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
    std::printf("%d boss animation checks passed (30/60/120/240 Hz).\n",checks);
}
