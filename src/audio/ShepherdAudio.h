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
    case GAZE:return "shep_gaze_tell";
    case BEAM:return "shep_beam_tell";
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
    const std::string cue=name;
    const bool reward=cue=="shep_pin"||cue=="shep_counter"||cue=="shep_break"||cue=="shep_seal"||cue=="shep_implode";
    const bool texture=cue=="shep_strain";
    const bool launch=cue=="shep_moon";
    const bool motion=cue=="shep_motion"||cue=="shep_lift";
    const bool impact=cue=="shep_contact"||cue=="shep_grip"||cue=="shep_sweep"||cue=="shep_wound"||cue=="shep_wound_heavy"||cue=="shep_gaze_fire"||cue=="shep_beam";
    if(motion)o.role=SoundRole::CHATTER;
    o.positional=true;o.pos=at;
    o.floor=tell?.8f:impact?.85f:reward?.85f:motion?.25f:texture?.3f:launch?.25f:.5f;
    o.priority=tell||reward||impact;
    o.unducked=tell||reward||impact;
    // Repeated launches and the pin's friction support the attack. They must
    // leave room for the lock/parry cue and the actual heart-impact reward.
    o.volume=weight*(texture?.75f:launch?.8f:1.f);
    return o;
}
}
