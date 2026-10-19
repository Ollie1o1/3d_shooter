#pragma once
// =============================================================================
// Difficulty.h — how hard the enemies push. Picked in the settings
// (GAMEPLAY → DIFFICULTY) and read live, so a change applies straight away.
//
// LENIENT is the game as it first shipped. STANDARD (the default) hits harder,
// attacks more often and leads its shots, so strafing in a straight line
// stops being a free dodge. VIOLENT and BRUTAL push all of it further and cut
// how much health you get back for fighting.
// No OpenGL in here: the enemy AI reads it in the headless tests too.
// =============================================================================

struct DifficultyTuning {
    const char* name;
    float damage;        // enemy damage ×
    float health;        // enemy health ×
    float attackRate;    // enemies attack this many times as often
    float shotSpeed;     // enemy projectile speed ×
    float lead;          // 0: shoot where you are, 1: where you'll be when it arrives
    float windup;        // telegraph length × (shorter = less time to react)
    float moveSpeed;     // enemy movement ×
    float heal;          // health back from hits and kills ×
    float drops;         // chance of a health drop ×
    float waveSize;      // ARENA: enemies per wave ×
    int   maxAliveBonus; // ARENA: this many more on the field at once
};

constexpr int DIFFICULTY_LEVELS = 4;
constexpr int DIFFICULTY_DEFAULT = 1;   // STANDARD

inline const DifficultyTuning& difficulty(int level) {
    static const DifficultyTuning T[DIFFICULTY_LEVELS] = {
        //  name        dmg   hp    rate  shot  lead  wind  move  heal  drop  wave  +alive
        {"LENIENT",   0.85f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,  0},
        {"STANDARD",  1.3f,  1.1f, 1.25f,1.12f,0.5f, 0.92f,1.06f,0.7f, 0.85f,1.3f,  2},
        {"VIOLENT",   1.65f, 1.25f,1.45f,1.22f,0.8f, 0.84f,1.12f,0.5f, 0.7f, 1.55f, 4},
        {"BRUTAL",    2.0f,  1.4f, 1.65f,1.32f,1.0f, 0.76f,1.2f, 0.35f,0.55f,1.8f,  6},
    };
    return T[level < 0 ? 0 : level >= DIFFICULTY_LEVELS ? DIFFICULTY_LEVELS - 1 : level];
}
