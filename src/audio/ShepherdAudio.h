#pragma once
#include "audio/MixTable.h"
#include "audio/SfxMixer.h"
#include "bosses/shepherd/ShepherdCombat.h"

namespace shepherdaudio {
inline const char* attackCue(int attack) {
    using namespace shephead;
    switch(attack){
    case SLAM:case SLAM2:return "shep_fist_tell";
    case SWEEP:return "shep_sweep_tell";
    case GRAB:return "shep_grab_tell";
    case GAZE:case BEAM:return "shep_gaze_tell";
    case PULL:case AIM:return "shep_gather";
    // Memory and volley cues belong to their individual records/releases.
    case ECHO:case ORRERY:return nullptr;
    default:return nullptr;
    }
}
inline SfxMixer::Opts options(const char* name,glm::vec3 at,float weight=1.f){
    SfxMixer::Opts o;
    const auto* entry=mixEntry(name);
    const bool tell=entry&&entry->cls==MixClass::TELL;
    o.group=tell?SoundGroup::ENEMY:SoundGroup::WORLD;
    o.role=tell?SoundRole::TELL:SoundRole::ACTION;
    o.positional=true;o.pos=at;o.floor=tell?.8f:.65f;
    o.priority=true;o.unducked=true;o.volume=weight;
    return o;
}
}
