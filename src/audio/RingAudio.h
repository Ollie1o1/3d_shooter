#pragma once
#include "audio/SfxMixer.h"

namespace ringaudio {
constexpr float FOOTSTEP_GAIN=.25f, LANDING_GAIN=.5f;
inline SfxMixer::Opts warning() {
    SfxMixer::Opts o;
    o.group=SoundGroup::WORLD;o.role=SoundRole::TELL;
    o.priority=true;o.unducked=true;
    return o;
}
inline SfxMixer::Opts contact(glm::vec3 at) {
    SfxMixer::Opts o;
    o.group=SoundGroup::WORLD;o.role=SoundRole::CHATTER;
    o.positional=true;o.pos=at;
    return o;
}
}
