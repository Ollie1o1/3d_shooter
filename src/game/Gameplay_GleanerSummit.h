#pragma once
// =============================================================================
// Gameplay_GleanerSummit.h — GameplayState: THE SUMMIT, the end of the Gleaner
// fight (docs/chapter-4-design.md §5, phase 4). Up out of its chimney onto its
// roof: it tries to stand one last time, and THE EYE (EyeCore.h) tears out of
// its chest and rises on its cables over the roof's front. Its chimneys are your
// cover. Bring it down, walk up to it where it lies and tear it out (the
// interact key): the Gleaner falls, seen from far off across the plain; the
// meteors stop; the real moon opens again; the chapter's card.
// Included at the end of GameplayState.h (after Gameplay_GleanerInside.h).
// =============================================================================

namespace summit {
constexpr float END_LEN = 14.f;                      // the collapse, from the tear to the card
constexpr float CUT = 1.f;                           // the white: the far shot from here
constexpr float FALL0 = 1.6f, FALL1 = 9.f;           // it sinks into the plain
constexpr float SLAM = 7.2f;                         // its chest hits the ground
constexpr float MOON0 = 8.f, MOON1 = 11.5f;          // the real moon opens
constexpr float DARK0 = 12.8f;                       // to black, then the card
inline float sm(float a, float b, float x) { float u = std::clamp((x - a) / (b - a), 0.f, 1.f); return u * u * (3.f - 2.f * u); }
}

// Where the eye hangs: over the roof's front edge, high enough that you look up to it
inline glm::vec3 GameplayState::eyeHang() const {
    return {(deckLo.x + deckHi.x) * 0.5f, deckY + 15.f, deckHi.y + 14.f};
}

// The chimneys as boxes in the world (cover from its beams)
inline int GameplayState::summitCover(AABB* out) const {
    const float K = Gleaner::SCALE;
    for (int c = 0; c < 3; ++c) {
        glm::vec3 p = gleaner.toWorld(gleaner::chimney(c));
        float h = 4.5f * K;
        out[c] = {{p.x - h, deckY, p.z - h}, {p.x + h, deckY + gleaner::chimneyTall(c) * K, p.z + h}};
    }
    return 3;
}
// How far a beam from `o` along `d` goes before a chimney stops it (or `maxT`)
inline float GameplayState::eyeBeamReach(glm::vec3 o, glm::vec3 d, float maxT) const {
    AABB cover[3]; int n = summitCover(cover);
    float best = maxT;
    for (int i = 0; i < n; ++i) { float t = rayBoxHit(o, d, cover[i]); if (t > 0.f && t < best) best = t; }
    if (d.y < -0.01f) best = std::min(best, (deckY - o.y) / d.y);   // (the roof itself)
    return best;
}

inline void GameplayState::wakeEye() {
    eye.wake(gleaner.eye(), eyeHang(), deckLo, deckHi, deckY);
    gleaner.scream();
    pushBanner("THE EYE", "", {1.f, 0.2f, 0.12f}, 2.6f);
    audio.play("mini_sting", 128, SoundGroup::UI, true);
    shake(2.f, 0.03f);
}

// The summit: the eye's fight, then the collapse (its own cutscene)
inline void GameplayState::updateSummit(float dt) {
    if (quietT >= 0.f) { updateQuiet(dt); return; }   // (after it: Gameplay_GleanerFinale.h)
    if (tearT >= 0.f) { updateTear(dt); return; }
    if (summitEndT >= 0.f) { updateSummitEnd(dt); return; }
    EyeCore& e = eye;
    if (e.state == EyeCore::DORMANT) {
        if (onGleanerDeck() && player.onGround && !playerDead) wakeEye();
        return;
    }
    gleaner.eyeOpen -= gleaner.eyeOpen * std::min(1.f, dt * 1.5f);   // (its chest is empty now)
    const glm::vec3 chest = player.position + glm::vec3{0.f, 1.1f, 0.f};
    e.tick(dt, chest);
    SfxMixer::Opts o; o.group = SoundGroup::ENEMY; o.positional = true; o.priority = true; o.pos = e.pos; o.floor = 0.5f;
    for (const auto& ev : e.events) {
        switch (ev.kind) {
        case EyeEvent::RISE: { SfxMixer::Opts f = o; f.pitch = 0.5f; audio.playOpts("ab_pulse", f); break; }
        case EyeEvent::RISEN: { SfxMixer::Opts f = o; f.pitch = 0.45f; audio.playOpts("anvil", f); shake(0.6f, 0.02f); break; }
        case EyeEvent::SWEEP_TELL: { SfxMixer::Opts f = o; f.pitch = 0.6f; audio.playOpts("telegraph", f); break; }
        case EyeEvent::SWEEP: { SfxMixer::Opts f = o; f.pitch = 0.5f; audio.playOpts("ab_pulse", f); shake(0.3f, 0.015f); break; }
        case EyeEvent::LANCE_AIM: { SfxMixer::Opts f = o; f.pitch = 1.2f; audio.playOpts("telegraph", f); break; }
        case EyeEvent::LANCE: { SfxMixer::Opts f = o; f.pitch = 0.7f; f.floor = 0.7f; audio.playOpts("explosion", f); shake(0.25f, 0.03f); break; }
        case EyeEvent::CALL:
            for (int k = 0; k < (e.enraged() ? 4 : 3); ++k) {
                glm::vec3 lead = player.position + glm::vec3{player.velocity.x, 0.f, player.velocity.z} * 1.2f;
                glm::vec3 at = k == 0 ? lead : lead + glm::vec3{e.rnd() * 24.f - 12.f, 0.f, e.rnd() * 24.f - 12.f};
                at.x = std::clamp(at.x, deckLo.x + 3.f, deckHi.x - 3.f); at.z = std::clamp(at.z, deckLo.y + 3.f, deckHi.y - 3.f);
                meteors.launchCalled(at, 5.f, deckY);
            }
            { SfxMixer::Opts f = o; f.pitch = 0.8f; f.floor = 0.6f; audio.playOpts("caller_toll", f); }
            break;
        case EyeEvent::WEEP: {   // a tear of slag at you: punch it back
            glm::vec3 aimAt = player.camera.position - glm::vec3{0.f, 0.4f, 0.f} + glm::vec3{e.rnd() - 0.5f, 0.f, e.rnd() - 0.5f} * 2.f;
            glm::vec3 v = glm::normalize(aimAt - ev.at) * 32.f;
            if (Projectile* pr = projSystem.fire(ev.at, v, 15.f * tune().damage, false, {2.6f, 0.4f, 0.12f}, false, 0.f, 1.9f)) pr->parryDamage = 420.f;
            SfxMixer::Opts f = o; f.pitch = 0.75f; audio.playOpts("telegraph", f);
            break;
        }
        case EyeEvent::STUN: {
            SfxMixer::Opts f = o; f.pitch = 0.8f; audio.playOpts("stun", f);
            ui.toast("IT REELS", "ITS LIDS ARE OPEN - x2", {1.f, 0.6f, 0.3f}, 1.6f);
            fx.spawnBurst(e.pos, {2.6f, 0.4f, 0.15f}, 50, 10.f, 0.8f, 8.f);
            break;
        }
        case EyeEvent::FALTER:
            gleaner.scream();
            fx.spawnBurst(e.pos, {2.8f, 0.5f, 0.2f}, 90, 16.f, 1.2f, 6.f);
            shake(1.f, 0.05f); hitStopFrames = std::max(hitStopFrames, 6);
            pushBanner("EYE EXPOSED", "REACH THE ROOF'S EDGE", {1.f, 0.45f, 0.2f}, 3.f);
            break;
        case EyeEvent::RECOVER: { SfxMixer::Opts f = o; f.pitch = 0.55f; audio.playOpts("boost", f); break; }
        case EyeEvent::FALL:
            gleaner.scream();
            fx.spawnBurst(e.pos, {3.f, 0.5f, 0.2f}, 120, 18.f, 1.4f, 6.f);
            shake(1.5f, 0.06f); hitStopFrames = std::max(hitStopFrames, 10);
            styleSystem.addStyle(400.f, StyleSource::PARRY); gainXp(300);
            pushBanner("APPROACH THE EYE", "", {1.f, 0.3f, 0.15f}, 5.f);
            break;
        default: break;
        }
    }
    e.events.clear();
    shedFightPotion(6, e.health01(), e.pos, e.alive());   // (one at each of its falters)
    shedFightPotion(7, e.health01(), e.pos, e.alive());
    if (!playerDead && e.alive()) {
        // Its sweep: burns along its line (jump it; a chimney between it and you shelters you)
        if (e.state == EyeCore::SWEEP && eyeHitCd <= 0.f) {
            glm::vec3 s = e.sweepPoint();
            glm::vec2 d{player.position.x - s.x, player.position.z - s.z};
            if (glm::length(d) < EyeCore::SWEEP_HALF && player.position.y < deckY + 1.4f) {
                glm::vec3 to = chest - e.pos; float l = glm::length(to);
                if (eyeBeamReach(e.pos, to / l, l) >= l - 0.5f) {
                    eyeHitCd = 0.6f;
                    damagePlayer(26.f * tune().damage, s, 0.3f, 0.05f);
                    if (!g_godMode) player.velocity += glm::vec3{0.f, 7.f, 0.f};
                }
            }
        }
        // Its lance: a straight line, gold (no parry)
        if (e.state == EyeCore::LANCE && eyeHitCd <= 0.f) {
            glm::vec3 d = e.lanceDir();
            float reach = eyeBeamReach(e.pos, d, 260.f);
            float along = glm::clamp(glm::dot(chest - e.pos, d), 0.f, reach);
            if (glm::length(e.pos + d * along - chest) < EyeCore::LANCE_HALF) {
                eyeHitCd = 0.8f;
                damagePlayer(34.f * tune().damage, e.pos, 0.4f, 0.06f);
            }
        }
        // Its tears punched back into it
        for (auto& pr : projSystem.pool) {
            if (!pr.alive || !pr.parried) continue;
            if (glm::length(pr.position - e.pos) < EyeCore::R + 1.2f) {
                pr.alive = false;
                e.hurt(pr.parryDamage > 0.f ? pr.parryDamage : 300.f, 2);
                e.stun();
                fx.spawnBurst(pr.position, {2.6f, 0.4f, 0.15f}, 30, 8.f, 0.6f, 10.f);
                styleSystem.addStyle(80.f, StyleSource::PARRY);
                ui.feed("RETURNED", {1.f, 0.7f, 0.3f});
            }
        }
    }
    eyeHitCd = std::max(0.f, eyeHitCd - dt);
    // You cannot walk through it (fallen onto the roof, or faltered at its edge)
    if (e.state != EyeCore::TORN) {
        glm::vec3 d = player.position - e.pos; d.y = 0.f;
        float len = glm::length(d), r = EyeCore::R + 0.7f;
        if (len < r && player.position.y < e.pos.y + EyeCore::R && player.position.y > e.pos.y - EyeCore::R - 2.f)
            player.position += (len > 0.05f ? d / len : glm::vec3{0.f, 0.f, -1.f}) * (r - len);
    }
}

// The collapse: the white, then the far shot of it going down into the plain; the meteors stop; the moon
inline void GameplayState::updateSummitEnd(float dt) {
    const float t0 = summitEndT;
    if (storySkipKey && t0 > 2.f && t0 < summit::DARK0) summitEndT = summit::DARK0;
    summitEndT += dt;
    const float t = summitEndT;
    auto crossed = [&](float at) { return t0 < at && t >= at; };
    player.velocity = glm::vec3{0.f};
    director.hold = true;
    Gleaner& g = gleaner;
    if (t >= summit::FALL0) {   // it goes down: up on its knuckles no more, sinking into the plain, its head dropping
        float u = summit::sm(summit::FALL0, summit::FALL1, t);
        g.rise = 1.f - u;
        g.headHold = std::max(0.f, 1.f - u * 2.f);
        g.eyeOpen = 0.f;
    }
    if (crossed(summit::CUT)) meteors.stopped = true;   // the sky goes quiet
    if (crossed(summit::FALL0 + 0.2f)) {   // no roar left in it (its last was as you tore its eye out): a long low groan
        SfxMixer::Opts o; o.group = SoundGroup::UI; o.priority = true; o.unducked = true; o.pitch = 0.55f; o.volume = 1.f;
        audio.playOpts("gleaner_groan", o);
        audio.duck(14.f, 6.f);
    }
    if (crossed(summit::FALL0 + 2.5f)) { SfxMixer::Opts o; o.group = SoundGroup::UI; o.priority = true; o.unducked = true; o.pitch = 0.7f; audio.playOpts("gleaner_groan", o); }
    if (crossed(summit::SLAM)) {
        SfxMixer::Opts o; o.group = SoundGroup::UI; o.priority = true; o.unducked = true; o.pitch = 0.6f; audio.playOpts("gleaner_slam", o);
        o.pitch = 0.4f; audio.playOpts("explosion", o);
        for (int k = 0; k < 5; ++k) gleanerDust.push_back({g.pos + g.right() * ((k - 2) * 70.f) + g.fwd() * (40.f * (k % 2)), 0.f, 40.f * Gleaner::SCALE});
        shake(2.5f, 0.05f);
    }
    if (crossed(summit::MOON0)) { SfxMixer::Opts o; o.group = SoundGroup::UI; o.priority = true; o.pitch = 0.5f; audio.playOpts("gleaner_blink", o); }
    if (t >= summit::END_LEN) beginQuiet();   // then the quiet, and the portal (Gameplay_GleanerFinale.h)
}

// The far shot: from out on the plain, it filling the frame and going down
inline void GameplayState::summitShot(glm::vec3& eyeAt, glm::vec3& at) const {
    const float t = summitEndT;
    const glm::vec3 c = gleaner.pos;
    const float u = summit::sm(summit::CUT, summit::END_LEN, t);
    eyeAt = c + glm::vec3{-380.f + 60.f * u, 30.f + 10.f * u, 760.f - 40.f * u};
    at = c + glm::vec3{0.f, glm::mix(190.f, 60.f, summit::sm(summit::FALL0, summit::FALL1, t)), 0.f};
}
inline float GameplayState::summitMoonOpen() const {
    if (summitEndT < 0.f) return 0.f;
    return summit::sm(summit::MOON0, summit::MOON1, summitEndT);
}

// THE EYE in the world: its shell, its lids, its pupil, its cables, its beams and their tells
inline void GameplayState::gatherEye(std::vector<BoxInstance>& out) {
    using namespace rig; using glm::vec3;
    const EyeCore& e = eye;
    if (quietT >= 0.f || e.state == EyeCore::DORMANT || (e.state == EyeCore::TORN && summitEndT > summit::CUT)) return;
    gatherTear(out);
    const float ball = (float)meshlib::lumpBall();
    const float R = EyeCore::R, pulse = 0.8f + 0.2f * std::sin(gameClock * 3.f);
    const vec3 red{3.2f, 0.35f, 0.12f}, gold{2.6f, 1.9f, 0.5f}, iron{0.1f, 0.08f, 0.075f}, rust{0.24f, 0.11f, 0.06f};
    const vec3 f = e.facing;
    vec3 r = glm::cross(f, vec3{0.f, 1.f, 0.f}); r = glm::length(r) > 0.01f ? glm::normalize(r) : vec3{1.f, 0.f, 0.f};
    const vec3 u = glm::cross(r, f);
    auto basis = [&](vec3 at, vec3 x, vec3 y, vec3 z, vec3 size) {
        glm::mat4 m(1.f);
        m[0] = glm::vec4(x * size.x, 0.f); m[1] = glm::vec4(y * size.y, 0.f); m[2] = glm::vec4(z * size.z, 0.f); m[3] = glm::vec4(at, 1.f);
        return m;
    };
    const float dim = e.state == EyeCore::FALLEN ? 0.35f + 0.25f * std::sin(gameClock * 2.f)
                    : e.state == EyeCore::TORN ? (tearT >= 0.f ? glm::mix(1.4f, 0.04f, finale::sm(finale::PULL1 - 0.2f, finale::PULL1 + 0.5f, tearT)) : 0.04f) : 1.f;   // (its core out: dark)
    const vec3 glow = red * pulse * dim + vec3{2.f} * e.flash;
    // the core, and its iron back
    push(out, T(e.pos) * S(vec3{R * 2.f}), {0.5f, 0.06f, 0.03f}, glow * 0.55f); out.back().mesh = ball;
    push(out, T(e.pos - f * (R * 0.35f)) * S(vec3{R * 2.1f}), iron, glow * 0.04f); out.back().mesh = ball;
    // the iris and its slit pupil, on the face toward you
    push(out, basis(e.pos + f * (R * 0.82f), r, u, f, vec3{R * 1.3f, R * 1.3f, R * 0.5f}), {0.7f, 0.1f, 0.04f}, glow * (0.5f + 1.2f * e.open)); out.back().mesh = ball;
    vec3 pupilGlow = e.state == EyeCore::LANCE_AIM || e.state == EyeCore::LANCE ? gold * 1.2f : vec3{0.f};
    if (tearT >= 0.f) pupilGlow = vec3{3.2f, 1.3f, 0.5f} * 1.6f * (1.f - finale::sm(finale::PULL1 - 0.2f, finale::PULL1 + 0.4f, tearT));   // (torn open: white-hot inside)
    push(out, basis(e.pos + f * (R * 1.06f), r, u, f, vec3{0.5f + 0.3f * e.open, R * 1.3f * (0.3f + 0.7f * e.open), 0.25f}), {0.02f, 0.f, 0.f}, pupilGlow);
    // six lids round it: shut over the face, swinging back as it opens
    for (int k = 0; k < 6; ++k) {
        float a = k * 1.0472f + gameClock * 0.05f;
        vec3 radial = r * std::cos(a) + u * std::sin(a);
        float o = e.open;
        vec3 n = glm::normalize(f * (1.f - 0.85f * o) + radial * (0.35f + 0.9f * o));   // the plate's outward face
        vec3 at = e.pos + n * (R * 1.02f);
        vec3 x = glm::normalize(glm::cross(n, radial)), y = glm::cross(x, n);
        push(out, basis(at, x, y, n, vec3{R * 0.95f, R * 0.95f, 0.45f}), k % 2 ? rust : iron * 1.3f, glow * 0.06f);
        push(out, basis(at + n * 0.25f + y * (R * 0.42f), x, y, n, vec3{R * 0.8f, 0.12f, 0.12f}), {0.f, 0.f, 0.f}, glow * 0.5f);   // its lit edge
    }
    // its cables: down into its chest, swaying
    if (e.state != EyeCore::FALLEN && e.state != EyeCore::TORN) {
        const vec3 chest = gleaner.eye();
        for (int k = 0; k < 4; ++k) {
            float a = k * 1.5708f + 0.4f;
            vec3 a0 = e.pos - f * (R * 0.6f) + (r * std::cos(a) + u * std::sin(a)) * (R * 0.6f);
            vec3 b0 = chest + vec3{std::cos(a) * 18.f, 0.f, std::sin(a) * 18.f};
            vec3 mid = (a0 + b0) * 0.5f + vec3{std::sin(gameClock * 0.7f + k) * 6.f, -20.f, 0.f};
            for (int s = 0; s < 2; ++s) {
                vec3 p = s ? mid : a0, q = s ? b0 : mid, d = q - p;
                float len = glm::length(d);
                vec3 z = d / len, x = glm::normalize(glm::cross(z, std::abs(z.y) > 0.9f ? vec3{1.f, 0.f, 0.f} : vec3{0.f, 1.f, 0.f})), y = glm::cross(z, x);
                push(out, basis((p + q) * 0.5f, x, y, z, vec3{0.9f, 0.9f, len}), {0.05f, 0.035f, 0.03f}, red * 0.05f);
            }
        }
    }
    // the sweep: a red line drawn across the roof first, then the beam burning along it
    auto beam = [&](vec3 a, vec3 b, float w, vec3 col) {
        vec3 d = b - a; float len = glm::length(d); if (len < 0.1f) return;
        vec3 z = d / len, x = glm::normalize(glm::cross(z, std::abs(z.y) > 0.9f ? vec3{1.f, 0.f, 0.f} : vec3{0.f, 1.f, 0.f})), y = glm::cross(z, x);
        push(out, basis((a + b) * 0.5f, x, y, z, vec3{w, w, len}), {0.f, 0.f, 0.f}, col);
    };
    if (e.state == EyeCore::SWEEP_TELL) {
        float k = 0.5f + 0.5f * std::sin(gameClock * 20.f);
        beam(e.sweepA, e.sweepB, 0.25f, red * (0.6f + 0.6f * k));
        for (int i = 0; i < 12; ++i) {   // ticks across it, how wide it burns
            vec3 p = glm::mix(e.sweepA, e.sweepB, (i + 0.5f) / 12.f), d = glm::normalize(e.sweepB - e.sweepA), side = glm::cross(d, vec3{0.f, 1.f, 0.f});
            beam(p - side * EyeCore::SWEEP_HALF, p + side * EyeCore::SWEEP_HALF, 0.12f, red * 0.5f);
        }
    }
    if (e.state == EyeCore::SWEEP) {
        vec3 s = e.sweepPoint(), d = s - e.pos; float l = glm::length(d);
        float reach = eyeBeamReach(e.pos, d / l, l);
        beam(e.pos + f * R, e.pos + d / l * reach, 1.1f, red * 2.4f);
        beam(e.pos + f * R, e.pos + d / l * reach, 0.4f, vec3{3.f, 2.2f, 1.6f});
        beam(e.sweepA, s, 0.6f, red * 0.9f);   // the scorch it leaves
        push(out, T(s) * S(vec3{EyeCore::SWEEP_HALF * 2.f, 0.4f, EyeCore::SWEEP_HALF * 2.f}), vec3{0.f}, red * 2.f); out.back().mesh = ball;
    }
    if (e.state == EyeCore::LANCE_AIM) {   // it narrows on you: a thin gold thread, shaking until it locks
        vec3 d = e.lanceDir(); float reach = eyeBeamReach(e.pos, d, 260.f);
        float k = e.aimLocked() ? 1.f : 0.4f + 0.3f * std::sin(gameClock * 30.f);
        beam(e.pos + f * R, e.pos + d * reach, e.aimLocked() ? 0.16f : 0.07f, gold * k);
    }
    if (e.state == EyeCore::LANCE) {
        vec3 d = e.lanceDir(); float reach = eyeBeamReach(e.pos, d, 260.f);
        beam(e.pos + f * R, e.pos + d * reach, 1.6f, gold * 2.f);
        beam(e.pos + f * R, e.pos + d * reach, 0.6f, vec3{3.5f, 3.2f, 2.5f});
        push(out, T(e.pos + d * reach) * S(vec3{4.f}), vec3{0.f}, gold * 2.f); out.back().mesh = ball;
    }
}

// Over the collapse: the white of the tear, the dark, the skip prompt
inline void GameplayState::renderSummitEnd() {
    ui.begin2D(); auto& U = ui.ui;
    const float t = summitEndT;
    const int bar = (int)(70.f * summit::sm(summit::CUT, summit::CUT + 1.f, t));
    if (bar > 0) { U.rect(0, 0, SCREEN_W, bar, {0.02f, 0.f, 0.f, 1}); U.rect(0, SCREEN_H - bar, SCREEN_W, bar, {0.02f, 0.f, 0.f, 1}); }
    if (t > 2.f && t < summit::DARK0) {
        std::string skip = "HOLD " + binds().label(Act::JUMP) + " TO SKIP";
        U.text(skip.c_str(), SCREEN_W / 2, SCREEN_H - 24, 1, {0.7f, 0.45f, 0.4f, 0.7f}, true);
    }
    float white = t < summit::CUT ? summit::sm(0.f, summit::CUT * 0.9f, t) : 1.f - summit::sm(summit::CUT, summit::CUT + 1.2f, t);
    if (white > 0.001f) U.rect(0, 0, SCREEN_W, SCREEN_H, {1.f, 0.95f, 0.9f, white});
    float dark = summit::sm(summit::DARK0, summit::END_LEN, t);
    if (dark > 0.001f) U.rect(0, 0, SCREEN_W, SCREEN_H, {0.f, 0.f, 0.f, dark});
    ui.end2D();
}
