#pragma once
#include "audio/ShepherdAudio.h"

inline SoundHandle GameplayState::shepherdSound(const char* name,glm::vec3 at,float weight){
    return audio.playOpts(name,shepherdaudio::options(name,at,weight));
}
inline void GameplayState::stopShepherdAudio(){
    audio.stop(shepWindSound,.012f);shepWindSound=0;
    for(auto& h:shepStrainSound){audio.stop(h,.012f);h=0;}
    for(auto& h:shepMotionSound){audio.stop(h,.08f);h=0;}
    for(auto& t:shepMotionCooldown)t=0.f;
    shepAudioPoseReady=false;
}
inline void GameplayState::shepherdHitSound(glm::vec3 at,float damage){
    if(shepHitSoundT>0.f||damage<=0.f)return;
    shepHitSoundT=.085f; // aggregate pellets, but keep every ordinary rifle hit
    shepherdSound(damage>=60.f?"shep_wound_heavy":"shep_wound",at,damage>=60.f?1.f:.85f);
}
inline void GameplayState::updateShepherdMotionAudio(float dt){
    if(playerDead||shepHeadHp<=0.f||level.arenaAt(player.position)!=10||dt<=0.f)return;
    const glm::vec3 now[]={glm::vec3(shepChestPose()*glm::vec4(shephead::chest(shepHeadRise),1)),shepHandPose(0),shepHandPose(1)};
    const bool attacking=shepAtk!=shephead::NONE||shepRiseT>=0.f||shepRearT>=0.f||shepGrabT>=0.f;
    int candidate=-1;float strongest=0.f;
    for(int k=0;k<3;++k){
        shepMotionCooldown[k]=std::max(0.f,shepMotionCooldown[k]-dt);
        if(attacking){audio.stop(shepMotionSound[k],.035f);shepMotionSound[k]=0;}
        if(shepAudioPoseReady&&!attacking){
            const float speed=glm::length(now[k]-shepAudioPose[k])/dt;
            const bool planted=k>0&&(shepArmDown[k-1]>0.f||shepPinT[k-1]>0.f);
            const float strength=speed/(k?24.f:5.f);
            if(!planted&&strength>1.f&&strength>strongest&&shepMotionCooldown[k]<=0.f){candidate=k;strongest=strength;}
            if(shepMotionSound[k]&&int(shepMotionClock*10)!=int((shepMotionClock-dt)*10))audio.moveSource(shepMotionSound[k],now[k]);
        }
        shepAudioPose[k]=now[k];
    }
    if(candidate>=0){
        // One short mechanism at a time, never a repeating three-voice bed.
        for(auto& h:shepMotionSound){audio.stop(h,.035f);h=0;}
        shepMotionSound[candidate]=shepherdSound(candidate?"shep_lift":"shep_motion",now[candidate],candidate?.65f:.55f);
        for(auto& t:shepMotionCooldown)t=6.5f;
    }
    shepAudioPoseReady=true;
}
