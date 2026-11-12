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
    for(int track:{0,9,10}){
        for(int rate:{22050,44100,48000}){
            const float bar=240.f/musicTrack(track).bpm;
            const int bars=combatscore::bars(track);
            const float seconds=(bars+2)*bar+.1f; // include the form's return
            auto x=render(track,rate,seconds);renderedSeconds+=seconds;
            bool finite=true;float peak=0;
            for(float v:x){finite&=std::isfinite(v);peak=std::max(peak,std::fabs(v));}
            check(finite && peak<.9f,"complete form and return remain finite with PCM headroom");
            auto sectionPower=[&](int b){return power(x,int((b*bar+.25f)*rate),int(((b+2)*bar-.15f)*rate));};
            if(track==0){
                const double contrast=10*std::log10(sectionPower(9)/sectionPower(12));
                std::printf("  SUNSET %d Hz: peak/breakdown %.2f dB; PCM peak %.3f\n",rate,contrast,peak);
                check(contrast>1.5,"rendered breakdown leaves audible dynamic room for the peak");
            }else{
                // A boss never lets up: sections change rhythm (techno, DnB, hybrid,
                // hardstyle) at full speed and level, with no half-time lull.
                double lo=1e9,hi=0;
                for(int b=0;b<bars;b+=2){double p=sectionPower(b);lo=std::min(lo,p);hi=std::max(hi,p);}
                const double spread=10*std::log10(hi/lo);
                std::printf("  %s %d Hz: loudest/quietest two bars %.2f dB; PCM peak %.3f\n",musicTrack(track).name,rate,spread,peak);
                check(spread<5.5,"no two bars of a boss's form fall far below its loudest");
            }
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
    // The boss forms use every rhythm section, the hardstyle bars only at the top of a fight,
    // and at boss intensity the other break and techno tracks trade bars with the other style.
    for(int track:{9,10}){
        bool techno=false,dnb=false,hybrid=false,hard=false,half=false;
        for(int b=0;b<32;++b){
            const auto f=combatscore::form(track,b);using combatscore::Style;
            techno|=f.style==Style::TECHNO;dnb|=f.style==Style::DNB;hybrid|=f.style==Style::HYBRID;hard|=f.style==Style::HARD;
            int kicks=0;for(int s=0;s<16;++s)kicks+=combatscore::hit(track,b,s,0)=='x';
            half|=kicks<2;
        }
        check(techno && dnb && hybrid && hard && !half,"boss form moves through techno, DnB, hybrid and hardstyle without a half-time bar");
        // Just under and just over the threshold: the same score everywhere except the hardstyle bars.
        auto take=[&](float intensity){
            MusicSynth m;m.setTrack(track);m.setVolume(.9f);m.setIntensity(intensity);
            const int n=int(25*240.f/musicTrack(track).bpm*44100);
            std::vector<float> x(n*2);m.render(x.data(),n);return x;
        };
        const auto under=take(1.14f),over=take(1.15f);
        const float bar=240.f/musicTrack(track).bpm;
        auto diff=[&](int b0,int b1){double d=0;for(int i=int(b0*bar*44100)*2;i<int(b1*bar*44100)*2;++i){double e=over[i]-under[i];d+=e*e;}return d/((b1-b0)*bar*88200);};
        // (bars before the first hardstyle section: after it the drums' noise sequence differs anyway)
        const double ratio=10*std::log10(diff(4,8)/diff(1,4));
        std::printf("  %s: hardstyle bars change %.1f dB more than their neighbours across the threshold\n",musicTrack(track).name,ratio);
        check(ratio>6,"hardstyle kick only takes over at the top of the fight");
    }
    for(int track:{4,7,15}){
        auto bars=[&](float intensity){
            MusicSynth m;m.setTrack(track);m.setVolume(.9f);m.setIntensity(intensity);
            const float bar=240.f/musicTrack(track).bpm;const int n=int(16*bar*44100);
            std::vector<float> x(n*2);m.render(x.data(),n);return x;
        };
        const auto calm=bars(1.f),boss=bars(1.35f);
        const float bar=240.f/musicTrack(track).bpm;
        auto diff=[&](int b0,int b1){double d=0;for(int i=int(b0*bar*44100)*2;i<int(b1*bar*44100)*2;++i){double e=boss[i]-calm[i];d+=e*e;}return d/((b1-b0)*bar*88200);};
        check(diff(4,16)>diff(1,4)*1.5,"boss intensity trades bars 5-16 for hardstyle and the other rhythm section");
    }
    // Pause must still remove high-frequency energy from the new themes.
    for(int track:{0,9,10}){
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
