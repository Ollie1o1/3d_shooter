#pragma once
#include "world/Level.h"

inline LevelData buildTraining() {
    LevelData L; LevelBuilder b{L}; b.mat=Mat::CONCRETE;
    const glm::vec3 floor{.23f,.29f,.32f}, wall{.12f,.18f,.22f}, cyan{.2f,1.1f,1.4f}, gold{1.2f,.7f,.15f};
    b.wall(-25,-1,-25,25,0,25,floor);
    b.wall(-26,0,-26,-25,12,26,wall); b.wall(25,0,-26,26,12,26,wall);
    b.wall(-25,0,-26,25,12,-25,wall); b.wall(-25,0,25,25,12,26,wall);
    b.mat=Mat::METAL;
    b.wall(-4,0,-16,4,4,-12,wall); // a gunner's ledge, reachable by a double jump
    b.wall(12,0,-18,20,5,-10,wall);
    b.wall(-18,0,-8,-13,1.4f,-3,wall);
    b.wall(-19,0,-18,-14,3,-13,wall);
    b.wall(-19,9,-20,20,9.5f,-19,wall); // a real grapple rail
    b.neon(-24,.03f,-24,-23.8f,.06f,24,cyan);
    b.neon(23.8f,.03f,-24,24,.06f,24,cyan);
    b.neon(-4,4.02f,-12.2f,4,4.08f,-12,cyan);
    b.neon(12,5.02f,-10.2f,20,5.08f,-10,gold);
    b.neon(-19,8.9f,-19.2f,20,9,-19,cyan);
    for(float z=-20;z<24;z+=4)b.prop(-.08f,.01f,z,.08f,.02f,z+1,{.65f,.72f,.75f});
    Arena a;
    a.name="COMBAT PRACTICE";a.subtitle="REPEAT / EXPERIMENT / IMPROVE";
    a.zone=LevelBuilder::aabb(-25,0,-25,25,24,25);a.bounds=a.zone;
    a.playerStart={0,0,12};a.startYaw=-90;a.scripted=true;a.ambient=Ambient::DUST;
    a.theme=Theme{{.025f,.045f,.08f},{.15f,.25f,.32f},{.08f,.1f,.13f},
        glm::normalize(glm::vec3{.3f,.8f,-.4f}),{1,.85f,.65f},.04f,0,
        {.06f,.1f,.16f},.2f,glm::normalize(glm::vec3{.3f,-.8f,.4f}),{.85f,.95f,1.f},
        {.5f,.57f,.65f},{.22f,.25f,.29f},{.1f,.16f,.22f},.004f};
    L.arenas.push_back(a);
    return L;
}
