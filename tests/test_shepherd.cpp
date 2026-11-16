#include "bosses/shepherd/ShepherdCombat.h"
#include "render/HeadMesh.h"
#include "render/PropFile.h"
#include "world/LevelShepherd.h"
#include "core/Balance.h"
#include <cstdio>
#include <string>
static int failures=0;
#define CHECK(c,m) do { if(!(c)){++failures;std::printf("FAIL: %s\n",m);}else std::printf("ok: %s\n",m); }while(0)
int main(){
    using namespace shephead;
    for(int id:{headmesh::orb(),headmesh::capsule()}){
        const auto& m=*meshlib::get(id);bool finite=true;float facing=0;
        for(size_t i=0;i<m.pos.size();++i){finite&=std::isfinite(glm::length(m.pos[i]))&&std::isfinite(glm::length(m.nrm[i]));facing+=glm::dot(m.pos[i],m.nrm[i]);}
        float winding=0;for(size_t i=0;i+2<m.idx.size();i+=3){auto a=m.pos[m.idx[i]],b=m.pos[m.idx[i+1]],c=m.pos[m.idx[i+2]];winding+=glm::dot((a+b+c)/3.f,glm::cross(b-a,c-a));}
        CHECK(finite&&facing>0&&winding>0,"anatomical meshes have finite outward normals and front-facing triangles");
    }
    for(const char* name:{"shepherd_head","shepherd_torso","shepherd_shoulder","shepherd_upperarm","shepherd_forearm","shepherd_palm","shepherd_finger"}){
        const auto* p=propfile::get(name);CHECK(p&&p->model.parts.size()<=16,"Blender anatomy loads within the material batching budget");
        if(!p)continue;bool valid=true;size_t tris=0;
        for(const auto& part:p->model.parts){const auto* m=meshlib::get(part.mesh);valid&=m&&m->nrm.size()==m->pos.size();if(!m)continue;
            tris+=m->idx.size()/3;
            for(size_t i=0;i<m->pos.size();++i)valid&=std::isfinite(glm::length(m->pos[i]))&&std::isfinite(glm::length(m->nrm[i]));
        }
        CHECK(valid&&tris>100&&tris<50000,"exported anatomy has finite positions/normals and a bounded triangle count");
    }
    const auto* head=propfile::get("shepherd_head");const auto* torso=propfile::get("shepherd_torso");
    auto actualWidth=[](const propfile::Prop* prop){if(!prop)return 0.f;float lo=1e9f,hi=-1e9f;
        for(const auto& part:prop->model.parts)if(const auto* mesh=meshlib::get(part.mesh))for(auto pos:mesh->pos){float x=(part.rel*glm::vec4(pos,1.f)).x;lo=std::min(lo,x);hi=std::max(hi,x);}return hi-lo;};
    CHECK(head&&torso&&actualWidth(torso)*70.f>actualWidth(head)*64.f*1.7f,"adult torso is substantially broader than the skull");
    bool leftEye=false,rightEye=false;
    if(head)for(const auto& p:head->model.parts)if(p.chan==1&&p.k.x>1.f)if(const auto* mesh=meshlib::get(p.mesh))for(auto pos:mesh->pos){float x=(p.rel*glm::vec4(pos,1.f)).x;leftEye|=x<-.06f;rightEye|=x>.06f;}
    CHECK(leftEye&&rightEye,"both anatomical eyeballs survive the glTF export");
    for(int side=0;side<2;++side){
        bool sound=true;
        for(auto hand:{handRest(side,1.f),glm::vec3{-600,142,-3940},glm::vec3{-640,142,-4020}}){
            auto sh=shoulder(side,1.f),el=elbow(side,1.f,hand);float a=glm::length(el-sh),b=glm::length(hand-el);
            sound&=std::isfinite(a)&&std::isfinite(b)&&a>=UPPER_ARM-.01f&&b>=FOREARM-.01f&&std::fabs(a/b-UPPER_ARM/FOREARM)<.002f;
        }
        CHECK(sound,"long arm segments retain their proportions from idle through arena-edge punches");
    }
    CHECK(checkpointHp(HP-1)==HP&&checkpointHp(HP*2.f/3.f-1)==HP*2.f/3.f&&checkpointHp(HP/3.f-1)==HP/3.f&&checkpointHp(0)==0,"retry preserves the current movement and a defeated boss");
    CHECK(nextAttack(1,0)==ECHO&&nextAttack(2,0)==PULL,"new movements start with their signature attack");
    CHECK(gazeIndex(1.8f,1)==0&&gazeIndex(2.1f,1)==1&&gazeIndex(2.8f,1)==2&&gazeIndex(2.8f,0)==0,"head aim and eye rays use the same next gaze target after each discharge");
    CHECK(sweepHits({-600,143,-3980},1.8f,-605,-595,-3980,140)&&!sweepHits({-600,140,-3968},1.8f,-605,-595,-3980,140),"a sweep catches a jump but leaving its depth lane avoids the hand");
    CHECK(grabHits({-600,144,-3980},1.8f,{-600,140,-3980})&&!grabHits({-590,140,-3980},1.8f,{-600,140,-3980}),"an open hand catches jumping in place and can be dodged sideways");
    for(int phase=0;phase<3;++phase){int spatial=0;bool sweep=false,grab=false,gaze=false;
        for(int c=0;c<72;++c){auto attack=nextAttack(phase,c);spatial+=attack==SWEEP||attack==GRAB||attack==GAZE;sweep|=attack==SWEEP;grab|=attack==GRAB;gaze|=attack==GAZE;}
        CHECK(sweep&&grab&&gaze&&spatial>=32,"each phase requires lane changes, grab defence and gaze routing, beyond jumping");
    }
    const glm::vec3 at{-600,140,-3980};
    CHECK(hitBody(at+glm::vec3{0,2,8},{0,0,-1},handBox(at))<.01f,"a blade already inside a giant palm registers contact");
    CHECK(slamHits(at,at)&&!slamHits(at+glm::vec3{8,0,0},at)&&!slamHits(at+glm::vec3{0,12,0},at),"slams respect the marked footprint and vertical escape");
    for(int k=0;k<3;++k){bool inside=true,moving=false;auto first=moon(k,0);
        for(int i=0;i<300;++i){auto p=moon(k,i*.1f);inside&=p.x>shep::RX0+5&&p.x<shep::RX1-5&&p.z>shep::RIDGE_Z+12&&p.z<shep::RZ_S-8&&p.y>MOON_RADIUS;moving|=glm::length(p-first)>10;}
        CHECK(inside&&moving,"each moon has a moving, reachable orbit inside the court");}
    auto level=buildShepherd();
    CHECK(level.arenas.size()==12&&level.story.chapter==6&&level.story.part==1,"chapter checkpoints and arena indices remain compatible");
    CHECK(std::string(level.arenas[3].name)=="THE ORRERY","the vent arena is replaced by the orrery");
    for(int k=7;k<10;++k)CHECK(level.arenas[k].waves.size()==1,"ascent ledges each use one composed encounter");
    CHECK(level.arenas[9].goals.size()==1&&level.arenas[9].goals[0].kind==WaveGoal::EXTERNAL&&!level.arenas[9].goals[0].refill,"the last ascent is a finite boarding encounter instead of another kill gate");
    CHECK(level.shepLedgeMooring>=0&&level.shepLedgeRelay>=0&&level.arenas[7].goals.size()==1&&level.arenas[8].goals.size()==1
        &&level.arenas[7].goals[0].kind==WaveGoal::EXTERNAL&&level.arenas[8].goals[0].kind==WaveGoal::EXTERNAL
        &&!level.arenas[7].goals[0].refill&&!level.arenas[8].goals[0].refill,"all three ascent encounters have distinct objectives with finite guards");
    const auto& court=level.arenas[10];
    CHECK((shep::FALL_X1-shep::FALL_X0)*(shep::FALL_ZS-shep::FALL_ZN)>29000.f,"celestial court has over two and a half times the old floor area");
    CHECK(level.groundAt(court.playerStart.x,court.playerStart.z,shep::FALL_Y)==shep::FALL_Y&&level.arenaAt(shep::crown())==10,"practice spawn and the ascent landing remain on the court floor");
    bool ramps=true;
    for(float x:{-682.f,-518.f}){
        ramps&=std::fabs(level.groundAt(x,-4034.f,shep::FALL_Y+3.f)-(shep::FALL_Y+1.5f))<.01f;
        ramps&=std::fabs(level.groundAt(x,-3975.f,shep::FALL_Y+3.f)-(shep::FALL_Y+3.f))<.01f;
        ramps&=std::fabs(level.groundAt(x,-3922.f,shep::FALL_Y+3.f)-(shep::FALL_Y+1.5f))<.01f;
    }
    CHECK(ramps,"both raised galleries have continuous ramps at both ends");
    bool cover=true;int count=0;
    for(const auto& shelter:level.sweepShelters)if(shelter.section==10){++count;
        cover&=shelter.post.x>court.bounds.min.x&&shelter.post.x<court.bounds.max.x&&shelter.post.z>court.bounds.min.z&&shelter.post.z<court.bounds.max.z;
        cover&=std::fabs(level.groundAt(shelter.post.x,shelter.post.z,shep::FALL_Y)-shep::FALL_Y)<.01f;
    }
    CHECK(cover&&count==16,"sixteen orbital vanes retain reachable posts around the clearer central lane");
    bool terraces=true;
    for(int side=0;side<2;++side){
        const float x=shep::courtTerrace(side).x;
        terraces&=std::fabs(level.groundAt(x,-4010.f,shep::FALL_Y+3.f)-(shep::FALL_Y+1.5f))<.01f;
        terraces&=std::fabs(level.groundAt(x,-3978.f,shep::FALL_Y+3.f)-(shep::FALL_Y+3.f))<.01f;
        terraces&=std::fabs(level.groundAt(x,-3934.f,shep::FALL_Y+3.f)-(shep::FALL_Y+1.5f))<.01f;
        float midX=side?-568.f:-632.f;
        terraces&=std::fabs(level.groundAt(midX,shep::COURT_CROSS_Z,shep::FALL_Y+3.f)-(shep::FALL_Y+1.5f))<.01f;
    }
    CHECK(terraces,"inner terraces have sloped access at both ends and from the heart lane");
    CHECK(level.shepCourtClamps.size()==2,"two dynamic clamps control the optional gallery crossovers");
    // Walk the actual collision geometry from the heart lane to each gallery.
    // Before a break the clamp blocks it; after the break it is a continuous route.
    balanceInit();Uint8 keys[SDL_NUM_SCANCODES]={};
    for(int side=0;side<2;++side)for(bool open:{false,true}){
        level.movers[level.shepCourtClamps[side]].gone=open;level.updateMovers(0.f);
        Player p({side?-578.f:-622.f,shep::FALL_Y,shep::COURT_CROSS_Z});
        for(int frame=0;frame<60*12;++frame){
            p.floorY=level.baseFloor(p.position.x,p.position.z,p.radius);
            p.velocity.x=shep::courtSide(side)*6.f;
            p.update(1.f/60,keys,level.walls.data(),(int)level.walls.size(),true);
            if(std::fabs(p.position.x+600.f)>81.f)break;
        }
        CHECK(open?(std::fabs(p.position.x+600.f)>81.f&&std::fabs(p.position.y-shep::FALL_Y-3.f)<.1f):(std::fabs(p.position.x+600.f)<68.f),"a broken clamp opens a walkable crossover while an intact clamp blocks that route");
    }
    std::printf("%d failures\n",failures);return failures?1:0;
}
