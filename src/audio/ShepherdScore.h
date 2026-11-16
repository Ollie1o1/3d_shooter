#pragma once
#include "audio/CombatScore.h"

// COUNTERWEIGHT: the machine's five-note count is gradually answered by the
// player. Every movement keeps 174 BPM; changes land on a bar, never hold up combat.
namespace shepherdscore {
enum Movement { ASCENT, CONTROL, RESISTANCE, COLLAPSE, LAST_COUNT };
inline int clampMovement(int m){return std::clamp(m,0,4);}
inline combatscore::Form form(int movement,int bar,float within=0){
    using namespace combatscore;
    const int m=clampMovement(movement),b=std::max(0,bar)%16;
    static constexpr int harmony[5][8]={
        {0,0,6,6,3,3,5,4}, {0,0,0,6,0,0,5,4},
        {0,3,5,4,0,6,5,4}, {0,6,3,4,0,5,3,4}, {0,0,0,4,0,0,5,4}};
    Form f;f.degree=harmony[m][b%8];f.arp=.65f;f.theme=.85f;
    if(m==ASCENT){f.style=Style::DNB;f.filter=.55f+.3f*(b%4)/3.f;f.theme=.55f;f.arp=.45f;}
    if(m==CONTROL){f.style=b<8?Style::TECHNO:Style::DNB;f.filter=.65f;}
    if(m==RESISTANCE){f.style=b<8?Style::DNB:Style::HYBRID;f.arp=.8f;f.theme=1;}
    if(m==COLLAPSE){f.style=b<4||b>=12?Style::HARD:Style::HYBRID;f.theme=1;f.arp=.85f;}
    if(m==LAST_COUNT){f.style=Style::HYBRID;f.theme=.65f;f.arp=.4f;f.filter=.85f;}
    f.section=b%8<4?Section::STATEMENT:Section::ANSWER;
    if(m>=RESISTANCE && b>=8)f.section=Section::PEAK;
    if(b%8==7){f.section=Section::TURN;f.rise=std::clamp(within,0.f,1.f);}
    return f;
}
inline combatscore::Note theme(int movement,int bar,int step){
    using combatscore::Note;
    // Deliberate spaces leave attack tells room. The answer opens upward when
    // the heart is exposed; the final count reduces back to the original motif.
    static constexpr Note count[]={
        {0,0,3,.9f},{6,7,3,.83f},{12,10,3,.8f},
        {22,7,4,.85f},{28,0,4,.95f},
        {32,0,3,.87f},{38,7,3,.8f},{44,10,3,.82f},
        {54,5,4,.78f},{60,2,4,.85f}};
    static constexpr Note answer[]={
        {0,12,4,.95f},{6,10,3,.85f},{12,7,4,.9f},
        {20,3,4,.85f},{28,5,3,.82f},
        {32,7,4,.95f},{40,10,4,.85f},{46,12,2,.9f},
        {52,7,4,.9f},{60,11,4,.88f}};
    const bool reply=movement>=RESISTANCE && movement<LAST_COUNT && bar%8>=4;
    if(movement==ASCENT && bar%2==1)return {-1,0,0,0};
    const int at=(bar%4)*16+step;
    const auto* notes=reply?answer:count;
    for(int i=0;i<10;++i)if(notes[i].at==at)return notes[i];
    return {-1,0,0,0};
}
inline combatscore::Note acid(int movement,int bar,int step){
    static constexpr int pitches[4]={0,7,10,7};
    if(step%4!=2 || (movement==LAST_COUNT && step!=2))return {-1,0,0,0};
    return {step,pitches[(bar+step/4)%4],1,.7f,false};
}
inline char hit(int movement,int bar,int step,int instrument,bool hard){
    using combatscore::Style;
    auto style=form(movement,bar).style;
    if(style==Style::HARD && !hard)style=Style::TECHNO;
    if(instrument==0){
        if(style==Style::DNB)return bar%2?"x.........x..x.."[step]:"x.....x...x....."[step];
        return bar%8==7?"x...x...x...x..x"[step]:"x...x...x...x..."[step];
    }
    if(instrument==1){
        if(bar%8==7 && step>=12)return "x.gx"[step-12];
        return style==Style::DNB||style==Style::HYBRID?"....x..g.g..x..g"[step]:"....x.......x..."[step];
    }
    if(style==Style::HARD)return "..o...o...o...o."[step];
    return movement==ASCENT?"x.X.x.x.x.X.x.xo"[step]:"x.XxxxX.x.XxxxXo"[step];
}
inline const char* bass(int movement,int bar){
    const auto style=form(movement,bar).style;
    if(style==combatscore::Style::TECHNO||style==combatscore::Style::HARD)return nullptr;
    return bar%2?"x---x-f.x--.o-f.":"x--.x-f.x--.o-f.";
}
}
