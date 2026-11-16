#pragma once
// Blender-authored pieces, kept on the live combat skeleton. The sculpt is
// near-black anatomy with cool undertones inside the Shepherd's black spacecraft hull.
#include "render/PropDraw.h"
namespace shepherdmodel {
inline const char* const PIECES[]={"shepherd_head","shepherd_torso","shepherd_shoulder",
    "shepherd_upperarm","shepherd_forearm","shepherd_palm","shepherd_finger"};
constexpr float HEAD_SCALE=64.f, TORSO_SCALE=70.f;
inline void preload(){for(const char* name:PIECES)propfile::get(name);}
inline void piece(std::vector<BoxInstance>& out,const char* name,const glm::mat4& m,glm::vec3 light,float glow=1.f){
    const size_t first=out.size();
    propdraw::push(out,name,m,light,glow,true);
    // Exported material tags distinguish carbon anatomy from the ship's armor.
    // Keep emissive eyes, seams and the magnetic palm target untouched.
    if(const auto* prop=propfile::get(name))for(size_t i=0;i<prop->model.parts.size();++i){
        const auto& p=prop->model.parts[i];
        if(p.chan<0)out[first+i].surface=p.tex>=0?12.f:std::string(name)=="shepherd_head"?13.f:11.f;
    }
}
// Bake the GLB node transforms into these cached vertex positions so every
// material shares the same anatomical spine coordinates in the vertex shader.
inline void torso(std::vector<BoxInstance>& out,const glm::mat4& m,glm::vec3 light,glm::vec4 pose){
    struct Part { int mesh; glm::vec3 color,k; int chan,tex; float glowScale; };
    static const std::vector<Part> pieces=[](){
        std::vector<Part> parts;
        if(const auto* prop=propfile::get("shepherd_torso"))for(const auto& p:prop->model.parts){
            const auto* source=meshlib::get(p.mesh);if(!source)continue;
            auto mesh=*source;auto normal=glm::transpose(glm::inverse(glm::mat3(p.rel)));
            for(size_t i=0;i<mesh.pos.size();++i){mesh.pos[i]=glm::vec3(p.rel*glm::vec4(mesh.pos[i],1));mesh.nrm[i]=glm::normalize(normal*mesh.nrm[i]);}
            parts.push_back({meshlib::addShared(std::move(mesh)),p.color,p.k,p.chan,p.tex,p.glowScale});
        }
        return parts;
    }();
    for(const auto& p:pieces){BoxInstance b;b.model=m;b.mesh=float(p.mesh);b.deform=pose;
        b.color=p.glowScale>=0.f?light*p.glowScale:p.color;
        b.emissive=p.chan==1?light*p.k:p.chan==0?p.k:glm::vec3{0};
        b.surface=p.chan<0?(p.tex>=0?12.f:11.f):0.f;out.push_back(b);
    }
}
inline void limb(std::vector<BoxInstance>& out,const char* name,glm::vec3 a,glm::vec3 b,float width,float depth,glm::vec3 light){
    const auto d=b-a;const float len=glm::length(d);if(len<.001f)return;
    piece(out,name,propdraw::along((a+b)*.5f,d)*glm::scale(glm::mat4{1.f},glm::vec3{width,depth,len}),light);
}
}
