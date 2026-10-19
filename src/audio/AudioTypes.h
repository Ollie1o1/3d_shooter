#pragma once
// AudioTypes.h — the names the sound engine shares with the rest of the game
// (kept tiny so level data can name a reverb space without the mixer).
#include <cstdint>

// Who a sound belongs to: each group has its own voice cap (SfxMixer::GROUP_CAP),
// reverb send, and the duck applies to ENEMY and WORLD
enum class SoundGroup : uint8_t { PLAYER, ENEMY, WORLD, UI, COUNT };

// The reverb of a place (Arena::space): OPEN yard, METAL halls, a long dark
// HALL, a tight echoing SHAFT
enum class ReverbSpace : uint8_t { OPEN, METAL, HALL, SHAFT, COUNT };

// What a sound is for, so the mix can rank it (SfxMixer): a wind-up TELL cuts
// through, enemy CHATTER (idles, steps) steps back under it; ACTION is the
// rest of the game, UI the menus and announcements
enum class SoundRole : uint8_t { CHATTER, TELL, ACTION, UI, COUNT };

using SoundHandle = uint32_t;   // a playing sound, for moveSource/stop; 0 = none
