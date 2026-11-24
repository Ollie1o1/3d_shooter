// Prints the enemy voice bank (VoiceTable.h) with each enemy's profile (VoiceSynth.h), one voice per
// line, for tools/build_real_voices.py:  name kind type attack variants class f0 voices body size boss duration
// duration: a TELL's wind-up seconds (its sound must fit it); 0 otherwise
#include "audio/VoiceSynth.h"
#include <cstdio>
int main() {
    static const char* K[] = {"spawn", "idle", "move", "tell", "attack", "hurt", "death", "special"};
    static const char* B[] = {"SERVO", "SKITTER", "HYDRAULIC", "WINGS", "HEAVY", "TREADS", "SCRAPE", "CRACKLE", "HUM", "GRIND", "GEARS", "ARMOUR", "CHAINS"};
    static const char* C[] = {"GUN", "FOLEY", "PLAYER", "ACTION", "WORLD", "UI", "TELL", "CHATTER"};
    for (const VoiceSpec& v : voiceBank()) {
        const bool any = v.type == EnemyType::COUNT;
        const VoiceSynth::Profile p = any ? VoiceSynth::Profile{} : VoiceSynth::profileOf(v.type);
        const float dur = v.kind == VoiceKind::TELL ? tellDur(v.type, v.attack) : 0.f;
        std::printf("%s %s %s %s %d %s %.0f %d %s %.2f %d %.3f\n", v.name.c_str(), K[(int)v.kind], any ? "any" : voiceKey(v.type),
                    v.attack == AttackKind::NONE ? "-" : attackKey(v.attack), v.variants, C[(int)v.cls], p.f0, p.voices, B[(int)p.body], p.size,
                    any ? 0 : (int)isBoss(v.type), dur);
    }
}
