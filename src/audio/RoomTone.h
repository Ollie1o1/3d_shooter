#pragma once
// =============================================================================
// RoomTone.h — each arena's own air, synthesized live under the music: no
// audio files, just noise, filters and a few sine partials.
//
//   YARD     a warm breeze; birds by day, cicadas once night falls (setNight)
//   FOUNDRY  furnace roar and rumble, mains hum, distant clanks that echo
//            down the hall, now and then a burst of steam
//   SPIRE    high wind whistling through the stone, a far-off bell
//   CORE     reactor hum and thrum, cooling fans, chirps of data
//   SANCTUM  a slow beating drone, cold hall air, a distant boom
//
// Walking from one arena to the next crossfades over a couple of seconds.
// setLevel ducks it (under a fight, a pause). The game thread sets atomics;
// the audio thread (AudioSystem's callback) renders. Tests render it headless.
// =============================================================================
#include <atomic>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include "audio/AudioTypes.h"

class RoomToneSynth {
public:
    void setSampleRate(float r) { sr = r; }
    void setRoom(RoomTone t)    { target.store((int)t, std::memory_order_relaxed); }
    void setNight(float n)      { night.store(std::clamp(n, 0.f, 1.f), std::memory_order_relaxed); }
    void setLevel(float l)      { level.store(std::max(0.f, l), std::memory_order_relaxed); }
    void setVolume(float v)     { volume.store(std::max(0.f, v), std::memory_order_relaxed); }

    // Audio thread: add the room (stereo, interleaved) into buf
    void render(float* buf, int frames) {
        const int want = target.load(std::memory_order_relaxed);
        const float lvlTarget = level.load(std::memory_order_relaxed) * volume.load(std::memory_order_relaxed);
        const float nightTarget = night.load(std::memory_order_relaxed);
        const float dt = (float)frames / sr;
        bool any = false;
        float g0[ROOMS], g1[ROOMS];
        for (int r = 1; r < ROOMS; ++r) {   // crossfade: ~2.5 s in, ~2.5 s out
            g0[r] = gain[r];
            float goal = r == want ? 1.f : 0.f;
            gain[r] += std::clamp(goal - gain[r], -dt / 2.5f, dt / 2.5f);
            g1[r] = gain[r];
            any = any || g0[r] > 1e-4f || g1[r] > 1e-4f;
        }
        float lv0 = levelNow;
        levelNow += (lvlTarget - levelNow) * std::min(1.f, dt * 2.f);
        nightNow += (nightTarget - nightNow) * std::min(1.f, dt * 0.5f);
        if (!any) return;
        for (int f = 0; f < frames; ++f) {
            float t = (float)(f + 1) / (float)frames;
            float lv = lv0 + (levelNow - lv0) * t;
            float L = 0.f, R = 0.f;
            for (int r = 1; r < ROOMS; ++r) {
                float g = g0[r] + (g1[r] - g0[r]) * t;
                if (g <= 1e-4f) continue;
                float l = 0.f, rr = 0.f;
                switch ((RoomTone)r) {
                    case RoomTone::YARD:    yard(l, rr);    break;
                    case RoomTone::FOUNDRY: foundry(l, rr); break;
                    case RoomTone::SPIRE:   spire(l, rr);   break;
                    case RoomTone::CORE:    core(l, rr);    break;
                    case RoomTone::SANCTUM: sanctum(l, rr); break;
                    default: break;
                }
                L += l * g; R += rr * g;
            }
            partials(L, R);
            buf[2 * f]     += L * lv;
            buf[2 * f + 1] += R * lv;
        }
    }

private:
    static constexpr int ROOMS = (int)RoomTone::COUNT;
    static constexpr float TAU = 6.2831853f;
    float sr = 44100.f;
    std::atomic<int>   target{0};
    std::atomic<float> night{0.f}, level{1.f}, volume{1.f};
    float gain[ROOMS] = {};
    float levelNow = 1.f, nightNow = 0.f;
    uint32_t rng = 0x9E3779B9u;

    float white() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (float)(rng & 0xFFFFFF) / 8388607.5f - 1.f; }
    float uni(float a, float b) { return a + (b - a) * (white() * 0.5f + 0.5f); }
    float onePole(float hz) const { return 1.f - std::exp(-TAU * hz / sr); }
    static void pan(float v, float p, float& l, float& r) {   // p -1 (left) .. 1 (right), equal power
        float a = (p + 1.f) * 0.7853982f;
        l += v * std::cos(a); r += v * std::sin(a);
    }

    // A state-variable filter (TPT): band-pass and low-pass from one update
    struct SVF {
        float ic1 = 0.f, ic2 = 0.f, bp = 0.f, lp = 0.f;
        void run(float x, float g, float k) {
            float a1 = 1.f / (1.f + g * (g + k)), a2 = g * a1, a3 = g * a2;
            float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.f * v1 - ic1; ic2 = 2.f * v2 - ic2; bp = v1; lp = v2;
        }
    };
    float svfG(float hz) const { return std::tan(3.14159265f * std::min(hz, sr * 0.45f) / sr); }

    // Struck things (clanks, bells, booms) are sums of decaying sine partials
    struct Partial { float ph = 0.f, hz = 0.f, amp = 0.f, decay = 1.f, glide = 1.f, pan = 0.f; int delay = 0; };
    static constexpr int MAX_PARTIALS = 40;
    Partial part[MAX_PARTIALS];
    void strike(float hz, float amp, float seconds, float p, float delaySec = 0.f, float glideTo = 1.f) {
        for (auto& q : part) if (q.amp < 1e-5f) {
            q.ph = 0.f; q.hz = hz; q.amp = amp; q.pan = p;
            q.decay = std::exp(-6.9f / (seconds * sr));          // -60 dB over `seconds`
            q.glide = std::pow(glideTo, 1.f / (seconds * sr));   // pitch drifts toward hz * glideTo
            q.delay = (int)(delaySec * sr);
            return;
        }
    }
    void partials(float& L, float& R) {
        for (auto& q : part) {
            if (q.amp < 1e-5f) continue;
            if (q.delay > 0) { --q.delay; continue; }
            q.ph += q.hz / sr; q.ph -= std::floor(q.ph);
            q.hz *= q.glide;
            pan(std::sin(TAU * q.ph) * q.amp, q.pan, L, R);
            q.amp *= q.decay;
        }
    }
    // Countdown to the next event, in samples
    struct Timer { float left = 0.f; bool due(float sr_) { left -= 1.f; if (left > 0.f) return false; (void)sr_; return true; } };

    // ---- SUNSET YARD ----------------------------------------------------------
    float yBreezeL = 0.f, yBreezeR = 0.f, yGust = 0.f;
    Timer yBird{44100.f * 3.f};
    int   yChirps = 0; float yChirpT = -1.f, yChirpLen = 0.f, yChirpPan = 0.f, yChirpHz = 0.f, yChirpPh = 0.f, yChirpGap = 0.f;
    float yCicPh[2] = {}, yCicAM[2] = {}, yCicEnv[2] = {}, yCicOn[2] = {0.f, 0.f}; Timer yCic[2] = {{44100.f}, {44100.f * 2.f}};
    void yard(float& L, float& R) {
        const float n = nightNow, day = 1.f - n;
        // Breeze: soft low noise with slow gusts, a little different in each ear
        yGust += 0.09f / sr;
        float gust = 0.65f + 0.35f * std::sin(TAU * yGust) * std::sin(TAU * yGust * 0.41f + 2.f);
        float k = onePole(420.f);
        yBreezeL += k * (white() - yBreezeL); yBreezeR += k * (white() - yBreezeR);
        float b = 0.27f * gust * (1.f - 0.35f * n);
        L += yBreezeL * b; R += yBreezeR * b;
        // Birds by day: little rising chirps in twos and threes
        if (day > 0.05f && yBird.due(sr)) {
            yBird.left = uni(4.f, 11.f) * sr / std::max(day, 0.2f);
            yChirps = 2 + (int)uni(0.f, 2.99f); yChirpPan = uni(-0.8f, 0.8f); yChirpHz = uni(2400.f, 3300.f);
            yChirpT = 0.f; yChirpLen = uni(0.05f, 0.09f); yChirpGap = 0.f;
        }
        if (yChirps > 0) {
            if (yChirpGap > 0.f) yChirpGap -= 1.f / sr;
            else {
                yChirpT += 1.f / sr;
                float u = yChirpT / yChirpLen;
                float hz = yChirpHz * (1.f + 0.35f * u);
                yChirpPh += hz / sr; yChirpPh -= std::floor(yChirpPh);
                float env = std::sin(3.14159f * std::min(u, 1.f));
                pan(std::sin(TAU * yChirpPh) * env * env * 0.045f * day, yChirpPan, L, R);
                if (u >= 1.f) { --yChirps; yChirpT = 0.f; yChirpGap = uni(0.05f, 0.12f); }
            }
        }
        // Cicadas at night: a high buzz chopped fast, in long rising bursts, one each side
        if (n > 0.02f) for (int i = 0; i < 2; ++i) {
            if (yCic[i].due(sr)) { yCicOn[i] = yCicOn[i] > 0.5f ? 0.f : 1.f; yCic[i].left = sr * (yCicOn[i] > 0.5f ? uni(2.5f, 5.f) : uni(1.f, 3.f)); }
            yCicEnv[i] += (yCicOn[i] - yCicEnv[i]) * onePole(1.2f);
            float hz = i ? 5100.f : 4450.f;
            yCicPh[i] += hz / sr; yCicPh[i] -= std::floor(yCicPh[i]);
            yCicAM[i] += (i ? 52.f : 46.f) / sr; yCicAM[i] -= std::floor(yCicAM[i]);
            float chop = std::sin(3.14159f * yCicAM[i]); chop *= chop; chop *= chop;
            pan(std::sin(TAU * yCicPh[i]) * chop * yCicEnv[i] * 0.026f * n, i ? 0.7f : -0.7f, L, R);
        }
    }

    // ---- THE FOUNDRY ----------------------------------------------------------
    float fBrown = 0.f, fRumble = 0.f, fBreath = 0.f, fHumPh = 0.f; SVF fRoar;
    Timer fClank{44100.f * 2.f}, fSteam{44100.f * 9.f};
    float fSteamT = -1.f, fSteamPan = 0.f, fSteamHP = 0.f;
    void foundry(float& L, float& R) {
        // Rumble: brown noise, low-passed
        fBrown = std::clamp(fBrown * 0.998f + white() * 0.04f, -1.f, 1.f);
        fRumble += onePole(90.f) * (fBrown - fRumble);
        // The furnace breathing: a band of noise that swells and falls
        fBreath += 0.17f / sr;
        fRoar.run(white(), svfG(330.f), 1.3f);
        float roar = fRoar.bp * (0.55f + 0.45f * std::sin(TAU * fBreath)) * 0.10f;
        // Mains hum through the machinery
        fHumPh += 50.f / sr; fHumPh -= std::floor(fHumPh);
        float hum = (std::sin(TAU * fHumPh) + 0.5f * std::sin(2.f * TAU * fHumPh) + 0.25f * std::sin(3.f * TAU * fHumPh)) * 0.008f;
        float c = fRumble * 0.07f + hum;
        L += c + roar * 0.9f; R += c + roar * 1.1f;
        // A clank somewhere in the hall, and its echo off the far wall
        if (fClank.due(sr)) {
            fClank.left = uni(2.5f, 8.f) * sr;
            float hz = uni(160.f, 420.f), p = uni(-0.9f, 0.9f), a = uni(0.012f, 0.03f), d = uni(0.6f, 1.4f);
            const float ratio[4] = {1.f, 2.76f, 5.40f, 8.93f}, amp[4] = {1.f, 0.6f, 0.4f, 0.25f};
            float echo = uni(0.28f, 0.42f);
            for (int k = 0; k < 4; ++k) {
                strike(hz * ratio[k], a * amp[k], d / (1.f + k * 0.5f), p);
                strike(hz * ratio[k], a * amp[k] * 0.3f, d / (1.f + k * 0.5f), -p * 0.6f, echo);
            }
        }
        // A burst of steam: hiss that swells, holds and dies away
        if (fSteam.due(sr)) { fSteam.left = uni(10.f, 22.f) * sr; fSteamT = 0.f; fSteamPan = uni(-0.8f, 0.8f); }
        if (fSteamT >= 0.f) {
            fSteamT += 1.f / sr;
            float env = fSteamT < 0.06f ? fSteamT / 0.06f : fSteamT < 0.9f ? 1.f : std::max(0.f, 1.f - (fSteamT - 0.9f) / 0.8f);
            float w = white();
            fSteamHP += onePole(2500.f) * (w - fSteamHP);
            pan((w - fSteamHP) * env * 0.035f, fSteamPan, L, R);
            if (fSteamT > 1.7f) fSteamT = -1.f;
        }
    }

    // ---- THE SPIRE ------------------------------------------------------------
    float sAirL = 0.f, sAirR = 0.f, sGust = 0.f, sDrift[2] = {0.3f, 0.7f}, sDriftV[2] = {}; SVF sWh[2];
    Timer sBell{44100.f * 6.f};
    void spire(float& L, float& R) {
        sGust += 0.06f / sr;
        float gust = 0.55f + 0.45f * std::sin(TAU * sGust) * std::sin(TAU * sGust * 0.53f + 1.3f);
        float k = onePole(650.f);
        sAirL += k * (white() - sAirL); sAirR += k * (white() - sAirR);
        float a = 0.15f * gust;
        L += sAirL * a; R += sAirR * a;
        // Whistles: narrow bands of noise whose pitch wanders with the gusts
        for (int i = 0; i < 2; ++i) {
            sDriftV[i] += (white() * 0.002f - sDriftV[i]) * 0.0005f;
            sDrift[i] = std::clamp(sDrift[i] + sDriftV[i] * 0.02f, 0.f, 1.f);
            float hz = (i ? 950.f : 620.f) + (i ? 700.f : 600.f) * sDrift[i] + 120.f * gust;
            sWh[i].run(white(), svfG(hz), 0.08f);
            pan(sWh[i].bp * 0.07f * gust * gust, i ? 0.6f : -0.6f, L, R);
        }
        // A bell somewhere far below
        if (sBell.due(sr)) {
            sBell.left = uni(16.f, 34.f) * sr;
            const float notes[4] = {196.f, 220.f, 246.9f, 261.6f};
            float hz = notes[(int)uni(0.f, 3.99f)], p = uni(-0.5f, 0.5f);
            const float ratio[7] = {0.5f, 1.f, 1.19f, 1.5f, 2.f, 2.52f, 3.f}, amp[7] = {0.5f, 1.f, 0.6f, 0.4f, 0.35f, 0.2f, 0.12f};
            for (int k = 0; k < 7; ++k) strike(hz * ratio[k], 0.012f * amp[k], uni(4.f, 7.f) / (1.f + k * 0.25f), p);
        }
    }

    // ---- THE CORE -------------------------------------------------------------
    float cHumPh = 0.f, cThrob = 0.f, cSubPh = 0.f; SVF cFan;
    Timer cData{44100.f};
    int cBlips = 0; float cBlipT = 0.f, cBlipLen = 0.f, cBlipHz = 0.f, cBlipPh = 0.f, cBlipPan = 0.f, cBlipGap = 0.f;
    void core(float& L, float& R) {
        cHumPh += 60.f / sr; cHumPh -= std::floor(cHumPh);
        cThrob += 0.4f / sr; cThrob -= std::floor(cThrob);
        float h = 0.f;
        const float amp[5] = {1.f, 0.5f, 0.35f, 0.2f, 0.12f};
        for (int k = 0; k < 5; ++k) h += amp[k] * std::sin(TAU * cHumPh * (k + 1));
        h = std::tanh(h * 1.4f) * 0.022f * (0.75f + 0.25f * std::sin(TAU * cThrob));
        // The reactor's thrum, felt more than heard
        cSubPh += 45.f / sr; cSubPh -= std::floor(cSubPh);
        float sub = std::sin(TAU * cSubPh) * 0.03f * (0.6f + 0.4f * std::sin(TAU * cThrob * 0.6f));
        cFan.run(white(), svfG(2200.f), 1.8f);
        float fan = cFan.bp * 0.012f;
        L += h + sub + fan; R += h + sub - fan * 0.3f;
        // Data: bursts of short square-ish blips on a pentatonic scale
        if (cData.due(sr)) { cData.left = uni(1.2f, 4.f) * sr; cBlips = 2 + (int)uni(0.f, 4.99f); cBlipPan = uni(-0.9f, 0.9f); cBlipGap = 0.f; cBlipT = 0.f; cBlipLen = 0.f; }
        if (cBlips > 0) {
            if (cBlipGap > 0.f) cBlipGap -= 1.f / sr;
            else {
                if (cBlipT == 0.f) {
                    const float scale[5] = {1.f, 1.125f, 1.25f, 1.5f, 1.6875f};
                    cBlipHz = 1046.5f * scale[(int)uni(0.f, 4.99f)] * (uni(0.f, 1.f) < 0.3f ? 2.f : 1.f);
                    cBlipLen = uni(0.035f, 0.07f);
                }
                cBlipT += 1.f / sr;
                cBlipPh += cBlipHz / sr; cBlipPh -= std::floor(cBlipPh);
                float v = std::sin(TAU * cBlipPh) + 0.3f * std::sin(3.f * TAU * cBlipPh);
                float u = cBlipT / cBlipLen, env = u < 0.1f ? u / 0.1f : 1.f - (u - 0.1f) / 0.9f;
                pan(v * std::max(env, 0.f) * 0.010f, cBlipPan, L, R);
                if (u >= 1.f) { --cBlips; cBlipT = 0.f; cBlipGap = uni(0.03f, 0.09f); }
            }
        }
    }

    // ---- THE SANCTUM ----------------------------------------------------------
    float nPh[4] = {}, nSwell = 0.f, nAir = 0.f;
    Timer nBoom{44100.f * 5.f};
    float nRumbleT = -1.f, nRumbleLP = 0.f, nRumblePan = 0.f;
    void sanctum(float& L, float& R) {
        nSwell += 0.05f / sr;
        float swell = 0.7f + 0.3f * std::sin(TAU * nSwell);
        const float hz[4] = {55.f, 55.45f, 82.4f, 110.3f}, amp[4] = {0.03f, 0.03f, 0.016f, 0.009f};
        float d = 0.f;
        for (int k = 0; k < 4; ++k) { nPh[k] += hz[k] / sr; nPh[k] -= std::floor(nPh[k]); d += std::sin(TAU * nPh[k]) * amp[k]; }
        d *= swell;
        nAir += onePole(280.f) * (white() - nAir);
        float air = nAir * 0.05f;
        L += d + air; R += d - air * 0.6f + white() * 0.0004f;
        // A distant boom: a falling sub thud with a roll of low noise after it
        if (nBoom.due(sr)) {
            nBoom.left = uni(14.f, 30.f) * sr;
            float p = uni(-0.6f, 0.6f);
            strike(40.f, 0.09f, 2.6f, p, 0.f, 0.72f);
            strike(62.f, 0.035f, 1.6f, p, 0.f, 0.8f);
            nRumbleT = 0.f; nRumblePan = p;
        }
        if (nRumbleT >= 0.f) {
            nRumbleT += 1.f / sr;
            nRumbleLP += onePole(160.f) * (white() - nRumbleLP);
            float env = nRumbleT < 0.04f ? nRumbleT / 0.04f : std::exp(-(nRumbleT - 0.04f) * 2.2f);
            pan(nRumbleLP * env * 0.12f, nRumblePan, L, R);
            if (nRumbleT > 2.5f) nRumbleT = -1.f;
        }
    }
};
