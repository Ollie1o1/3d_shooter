#pragma once
// =============================================================================
// Gameplay_ShepherdHead.h — THE SHEPHERD, the fight itself (the user, after the
// first full pass: "i want an actual boss that you have to fight in the end";
// they chose the giant face). In THE FALL (arena 10) its ridge rises up as a
// head: no eye, no mouth, a face made only of its lamps, which move together and
// follow you (design 13.8), and two of its tines as arms.
//
//   SLAM    an arm comes down where the red ring is (it follows you, then
//           locks): a ring runs out along the hull from it (jump it). Its hand
//           lies there a few seconds: shoot the hand (more damage).
//   BEAM    its lamps sweep a line of light across the crown: get behind a fin
//           (the beam comes from its face, north) or dash through it.
//   AIM     the aimed sweep from its face, the red cone and the corner count:
//           one hit. Out of the cone, or behind a fin by its lit post.
//   FLEECE  its capsules thrown down burning (gold rings).
//   PULL    (last third) it draws you in toward it, then beams: HOLDFAST to
//           something heavy and the line holds.
//
// After every attack its face OPENS (its lamps white) for a few seconds: that
// is when it takes full damage. Shut, it barely marks. Three thirds: at two
// thirds and one third it rears up and adds attacks. At nothing its face breaks,
// the core opens: THE LAST COUNT, HOLDFAST into the core (Gameplay_ShepherdFall.h).
// =============================================================================

namespace shephead {
constexpr float HP = 2400.f, SHUT_MULT = 0.15f, HAND_MULT = 1.5f;
constexpr float RISE = 5.f, OPEN = 3.6f, REAR = 2.6f;
constexpr float SLAM_TELL = 1.5f, SLAM_LOCK = 0.9f, SLAM_R = 4.5f, SLAM_DAMAGE = 35.f, HAND_DOWN = 3.2f;
constexpr float BEAM_TELL = 1.3f, BEAM_SPEED = 45.f, BEAM_HALF = 1.6f, BEAM_DAMAGE = 28.f;
constexpr float AIM_WARN = 3.5f, AIM_FLASH = 1.f;
constexpr float PULL_LEN = 3.f, PULL_SPEED = 9.f;
enum Atk { NONE, SLAM, SLAM2, BEAM, AIM, FLEECE, PULL };
inline glm::vec3 headC(float rise) { return {-600.f, 100.f + 77.f * rise, -4060.f}; }   // (risen: its chin just over the crown, 109 m wide)   // its head's middle (risen: 176 m)
constexpr float HW = 46.f, HH = 33.f, HD = 15.f;   // half sizes
inline glm::vec3 shoulder(int k, float rise) { return headC(rise) + glm::vec3{k ? 50.f : -50.f, -12.f, 6.f}; }
inline glm::vec3 handRest(int k, float rise) { return glm::vec3{k ? -548.f : -652.f, shep::FALL_Y + 14.f * rise, -4036.f}; }
}

inline float GameplayState::shepHeadThird() const { return shepHeadHp / shephead::HP; }

inline void GameplayState::shepHeadReset() {
    using namespace shephead;
    if (!shepHeadArmed) { shepHeadArmed = true; shepHeadHp = HP; shepHeadSeen = false; }
    else if (shepHeadHp > 0.f) shepHeadHp = shepHeadHp > HP * 0.66f ? HP : shepHeadHp > HP * 0.33f ? HP * 0.66f : HP * 0.33f;   // (a retry: from the start of the third you died in)
    shepAtk = NONE; shepAtkT = 0.f; shepIdleT = 2.f; shepOpenT = 0.f; shepRearT = -1.f; shepCycle = 0;
    shepArmDown[0] = shepArmDown[1] = 0.f; shepBeamOn = false; shepHeadRise = shepHeadSeen ? 1.f : 0.f; shepRiseT = shepHeadSeen ? -1.f : 0.f;
    shepHeadPhase = shepHeadHp > HP * 0.66f ? 0 : shepHeadHp > HP * 0.33f ? 1 : 2;
    shepGazeX = -600.f;
}

// Shelter from its face (a beam, north): something heavy between you and it, close
inline bool GameplayState::shepFinCovers(glm::vec3 feet) const {
    const glm::vec3 o = feet + glm::vec3{0.f, 1.f, 0.f};
    for (const SweepShelter& s : sweepShelters) {
        if (!s.alive) continue;
        const float t = rayBoxHit(o, glm::vec3{0.f, 0.f, -1.f}, s.box);
        if (t > 0.f && t < 16.f) return true;
    }
    return false;
}

inline void GameplayState::shepHeadHurt(float dmg) {
    using namespace shephead;
    if (shepHeadHp <= 0.f) return;
    const float before = shepHeadHp;
    shepHeadHp = std::max(0.f, shepHeadHp - dmg); shepherdDamageT = 0.3f;
    for (int t = 0; t < 2; ++t) {   // a third gone: it rears up
        const float line = HP * (t ? 0.33f : 0.66f);
        if (before > line && shepHeadHp <= line && shepHeadHp > 0.f) {
            shepHeadPhase = t + 1; shepRearT = 0.f;
            if (shepAtk == AIM) sweep.reset();
            shepAtk = NONE; shepBeamOn = false; shepOpenT = 0.f;
        }
    }
    if (shepHeadHp <= 0.f) {   // its face breaks: the core opens (THE LAST COUNT)
        sweep.reset(); shepAtk = NONE; shepBeamOn = false;
        pushBanner("ITS FACE BREAKS", "ITS CORE IS OPEN", POST_LIT * 0.7f, 3.f);
        shake(2.f, 0.08f);
        const glm::vec3 c = headC(shepHeadRise);
        for (int k = 0; k < 6; ++k) fx.spawnBurst(c + glm::vec3{cosmetic::range(-HW, HW), cosmetic::range(-HH, HH), HD}, POST_LIT, 40, 16.f, 1.2f, 3.f);
        SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.pitch = 0.35f; audio.playOpts("slam", o);
        for (auto& e : enemies) if (e.targetable() && e.takeDamage(1e6f)) onEnemyKilled(e, StyleSource::ENVIRONMENT);
        shepLastT = 0.f;
    }
}

// Its turn: rise, rear, pick an attack, run it, open its face after
inline void GameplayState::shepHead(float dt) {
    using namespace shephead;
    const glm::vec3 p = player.position;
    shepGazeX += (glm::clamp(p.x, shep::FALL_X0, shep::FALL_X1) - shepGazeX) * std::min(1.f, dt * 2.f);   // its lamps follow you
    for (float& a : shepArmDown) a = std::max(0.f, a - dt);
    if (shepHeadHp <= 0.f) return;
    // It rises over its ridge
    if (shepRiseT >= 0.f) {
        const float before = shepRiseT;
        shepRiseT += dt;
        shepHeadRise = glm::clamp(shepRiseT / RISE, 0.f, 1.f); shepHeadRise = shepHeadRise * shepHeadRise * (3.f - 2.f * shepHeadRise);
        shake(0.12f, 0.02f + 0.03f * std::sin(3.14159f * shepRiseT / RISE));
        if (std::floor(before / 1.2f) != std::floor(shepRiseT / 1.2f)) {
            SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.pitch = 0.3f; o.volume = 0.9f; audio.playOpts("slam", o);
        }
        if (before < 1.f && shepRiseT >= 1.f) pushBanner("THE SHEPHERD", "IT FACES YOU", SWEEP_ICE * 0.8f, 4.f);
        if (shepRiseT >= RISE) {
            shepRiseT = -1.f; shepHeadRise = 1.f; shepHeadSeen = true; shepIdleT = 1.5f;
            ui.toast("ITS FACE", "AFTER EVERY ATTACK ITS FACE OPENS, WHITE: THAT'S WHEN TO HIT IT", POST_LIT * 0.4f, 6.f);
        }
        return;
    }
    // It rears up: a third gone
    if (shepRearT >= 0.f) {
        const float before = shepRearT;
        shepRearT += dt;
        if (before == 0.f) {
            static const char* t1[2] = {"YOU ARE NOT COUNTED.", "WITHOUT ME,"};
            static const char* t2[2] = {"IT REARS UP", "THEY DRIFT."};
            pushBanner(t1[shepHeadPhase - 1], t2[shepHeadPhase - 1], glm::vec3{0.9f, 0.9f, 0.82f}, 3.5f);
            shake(1.5f, 0.06f);
            SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.pitch = 0.32f; o.volume = 1.f;
            audio.playOpts("slam", o); o.delay = 0.6f; audio.playOpts("slam", o);
        }
        if (shepRearT >= REAR) { shepRearT = -1.f; shepIdleT = 0.8f; }
        return;
    }
    if (playerDead) return;
    shepOpenT = std::max(0.f, shepOpenT - dt);
    // Pick the next attack
    if (shepAtk == NONE) {
        if (shepOpenT > 0.f || (shepIdleT -= dt) > 0.f) return;
        static const Atk p0[] = {SLAM, AIM, SLAM, FLEECE};
        static const Atk p1[] = {SLAM, BEAM, AIM, SLAM, BEAM, FLEECE};
        static const Atk p2[] = {PULL, SLAM2, AIM, SLAM, BEAM, SLAM2};
        const Atk* list = shepHeadPhase == 0 ? p0 : shepHeadPhase == 1 ? p1 : p2;
        const int n = shepHeadPhase == 0 ? 4 : 6;
        shepAtk = list[shepCycle % n]; ++shepCycle; shepAtkT = 0.f;
        shepSlamArm = (shepSlamArm + 1) % 2;
        if (shepAtk == SLAM || shepAtk == SLAM2) { shepSlamAt[0] = shepSlamAt[1] = p; audio.play("telegraph", 128, SoundGroup::WORLD, true); }
        if (shepAtk == BEAM) {
            shepBeamDir = p.x < -600.f ? 1.f : -1.f;   // (it starts on the side away from you and sweeps across toward you)
            shepBeamX = shepBeamDir > 0.f ? shep::FALL_X0 - 2.f : shep::FALL_X1 + 2.f;
            shepBeamOn = false; shepBeamHit = false;
            audio.play("telegraph", 128, SoundGroup::WORLD, true);
        }
        if (shepAtk == AIM) {
            const glm::vec3 o = headC(1.f) + glm::vec3{0.f, -6.f, HD};
            SweepSpec s = sweeptier::aimed(o, p, AIM_WARN);
            s.flashLead = AIM_FLASH; s.range = 30.f;
            const float dist = glm::length(glm::vec2{p.x - o.x, p.z - o.z});
            s.coneHalf = glm::clamp(std::atan(10.f / std::max(dist, 1.f)), 0.05f, 0.6f);
            sweep.start(s); ++sweepCount; shepFallPassed = false;
        }
        if (shepAtk == PULL) { ui.feed("IT DRAWS YOU IN: HOLD ON TO SOMETHING", HOLDFAST_STEEL); audio.play("telegraph", 128, SoundGroup::WORLD, true); }
        return;
    }
    shepAtkT += dt;
    auto finish = [&]() {   // every attack ends with its face open
        shepAtk = NONE; shepOpenT = OPEN; shepIdleT = shepHeadPhase == 0 ? 1.2f : shepHeadPhase == 1 ? 0.9f : 0.6f;
        audio.play("drive_ready", 90, SoundGroup::UI, true);
    };
    if (shepAtk == SLAM || shepAtk == SLAM2) {
        const int arms = shepAtk == SLAM2 ? 2 : 1;
        if (shepAtkT < SLAM_LOCK)   // the ring follows you, then locks
            for (int a = 0; a < arms; ++a) shepSlamAt[a] = p + glm::vec3{arms == 2 ? (a ? 5.f : -5.f) : 0.f, 0.f, 0.f};
        for (int a = 0; a < arms; ++a) { shepSlamAt[a].y = shep::FALL_Y; shepSlamAt[a].x = glm::clamp(shepSlamAt[a].x, shep::FALL_X0 + 3.f, shep::FALL_X1 - 3.f); shepSlamAt[a].z = glm::clamp(shepSlamAt[a].z, shep::FALL_ZN + 3.f, shep::FALL_ZS - 3.f); }
        if (shepAtkT >= SLAM_TELL && shepAtkT - dt < SLAM_TELL) {
            for (int a = 0; a < arms; ++a) {
                const glm::vec3 at = shepSlamAt[a];
                const int arm = arms == 2 ? a : shepSlamArm;
                shepArmDown[arm] = HAND_DOWN; shepHandAt[arm] = at;
                wakeRings.push_back({at, 0.6f, false});
                fx.spawnBurst(at + glm::vec3{0.f, 0.5f, 0.f}, glm::vec3{2.f, 1.2f, 0.6f}, 50, 14.f, 1.f, 3.f);
                fx.spawnShockwave(at, 10.f, glm::vec3{2.4f, 1.3f, 0.35f});
                if (!playerDead && glm::length(glm::vec2{p.x - at.x, p.z - at.z}) < SLAM_R && invincFrames <= 0.f) {
                    damagePlayer(SLAM_DAMAGE, at, 0.5f, 0.07f, 0.f, {EnemyType::COUNT, DamageKind::SHOCKWAVE});
                    const glm::vec3 out = glm::normalize(glm::vec3{p.x - at.x, 0.f, p.z - at.z} + glm::vec3{1e-4f});
                    player.velocity += out * 12.f + glm::vec3{0.f, 6.f, 0.f}; player.launched();
                }
                for (auto& e : enemies) if (e.targetable() && glm::length(glm::vec2{e.position.x - at.x, e.position.z - at.z}) < SLAM_R && e.takeDamage(1e6f)) onEnemyKilled(e, StyleSource::ENVIRONMENT);
            }
            shake(1.1f, 0.06f);
            SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.pitch = 0.5f; o.volume = 1.f; audio.playOpts("slam", o);
            if (!shepHandHint) { shepHandHint = true; ui.toast("ITS HAND", "IT LIES THERE A MOMENT: SHOOT IT", glm::vec3{2.4f, 1.3f, 0.35f} * 0.6f, 3.f); }
        }
        if (shepAtkT >= SLAM_TELL + 0.4f) finish();
    } else if (shepAtk == BEAM || shepAtk == PULL) {
        if (shepAtk == PULL) {   // it draws everything in
            if (!holdfast.held() && !playerDead) {
                const glm::vec3 to = glm::normalize(glm::vec3{headC(1.f).x - p.x, 0.f, headC(1.f).z - p.z} + glm::vec3{1e-4f});
                player.velocity.x += to.x * PULL_SPEED * dt * 4.f; player.velocity.z += to.z * PULL_SPEED * dt * 4.f;
            } else if (std::fmod(gameClock, 0.5f) < dt) ui.feed("THE LINE HOLDS", HOLDFAST_STEEL);
            if (std::fmod(gameClock, 0.05f) < dt) fx.spawnSpray(p + glm::vec3{cosmetic::range(-8.f, 8.f), 0.3f, 6.f}, glm::vec3{0.f, 0.1f, -1.f}, SWEEP_ICE * 0.6f, 2, 14.f, 0.8f, 0.4f);
            if (shepAtkT >= PULL_LEN) {   // then the beam
                shepAtk = BEAM; shepAtkT = 0.f;
                shepBeamDir = p.x < -600.f ? 1.f : -1.f; shepBeamX = shepBeamDir > 0.f ? shep::FALL_X0 - 2.f : shep::FALL_X1 + 2.f;
                shepBeamOn = false; shepBeamHit = false;
            }
            return;
        }
        if (shepAtkT >= BEAM_TELL) {
            if (!shepBeamOn) { shepBeamOn = true; SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.pitch = 0.6f; audio.playOpts("barrier", o); }
            const float before = shepBeamX;
            shepBeamX += shepBeamDir * BEAM_SPEED * dt;
            const float lo = std::min(before, shepBeamX) - BEAM_HALF, hi = std::max(before, shepBeamX) + BEAM_HALF;
            if (!shepBeamHit && !playerDead && p.x > lo && p.x < hi && p.y < shep::FALL_Y + 4.5f && invincFrames <= 0.f && !shepFinCovers(p)) {
                shepBeamHit = true;
                damagePlayer(BEAM_DAMAGE, glm::vec3{p.x, p.y + 1.f, p.z - 5.f}, 0.4f, 0.06f, 0.f, {EnemyType::COUNT, DamageKind::UNKNOWN});
                player.velocity += glm::vec3{shepBeamDir * 10.f, 4.f, 4.f}; player.launched();
                ui.feed("BURNED", glm::vec3{2.4f, 1.3f, 0.35f});
            }
            for (auto& e : enemies) if (e.targetable() && e.position.x > lo && e.position.x < hi && !shepFinCovers(e.position) && e.takeDamage(1e6f)) onEnemyKilled(e, StyleSource::ENVIRONMENT);
            if (std::fmod(gameClock, 0.03f) < dt) fx.spawnBurst({shepBeamX, shep::FALL_Y + 0.4f, cosmetic::range(shep::FALL_ZN, shep::FALL_ZS)}, glm::vec3{2.4f, 1.5f, 0.6f}, 3, 6.f, 0.5f, 1.f);
            if (shepBeamX < shep::FALL_X0 - 6.f || shepBeamX > shep::FALL_X1 + 6.f) { shepBeamOn = false; finish(); }
        }
    } else if (shepAtk == AIM) {
        if (sweep.active() && sweep.passed > 0 && !shepFallPassed) {   // it fired: strays climb the hull behind it
            shepFallPassed = true;
            int strays = 0; for (const auto& e : enemies) strays += e.alive && e.type == EnemyType::STRAY;
            for (int sd = -1; sd <= 1 && strays < 4; sd += 2, ++strays)
                spawnEnemy(EnemyType::STRAY, {sd < 0 ? shep::FALL_X0 + 2.f : shep::FALL_X1 - 2.f, shep::FALL_Y, glm::clamp(p.z + cosmetic::range(-12.f, 12.f), shep::FALL_ZN + 6.f, shep::FALL_ZS - 4.f)});
        }
        if (!sweep.active() && shepAtkT > AIM_FLASH + 0.5f) finish();
    } else if (shepAtk == FLEECE) {
        if (std::floor((shepAtkT - dt) / 0.5f) != std::floor(shepAtkT / 0.5f) && shepAtkT < 2.1f) {
            glm::vec3 to = p + glm::vec3{cosmetic::range(-8.f, 8.f), 0.f, cosmetic::range(-8.f, 8.f)};
            to.x = glm::clamp(to.x, shep::FALL_X0 + 4.f, shep::FALL_X1 - 4.f); to.z = glm::clamp(to.z, shep::FALL_ZN + 6.f, shep::FALL_ZS - 4.f); to.y = shep::FALL_Y;
            shepThrowCap(to, glm::vec3{to.x, shep::FALL_Y + 30.f, shep::FALL_ZN - 6.f});
        }
        if (shepAtkT >= 3.2f) finish();
    }
}

// Rounds into it: its face (full damage only while open), a hand lying on the hull
inline bool GameplayState::shootHead(glm::vec3 from, glm::vec3 dir, float& distance, float dmg) {
    using namespace shephead;
    if (level.arenaAt(player.position) != 10 || shepHeadHp <= 0.f || shepHeadRise < 0.99f) return false;
    for (int k = 0; k < 2; ++k) {
        if (shepArmDown[k] <= 0.f) continue;
        const glm::vec3 h = shepHandAt[k] + glm::vec3{0.f, 2.5f, 0.f};
        const float t = rayBoxHit(from, dir, AABB{h - glm::vec3{3.f, 2.5f, 3.f}, h + glm::vec3{3.f, 2.5f, 3.f}});
        if (t <= 0.f || t >= distance) continue;
        distance = t;
        fx.spawnHitSparks(from + dir * t, glm::vec3{2.4f, 1.3f, 0.35f});
        shepHeadHurt(dmg * HAND_MULT);
        return true;
    }
    const glm::vec3 c = headC(shepHeadRise);
    const float t = rayBoxHit(from, dir, AABB{c - glm::vec3{HW, HH, HD}, c + glm::vec3{HW, HH, HD}});
    if (t <= 0.f || t >= distance) return false;
    distance = t;
    if (shepOpenT > 0.f) { fx.spawnHitSparks(from + dir * t, POST_LIT); shepHeadHurt(dmg); }
    else {
        fx.spawnHitSparks(from + dir * t, glm::vec3{0.7f});
        shepHeadHurt(dmg * SHUT_MULT);
        if (shepVentMsgT <= 0.f) { shepVentMsgT = 2.5f; ui.feed("SHUT: HIT IT WHEN ITS FACE OPENS", glm::vec3{0.75f}); }
    }
    return true;
}

// The head: its skull, its brow, its face of lamps (following you), its two tine-arms
inline void GameplayState::gatherHead(std::vector<BoxInstance>& out) {
    using namespace rig;
    using namespace shephead;
    if (level.arenaAt(player.position) != 10 && shepFinT < 0.f) return;
    const float rise = shepHeadRise;
    if (rise <= 0.001f && shepRiseT < 0.f) return;
    const glm::vec3 c = headC(rise) + glm::vec3{0.f, shepRearT >= 0.f ? 8.f * std::sin(3.14159f * shepRearT / REAR) : 0.f, 0.f};
    const glm::vec3 hull{0.2f, 0.19f, 0.18f}, amber{2.4f, 1.3f, 0.35f};
    const bool dead = shepHeadHp <= 0.f;
    const float lean = glm::clamp((shepGazeX + 600.f) / 50.f, -1.f, 1.f) * 0.12f;
    const glm::mat4 H = T(c) * RY(-lean);
    push(out, H * S(glm::vec3{2.f * HW, 2.f * HH, 2.f * HD}), hull);                                   // its skull
    push(out, H * T(glm::vec3{0.f, HH + 3.f, -2.f}) * S(glm::vec3{2.f * HW - 6.f, 6.f, 2.f * HD - 4.f}), hull * 0.8f);   // its crest
    push(out, H * T(glm::vec3{0.f, HH * 0.45f, HD + 1.5f}) * S(glm::vec3{2.f * HW + 2.f, 2.2f, 3.f}), hull * 1.3f);       // its brow
    // its face: lamps in a grid, moving together. Where you are, they burn brightest
    const float open = shepOpenT > 0.f ? 1.f : 0.f;
    for (int row = 0; row < 6; ++row)
        for (int col = 0; col < 9; ++col) {
            const glm::vec3 at{(col - 4) * 9.5f, HH * 0.3f - row * 8.6f, HD + 0.5f};
            const float gx = (shepGazeX + 600.f) / 50.f * 4.f;
            const float focus = std::exp(-0.35f * (col - 4 - gx) * (col - 4 - gx));
            glm::vec3 g = amber * (0.25f + 0.9f * focus);
            if (open > 0.f) g = POST_LIT * (1.6f + 0.6f * std::sin(gameClock * 10.f + col));
            if ((shepAtk == BEAM && !shepBeamOn) || shepAtk == PULL) g = SWEEP_RED * (1.f + 2.f * focus);
            if (shepAtk == AIM && sweep.active()) g = SWEEP_RED * (0.6f + 1.5f * std::exp(-0.4f * ((col - 4) * (col - 4) + (row - 3) * (row - 3))));
            if (shepRearT >= 0.f) g = POST_LIT * 2.5f;
            if (dead) g = (ring::hash((float)(row * 9 + col), std::floor(gameClock * 8.f)) < 0.15f) ? amber * 0.8f : glm::vec3{0.01f};
            push(out, H * T(at) * S(glm::vec3{6.f, 4.2f, 0.6f}), glm::vec3{0.05f}, g);
        }
    if (dead) push(out, H * T(glm::vec3{0.f, -4.f, HD + 0.8f}) * S(glm::vec3{8.f, 30.f, 0.4f}), glm::vec3{0.f}, POST_LIT * 2.f);   // the crack
    // its arms: shoulder, elbow, hand (a crook's hook of a hand)
    for (int k = 0; k < 2; ++k) {
        const glm::vec3 sh = shoulder(k, rise) + (c - headC(rise));
        glm::vec3 hand = handRest(k, rise) + glm::vec3{0.f, 3.f * std::sin(gameClock * 0.7f + k), 0.f};
        const bool slamming = (shepAtk == SLAM && shepSlamArm == k) || shepAtk == SLAM2;
        const int si = shepAtk == SLAM2 ? k : 0;
        if (shepArmDown[k] > 0.f) hand = shepHandAt[k];
        else if (slamming) {
            const float u = glm::clamp(shepAtkT / SLAM_TELL, 0.f, 1.f);
            hand = glm::mix(handRest(k, rise), shepSlamAt[si] + glm::vec3{0.f, 34.f, 0.f}, std::min(1.f, u * 1.6f));
        }
        const glm::vec3 elbow = (sh + hand) * 0.5f + glm::vec3{k ? 14.f : -14.f, 16.f, 6.f};
        auto bone = [&](glm::vec3 a, glm::vec3 b, float w) {
            const glm::vec3 d = b - a; const float len = glm::length(d);
            if (len < 0.01f) return;
            const glm::vec3 f = d / len;
            const float yaw = std::atan2(f.x, f.z), pitch = -std::asin(glm::clamp(f.y, -1.f, 1.f));
            push(out, T((a + b) * 0.5f) * RY(yaw) * RX(pitch) * S(glm::vec3{w, w, len}), hull * 1.1f);
        };
        bone(sh, elbow, 6.f); bone(elbow, hand + glm::vec3{0.f, 3.f, 0.f}, 4.5f);
        push(out, T(hand + glm::vec3{0.f, 2.5f, 0.f}) * S(glm::vec3{6.f, 5.f, 6.f}), hull * 1.2f, shepArmDown[k] > 0.f ? amber * (1.2f + 0.4f * std::sin(gameClock * 9.f)) : amber * 0.15f);   // its hand (lit while it lies there: shoot it)
    }
    // the slam's ring on the hull: red, closing as it comes
    if (shepAtk == SLAM || shepAtk == SLAM2)
        for (int a = 0; a < (shepAtk == SLAM2 ? 2 : 1); ++a) {
            const float u = glm::clamp(shepAtkT / SLAM_TELL, 0.f, 1.f), r = SLAM_R * (1.6f - 0.6f * u);
            const glm::vec3 col = shepAtkT < SLAM_LOCK ? SWEEP_RED * 0.8f : SWEEP_RED * 2.f;
            for (int i = 0; i < 24; ++i) {
                const float ang = 6.2831853f * i / 24.f;
                push(out, T(shepSlamAt[a] + glm::vec3{std::cos(ang) * r, 0.07f, std::sin(ang) * r}) * RY(-ang) * S(glm::vec3{0.3f, 0.05f, 6.2831853f * r / 24.f}), col * 0.2f, col);
            }
        }
    // the beam: a red line where it will start, then a wall of light crossing the crown
    if (shepAtk == BEAM) {
        const float zc = (shep::FALL_ZN + shep::FALL_ZS) * 0.5f, zl = shep::FALL_ZS - shep::FALL_ZN;
        if (!shepBeamOn) push(out, T(glm::vec3{shepBeamX + shepBeamDir * 3.f, shep::FALL_Y + 0.06f, zc}) * S(glm::vec3{0.6f, 0.05f, zl}), SWEEP_RED * 0.2f, SWEEP_RED * 2.f);
        else push(out, T(glm::vec3{shepBeamX, shep::FALL_Y + 2.2f, zc}) * S(glm::vec3{2.f * BEAM_HALF, 4.4f, zl}), glm::vec3{0.f}, glm::vec3{2.4f, 1.6f, 0.7f} * 2.2f);
    }
}

inline void GameplayState::headMarkers(const glm::mat4& view, const glm::mat4& proj) {
    using namespace shephead;
    if (shepHeadHp <= 0.f || shepHeadRise < 0.99f) return;
    char buf[48]; float sx, sy;
    if (shepOpenT > 0.f) {
        const glm::vec3 at = headC(1.f) + glm::vec3{0.f, HH + 6.f, HD};
        const bool on = projectToScreen(at, view, proj, sx, sy);
        snprintf(buf, sizeof(buf), "ITS FACE IS OPEN  %dS", (int)std::ceil(shepOpenT));
        ui.renderMarker(sx, sy, on, glm::vec3{1.f}, buf);
    }
    for (int k = 0; k < 2; ++k) {
        if (shepArmDown[k] <= 0.f) continue;
        const glm::vec3 at = shepHandAt[k] + glm::vec3{0.f, 6.f, 0.f};
        const bool on = projectToScreen(at, view, proj, sx, sy);
        ui.renderMarker(sx, sy, on, glm::vec3{1.f, 0.6f, 0.15f}, "ITS HAND: SHOOT IT");
    }
}

inline const char* GameplayState::headObjective() {
    using namespace shephead;
    static char line[110];
    const char* step = "";
    if (shepRiseT >= 0.f) step = "IT RISES";
    else if (shepRearT >= 0.f) step = "IT REARS UP";
    else if (shepOpenT > 0.f) step = "ITS FACE IS OPEN: HIT IT NOW";
    else if (shepAtk == SLAM || shepAtk == SLAM2) step = "OUT OF THE RED RING. JUMP ITS SHOCKWAVE";
    else if (shepAtk == BEAM) step = "ITS BEAM: GET BEHIND A FIN, OR DASH THROUGH";
    else if (shepAtk == PULL) step = "IT DRAWS YOU IN: HOLDFAST TO SOMETHING HEAVY";
    else if (shepAtk == AIM) step = "OUT OF THE RED CONE, OR BEHIND A FIN";
    else if (shepAtk == FLEECE) step = "ITS FLEECE IS COMING DOWN: GOLD RINGS";
    else step = "WATCH IT. HIT ITS FACE WHEN IT OPENS";
    snprintf(line, sizeof(line), "%d/3  /  %s", shepHeadPhase + 1, step);
    return line;
}
