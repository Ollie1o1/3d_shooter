#pragma once
// =============================================================================
// MasterBus.h — the last stage of the whole mix: a gain, then a look-ahead
// peak limiter. No SDL, so tests and the preview tools use the same one.
//
// Before this, the mix was rounded off twice with tanh (once inside SfxMixer,
// once here) and scaled by 0.7. Anything past 0.8 was bent by the curve: every
// explosion over the music, every shotgun next to a boss hit, came out with
// harmonic distortion, and the 0.7 kept the quiet parts quiet. A limiter turns
// the whole mix down for the few milliseconds a peak needs, cleanly, and
// leaves everything under the ceiling untouched.
//
// How it guarantees the ceiling: each sample asks for the gain that would put
// it at the ceiling; that request is held for LOOKAHEAD samples (a running
// minimum), eased back up (release), then box-averaged over LOOKAHEAD samples.
// The audio is delayed LOOKAHEAD-1 samples, so a peak arrives exactly when the
// averaged gain has finished coming down for it: no overs, no clicks.
// Stereo-linked (one gain for both sides) so the image doesn't move.
// =============================================================================
#include <algorithm>
#include <array>
#include <cmath>

class MasterBus {
public:
    static constexpr int LOOKAHEAD = 64;          // ~1.5 ms at 44.1 kHz: shorter than any transient's rise you'd hear
    float inputGain = 0.85f;                      // the mix's level into the limiter (was a fixed 0.7 after the clip)
    float ceiling   = 0.84f;                      // ~-1.5 dBFS: headroom for the DAC's reconstruction between samples

    void setSampleRate(float sr) {
        relK = 1.f - std::exp(-1.f / (0.120f * std::max(8000.f, sr)));   // 120 ms back up
    }
    explicit MasterBus(float sr = 44100.f) { setSampleRate(sr); reset(); }

    void reset() {
        delayL.fill(0.f); delayR.fill(0.f); req.fill(1.f); box.fill(1.f);
        dpos = 0; env = 1.f; boxSum = (float)LOOKAHEAD; gainNow = 1.f; minDepth = 1.f;
    }

    // interleaved stereo, in place
    void process(float* io, int frames) {
        for (int f = 0; f < frames; ++f) {
            float l = io[2 * f] * inputGain, r = io[2 * f + 1] * inputGain;
            if (!std::isfinite(l) || std::fabs(l) > 1e6f) l = 0.f;   // NaN / inf: silence rather than a screech
            if (!std::isfinite(r) || std::fabs(r) > 1e6f) r = 0.f;
            const float pk = std::max(std::fabs(l), std::fabs(r));
            const float want = pk > ceiling ? ceiling / pk : 1.f;
            // running minimum of the requests over the last LOOKAHEAD samples
            req[dpos] = want;
            float held = 1.f;
            for (float q : req) held = std::min(held, q);
            // release: fall at once, rise slowly
            env = held < env ? held : env + (held - env) * relK;
            // box average over LOOKAHEAD: the gain comes down smoothly across the look-ahead
            boxSum += env - box[dpos]; box[dpos] = env;
            float g = boxSum / (float)LOOKAHEAD;
            // delayed audio (LOOKAHEAD - 1 samples)
            const int rd = (dpos + 1) % LOOKAHEAD;
            const float dl = delayL[rd], dr = delayR[rd];
            delayL[dpos] = l; delayR[dpos] = r;
            dpos = rd;
            float ol = dl * g, orr = dr * g;
            // float rounding in the running sum can leave a hair over: a hard stop at the ceiling, never reached in practice
            ol = std::clamp(ol, -ceiling, ceiling); orr = std::clamp(orr, -ceiling, ceiling);
            io[2 * f] = ol; io[2 * f + 1] = orr;
            gainNow = g; minDepth = std::min(minDepth, g);
        }
        boxSum = 0.f; for (float b : box) boxSum += b;   // re-sum once a buffer: no drift
    }

    float gain() const { return gainNow; }                   // the limiter's gain now (1 = not limiting)
    float deepest() { float d = minDepth; minDepth = 1.f; return d; }   // the deepest gain since the last call (dev meter)

private:
    std::array<float, LOOKAHEAD> delayL{}, delayR{}, req{}, box{};
    int dpos = 0;
    float env = 1.f, boxSum = (float)LOOKAHEAD, relK = 0.f, gainNow = 1.f, minDepth = 1.f;
};
