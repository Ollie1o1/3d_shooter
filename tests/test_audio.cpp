// Offline audio regression: shipped combat transients and the real callback DSP.
#include "audio/ImpactAudio.h"
#include "audio/MixTable.h"
#include "audio/MusicSynth.h"
#include "audio/SfxMixer.h"
#include "audio/ShepherdAudio.h"
#include "audio/RingAudio.h"
#include <fstream>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <cassert>

static int failures = 0;
static void check(bool ok, const char* message) {
    std::printf("%s %s\n", ok ? "ok:" : "FAIL:", message);
    failures += !ok;
}
static std::vector<float> wav(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::vector<unsigned char> b((std::istreambuf_iterator<char>(f)), {});
    auto u16 = [&](size_t p) { return unsigned(b[p]) | (unsigned(b[p + 1]) << 8); };
    auto u32 = [&](size_t p) { return u16(p) | (u16(p + 2) << 16); };
    if (b.size() < 44 || std::memcmp(b.data(), "RIFF", 4) || std::memcmp(b.data() + 8, "WAVE", 4)) return {};
    bool format = false;
    for (size_t p = 12; p + 8 <= b.size();) {
        size_t len = u32(p + 4), start = p + 8;
        if (len > b.size() - start) return {};
        if (!std::memcmp(b.data() + p, "fmt ", 4) && len >= 16)
            format = u16(start) == 1 && u16(start + 2) == 1 && u32(start + 4) == 44100 && u16(start + 14) == 16;
        if (!std::memcmp(b.data() + p, "data", 4) && format) {
            std::vector<float> x(len / 2);
            for (size_t i = 0; i < x.size(); ++i) x[i] = (int16_t)u16(start + 2 * i) / 32768.f;
            return x;
        }
        p = start + len + (len & 1);
    }
    return {};
}

static void checkMenuPlayback() {
  const auto& menu = musicTrack(MENU_MUSIC_TRACK);
  assert(menu.root <= 45 && menu.bpm >= 140.f && menu.techno);
  for (int chord : menu.chords) assert(chord == 0);
  assert(!std::strchr(menu.bass, 'o') && !std::strchr(menu.bass, 'f'));
  MusicSynth m; std::vector<float> b(8820);
  for (int rate : {44100,48000}) {
    m.setSampleRate((float)rate);
    for (int t=0;t<MUSIC_TRACKS;++t) {
      m.setTrack(t);m.setIntensity(1.3f);m.setDrive(true);m.stinger();
      m.render(b.data(),4410); assert(m.currentTrack()==t);
      if(t != MENU_MUSIC_TRACK) m.rewind();
      m.render(b.data(),4410);
      m.setDrive(false);m.setSolo(false);m.setIntensity(.55f);m.setTrack(MENU_MUSIC_TRACK);
      m.render(b.data(),4410);
      assert(m.currentTrack()==MENU_MUSIC_TRACK && !m.rewinding());
      for(float v:b) assert(std::isfinite(v) && std::fabs(v)<=1.f);
      for(int k=0;k<130;++k) m.render(b.data(),4410);
      assert(m.currentTrack()==MENU_MUSIC_TRACK);
    }
  }
  std::printf("All %d tracks: menu transitions, rewind cancellation, and sustained playback passed at 44.1/48 kHz.\n",MUSIC_TRACKS);
}

int main() {
    checkMenuPlayback();
    bool shepherdAssets=true,shepherdLevels=true,shepherdOnset=true,shepherdVariants=true;
    int shepherdFamilies=0;
    for(const auto& entry:mixTable())if(std::string(entry.name).find("shep_")==0||std::string(entry.name).find("ring_")==0){
        ++shepherdFamilies;std::vector<float> previous;
        for(int v=0;v<3;++v){
            auto x=wav(std::string("assets/sfx/")+entry.name+(v?"_"+std::to_string(v):"")+".wav");
            if(x.empty()){shepherdAssets=false;continue;}
            shepherdAssets&=std::all_of(x.begin(),x.end(),[](float a){return std::isfinite(a)&&std::fabs(a)<.98f;})&&std::fabs(x.back())<.001f;
            shepherdLevels&=std::fabs(shortTermDb(x.data(),x.size(),44100)+entry.trimDb-mixTargetDb(entry.cls))<.25f;
            size_t onset=0;while(onset<x.size()&&std::fabs(x[onset])<.01f)++onset;
            shepherdOnset&=onset<221;shepherdVariants&=previous.empty()||x!=previous;previous=std::move(x);
        }
    }
    check(shepherdFamilies==30&&shepherdAssets&&shepherdLevels,"all 90 Chapter 6 cue assets leave headroom and meet their mix targets");
    check(shepherdOnset&&shepherdVariants,"Chapter 6 cues have immediate onset, distinct variants and clean tails");
    {
        bool rhythms=true;
        for(auto spec:{std::pair<const char*,int>{"shep_fist_tell",2},{"shep_sweep_tell",4},{"shep_grab_tell",3},{"shep_gaze_tell",2},{"shep_gather",3}})for(int v=0;v<3;++v){
            auto x=wav(std::string("assets/sfx/")+spec.first+(v?"_"+std::to_string(v):"")+".wav");
            std::vector<double> env;double peak=0;
            for(size_t p=0;p+220<=x.size();p+=220){double e=0;for(size_t i=p;i<p+220;++i)e+=x[i]*x[i];env.push_back(e);peak=std::max(peak,e);}
            int groups=0,last=-999;for(int i=0;i<int(env.size());++i)if(env[i]>peak*.2){if(i-last>4)++groups;last=i;}
            if(groups!=spec.second)std::printf("  signature %s variant %d: %d pulse groups, expected %d\n",spec.first,v,groups,spec.second);
            rhythms&=groups==spec.second;
        }
        check(rhythms,"warning variants preserve separate piston, winch, latch, glass and pressure-pulse rhythms rather than continuous swells");
        auto idle=shepherdaudio::options("shep_motion",{});
        check(idle.role==SoundRole::CHATTER&&!idle.priority&&!idle.unducked&&idle.floor<=.25f&&
              wav("assets/sfx/shep_motion.wav").size()<44100*.4&&wav("assets/sfx/shep_lift.wav").size()<44100*.55,
              "idle mechanisms are brief background foley, separate from protected attack tells");
    }
    {
        const auto launch=shepherdaudio::options("shep_moon",{});
        const auto strain=shepherdaudio::options("shep_strain",{});
        const auto reward=shepherdaudio::options("shep_counter",{});
        check(!launch.priority&&!launch.unducked&&launch.floor<.3f&&launch.volume<=.8f&&
              !strain.priority&&!strain.unducked&&strain.floor<=.3f&&strain.volume<=.75f&&reward.priority&&reward.unducked,
              "repeated launches and tension leave priority and level for warnings and counter rewards");
        check(ringaudio::warning().role==SoundRole::TELL&&ringaudio::warning().unducked&&
              ringaudio::contact({}).role==SoundRole::CHATTER&&!ringaudio::contact({}).priority,
              "ring sweep warnings cut through while capsule knocks and hull creaks step back");
        check(std::strcmp(shepherdaudio::attackCue(shephead::GAZE),shepherdaudio::attackCue(shephead::BEAM))!=0,
              "the locked gaze and travelling beam announce different responses");
    }
    {
        auto loudest=[](const std::vector<float>& x){double best=0;const size_t win=2205;
            for(size_t p=0;p+win<=x.size()/2;p+=win/4){double sum=0;for(size_t i=p;i<p+win;++i)sum+=(x[i*2]*x[i*2]+x[i*2+1]*x[i*2+1])*.5;best=std::max(best,sum/win);}return best;};
        auto render=[&](const char* name,SfxMixer::Opts o){SfxMixer m(44100,9217);m.addSound(name,wav(std::string("assets/sfx/")+name+".wav"));o.volume*=mixGain(name);m.setListener({0,0,0},{1,0,0},{0,0,-1});m.setVacuum(1);m.play(name,o);std::vector<float> x(44100*3*2);m.render(x.data(),44100*3);return loudest(x);};
        SfxMixer::Opts step;step.volume=64.f/128.f*ringaudio::FOOTSTEP_GAIN;
        const auto foot=render("step_metal",step);
        const auto smack=render("shep_contact",shepherdaudio::options("shep_contact",{0,30,240}));
        const auto wound=render("shep_wound",shepherdaudio::options("shep_wound",{0,30,240},.85f));
        check(10*std::log10(smack/foot)>12&&10*std::log10(wound/foot)>10,
              "distant giant smacks and damaging hits clearly outrank a maximum-speed close footstep in vacuum");
        bool body=true;for(const char* name:{"shep_contact","shep_counter","shep_break","shep_wound_heavy"}){
            auto x=wav(std::string("assets/sfx/")+name+".wav");double all=0,tail=0;for(size_t i=0;i<x.size();++i){all+=x[i]*x[i];if(i>=13230)tail+=x[i]*x[i];}body&=tail>all*.025;
        }
        check(body,"large impacts retain audible structural resonance after 300 ms instead of padded silence");
    }
    // A cancelled continuous sound releases its waveform rather than chopping
    // an arbitrary nonzero sample. Repeated death/reset calls cannot extend it.
    for(int rate:{22050,44100,48000}) {
        SfxMixer m(float(rate),1234);m.addSound("steady",std::vector<float>(rate,.25f));
        SfxMixer::Opts o;o.group=SoundGroup::UI;
        const auto h=m.play("steady",o);std::vector<float> b(256*2);
        m.render(b.data(),256);const float before=b[510];
        m.stop(h,.012f);std::fill(b.begin(),b.end(),0.f);m.render(b.data(),1);
        bool clean=std::fabs(b[0]-before)<.001f;float last=b[0];
        for(int i=1;i<int(rate*.02f);++i){
            m.stop(h,.012f);std::fill(b.begin(),b.end(),0.f);m.render(b.data(),1);
            clean&=std::isfinite(b[0])&&std::fabs(b[0]-last)<.002f;last=b[0];
        }
        check(clean&&!m.isPlaying(h),"12 ms cancellation is click-free and bounded under repeated stops at each device rate");
        o.delay=.2f;auto delayed=m.play("steady",o);m.stop(delayed,.012f);m.render(b.data(),256);
        check(!m.isPlaying(delayed),"cancelling a queued sound prevents its future onset entirely");
    }
    {
        auto clip=wav("assets/sfx/shep_fist_tell.wav");
        auto energy=[&](float vacuum,bool stress){
            SfxMixer m(44100,9217);m.addSound("shep_fist_tell",clip);
            m.setListener({0,0,0},{1,0,0},{0,0,-1});m.setVacuum(vacuum);
            if(stress){m.duck(18,.5f);m.setFocus(1);}
            auto o=shepherdaudio::options("shep_fist_tell",{0,0,240});m.play("shep_fist_tell",o);
            std::vector<float> b(512*2);double e=0;
            for(int k=0;k<60;++k){std::fill(b.begin(),b.end(),0);m.render(b.data(),512);for(float x:b)e+=x*x;}
            return e;
        };
        double normal=energy(0,false),vacuum=energy(1,true);
        check(normal>1&&std::fabs(vacuum-normal)<normal*.001,"a Shepherd warning remains audible 240 metres behind the listener through vacuum, ducking and Overdrive focus");
        check(shepherdaudio::options("shep_fist_tell",{}).role==SoundRole::TELL&&shepherdaudio::options("shep_counter",{}).floor>0,"Shepherd warnings and counter rewards use protected mixer roles and audibility floors");
    }
    bool assets = true, onset = true, levels = true, variants = true, tails = true;
    SfxMixer mixer;
    for (const char* name : {"sword_draw", "sword_swing", "sword_thrust", "sword_guard", "sword_hit",
                            "sword_heavy", "sword_pierce", "sword_block", "sword_break", "parry", "parry_heavy",
                            "hit_body", "hit_armor", "impact", "impact_metal", "impact_wood", "impact_ice"}) {
        std::vector<float> previous;
        for (int v = 0; v < 3; ++v) {
            auto x = wav("assets/sfx/" + std::string(name) + (v ? "_" + std::to_string(v) : "") + ".wav");
            if (x.empty()) { assets = false; continue; }
            assets &= std::all_of(x.begin(), x.end(), [](float f) { return std::isfinite(f) && std::fabs(f) < .98f; });
            size_t first = 0;
            while (first < x.size() && std::fabs(x[first]) < .01f) ++first;
            onset &= first < 44100 * .005; // No hidden pre-roll on a combat cue.
            const auto* mix = mixEntry(name);
            levels &= mix && std::fabs(shortTermDb(x.data(), x.size(), 44100) + mix->trimDb - mixTargetDb(mix->cls)) < .25f;
            variants &= previous.empty() || x != previous;
            tails &= std::fabs(x.back()) < .001f;
            previous = x;
            mixer.addSound(name, std::move(x));
        }
    }
    check(assets, "all 51 authored combat WAVs are valid and leave peak headroom");
    check(onset, "every combat cue starts within 5 ms");
    check(levels, "every variant matches its mix class within 0.25 dB");
    check(variants && tails, "variants differ and end without an abrupt sample edge");
    check(std::string(surfaceImpactSound(Mat::HULL)) == "impact_metal" &&
          std::string(surfaceImpactSound(Mat::SURGICAL)) == "impact_metal" &&
          std::string(surfaceImpactSound(Mat::SALVAGE)) == "impact_metal" &&
          std::string(surfaceImpactSound(Mat::WOOD)) == "impact_wood" &&
          std::string(surfaceImpactSound(Mat::ICE)) == "impact_ice" &&
          std::string(surfaceImpactSound(Mat::ROCK)) == "impact",
          "chapter materials retain distinct steel, wood, ice and stone impacts");
    // Ducking must create space without stealing or attenuating the player's parry.
    SfxMixer::Opts o; o.priority = true; o.unducked = true;
    auto h = mixer.play("parry_heavy", o);
    mixer.duck(6.f, .12f);
    std::vector<float> mix(4410 * 2, .15f);
    mixer.render(mix.data(), 4410);
    check(mixer.isPlaying(h) && mixer.duckLevel() < .53f, "heavy parry survives while the background dips 6 dB");
    {   // Chapter 6's vacuum: far enemy sounds go thin and dull; tells, what you touch and your own don't
        auto clip = wav("assets/sfx/impact.wav");
        auto energy = [&](float vac, float dist, SoundRole role, SoundGroup group) {
            SfxMixer m(44100.f, 1234u); m.addSound("impact", clip);
            m.setListener({0, 0, 0}, {1, 0, 0}, {0, 0, -1});
            m.setVacuum(vac);
            SfxMixer::Opts q; q.group = group; q.positional = true; q.pos = {0, 0, -dist}; q.role = role;
            m.play("impact", q);
            std::vector<float> out(8192 * 2, 0.f); double e = 0;
            for (int k = 0; k < 4; ++k) { std::fill(out.begin(), out.end(), 0.f); m.render(out.data(), 8192); for (float f : out) e += (double)f * f; }
            return e;
        };
        const double farAir = energy(0, 25, SoundRole::ACTION, SoundGroup::ENEMY), farVac = energy(1, 25, SoundRole::ACTION, SoundGroup::ENEMY);
        const double tellAir = energy(0, 25, SoundRole::TELL, SoundGroup::ENEMY), tellVac = energy(1, 25, SoundRole::TELL, SoundGroup::ENEMY);
        const double nearAir = energy(0, 1.5f, SoundRole::ACTION, SoundGroup::WORLD), nearVac = energy(1, 1.5f, SoundRole::ACTION, SoundGroup::WORLD);
        const double youAir = energy(0, 25, SoundRole::ACTION, SoundGroup::PLAYER), youVac = energy(1, 25, SoundRole::ACTION, SoundGroup::PLAYER);
        check(!clip.empty() && farVac < farAir * 0.3, "vacuum: a far enemy sound comes through thin (well under a third of its energy)");
        check(std::fabs(tellVac - tellAir) < tellAir * 1e-3 && std::fabs(nearVac - nearAir) < nearAir * 1e-3 && std::fabs(youVac - youAir) < youAir * 1e-3,
              "vacuum: tells, sounds within reach and your own are untouched");
    }
    // Exercise device rates, small callbacks, and all live transitions over multiple phrases.
    bool stable = true, clock = true;
    for (float sr : {22050.f, 44100.f, 48000.f}) {
        MusicSynth score; score.setSampleRate(sr); score.setVolume(1.f);
        std::vector<float> block(512 * 2);
        for (int t = 0; t < MUSIC_TRACKS; ++t) {
            score.setTrack(t); score.setIntensity(1.3f); score.setDrive(t % 2 != 0);
            int frames = (int)(sr * 4);
            for (int done = 0; done < frames; done += 512) {
                int n = std::min(512, frames - done);
                if (done == 512 * 80) { score.stinger(); score.setSolo(true); }
                if (done == 512 * 100) { score.setSolo(false); score.rewind(); }
                score.render(block.data(), n);
                bool clean = std::all_of(block.begin(), block.begin() + n * 2,
                    [](float f) { return std::isfinite(f) && std::fabs(f) <= 1.f; });
                if (!clean && stable) std::printf("bad sample: track %d at %.0f Hz, frame %d\n", t, sr, done);
                stable &= clean;
            }
            bool inTime = score.currentTrack() == t && std::fabs(score.samplesPerStep() - sr * 60.f / musicTrack(t).bpm / 4.f) < .01f;
            if (!inTime) std::printf("clock: requested %d, playing %d, %.0f Hz\n", t, score.currentTrack(), sr);
            clock &= inTime;
        }
    }
    check(stable && clock, "all tracks, stingers, OVERDRIVE, solo and rewind remain stable at 22.05/44.1/48 kHz");
    std::printf("%d audio failures\n", failures);
    return failures ? 1 : 0;
}
