#pragma once
#include "core/Bindings.h"
#include <string>
#include <algorithm>

namespace lessons {
constexpr int COUNT = 8;
inline const char* title(int i) {
    static const char* names[] = {"PARRY", "FINISHER", "ENEMY GRAPPLE", "SHOTGUN JUMP",
        "GRENADE PUNCH", "OVERDRIVE", "MOVEMENT", "FREE PRACTICE"};
    return names[std::clamp(i,0,COUNT-1)];
}
inline std::string action(int i, const Bindings& b) {
    switch(i) {
        case 0:return b.label(Act::PARRY)+" AS A SHOT REACHES YOU. AIM THE RETURN AT ITS OWNER.";
        case 1:return b.label(Act::INTERACT)+" NEAR A GLOWING ENEMY. THIS TARGET IS ALREADY WOUNDED.";
        case 2:return b.label(Act::GRAPPLE)+" ON THE GUNNER ABOVE. LIGHT ENEMIES COME TO YOU.";
        case 3:return "SELECT 2. LOOK DOWN AND FIRE RMB WITH TWO SHELLS LOADED.";
        case 4:return b.label(Act::GRENADE)+" THEN "+b.label(Act::PARRY)+" WHILE YOUR GRENADE IS WITHIN REACH.";
        case 5:return "CHAIN KILLS, PARRIES AND DIFFERENT WEAPONS TO FILL YOUR STYLE METER.";
        case 6:return "REACH THE GOLD PLATFORM. JUMP TWICE, DASH OR GRAPPLE THE OVERHEAD RAIL.";
        default:return "TRY ANY WEAPON OR MOD. [ / ] SELECT AN ENEMY. BACKSPACE RESETS THE ROOM.";
    }
}
inline const char* why(int i) {
    static const char* text[] = {
        "A PARRY RETURNS DAMAGE AND GIVES A BRIEF MOMENT OF SAFETY. WHITE TELLS CAN BE PARRIED.",
        "FINISHERS RESTORE HEALTH, RELOAD YOUR GUN AND GIVE BRIEF SAFETY. E NEVER PARRIES.",
        "LIGHT ENEMIES ARE PULLED AND STUNNED. HEAVY ENEMIES PULL YOU TO THEM. JUMP TO RELEASE.",
        "THE DOUBLE SHOT SPENDS TWO SHELLS AND PUSHES YOU BACK. AIM DOWN TO TURN RECOIL INTO HEIGHT.",
        "PUNCHING YOUR OWN GRENADE DETONATES IT. YOUR AIM AND DISTANCE SET UP THE BLAST.",
        "VARY YOUR ATTACKS: STALE WEAPONS EARN LESS STYLE. OVERDRIVE REFILLS GUNS; KILLS EXTEND IT.",
        "AIR MOVEMENT PRESERVES MOMENTUM. SLIDE INTO SPACE; SLAM FROM HEIGHT TO HIT HARDER.",
        "TARGETS RETURN AFTER DEATH. DAMAGE CANNOT KILL YOU HERE. TAB OPENS THE ARMORY."
    };
    return text[std::clamp(i,0,COUNT-1)];
}
}
