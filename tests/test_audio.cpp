// Offline audio regression: shipped combat transients and the real callback DSP.
#include "audio/ImpactAudio.h"
#include "audio/MixTable.h"
#include "audio/MusicSynth.h"
#include "audio/SfxMixer.h"
#include <fstream>
#include <cstdio>
#include <cstring>
#include <iterator>

static int failures = 0;
static void check(bool ok, const char* message) {
    std::printf("%s %s\n", ok ? "ok:" : "FAIL:", message);
    failures += !ok;
}
static std::vector<float> wav(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::vector<unsigned char> b((std::istreambuf_iterator<char>(f)), {});
    auto u16 = [&](size_t p) { return unsigned(b[p]) | (unsigned(b[p + 1]) << 8); };
    auto u32 = [&](size_t p) { return u16(p) | (u16(p + 2) << 16); };
    if (b.size() < 44 || std::memcmp(b.data(), "RIFF", 4) || std::memcmp(b.data() + 8, "WAVE", 4)) return {};
    bool format = false;
    for (size_t p = 12; p + 8 <= b.size();) {
        size_t len = u32(p + 4), start = p + 8;
        if (len > b.size() - start) return {};
        if (!std::memcmp(b.data() + p, "fmt ", 4) && len >= 16)
            format = u16(start) == 1 && u16(start + 2) == 1 && u32(start + 4) == 44100 && u16(start + 14) == 16;
        if (!std::memcmp(b.data() + p, "data", 4) && format) {
            std::vector<float> x(len / 2);
            for (size_t i = 0; i < x.size(); ++i) x[i] = (int16_t)u16(start + 2 * i) / 32768.f;
            return x;
        }
        p = start + len + (len & 1);
    }
    return {};
}

int main() {
    bool assets = true, onset = true, levels = true, variants = true, tails = true;
    SfxMixer mixer;
    for (const char* name : {"sword_draw", "sword_swing", "sword_thrust", "sword_guard", "sword_hit",
                            "sword_heavy", "sword_pierce", "sword_block", "sword_break", "parry", "parry_heavy",
                            "hit_body", "hit_armor", "impact", "impact_metal", "impact_wood", "impact_ice"}) {
        std::vector<float> previous;
        for (int v = 0; v < 3; ++v) {
            auto x = wav("assets/sfx/" + std::string(name) + (v ? "_" + std::to_string(v) : "") + ".wav");
            if (x.empty()) { assets = false; continue; }
            assets &= std::all_of(x.begin(), x.end(), [](float f) { return std::isfinite(f) && std::fabs(f) < .98f; });
            size_t first = 0;
            while (first < x.size() && std::fabs(x[first]) < .01f) ++first;
            onset &= first < 44100 * .005; // No hidden pre-roll on a combat cue.
            const auto* mix = mixEntry(name);
            levels &= mix && std::fabs(shortTermDb(x.data(), x.size(), 44100) + mix->trimDb - mixTargetDb(mix->cls)) < .25f;
            variants &= previous.empty() || x != previous;
            tails &= std::fabs(x.back()) < .001f;
            previous = x;
            mixer.addSound(name, std::move(x));
        }
    }
    check(assets, "all 51 authored combat WAVs are valid and leave peak headroom");
    check(onset, "every combat cue starts within 5 ms");
    check(levels, "every variant matches its mix class within 0.25 dB");
    check(variants && tails, "variants differ and end without an abrupt sample edge");
    check(std::string(surfaceImpactSound(Mat::HULL)) == "impact_metal" &&
          std::string(surfaceImpactSound(Mat::SURGICAL)) == "impact_metal" &&
          std::string(surfaceImpactSound(Mat::SALVAGE)) == "impact_metal" &&
          std::string(surfaceImpactSound(Mat::WOOD)) == "impact_wood" &&
          std::string(surfaceImpactSound(Mat::ICE)) == "impact_ice" &&
          std::string(surfaceImpactSound(Mat::ROCK)) == "impact",
          "chapter materials retain distinct steel, wood, ice and stone impacts");
    // Ducking must create space without stealing or attenuating the player's parry.
    SfxMixer::Opts o; o.priority = true; o.unducked = true;
    auto h = mixer.play("parry_heavy", o);
    mixer.duck(6.f, .12f);
    std::vector<float> mix(4410 * 2, .15f);
    mixer.render(mix.data(), 4410);
    check(mixer.isPlaying(h) && mixer.duckLevel() < .53f, "heavy parry survives while the background dips 6 dB");
    // Exercise device rates, small callbacks, and all live transitions over multiple phrases.
    bool stable = true, clock = true;
    for (float sr : {22050.f, 44100.f, 48000.f}) {
        MusicSynth score; score.setSampleRate(sr); score.setVolume(1.f);
        std::vector<float> block(512 * 2);
        for (int t = 0; t < MUSIC_TRACKS; ++t) {
            score.setTrack(t); score.setIntensity(1.3f); score.setDrive(t % 2 != 0);
            int frames = (int)(sr * 4);
            for (int done = 0; done < frames; done += 512) {
                int n = std::min(512, frames - done);
                if (done == 512 * 80) { score.stinger(); score.setSolo(true); }
                if (done == 512 * 100) { score.setSolo(false); score.rewind(); }
                score.render(block.data(), n);
                bool clean = std::all_of(block.begin(), block.begin() + n * 2,
                    [](float f) { return std::isfinite(f) && std::fabs(f) <= 1.f; });
                if (!clean && stable) std::printf("bad sample: track %d at %.0f Hz, frame %d\n", t, sr, done);
                stable &= clean;
            }
            bool inTime = score.currentTrack() == t && std::fabs(score.samplesPerStep() - sr * 60.f / musicTrack(t).bpm / 4.f) < .01f;
            if (!inTime) std::printf("clock: requested %d, playing %d, %.0f Hz\n", t, score.currentTrack(), sr);
            clock &= inTime;
        }
    }
    check(stable && clock, "all tracks, stingers, OVERDRIVE, solo and rewind remain stable at 22.05/44.1/48 kHz");
    std::printf("%d audio failures\n", failures);
    return failures ? 1 : 0;
}
