#pragma once
#include "audio/ShepherdAudio.h"

inline SoundHandle GameplayState::shepherdSound(const char* name,glm::vec3 at,float weight){
    return audio.playOpts(name,shepherdaudio::options(name,at,weight));
}
inline void GameplayState::stopShepherdAudio(){
    audio.stop(shepWindSound);shepWindSound=0;
    for(auto& h:shepStrainSound){audio.stop(h);h=0;}
}
