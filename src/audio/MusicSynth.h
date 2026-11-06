#pragma once
// =============================================================================
// MusicSynth.h — the soundtrack, synthesized live. No audio files: a small
// step sequencer drives drums, a rolling bass, a pad, an arpeggio and a lead,
// all built from oscillators, noise and filters, into a stereo float buffer.
// AudioSystem feeds it to SDL_mixer (Mix_HookMusic); the tests render it
// headless.
//
// Industrial techno / DnB at 170-178 BPM, plus a quiet exploration bed, with
// a track for each area and boss. It plays in layers that follow the action — the game sets an
// intensity and the layers fade in and out on their own:
//
//   intensity  0     pad and sub bass, quiet hats          (menu bed, death)
//              0.6   + kick, rolling bass (filtered)        (tubes, breathers)
//              1.0   + snare, open filter, arpeggio         (fighting)
//              1.3   + lead, crash on every bar             (the boss, OVERDRIVE)
//
// The fight can push it further (Gameplay_Flow.h updateMusic): OVERDRIVE (setDrive) adds a
// distorted power-chord stab on every eighth, sixteenth hats and the bass wide open; the last
// enemy of a wave (setSolo) strips it to the drums and the sub; a wave's end (stinger) rings a
// chord in the track's key over a crash.
// It doesn't just loop: every second pass through a track's four bars is its B side (the arp
// an octave up, the lead doubled an octave over itself), and the end of each B side has a drum
// fill into the next (the techno tracks have their own clap roll and riser). The snare, the lead,
// the arp, the pad and the stinger go into a plate reverb, the drums' kick and the bass stay dry.
// The breaks add ghost notes, pickup kicks and rides; the bass moves through a
// sixteen-bar filter phrase, with its sub centered and only upper harmonics widened.
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
    int   techno;          // 1: hard techno voicing (driven kick, clap, acid arp, a riser into every fourth bar); 2: harder still
                           // (a distorted kick with a long tail, a snare under the clap, a dirtier bass and sub, a deeper pump)
    bool breaks = false;   // two-step drums, short transients, moving Reese bass and rave stabs
};

constexpr int MUSIC_TRACKS = 17;

inline const MusicTrack& musicTrack(int i) {
    static const MusicTrack T[] = {
        {"SUNSET", 174.f, 57, {0, 5, 2, 6},
         "x.........x..x..", "....x..g.g..x..g", "x.XxxxX.x.XxxxXo",
         "x--.x-f.x--.o-f.", "0123012301230123", "3---2---1-2-0---3---2---1---0---", 0, true},
        {"FOUNDRY", 172.f, 50, {0, 0, 5, 6},
         "x...x...x...x..x", "....x..g....x...", "xxXxxxXxxxXxxxXo",
         ".xxx.xxx.xxx.xfx", "0.1.2.1.3.2.1.2.", "0-0-3---2-1-0---0-0-3---1---2---", 1},
        {"SPIRE", 172.f, 52, {0, 5, 3, 4},
         "x.....x...x.....", "....x..g.g..x..g", "x.XxxxX.x.XxxxXo",
         "x--.o-f.x---x-f.", "0213021302130213", "2-------1-------3-------2---1---", 0, true},
        {"CORE", 176.f, 49, {0, 5, 6, 4},
         "x.........x..x..", "....x..g.g..x..g", "x.XxxxX.x.XxxxXo",
         "x--.x-f.x--.o-f.", "0123321001233210", "3---2-3-1---0---3-2-1-0-1---2---", 0, true},
        // The Sovereign: a driving break under its falling theme
        {"SOVEREIGN", 174.f, 46, {0, 6, 5, 4},
         "x..x......x...x.", "....x..g.g..x..g", "x.XxxxX.x.XxxxXo",
         "x---x-f.x--.o-f.", "3210321032103201", "3-------2---1---0-------1-2-3---", 0, true},
        // The Nave: cold bells over a fast, sparse two-step break
        {"NAVE", 170.f, 45, {0, 3, 5, 4},
         "x.........x..x..", "....x..g.g..x..g", "x.XxxxX.x.XxxxXo",
         "x---.--fx---o-f.", "0.2.1.3.0.2.1.3.", "2-------3---1---0-------1---2---", 0, true},
        // The Orrery: bright and turning, an arp that goes round and round
        {"ORRERY", 174.f, 48, {0, 4, 5, 3},
         "x.....x...x..x..", "....x..g.g..x..g", "x.XxxxX.x.XxxxXo",
         "x--.o-f.x--.x-f.", "0123210301232103", "3---2---1---2---3---1---0-------", 0, true},
        // The Descent: rolling industrial techno, low brass and chains
        {"DESCENT", 172.f, 43, {0, 6, 3, 5},
         "x...x...x...x...", "....x.......x..g", "x.x.X.x.x.xxX.xo",
         ".xxx.xxx.xxx.xfx", "0..1..2..3..2..1", "3-------2-------1---0---1-------", 1},
        // The Reliquary: hollow bells over a stripped-back break
        {"RELIQUARY", 172.f, 41, {0, 3, 6, 4},
         "x.........x.....", "....x..g.g..x..g", "x.XxxxX.x.XxxxXo",
         "x---.--.o---x-f.", "0...2...1...3...", "3-------------2-1---------------", 0, true},
        // The Maw: a heavy sub beneath fast drums, a lead that climbs and falls away
        {"LEVIATHAN", 174.f, 38, {0, 5, 6, 4},
         "x.........x..x..", "....x..g.g..x..g", "x.XxxxX.x.XxxxXo",
         "x---x-f.x---o-f.", "0.1.2.3.2.1.0.1.", "0-------1---2---3-------2-1-0---", 0, true},
        // The Zenith (!!!!!): hard techno. A driven four-on-the-floor, a rolling off-beat bass pumping
        // under it, claps on two and four, an acid line whose filter opens over four bars, staccato stabs
        // on top, and a riser and clap roll into every fourth bar. It never lets up.
        {"ZENITH", 172.f, 50, {0, 0, 5, 6},
         "x...x...x...x...", "....x.......x...", "xxoxxxoxxxoxxxoX",
         ".xxx.xxx.xxx.xxx", "0030102300301023", "3.3.3.2.3.3.3.0.3.3.3.2.1.1.1.2.", 1},
        // MORNINGSTAR (inside the eclipse): faster and harder than the Zenith. A distorted kick with a long
        // tail and a pickup into every bar, clap and snare together, a dirty rolling bass over a heavy sub,
        // sixteenth hats, the acid line, stabs on top
        {"MORNINGSTAR", 178.f, 47, {0, 5, 3, 6},
         "x...x...x...x..x", "....x.......x..g", "xXoxxXoxxXoxxXoX",
         ".xxx.xxx.xxx.xfx", "0303102303031023", "3...3...2.1.0...3...3...1.2.3...", 2},
        // The lab has a separate harmonic language. These tracks use a four-part vowel
        // ensemble and pipe organ below, rather than the Zenith/Morningstar acid voices.
        {"OBSERVATION", 172.f, 50, {0, 5, 3, 4},   // a breakbeat under the choir: two-step kick, the snare on the backbeat, ghosts
         "x.........x..x..", "....x..g.g..x..g", "x.XxxxX.x.XxxxXo",
         "x--.x-f.x--.o-f.", "0.......2.......", "0-------2-------1-------0-------", 0, true},
        {"RELEASE DENIED", 176.f, 50, {0, 5, 3, 4},
         "x.........x..x..", "....x..g.g..x..g", "x.XxxxX.x.XxxxXo",
         "x--.x-f.x--.o-f.", "0.2.1.2.0.2.3.2.", "0---2-1-0---3---2---1-2-0-------", 0, true},
        // THE BLOOD MOON PLAIN (Chapter 4): not this synth's band at all. An engine of its own
        // (plainStep/plainSample below): dark club techno at 174 (a distorted kick pumping everything,
        // a rolling bass, an acid line) under an orchestra moving at half its speed (war drums, a choir,
        // the horn theme, tremolo strings, a tolling bell, low brass, anvils), in E. The patterns here go unused.
        {"RECLAMATION", 174.f, 40, {0, 5, 3, 4},
         "x.......x..x....", "........x.......", "x...x...x...x.x.",
         "x-------x---f---", "0.......3.......", "0-------1---2---3-------2---1---", 0},
        // THE COLD STORE (Chapter 5), on the lab's choir and organ: slow and glassy, a music-box arp
        // turning over a held bass, the lead falling a step a bar. Outdoors the wind carries most of it.
        {"COLD STORAGE", 96.f, 45, {0, 5, 3, 6},
         "x...............", "................", "....x.......x..o",
         "x-------o-------", "0..2..1..3..2..1", "3-------2-------1-------0-------", 0},
        // THE RESTORER: colder strings and choir over a fast break, the lead climbing and turning back
        {"THE RESTORER", 174.f, 41, {0, 6, 5, 4},
         "x..x......x..x..", "....x..g.g..x..g", "x.XxxxX.x.XxxxXo",
         "x--.x-f.x--.o-f.", "0.1.2.3.2.1.0.2.", "3---2---1-------0---1---2---3---", 0, true},
    };
    return T[((i % MUSIC_TRACKS) + MUSIC_TRACKS) % MUSIC_TRACKS];
}

class MusicSynth {
public:
    // ---- set from the game thread ----
    void setTrack(int t)       { wantTrack.store(t); }
    void setIntensity(float x) { wantLevel.store(x); }
    void setVolume(float v)    { volume.store(v); }
    void setMuffle(bool m)     { muffled.store(m); }
    void setDrive(bool d)      { wantDrive.store(d); }
    void setSolo(bool s)       { wantSolo.store(s); }
    void stinger()             { stingWanted.store(true); }
    // A tape rewind (the Restorer's ROLLBACK): the last moment of the score plays backwards, faster and
    // faster, then the song picks up half a bar earlier than where it was
    void rewind()              { rewindWanted.store(true); }
    bool rewinding() const     { return rewindT > 0; }
    void setSampleRate(float sr) {
        SR = sr; delayL.assign((size_t)(sr * 1.5f), 0.f); delayR = delayL; dpos = 0;
        hist.assign((size_t)(sr * HIST_SECONDS) * 2, 0.f); histPos = 0; rewindT = 0;
        static const int COMB[4] = {1309, 1427, 1559, 1663}, AP[2] = {556, 441};
        const float k = sr / 44100.f;
        for (int i = 0; i < 4; ++i) {
            combL[i].buf.assign((size_t)std::max(1, (int)(COMB[i] * k)), 0.f);
            combR[i].buf.assign((size_t)std::max(1, (int)((COMB[i] + 23) * k)), 0.f);
            combL[i].i = combR[i].i = 0; combL[i].store = combR[i].store = 0.f;
        }
        for (int i = 0; i < 2; ++i) {
            apL[i].buf.assign((size_t)std::max(1, (int)(AP[i] * k)), 0.f);
            apR[i].buf.assign((size_t)std::max(1, (int)((AP[i] + 23) * k)), 0.f);
            apL[i].i = apR[i].i = 0;
        }
    }

    MusicSynth() { setSampleRate(44100.f); }

    // Render `frames` stereo frames, interleaved L R, into out (overwrites).
    void render(float* out, int frames) {
        float vol = volume.load();
        const int histFrames = (int)hist.size() / 2;
        if (rewindWanted.exchange(false) && histFrames > 0 && rewindT <= 0) {
            rewindT = (int)(REWIND_SECONDS * SR); rewindLen = rewindT; readHead = (double)histPos;
        }
        for (int f = 0; f < frames; ++f) {
            if (rewindT > 0) {   // the tape runs back: read the history backwards, the clock held
                float u = 1.f - (float)rewindT / (float)rewindLen;            // 0..1 through the rewind
                readHead -= 1.0 + 2.5 * u * u;                                // faster as it goes
                double h = std::fmod(readHead, (double)histFrames); if (h < 0) h += histFrames;
                int i0 = (int)h, i1 = (i0 + 1) % histFrames; float fr = (float)(h - i0);
                float env = std::min(1.f, (float)rewindT / (0.06f * SR)) * std::min(1.f, u * 40.f + 0.3f);   // no clicks in or out
                float wobble = 0.85f + 0.15f * std::sin(u * 40.f);
                out[2 * f]     = (hist[2 * i0] * (1 - fr) + hist[2 * i1] * fr) * env * wobble;
                out[2 * f + 1] = (hist[2 * i0 + 1] * (1 - fr) + hist[2 * i1 + 1] * fr) * env * wobble;
                if (--rewindT == 0) step = ((step - 8) % 64 + 64) % 64;       // and play on from half a bar back
                continue;
            }
            if ((blockPos++ & 31) == 0) updateBlock();
            if (sampleInStep <= 0) { nextStep(); sampleInStep += stepLen; }
            --sampleInStep;
            float L = 0.f, R = 0.f;
            synthSample(musicTrack(track), L, R);
            // Master: muffle filter, gentle saturation, volume
            float m = muffleAmt;
            if (m > 0.001f) {
                float lo = mufL.lp(L, gMuf, kMuf), ro = mufR.lp(R, gMuf, kMuf);
                L += (lo - L) * m; R += (ro - R) * m;
            }
            // Long driven techno kicks otherwise measure ~8 dB above the breaks.
            // Leave the quiet bed alone; reserve space for combat at full intensity.
            const MusicTrack& playing = musicTrack(track);
            float trim = playing.techno ? 1.f - (playing.techno >= 2 ? 0.64f : 0.6f) * gKick
                                       : plainTrack(track) ? 0.65f : 1.f;
            L = softClip(L * 0.75f * trim) * vol;
            R = softClip(R * 0.75f * trim) * vol;
            out[2 * f]     = L;
            out[2 * f + 1] = R;
            if (histFrames > 0) { hist[2 * histPos] = L; hist[2 * histPos + 1] = R; histPos = (histPos + 1) % histFrames; }
        }
    }

    float level() const { return lvl; }       // smoothed intensity (tests)
    int   currentTrack() const { return track; }
    // The clock, for layers made in time and in key with it (DriveBed.h): the audio thread, after render()
    double stepPos() const { return (double)std::max(step, 0) + 1.0 - (double)sampleInStep / std::max(1.f, stepLen); }   // in sixteenths
    float  samplesPerStep() const { return stepLen; }
    int    rootNote() const { return musicTrack(track).root; }
    int    pass() const { return cycle; }   // times round the four bars (odd: the B side)
    static bool labTrack(int t) { return t == 12 || t == 13 || t == 15 || t == 16; }   // the lab's choir and organ (and Chapter 5's)
    // The plain's score, one instrument at a time (tests, tuning): a bit per layer, set = silent.
    // 1 drone, 2 choir, 4 strings, 8 bell, 16 war drums, 32 low brass, 64 anvils, 128 horn
    unsigned plainMute = 0;
    static bool plainTrack(int t) { return t == 14; }            // the plain's brass and anvils

private:
    std::atomic<int>   wantTrack{0};
    std::atomic<float> wantLevel{0.5f};
    std::atomic<float> volume{0.6f};
    std::atomic<bool>  muffled{false};
    std::atomic<bool>  wantDrive{false}, wantSolo{false}, stingWanted{false}, rewindWanted{false};
    static constexpr float HIST_SECONDS = 1.5f, REWIND_SECONDS = 0.6f;
    std::vector<float> hist;         // the last moment of output (stereo), for rewind()
    int    histPos = 0, rewindT = 0, rewindLen = 1;
    double readHead = 0;

    float SR = 44100.f;
    int   track = 0;
    float lvl = 0.5f, muffleAmt = 0.f;
    int   step = -1;                 // 0..63 across the four-bar progression
    int   cycle = 0;                 // times round it on this track (odd: the B side)
    float bSide = 0.f;               // smoothed 0..1: the B side's octaves (no clicks)
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

    // A choir that sounds sung, not synthesized: four parts of four singers each, every singer a
    // little sharp or flat of the others, drifting, with their own vibrato that only comes in once
    // the note has settled, scooping up into each new note, breath in the tone, the section's
    // vowel through four formants (and a singer's formant for the ring of it), spread across the
    // stereo field. setVowel() moves the vowel; setNote() a part's pitch.
    struct HumanChoir {
        static constexpr int P = 4, S = 4;
        Osc src[P][S];
        float vibPh[P][S] = {}, vibRate[P][S] = {}, detune[P][S] = {}, drift[P][S] = {}, driftTo[P][S] = {}, pan[P][S] = {}, shimPh[P][S] = {};
        float tilt[P][S] = {};          // one-pole lowpass state: the glottis's soft top
        SVF formL[P][5], formR[P][5];
        float hz[P] = {146.8f, 174.6f, 220.f, 293.7f}, age[P] = {9, 9, 9, 9}, scoop[P] = {0, 0, 0, 0};
        float F[5] = {700, 1100, 2600, 3300, 2900}, Fto[5] = {700, 1100, 2600, 3300, 2900};
        float driftClock = 0.f;
        uint32_t r = 0x9e3779b9u;
        bool ready = false;
        float rnd() { r ^= r << 13; r ^= r >> 17; r ^= r << 5; return (float)(r & 0xffffff) / 16777216.f; }
        void init() {
            for (int p = 0; p < P; ++p) for (int k = 0; k < S; ++k) {
                detune[p][k] = (rnd() - 0.5f) * 0.016f;         // +-14 cents
                vibRate[p][k] = 4.6f + 1.5f * rnd();
                vibPh[p][k] = rnd(); shimPh[p][k] = rnd();
                pan[p][k] = (k + 0.5f) / S * 0.8f + 0.1f + (p % 2 ? 0.05f : -0.05f);
                src[p][k].ph = rnd();
            }
            ready = true;
        }
        void setNote(int p, float h) {
            if (std::fabs(h - hz[p]) > 0.5f) { scoop[p] = std::min(0.03f, std::fabs(std::log2(h / std::max(1.f, hz[p]))) * 0.25f + 0.012f); age[p] = 0.f; }
            hz[p] = h;
        }
        void setVowel(const float* f4) { for (int j = 0; j < 4; ++j) Fto[j] = f4[j]; }
        // One sample of the whole choir (L, R), at `amp`
        void sample(MusicSynth& m, float amp, float& L, float& R) {
            if (!ready) init();
            const float dt = 1.f / m.SR;
            for (int j = 0; j < 4; ++j) F[j] += (Fto[j] - F[j]) * 0.0009f;   // vowels move like a mouth, not a switch
            driftClock += dt;
            const bool newDrift = driftClock > 0.45f;
            if (newDrift) driftClock = 0.f;
            static const float W[5] = {1.f, 0.62f, 0.3f, 0.12f, 0.16f}, K[5] = {0.11f, 0.12f, 0.13f, 0.15f, 0.18f};
            float g[5]; for (int j = 0; j < 4; ++j) g[j] = m.gOf(F[j]); g[4] = m.gOf(2900.f);
            for (int p = 0; p < P; ++p) {
                age[p] += dt;
                float sc = 1.f - scoop[p] * std::exp(-age[p] * 14.f);    // into the note from just under it
                float vibIn = std::min(1.f, std::max(0.f, (age[p] - 0.22f) * 2.5f));
                float sl = 0.f, sr = 0.f;
                for (int k = 0; k < S; ++k) {
                    if (newDrift) driftTo[p][k] = (rnd() - 0.5f) * 0.006f;
                    drift[p][k] += (driftTo[p][k] - drift[p][k]) * 0.00012f;
                    vibPh[p][k] += vibRate[p][k] * dt; if (vibPh[p][k] >= 1.f) vibPh[p][k] -= 1.f;
                    shimPh[p][k] += (0.3f + 0.2f * k) * dt; if (shimPh[p][k] >= 1.f) shimPh[p][k] -= 1.f;
                    float vib = std::sin(6.2831853f * vibPh[p][k]) * 0.0045f * vibIn;
                    float f = hz[p] * (1.f + detune[p][k] + drift[p][k] + vib) * sc;
                    float raw = src[p][k].saw(f, m.SR);
                    tilt[p][k] += (raw - tilt[p][k]) * 0.35f;              // the glottis rounds the top off
                    float v = tilt[p][k] + m.noise() * 0.22f;              // and breath goes through it
                    v *= 0.85f + 0.15f * std::sin(6.2831853f * shimPh[p][k]);
                    sl += v * (1.f - pan[p][k]); sr += v * pan[p][k];
                }
                float ol = 0.f, orr = 0.f;
                for (int j = 0; j < 5; ++j) { formL[p][j].lp(sl, g[j], K[j]); formR[p][j].lp(sr, g[j], K[j]); ol += formL[p][j].band * W[j]; orr += formR[p][j].band * W[j]; }
                float pg = p == 3 ? 1.15f : 1.f;
                L += ol * amp * pg * 0.5f; R += orr * amp * pg * 0.5f;
            }
        }
    };
    HumanChoir humanChoir;

    // ---- voices ----
    struct Hit { float t = 1e9f, vel = 1.f; };                // time since trigger (s)
    Hit kick, snare, hatC, hatO, crash, ride;
    SVF snareF, hatF, crashF, rideF;
    Osc kickOsc, snareOsc, snareShell;

    float bassNote = 45.f, bassTarget = 45.f, bassEnvT = 1e9f, bassAmp = 0.f;
    bool  bassGate = false;
    Osc   bass1, bass2, sub;
    SVF   bassF, reeseLowL, reeseLowR;
    float bassMotion = 0.f, phraseEnergy = 1.f, reeseCut = 0.f, reeseSplit = 0.f;

    float padNotes[4] = {57, 60, 64, 69};
    float padCur[4]   = {57, 60, 64, 69};
    Osc   pad[8];
    SVF   padFL, padFR;

    float arpNote = 69.f, arpT = 1e9f; bool arpLeft = false;
    Osc   arpOsc; SVF arpF;
    float acidBase = 400.f;          // techno: the acid filter's cutoff before its pluck, opening over four bars
    SVF   riserF;

    float leadNote = 81.f, leadCur = 81.f, leadT = 1e9f, leadAmp = 0.f; bool leadOn = false;
    Osc   leadOsc, leadOsc2, vib; SVF leadF;

    std::vector<float> delayL, delayR; size_t dpos = 0;
    // The plate: 4 damped combs and 2 allpasses a side (SfxMixer's Freeverb-lite, longer and darker)
    struct Comb {
        std::vector<float> buf; size_t i = 0; float store = 0.f;
        float run(float in, float fb, float damp) {
            float out = buf[i];
            store = out * (1.f - damp) + store * damp;
            if (std::fabs(store) < 1e-15f) store = 0.f;
            float w = in + store * fb;
            buf[i] = std::fabs(w) < 1e-15f ? 0.f : w;
            if (++i >= buf.size()) i = 0;
            return out;
        }
    };
    struct Allpass {
        std::vector<float> buf; size_t i = 0;
        float run(float in) {
            float b = buf[i], out = b - in, w = in + b * 0.5f;
            buf[i] = std::fabs(w) < 1e-15f ? 0.f : w;
            if (++i >= buf.size()) i = 0;
            return out;
        }
    };
    Comb  combL[4], combR[4];
    Allpass apL[2], apR[2];
    Osc   leadHi, leadHi2;
    // Four glottal voices, two singers each, through moving AH / OH / EH formants.
    // This is an actual vocal layer, separate from the existing saw pad.
    Osc choirOsc[8], choirVib[4], organ[4];SVF choirForm[4][3];
    float choirHz[4]={146.83f,174.61f,220.f,293.66f}, choirNote[4]={50,53,57,62};
    float choirG[3]={0,0,0}, choirAmp=0, choirClock=0, organHz=293.66f;
    float F1lab=800, F2lab=1150, F3lab=2900;
    SVF mufL, mufR;
    // ======== THE PLAIN's score (track 14) ========
    // Its own orchestra, layered by intensity: below ~0.9 the approach (a low drone, an "oo" choir,
    // strings trembling on a minor-second cluster, a bell tolling every two bars, one war drum, the
    // horn theme an octave down and far off); from 0.9 the fight (the full taiko pattern, low brass
    // stabbing 3-3-2, the choir opening to "ah", the strings on the chord, the horn theme up close,
    // anvils on two and four).
    struct Strike { float t = 1e9f, hz = 55.f, vel = 1.f; };
    Strike taikoBig, taikoMid, brassHit, bellToll, anvilHit;
    Osc  taikoOsc[3], bellOsc[5], brassStab[6], hornOsc[3], hornVib, strOsc[8], droneOsc[4];
    SVF  taikoF, brassStabF, hornF1, hornF2, hornLP, strFL, strFR, droneF;
    float hornCur = 52.f, hornTarget = 52.f, hornAmp = 0.f, hornT = 0.f; bool hornOn = false; int hornEnd = -1;
    float plainRoot = 40.f, strNotes[4] = {76, 77, 75, 76}, plainFormant[3] = {300, 870, 2240};
    float strCur[4] = {76, 77, 75, 76}, tremPh = 0.f;

    // The club under it: kick, clap, hats, the rolling bass, the acid line, and the pump they all breathe with
    Strike pKick, pClap, pHat, pHatO, pBass;
    Osc  pKickOsc, pBassOsc[2], acidOsc[2];
    SVF  pClapF, pHatF, pBassF, acidF;
    float acidCur = 52.f, acidTo = 52.f, acidEnv = 0.f, acidAcc = 0.f, acidT = 9.f; bool acidSlide = false, acidOn = false;
    float pumpT = 9.f;
    // The orchestra's clock runs at half the drums' (its chords, its theme, its war drums): 0..127 through its eight bars
    int plainPos16() const { return (cycle & 3) * 64 + std::max(step, 0); }
    int plainPos() const { return plainPos16() / 2; }

    void plainStep(int s) {
        // The bass, then the choir's four voices, for each of the eight bars
        static const int CH[8][5] = {{40, 52, 55, 59, 64}, {36, 52, 55, 60, 64}, {33, 52, 57, 60, 64}, {35, 51, 54, 59, 63},
                                     {40, 52, 55, 59, 64}, {41, 53, 57, 60, 65}, {36, 52, 55, 60, 64}, {35, 51, 54, 59, 66}};
        // The Gleaner's theme: (step in the eight bars, note, length)
        struct Note { int at, note, len; };
        static const Note THEME[] = {{0, 64, 8}, {8, 67, 4}, {12, 66, 4}, {16, 64, 6}, {22, 62, 2}, {24, 64, 8},
                                     {32, 60, 8}, {40, 59, 4}, {44, 57, 4}, {48, 59, 14},
                                     {64, 64, 8}, {72, 67, 4}, {76, 69, 4}, {80, 69, 6}, {86, 67, 2}, {88, 65, 8},
                                     {96, 64, 8}, {104, 60, 4}, {108, 62, 4}, {112, 59, 8}, {120, 63, 4}, {124, 66, 4}};
        const int pos = plainPos(), b8 = pos / 16;
        const bool fight = lvl >= 0.9f;
        // ---- the club: every sixteenth ----
        {
            if (lvl >= 0.55f ? s % 4 == 0 : (lvl >= 0.3f && s == 0)) { pKick = Strike{0.f, 0.f, lvl >= 0.55f ? 1.f : 0.6f}; pumpT = 0.f; }
            if (lvl >= 0.3f && s % 4 != 0) pBass = Strike{0.f, midiHz((float)CH[b8][0] + 12.f), s % 4 == 2 ? 1.f : 0.7f};
            if (lvl >= 0.55f && s % 4 == 2) pHatO = Strike{0.f, 0.f, 1.f};
            if (lvl >= 0.9f && s % 4 != 2) pHat = Strike{0.f, 0.f, s % 2 ? 0.75f : 0.45f};
            if (lvl >= 0.9f && (s == 4 || s == 12)) pClap = Strike{0.f, 0.f, 1.f};
            if (lvl >= 0.9f && (pos / 16) % 4 == 3 && (pos % 16) >= 12 && s % 2 == 1) pClap = Strike{0.f, 0.f, 0.55f};   // a roll into the phrase
            // The acid line: E phrygian round the bar's root, accents, slides
            static const int AC[16] = {0, 0, 12, 0, 1, 99, 0, 3, 0, 12, 0, 7, 99, 1, 0, 10};
            static const bool ACC[16] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0}, SLIDE[16] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
            if (lvl >= 0.85f && AC[s] != 99) {
                acidTo = (float)CH[b8][0] + 24.f + (float)AC[s];
                acidSlide = SLIDE[(s + 15) % 16];
                if (!acidSlide) { acidCur = acidTo; acidEnv = 1.f; }
                acidAcc = ACC[s] ? 1.f : 0.f; acidOn = true; acidT = 0.f;
            } else if (AC[s] == 99) acidOn = false;
        }
        if (plainPos16() % 2) return;   // (the orchestra moves on the eighths)
        s = pos % 16;
        if (s == 0) {
            plainRoot = (float)CH[b8][0];
            for (int k = 0; k < 4; ++k) { choirNote[k] = (float)CH[b8][k + 1]; choirHz[k] = midiHz(choirNote[k]); }
            if (fight) for (int k = 0; k < 4; ++k) strNotes[k] = (float)CH[b8][k + 1] + 24.f;
            else { strNotes[0] = 76; strNotes[1] = 77; strNotes[2] = 75; strNotes[3] = 76; }   // E F E-flat E: it will not resolve
            if (b8 % 2 == 0) bellToll = Strike{0.f, midiHz(plainRoot + 24.f), 1.f};
            const float oo[3] = {300, 870, 2240}, ah[3] = {700, 1100, 2600};
            float a = sstep(0.8f, 1.1f, lvl);
            for (int k = 0; k < 3; ++k) { plainFormant[k] = oo[k] + (ah[k] - oo[k]) * a; choirG[k] = gOf(plainFormant[k]); }
        }
        // War drums: one great drum every other bar on the approach; the full pattern in the fight, a fill into the top
        const char* pat = !fight ? (b8 % 2 == 0 ? "B..............." : "................")
                        : b8 == 7 ? "B.B.m.m.BmBmBBBB" : "B..m..B...m.B.m.";
        char c = pat[s];
        if (c == 'B') taikoBig = Strike{0.f, 52.f, fight ? 1.f : 0.7f};
        else if (c == 'm') taikoMid = Strike{0.f, 96.f, 0.8f};
        if (lvl >= 0.95f && (s == 0 || s == 3 || s == 6 || s == 8 || s == 11 || s == 14)) brassHit = Strike{0.f, midiHz(plainRoot), s == 0 ? 1.f : 0.72f};
        if (lvl >= 1.f && (s == 4 || s == 12)) anvilHit = Strike{0.f, 0.f, 1.f};
        if (drive > 0.02f && s % 2 == 0) stabT = 0.f;   // OVERDRIVE: a distorted power chord on every eighth, here too
        // The horn: the theme, an octave down and distant on the approach
        if (pos == hornEnd) hornOn = false;
        for (const auto& n : THEME) if (n.at == pos) {
            hornTarget = (float)n.note - (fight ? 0.f : 12.f);
            if (!hornOn || hornT > 0.3f) hornCur = hornTarget - 0.6f;   // (each attack scoops up into its note)
            hornOn = true; hornT = 0.f; hornEnd = (n.at + n.len) % 128;
        }
    }

    void plainSample(float& L, float& R) {
        const float dt = 1.f / SR;
        const float fight = sstep(0.85f, 1.05f, lvl), all = sstep(1.1f, 1.3f, lvl);
        float dryL = 0.f, dryR = 0.f, wet = 0.f;
        // The drone: the bar's root two octaves down and its fifth, felt more than heard
        {
            float hz = midiHz(plainRoot - 12.f);
            float d = droneOsc[0].saw(hz * 1.002f, SR) + droneOsc[1].saw(hz * 0.998f, SR) + 0.6f * droneOsc[2].saw(hz * 1.5f, SR);
            float o = droneF.lp(d, gOf(110.f + 60.f * fight), 0.7f) * 0.085f * (plainMute & 1u ? 0.f : 1.f);
            dryL += o; dryR += o;
        }
        // The choir
        {
            choirAmp += (1.f - choirAmp) * 0.00005f;
            float gl = 0.f, gr = 0.f;
            for (int k = 0; k < 4; ++k) {
                float hz = choirHz[k] * (1.f + choirVib[k].sine(4.3f + k * 0.21f, SR) * 0.004f);
                float glottal = (choirOsc[k * 2].saw(hz * 1.003f, SR) + choirOsc[k * 2 + 1].saw(hz * 0.997f, SR)) * 0.5f + noise() * 0.02f;
                float v = 0.f; const float w[3] = {0.9f, 0.5f, 0.15f};
                for (int j = 0; j < 3; ++j) { choirForm[k][j].lp(glottal, choirG[j], 0.16f); v += choirForm[k][j].band * w[j]; }
                gl += v * (k % 2 ? 0.6f : 1.f); gr += v * (k % 2 ? 1.f : 0.6f);
            }
            float g = choirAmp * (0.055f + 0.05f * fight + 0.03f * all) * (plainMute & 2u ? 0.f : 1.f);
            dryL += gl * g; dryR += gr * g; wet += (gl + gr) * g * 1.4f;
        }
        // Strings, trembling
        {
            tremPh += (2.f + 10.5f * fight) * dt; if (tremPh >= 1.f) tremPh -= 1.f;   // slow and uneasy far off, a shiver in the fight
            float trem = 0.65f + 0.35f * std::sin(6.2831853f * tremPh);
            float sl = 0.f, sr = 0.f;
            for (int k = 0; k < 4; ++k) {
                strCur[k] += (strNotes[k] - strCur[k]) * 0.0004f;
                float hz = midiHz(strCur[k]);
                float v = strOsc[2 * k].saw(hz * 1.004f, SR) + strOsc[2 * k + 1].saw(hz * 0.996f, SR);
                sl += v * (k % 2 ? 0.5f : 1.f); sr += v * (k % 2 ? 1.f : 0.5f);
            }
            float g = (0.045f + 0.035f * fight) * trem * (plainMute & 4u ? 0.f : 1.f);
            sl = strFL.lp(sl, gOf(3400.f), 0.6f) * g; sr = strFR.lp(sr, gOf(3400.f), 0.6f) * g;
            dryL += sl; dryR += sr; wet += (sl + sr) * 1.2f;
        }
        // The bell
        if (bellToll.t < 9.f) {
            static const float RATIO[5] = {1.f, 2.f, 2.4f, 3.f, 4.07f}, DECAY[5] = {0.45f, 0.7f, 0.9f, 1.3f, 1.9f};
            float b = 0.f;
            for (int k = 0; k < 5; ++k) b += bellOsc[k].sine(bellToll.hz * RATIO[k], SR) * std::exp(-bellToll.t * DECAY[k]) / (1.f + k * 0.6f);
            b *= 0.09f * std::min(1.f, bellToll.t * 300.f) * (plainMute & 8u ? 0.f : 1.f);
            dryL += b * 0.8f; dryR += b; wet += b * 2.f;
            bellToll.t += dt;
        }
        // War drums
        float drums = 0.f;
        if (taikoBig.t < 2.5f) {
            float t = taikoBig.t, hz = 46.f + 70.f * std::exp(-t * 16.f);
            float body = taikoOsc[0].sine(hz, SR) * std::exp(-t * 2.4f);
            float skin = taikoF.lp(noise(), gOf(1400.f), 0.7f) * std::exp(-t * 28.f) * 0.8f;
            float sub = taikoOsc[1].sine(34.f, SR) * std::exp(-t * 1.6f) * 0.6f * std::min(1.f, t * 60.f);
            drums += std::tanh((body + skin + sub) * 1.6f) * 0.42f * taikoBig.vel;
            taikoBig.t += dt;
        }
        if (taikoMid.t < 0.9f) {
            float t = taikoMid.t, hz = 88.f + 90.f * std::exp(-t * 26.f);
            drums += (taikoOsc[2].sine(hz, SR) * std::exp(-t * 6.f) + noise() * std::exp(-t * 40.f) * 0.4f) * 0.22f * taikoMid.vel;
            taikoMid.t += dt;
        }
        if (plainMute & 16u) drums = 0.f;
        dryL += drums; dryR += drums; wet += drums * 0.25f;
        // Low brass, stabbing
        if (brassHit.t < 0.6f) {
            float t = brassHit.t, raw = 0.f;
            const float mul[3] = {1.f, 1.5f, 2.f};
            for (int k = 0; k < 3; ++k) raw += brassStab[2 * k].saw(brassHit.hz * mul[k] * 1.004f, SR) + brassStab[2 * k + 1].saw(brassHit.hz * mul[k] * 0.996f, SR);
            float e = std::min(1.f, t * 180.f) * std::exp(-t * 5.5f);
            float o = std::tanh(brassStabF.lp(raw * 0.2f, gOf(260.f + 2600.f * std::exp(-t * 9.f)), 0.7f) * 2.4f) * e * 0.7f * brassHit.vel * fight * (plainMute & 32u ? 0.f : 1.f);
            dryL += o; dryR += o; wet += o * 0.5f;
            brassHit.t += dt;
        }
        // Anvils
        if (anvilHit.t < 1.6f) {
            float t = anvilHit.t;
            float a = (std::sin(6.2831853f * 523.f * t) * std::exp(-t * 3.f) + std::sin(6.2831853f * 1187.f * t) * 0.6f * std::exp(-t * 4.5f)
                     + std::sin(6.2831853f * 1856.f * t) * 0.35f * std::exp(-t * 6.f) + noise() * std::exp(-t * 90.f)) * 0.18f * (plainMute & 64u ? 0.f : 1.f);
            dryL += a * 0.7f; dryR += a; wet += a * 1.5f;
            anvilHit.t += dt;
        }
        // The horn: the theme
        {
            hornCur += (hornTarget - hornCur) * 0.0025f;
            float vib = hornVib.sine(5.2f, SR) * 0.1f * sstep(0.25f, 0.6f, hornT);
            float hz = midiHz(hornCur + vib);
            float target = hornOn ? 1.f : 0.f;
            hornAmp += (target - hornAmp) * (hornOn ? 0.0016f : 0.0004f);
            float raw = hornOsc[0].saw(hz, SR) * 0.6f + hornOsc[1].square(hz * 1.003f, SR) * 0.3f + hornOsc[2].saw(hz * 0.5f, SR) * 0.35f * fight;
            float f1 = hornF1.lp(raw, gOf(480.f), 0.35f), f2 = hornF2.lp(raw, gOf(1250.f), 0.4f);
            float h = hornLP.lp(hornF1.band * 1.2f + hornF2.band * 0.7f + f1 * 0.2f + f2 * 0.0f, gOf(2600.f), 0.6f);
            h = std::tanh(h * (1.4f + 1.2f * fight)) * hornAmp * (0.07f + 0.08f * fight) * (plainMute & 128u ? 0.f : 1.f);
            dryL += h * 0.9f; dryR += h; wet += h * (2.2f - 1.2f * fight);   // far off: mostly the hall
            hornT += dt;
        }
        // ---- the club ----
        float club = 0.f;
        const float pump = 1.f - 0.55f * std::exp(-pumpT * 9.f) * sstep(0.5f, 0.6f, lvl);   // everything breathes with the kick
        pumpT += dt;
        if (pKick.t < 0.5f) {
            float t = pKick.t, hz = 46.f + 190.f * std::exp(-t * 38.f);
            float body = std::tanh(pKickOsc.sine(hz, SR) * std::exp(-t * 6.5f) * 3.6f);
            club += (body + noise() * std::exp(-t * 900.f) * 0.45f) * 0.5f * pKick.vel;
            pKick.t += dt;
        }
        if (pBass.t < 0.3f) {
            float t = pBass.t, e = std::min(1.f, t * 600.f) * std::exp(-t * 15.f);
            float raw = pBassOsc[0].saw(pBass.hz, SR) + pBassOsc[1].saw(pBass.hz * 1.007f, SR) * 0.6f;
            float o = std::tanh(pBassF.lp(raw, gOf(150.f + (500.f + 900.f * fight) * e), 0.5f) * 2.f) * e * 0.26f * pBass.vel;
            club += o * pump;
            pBass.t += dt;
        }
        float hats = 0.f;
        if (pHat.t < 0.12f) { hats += pHatF.hp(noise(), gOf(7800.f), 0.6f) * std::exp(-pHat.t * 55.f) * 0.09f * pHat.vel; pHat.t += dt; }
        if (pHatO.t < 0.4f) { hats += pHatF.hp(noise(), gOf(6500.f), 0.5f) * std::exp(-pHatO.t * 11.f) * 0.07f; pHatO.t += dt; }
        if (pClap.t < 0.35f) {
            float t = pClap.t, e = (t < 0.026f ? (std::fmod(t, 0.009f) < 0.004f ? 1.f : 0.25f) : std::exp(-(t - 0.026f) * 16.f));
            float c = pClapF.lp(noise(), gOf(1250.f), 0.9f);
            hats += pClapF.band * e * 0.38f * pClap.vel; (void)c;
            pClap.t += dt;
        }
        float acid = 0.f;
        if (lvl >= 0.8f) {
            acidCur += (acidTo - acidCur) * (acidSlide ? 0.0012f : 1.f);
            acidEnv *= std::exp(-dt * (8.f - 3.f * acidAcc));
            float gate = acidOn ? 1.f : std::exp(-acidT * 30.f);
            acidT += dt;
            float raw = acidOsc[0].saw(midiHz(acidCur), SR) + acidOsc[1].square(midiHz(acidCur) * 0.5f, SR) * 0.25f;
            float open = sstep(0.85f, 1.25f, lvl) + drive * 0.5f;
            float cyc = (float)(plainPos16() % 64) / 64.f;
            float cut = 160.f + (380.f + 1700.f * open * (0.4f + 0.6f * cyc * cyc)) * (0.25f + acidEnv * (1.f + acidAcc));
            acid = std::tanh(acidF.lp(raw * 0.6f, gOf(cut), 0.13f) * (2.2f + 1.5f * acidAcc)) * gate * 0.12f * sstep(0.82f, 0.95f, lvl) * pump;
        }
        dryL += club + hats * 0.85f + acid * 0.9f; dryR += club + hats + acid; wet += acid * 0.4f + hats * 0.2f;
        drums += club + hats;
        // The last enemy (setSolo): everything but the war drums falls away
        const float keep = 1.f - 0.9f * solo;
        dryL = (dryL - drums) * keep + drums * (1.f + 0.15f * solo); dryR = (dryR - drums) * keep + drums * (1.f + 0.15f * solo); wet *= keep;
        // OVERDRIVE: the power chord on the bar's root, stabbing every eighth
        if (drive > 0.002f && stabT < 0.25f) {
            float notes[3] = {plainRoot + 12.f, plainRoot + 19.f, plainRoot + 24.f}, raw = 0.f;
            for (int k = 0; k < 3; ++k) raw += stab[2 * k].saw(midiHz(notes[k]) * 1.004f, SR) + stab[2 * k + 1].saw(midiHz(notes[k]) * 0.996f, SR);
            float e = std::exp(-stabT * 13.f) * std::min(1.f, stabT * 400.f);
            float st = std::tanh(stabF.lp(raw * 0.25f, gOf(1800.f + 2600.f * std::exp(-stabT * 20.f)), 0.6f) * 2.2f) * e * 0.48f * drive;
            dryL += st; dryR += st;
            stabT += dt;
        }
        // The hall: the plate, long
        float in = wet * 0.05f, pl = 0.f, pr = 0.f;
        for (int c2 = 0; c2 < 4; ++c2) { pl += combL[c2].run(in, 0.88f, 0.3f); pr += combR[c2].run(in, 0.88f, 0.3f); }
        for (int a2 = 0; a2 < 2; ++a2) { pl = apL[a2].run(pl); pr = apR[a2].run(pr); }
        // A wave's stinger still rings over it (Gameplay calls stinger() on the scream)
        float stingOut = 0.f;
        if (stingT < 3.f) {
            float raw = 0.f;
            for (int k = 0; k < 4; ++k) raw += sting[2 * k].saw(midiHz(choirNote[k] + 12.f) * 1.003f, SR);
            stingOut = stingF.lp(raw * 0.1f, gOf(1800.f * std::exp(-stingT * 0.8f) + 300.f), 0.5f) * std::min(1.f, stingT * 150.f) * std::exp(-stingT * 1.2f) * 0.6f;
            stingT += dt;
        }
        L = (dryL + pl * 1.1f + stingOut) * 1.25f;
        R = (dryR + pr * 1.1f + stingOut) * 1.25f;
    }

    // The fight's layers: OVERDRIVE's stabs, the last enemy's solo, a wave's stinger
    float drive = 0.f, solo = 0.f;   // smoothed 0..1
    float stabT = 1e9f; Osc stab[6]; SVF stabF;
    float stingT = 1e9f; float stingNotes[4] = {0, 0, 0, 0}; Osc sting[8]; SVF stingF;

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
        float dw = wantDrive.load() ? 1.f : 0.f, sw = wantSolo.load() ? 1.f : 0.f;
        drive += (dw - drive) * std::min(1.f, rate * (dw > drive ? 8.f : 1.5f));
        solo  += (sw - solo)  * std::min(1.f, rate * 4.f);
        if (stingWanted.exchange(false)) {   // a chord in the track's key (on the current bar's chord), over a crash
            const MusicTrack& st = musicTrack(track);
            int deg = st.chords[std::max(step, 0) / 16];
            for (int k = 0; k < 4; ++k) stingNotes[k] = (float)chordTone(st, deg, k, 1);
            stingT = 0.f; crash = Hit{0.f, 1.f};
        }
        bSide += ((cycle & 1 ? 1.f : 0.f) - bSide) * std::min(1.f, rate * 6.f);
        float mt = muffled.load() ? 1.f : 0.f;
        muffleAmt += (mt - muffleAmt) * std::min(1.f, rate * 6.f);
        gPad   = 0.55f + 0.2f * (1.f - sstep(0.9f, 1.3f, lvl));
        gKick  = sstep(0.25f, 0.6f, lvl);
        gBass  = sstep(0.3f, 0.7f, lvl);
        gHat   = 0.3f + 0.7f * sstep(0.2f, 0.8f, lvl);
        gSnare = sstep(0.7f, 1.f, lvl);
        gArp   = sstep(0.8f, 1.1f, lvl);
        gLead  = sstep(1.12f, 1.32f, lvl);
        gBassF = gOf(260.f + 2400.f * std::max(sstep(0.45f, 1.2f, lvl), drive));
        // Sixteen-bar form: three driving phrases, then a two-bar filtered turn.
        const bool turn = (cycle % 4 == 3) && step >= 32;
        phraseEnergy += ((turn ? 0.48f : 1.f) - phraseEnergy) * std::min(1.f, rate * 4.f);
        bassMotion += rate * musicTrack(track).bpm / 60.f * 0.125f;
        if (bassMotion >= 1.f) bassMotion -= 1.f;
        reeseCut = gOf((420.f + 1250.f * (0.5f + 0.5f * std::sin(6.2831853f * bassMotion))) * (0.5f + 0.5f * phraseEnergy));
        reeseSplit = gOf(220.f);
        gMuf   = gOf(520.f);
        gSnareF = gOf(1600.f); gHatF = gOf(7200.f); gArpF = gOf(2600.f + 1800.f * sstep(1.f, 1.3f, lvl));
        gLeadF = gOf(3200.f); gPadF = gOf(700.f + 500.f * sstep(0.6f, 1.2f, lvl)); gCrashF = gOf(5000.f);
        float cyc = (std::max(step, 0) + 1.f - sampleInStep / std::max(1.f, stepLen)) / 64.f;   // 0..1 across the four bars
        acidBase = 320.f + 1900.f * cyc * cyc;
        if(labTrack(track)){
            const int vowel=(std::max(step,0)/8)%3;
            static const float f[3][3]={{800,1150,2900},{450,800,2830},{400,1600,2700}};   // ah, oh, eh
            F1lab=f[vowel][0];F2lab=f[vowel][1];F3lab=f[vowel][2];
            for(int k=0;k<3;++k)choirG[k]=gOf(f[vowel][k]);
            for(int k=0;k<4;++k)choirHz[k]=midiHz(choirNote[k]);
            organHz=midiHz(leadNote-12.f);
        }
    }

    void nextStep() {
        step = (step + 1) % 64;
        if (step == 0) ++cycle;
        if (step % 16 == 0) {                                     // bar line: maybe change track
            int want = ((wantTrack.load() % MUSIC_TRACKS) + MUSIC_TRACKS) % MUSIC_TRACKS;
            if (want != track) { track = want; step = 0; cycle = 0; }
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
        if (plainTrack(track)) { plainStep(s); return; }   // the plain's own score
        if(labTrack(track) && s%4==0){
            // An eight-bar soprano answer over the repeating four-bar lower voices.
            static const int theme[32]={7,7,9,8,7,4,5,7,5,5,4,2,4,5,7,4,
                                        7,9,11,9,8,7,5,4,5,7,4,2,4,2,1,0};
            for(int k=0;k<3;++k)choirNote[k]=(float)chordTone(tr,deg,k,0);
            choirNote[3]=(float)scaleNote(tr,theme[((cycle%2)*16+step/4)%32],0);
            // The dominant's raised third leads back into D minor.
            if(deg==4)choirNote[1]+=1;
            choirClock=0;
        }
        auto at = [&](const char* p, int i) { return p[i % (int)std::strlen(p)]; };
        char k = at(tr.kick, s);
        if (k == 'x') kick = Hit{0.f, 1.f};
        if (tr.breaks && bar % 2 == 1 && s == 14) kick = Hit{0.f, 0.72f};
        if (tr.breaks && bar == 3 && (cycle & 1) && s == 15) kick = Hit{0.f, 0.82f};
        char sn = at(tr.snare, s);
        if (sn == 'x') snare = Hit{0.f, 1.f}; else if (sn == 'g') snare = Hit{0.f, 0.35f};
        if (tr.techno && bar == 3 && s >= 8 && (s >= 12 || s % 2 == 0))   // the clap roll into the next phrase
            snare = Hit{0.f, 0.35f + 0.65f * (s - 8) / 7.f};
        else if (!tr.techno && bar == 3 && (cycle & 1) && s >= 12 && gSnare > 0.3f)   // the B side's last beat: a fill into the top
            snare = Hit{0.f, 0.5f + 0.5f * (s - 12) / 3.f};
        // Ghost notes stay softer than the backbeat; accents imply a played break.
        if (tr.breaks && snare.t == 0.f && snare.vel < 0.5f)
            snare.vel = 0.16f + 0.07f * ((s + bar) % 3);
        if (tr.breaks && (cycle & 1) && s % 4 == 2) ride = Hit{0.f, bar % 2 ? 0.55f : 0.38f};
        char h = at(tr.hat, s);
        if (h == 'x') hatC = Hit{0.f, 0.55f}; else if (h == 'X') hatC = Hit{0.f, 1.f}; else if (h == 'o') hatO = Hit{0.f, 1.f};
        else if (h == '.' && drive > 0.3f) hatC = Hit{0.f, 0.4f * drive};   // OVERDRIVE: the gaps fill with sixteenths
        if (drive > 0.02f && s % 2 == 0) stabT = 0.f;                         // OVERDRIVE: a stab on every eighth

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
        if (plainTrack(track)) { plainSample(L, R); return; }
        const float dt = 1.f / SR;
        const bool techno = tr.techno != 0, hard = tr.techno >= 2, breaks = tr.breaks;
        // ---- drums ----
        float drums = 0.f, hatOut = 0.f;
        if (kick.t < 0.6f) {
            float t = kick.t;
            if (breaks) {
                // Short kick leaves room before the next broken-beat hit.
                float hz = 49.f + 185.f * std::exp(-t * 72.f);
                float env = std::exp(-t * 23.f) * std::min(1.f, t * 1800.f);
                float punch = std::tanh(kickOsc.sine(hz, SR) * 1.8f) * env;
                float click = noise() * std::exp(-t * 850.f) * 0.3f;
                drums += (punch * 1.15f + click) * gKick * kick.vel;
            } else if (hard) {                                           // harder still: a distorted body with a long tail, a hard click
                float hz = 44.f + 260.f * std::exp(-t * 48.f);
                float body = std::tanh(kickOsc.sine(hz, SR) * std::exp(-t * 5.f) * 5.5f) * 1.f;
                float click = noise() * std::exp(-t * 1100.f) * 0.7f;
                drums += (body + click) * gKick * kick.vel;
            } else if (techno) {                                  // harder: a faster, deeper drop, driven, a sharper click
                float hz = 47.f + 190.f * std::exp(-t * 40.f);
                float body = std::tanh(kickOsc.sine(hz, SR) * std::exp(-t * 7.5f) * 3.2f) * 1.35f;
                float click = noise() * std::exp(-t * 900.f) * 0.55f;
                drums += (body + click) * gKick * kick.vel;
            } else {
                float hz = 46.f + 120.f * std::exp(-t * 32.f);
                float body = kickOsc.sine(hz, SR) * std::exp(-t * 6.5f);
                float click = noise() * std::exp(-t * 500.f) * 0.25f;
                drums += (body * 1.05f + click) * gKick * kick.vel;
            }
            kick.t += dt;
        }
        // Sidechain: the pad and bass duck under the kick
        float kd = kick.t < 0.4f ? std::exp(-kick.t * 9.f) * gKick : 0.f;
        float sd = breaks && snare.t < 0.12f ? std::exp(-snare.t * 38.f) * snare.vel * gSnare : 0.f;
        duck = 1.f - std::min(0.78f, (hard ? 0.7f : 0.55f) * kd + 0.32f * sd);
        float snareOut = 0.f;
        if (snare.t < 0.5f) {
            float t = snare.t;
            if (breaks) {
                float snap = snareF.hp(noise(), gSnareF * 0.75f, 1.f) * std::exp(-t * 31.f);
                float shell = snareOsc.sine(182.f + 80.f * std::exp(-t * 80.f), SR) * std::exp(-t * 36.f);
                shell += snareShell.sine(337.f, SR) * std::exp(-t * 55.f) * 0.23f;
                snareOut = (snap * 0.78f + shell * 0.75f) * gSnare * snare.vel;
            } else if (techno) {                                         // a clap: three quick slaps and a short tail, band-passed
                snareF.lp(noise(), gSnareF * 0.8f, 0.9f);
                float e = std::exp(-t * 16.f) * 0.55f;
                for (float o : {0.f, 0.011f, 0.022f}) if (t >= o) e += std::exp(-(t - o) * 190.f);
                snareOut += snareF.band * e * 0.62f * gSnare * snare.vel;
                if (hard) snareOut += snareOsc.sine(200.f - 40.f * t, SR) * std::exp(-t * 24.f) * 0.4f * gSnare * snare.vel;   // a snare's body under it
            } else {
                float n = snareF.hp(noise(), gSnareF, 1.2f) * std::exp(-t * 14.f);
                float body = snareOsc.sine(185.f - 30.f * t, SR) * std::exp(-t * 28.f);
                snareOut += (n * 0.55f + body * 0.45f) * gSnare * snare.vel;
            }
            snare.t += dt;
            drums += snareOut;
        }
        if (hatC.t < 0.12f || hatO.t < 0.6f) {
            float n = hatF.hp(noise(), gHatF, 1.0f);
            float e = 0.f;
            if (hatC.t < 0.12f) { e += std::exp(-hatC.t * 70.f) * hatC.vel; hatC.t += dt; }
            if (hatO.t < 0.6f)  { e += std::exp(-hatO.t * 8.f) * 0.6f;      hatO.t += dt; }
            hatOut = n * e * 0.16f * gHat;
        }
        float rideOut = 0.f;
        if (ride.t < 0.3f) {
            rideOut = rideF.hp(noise(), gHatF * 0.8f, 1.1f) * std::exp(-ride.t * 24.f) * ride.vel * gHat * 0.09f;
            ride.t += dt;
        }
        float crashOut = 0.f;
        if (crash.t < 2.f) {
            crashOut = crashF.hp(noise(), gCrashF, 1.f) * std::exp(-crash.t * 2.4f) * 0.08f * std::max(gLead, stingT < 2.f ? 0.8f : 0.f);
            crash.t += dt;
        }

        // ---- bass: two detuned saws through a filter with a pluck envelope, plus a sub ----
        bassNote += (bassTarget - bassNote) * 0.02f;
        float bassOut = 0.f, bassWidth = 0.f;
        {
            float target = !bassGate ? 0.f : techno ? 0.15f + 0.85f * std::exp(-bassEnvT * 20.f)   // techno: short rolling plucks
                                                    : 0.35f + 0.65f * std::exp(-bassEnvT * 9.f);
            bassAmp += (target - bassAmp) * 0.012f;               // ~2 ms: no clicks on or off
            float env = bassAmp;
            float hz = midiHz(bassNote);
            float detune = breaks ? 0.012f : 0.004f;
            float a = bass1.saw(hz * (1.f + detune), SR), b = bass2.saw(hz * (1.f - detune), SR);
            float raw = a + b;
            float cut = gBassF * (0.8f + 1.4f * std::exp(-bassEnvT * 14.f));
            float f = bassF.lp(raw * 0.5f, breaks ? reeseCut : std::min(cut, 2.f), breaks ? 0.85f : 0.7f);
            if (breaks) {
                float upperL = a - reeseLowL.lp(a, reeseSplit, 1.4f);
                float upperR = b - reeseLowR.lp(b, reeseSplit, 1.4f);
                bassWidth = (upperL - upperR) * 0.045f * env * gBass * duck;
            }
            float sb = sub.sine(hz * 0.5f, SR) * 0.6f;
            bassOut = hard ? std::tanh((f * 2.6f + sb * 1.4f) * env) * 0.28f * gBass * duck   // (dirtier, a heavier sub)
                           : std::tanh((f * 1.6f + sb) * env) * (techno ? 0.32f : 0.42f) * gBass * duck;   // (techno: under the kick)
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
        float subOut = pad[6].sine(midiHz(padCur[3]), SR) * (breaks ? 0.08f : 0.24f);
        padL = padFL.lp(padL * 0.11f, gPadF, 0.4f);
        padR = padFR.lp(padR * 0.11f, gPadF, 0.4f);
        float pg = gPad * (techno ? 0.4f + 0.3f * duck : 0.65f + 0.35f * duck);   // techno: a darker bed that pumps harder

        // ---- arp: a plucked square, alternating sides, into the delay ----
        // (techno: an acid line - a saw an octave down through a screaming resonant filter)
        float arpOut = 0.f;
        if (arpT < 0.3f) {
            if (techno || breaks) {
                float a = arpOsc.saw(midiHz(arpNote - 12.f), SR) * std::exp(-arpT * 11.f);
                float g = gOf(acidBase * (1.f + 3.f * std::exp(-arpT * 28.f)));
                arpOut = std::tanh(arpF.lp(a, g, breaks ? 0.65f : 0.22f) * 1.8f) * (breaks ? 0.065f * phraseEnergy : 0.085f) * gArp;
            } else {
                float a = arpOsc.square(midiHz(arpNote + (bSide > 0.5f ? 12.f : 0.f)), SR) * std::exp(-arpT * 16.f);
                arpOut = arpF.lp(a, gArpF, 0.6f) * 0.13f * gArp;
            }
            arpT += dt;
        }
        // ---- techno: a noise riser through the last bar of every four ----
        float riser = 0.f;
        if (techno && step >= 48) {
            float u = (step - 48 + 1.f - sampleInStep / std::max(1.f, stepLen)) / 16.f;
            riser = riserF.hp(noise(), gOf(600.f + 7000.f * u * u), 0.8f) * u * u * 0.1f * gArp;
        }
        // ---- lead: a saw with vibrato, legato, into the delay ----
        float leadOut = 0.f;
        if (gLead > 0.001f) {
            leadCur += (leadNote - leadCur) * 0.004f;
            float v = vib.sine(5.5f, SR) * 0.12f * std::min(1.f, leadT * 2.f);
            float hz = midiHz(leadCur + v);
            float target = leadOn ? 0.75f + 0.25f * std::exp(-leadT * 3.f) : 0.f;
            leadAmp += (target - leadAmp) * 0.004f;
            float env = leadAmp * (breaks ? std::exp(-leadT * 8.f) * phraseEnergy : 1.f);
            float raw = leadOsc.saw(hz, SR) * 0.6f + leadOsc2.saw(hz * 1.006f, SR) * 0.4f;
            if (bSide > 0.001f) raw += (leadHi.saw(hz * 2.f, SR) * 0.6f + leadHi2.saw(hz * 2.012f, SR) * 0.4f) * 0.35f * bSide;   // the B side: an octave over it
            leadOut = leadF.lp(raw, gLeadF, 0.5f) * env * 0.12f * gLead;
            leadT += dt;
        }

        float choirL=0,choirR=0;
        if(labTrack(track)){
            float beat=SR*60.f/tr.bpm/SR; // seconds, independent of audio rate
            float target=std::min(1.f,choirClock/.035f)*(.78f+.22f*std::exp(-choirClock*12.f));
            target*=1.f-.3f*sstep(beat*.75f,beat,choirClock);
            choirAmp+=(target-choirAmp)*.002f;choirClock+=dt;
            {   // the ward's choir: sung (HumanChoir)
                for(int k=0;k<4;++k)humanChoir.setNote(k,choirHz[k]);
                float vf[4]={F1lab,F2lab,F3lab,3300.f};humanChoir.setVowel(vf);
                float gain=choirAmp*(track==13?.12f:.09f);
                humanChoir.sample(*this,gain,choirL,choirR);
            }
            // Organ doubles the theme in octaves; strings articulate the sixteenth pulse.
            float pipe=organ[0].sine(organHz*.5f,SR)*.45f+organ[1].sine(organHz,SR)*.6f+organ[2].sine(organHz*2,SR)*.22f+organ[3].sine(organHz*3,SR)*.1f;
            leadOut=pipe*leadAmp*.2f*gLead;
            padL*=.2f;padR*=.2f;arpOut*=.65f;
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

        // ---- OVERDRIVE: a distorted power chord (root, fifth, octave, detuned) stabbing every eighth, pumping with the kick ----
        float stabOut = 0.f;
        if (drive > 0.002f && stabT < 0.25f) {
            int deg = tr.chords[std::max(step, 0) / 16];
            float notes[3] = {(float)chordTone(tr, deg, 0, 0), (float)chordTone(tr, deg, 2, 0), (float)chordTone(tr, deg, 3, 0)};
            float raw = 0.f;
            for (int k = 0; k < 3; ++k) raw += stab[2 * k].saw(midiHz(notes[k]) * 1.004f, SR) + stab[2 * k + 1].saw(midiHz(notes[k]) * 0.996f, SR);
            float e = std::exp(-stabT * 13.f) * std::min(1.f, stabT * 400.f);
            stabOut = std::tanh(stabF.lp(raw * 0.25f, gOf(1800.f + 2600.f * std::exp(-stabT * 20.f)), 0.6f) * 2.2f) * e * 0.32f * drive * duck;
            stabT += dt;
        }
        // ---- a wave's end: the chord rings out, bright, into the delay ----
        float stingOut = 0.f;
        if (stingT < 3.f) {
            float raw = 0.f;
            for (int k = 0; k < 4; ++k) raw += sting[2 * k].saw(midiHz(stingNotes[k]) * 1.003f, SR) + sting[2 * k + 1].square(midiHz(stingNotes[k]) * 0.997f, SR) * 0.5f;
            float e = std::min(1.f, stingT * 150.f) * std::exp(-stingT * 1.6f);
            stingOut = stingF.lp(raw * 0.1f, gOf(2400.f * std::exp(-stingT * 0.8f) + 400.f), 0.5f) * e * 0.75f;
            stingT += dt;
        }
        // ---- the plate: the snare (the clap a little), the lead, the arp, the pad, the stinger ----
        float plateL = 0.f, plateR = 0.f;
        {
            const float keepS = 1.f - 0.9f * solo;
            float in = (snareOut * (breaks ? 0.15f : techno ? 0.25f : 0.5f) + leadOut * 0.6f + arpOut * 0.3f + (padL + padR) * pg * 0.12f * keepS
                        + stingOut * 0.5f + stabOut * 0.1f + (choirL+choirR)*1.8f) * 0.05f;
            for (int c = 0; c < 4; ++c) { plateL += combL[c].run(in, 0.84f, 0.38f); plateR += combR[c].run(in, 0.84f, 0.38f); }
            for (int a2 = 0; a2 < 2; ++a2) { plateL = apL[a2].run(plateL); plateR = apR[a2].run(plateR); }
            plateL *= 0.9f; plateR *= 0.9f;
        }
        // The last enemy: everything but the drums and the sub falls away
        const float keep = 1.f - 0.9f * solo;
        float mono = drums * (1.f + 0.15f * solo) + bassOut * (1.f - 0.75f * solo) + subOut * pg + crashOut + riser * keep + stabOut;
        L = mono + (padL * pg + bassWidth + rideOut) * keep + hatOut * 0.8f + (sendL + dl * 0.3f + leadOut * 0.3f) * keep + stingOut + plateL + choirL*keep;
        R = mono + (padR * pg - bassWidth + rideOut * 0.4f) * keep + hatOut * 1.0f + (sendR + dr * 0.3f + leadOut * 0.3f) * keep + stingOut + plateR + choirR*keep;
    }
};
