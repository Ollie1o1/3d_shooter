#pragma once
#include <algorithm>
#include <cstddef>

// Authored music, expressed in sixteenths rather than a chord-tone character
// loop. Evaluated on the sequencer clock; no allocation or random choices.
namespace combatscore {
constexpr int SUNSET=0, ZENITH=10, MORNINGSTAR=11;
inline bool owns(int track){return track==SUNSET || track==ZENITH || track==MORNINGSTAR;}
inline int bars(int track){return track==SUNSET?16:32;}
enum class Section { STATEMENT, ANSWER, BREAKDOWN, BUILD, PEAK, TURN };
// How a boss section is played. The bosses never drop to half-time: contrast
// comes from changing the rhythm section at full speed instead of the level.
//   TECHNO  four-on-the-floor driven kick, clap, rolling off-beat bass
//   DNB     two-step break, snare with ghosts, sustained Reese bass
//   HYBRID  the four-on-the-floor kick under the break's snares and Reese
//   HARD    hardstyle: a pitched, distorted kick whose tail is the bass, clap, screech lead.
//           Only at the top of a fight (MusicSynth hardAmt); below that it plays as TECHNO.
enum class Style { TECHNO, DNB, HYBRID, HARD };
inline const char* name(Section s){
    switch(s){case Section::STATEMENT:return "STATEMENT";case Section::ANSWER:return "ANSWER";
    case Section::BREAKDOWN:return "BREAKDOWN";case Section::BUILD:return "BUILD";
    case Section::PEAK:return "PEAK";case Section::TURN:return "TURN";}return "";
}
struct Form {
    Section section=Section::STATEMENT;
    int degree=0;
    float drums=1, bass=1, arp=1, theme=1, filter=1, rise=0;
    Style style=Style::TECHNO;
};
inline Form form(int track,int absoluteBar,float withinBar=0){
    const int b=std::max(0,absoluteBar)%bars(track);
    withinBar=std::clamp(withinBar,0.f,1.f);
    Form f;
    if(track==SUNSET){
        static constexpr int chords[]={0,5,2,6,0,3,5,6,0,5,2,6,3,3,5,6};
        f.degree=chords[b];f.arp=.6f;f.theme=.9f;
        if(b>=4 && b<8)f.section=Section::ANSWER;
        if(b>=8 && b<12){f.section=Section::PEAK;f.theme=1;}
        if(b>=12 && b<14){f.section=Section::BREAKDOWN;f.drums=.78f;f.bass=.6f;f.arp=.2f;f.theme=.55f;f.filter=.55f;}
        if(b>=14){f.section=Section::BUILD;f.rise=(b-14+withinBar)/2;f.arp=.4f+.3f*f.rise;}
    }else{
        static constexpr int zen[]={0,0,5,6,0,3,5,4,0,0,5,6,3,3,5,4,
                                     0,0,5,4,0,5,3,4,0,6,5,4,3,3,5,4};
        static constexpr int morning[]={0,0,6,5,0,3,5,4,3,3,6,4,0,0,5,4,
                                         0,6,5,4,0,3,5,4,3,6,5,4,0,0,5,4};
        f.degree=track==ZENITH?zen[b]:morning[b];
        if(track==ZENITH){
            if(b<4){f.theme=.75f;f.filter=.6f;}
            else if(b<8){f.section=Section::ANSWER;f.filter=.85f;}
            else if(b<16){f.section=Section::PEAK;f.style=Style::DNB;}
            else if(b<18){f.section=Section::BUILD;f.rise=(b-16+withinBar)/2;f.bass=.75f+.25f*f.rise;f.filter=.5f+.5f*f.rise;f.theme=.8f;}
            else if(b<26){f.section=Section::PEAK;if(b>=22)f.style=Style::HARD;}
            else if(b<31){f.section=Section::PEAK;f.style=Style::HYBRID;}
            else {f.section=Section::TURN;f.style=Style::HYBRID;f.rise=withinBar;}
        }else{
            if(b<4){f.section=Section::ANSWER;f.style=Style::HYBRID;f.filter=.85f;}
            else if(b<8){f.section=Section::PEAK;f.style=Style::HYBRID;}
            else if(b<16){f.section=Section::PEAK;f.style=Style::DNB;}
            else if(b<18){f.section=Section::BUILD;f.rise=(b-16+withinBar)/2;f.bass=.8f+.2f*f.rise;f.filter=.5f+.5f*f.rise;}
            else if(b<26){f.section=Section::PEAK;if(b>=20)f.style=Style::HARD;}
            else if(b<30){f.section=Section::PEAK;f.style=Style::DNB;}
            else {f.section=Section::BUILD;f.style=Style::DNB;f.rise=(b-30+withinBar)/2;f.filter=.6f+.4f*f.rise;}
        }
    }
    return f;
}
struct Note { int at, pitch, length; float velocity; bool slide=false; };
// Pitch is a semitone offset from the home key, not the current chord: a
// recognisable melody survives the changing harmony. An eight-bar response
// resolves onto the leading tone over the boss's dominant in bar eight.
inline Note theme(int track,int absoluteBar,int step){
    static constexpr Note sunset[]={
        {0,0,3,.86f},{3,3,2,.74f},{6,7,4,1},{12,5,3,.78f},
        {18,3,3,.8f},{24,2,3,.7f},{28,0,3,.88f},
        {32,0,3,.83f},{36,3,2,.7f},{39,7,5,1},{46,10,2,.78f},
        {48,7,4,.9f},{54,5,3,.77f},{60,2,3,.78f},
        {64,3,4,.85f},{70,7,3,.94f},{76,12,3,.88f},
        {80,10,4,.82f},{86,7,3,.78f},{92,5,3,.72f},
        {96,3,3,.82f},{100,2,2,.7f},{103,0,5,.93f},
        {114,2,3,.76f},{120,0,6,.9f}};
    static constexpr Note zenith[]={
        {0,0,3,.9f},{3,3,2,.72f},{6,7,4,1},{12,5,3,.8f},
        {18,3,3,.82f},{24,2,3,.72f},{28,0,3,.9f},
        {32,0,3,.87f},{36,3,2,.74f},{39,7,5,1},{46,10,2,.8f},
        {48,7,5,.9f},{56,5,3,.78f},{62,3,2,.74f},
        {64,12,4,.96f},{70,10,3,.82f},{76,7,3,.88f},
        {80,5,4,.82f},{86,7,3,.88f},{92,3,3,.77f},
        {96,5,5,.87f},{104,3,3,.76f},{110,2,2,.72f},
        {112,7,5,.88f},{120,11,6,.9f}};
    static constexpr Note morning[]={
        {0,0,3,.95f},{3,3,2,.78f},{6,7,3,1},{10,2,2,.8f},{14,0,2,.86f},
        {18,10,3,.86f},{24,7,3,.9f},{28,5,3,.82f},
        {32,0,3,.94f},{36,3,2,.78f},{39,7,4,1},{44,12,3,.92f},
        {48,10,4,.86f},{54,7,3,.9f},{60,5,3,.82f},
        {64,12,4,1},{70,10,3,.88f},{76,7,3,.94f},
        {80,5,4,.84f},{86,3,3,.82f},{92,2,3,.8f},
        {96,0,5,.95f},{104,3,3,.8f},{110,5,2,.78f},
        {112,7,5,.92f},{120,11,6,.95f}};
    const auto section=form(track,absoluteBar).section;
    // A held fragment followed by a full bar of space in the breakdown.
    if(section==Section::BREAKDOWN){
        if(absoluteBar%2==0 && step==0)return {0,track==SUNSET?0:7,10,.72f};
        return {-1,0,0,0};
    }
    const int position=(absoluteBar%8)*16+step;
    const Note* notes=track==SUNSET?sunset:track==ZENITH?zenith:morning;
    const size_t count=track==SUNSET?sizeof(sunset)/sizeof(Note):track==ZENITH?sizeof(zenith)/sizeof(Note):sizeof(morning)/sizeof(Note);
    for(size_t i=0;i<count;++i)if(notes[i].at==position)return notes[i];
    return {-1,0,0,0};
}
inline Note acid(int track,int absoluteBar,int step){
    static constexpr int z[4][16]={
        {0,-1,0,7,12,-1,7,-1,3,-1,0,7,-1,10,7,-1},
        {0,-1,7,-1,3,7,-1,12,10,-1,7,-1,5,3,-1,2},
        {0,0,-1,7,12,-1,10,7,3,-1,7,-1,12,10,7,-1},
        {7,-1,7,12,11,-1,7,-1,2,-1,7,11,-1,12,11,-1}};
    static constexpr int m[4][16]={
        {0,-1,0,12,3,-1,7,-1,0,2,-1,7,10,-1,7,2},
        {0,0,-1,7,12,-1,10,7,3,-1,7,12,10,-1,7,-1},
        {0,-1,7,12,3,7,-1,10,0,-1,2,7,-1,12,10,7},
        {7,-1,11,12,7,-1,2,7,11,-1,7,12,11,-1,7,-1}};
    int row=absoluteBar%4;
    const auto f=form(track,absoluteBar);
    if(f.section==Section::ANSWER)row=(row+1)%4;
    const int p=(track==ZENITH?z:m)[row][step];
    if(p<0 || (f.section==Section::BREAKDOWN && step%4!=0))return {-1,0,0,0};
    return {step,p,step%4==0?2:1,step%4==0?1.f:.68f,step==3 || step==7 || step==14};
}
inline char hit(int track,int bar,int step,int instrument,bool hard=true){
    auto f=form(track,bar);
    if(f.style==Style::HARD){
        if(!hard)f.style=Style::TECHNO;
        else{   // the last bar rolls the kick into the next section
            const bool last=form(track,bar+1).style!=Style::HARD;
            if(instrument==0)return last?"x...x...x.x.xxxx"[step]:"x...x...x...x..."[step];
            if(instrument==1)return "....x.......x..."[step];
            return "..o...o...o...o."[step];
        }
    }
    if(track==SUNSET){
        static constexpr const char* kicks[]={"x.........x..x..","x.....x...x.....","x..x......x..x..","x.....x..x....x."};
        static constexpr const char* snares[]={"....x..g....x...","....x.....g.x..g","....x..g....x.g.","....x.g...g.x.gg"};
        static constexpr const char* hats[]={"x.x.Xxx.x.x.Xx.o","x.X.x.X.x.Xxxx.o","x.x.X.x.x.Xxx.Xo","x.XxxxX.x.x.Xx.o"};
        if(f.section==Section::BREAKDOWN && instrument==2)return step%4==0?'x':'.';
        return (instrument==0?kicks:instrument==1?snares:hats)[bar%4][step];
    }
    // 'r' is a roll hit: the synth ramps its velocity through the bar.
    const bool lastOfBuild=(f.section==Section::BUILD || f.section==Section::TURN) && form(track,bar+1).section!=f.section;
    const bool morning=track==MORNINGSTAR;
    if(f.style==Style::DNB){
        static constexpr const char* zk[]={"x.........x.....","x.........x..x..","x..x......x.....","x.........x...x."};
        static constexpr const char* mk[]={"x.........x..x..","x..x......x.....","x.....x...x..x..","x..x......x..xx."};
        static constexpr const char* snares[]={"....x..g.g..x...","....x..g....x..g","....x.g..g..x...","....x..g.g..x.gg"};
        static constexpr const char* hats[]={"x.XxxxX.x.XxxxXo","x.X.xxX.x.XxxxX.","x.XxxxX.x.Xxx.Xo","x.XxxxXxx.XxxxXo"};
        if(instrument==0)return (morning?mk:zk)[bar%4][step];
        if(instrument==1)return lastOfBuild?"....x.r.r.r.rrrr"[step]:snares[bar%4][step];
        return hats[bar%4][step];
    }
    if(instrument==0){
        if(morning && f.style==Style::TECHNO && bar%2==1)return "x...x...x..xx..."[step];
        return bar%4==3?"x...x...x...x..x"[step]:"x...x...x...x..."[step];
    }
    if(instrument==1){
        if(lastOfBuild)return "r.r.r.r.rrrrrrrr"[step];
        if(f.style==Style::HYBRID)return bar%2?"....x..g.g..x..g"[step]:"....x..g....x..."[step];
        return f.section==Section::BUILD?"....x.......x.gx"[step]:"....x.......x..."[step];
    }
    if(f.style==Style::HYBRID)return "x.oxx.oXx.oxx.oX"[step];
    return track==ZENITH?"x.xox.X.x.xox.Xo"[step]:"xXoxx.xoxXoxx.Xo"[step];
}
// The boss's bass line for this bar, or nullptr for the track's own techno roll.
inline const char* bass(int track,int bar){
    const auto f=form(track,bar);
    if(track==SUNSET || f.style==Style::TECHNO || f.style==Style::HARD)return nullptr;
    return bar%2?"x---x-f.x--.o-f.":"x--.x-f.x--.o-f.";
}
} // namespace combatscore
