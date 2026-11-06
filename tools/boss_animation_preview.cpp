#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
// CPU silhouette preview of the actual game meshes and joint transforms; no graphics context.
#include "core/Balance.h"
#include "render/EnemyModel.h"
#include <fstream>
#include <array>
#include <string>
using glm::vec3;using glm::vec4;
constexpr int W=400,H=400;
void preview(const Enemy& e,const std::string& path){
    std::vector<BoxInstance> parts;rig::buildEnemy(e,e.age,parts);
    std::vector<vec3> pixels(W*H,vec3{.035f,.045f,.065f});std::vector<float> depth(W*H,1e20f);
    const float height=e.stats().height;
    const auto target=vec3{0,height*.48f,0};
    const auto V=glm::lookAt(target+vec3{4.8f,2.05f,6.8f},target,vec3{0,1,0});
    const float zoom=260.f/height;
    auto project=[&](vec3 p){vec3 v=vec3(V*vec4(p,1));return vec3{W*.5f+v.x*zoom,H*.5f-v.y*zoom,-v.z};};
    auto edge=[](vec3 a,vec3 b,vec3 p){return (p.x-a.x)*(b.y-a.y)-(p.y-a.y)*(b.x-a.x);};
    auto tri=[&](vec3 a,vec3 b,vec3 c,vec3 col,vec3 emi){
        vec3 n=glm::cross(b-a,c-a);if(glm::length(n)<1e-8f)return;n=glm::normalize(n);
        vec3 color=col*(.3f+.7f*std::fabs(glm::dot(n,glm::normalize(vec3{-.5f,1,.7f}))))+emi*.28f;
        color=glm::clamp(color,vec3{0},vec3{1});
        a=project(a);b=project(b);c=project(c);float area=edge(a,b,c);if(std::fabs(area)<.0001f)return;
        int x0=std::max(0,(int)std::floor(std::min({a.x,b.x,c.x}))),x1=std::min(W-1,(int)std::ceil(std::max({a.x,b.x,c.x})));
        int y0=std::max(0,(int)std::floor(std::min({a.y,b.y,c.y}))),y1=std::min(H-1,(int)std::ceil(std::max({a.y,b.y,c.y})));
        for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){vec3 p{ x+.5f,y+.5f,0};
            float u=edge(b,c,p)/area,v=edge(c,a,p)/area,w=1-u-v;if(u<0 || v<0 || w<0)continue;
            float z=u*a.z+v*b.z+w*c.z;int k=y*W+x;if(z<depth[k]){depth[k]=z;pixels[k]=color;}
        }
    };
    tri({-6,-.02f,-6},{6,-.02f,6},{6,-.02f,-6},{.075f,.09f,.115f},{0,0,0});
    tri({-6,-.02f,-6},{-6,-.02f,6},{6,-.02f,6},{.075f,.09f,.115f},{0,0,0});
    const vec3 cube[]={{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,.5f,-.5f},{-.5f,.5f,-.5f},{-.5f,-.5f,.5f},{.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f}};
    const int ids[]={0,1,2,0,2,3,4,6,5,4,7,6,0,4,5,0,5,1,3,2,6,3,6,7,0,3,7,0,7,4,1,5,6,1,6,2};
    for(auto& p:parts){
        auto transform=[&](vec3 v){return vec3(p.model*vec4(v,1));};
        if(const auto* m=meshlib::get((int)p.mesh)){
            for(size_t k=0;k+2<m->idx.size();k+=3)tri(transform(m->pos[m->idx[k]]),transform(m->pos[m->idx[k+1]]),transform(m->pos[m->idx[k+2]]),p.color,p.emissive);
        }else for(int k=0;k<36;k+=3)tri(transform(cube[ids[k]]),transform(cube[ids[k+1]]),transform(cube[ids[k+2]]),p.color,p.emissive);
    }
    std::ofstream f(path,std::ios::binary);f<<"P6\n"<<W<<" "<<H<<"\n255\n";for(auto c:pixels)for(int k=0;k<3;++k){unsigned char v=(unsigned char)(std::pow(c[k],1.f/2.2f)*255);f.write((char*)&v,1);}
}
int main(){
    balanceInit();
    const AttackKind moves[]={AttackKind::SU_SEVER,AttackKind::SU_PERFUSION,AttackKind::SU_LATTICE,AttackKind::SU_CASCADE,AttackKind::SU_SUTURE,AttackKind::SU_RECALL};
    for(int row=0;row<7;++row){
        Enemy e(row==0?EnemyType::ABSOLUTE:EnemyType::SUCCESSOR,{0,0,0});e.state=EnemyState::ACTIVE;e.spawnTimer=0;e.suPhase=2;e.suGait=0;
        const float keys[]={.0f,row==0?.65f:.42f,row==0?.84f:.70f,row==0?.91f:.81f,1.02f};int shot=0;
        for(int frame=0;frame<150 && shot<5;++frame){
            float time=frame/120.f;e.age=time;
            if(row==0){e.abArrive=Enemy::AB_ARRIVE-time/1.05f*Enemy::AB_ARRIVE;e.abArrive=std::max(.0001f,e.abArrive);rig::tickAbsoluteBody(e,1.f/120);}
            else{e.telegraphDuration=.8f;e.telegraphTimer=std::max(0.f,.8f-time);e.attack=time<.8f?moves[row-1]:AttackKind::NONE;if(time>=.8f){e.suLanded=moves[row-1];e.suSince=time-.8f;}rig::tickSuccessorBody(e,1.f/120);}
            if(time>=keys[shot]){preview(e,"build/boss-pose-"+std::to_string(row)+"-"+std::to_string(shot)+".ppm");++shot;}
        }
    }
}
