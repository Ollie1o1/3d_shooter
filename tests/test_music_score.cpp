// Exercise the authored arrangements through the real synth, including complete
// forms. These checks concern rendered sound, not just score-table contents.
#include "audio/MusicSynth.h"
#include <cstdio>
#include <chrono>

static int failures=0;
static void check(bool ok,const char* message){
    std::printf("%s %s\n",ok?"ok:":"FAIL:",message);failures+=!ok;
}
static double power(const std::vector<float>& x,int first,int last){
    double sum=0;for(int i=first*2;i<last*2;++i)sum+=double(x[i])*x[i];
    return sum/((last-first)*2);
}
static std::vector<float> render(int track,int rate,float seconds,bool mute=false,int chunk=512){
    MusicSynth m;m.setSampleRate(float(rate));m.setTrack(track);m.setVolume(.9f);
    m.setIntensity(track==0?1.f:1.3f);m.setThemeMuted(mute);
    const int frames=int(seconds*rate);std::vector<float> x(frames*2);
    for(int p=0;p<frames;p+=chunk)m.render(x.data()+p*2,std::min(chunk,frames-p));
    return x;
}
int main(){
    auto start=std::chrono::steady_clock::now();double renderedSeconds=0;
    for(int track:{0,10,11}){
        for(int rate:{22050,44100,48000}){
            const float bar=240.f/musicTrack(track).bpm;
            const int bars=combatscore::bars(track);
            const float seconds=(bars+2)*bar+.1f; // include the form's return
            auto x=render(track,rate,seconds);renderedSeconds+=seconds;
            bool finite=true;float peak=0;
            for(float v:x){finite&=std::isfinite(v);peak=std::max(peak,std::fabs(v));}
            check(finite && peak<.9f,"complete form and return remain finite with PCM headroom");
            const int peakBar=track==0?9:track==10?22:18;
            const int breakBar=track==0?12:track==10?12:8;
            auto sectionPower=[&](int b){return power(x,int((b*bar+.25f)*rate),int(((b+2)*bar-.15f)*rate));};
            const double contrast=10*std::log10(sectionPower(peakBar)/sectionPower(breakBar));
            std::printf("  %s %d Hz: peak/breakdown %.2f dB; PCM peak %.3f\n",musicTrack(track).name,rate,contrast,peak);
            check(contrast>1.5,"rendered breakdown leaves audible dynamic room for the peak");
            // Chunk sizes must not alter pitches, note gates or form boundaries.
            auto other=render(track,rate,seconds,false,997);renderedSeconds+=seconds;
            check(x==other,"audio is identical across irregular callback buffer sizes");
        }
        const int rate=44100;const float seconds=12;
        auto full=render(track,rate,seconds),backing=render(track,rate,seconds,true);
        renderedSeconds+=seconds*2;double diff=0;
        for(size_t i=rate*2;i<full.size();++i){double d=full[i]-backing[i];diff+=d*d;}
        diff=std::sqrt(diff/(full.size()-rate*2));
        std::printf("  %s theme contribution RMS %.4f\n",musicTrack(track).name,diff);
        check(diff>.006,"theme contributes to the rendered combat mix at gameplay intensity");
    }
    // Pause must still remove high-frequency energy from the new themes.
    for(int track:{0,10,11}){
        MusicSynth a,b;for(auto* m:{&a,&b}){m->setTrack(track);m->setIntensity(1.3f);m->setVolume(.9f);}
        std::vector<float> x(44100*2),y(x.size());
        for(int i=0;i<4;++i){a.render(x.data(),44100);b.render(y.data(),44100);}
        b.setMuffle(true);
        for(int i=0;i<2;++i){a.render(x.data(),44100);b.render(y.data(),44100);}
        double dry=0,paused=0;
        for(size_t i=2;i<x.size();++i){dry+=std::pow(x[i]-x[i-2],2);paused+=std::pow(y[i]-y[i-2],2);}
        check(paused<dry*.5,"pause still suppresses high-frequency energy in the new arrangements");
        // A long-form score must not inherit the previous boss's position.
        b.setTrack(MENU_MUSIC_TRACK);b.render(y.data(),44100);
        b.setTrack(track);b.render(y.data(),44100);
        check(b.currentTrack()==track && b.scoreBar()<1,"return from menu restarts the authored form");
        // Rewind just after a four-bar boundary used to wrap step forward by
        // 3.5 bars while leaving the new long-form cycle counter unchanged.
        MusicSynth tape;tape.setTrack(track);tape.setIntensity(1.3f);
        const int beforeFrames=int((240.f/musicTrack(track).bpm*4+.2f)*44100);
        std::vector<float> tapeAudio(beforeFrames*2);tape.render(tapeAudio.data(),beforeFrames);
        const double before=(tape.pass()-1)*64+tape.stepPos();
        tape.rewind();tape.render(y.data(),int(.6f*44100));
        const double after=(tape.pass()-1)*64+tape.stepPos();
        check(!tape.rewinding() && std::fabs(before-after-8)<.01,"rewind crosses a four-bar boundary without jumping ahead in the form");
    }
    double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::printf("%.0f seconds of complete-form audio rendered in %.2fs; %d failures\n",renderedSeconds,elapsed,failures);
    return failures?1:0;
}
