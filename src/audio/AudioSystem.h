#pragma once
#include <atomic>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "audio/MusicSynth.h"
#include "audio/SfxMixer.h"
#include "audio/MixTable.h"
#include "audio/RoomTone.h"

// SDL_mixer may not be available — guard the include
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

// SDL_mixer opens the device and gives us one callback (its music hook): the
// soundtrack (MusicSynth) renders into it, then the effects (SfxMixer) mix on
// top, then it's converted to the device's format. Effects are placed in the
// world with playAt; the listener is the camera (setListener, every frame).
class AudioSystem {
public:
    bool  initialized  = false;
    float masterVolume = 1.f;   // 0..1, from GameSettings::audioVolume
    MusicSynth music;           // the live soundtrack (MusicSynth.h), always running
    SfxMixer   sfx;             // the effects (SfxMixer.h)
    RoomToneSynth room;         // each arena's background air (RoomTone.h)

    AudioSystem() {
#if HAS_SDL_MIXER
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            std::cerr << "SDL_mixer init failed: " << Mix_GetError() << "\n";
            return;
        }
        initialized = true;
        int freq = 44100, ch = 2; Uint16 fmt = AUDIO_S16SYS;
        Mix_QuerySpec(&freq, &fmt, &ch);
        outFormat = fmt; outChannels = ch; outRate = freq;
        music.setSampleRate((float)freq);
        sfx.setSampleRate((float)freq);
        room.setSampleRate((float)freq);
        musicBuf.resize(16384);          // so the audio thread never has to allocate
        if (const char* solo = std::getenv("OVERDRIVE_SOLO")) {   // dev: hear one role (tell / chatter / action / ui)
            const std::string s = solo;
            int r = s == "tell" ? 1 : s == "chatter" ? 0 : s == "action" ? 2 : s == "ui" ? 3 : -1;
            if (r >= 0) sfx.devSolo(1 << r);
        }
#endif
    }

    // After the last loadSound: from here on the audio thread mixes (the bank is fixed)
    void start() {
#if HAS_SDL_MIXER
        if (initialized && !started) { Mix_HookMusic(&AudioSystem::musicCallback, this); started = true; }
#endif
    }

    // Music volume (0..1) on top of the master volume
    void setMusicVolume(float v) { music.setVolume(v * masterVolume * 0.8f); }

    ~AudioSystem() {
#if HAS_SDL_MIXER
        if (started) Mix_HookMusic(nullptr, nullptr);
        if (initialized) Mix_CloseAudio();
#endif
    }

    // path (…/name.wav) plus any variants beside it (name_1.wav … name_8.wav).
    // An .ogg beside a name wins over its .wav: the web build ships Ogg Vorbis (Makefile)
    void loadSound(const std::string& name, const std::string& path) {
#if HAS_SDL_MIXER
        if (!initialized || started) return;
        std::string stem = path.size() > 4 ? path.substr(0, path.size() - 4) : path;
        loadAny(name, stem);
        for (int k = 1; k <= 8; ++k) loadAny(name, stem + "_" + std::to_string(k));
#else
        (void)name; (void)path;
#endif
    }

    // A sound built in code (the enemy voices, VoiceSynth.h) at its own rate
    void addBuffer(const std::string& name, std::vector<float> mono, float srcRate) {
        if (!initialized || started) return;
        sfx.addSound(name, std::move(mono), srcRate);
    }

    // Not placed: the player's own sounds, announcements, UI
    SoundHandle play(const std::string& name, int volume = 128, SoundGroup g = SoundGroup::PLAYER, bool priority = false) {
        if (!initialized) return 0;
        SfxMixer::Opts o; o.volume = volume / 128.f * masterVolume * mixGain(name); o.group = g; o.priority = priority;
        return sfx.play(name, o);
    }
    // Placed in the world: louder near, panned to its side, darker far away / behind.
    // floor (0..1): a must-hear cue (a kill, a deflect) never fades below it
    SoundHandle playAt(const std::string& name, glm::vec3 pos, int volume = 128, SoundGroup g = SoundGroup::WORLD,
                       bool priority = false, float floor = 0.f) {
        if (!initialized) return 0;
        SfxMixer::Opts o; o.volume = volume / 128.f * masterVolume * mixGain(name); o.group = g; o.priority = priority;
        o.positional = true; o.pos = pos; o.floor = floor;
        return sfx.play(name, o);
    }
    // Every option the mixer has (roles, pitch, drive, delay: the enemy voices)
    SoundHandle playOpts(const std::string& name, SfxMixer::Opts o) {
        if (!initialized) return 0;
        o.volume *= masterVolume * mixGain(name);
        return sfx.play(name, o);
    }
    void moveSource(SoundHandle h, glm::vec3 p) { if (initialized) sfx.move(h, p); }
    void stop(SoundHandle h) { if (initialized) sfx.stop(h); }
    void setListener(glm::vec3 pos, glm::vec3 right, glm::vec3 fwd) { if (initialized) sfx.setListener(pos, right, fwd); }
    void setSpace(ReverbSpace s) { if (initialized) sfx.setSpace(s); }
    void duck(float dB, float seconds) { if (initialized) sfx.duck(dB, seconds); }
    // 0..1: how much of the whole mix goes through a low-pass (low health: the world goes dull)
    void setMuffle(float m) { muffle.store(std::max(0.f, std::min(1.f, m)), std::memory_order_relaxed); }
    // 0..1: the rush of air past you (fast falls, the grapple, a dash), made in the callback
    void setWind(float w) { wind.store(std::max(0.f, std::min(1.f, w)), std::memory_order_relaxed); }
    // 0..1: the grapple's rope singing under load, made in the callback with the wind
    void setRope(float r) { rope.store(std::max(0.f, std::min(1.f, r)), std::memory_order_relaxed); }
    // The place's background air (crossfades), how loud it is now (1 = between
    // fights), and how far night has fallen (the Yard's birds give way to cicadas)
    void setRoom(RoomTone t)    { room.setRoom(t); }
    void setRoomLevel(float l)  { room.setLevel(l); room.setVolume(masterVolume); }
    void setRoomNight(float n)  { room.setNight(n); }

    // --audiodump FILE SECONDS: capture the final mix, written as a 16-bit WAV
    void startDump(const std::string& path, float seconds) {
        dumpPath = path;
        dumpBuf.assign(2 * (size_t)std::max(1.f, seconds * (float)outRate), 0.f);
        dumpWritten.store(0, std::memory_order_relaxed);
        dumping.store(true, std::memory_order_release);
    }
    bool dumpDone() const {
        return !dumpPath.empty() && dumpWritten.load(std::memory_order_acquire) >= (int)(dumpBuf.size() / 2);
    }
    void writeDump() {
        FILE* f = std::fopen(dumpPath.c_str(), "wb");
        if (!f) { dumpPath.clear(); return; }
        uint32_t frames = (uint32_t)(dumpBuf.size() / 2), data = frames * 4, rate = (uint32_t)outRate, br = rate * 4;
        uint32_t riff = 36 + data, fmtLen = 16; uint16_t pcm = 1, chans = 2, align = 4, bits = 16;
        std::fwrite("RIFF", 1, 4, f); std::fwrite(&riff, 4, 1, f); std::fwrite("WAVEfmt ", 1, 8, f);
        std::fwrite(&fmtLen, 4, 1, f); std::fwrite(&pcm, 2, 1, f); std::fwrite(&chans, 2, 1, f);
        std::fwrite(&rate, 4, 1, f); std::fwrite(&br, 4, 1, f); std::fwrite(&align, 2, 1, f); std::fwrite(&bits, 2, 1, f);
        std::fwrite("data", 1, 4, f); std::fwrite(&data, 4, 1, f);
        for (float v : dumpBuf) { int16_t s = (int16_t)(std::max(-1.f, std::min(1.f, v)) * 32767.f); std::fwrite(&s, 2, 1, f); }
        std::fclose(f);
        std::fprintf(stderr, "audiodump: %u frames at %u Hz -> %s\n", frames, rate, dumpPath.c_str());
        dumpPath.clear();
    }

private:
    bool started = false;
    int  outRate = 44100;
    std::string dumpPath;
    std::vector<float> dumpBuf;
    std::atomic<int>  dumpWritten{0};
    std::atomic<bool> dumping{false};
    std::atomic<float> muffle{0.f};
    std::atomic<float> wind{0.f};
    std::atomic<float> rope{0.f};
    float ropeNow = 0.f, ropePh = 0.f, ropePh2 = 0.f, ropeLP = 0.f;   // audio thread only
    float windNow = 0.f, wL1 = 0.f, wL2 = 0.f, wR1 = 0.f, wR2 = 0.f, gustPh = 0.f; uint32_t windRng = 0x1234567u;   // audio thread only
    float muffleNow = 0.f, lpL = 0.f, lpR = 0.f;   // audio thread only
#if HAS_SDL_MIXER
    Uint16 outFormat = AUDIO_S16SYS;
    int    outChannels = 2;
    std::vector<float> musicBuf;

    void loadAny(const std::string& name, const std::string& stem) {
        if (!loadOgg(name, stem + ".ogg")) loadWav(name, stem + ".wav");
    }

    // One WAV → mono float at the device rate, into the bank (missing files are skipped)
    void loadWav(const std::string& name, const std::string& path) {
        SDL_AudioSpec spec; Uint8* raw = nullptr; Uint32 len = 0;
        if (!SDL_LoadWAV(path.c_str(), &spec, &raw, &len)) return;
        addConverted(name, raw, len, spec.format, spec.channels, spec.freq);
        SDL_FreeWAV(raw);
    }

    // One Ogg Vorbis: SDL_mixer decodes it to the device's format, then the same path as a WAV
    bool loadOgg(const std::string& name, const std::string& path) {
        SDL_RWops* rw = SDL_RWFromFile(path.c_str(), "rb");
        if (!rw) return false;
        Mix_Chunk* c = Mix_LoadWAV_RW(rw, 1);
        if (!c) { std::cerr << "could not decode " << path << ": " << Mix_GetError() << "\n"; return false; }
        addConverted(name, c->abuf, c->alen, outFormat, outChannels, outRate);
        Mix_FreeChunk(c);
        return true;
    }

    // Any PCM → mono float at the device rate, into the bank
    void addConverted(const std::string& name, const Uint8* raw, Uint32 len, SDL_AudioFormat fmt, int channels, int rate) {
        SDL_AudioCVT cvt;
        if (SDL_BuildAudioCVT(&cvt, fmt, (Uint8)channels, rate, AUDIO_F32SYS, 1, outRate) < 0) return;
        std::vector<Uint8> work((size_t)len * (size_t)std::max(1, cvt.len_mult));
        std::memcpy(work.data(), raw, len);
        int bytes = (int)len;
        if (cvt.needed) {
            cvt.len = (int)len; cvt.buf = work.data();
            if (SDL_ConvertAudio(&cvt) < 0) return;
            bytes = cvt.len_cvt;
        }
        std::vector<float> mono((size_t)bytes / sizeof(float));
        std::memcpy(mono.data(), work.data(), mono.size() * sizeof(float));
        sfx.addSound(name, std::move(mono));
    }

    // Audio thread: the rope. A taut line humming: two slightly detuned tones that
    // climb with the pull, a little bowed grit on them, softened by a low-pass
    void addRope(float* buf, int frames) {
        float target = rope.load(std::memory_order_relaxed) * masterVolume;
        if (target < 0.001f && ropeNow < 0.001f) { ropeNow = 0.f; return; }
        float step = (target - ropeNow) / (float)std::max(1, frames);
        const float twoPi = 6.2831853f, k = 1.f - std::exp(-twoPi * 1800.f / (float)outRate);
        for (int f = 0; f < frames; ++f) {
            ropeNow += step;
            float hz = 150.f + 330.f * ropeNow;
            ropePh += hz / (float)outRate; ropePh2 += hz * 1.007f / (float)outRate;
            ropePh -= std::floor(ropePh); ropePh2 -= std::floor(ropePh2);
            windRng ^= windRng << 13; windRng ^= windRng >> 17; windRng ^= windRng << 5;
            float grit = (float)(windRng & 0xFFFF) / 32768.f - 1.f;
            float saw = (2.f * ropePh - 1.f) * 0.6f + std::sin(twoPi * ropePh2) * 0.5f;
            ropeLP += k * (saw * (1.f + 0.25f * grit) - ropeLP);
            float v = ropeLP * ropeNow * 0.05f;
            buf[2 * f] += v; buf[2 * f + 1] += v;
        }
    }

    // Audio thread: wind. Noise through a band that opens with speed (a low roar
    // that brightens into a hiss), separate noise per ear, a slow gust in it
    void addWind(float* buf, int frames) {
        addRope(buf, frames);
        float target = wind.load(std::memory_order_relaxed) * masterVolume;
        if (target < 0.001f && windNow < 0.001f) { windNow = 0.f; return; }
        float step = (target - windNow) / (float)std::max(1, frames);
        auto noise = [this]() { windRng ^= windRng << 13; windRng ^= windRng >> 17; windRng ^= windRng << 5; return (float)(windRng & 0xFFFF) / 32768.f - 1.f; };
        const float twoPi = 6.2831853f;
        for (int f = 0; f < frames; ++f) {
            windNow += step;
            float cut = 250.f + 2200.f * windNow * windNow;
            float kHi = 1.f - std::exp(-twoPi * cut / (float)outRate), kLo = 1.f - std::exp(-twoPi * cut * 0.2f / (float)outRate);
            float nl = noise(), nr = noise();
            wL1 += kHi * (nl - wL1); wL2 += kLo * (nl - wL2);
            wR1 += kHi * (nr - wR1); wR2 += kLo * (nr - wR2);
            gustPh += 0.7f / (float)outRate;
            float gust = 0.8f + 0.2f * std::sin(twoPi * gustPh) * std::sin(twoPi * gustPh * 0.37f + 1.f);
            float g = windNow * windNow * 0.22f * gust;
            buf[2 * f]     += (wL1 - wL2) * g;
            buf[2 * f + 1] += (wR1 - wR2) * g;
        }
    }

    // Audio thread: a one-pole low-pass (~900 Hz) blended in by `muffle`, eased across the buffer
    void applyMuffle(float* buf, int frames) {
        float target = muffle.load(std::memory_order_relaxed);
        if (target < 0.001f && muffleNow < 0.001f) { muffleNow = 0.f; lpL = lpR = 0.f; return; }
        const float k = 1.f - std::exp(-6.2831853f * 900.f / (float)outRate);
        float step = (target - muffleNow) / (float)std::max(1, frames);
        for (int f = 0; f < frames; ++f) {
            muffleNow += step;
            lpL += k * (buf[2 * f] - lpL); lpR += k * (buf[2 * f + 1] - lpR);
            float m = muffleNow * 0.85f;   // never all the way: the cues still have to read
            buf[2 * f]     += (lpL - buf[2 * f]) * m;
            buf[2 * f + 1] += (lpR - buf[2 * f + 1]) * m;
        }
    }

    // Audio thread: music, then effects on top, then convert to the device's format
    static void musicCallback(void* self, Uint8* stream, int len) {
        auto* a = static_cast<AudioSystem*>(self);
        int ch = a->outChannels > 0 ? a->outChannels : 2;
        int bytesPerSample = SDL_AUDIO_BITSIZE(a->outFormat) / 8;
        int frames = len / (bytesPerSample * ch);
        if ((int)a->musicBuf.size() < frames * 2) a->musicBuf.resize(frames * 2);
        a->music.render(a->musicBuf.data(), frames);
        a->sfx.render(a->musicBuf.data(), frames);
        a->room.render(a->musicBuf.data(), frames);
        a->addWind(a->musicBuf.data(), frames);
        a->applyMuffle(a->musicBuf.data(), frames);
        const float* src = a->musicBuf.data();
        if (a->dumping.load(std::memory_order_acquire)) {
            int w = a->dumpWritten.load(std::memory_order_relaxed), cap = (int)(a->dumpBuf.size() / 2);
            int n = std::min(frames, cap - w);
            if (n > 0) std::memcpy(&a->dumpBuf[2 * (size_t)w], src, sizeof(float) * 2 * (size_t)n);
            a->dumpWritten.store(w + std::max(n, 0), std::memory_order_release);
            if (w + n >= cap) a->dumping.store(false, std::memory_order_relaxed);
        }
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
