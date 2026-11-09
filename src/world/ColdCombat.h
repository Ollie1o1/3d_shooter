#pragma once
#include "world/SpawnSafety.h"
#include <array>

// Authored combat furniture. Included after LevelCold.h's location definitions.
namespace coldcombat {
inline void chillerDeck(LevelBuilder& b,int k) {
    using glm::vec3;
    const auto c=cold::chiller(k).c;
    const vec3 iron{.22f,.27f,.31f},frost{.62f,.7f,.77f},dark{.065f,.09f,.12f},amber{.65f,.32f,.07f};
    b.mat=Mat::METAL;
    // A second service roof, connected above the fan. Both approaches are
    // walkable ramps; the floor below remains tall enough to cross freely.
    b.wall(c.x+7,2.9f,c.z-1.5f,c.x+15,3.2f,c.z+3,frost*.65f);
    for(float x:{7.f,14.6f})for(float z:{-1.5f,2.6f})
        b.wall(c.x+x,0,c.z+z,c.x+x+.4f,2.9f,c.z+z+.4f,iron);
    b.wall(c.x-9,2.9f,c.z-1.6f,c.x+8,3.2f,c.z+.4f,iron);
    b.kit().box(c+vec3{11,3.24f,.75f},{8.2f,.08f,4.7f},frost);
    b.kit().box(c+vec3{-.5f,3.24f,-.6f},{17,.08f,2},frost*.8f);
    const AABB ramp=LevelBuilder::aabb(c.x-1.3f,0,c.z+3,c.x+1.3f,3.2f,c.z+13);
    b.L.ramps.push_back({LevelBuilder::aabb(c.x-1.3f,0,c.z+2.4f,c.x+1.3f,3.2f,c.z+3.7f),2,3.2f,3.2f});
    b.L.ramps.push_back({ramp,2,3.2f,0});
    b.kit().box(c+vec3{0,1.45f,8},{std::hypot(10.f,3.2f),.3f,2.6f},iron,-1.5707963f,0,-std::atan2(3.2f,10.f));
    // Solid landings at each end of the slope, including the full body width.
    b.wall(c.x-1.3f,2.9f,c.z-.5f,c.x+1.3f,3.2f,c.z+3.1f,iron);
    b.wall(c.x-1.4f,-.5f,c.z+12.8f,c.x+1.4f,0,c.z+14,iron);
    b.kit().box(c+vec3{0,3.24f,1.3f},{2.6f,.08f,3.6f},frost*.8f);
    for(float x:{7.1f,14.8f})b.neon(c.x+x,3.24f,c.z-1.3f,c.x+x+.1f,3.3f,c.z+2.8f,amber);
    for(float z:{-1.5f,.3f})b.neon(c.x-8.8f,3.24f,c.z+z,c.x+7.8f,3.3f,c.z+z+.08f,amber*.7f);
    for(int f=0;f<8;++f)b.kit().box(c+vec3{7.5f+f,.9f,-1.55f},{.1f,1.5f,.12f},dark);
    b.neon(c.x+8,.8f,c.z+3.02f,c.x+13,1.f,c.z+3.08f,amber*.7f);
    // Deep compressor banks interrupt the north/south shooting lanes. Their
    // rounded feed pipes and pressure gauges explain the cover's silhouette.
    for(vec3 local:{vec3{8,0,10},vec3{-10,0,-9}}) {
        const auto p=c+local;
        b.wall(p.x-3.4f,0,p.z-1.4f,p.x+3.4f,2.2f,p.z+1.4f,iron);
        b.kit().box(p+vec3{0,2.26f,0},{7,.12f,3},frost);
        for(float x:{-2.6f,0.f,2.6f}) {
            b.kit().rod(p+vec3{x,.4f,1.5f},p+vec3{x,1.9f,1.5f},.15f,dark,8);
            b.kit().column(p+vec3{x,2.3f,0},.3f,.16f,frost,10);
        }
        b.neon(p.x-2.8f,1.5f,p.z+1.43f,p.x+2.8f,1.66f,p.z+1.49f,amber*.8f);
    }
    b.wall(c.x-7,0,c.z+13,c.x-1,1.6f,c.z+14.5f,iron);
    b.kit().box(c+vec3{-4,1.55f,13.75f},{6.15f,.1f,1.65f},frost);
    for(float x:{-6.8f,-1.3f})b.neon(c.x+x,.1f,c.z+13,c.x+x+.1f,1.6f,c.z+13.08f,amber*.6f);
    // Low, sparse route studs lead across the actual walkways, leaving the
    // red vent rings and the player's cyan distinct from ambient machinery.
    for(int n=0;n<5;++n)b.kit(true).box(c+vec3{-3.5f+n*1.7f,.04f,4.2f},{.32f,.04f,.14f},amber*.5f);
}

inline void dispatchYard(LevelBuilder& b) {
    using glm::vec3;
    const auto c=cold::station();const vec3 iron{.19f,.24f,.3f},frost{.68f,.76f,.82f},amber{.7f,.36f,.08f};
    b.mat=Mat::METAL;
    // A loading balcony with an approach at either end, below the grapple-only
    // canopy and signal gantry. The west entrance and east exit stay open.
    b.wall(c.x-24.5f,0,c.z-6,c.x-22.5f,3.2f,c.z+7,iron);
    for(bool north:{true,false}) {
        float z0=north?-14.f:7.f,z1=north?-6.f:15.f;
        b.L.ramps.push_back({LevelBuilder::aabb(c.x-24.5f,0,c.z+z0,c.x-22.5f,3.2f,c.z+z1),2,north?0.f:3.2f,north?3.2f:0.f});
        b.kit().box(c+vec3{-23.5f,1.45f,(z0+z1)*.5f},{std::hypot(8.f,3.2f),.3f,2},iron,-1.5707963f,0,(north?1.f:-1.f)*std::atan2(3.2f,8.f));
    }
    b.neon(c.x-24.45f,3.23f,c.z-5.9f,c.x-24.35f,3.3f,c.z+6.9f,amber*.7f);
    // Two offset cargo islands let you break the Lancer's aim and change side.
    // Their low step is reachable from the platform, and the top is standable.
    for(float side:{-1.f,1.f}) {
        const auto p=c+vec3{side*10.5f,.6f,2.f};
        b.wall(p.x-2.5f,p.y,p.z-1.7f,p.x+2.5f,p.y+1.8f,p.z+1.7f,iron);
        b.wall(p.x-2.3f,p.y,p.z+1.7f,p.x+2.3f,p.y+.65f,p.z+2.8f,iron*.8f);
        b.kit().box(p+vec3{0,1.85f,0},{5.15f,.1f,3.55f},frost);
        for(float x:{-2.f,0.f,2.f})b.kit().box(p+vec3{x,.9f,-1.76f},{.14f,1.7f,.12f},frost*.65f);
        b.neon(p.x-2.3f,p.y+1.3f,p.z+1.72f,p.x+2.3f,p.y+1.45f,p.z+1.78f,amber*.55f);
    }
    // An ordinary loading ramp makes the platform's 60 cm lip readable and
    // traversable without a compulsory hop on every crossing.
    b.L.ramps.push_back({LevelBuilder::aabb(c.x-3,0,c.z+10,c.x+3,.6f,c.z+15),2,.6f,0});
    b.kit().box(c+vec3{0,.18f,12.5f},{std::hypot(5.f,.6f),.24f,6},iron,-1.5707963f,0,-std::atan2(.6f,5.f));
    for(float side:{-1.f,1.f})b.neon(c.x+side*2.8f,.05f,c.z+13,c.x+side*2.8f+.08f,.09f,c.z+14.6f,amber*.5f);
}

inline std::array<AABB,7> vaultLowCover() {
    const auto c=cold::vaultC();std::array<AABB,7> out;
    for(int k=0;k<3;++k) {
        const glm::vec3 p=c+(k==0?glm::vec3{-24,0,0}:k==1?glm::vec3{24,0,0}:glm::vec3{0,0,24});
        const glm::vec3 h=k<2?glm::vec3{.8f,0,4}:glm::vec3{4,0,.8f};
        out[k]={p-h,p+h+glm::vec3{0,1.6f,0}};
    }
    for(int k=0;k<4;++k){const auto p=cold::drum(k);out[k+3]={p-glm::vec3{1.3f,0,1.3f},p+glm::vec3{1.3f,1.4f,1.3f}};}
    return out;
}

inline void vault(LevelBuilder& b) {
    using glm::vec3;const auto c=cold::vaultC();
    const vec3 iron{.17f,.23f,.28f},frost{.65f,.74f,.81f},amber{.65f,.3f,.06f};
    b.mat=Mat::METAL;
    const auto covers=vaultLowCover();
    for(int k=0;k<3;++k) {
        const auto& box=covers[k];b.wall(box.min.x,0,box.min.z,box.max.x,box.max.y,box.max.z,iron);
        const auto p=(box.min+box.max)*.5f;
        const bool eastWest=k<2;
        b.kit().box(p+vec3{0,.74f,0},box.max-box.min+vec3{.12f,-1.48f,.12f},frost);
        for(int n=0;n<5;++n) {
            const auto q=p+(eastWest?vec3{0,0,-3.f+n*1.5f}:vec3{-3.f+n*1.5f,0,0});
            b.kit().box(q,eastWest?vec3{1.7f,1.4f,.1f}:vec3{.1f,1.4f,1.7f},frost*.7f);
        }
        const auto inner=p+(k==0?vec3{.85f,0,0}:k==1?vec3{-.85f,0,0}:vec3{0,0,-.85f});
        b.neon(inner.x-(eastWest?.03f:3.f),.9f,inner.z-(eastWest?3.f:.03f),inner.x+(eastWest?.03f:3.f),1.04f,inner.z+(eastWest?3.f:.03f),amber*.65f);
        // Stored cryo cassettes hang outside the playable deck, giving the
        // arena a worked-in edge while keeping the floor grid and drums clear.
        const auto outward=glm::normalize(glm::vec3{p.x-c.x,0,p.z-c.z});
        const auto tangent=vec3{-outward.z,0,outward.x};
        for(int n=-2;n<=2;++n) {
            auto q=c+outward*32.f+tangent*(n*2.6f);
            b.kit().rod(q+vec3{0,1,0},q+vec3{0,10,0},.09f,iron,8);
            b.kit().column(q+vec3{0,2,0},.75f,3.7f,iron,10);
            for(float y:{2.2f,5.3f})b.kit().column(q+vec3{0,y,0},.79f,.14f,frost,10);
        }
    }
    // A lit approach to the north dispatch rail, outside the dynamic rack field.
    for(float x:{-5.5f,5.5f})b.neon(c.x+x,.04f,c.z-28,c.x+x+.1f,.1f,c.z-20,amber*.5f);
}

// Rewind destinations must stay usable after the Restorer has placed racks
// and statues. Search locally, retaining the view and the attack's position cue.
inline bool vaultLanding(const LevelData& level,glm::vec3 wanted,float radius,float height,glm::vec3& result) {
    auto clear=[&](glm::vec3 p){return glm::length(glm::vec2{p.x-cold::vaultC().x,p.z-cold::vaultC().z})<cold::VAULT_R-radius-.3f &&
        p.y>=-.1f && p.y<18.f && spawnClearance(level,p,radius,height,true);};
    if(clear(wanted)){result=wanted;return true;}
    for(float r=.75f;r<=6.01f;r+=.75f)for(int k=0;k<16;++k) {
        const float angle=k*6.2831853f/16;auto p=wanted+glm::vec3{r*std::cos(angle),0,r*std::sin(angle)};
        p.y=level.groundAt(p.x,p.z,wanted.y+.1f);if(clear(p)){result=p;return true;}
    }
    return false;
}

inline bool chillerRecovery(const LevelData& level,glm::vec3 feet,float radius,float height) {
    if(feet.y<-.1f || feet.y>3.3f)return false;
    bool deck=false;
    for(int k=0;k<3;++k) {
        const auto d=cold::chiller(k);
        deck|=glm::length(glm::vec2{feet.x-d.c.x,feet.z-d.c.z})<d.r-3.f;
    }
    if(!deck)return false;
    for(int k=0;k<cold::VENTS;++k) {
        const auto v=cold::vent(k);
        if(glm::length(glm::vec2{feet.x-v.x,feet.z-v.z})<cold::VENT_R+radius+1.f)return false;
    }
    return spawnClearance(level,feet,radius,height,true);
}

// Pursuit follows the two catwalks' centre line through their shared junction.
// A player on the same deck remains the direct target; this only supplies a
// movement waypoint, never a false attack target.
inline bool chillerRoute(glm::vec3 from,glm::vec3 target,glm::vec3& via) {
    auto nearestDeck=[](glm::vec3 p) {
        int best=0;float d=1e9f;
        for(int k=0;k<3;++k){auto q=p-cold::chiller(k).c;q.y=0;float n=glm::dot(q,q);if(n<d){d=n;best=k;}}
        return best;
    };
    const int dest=nearestDeck(target);
    const auto home=cold::chiller(dest);
    if(nearestDeck(from)==dest && glm::length(glm::vec2{from.x-home.c.x,from.z-home.c.z})<home.r-3.f)return false;
    int link=0;float t=0,best=1e9f;
    for(int k=0;k<2;++k) {
        auto a=cold::chiller(k).c,d=cold::chiller(k+1).c-a;
        auto flat=from-a;flat.y=0;
        float u=glm::clamp(glm::dot(flat,d)/glm::dot(d,d),0.f,1.f);
        auto delta=flat-d*u;float n=glm::dot(delta,delta);
        if(n<best){best=n;link=k;t=u;}
    }
    const bool forward=dest>link;
    int next=forward?link+1:link;
    // Turn at the junction itself, rather than cutting from mid-catwalk
    // toward the far deck across the unsupported inside corner.
    if(forward&&t>.96f&&next<dest)++next;
    if(!forward&&t<.04f&&next>dest)--next;
    via=cold::chiller(next).c;
    return true;
}

// The warning on a low cover top is clipped to the dangerous grid cell, so
// standing on cover never conceals the FLASH FREEZE tell beneath your feet.
inline bool freezeTop(glm::vec3 cell,const AABB& cover,AABB& top) {
    const float half=2.35f;
    top={{std::max(cell.x-half,cover.min.x),cover.max.y+.04f,std::max(cell.z-half,cover.min.z)},
         {std::min(cell.x+half,cover.max.x),cover.max.y+.08f,std::min(cell.z+half,cover.max.z)}};
    return top.max.x-top.min.x>.06f && top.max.z-top.min.z>.06f;
}
}
