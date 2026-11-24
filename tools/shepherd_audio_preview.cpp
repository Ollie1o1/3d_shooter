// Offline audition through the real score, effects mixer and Overdrive bed.
#include "audio/MusicSynth.h"
#include "audio/ShepherdAudio.h"
#include "audio/RingAudio.h"
#include "audio/DriveBed.h"
#include "audio/MasterBus.h"
static MasterBus gBus;   // the game's own last stage (AudioSystem)
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
struct Event{float at;const char* name;glm::vec3 pos;float weight=1.f;};
static const Event EVENTS[]={
    {.4f,"shep_motion",{0,70,-180},.55f},
    {2,"shep_fist_tell",{-20,35,-40}},{3.38f,"shep_parry",{-6,0,-20}},{3.65f,"shep_contact",{-6,0,-20}},
    {4.1f,"shep_pin",{-6,0,-20}},{4.1f,"shep_strain",{-6,0,-20},.65f},{5,"shep_counter",{0,40,-200}},
    {6,"shep_wound",{0,70,-180},.85f},{6.2f,"shep_wound",{0,70,-180},.85f},
    {7,"shep_sweep_tell",{-70,6,-40}},{8.25f,"shep_sweep",{0,6,-40}},{9.3f,"shep_sweep_tell",{70,6,-20}},{9.65f,"shep_sweep",{0,6,-20}},
    {10,"shep_wound_heavy",{0,70,-180}},
    {11,"shep_grab_tell",{30,30,-50}},{11.65f,"shep_lock",{5,0,-20}},{12.63f,"shep_parry",{5,0,-20}},{12.85f,"shep_grip",{5,0,-20}},
    {14,"shep_lift",{30,40,-80},.65f},
    {15,"shep_gaze_tell",{0,120,-200}},{16.25f,"shep_lock",{0,0,-15}},{17.05f,"shep_gaze_fire",{0,0,-15},.7f},
    {19,"shep_memory",{-10,0,-20}},{19.65f,"shep_memory",{0,0,-20}},{20.3f,"shep_memory",{10,0,-20}},
    {20.8f,"shep_contact",{-10,0,-20}},{21.45f,"shep_contact",{0,0,-20}},{22.1f,"shep_contact",{10,0,-20}},
    {23,"shep_moon",{-35,90,-160},.65f},{23.32f,"shep_moon",{0,100,-160},.65f},{23.64f,"shep_moon",{35,90,-160},.65f},
    {24,"shep_gather",{0,40,-200}},{26,"shep_wound_heavy",{0,70,-180}},{27,"shep_break",{-70,3,-20}},{30,"shep_seal",{0,0,-20}},{31.5f,"shep_implode",{0,0,-20}}
};
static bool writeWav(const char* path,const std::vector<float>& out,int rate){
    const int frames=int(out.size()/2);
    std::ofstream f(path,std::ios::binary);auto u16=[&](uint16_t x){char b[]={char(x),char(x>>8)};f.write(b,2);};auto u32=[&](uint32_t x){u16(x&65535);u16(x>>16);};
    f.write("RIFF",4);u32(36+frames*4);f.write("WAVEfmt ",8);u32(16);u16(1);u16(2);u32(rate);u32(rate*4);u16(4);u16(16);f.write("data",4);u32(frames*4);
    float peak=0;double energy=0,mono=0;
    for(size_t i=0;i<out.size();++i){const float v=out[i];if(!std::isfinite(v))return false;peak=std::max(peak,std::fabs(v));energy+=v*v;u16(uint16_t(int16_t(std::clamp(v,-1.f,1.f)*32767)));if(i%2==0){float m=(v+out[i+1])*.5f;mono+=m*m;}}
    const double monoDb=10*std::log10(std::max(1e-12,2*mono/energy));
    std::printf("%s: %.1f seconds, peak %.3f, RMS %.4f, mono change %.2f dB\n",path,float(frames)/rate,peak,std::sqrt(energy/out.size()),monoDb);
    return bool(f)&&peak<.69f&&monoDb> -4.f;
}
static bool render(bool stress){
    constexpr int SR=44100,FRAMES=42*SR;
    MusicSynth music;music.setSampleRate(SR);music.setTrack(SHEPHERD_MUSIC_TRACK);music.setShepherdMovement(shepherdscore::CONTROL);music.setIntensity(1.3f);music.setVolume(.6f);
    SfxMixer mixer(SR,2197);DriveBed drive;
    mixer.setListener({0,1.7f,0},{1,0,0},{0,0,-1});mixer.setVacuum(1);
    for(const auto& entry:mixTable())if(std::string(entry.name).find("shep_")==0||!std::strcmp(entry.name,"revolver")||!std::strcmp(entry.name,"shotgun")||!std::strcmp(entry.name,"kar")||!std::strcmp(entry.name,"step_metal")){
        for(int v=0;v<3;++v){std::string name=std::string(entry.name)+(v?"_"+std::to_string(v):"");
            auto x=read(name.c_str());if(x.empty()){if(!v){std::fprintf(stderr,"missing %s\n",entry.name);return false;}break;}
            mixer.addSound(entry.name,std::move(x),SR);}
    }
    std::vector<float> out(FRAMES*2),block(256*2);size_t event=0;float nextShot=2,nextFoot=.4f;bool focused=false;SoundHandle strain=0;
    for(int pos=0;pos<FRAMES;){
        const float t=float(pos)/SR;
        if(t>=nextFoot&&t<29){nextFoot+=.38f;SfxMixer::Opts o;o.volume=.5f*ringaudio::FOOTSTEP_GAIN*mixGain("step_metal");mixer.play("step_metal",o);}
        music.setShepherdMovement(t<10?shepherdscore::CONTROL:t<20?shepherdscore::RESISTANCE:t<30?shepherdscore::COLLAPSE:shepherdscore::LAST_COUNT);
        while(event<std::size(EVENTS)&&EVENTS[event].at<=t){auto e=EVENTS[event++];auto o=shepherdaudio::options(e.name,e.pos,e.weight);o.volume*=mixGain(e.name);
            if(!std::strcmp(e.name,"shep_counter"))mixer.stop(strain,.012f);
            auto h=mixer.play(e.name,o);if(!std::strcmp(e.name,"shep_strain"))strain=h;}
        if(stress&&t>=nextShot&&t<29){const char* gun=t<10?"revolver":t<18?"shotgun":"kar";nextShot+=t<10?.22f:t<18?.72f:.2f;SfxMixer::Opts o;o.volume=mixGain(gun);o.priority=true;mixer.play(gun,o);}
        const bool focus=stress&&t>=18&&t<25;
        if(focus!=focused){focused=focus;mixer.setFocus(focus?1.f:0.f);music.setDrive(focus);drive.set(focus,0);}
        if(t>=31.5f){music.setVolume(0.f);music.setDrive(false);drive.set(false,0);}
        int n=std::min(256,FRAMES-pos);
        // Split callbacks at cue times; no warning is quantized to a music beat.
        if(event<std::size(EVENTS))n=std::min(n,std::max(1,int(std::ceil(EVENTS[event].at*SR))-pos));
        music.render(block.data(),n);mixer.render(block.data(),n);
        drive.render(block.data(),n,SR,float(music.rootNote()),music.samplesPerStep(),music.stepPos(),1.f);
        for(int i=0;i<2*n;++i)if(!std::isfinite(block[i]))return false;
        gBus.process(block.data(),n);for(int i=0;i<2*n;++i)out[pos*2+i]=block[i];
        pos+=n;
    }
    const char* path=stress?"build/shepherd-audio/shepherd-stress.wav":"build/shepherd-audio/shepherd-mix.wav";
    return writeWav(path,out,SR);
}
static bool crossing(){
    constexpr int SR=44100,FRAMES=22*SR;
    MusicSynth music;music.setTrack(RING_MUSIC_TRACK);music.setIntensity(.38f);music.setVolume(.6f*.55f);
    SfxMixer mixer(SR,2197);mixer.setVacuum(1);mixer.setListener({0,1.7f,0},{1,0,0},{0,0,-1});
    for(const char* name:{"ring_knock","ring_creak","ring_sweep_warn","ring_sweep_pass","ring_debris","count_tick"}){
        auto x=read(name);if(x.empty())return false;mixer.addSound(name,std::move(x),SR);
    }
    const Event events[]={{1,"ring_knock",{0,0,0}},{4,"ring_creak",{1,1,0}},{8,"ring_sweep_warn",{}},
        {10,"count_tick",{}},{11,"count_tick",{}},{12,"count_tick",{}},{13,"count_tick",{}},{14,"count_tick",{}},{15,"count_tick",{}},
        {16,"ring_sweep_pass",{}},{16.4f,"ring_debris",{}},{19,"ring_knock",{0,0,0}}};
    std::vector<float> out(FRAMES*2);size_t event=0;
    for(int p=0;p<FRAMES;){
        while(event<std::size(events)&&events[event].at*SR<=p){auto e=events[event++];
            auto o=!std::strcmp(e.name,"ring_sweep_warn")?ringaudio::warning():ringaudio::contact(e.pos);
            if(!std::strcmp(e.name,"count_tick")){o=SfxMixer::Opts{};o.group=SoundGroup::UI;}
            o.volume*=mixGain(e.name);mixer.play(e.name,o);
        }
        int n=std::min(256,FRAMES-p);if(event<std::size(events))n=std::min(n,std::max(1,int(events[event].at*SR)-p));
        music.render(out.data()+2*p,n);mixer.render(out.data()+2*p,n);
        gBus.process(out.data()+2*p,n);
        p+=n;
    }
    return writeWav("build/shepherd-audio/ring-crossing.wav",out,SR);
}
static bool vocabulary(){
    constexpr int SR=44100,FRAMES=34*SR;
    SfxMixer mixer(SR,2197);mixer.setVacuum(1);mixer.setListener({0,1.7f,0},{1,0,0},{0,0,-1});
    for(const auto& entry:mixTable())if(std::string(entry.name).find("shep_")==0){auto x=read(entry.name);if(x.empty())return false;mixer.addSound(entry.name,std::move(x),SR);}
    const Event cues[]={
        {.5f,"shep_motion",{-5,2,-10},.55f},
        {4,"shep_fist_tell",{-20,35,-40}},{5.38f,"shep_parry",{-6,0,-20}},{5.65f,"shep_contact",{-6,0,-20}},
        {9,"shep_sweep_tell",{-70,6,-40}},{10.25f,"shep_sweep",{0,6,-40}},
        {14,"shep_grab_tell",{30,30,-50}},{14.65f,"shep_lock",{5,0,-20}},{15.63f,"shep_parry",{5,0,-20}},{15.85f,"shep_grip",{5,0,-20}},
        {19,"shep_gaze_tell",{0,120,-200}},{20.25f,"shep_lock",{0,0,-15}},{21.05f,"shep_gaze_fire",{0,0,-15},.7f},
        {24,"shep_gather",{0,40,-200}},{27.2f,"shep_contact",{0,40,-200}},
        {30,"shep_pin",{-6,0,-20}},{30,"shep_strain",{-6,0,-20},.65f},{30.9f,"shep_counter",{0,40,-200}}
    };
    std::vector<float> out(FRAMES*2);size_t event=0;
    for(int p=0;p<FRAMES;){
        while(event<std::size(cues)&&cues[event].at*SR<=p){auto e=cues[event++];auto o=shepherdaudio::options(e.name,e.pos,e.weight);o.volume*=mixGain(e.name);mixer.play(e.name,o);}
        int n=std::min(256,FRAMES-p);if(event<std::size(cues))n=std::min(n,std::max(1,int(std::lround(cues[event].at*SR))-p));
        mixer.render(out.data()+2*p,n);gBus.process(out.data()+2*p,n);p+=n;
    }
    std::ofstream labels("build/shepherd-audio/attack-language.md");
    labels<<"# Shepherd attack language\n\nNo music, footsteps or weapon fire. Quiet gaps separate each family.\n\n0.5 s: idle latch (quiet)\n4 s: fist — two piston knocks, white-window chime, drum/steel contact\n9 s: sweep — accelerating chain winch, torn teeth and carriage thud\n14 s: grab — three closing finger latches, lock, grip\n19 s: gaze — glassy electrical dyad, lock, discharge\n24 s: gather — three descending pressure knocks, hull contact\n30 s: pin — spike bite, beating tension, counterweight fracture\n";
    return writeWav("build/shepherd-audio/attack-language.wav",out,SR);
}
int main(){return render(false)&&render(true)&&crossing()&&vocabulary()?0:1;}
