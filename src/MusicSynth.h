#pragma once
// =============================================================================
// MusicSynth.h — the soundtrack, synthesized live. No audio files: a small
// step sequencer drives drums, a rolling bass, a pad, an arpeggio and a lead,
// all built from oscillators, noise and filters, into a stereo float buffer.
// AudioSystem feeds it to SDL_mixer (Mix_HookMusic); the tests render it
// headless.
//
// Darksynth / industrial at 148-172 BPM, in four tracks (one per kind of
// place). It plays in layers that follow the action — the game sets an
// intensity and the layers fade in and out on their own:
//
//   intensity  0     pad and sub bass, quiet hats          (menu bed, death)
//              0.6   + kick, rolling bass (filtered)        (tubes, breathers)
//              1.0   + snare, open filter, arpeggio         (fighting)
//              1.3   + lead, crash on every bar             (the boss, OVERDRIVE)
//
// muffle() runs everything through a low-pass (pause menu). Parameters are
// atomics: the game thread sets them, the audio thread reads them.
// =============================================================================
#include <atomic>
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cstring>

struct MusicTrack {
    const char* name;
    float bpm;
    int   root;            // MIDI note of the key (natural minor)
    int   chords[4];       // scale degree of each bar's chord (0 = i)
    // 16 steps a bar. kick/snare: 'x' hit, 'g' ghost. hat: 'x' soft, 'X'
    // accent, 'o' open. bass: 'x' root, 'o' root an octave up, 'f' fifth,
    // '-' hold. arp: chord tone 0-3. '.' is a rest everywhere.
    const char* kick;
    const char* snare;
    const char* hat;
    const char* bass;
    const char* arp;
    const char* lead;      // 32 steps (two bars): chord tone 0-3, '-' hold, '.' rest
};

inline const MusicTrack& musicTrack(int i) {
    static const MusicTrack T[] = {
        {"SUNSET", 150.f, 57, {0, 5, 2, 6},
         "x...x...x...x...", "....x.......x...", "x.X.x.X.x.X.x.Xo",
         "x.xxx.xxx.xxx.xo", "0123012301230123", "3---2---1-2-0---3---2---1---0---"},
        {"FOUNDRY", 156.f, 50, {0, 0, 5, 6},
         "x..x..x...x.x...", "....x..g....x...", "xxXxxxXxxxXxxxXo",
         "xx.xxo.xxx.xxo.x", "0.1.2.1.3.2.1.2.", "0-0-3---2-1-0---0-0-3---1---2---"},
        {"SPIRE", 148.f, 52, {0, 5, 3, 4},
         "x.......x.x.....", "....x.......x...", "x.X.x.X.x.X.x.X.",
         "x..xx..xx..xx.ox", "0213021302130213", "2-------1-------3-------2---1---"},
        {"CORE", 172.f, 49, {0, 5, 6, 4},
         "x.........x.....", "....x.......x..g", "xXxXxXxXxXxXxXxo",
         "x-.xo-.xx-.xo-fx", "0123321001233210", "3---2-3-1---0---3-2-1-0-1---2---"},
    };
    return T[((i % 4) + 4) % 4];
}
constexpr int MUSIC_TRACKS = 4;

class MusicSynth {
public:
    // ---- set from the game thread ----
    void setTrack(int t)       { wantTrack.store(t); }
    void setIntensity(float x) { wantLevel.store(x); }
    void setVolume(float v)    { volume.store(v); }
    void setMuffle(bool m)     { muffled.store(m); }
    void setSampleRate(float sr) { SR = sr; delayL.assign((size_t)(sr * 1.5f), 0.f); delayR = delayL; dpos = 0; }

    MusicSynth() { setSampleRate(44100.f); }

    // Render `frames` stereo frames, interleaved L R, into out (overwrites).
    void render(float* out, int frames) {
        const MusicTrack& tr = musicTrack(track);
        float vol = volume.load();
        for (int f = 0; f < frames; ++f) {
            if ((blockPos++ & 31) == 0) updateBlock();
            if (sampleInStep <= 0) { nextStep(); sampleInStep += stepLen; }
            --sampleInStep;
            float L = 0.f, R = 0.f;
            synthSample(tr, L, R);
            // Master: muffle filter, gentle saturation, volume
            float m = muffleAmt;
            if (m > 0.001f) {
                float lo = mufL.lp(L, gMuf, kMuf), ro = mufR.lp(R, gMuf, kMuf);
                L += (lo - L) * m; R += (ro - R) * m;
            }
            L = softClip(L * 0.75f) * vol;
            R = softClip(R * 0.75f) * vol;
            out[2 * f]     = L;
            out[2 * f + 1] = R;
        }
    }

    float level() const { return lvl; }       // smoothed intensity (tests)
    int   currentTrack() const { return track; }

private:
    std::atomic<int>   wantTrack{0};
    std::atomic<float> wantLevel{0.5f};
    std::atomic<float> volume{0.6f};
    std::atomic<bool>  muffled{false};

    float SR = 44100.f;
    int   track = 0;
    float lvl = 0.5f, muffleAmt = 0.f;
    int   step = -1;                 // 0..63 across the four-bar progression
    float sampleInStep = 0.f, stepLen = 1.f;
    uint32_t blockPos = 0, rng = 0x9E3779B9u;

    // ---- building blocks ----
    static float midiHz(float n) { return 440.f * std::pow(2.f, (n - 69.f) / 12.f); }
    float noise() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (float)(int32_t)rng * (1.f / 2147483648.f); }
    static float softClip(float x) { return std::tanh(x); }
    static float polyblep(float t, float dt) {
        if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
        if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
        return 0.f;
    }
    struct Osc {
        float ph = 0.f;
        float saw(float hz, float sr) {
            float dt = hz / sr; ph += dt; if (ph >= 1.f) ph -= 1.f;
            return 2.f * ph - 1.f - polyblep(ph, dt);
        }
        float square(float hz, float sr) {
            float dt = hz / sr; ph += dt; if (ph >= 1.f) ph -= 1.f;
            float p2 = ph + 0.5f; if (p2 >= 1.f) p2 -= 1.f;
            return (ph < 0.5f ? 1.f : -1.f) + polyblep(ph, dt) - polyblep(p2, dt);
        }
        float sine(float hz, float sr) {
            ph += hz / sr; if (ph >= 1.f) ph -= 1.f;
            return std::sin(ph * 6.2831853f);
        }
    };
    // Zero-delay-feedback state variable filter (low/high/band)
    struct SVF {
        float ic1 = 0.f, ic2 = 0.f, band = 0.f, high = 0.f;
        float lp(float x, float g, float k) {
            float a1 = 1.f / (1.f + g * (g + k)), a2 = g * a1, a3 = g * a2;
            float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.f * v1 - ic1; ic2 = 2.f * v2 - ic2;
            band = v1; high = x - k * v1 - v2;
            return v2;
        }
        float hp(float x, float g, float k) { lp(x, g, k); return high; }
    };
    float gOf(float hz) const { return std::tan(3.1415927f * std::min(hz, SR * 0.45f) / SR); }

    // ---- voices ----
    struct Hit { float t = 1e9f, vel = 1.f; };                // time since trigger (s)
    Hit kick, snare, hatC, hatO, crash;
    SVF snareF, hatF, crashF;
    Osc kickOsc, snareOsc;

    float bassNote = 45.f, bassTarget = 45.f, bassEnvT = 1e9f, bassAmp = 0.f;
    bool  bassGate = false;
    Osc   bass1, bass2, sub;
    SVF   bassF;

    float padNotes[4] = {57, 60, 64, 69};
    float padCur[4]   = {57, 60, 64, 69};
    Osc   pad[8];
    SVF   padFL, padFR;

    float arpNote = 69.f, arpT = 1e9f; bool arpLeft = false;
    Osc   arpOsc; SVF arpF;

    float leadNote = 81.f, leadCur = 81.f, leadT = 1e9f, leadAmp = 0.f; bool leadOn = false;
    Osc   leadOsc, leadOsc2, vib; SVF leadF;

    std::vector<float> delayL, delayR; size_t dpos = 0;
    SVF mufL, mufR;

    // Per-block parameters
    float gKick = 0, gBass = 0, gHat = 0, gSnare = 0, gArp = 0, gLead = 0, gPad = 0;
    float gBassF = 0, gMuf = 0, kMuf = 1.4f, gSnareF = 0, gHatF = 0, gArpF = 0, gLeadF = 0, gPadF = 0, gCrashF = 0;
    float duck = 1.f;

    static float sstep(float a, float b, float x) {
        float t = std::min(1.f, std::max(0.f, (x - a) / (b - a)));
        return t * t * (3.f - 2.f * t);
    }

    int scaleNote(const MusicTrack& tr, int degree, int octaveOff = 0) const {
        static const int MINOR[7] = {0, 2, 3, 5, 7, 8, 10};
        int oct = degree / 7, d = degree % 7;
        return tr.root + MINOR[d] + 12 * (oct + octaveOff);
    }
    // Chord tone k (0 root, 1 third, 2 fifth, 3 octave) of the chord on `deg`
    int chordTone(const MusicTrack& tr, int deg, int k, int octaveOff = 0) const {
        if (k == 3) return scaleNote(tr, deg, octaveOff + 1);
        return scaleNote(tr, deg + 2 * k, octaveOff);
    }

    void updateBlock() {
        float target = wantLevel.load();
        float rate = 32.f / SR;                                   // per block
        lvl += (target - lvl) * std::min(1.f, rate * (target > lvl ? 1.2f : 0.6f));
        float mt = muffled.load() ? 1.f : 0.f;
        muffleAmt += (mt - muffleAmt) * std::min(1.f, rate * 6.f);
        gPad   = 0.55f + 0.2f * (1.f - sstep(0.9f, 1.3f, lvl));
        gKick  = sstep(0.25f, 0.6f, lvl);
        gBass  = sstep(0.3f, 0.7f, lvl);
        gHat   = 0.3f + 0.7f * sstep(0.2f, 0.8f, lvl);
        gSnare = sstep(0.7f, 1.f, lvl);
        gArp   = sstep(0.8f, 1.1f, lvl);
        gLead  = sstep(1.12f, 1.32f, lvl);
        gBassF = gOf(260.f + 2400.f * sstep(0.45f, 1.2f, lvl));
        gMuf   = gOf(520.f);
        gSnareF = gOf(1600.f); gHatF = gOf(7200.f); gArpF = gOf(2600.f + 1800.f * sstep(1.f, 1.3f, lvl));
        gLeadF = gOf(3200.f); gPadF = gOf(700.f + 500.f * sstep(0.6f, 1.2f, lvl)); gCrashF = gOf(5000.f);
    }

    void nextStep() {
        step = (step + 1) % 64;
        if (step % 16 == 0) {                                     // bar line: maybe change track
            int want = ((wantTrack.load() % MUSIC_TRACKS) + MUSIC_TRACKS) % MUSIC_TRACKS;
            if (want != track) { track = want; step = 0; }
        }
        const MusicTrack& tr = musicTrack(track);
        stepLen = SR * 60.f / tr.bpm / 4.f;
        int s = step % 16, bar = step / 16;
        int deg = tr.chords[bar];

        if (s == 0) {                                             // new chord: pad, crash
            for (int k = 0; k < 3; ++k) padNotes[k] = (float)chordTone(tr, deg, k, 0);
            padNotes[3] = (float)scaleNote(tr, deg, -2);          // sub, two octaves down
            if (gLead > 0.05f) crash = Hit{0.f, 1.f};
        }
        auto at = [&](const char* p, int i) { return p[i % (int)std::strlen(p)]; };
        char k = at(tr.kick, s);
        if (k == 'x') kick = Hit{0.f, 1.f};
        char sn = at(tr.snare, s);
        if (sn == 'x') snare = Hit{0.f, 1.f}; else if (sn == 'g') snare = Hit{0.f, 0.35f};
        char h = at(tr.hat, s);
        if (h == 'x') hatC = Hit{0.f, 0.55f}; else if (h == 'X') hatC = Hit{0.f, 1.f}; else if (h == 'o') hatO = Hit{0.f, 1.f};

        char b = at(tr.bass, s);
        if (b != '.' && b != '-') {
            int n = chordTone(tr, deg, 0, -1);
            if (b == 'o') n += 12;
            if (b == 'f') n = chordTone(tr, deg, 2, -1);
            bassTarget = (float)n; bassEnvT = 0.f; bassGate = true;
        } else if (b == '.') bassGate = false;

        char a = at(tr.arp, s);
        if (a >= '0' && a <= '3') { arpNote = (float)chordTone(tr, deg, a - '0', 1); arpT = 0.f; arpLeft = !arpLeft; }

        char l = at(tr.lead, step % 32);
        if (l >= '0' && l <= '3') { leadNote = (float)chordTone(tr, deg, l - '0', 2); leadT = 0.f; leadOn = true; }
        else if (l == '.') leadOn = false;
    }

    void synthSample(const MusicTrack& tr, float& L, float& R) {
        const float dt = 1.f / SR;
        (void)tr;
        // ---- drums ----
        float drums = 0.f, hatOut = 0.f;
        if (kick.t < 0.6f) {
            float t = kick.t;
            float hz = 46.f + 120.f * std::exp(-t * 32.f);
            float body = kickOsc.sine(hz, SR) * std::exp(-t * 6.5f);
            float click = noise() * std::exp(-t * 500.f) * 0.25f;
            drums += (body * 1.05f + click) * gKick * kick.vel;
            kick.t += dt;
        }
        // Sidechain: the pad and bass duck under the kick
        float kd = kick.t < 0.4f ? std::exp(-kick.t * 9.f) * gKick : 0.f;
        duck = 1.f - 0.55f * kd;
        if (snare.t < 0.5f) {
            float t = snare.t;
            float n = snareF.hp(noise(), gSnareF, 1.2f) * std::exp(-t * 14.f);
            float body = snareOsc.sine(185.f - 30.f * t, SR) * std::exp(-t * 28.f);
            drums += (n * 0.55f + body * 0.45f) * gSnare * snare.vel;
            snare.t += dt;
        }
        if (hatC.t < 0.12f || hatO.t < 0.6f) {
            float n = hatF.hp(noise(), gHatF, 1.0f);
            float e = 0.f;
            if (hatC.t < 0.12f) { e += std::exp(-hatC.t * 70.f) * hatC.vel; hatC.t += dt; }
            if (hatO.t < 0.6f)  { e += std::exp(-hatO.t * 8.f) * 0.6f;      hatO.t += dt; }
            hatOut = n * e * 0.16f * gHat;
        }
        float crashOut = 0.f;
        if (crash.t < 2.f) {
            crashOut = crashF.hp(noise(), gCrashF, 1.f) * std::exp(-crash.t * 2.4f) * 0.08f * gLead;
            crash.t += dt;
        }

        // ---- bass: two detuned saws through a filter with a pluck envelope, plus a sub ----
        bassNote += (bassTarget - bassNote) * 0.02f;
        float bassOut = 0.f;
        {
            float target = bassGate ? 0.35f + 0.65f * std::exp(-bassEnvT * 9.f) : 0.f;
            bassAmp += (target - bassAmp) * 0.012f;               // ~2 ms: no clicks on or off
            float env = bassAmp;
            float hz = midiHz(bassNote);
            float raw = bass1.saw(hz * 1.004f, SR) + bass2.saw(hz * 0.996f, SR);
            float cut = gBassF * (0.8f + 1.4f * std::exp(-bassEnvT * 14.f));
            float f = bassF.lp(raw * 0.5f, std::min(cut, 2.f), 0.7f);
            float sb = sub.sine(hz * 0.5f, SR) * 0.6f;
            bassOut = std::tanh((f * 1.6f + sb) * env) * 0.42f * gBass * duck;
            bassEnvT += dt;
        }

        // ---- pad: three chord notes, two detuned saws each, split across L/R ----
        float padL = 0.f, padR = 0.f;
        for (int k = 0; k < 3; ++k) {
            padCur[k] += (padNotes[k] - padCur[k]) * 0.0015f;
            float hz = midiHz(padCur[k]);
            padL += pad[2 * k].saw(hz * 1.003f, SR);
            padR += pad[2 * k + 1].saw(hz * 0.997f, SR);
        }
        padCur[3] += (padNotes[3] - padCur[3]) * 0.002f;
        float subOut = pad[6].sine(midiHz(padCur[3]), SR) * 0.24f;
        padL = padFL.lp(padL * 0.11f, gPadF, 0.4f);
        padR = padFR.lp(padR * 0.11f, gPadF, 0.4f);
        float pg = gPad * (0.65f + 0.35f * duck);

        // ---- arp: a plucked square, alternating sides, into the delay ----
        float arpOut = 0.f;
        if (arpT < 0.3f) {
            float a = arpOsc.square(midiHz(arpNote), SR) * std::exp(-arpT * 16.f);
            arpOut = arpF.lp(a, gArpF, 0.6f) * 0.13f * gArp;
            arpT += dt;
        }
        // ---- lead: a saw with vibrato, legato, into the delay ----
        float leadOut = 0.f;
        if (gLead > 0.001f) {
            leadCur += (leadNote - leadCur) * 0.004f;
            float v = vib.sine(5.5f, SR) * 0.12f * std::min(1.f, leadT * 2.f);
            float hz = midiHz(leadCur + v);
            float target = leadOn ? 0.75f + 0.25f * std::exp(-leadT * 3.f) : 0.f;
            leadAmp += (target - leadAmp) * 0.004f;
            float env = leadAmp;
            float raw = leadOsc.saw(hz, SR) * 0.6f + leadOsc2.saw(hz * 1.006f, SR) * 0.4f;
            leadOut = leadF.lp(raw, gLeadF, 0.5f) * env * 0.12f * gLead;
            leadT += dt;
        }

        // ---- delay (dotted eighth, ping-pong) ----
        size_t dlen = delayL.size();
        size_t dtime = std::min(dlen - 1, (size_t)(stepLen * 3.f));
        size_t rd = (dpos + dlen - dtime) % dlen;
        float dl = delayL[rd], dr = delayR[rd];
        float sendL = arpOut * (arpLeft ? 1.f : 0.4f) + leadOut * 0.5f;
        float sendR = arpOut * (arpLeft ? 0.4f : 1.f) + leadOut * 0.5f;
        delayL[dpos] = sendL + dr * 0.38f;
        delayR[dpos] = sendR + dl * 0.38f;
        dpos = (dpos + 1) % dlen;

        float mono = drums + bassOut + subOut * pg + crashOut;
        L = mono + padL * pg + hatOut * 0.8f + sendL + dl * 0.3f + leadOut * 0.3f;
        R = mono + padR * pg + hatOut * 1.0f + sendR + dr * 0.3f + leadOut * 0.3f;
    }
};
