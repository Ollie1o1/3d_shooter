#pragma once
// =============================================================================
// SfxMixer.h — the sound-effects mixer. Runs in the audio callback after
// MusicSynth has filled the buffer; no SDL, so tests drive it offline.
//
// THREADS: the game thread calls play / move / stop / setListener / setSpace /
// duck, which only push commands into a lock-free single-producer ring. The
// audio thread calls render(), which drains the ring, then mixes. addSound()
// is for loading, before the audio thread starts calling render().
//
// VOICES: 48, in four groups with caps (PLAYER 12, ENEMY 20, WORLD 12, UI 4).
// A full group gives its quietest voice to the new sound; a priority voice
// (the player's gun, a boss attack) is never taken by a non-priority sound.
// Every play gets a little pitch and gain jitter, and a sound with variants
// (name_1.wav ...) never plays the same one twice running.
// =============================================================================
#include "AudioTypes.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

class SfxMixer {
public:
    static constexpr int MAX_VOICES = 48;
    static constexpr int CHUNK      = 512;    // frames mixed per inner pass
    static constexpr int RING       = 512;    // commands buffered between renders
    static constexpr int GROUP_CAP[(int)SoundGroup::COUNT] = {12, 20, 12, 4};
    static constexpr float REF_DIST = 4.f;    // full level inside this
    static constexpr float ROLLOFF  = 1.f;
    static constexpr float MAX_DIST = 90.f;   // silent beyond (fading over the last 10 m)

    struct Opts {
        float      volume     = 1.f;                 // linear, before jitter
        SoundGroup group      = SoundGroup::PLAYER;
        bool       priority   = false;               // never stolen by a non-priority sound
        bool       positional = false;               // placed at pos in the world
        glm::vec3  pos{0.f};
    };

    explicit SfxMixer(float sampleRate = 44100.f, uint32_t seed = 0x9E3779B9u) : rng(seed ? seed : 1u) {
        setSampleRate(sampleRate);
    }
    SfxMixer(const SfxMixer&) = delete;
    SfxMixer& operator=(const SfxMixer&) = delete;

    void  setSampleRate(float sr) { rate = sr; }
    float sampleRate() const { return rate; }

    // --- loading (before render() runs) ---------------------------------------
    void addSound(const std::string& name, std::vector<float> mono) {
        if (mono.empty()) return;
        names[name].push_back((int)bank.size());
        bank.push_back(std::move(mono));
    }
    bool has(const std::string& name) const { return names.count(name) > 0; }
    int  variants(const std::string& name) const {
        auto it = names.find(name);
        return it == names.end() ? 0 : (int)it->second.size();
    }

    // --- game thread ----------------------------------------------------------
    SoundHandle play(const std::string& name, const Opts& o) {
        auto it = names.find(name);
        if (it == names.end() || !(o.volume > 0.f)) return 0;
        const std::vector<int>& ids = it->second;
        int pick = 0;
        if (ids.size() > 1) {
            auto lv = lastVariant.find(name);
            int last = lv == lastVariant.end() ? -1 : lv->second;
            if (last < 0) pick = (int)(next() % ids.size());
            else { pick = (int)(next() % (ids.size() - 1)); if (pick >= last) ++pick; }   // never twice running
            lastVariant[name] = pick;
        }
        const bool ui = o.group == SoundGroup::UI;
        Cmd c{};
        c.kind = Cmd::PLAY; c.serial = ++serials; if (c.serial == 0) c.serial = ++serials;
        c.sample = ids[pick];
        c.pitch = 1.f + (ui ? 0.01f : 0.04f) * uni();
        c.gain  = o.volume * (ui ? 1.f : dbToLin(1.5f * uni()));
        c.group = o.group; c.priority = o.priority; c.positional = o.positional; c.a = o.pos;
        lastPitch = c.pitch; lastPick = pick;
        return push(c) ? c.serial : 0;
    }
    void move(SoundHandle h, glm::vec3 p) { if (!h) return; Cmd c{}; c.kind = Cmd::MOVE; c.serial = h; c.a = p; push(c); }
    void stop(SoundHandle h) { if (!h) return; Cmd c{}; c.kind = Cmd::STOP; c.serial = h; push(c); }
    void setListener(glm::vec3 pos, glm::vec3 right, glm::vec3 fwd) {
        Cmd c{}; c.kind = Cmd::LISTENER; c.a = pos; c.b = right; c.c = fwd; push(c);
    }
    float lastPlayPitch() const { return lastPitch; }
    int   lastPlayVariant() const { return lastPick; }

    // --- audio thread ---------------------------------------------------------
    // io: interleaved stereo, `frames` long, already holding the music
    void render(float* io, int frames) {
        drain();
        for (int done = 0; done < frames;) {
            int n = std::min(CHUNK, frames - done);
            mixChunk(io + 2 * done, n);
            done += n;
        }
    }

    // --- introspection (audio-thread state; tests) -----------------------------
    bool isPlaying(SoundHandle h) const {
        for (const auto& v : voices) if (v.active && v.serial == h) return true;
        return false;
    }
    int active(SoundGroup g) const {
        int n = 0;
        for (const auto& v : voices) n += v.active && v.group == g;
        return n;
    }

private:
    struct Cmd {
        enum Kind : uint8_t { PLAY, MOVE, STOP, LISTENER, SPACE, DUCK } kind;
        uint32_t   serial;
        int        sample;
        float      gain, pitch, x, y;
        SoundGroup group;
        bool       priority, positional;
        glm::vec3  a, b, c;
    };
    struct Voice {
        bool       active = false, priority = false, positional = false, fresh = true;
        uint32_t   serial = 0;
        int        sample = 0;
        SoundGroup group = SoundGroup::PLAYER;
        double     pos = 0.0;            // playhead, in samples (fractional: pitch)
        float      pitch = 1.f, gain = 1.f;
        glm::vec3  at{0.f};
        float      curL = 0.f, curR = 0.f, lp = 1.f;   // smoothed gains and low-pass coefficient
        float      z = 0.f;                            // low-pass state
        float      loud = 0.f;                         // last target level (for stealing)
    };
    struct Listener { glm::vec3 pos{0.f}, right{1.f, 0.f, 0.f}, fwd{0.f, 0.f, -1.f}; };

    float rate = 44100.f;
    std::vector<std::vector<float>> bank;
    std::unordered_map<std::string, std::vector<int>> names;
    std::unordered_map<std::string, int> lastVariant;   // game thread
    uint32_t rng, serials = 0;
    float    lastPitch = 1.f;
    int      lastPick = -1;

    std::array<Cmd, RING> ring{};
    std::atomic<uint32_t> head{0}, tail{0};   // head: next write (game thread), tail: next read (audio thread)

    std::array<Voice, MAX_VOICES> voices{};
    Listener lis;

    static float dbToLin(float db) { return std::pow(10.f, db / 20.f); }
    uint32_t next() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }
    float uni() { return (next() & 0xFFFFFF) / 8388607.5f - 1.f; }   // -1..1

    bool push(const Cmd& c) {
        uint32_t h = head.load(std::memory_order_relaxed);
        if (h - tail.load(std::memory_order_acquire) >= (uint32_t)RING) return false;   // full: drop it
        ring[h % RING] = c;
        head.store(h + 1, std::memory_order_release);
        return true;
    }
    void drain() {
        uint32_t t = tail.load(std::memory_order_relaxed), h = head.load(std::memory_order_acquire);
        for (; t != h; ++t) apply(ring[t % RING]);
        tail.store(t, std::memory_order_release);
    }
    void apply(const Cmd& c) {
        switch (c.kind) {
            case Cmd::PLAY: startVoice(c); break;
            case Cmd::MOVE: for (auto& v : voices) if (v.active && v.serial == c.serial) v.at = c.a; break;
            case Cmd::STOP: for (auto& v : voices) if (v.active && v.serial == c.serial) v.active = false; break;
            case Cmd::LISTENER: lis.pos = c.a; lis.right = c.b; lis.fwd = c.c; break;
            default: break;
        }
    }
    void startVoice(const Cmd& c) {
        if (c.sample < 0 || c.sample >= (int)bank.size()) return;
        const int g = (int)c.group;
        int count = 0, freeSlot = -1, victim = -1;
        float quietest = 1e30f;
        for (int i = 0; i < MAX_VOICES; ++i) {
            const Voice& v = voices[i];
            if (!v.active) { if (freeSlot < 0) freeSlot = i; continue; }
            if ((int)v.group != g) continue;
            ++count;
            if (v.priority && !c.priority) continue;
            if (v.loud < quietest) { quietest = v.loud; victim = i; }
        }
        int slot = count < GROUP_CAP[g] ? freeSlot : victim;
        if (slot < 0) return;   // everything in the group outranks it
        Voice& v = voices[slot];
        v = Voice{};
        v.active = true; v.priority = c.priority; v.positional = c.positional;
        v.serial = c.serial; v.sample = c.sample; v.group = c.group;
        v.pitch = c.pitch; v.gain = c.gain; v.at = c.a; v.loud = c.gain;
    }

    // Target left/right gain and low-pass coefficient for a voice right now:
    // distance falloff, a pan law that keeps the centre at full level, and a
    // low-pass that closes with distance and when the source is behind you
    void target(const Voice& v, float& L, float& R, float& lp) const {
        float g = v.gain;
        lp = 1.f;
        if (!v.positional) { L = R = g; return; }
        glm::vec3 d = v.at - lis.pos;
        float dist = glm::length(d);
        if (!(dist < MAX_DIST)) { L = R = 0.f; return; }
        g *= REF_DIST / (REF_DIST + ROLLOFF * std::max(0.f, dist - REF_DIST));
        g *= std::min(1.f, (MAX_DIST - dist) / 10.f);
        float pan = 0.f, behind = 0.f;
        if (dist > 0.5f) {
            glm::vec3 nd = d / dist;
            pan = glm::clamp(glm::dot(nd, lis.right), -1.f, 1.f);
            behind = std::max(0.f, -glm::dot(nd, lis.fwd));
        }
        float th = (pan + 1.f) * 0.78539816f;   // 0 (hard left) .. pi/2 (hard right)
        L = g * std::min(1.f, 1.41421356f * std::cos(th));
        R = g * std::min(1.f, 1.41421356f * std::sin(th));
        float fc = 18000.f * std::pow(2500.f / 18000.f, dist / MAX_DIST) * (1.f - 0.5f * behind);
        fc = std::min(fc, 0.45f * rate);
        lp = 1.f - std::exp(-6.2831853f * fc / rate);
    }

    void mixChunk(float* io, int n) {
        for (auto& v : voices) if (v.active) mixVoice(v, io, n);
        for (int i = 0; i < 2 * n; ++i) io[i] = softClip(io[i]);
    }
    void mixVoice(Voice& v, float* io, int n) {
        float tL, tR, tlp;
        target(v, tL, tR, tlp);
        if (v.fresh) { v.curL = tL; v.curR = tR; v.lp = tlp; v.fresh = false; }
        v.loud = std::max(tL, tR);
        const std::vector<float>& s = bank[v.sample];
        const double end = (double)s.size() - 1.0;
        if (tL + tR + v.curL + v.curR <= 0.f) {   // out of earshot: keep time, skip the work
            v.pos += (double)v.pitch * n;
            if (v.pos >= end) v.active = false;
            return;
        }
        const float dL = (tL - v.curL) / n, dR = (tR - v.curR) / n, dlp = (tlp - v.lp) / n;
        for (int i = 0; i < n; ++i) {
            if (v.pos >= end) { v.active = false; break; }
            size_t k = (size_t)v.pos;
            float f = (float)(v.pos - (double)k);
            float x = s[k] + (s[k + 1] - s[k]) * f;
            v.lp += dlp; v.curL += dL; v.curR += dR;
            v.z += v.lp * (x - v.z);
            io[2 * i]     += v.z * v.curL;
            io[2 * i + 1] += v.z * v.curR;
            v.pos += v.pitch;
        }
        v.curL = tL; v.curR = tR; v.lp = tlp;
    }
    // Leaves anything under 0.8 alone, rounds the rest off toward 1
    static float softClip(float x) {
        float a = std::fabs(x);
        if (!(a < 1e6f)) return 0.f;   // NaN / inf: silence rather than a screech
        if (a <= 0.8f) return x;
        float y = 0.8f + 0.2f * std::tanh((a - 0.8f) / 0.2f);
        return x < 0.f ? -y : y;
    }
};
