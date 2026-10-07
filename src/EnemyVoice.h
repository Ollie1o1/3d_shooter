#pragma once
// =============================================================================
// EnemyVoice.h — who gets to speak. Each frame the game reports its living
// enemies (VoiceIn); the VoiceDirector answers with the voices to play
// (VoiceCue), each tagged with a role so the mixer can rank it (SfxMixer.h):
//   TELL     every wind-up within 40 m, out of sight too, at priority; the
//            same enemy not twice in 0.15 s; at most 4 a frame (the nearest)
//   ACTION   a wind-up's release; hurts (one per enemy per 0.4 s, six a
//            second); deaths, always, with a level floor (death())
//   CHATTER  idles and steps from the nearest 4 within 25 m; spawn cues
//            (the nearest 3 a frame)
// Bosses chatter whatever the cap. Hollowed voices are the same sounds,
// changed: Enraged higher and driven, Twinned doubled, Haloed shimmering.
// No GL, no audio: the game plays the cues (GameplayState::playVoice).
// =============================================================================
#include "VoiceTable.h"
#include "AudioTypes.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct VoiceIn {
    int        uid = 0;
    EnemyType  type = EnemyType::HUSK;
    Hollow     hollow = Hollow::NONE;
    glm::vec3  pos{0.f};          // where its voice comes from (its chest)
    float      scale = 1.f;       // TWINNED copies are smaller: higher voices
    float      moveSpeed = 0.f, health = 0.f;
    AttackKind attack = AttackKind::NONE;
    float      telegraphTimer = 0.f;
    bool       telegraphStarted = false, staggered = false, beamOn = false, halo = false;
    bool       enraged = false, rose = false;   // this tick: a boss's phase two; the Penitent stood
    int        linkCount = 0;                   // a CONDUCTOR's tethers
};

struct VoiceCue {
    enum Loop : uint8_t { ONCE, START, STOP };
    std::string name;
    glm::vec3   pos{0.f};
    int         uid = 0;
    SoundRole   role = SoundRole::ACTION;
    float       volume = 1.f, pitch = 1.f, drive = 0.f, delay = 0.f, floor = 0.f;
    float       duckDb = 0.f;    // > 0: duck the rest of the mix this much (a boss winding up)
    bool        priority = false;
    Loop        loop = ONCE;     // START: a held sound that follows uid until its STOP
};

class VoiceDirector {
public:
    static constexpr float TELL_RANGE = 40.f, CHATTER_RANGE = 25.f, SPAWN_RANGE = 60.f, BOSS_RANGE = 90.f;
    static constexpr float TELL_REPEAT = 0.15f, HURT_GAP = 0.4f, IDLE_MIN = 3.f, IDLE_MAX = 7.f, FORGET_AFTER = 0.5f;
    static constexpr int   MAX_TELLS = 4, CHATTER_VOICES = 4, HURTS_PER_SEC = 6, SPAWNS_PER_FRAME = 3;

    explicit VoiceDirector(uint32_t seed = 0x51ED5EEDu) : rng(seed ? seed : 1u) {}

    void reset() { st.clear(); frame.clear(); out.clear(); hurtTimes.clear(); deadNow.clear(); now = 0.f; }
    void begin(glm::vec3 listener, float dt) { lis = listener; step = dt; now += dt; frame.clear(); out.clear(); deadNow.clear(); }
    void enemy(const VoiceIn& in) { frame.push_back(in); }
    int  tracked() const { return (int)st.size(); }

    // A kill: its death cry (any range), its held voice stopped, forgotten
    std::vector<VoiceCue> death(const VoiceIn& in) {
        std::vector<VoiceCue> cs;
        auto it = st.find(in.uid);
        if (it != st.end() && it->second.beam) { VoiceCue s = cue(in, voiceName(in.type, VoiceKind::ATTACK, AttackKind::BEAM), SoundRole::ACTION); s.loop = VoiceCue::STOP; cs.push_back(s); }
        if (it != st.end()) st.erase(it);
        deadNow.insert(in.uid);
        VoiceCue c = cue(in, voiceName(in.type, VoiceKind::DEATH), SoundRole::ACTION);
        c.floor = 0.5f;
        c.priority = true;   // a kill is heard even when wind-ups fill every enemy voice
        dress(in, c, cs);
        return cs;
    }

    // Removed without a kill (an objective met, a boss taking its summons):
    // no cry, but its held voice stops now and it's forgotten
    std::vector<VoiceCue> forget(int uid) {
        std::vector<VoiceCue> cs;
        auto it = st.find(uid);
        if (it == st.end()) return cs;
        if (it->second.beam) { VoiceCue c; c.uid = uid; c.loop = VoiceCue::STOP; c.name = "v_seraph_atk_beam"; cs.push_back(c); }
        st.erase(it);
        deadNow.insert(uid);
        return cs;
    }

    const std::vector<VoiceCue>& end() {
        // Who may chatter: the nearest few (bosses always)
        std::vector<std::pair<float, int>> near;
        for (int i = 0; i < (int)frame.size(); ++i) {
            const float d = glm::length(frame[i].pos - lis);
            if (!isBoss(frame[i].type) && d < CHATTER_RANGE) near.push_back({d, i});
        }
        std::sort(near.begin(), near.end());
        std::vector<char> may(frame.size(), 0);
        for (int k = 0; k < (int)near.size() && k < CHATTER_VOICES; ++k) may[near[k].second] = 1;
        while (!hurtTimes.empty() && now - hurtTimes.front() >= 1.f) hurtTimes.erase(hurtTimes.begin());

        struct Ranked { float d; int i; VoiceCue c; };
        std::vector<Ranked> tells, spawns;
        std::vector<VoiceCue> rest;
        for (int i = 0; i < (int)frame.size(); ++i) {
            const VoiceIn& in = frame[i];
            if (deadNow.count(in.uid)) continue;   // killed this frame: its death cry says it all
            const bool boss = isBoss(in.type);
            const float d = glm::length(in.pos - lis);
            const bool fresh = st.find(in.uid) == st.end();
            State& s = st[in.uid];
            if (fresh) {
                s.lastHealth = in.health; s.halo = in.halo; s.links = in.linkCount;
                s.nextIdle = now + 0.5f + rng01() * IDLE_MAX * 0.5f;
                if (d < (boss ? BOSS_RANGE : SPAWN_RANGE)) spawns.push_back({d, i, cue(in, voiceName(in.type, VoiceKind::SPAWN), SoundRole::CHATTER)});
            }
            s.lastSeen = now;
            // The wind-up
            if (in.telegraphStarted && in.attack != AttackKind::NONE && now - s.lastTell >= TELL_REPEAT &&
                d < (boss ? BOSS_RANGE : TELL_RANGE)) {
                VoiceCue c = cue(in, voiceName(in.type, VoiceKind::TELL, in.attack), SoundRole::TELL);
                c.priority = true;
                const bool sweep = in.attack == AttackKind::CENSER_LOW || in.attack == AttackKind::CENSER_HIGH;
                if (boss && !sweep) c.duckDb = 6.f;
                tells.push_back({d, i, c});
                s.lastTell = now;
            }
            // The release: its wind-up ran out (a stagger breaks it off silently)
            if (s.telegraphing && in.telegraphTimer <= 0.f && !in.staggered && hasRelease(s.attack) && s.attack != AttackKind::BEAM)
                rest.push_back(cue(in, voiceName(in.type, VoiceKind::ATTACK, s.attack), SoundRole::ACTION));
            s.telegraphing = in.telegraphTimer > 0.f && in.attack != AttackKind::NONE;
            s.attack = in.attack;
            // A beam: held while it's on
            if (in.beamOn != s.beam) {
                VoiceCue c = cue(in, voiceName(in.type, VoiceKind::ATTACK, AttackKind::BEAM), SoundRole::ACTION);
                c.priority = true; c.loop = in.beamOn ? VoiceCue::START : VoiceCue::STOP;
                rest.push_back(c);
                s.beam = in.beamOn;
            }
            // Hurt
            if (in.health < s.lastHealth - 0.5f && now - s.lastHurt >= HURT_GAP && (int)hurtTimes.size() < HURTS_PER_SEC) {
                rest.push_back(cue(in, voiceName(in.type, VoiceKind::HURT), SoundRole::ACTION));
                s.lastHurt = now; hurtTimes.push_back(now);
            }
            s.lastHealth = in.health;
            // One-offs
            if (s.halo && !in.halo) { VoiceCue c = cue(in, VOICE_HALO_BREAK, SoundRole::ACTION); c.floor = 0.3f; rest.push_back(c); }
            s.halo = in.halo;
            if (in.type == EnemyType::CONDUCTOR && in.linkCount > s.links) rest.push_back(cue(in, "v_conductor_link", SoundRole::ACTION));
            s.links = in.linkCount;
            if (in.enraged && (in.type == EnemyType::WARDEN || in.type == EnemyType::SOVEREIGN)) {
                VoiceCue c = cue(in, std::string("v_") + voiceKey(in.type) + "_enrage", SoundRole::TELL); c.priority = true; c.duckDb = 6.f; rest.push_back(c);
            }
            if (in.rose && in.type == EnemyType::PENITENT) {
                VoiceCue c = cue(in, "v_penitent_rise", SoundRole::TELL); c.priority = true; c.duckDb = 6.f; rest.push_back(c);
            }
            // Chatter: idles and steps (the nearest few, and bosses)
            const bool speaks = may[i] || (boss && d < BOSS_RANGE);
            if (now >= s.nextIdle) {
                if (speaks) rest.push_back(cue(in, voiceName(in.type, VoiceKind::IDLE), SoundRole::CHATTER));
                s.nextIdle = now + IDLE_MIN + rng01() * (IDLE_MAX - IDLE_MIN);
            }
            const float cad = moveCadence(in.type), full = std::max(0.1f, statsOf(in.type).speed);
            const float ratio = std::min(1.5f, in.moveSpeed / full);
            if (cad > 0.f && ratio > 0.1f) {
                s.stride += step * cad * ratio;
                if (s.stride >= 1.f) {
                    s.stride -= std::floor(s.stride);
                    if (speaks) {
                        VoiceCue c = cue(in, voiceName(in.type, VoiceKind::MOVE), SoundRole::CHATTER);
                        c.volume = 0.5f + 0.5f * std::min(1.f, ratio);
                        if (in.type == EnemyType::RIPPER || in.type == EnemyType::MITE) c.volume *= 0.5f;   // many quick little steps: keep them under
                        rest.push_back(c);
                    }
                }
            }
        }
        // Forget whoever hasn't been seen for a while (their held voice stops)
        for (auto it = st.begin(); it != st.end();) {
            if (now - it->second.lastSeen > FORGET_AFTER) {
                if (it->second.beam) { VoiceCue c; c.uid = it->first; c.loop = VoiceCue::STOP; c.name = "v_seraph_atk_beam"; out.push_back(c); }
                it = st.erase(it);
            } else ++it;
        }
        auto byDist = [](const Ranked& a, const Ranked& b) { return a.d < b.d; };
        std::sort(tells.begin(), tells.end(), byDist);
        std::sort(spawns.begin(), spawns.end(), byDist);
        for (int k = 0; k < (int)tells.size() && k < MAX_TELLS; ++k) dress(frame[tells[k].i], tells[k].c, out);
        for (auto& c : rest) dress(frameOf(c.uid), c, out);
        for (int k = 0; k < (int)spawns.size() && k < SPAWNS_PER_FRAME; ++k) dress(frame[spawns[k].i], spawns[k].c, out);
        return out;
    }

private:
    struct State {
        float lastHealth = 0.f, lastTell = -1e9f, lastHurt = -1e9f, nextIdle = 0.f, stride = 0.f, lastSeen = 0.f;
        bool  telegraphing = false, beam = false, halo = false;
        AttackKind attack = AttackKind::NONE;
        int   links = 0;
    };
    std::unordered_map<int, State> st;
    std::unordered_set<int> deadNow;
    std::vector<VoiceIn> frame;
    std::vector<VoiceCue> out;
    std::vector<float> hurtTimes;
    glm::vec3 lis{0.f};
    float now = 0.f, step = 0.f;
    uint32_t rng;

    float rng01() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (rng & 0xFFFFFF) / 16777215.f; }
    const VoiceIn& frameOf(int uid) const {
        for (const auto& f : frame) if (f.uid == uid) return f;
        return frame.front();   // rest only holds cues from this frame's enemies
    }
    static VoiceCue cue(const VoiceIn& in, const std::string& name, SoundRole role) {
        VoiceCue c; c.name = name; c.pos = in.pos; c.uid = in.uid; c.role = role; return c;
    }
    // The Hollowed: the same voice, changed
    static void dress(const VoiceIn& in, VoiceCue c, std::vector<VoiceCue>& to) {
        if (c.loop == VoiceCue::STOP) { to.push_back(c); return; }
        if (in.scale < 0.999f) c.pitch /= std::sqrt(std::max(0.3f, in.scale));
        if (in.hollow == Hollow::ENRAGED) { c.pitch *= 1.12f; c.drive = 0.35f; }
        to.push_back(c);
        if (in.hollow == Hollow::TWINNED && c.loop == VoiceCue::ONCE) {
            VoiceCue e = c; e.pitch *= 1.0087f; e.delay = 0.012f; e.volume *= 0.7f; e.priority = false; e.duckDb = 0.f;
            to.push_back(e);
        }
        const bool idle = c.name.size() > 5 && c.name.compare(c.name.size() - 5, 5, "_idle") == 0;
        if (in.hollow == Hollow::HALOED && in.halo && (c.role == SoundRole::TELL || idle)) {
            VoiceCue s = c; s.name = VOICE_HALO_SHIMMER; s.role = SoundRole::CHATTER; s.priority = false; s.duckDb = 0.f; s.volume *= 0.8f;
            to.push_back(s);
        }
    }
};
