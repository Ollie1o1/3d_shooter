#pragma once
#include "enemies/EnemyTypes.h"
#include <cmath>
#include <string>

// Store the contact, not the attack an enemy happens to be doing when the
// death card is drawn. Unknown contacts deliberately offer no specific advice.
enum class DamageKind { UNKNOWN, PROJECTILE, MELEE, SHOCKWAVE, BEAM, EXPLOSION,
                        MARKED_FLOOR, PHANTOM, BURN, FALL, FURNACE, VENT, LOAD, SWEEP,
                        WHITE_STRIKE, GOLD_STRIKE, RED_STRIKE };
struct DamageCause {
    EnemyType enemy = EnemyType::COUNT;
    DamageKind kind = DamageKind::UNKNOWN;
    AttackKind attack = AttackKind::NONE;
};
struct DamageAdvice { const char* name; const char* counter; };
inline DamageAdvice damageAdvice(DamageCause c) {
    if(c.enemy==EnemyType::SOVEREIGN) {
        switch(c.attack) {
            case AttackKind::SWEEP:return {"SWEEP","PARRY THE WHITE SWEEP AS IT REACHES YOU."};
            case AttackKind::CLEAVE:return {"CLEAVE","PARRY THE WHITE CLEAVE AS IT LANDS."};
            case AttackKind::THRUST:return {"THRUST","PARRY THE WHITE THRUST AS IT REACHES YOU."};
            default:break;
        }
    }
    switch(c.kind) {
        case DamageKind::PROJECTILE:return {"PROJECTILE","BREAK LINE OF SIGHT OR PARRY THE INCOMING SHOT."};
        case DamageKind::MELEE:return {"CLOSE ATTACK","DASH OUT OF REACH DURING THE WIND-UP."};
        case DamageKind::SHOCKWAVE:return {"GROUND SHOCKWAVE","JUMP CLEAR OF THE GROUND ATTACK."};
        case DamageKind::BEAM:return {"BEAM","BREAK LINE OF SIGHT WITH SOLID COVER."};
        case DamageKind::EXPLOSION:return {"EXPLOSION","MAKE SPACE BEFORE THE EXPLOSION."};
        case DamageKind::MARKED_FLOOR:return {"MARKED FLOOR ATTACK","LEAVE THE MARKED GROUND BEFORE THE ATTACK LANDS."};
        case DamageKind::PHANTOM:return {"PHANTOM","MOVE ACROSS THE PHANTOM'S PATH."};
        case DamageKind::BURN:return {"BURNING GROUND",c.enemy==EnemyType::SOVEREIGN?"THE EDGES ARE BURNING. FIGHT IN THE CENTRE.":"LEAVE THE HAZARDOUS SURFACE."};
        case DamageKind::FALL:return {"FALL","USE THE RAMPS, GRAPPLE POINTS AND LANDING EDGES."};
        case DamageKind::FURNACE:return {"FURNACE","FOLLOW THE AMBER SALVAGE ROUTE AWAY FROM THE FIRE."};
        case DamageKind::VENT:return {"FREEZING VENT","LEAVE THE RED VENT WARNING BEFORE IT ERUPTS."};
        case DamageKind::SWEEP:return {"SWEPT. COUNT CORRECT","GET BEHIND A LIT MOORING POST BEFORE THE COUNT REACHES ZERO."};
        case DamageKind::LOAD:return {"FALLING LOAD","CLEAR THE AMBER SQUARE AFTER RELEASING THE LOAD."};
        case DamageKind::WHITE_STRIKE:return {"WHITE STRIKE","PARRY THE WHITE STRIKE AS IT REACHES YOU."};
        case DamageKind::GOLD_STRIKE:return {"UNPARRYABLE STRIKE","DODGE OR USE THE SWORD TO BLOCK. PARRY WILL NOT WORK."};
        case DamageKind::RED_STRIKE:return {"UNBLOCKABLE STRIKE","GET CLEAR OF THE ATTACK. GUARD WILL NOT STOP IT."};
        default:return {"",""};
    }
}
inline std::string damageLabel(DamageCause c) {
    std::string label;
    if(c.enemy>=EnemyType::HUSK && c.enemy<EnemyType::COUNT)label=statsOf(c.enemy).name;
    const auto advice=damageAdvice(c);
    if(*advice.name){if(!label.empty())label+=" / ";label+=advice.name;}
    return label;
}
struct DamageHistory {
    DamageCause last;
    void clear() { last={}; }
    void record(float amount,float healthBefore,DamageCause cause) {
        if(std::isfinite(amount) && amount>0 && healthBefore>0)last=cause;
    }
};
