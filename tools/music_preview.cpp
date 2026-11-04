// Render the actual live score to a stereo WAV for auditioning outside the game.
// Usage: music_preview TRACK SECONDS OUT.wav [intensity]
#include "audio/MusicSynth.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <cstdint>
int main(int argc,char** argv){
    if(argc<4){std::fprintf(stderr,"usage: music_preview TRACK SECONDS OUT.wav [intensity]\n");return 2;}
    int track=std::atoi(argv[1]),frames=(int)(44100*std::atof(argv[2]));
    if(track<0||track>=MUSIC_TRACKS||frames<1||frames>44100*600)return 2;
    MusicSynth m;m.setTrack(track);m.setVolume(.9f);m.setIntensity(argc>4?std::atof(argv[4]):1.3f);
    std::vector<float> samples((size_t)frames*2);m.render(samples.data(),frames);
    std::ofstream out(argv[3],std::ios::binary);auto u16=[&](uint16_t x){char b[]={char(x),char(x>>8)};out.write(b,2);};auto u32=[&](uint32_t x){u16(x&65535);u16(x>>16);};
    out.write("RIFF",4);u32(36+frames*4);out.write("WAVEfmt ",8);u32(16);u16(1);u16(2);u32(44100);u32(44100*4);u16(4);u16(16);out.write("data",4);u32(frames*4);
    double sum=0;float peak=0;for(float s:samples){if(!std::isfinite(s))return 1;peak=std::max(peak,std::fabs(s));sum+=s*s;u16((uint16_t)(int16_t)(std::clamp(s,-1.f,1.f)*32767));}
    std::printf("%s: %.1fs, RMS %.3f, peak %.3f\n",musicTrack(track).name,frames/44100.f,std::sqrt(sum/samples.size()),peak);return out?0:1;
}
