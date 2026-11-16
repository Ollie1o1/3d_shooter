#pragma once
// Small shared VFX primitives. The Shepherd's anatomy is authored in Blender
// (assets/props/shepherd_*.glb), loaded through bosses/shepherd/ShepherdModel.h.
#include "render/MeshLib.h"
#include <glm/glm.hpp>
#include <cmath>
#include <vector>

namespace headmesh {

inline void smoothNormals(meshlib::MeshData& m) {
    m.nrm.assign(m.pos.size(), glm::vec3(0.f));
    for (size_t i = 0; i + 2 < m.idx.size(); i += 3) {
        const glm::vec3 a = m.pos[m.idx[i]], b = m.pos[m.idx[i + 1]], c = m.pos[m.idx[i + 2]];
        const glm::vec3 n = glm::cross(b - a, c - a);
        for (int k = 0; k < 3; ++k) m.nrm[m.idx[i + k]] += n;
    }
    for (auto& n : m.nrm) { const float l = glm::length(n); n = l > 1e-12f ? n / l : glm::vec3(0.f, 1.f, 0.f); }
}

// Latitude meshes duplicate their first longitude for indexing. Average the
// two normals so the middle of the face and the finger sides have no crease.
inline void stitchSeam(meshlib::MeshData& m,int u,int v) {
    for(int j=0;j<=v;++j){size_t a=j*(u+1),b=a+u;auto n=m.nrm[a]+m.nrm[b];float len=glm::length(n);if(len>1e-8f)m.nrm[a]=m.nrm[b]=n/len;}
    for(int row:{0,v}){glm::vec3 n{0};for(int i=0;i<=u;++i)n+=m.nrm[row*(u+1)+i];float len=glm::length(n);if(len>1e-8f)for(int i=0;i<=u;++i)m.nrm[row*(u+1)+i]=n/len;}
}

inline int orb() {
    static int id=0; if(id)return id;
    meshlib::MeshData m; constexpr int U=32,V=24;
    for(int j=0;j<=V;++j)for(int i=0;i<=U;++i){
        float v=3.14159265f*j/V,u=6.2831853f*i/U;
        glm::vec3 n{std::sin(v)*std::sin(u),std::cos(v),std::sin(v)*std::cos(u)};
        m.pos.push_back(n*.5f);m.nrm.push_back(n);
    }
    for(int j=0;j<V;++j)for(int i=0;i<U;++i){uint32_t a=j*(U+1)+i,b=a+1,c=a+U+1,d=c+1;m.idx.insert(m.idx.end(),{a,c,b,b,c,d});}
    meshlib::centre(m);id=meshlib::add(std::move(m));return id;
}
inline int capsule() {
    static int id = 0;
    if (id) return id;
    const int NU = 16, NV = 16;
    meshlib::MeshData m;
    for (int j = 0; j <= NV; ++j) {   // a sphere split at its middle and pulled apart along Z
        const float v = 3.14159265f * (float)j / NV;
        for (int i = 0; i <= NU; ++i) {
            const float u = 6.2831853f * (float)i / NU;
            glm::vec3 p{0.5f * std::sin(v) * std::cos(u), 0.5f * std::sin(v) * std::sin(u), 0.5f * std::cos(v)};
            p.z = p.z * 0.6f + (p.z >= 0.f ? 0.2f : -0.2f);
            m.pos.push_back(p);
        }
    }
    for (int j = 0; j < NV; ++j)
        for (int i = 0; i < NU; ++i) {
            const uint32_t a = j * (NU + 1) + i, b = a + 1, c = a + (NU + 1), e = c + 1;
            m.idx.insert(m.idx.end(), {a, c, b, b, c, e});
        }
    smoothNormals(m);
    stitchSeam(m,NU,NV);
    meshlib::centre(m);
    id = meshlib::add(std::move(m));
    return id;
}

}  // namespace headmesh
