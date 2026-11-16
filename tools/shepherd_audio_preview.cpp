// Offline audition through the real score, effects mixer and Overdrive bed.
#include "audio/MusicSynth.h"
#include "audio/ShepherdAudio.h"
#include "audio/DriveBed.h"
#include <fstream>
#include <iterator>
#include <cstdint>
#include <cstring>
#include <cstdio>

static std::vector<float> read(const char* name){
    std::ifstream f(std::string("assets/sfx/")+name+".wav",std::ios::binary);
    std::vector<unsigned char> b((std::istreambuf_iterator<char>(f)),{});
    auto u16=[&](size_t p){return unsigned(b[p])|(unsigned(b[p+1])<<8);};
    auto u32=[&](size_t p){return u16(p)|(u16(p+2)<<16);};
    if(b.size()<44||std::memcmp(b.data(),"RIFF",4))return {};
    bool format=false;
    for(size_t p=12;p+8<=b.size();){
        size_t n=u32(p+4),start=p+8;if(n>b.size()-start)return {};
        if(!std::memcmp(b.data()+p,"fmt ",4)&&n>=16)format=u16(start)==1&&u16(start+2)==1&&u32(start+4)==44100&&u16(start+14)==16;
        if(!std::memcmp(b.data()+p,"data",4)&&format){std::vector<float> x(n/2);for(size_t i=0;i<x.size();++i)x[i]=(int16_t)u16(start+2*i)/32768.f;return x;}
        p=start+n+(n&1);
    }
    return {};
}
struct Event{float at;const char* name;glm::vec3 pos;};
static const Event EVENTS[]={
    {2,"shep_fist_tell",{-20,35,-40}},{3.38f,"shep_parry",{-6,0,-20}},{3.65f,"shep_contact",{-6,0,-20}},
    {4.1f,"shep_pin",{-6,0,-20}},{4.1f,"shep_strain",{-6,0,-20}},{5,"shep_counter",{0,40,-200}},
    {7,"shep_sweep_tell",{-70,6,-40}},{8.25f,"shep_sweep",{0,6,-40}},{9.3f,"shep_sweep_tell",{70,6,-20}},{9.65f,"shep_sweep",{0,6,-20}},
    {11,"shep_grab_tell",{30,30,-50}},{11.65f,"shep_lock",{5,0,-20}},{12.63f,"shep_parry",{5,0,-20}},{12.85f,"shep_grip",{5,0,-20}},
    {15,"shep_gaze_tell",{0,120,-200}},{16.25f,"shep_lock",{0,0,-15}},{17.05f,"shep_sweep",{0,0,-15}},
    {19,"shep_memory",{-10,0,-20}},{19.65f,"shep_memory",{0,0,-20}},{20.3f,"shep_memory",{10,0,-20}},
    {20.8f,"shep_contact",{-10,0,-20}},{21.45f,"shep_contact",{0,0,-20}},{22.1f,"shep_contact",{10,0,-20}},
    {24,"shep_gather",{0,40,-200}},{27,"shep_break",{-70,3,-20}},{30,"shep_seal",{0,0,-20}},{31.5f,"shep_implode",{0,0,-20}}
};
static bool render(bool stress){
    constexpr int SR=44100,FRAMES=42*SR;
    MusicSynth music;music.setSampleRate(SR);music.setTrack(SHEPHERD_MUSIC_TRACK);music.setShepherdMovement(shepherdscore::CONTROL);music.setIntensity(1.3f);music.setVolume(.6f);
    SfxMixer mixer(SR,2197);DriveBed drive;
    mixer.setListener({0,1.7f,0},{1,0,0},{0,0,-1});mixer.setVacuum(1);
    for(const auto& entry:mixTable())if(std::string(entry.name).find("shep_")==0||!std::strcmp(entry.name,"revolver")){
        auto x=read(entry.name);if(x.empty()){std::fprintf(stderr,"missing %s\n",entry.name);return false;}
        mixer.addSound(entry.name,std::move(x),SR);
    }
    std::vector<float> out(FRAMES*2),block(256*2);size_t event=0;float nextShot=2;bool focused=false;SoundHandle strain=0;
    for(int pos=0;pos<FRAMES;){
        const float t=float(pos)/SR;
        music.setShepherdMovement(t<10?shepherdscore::CONTROL:t<20?shepherdscore::RESISTANCE:t<30?shepherdscore::COLLAPSE:shepherdscore::LAST_COUNT);
        while(event<std::size(EVENTS)&&EVENTS[event].at<=t){auto e=EVENTS[event++];auto o=shepherdaudio::options(e.name,e.pos);o.volume*=mixGain(e.name);
            if(!std::strcmp(e.name,"shep_counter"))mixer.stop(strain);
            auto h=mixer.play(e.name,o);if(!std::strcmp(e.name,"shep_strain"))strain=h;}
        if(stress&&t>=nextShot&&t<29){nextShot+=.22f;SfxMixer::Opts o;o.volume=mixGain("revolver");o.priority=true;mixer.play("revolver",o);}
        const bool focus=stress&&t>=18&&t<25;
        if(focus!=focused){focused=focus;mixer.setFocus(focus?1.f:0.f);music.setDrive(focus);drive.set(focus,0);}
        if(t>=39)music.setVolume(0.f);
        int n=std::min(256,FRAMES-pos);
        // Split callbacks at cue times; no warning is quantized to a music beat.
        if(event<std::size(EVENTS))n=std::min(n,std::max(1,int(std::ceil(EVENTS[event].at*SR))-pos));
        music.render(block.data(),n);mixer.render(block.data(),n);
        drive.render(block.data(),n,SR,float(music.rootNote()),music.samplesPerStep(),music.stepPos(),1.f);
        for(int i=0;i<2*n;++i){if(!std::isfinite(block[i]))return false;out[pos*2+i]=SfxMixer::softClip(block[i])*.7f;}
        pos+=n;
    }
    const char* path=stress?"build/shepherd-audio/shepherd-stress.wav":"build/shepherd-audio/shepherd-mix.wav";
    std::ofstream f(path,std::ios::binary);auto u16=[&](uint16_t x){char b[]={char(x),char(x>>8)};f.write(b,2);};auto u32=[&](uint32_t x){u16(x&65535);u16(x>>16);};
    f.write("RIFF",4);u32(36+FRAMES*4);f.write("WAVEfmt ",8);u32(16);u16(1);u16(2);u32(SR);u32(SR*4);u16(4);u16(16);f.write("data",4);u32(FRAMES*4);
    float peak=0;for(float v:out){peak=std::max(peak,std::fabs(v));u16(uint16_t(int16_t(std::clamp(v,-1.f,1.f)*32767)));}
    std::printf("%s: 42 seconds, peak %.3f\n",path,peak);return bool(f)&&peak<.71f;
}
int main(){return render(false)&&render(true)?0:1;}
