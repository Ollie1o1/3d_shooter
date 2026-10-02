#pragma once
#include <string>
#include <unordered_map>
#include <iostream>
#include <vector>
#include "MusicSynth.h"

// SDL2_mixer may not be available — guard the include
#ifdef __has_include
#  if __has_include(<SDL2/SDL_mixer.h>)
#    include <SDL2/SDL_mixer.h>
#    define HAS_SDL_MIXER 1
#  else
#    define HAS_SDL_MIXER 0
#  endif
#else
#  include <SDL2/SDL_mixer.h>
#  define HAS_SDL_MIXER 1
#endif

class AudioSystem {
public:
    bool  initialized  = false;
    float masterVolume = 1.f;   // 0..1, from GameSettings::audioVolume
    MusicSynth music;           // the live soundtrack (MusicSynth.h), always running

    AudioSystem() {
#if HAS_SDL_MIXER
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            std::cerr << "SDL_mixer init failed: " << Mix_GetError() << "\n";
            return;
        }
        Mix_AllocateChannels(32);
        initialized = true;
        // The soundtrack is synthesized on the fly into SDL_mixer's music slot
        int freq = 44100, ch = 2; Uint16 fmt = AUDIO_S16SYS;
        Mix_QuerySpec(&freq, &fmt, &ch);
        outFormat = fmt; outChannels = ch;
        music.setSampleRate((float)freq);
        musicBuf.resize(16384);          // so the audio thread never has to allocate
        Mix_HookMusic(&AudioSystem::musicCallback, this);
#endif
    }

    // Music volume (0..1) on top of the master volume
    void setMusicVolume(float v) { music.setVolume(v * masterVolume * 0.8f); }

    ~AudioSystem() {
#if HAS_SDL_MIXER
        if (initialized) Mix_HookMusic(nullptr, nullptr);
        for (auto& [k,v] : sounds) Mix_FreeChunk(v);
        if (initialized) Mix_CloseAudio();
#endif
    }

    void loadSound(const std::string& name, const std::string& path) {
#if HAS_SDL_MIXER
        if (!initialized) return;
        Mix_Chunk* c = Mix_LoadWAV(path.c_str());
        if (!c) {
            // silently skip missing files
            return;
        }
        sounds[name] = c;
#else
        (void)name; (void)path;
#endif
    }

    void play(const std::string& name, int volume = 128) {
#if HAS_SDL_MIXER
        if (!initialized) return;
        auto it = sounds.find(name);
        if (it == sounds.end()) return;
        int ch = Mix_PlayChannel(-1, it->second, 0);
        if (ch >= 0) Mix_Volume(ch, (int)(volume * masterVolume));
#else
        (void)name; (void)volume;
#endif
    }

private:
#if HAS_SDL_MIXER
    std::unordered_map<std::string, Mix_Chunk*> sounds;
    Uint16 outFormat = AUDIO_S16SYS;
    int    outChannels = 2;
    std::vector<float> musicBuf;

    // Audio thread: render the synth and convert to the device's format
    static void musicCallback(void* self, Uint8* stream, int len) {
        auto* a = static_cast<AudioSystem*>(self);
        int ch = a->outChannels > 0 ? a->outChannels : 2;
        int bytesPerSample = SDL_AUDIO_BITSIZE(a->outFormat) / 8;
        int frames = len / (bytesPerSample * ch);
        if ((int)a->musicBuf.size() < frames * 2) a->musicBuf.resize(frames * 2);
        a->music.render(a->musicBuf.data(), frames);
        const float* src = a->musicBuf.data();
        if (SDL_AUDIO_ISFLOAT(a->outFormat) && bytesPerSample == 4) {
            float* out = reinterpret_cast<float*>(stream);
            for (int f = 0; f < frames; ++f) for (int c = 0; c < ch; ++c) out[f * ch + c] = src[2 * f + (c & 1)];
        } else if (bytesPerSample == 2 && SDL_AUDIO_ISSIGNED(a->outFormat)) {
            Sint16* out = reinterpret_cast<Sint16*>(stream);
            for (int f = 0; f < frames; ++f)
                for (int c = 0; c < ch; ++c) {
                    float v = src[2 * f + (c & 1)];
                    v = v < -1.f ? -1.f : v > 1.f ? 1.f : v;
                    out[f * ch + c] = (Sint16)(v * 32767.f);
                }
        } else {
            SDL_memset(stream, 0, len);   // unexpected device format: stay silent rather than screech
        }
    }
#endif
};
