#pragma once
#include <string>
#include <unordered_map>
#include <iostream>

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
    bool initialized = false;

    AudioSystem() {
#if HAS_SDL_MIXER
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            std::cerr << "SDL_mixer init failed: " << Mix_GetError() << "\n";
            return;
        }
        Mix_AllocateChannels(16);
        initialized = true;
#endif
    }

    ~AudioSystem() {
#if HAS_SDL_MIXER
        for (auto& [k,v] : sounds) Mix_FreeChunk(v);
        if (music) Mix_FreeMusic(music);
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
        if (ch >= 0) Mix_Volume(ch, volume);
#else
        (void)name; (void)volume;
#endif
    }

    void loadMusic(const std::string& path) {
#if HAS_SDL_MIXER
        if (!initialized) return;
        if (music) Mix_FreeMusic(music);
        music = Mix_LoadMUS(path.c_str());
        if (!music) std::cerr << "Failed to load music: " << Mix_GetError() << "\n";
#else
        (void)path;
#endif
    }

    void playMusic(int loops = -1, int volume = 80) {
#if HAS_SDL_MIXER
        if (!initialized || !music) return;
        Mix_VolumeMusic(volume);
        Mix_PlayMusic(music, loops);
#else
        (void)loops; (void)volume;
#endif
    }

    void stopMusic() {
#if HAS_SDL_MIXER
        if (initialized) Mix_HaltMusic();
#endif
    }

private:
#if HAS_SDL_MIXER
    std::unordered_map<std::string, Mix_Chunk*> sounds;
    Mix_Music* music = nullptr;
#endif
};
