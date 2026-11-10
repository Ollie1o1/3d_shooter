// Render the actual live score to a stereo WAV for auditioning outside the game.
// Usage: music_preview TRACK SECONDS OUT.wav [intensity] [--rate HZ]
//        [--drive] [--theme-muted] [--exercise]
#include "audio/MusicSynth.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <cstdint>
int main(int argc,char** argv){
    if(argc<4){std::fprintf(stderr,"usage: music_preview TRACK SECONDS OUT.wav [intensity] [--rate HZ] [--drive] [--theme-muted] [--exercise]\n");return 2;}
    int track=std::atoi(argv[1]),rate=44100,nextArg=4;
    const double seconds=std::atof(argv[2]);float intensity=1.3f;
    if(nextArg<argc && argv[nextArg][0]!='-')intensity=std::atof(argv[nextArg++]);
    bool drive=false,muted=false,exercise=false;
    for(int i=nextArg;i<argc;++i){
        if(!std::strcmp(argv[i],"--rate") && i+1<argc)rate=std::atoi(argv[++i]);
        else if(!std::strcmp(argv[i],"--drive"))drive=true;
        else if(!std::strcmp(argv[i],"--theme-muted"))muted=true;
        else if(!std::strcmp(argv[i],"--exercise"))exercise=true;
        else return 2;
    }
    if(track<0||track>=MUSIC_TRACKS||!std::isfinite(seconds)||seconds<=0||seconds>600||
       !std::isfinite(intensity)||intensity<0||intensity>1.5f||rate<22050||rate>96000)return 2;
    const int frames=(int)(rate*seconds);
    if(frames<1)return 2;
    MusicSynth m;m.setSampleRate(float(rate));m.setTrack(track);m.setVolume(.9f);
    m.setIntensity(intensity);m.setDrive(drive);m.setThemeMuted(muted);
    std::vector<float> samples((size_t)frames*2);
    const int events[]={8,16,20,24,28,32,36,40};int event=0;
    for(int pos=0;pos<frames;){
        if(exercise && event<8 && pos==events[event]*rate){
            switch(event++){
                case 0:m.setDrive(true);break;
                case 1:m.setDrive(false);m.setSolo(true);break;
                case 2:m.setSolo(false);m.stinger();break;
                case 3:m.setMuffle(true);break;
                case 4:m.setMuffle(false);break;
                case 5:m.rewind();break;
                case 6:m.setTrack(MENU_MUSIC_TRACK);break;
                case 7:m.setTrack(track);break;
            }
        }
        int count=std::min(256,frames-pos);
        if(exercise && event<8)count=std::min(count,events[event]*rate-pos);
        m.render(samples.data()+pos*2,count);pos+=count;
    }
    std::ofstream out(argv[3],std::ios::binary);auto u16=[&](uint16_t x){char b[]={char(x),char(x>>8)};out.write(b,2);};auto u32=[&](uint32_t x){u16(x&65535);u16(x>>16);};
    out.write("RIFF",4);u32(36+frames*4);out.write("WAVEfmt ",8);u32(16);u16(1);u16(2);u32(rate);u32(rate*4);u16(4);u16(16);out.write("data",4);u32(frames*4);
    double sum=0;float peak=0;for(float s:samples){if(!std::isfinite(s))return 1;peak=std::max(peak,std::fabs(s));sum+=s*s;u16((uint16_t)(int16_t)(std::clamp(s,-1.f,1.f)*32767));}
    std::printf("%s: %.1fs, %d Hz, RMS %.3f, peak %.3f\n",musicTrack(track).name,frames/float(rate),rate,std::sqrt(sum/samples.size()),peak);return out?0:1;
}
