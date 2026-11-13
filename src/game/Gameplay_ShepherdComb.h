#pragma once
// =============================================================================
// Gameplay_ShepherdComb.h — THE SHEPHERD, phase 1: THE COMB, redone after the
// user's first play (2026-11-13): "the bosses are lackluster and there is no
// unique vibe or creativity... just side by side". Now one tine, walked from its
// tip toward the Shepherd, in three zones, each its own place and set-piece, and
// the Shepherd itself in every one of them (LevelShepherd.h builds them).
//
//   THE PLOUGH     the tip tears through a field of capsules and throws them up
//                  onto the deck (gold ring where one lands: cover after). The
//                  Shepherd aims its sweep down the deck (the red cone). THE CROOK
//                  hooks from the top of a mast: parry its white hook and it is
//                  YANKED DOWN to the deck, where it fights with its pole (gold:
//                  jump or dash it) and, below half, cuts your HOLDFAST line.
//   THE STAMPEDE   the tine breaks up behind you, plate after plate: run. THE
//                  DROVER rolls its herd down the deck at you (gold lanes) and its
//                  herd circles it as a shield: shoot the four blocks apart and it
//                  is open. The Shepherd's other teeth grind across the deck (a red
//                  band: get out of it). It makes its stand on the KNUCKLE.
//   THE BELFRY     the root, a tower of bells. THE BELLWETHER stands under one and
//                  rings it (rings along its floor: jump them); shoot the chain
//                  above it and the bell comes down on it. Every so often the
//                  Shepherd tolls its great bell: rings on every floor at once.
//
// Each zone's sheepdog dead: that part of the tine breaks (a fifth of a fifth of
// the bar each... one third of the Comb). Track the zone with shepTines.
// =============================================================================

namespace shepcomb {
constexpr float CAP_FLIGHT = 1.25f, CAP_GAP = 3.2f, CAP_GAP_RAGE = 2.f, CAP_LIFE = 22.f, CAP_HIT = 2.8f, CAP_DAMAGE = 18.f;
constexpr float SWING_TELL = 0.85f, SWING_REACH = 7.5f, SWING_DAMAGE = 16.f, SWING_CD = 4.5f;
constexpr float PERCH_MAX = 35.f;        // the Crook comes down by itself after this long up its mast
constexpr float COLLAPSE_SPEED = 5.5f, COLLAPSE_CATCH = 9.f, SLAB_SHAKE = 9.f, SLAB_FALL = 2.f;
constexpr float BOULDER_R = 1.7f, BOULDER_SPEED = 15.f, BOULDER_DAMAGE = 16.f, BOULDER_GAP = 3.6f;
constexpr float SHIELD_HP = 70.f, SHIELD_R = 4.2f, SHIELD_OPEN = 7.f;
constexpr float GRIND_WARN = 1.8f, GRIND_SPEED = 70.f, GRIND_HZ = 4.5f, GRIND_DAMAGE = 40.f;
constexpr float HOP_EVERY = 9.f, HOP_LEN = 0.9f, BELL_FALL = 0.55f, CRUSH = 0.28f, BELL_HIT = 3.2f;
constexpr float TOLL_EVERY = 15.f, TOLL_WARN = 2.f;
}

inline void GameplayState::shepCombReset() {
    using namespace shepcomb;
    for (auto& c : shepCap) c = ShepCap{};
    shepCapT = 3.f; shepCapNext = 0; shepPerched = false; shepPerchT = 0.f; shepSwingT = -1.f; shepSwingCd = 3.f;
    shepChase = false; shepCollapseZ = shep::PLOUGH_N + 8.f; for (float& f : shepSlabFall) f = -1.f;
    shepBoulders.clear(); shepBoulderT = 2.f; shepGrindT = 8.f; shepGrindWarn = -1.f; shepGrindX = -1e9f; shepGrindHit = false;
    for (float& h : shepShieldHp) h = SHIELD_HP; shepShieldOpenT = 0.f;
    for (int i = 0; i < 5; ++i) { shepBellDown[i] = false; shepBellFall[i] = -1.f; }
    shepBellPerch = 2; shepHopT = -1.f; shepHopNext = HOP_EVERY; shepTollT = 10.f; shepTollWarn = -1.f; shepRehangT = -1.f;
    shepCutT = -1.f; shepCutCd = 0.f; shepBellT = -1.f; shepWasStaggered = false;
}

// Where the Bellwether stands under bell i (a step south of it, so it is under the bell's mouth)
inline glm::vec3 GameplayState::shepBellSpot(int i) const {
    const shep::Bell b = shep::bells()[i];
    return {b.top.x, b.floorY, b.top.z};
}
inline glm::vec3 GameplayState::shepShieldPos(int i, const Enemy& d) const {
    const float a = gameClock * 1.1f + 1.5708f * (float)i;
    return d.position + glm::vec3{std::cos(a) * shepcomb::SHIELD_R, 0.6f + 0.4f * std::sin(gameClock * 2.f + i), std::sin(a) * shepcomb::SHIELD_R};
}

// The movers it owns: the capsules (parked while in the air), the Stampede's plates
inline void GameplayState::shepCombMovers(float dt) {
    using namespace shepcomb;
    const glm::vec3 base{0.f, shep::capHalf().y, -3380.f};
    for (int i = 0; i < (int)level.shepCaps.size() && i < shep::CAPS; ++i) {
        Mover& m = level.movers[level.shepCaps[i]];
        ShepCap& c = shepCap[i];
        if (!c.down) { m.gone = true; continue; }
        m.gone = false; m.a = m.b = c.to + glm::vec3{0.f, shep::capHalf().y, 0.f} - base; m.drive = 0.f;
    }
    for (int i = 0; i < (int)level.shepSlabs.size(); ++i) {
        Mover& m = level.movers[level.shepSlabs[i]];
        if (shepSlabFall[i] < 0.f) { m.gone = false; m.drive = 0.f; continue; }
        shepSlabFall[i] += dt;
        const float u = glm::clamp(shepSlabFall[i] / SLAB_FALL, 0.f, 1.f);
        m.drive = u * u; m.gone = u >= 1.f;
    }
}

// A capsule thrown up off the bow onto the deck, landing at `to`
inline void GameplayState::shepThrowCap(glm::vec3 to) {
    ShepCap& c = shepCap[shepCapNext];
    shepCapNext = (shepCapNext + 1) % shep::CAPS;
    if (c.down) fx.spawnBurst(c.to + glm::vec3{0.f, 1.f, 0.f}, glm::vec3{0.6f}, 14, 6.f, 0.8f, 2.f);   // (the oldest goes back into the ice)
    c = ShepCap{};
    c.t = 0.f; c.to = to; c.from = {-shep::HALF - 6.f, shep::PLAIN_Y + 2.f, to.z + cosmetic::range(-6.f, 6.f)};
    c.spin = cosmetic::range(-1.f, 1.f);
    audio.playAt("ab_rumble", c.from, 100, SoundGroup::WORLD, true, 1.3f);
    fx.spawnSpray(c.from, glm::normalize(glm::vec3{0.6f, 1.f, 0.f}), SWEEP_ICE * 0.8f, 10, 14.f, 1.2f, 0.7f);
}

// The Drover sends one of its herd rolling down the deck at you
inline void GameplayState::shepLaunch(const Enemy& d) {
    ShepBoulder b;
    b.pos = {glm::clamp(player.position.x + cosmetic::range(-3.f, 3.f), -shep::HALF + 3.f, shep::HALF - 3.f), shepcomb::BOULDER_R, std::min(d.position.z, player.position.z - 26.f)};
    b.vel = {0.f, 0.f, shepcomb::BOULDER_SPEED};
    shepBoulders.push_back(b);
    audio.playAt("ab_rumble", b.pos, 120, SoundGroup::ENEMY, true, 0.7f);
}

// Rounds that hit the Comb's own targets: the Drover's herd shield, the belfry's chains
inline bool GameplayState::shootComb(glm::vec3 from, glm::vec3 dir, float& distance, float dmg) {
    using namespace shepcomb;
    if (!shepherd() || shepTines >= shep::TINES) return false;
    Enemy* d = shepDog();
    if (d && d->type == EnemyType::DROVER && shepShieldOpenT <= 0.f)
        for (int i = 0; i < 4; ++i) {
            if (shepShieldHp[i] <= 0.f) continue;
            const glm::vec3 c = shepShieldPos(i, *d);
            const float t = rayBoxHit(from, dir, AABB{c - glm::vec3{1.3f}, c + glm::vec3{1.3f}});
            if (t <= 0.f || t >= distance) continue;
            distance = t;
            shepShieldHp[i] -= dmg;
            fx.spawnHitSparks(from + dir * t, SWEEP_ICE);
            if (shepShieldHp[i] <= 0.f) {
                fx.spawnBurst(c, SWEEP_ICE, 30, 10.f, 0.8f, 3.f);
                audio.playAt("ab_shatter", c, 110, SoundGroup::WORLD, true, 1.1f);
                int left = 0; for (float h : shepShieldHp) left += h > 0.f;
                if (left == 0) {
                    shepShieldOpenT = SHIELD_OPEN; d->shepOpen = std::max(d->shepOpen, SHIELD_OPEN);
                    pushBanner("HERD SPLIT", "THE DROVER IS OPEN", glm::vec3{1.9f, 1.15f, 0.15f} * 0.6f, 2.5f);
                    audio.play("drive_ready", 110, SoundGroup::UI, true);
                } else { char b[24]; snprintf(b, sizeof(b), "HERD %d LEFT", left); ui.feed(b, SWEEP_ICE * 0.7f); }
            }
            return true;
        }
    if (shepTines == 2 && level.arenaAt(player.position) == 2)
        for (int i = 0; i < 5; ++i) {
            if (shepBellDown[i] || shepBellFall[i] >= 0.f) continue;
            const shep::Bell b = shep::bells()[i];
            const AABB chain{{b.top.x - 0.5f, b.top.y, b.top.z - 0.5f}, {b.top.x + 0.5f, shep::BEL_TOP, b.top.z + 0.5f}};
            const float t = rayBoxHit(from, dir, chain);
            if (t <= 0.f || t >= distance) continue;
            distance = t;
            shepBellFall[i] = 0.f;   // it comes down
            fx.spawnHitSparks(from + dir * t, glm::vec3{2.4f, 1.3f, 0.35f});
            audio.playAt("clank", from + dir * t, 128, SoundGroup::WORLD, true, 0.6f);
            return true;
        }
    return false;
}

// Each sheepdog's tine kit, by zone
inline void GameplayState::shepCombKit(Enemy& d, float dt) {
    using namespace shepcomb;
    const glm::vec3 p = player.position;
    d.shepOpen = std::max(0.f, d.shepOpen - dt);
    const bool stag = d.staggered();
    const bool newlyStag = stag && !shepWasStaggered;
    shepWasStaggered = stag;
    if (stag && sweep.sweptNow(d.position, sweepShelters)) {   // caught by the Shepherd's own sweep while it is down
        d.takeDamage(d.maxHealth * 0.2f);
        fx.spawnBurst(d.position + glm::vec3{0.f, 1.5f, 0.f}, SWEEP_ICE, 30, 12.f, 0.8f, 3.f);
        ui.feed("IT TOOK THE WAVE", SWEEP_ICE);
    }
    if (d.type == EnemyType::CROOK) {
        const glm::vec3 pc = shep::perch();
        if (shepPerched) {   // up its mast: it stays there (only its hook reaches you)
            shepPerchT += dt;
            d.position.x = pc.x; d.position.z = pc.z; d.velocity.x = d.velocity.z = 0.f;
            if (newlyStag || shepPerchT > PERCH_MAX) {   // parried: the line goes taut and it comes off its mast
                shepPerched = false; d.shepHigh = false;
                d.position = {pc.x, 0.f, pc.z + 4.f}; d.floorY = 0.f; d.velocity = glm::vec3{0.f};
                d.stagger(4.f); d.staggerTimer = std::max(d.staggerTimer, 4.f);
                if (newlyStag) d.takeDamage(d.maxHealth * 0.1f);
                fx.spawnBurst(d.position + glm::vec3{0.f, 0.5f, 0.f}, SWEEP_ICE, 40, 10.f, 0.9f, 3.f);
                shake(0.8f, 0.04f);
                SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.pitch = 0.7f; audio.playOpts("slam", o);
                pushBanner(newlyStag ? "YANKED DOWN" : "IT COMES DOWN FOR YOU", "IT'S ON THE DECK: HIT IT", glm::vec3{1.f, 0.75f, 0.2f}, 2.5f);
            }
        } else {   // on the deck: its pole
            shepSwingCd = std::max(0.f, shepSwingCd - dt);
            const float dist = glm::length(glm::vec2{p.x - d.position.x, p.z - d.position.z});
            if (shepSwingT < 0.f && shepSwingCd <= 0.f && !stag && dist < SWING_REACH && d.telegraphTimer <= 0.f && shepCutT < 0.f) {
                shepSwingT = SWING_TELL;
                audio.playAt("telegraph", d.position, 110, SoundGroup::ENEMY, true, 1.2f);
            }
            if (shepSwingT >= 0.f) {
                d.velocity.x = d.velocity.z = 0.f; d.attackTimer = std::max(d.attackTimer, 0.4f);
                if (stag) shepSwingT = -1.f;
                else if ((shepSwingT -= dt) < 0.f) {
                    shepSwingT = -1.f; shepSwingCd = SWING_CD;
                    fx.spawnShockwave(d.position + glm::vec3{0.f, 0.6f, 0.f}, SWING_REACH, glm::vec3{1.9f, 1.15f, 0.15f});
                    audio.playAt("whoosh", d.position, 120, SoundGroup::ENEMY, true, 0.7f);
                    if (dist < SWING_REACH && p.y < d.position.y + 1.1f && invincFrames <= 0.f && !playerDead) {   // jumped or dashed: it goes under you
                        damagePlayer(SWING_DAMAGE, d.position + glm::vec3{0.f, 1.f, 0.f}, 0.25f, 0.04f, 0.3f, {EnemyType::CROOK, DamageKind::MELEE});
                        const glm::vec3 out = glm::normalize(glm::vec3{p.x - d.position.x, 0.f, p.z - d.position.z} + glm::vec3{1e-4f});
                        player.velocity += out * 11.f + glm::vec3{0.f, 4.f, 0.f}; player.launched();
                        ui.feed("SWEPT ASIDE", glm::vec3{1.9f, 1.15f, 0.15f} * 0.6f);
                    }
                }
            }
            // Below half: it cuts the line you hold by (gold: let go)
            shepCutCd = std::max(0.f, shepCutCd - dt);
            if (shepCutT < 0.f && shepCutCd <= 0.f && holdfast.held() && d.targetable() && !stag && d.health < shepDogMax * 0.55f && d.telegraphTimer <= 0.f) {
                shepCutT = 1.1f;
                audio.playAt("telegraph", d.position, 120, SoundGroup::ENEMY, true, 0.8f);
                if (!shepCutHint) { shepCutHint = true; ui.toast("GOLD: IT CUTS YOUR LINE", "LET GO (" + binds().label(Act::HOLDFAST) + ") BEFORE IT LANDS", glm::vec3{1.9f, 1.15f, 0.15f} * 0.6f, 3.f); }
            }
            if (shepCutT >= 0.f) {
                d.velocity.x = d.velocity.z = 0.f;
                if (holdfast.held()) fx.spawnTracer(d.position + glm::vec3{0.f, 1.6f, 0.f}, glm::mix(d.position + glm::vec3{0.f, 1.6f, 0.f}, holdfast.point, 1.f - shepCutT / 1.1f), 0.05f, dt * 1.5f, glm::vec3{1.9f, 1.15f, 0.15f});
                if ((shepCutT -= dt) < 0.f || stag) {
                    shepCutT = -1.f; shepCutCd = 7.f;
                    if (holdfast.held() && !stag && !playerDead) {
                        holdfast.cut();
                        damagePlayer(6.f, d.position + glm::vec3{0.f, 1.5f, 0.f}, 0.2f, 0.03f, 0.f, {EnemyType::CROOK, DamageKind::MELEE});
                        crookDragT = 0.7f; crookDragUid = d.uid; grapple.release();
                        ui.feed("LINE CUT", glm::vec3{1.9f, 1.15f, 0.15f});
                    }
                }
            }
        }
    }
    if (d.type == EnemyType::DROVER) {
        const bool knuckle = !shepChase || shepCollapseZ <= shep::KNUCKLE_S + 1.f || p.z < shep::KNUCKLE_S;
        d.droverPost = knuckle ? glm::vec3{8.f * std::sin(gameClock * 0.4f), 4.f, -3628.f}
                               : glm::vec3{12.f * std::sin(gameClock * 0.5f), 8.f, std::max(p.z - 30.f, shep::KNUCKLE_N + 8.f)};
        if (shepShieldOpenT > 0.f && (shepShieldOpenT -= dt) <= 0.f) {   // the herd comes back round it
            for (float& h : shepShieldHp) h = SHIELD_HP;
            ui.feed("THE HERD RE-FORMS", SWEEP_ICE * 0.7f);
        }
        if (shepShieldOpenT > 0.f) d.shepOpen = std::max(d.shepOpen, 0.2f);
        if (shepChase && !knuckle && (shepBoulderT -= dt) <= 0.f) { shepBoulderT = BOULDER_GAP; shepLaunch(d); }   // (the stampede: its herd rolls on its own)
    }
    if (d.type == EnemyType::BELLWETHER) {
        // It stands under a bell, then hops to another (a leap you can see coming)
        if (shepHopT >= 0.f) {
            shepHopT += dt;
            const float u = glm::clamp(shepHopT / HOP_LEN, 0.f, 1.f);
            d.position = glm::mix(shepHopFrom, shepHopTo, u) + glm::vec3{0.f, 7.f * std::sin(3.14159f * u), 0.f};
            d.velocity = glm::vec3{0.f}; d.attackTimer = std::max(d.attackTimer, 0.8f);
            if (u >= 1.f) { shepHopT = -1.f; d.floorY = shepHopTo.y; fx.spawnBurst(shepHopTo + glm::vec3{0.f, 0.3f, 0.f}, glm::vec3{0.7f}, 16, 5.f, 0.6f, 2.f); }
        } else {
            const glm::vec3 spot = shepBellSpot(shepBellPerch);
            d.position.x = spot.x; d.position.z = spot.z; d.velocity.x = d.velocity.z = 0.f;
            if (!stag && (shepHopNext -= dt) <= 0.f && d.telegraphTimer <= 0.f) {
                int pick = -1;
                for (int tries = 0; tries < 12 && pick < 0; ++tries) {
                    const int c = (int)cosmetic::range(0.f, 4.99f);
                    if (c != shepBellPerch && !shepBellDown[c] && shepBellFall[c] < 0.f) pick = c;
                }
                if (pick < 0) for (int c = 0; c < 5; ++c) if (c != shepBellPerch) { pick = c; break; }
                shepHopFrom = d.position; shepHopTo = shepBellSpot(pick); shepBellPerch = pick; shepHopT = 0.f; shepHopNext = HOP_EVERY;
            }
        }
        if (shepBellT >= 0.f && (shepBellT -= dt) < 0.f && !playerDead) { shepBellT = -1.f; wakeRing(d); }
    }
}

// The Comb, each tick: its zones' set-pieces, its sheepdogs, the breaking
inline void GameplayState::shepComb(float dt) {
    using namespace shepfight;
    using namespace shepcomb;
    const glm::vec3 p = player.position;
    const int at = level.arenaAt(p);
    shepCombMovers(dt);
    if (shepTines >= shep::TINES) return;
    const int k = shepTines;
    // The sheepdog: found as it arrives (back hurt from the crossing, its full kit from here)
    if (!shepDog() && shepBreakT < 0.f && (shepDogUid >= 0 || at == shepTines)) {
        if (shepDogUid >= 0) { shepBreakT = 0.f; shepDogUid = -1; }   // it died: this part of the tine breaks
        else for (auto& e : enemies)
            if (e.alive && isSheepdog(e.type) && e.state == EnemyState::ACTIVE) {
                shepDogUid = e.uid; e.tineForm = true; e.health = e.maxHealth * DOG_HURT; shepDogMax = e.health;
                shepWasStaggered = false;
                static const char* lines[3] = {"UP ITS MAST: PARRY ITS HOOK TO PULL IT DOWN", "IT DRIVES THE HERD. RUN", "IT RINGS FOR THE SHEPHERD"};
                pushBanner(e.stats().name, lines[k], HOLDFAST_STEEL, 3.5f);
                audio.playAt("hook_yank", e.position, 110, SoundGroup::ENEMY, true, 0.5f);
                if (e.type == EnemyType::CROOK) { shepPerched = true; shepPerchT = 0.f; e.shepHigh = true; }
                if (e.type == EnemyType::BELLWETHER) { shepBellPerch = 2; shepHopNext = HOP_EVERY; }
                shepSweepT = std::max(shepSweepT, 6.f);
                break;
            }
    }
    Enemy* dog = shepDog();
    if (dog && g_devOverlay == "shepkill" && dog->age > 2.5f && dog->takeDamage(1e6f)) { onEnemyKilled(*dog, StyleSource::ENVIRONMENT); dog = nullptr; }   // dev: a zone breaking
    if (dog) shepCombKit(*dog, dt);
    const bool fight = dog != nullptr && shepBreakT < 0.f && !playerDead && at == k;
    const bool waveDone = shepSweepWas && !sweep.active();
    shepSweepWas = sweep.active();

    // ---- THE PLOUGH ----
    if (k == 0) {
        if (fight && !sweep.active() && (shepSweepT -= dt) <= 0.f) {   // its aim, down the deck from its ridge
            const glm::vec3 o = shep::shepherdEye();
            SweepSpec s = sweeptier::aimed(o, p, SWEEP_WARN);
            s.flashLead = SWEEP_FLASH; s.range = 40.f;
            const float dist = glm::length(glm::vec2{p.x - o.x, p.z - o.z});
            s.coneHalf = glm::clamp(std::atan(CONE_HALF_WIDTH / std::max(dist, 1.f)), 0.02f, 0.5f);
            sweep.start(s); ++sweepCount;
            shepSweepT = cosmetic::range(SWEEP_GAP_MIN, SWEEP_GAP_MAX) + SWEEP_FLASH + SWEEP_WARN;
            if (!shepConeHint) { shepConeHint = true; ui.toast("ITS AIM", "THE RED CONE: GET OUT OF IT, OR BEHIND COVER BY ITS LIT POST", SWEEP_RED * 0.8f, 4.5f); }
        }
        if (fight && waveDone) {   // strays climb on behind the wave
            int strays = 0; for (const auto& e : enemies) strays += e.alive && e.type == EnemyType::STRAY;
            for (int sd = -1; sd <= 1 && strays < 4; sd += 2, ++strays) spawnEnemy(EnemyType::STRAY, {sd * (shep::HALF - 2.f), 0.f, glm::clamp(p.z + cosmetic::range(-15.f, 15.f), shep::PLOUGH_N + 6.f, shep::TIP - 8.f)});
        }
        // It throws up capsules off its bow: they land where the gold rings are, and lie there as cover
        const bool rage = dog && dog->health < shepDogMax * 0.4f;
        if (fight && (shepCapT -= dt) <= 0.f) {
            shepCapT = rage ? CAP_GAP_RAGE : CAP_GAP;
            glm::vec3 to = rage ? p : p + glm::vec3{cosmetic::range(-12.f, 12.f), 0.f, cosmetic::range(-14.f, 10.f)};
            to.x = glm::clamp(to.x, -shep::HALF + 3.f, shep::HALF - 3.f); to.y = 0.f;
            to.z = glm::clamp(to.z, shep::PLOUGH_N + 4.f, shep::TIP - 6.f);
            if (glm::length(glm::vec2{to.x - shep::perch().x, to.z - shep::perch().z}) < 6.f) to.x += 9.f;
            shepThrowCap(to);
        }
        for (ShepCap& c : shepCap) {
            if (c.t >= 0.f && !c.down) {
                c.t += dt;
                if (c.t >= CAP_FLIGHT) {   // it lands
                    c.down = true; c.life = CAP_LIFE;
                    fx.spawnBurst(c.to + glm::vec3{0.f, 0.4f, 0.f}, SWEEP_ICE * 0.7f, 24, 8.f, 0.9f, 2.f);
                    audio.playAt("land_heavy", c.to, 120, SoundGroup::WORLD, true, 0.7f);
                    shake(0.25f, 0.02f);
                    if (!playerDead && glm::length(glm::vec2{p.x - c.to.x, p.z - c.to.z}) < CAP_HIT && p.y < 2.5f) {
                        damagePlayer(CAP_DAMAGE, c.to, 0.25f, 0.04f, 0.3f, {EnemyType::COUNT, DamageKind::UNKNOWN});
                        const glm::vec3 out = glm::normalize(glm::vec3{p.x - c.to.x, 0.f, p.z - c.to.z} + glm::vec3{1e-4f});
                        player.velocity += out * 9.f + glm::vec3{0.f, 4.f, 0.f}; player.launched();
                    }
                    for (auto& e : enemies)
                        if (e.targetable() && glm::length(glm::vec2{e.position.x - c.to.x, e.position.z - c.to.z}) < CAP_HIT && e.takeDamage(e.type == EnemyType::CROOK ? 40.f : 80.f) && !isSheepdog(e.type))
                            onEnemyKilled(e, StyleSource::ENVIRONMENT);
                }
            } else if (c.down && (c.life -= dt) <= 0.f) { c.down = false; c.t = -1.f; fx.spawnBurst(c.to + glm::vec3{0.f, 1.f, 0.f}, glm::vec3{0.6f}, 14, 6.f, 0.8f, 2.f); }
        }
        if (std::fmod(gameClock, 0.04f) < dt)   // its bow: ice and capsule-bits thrown up all down the west face
            fx.spawnSpray({-shep::HALF - 3.f, shep::PLAIN_Y + 1.f, cosmetic::range(shep::PLOUGH_N, shep::TIP)}, glm::normalize(glm::vec3{-0.6f, 1.f, cosmetic::range(-0.2f, 0.2f)}), SWEEP_ICE * 0.7f, 2, 13.f, 1.6f, 0.6f);
    }
    // ---- THE STAMPEDE ----
    if (k == 1) {
        if (!shepChase && fight && p.z < shep::PLOUGH_N - 2.f) {
            shepChase = true; shepCollapseZ = shep::PLOUGH_N + 6.f;
            pushBanner("IT IS BREAKING UP", "RUN", SWEEP_RED * 0.8f, 2.5f);
            shake(0.8f, 0.04f);
        }
        if (shepChase && shepCollapseZ > shep::KNUCKLE_S) {
            const float behind = shepCollapseZ - p.z;
            shepCollapseZ -= dt * (behind > 35.f ? COLLAPSE_CATCH : COLLAPSE_SPEED);
            if (dog == nullptr || shepBreakT >= 0.f) shepCollapseZ -= dt * 20.f;   // (it all goes, once the herd scatters)
            if (std::fmod(gameClock, 0.05f) < dt) {   // the breaking edge: dust and plating
                fx.spawnBurst({cosmetic::range(-shep::HALF, shep::HALF), 0.f, shepCollapseZ}, glm::vec3{0.7f, 0.65f, 0.6f}, 4, 6.f, 1.2f, -3.f);
                shake(0.06f, 0.012f);
            }
            if (std::fmod(gameClock, 1.1f) < dt) audio.playAt("ab_rumble", {0.f, 0.f, shepCollapseZ}, 110, SoundGroup::WORLD, true, 0.5f);
            for (int i = 0; i < shep::SLABS; ++i)
                if (shepSlabFall[i] < 0.f && shepCollapseZ < shep::slabZ(i) - 1.f) {
                    shepSlabFall[i] = 0.f;
                    audio.playAt("ab_shatter", {0.f, 0.f, shep::slabZ(i) - shep::SLAB * 0.5f}, 110, SoundGroup::WORLD, true, 0.5f);
                }
        }
        for (auto& b : shepBoulders) {   // its herd, rolling at you
            b.pos += b.vel * dt; b.life += dt; b.spin += dt * BOULDER_SPEED / BOULDER_R;
            const int slab = (int)((shep::PLOUGH_N - b.pos.z) / shep::SLAB);
            if (slab >= 0 && slab < shep::SLABS && shepSlabFall[slab] >= 0.f) b.vel.y -= 24.f * dt;   // (off the edge)
            if (!b.hit && !playerDead && glm::length(glm::vec2{p.x - b.pos.x, p.z - b.pos.z}) < BOULDER_R + 0.6f && p.y < b.pos.y + BOULDER_R) {
                b.hit = true;
                damagePlayer(BOULDER_DAMAGE, b.pos, 0.3f, 0.05f, 0.f, {EnemyType::DROVER, DamageKind::MELEE});
                holdfast.cut();
                player.velocity += glm::vec3{(p.x < b.pos.x ? -1.f : 1.f) * 8.f, 5.f, 6.f}; player.launched();
                ui.feed("RUN DOWN", HOLDFAST_STEEL);
            }
        }
        shepBoulders.erase(std::remove_if(shepBoulders.begin(), shepBoulders.end(), [&](const ShepBoulder& b) { return b.life > 6.f || b.pos.y < -20.f; }), shepBoulders.end());
        // The Shepherd's other teeth grind across the deck: a red band, then the tooth
        if (fight) {
            if (shepGrindWarn < 0.f && shepGrindX < -1e8f && (shepGrindT -= dt) <= 0.f) {
                shepGrindWarn = GRIND_WARN; shepGrindHit = false;
                shepGrindZ = glm::clamp(p.z - cosmetic::range(0.f, 8.f), shep::KNUCKLE_N + 6.f, shep::PLOUGH_N - 4.f);
                audio.play("telegraph", 128, SoundGroup::WORLD, true);
                audio.playAt("ab_rumble", {-60.f, 2.f, shepGrindZ}, 128, SoundGroup::WORLD, true, 0.4f);
                if (!shepGrindHint) { shepGrindHint = true; ui.toast("ITS OTHER TEETH", "A RED BAND ACROSS THE DECK: A TOOTH IS COMING THROUGH IT. GET OUT", SWEEP_RED * 0.8f, 4.f); }
            }
            if (shepGrindWarn >= 0.f && (shepGrindWarn -= dt) < 0.f) { shepGrindWarn = -1.f; shepGrindX = -shep::HALF - 40.f; }
        }
        if (shepGrindX > -1e8f) {
            shepGrindX += GRIND_SPEED * dt;
            shake(0.12f, 0.02f);
            if (!shepGrindHit && !playerDead && std::fabs(p.z - shepGrindZ) < GRIND_HZ && std::fabs(p.x - shepGrindX) < 30.f && p.y < 7.f) {
                shepGrindHit = true;
                damagePlayer(GRIND_DAMAGE, {shepGrindX - 30.f, 2.f, shepGrindZ}, 0.5f, 0.07f, 0.f, {EnemyType::COUNT, DamageKind::UNKNOWN});
                player.velocity += glm::vec3{9.f, 7.f, p.z < shepGrindZ ? -6.f : 6.f}; player.launched();
                ui.feed("GROUND UNDER IT", SWEEP_RED);
            }
            for (auto& e : enemies)
                if (e.targetable() && !isSheepdog(e.type) && std::fabs(e.position.z - shepGrindZ) < GRIND_HZ && std::fabs(e.position.x - shepGrindX) < 30.f && e.takeDamage(1e6f)) onEnemyKilled(e, StyleSource::ENVIRONMENT);
            if (shepGrindX > shep::HALF + 60.f) { shepGrindX = -1e9f; shepGrindT = cosmetic::range(9.f, 12.f); }
        }
        if (fight && (shepStrayT -= dt) <= 0.f && p.z < shep::KNUCKLE_S) {   // on the knuckle, strays climb up its sides now and then
            shepStrayT = 13.f;
            int strays = 0; for (const auto& e : enemies) strays += e.alive && e.type == EnemyType::STRAY;
            for (int sd = -1; sd <= 1 && strays < 3; sd += 2, ++strays) spawnEnemy(EnemyType::STRAY, {sd * (shep::HALF - 2.f), 0.f, cosmetic::range(shep::KNUCKLE_N + 6.f, shep::KNUCKLE_S - 6.f)});
        }
    }
    // ---- THE BELFRY ----
    if (k == 2) {
        for (int i = 0; i < 5; ++i) {   // bells coming down
            if (shepBellFall[i] < 0.f) continue;
            const float before = shepBellFall[i];
            shepBellFall[i] += dt;
            if (before < BELL_FALL && shepBellFall[i] >= BELL_FALL) {
                shepBellDown[i] = true;
                const glm::vec3 spot = shepBellSpot(i);
                fx.spawnBurst(spot + glm::vec3{0.f, 0.5f, 0.f}, glm::vec3{2.f, 1.4f, 0.6f}, 40, 10.f, 1.f, 3.f);
                fx.spawnShockwave(spot, 6.f, glm::vec3{2.4f, 1.3f, 0.35f});
                shake(0.9f, 0.05f);
                audio.playAt("ab_ting", spot, 128, SoundGroup::WORLD, true, 0.35f);
                SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.pitch = 0.6f; audio.playOpts("slam", o);
                if (dog && dog->type == EnemyType::BELLWETHER && shepHopT < 0.f && glm::length(glm::vec2{dog->position.x - spot.x, dog->position.z - spot.z}) < BELL_HIT && std::fabs(dog->floorY - spot.y) < 2.f) {
                    dog->takeDamage(dog->maxHealth * CRUSH);
                    dog->stagger(4.f); dog->staggerTimer = std::max(dog->staggerTimer, 4.f);
                    pushBanner("CRUSHED", "UNDER ITS OWN BELL", glm::vec3{2.4f, 1.3f, 0.35f} * 0.6f, 2.f);
                    styleSystem.addStyle(200.f, StyleSource::ENVIRONMENT);
                } else ui.feed("MISSED IT", glm::vec3{0.75f});
                if (!playerDead && glm::length(glm::vec2{p.x - spot.x, p.z - spot.z}) < BELL_HIT && std::fabs(p.y - spot.y) < 2.f)
                    damagePlayer(30.f, spot + glm::vec3{0.f, 3.f, 0.f}, 0.4f, 0.06f, 0.f, {EnemyType::COUNT, DamageKind::UNKNOWN});
                for (auto& e : enemies)
                    if (e.targetable() && !isSheepdog(e.type) && glm::length(glm::vec2{e.position.x - spot.x, e.position.z - spot.z}) < BELL_HIT && e.takeDamage(1e6f)) onEnemyKilled(e, StyleSource::ENVIRONMENT);
            }
        }
        bool any = false; for (int i = 0; i < 5; ++i) any |= !shepBellDown[i];
        if (!any && dog && shepRehangT < 0.f) shepRehangT = 6.f;   // all down: it hangs them again
        if (shepRehangT >= 0.f && (shepRehangT -= dt) < 0.f) {
            shepRehangT = -1.f;
            for (int i = 0; i < 5; ++i) { shepBellDown[i] = false; shepBellFall[i] = -1.f; }
            ui.feed("THE BELLS ARE HUNG AGAIN", glm::vec3{2.4f, 1.3f, 0.35f} * 0.5f);
        }
        // The Shepherd tolls its great bell: rings along every floor at once
        if (fight) {
            if (shepTollWarn < 0.f && (shepTollT -= dt) <= 0.f) {
                shepTollWarn = TOLL_WARN;
                audio.play("telegraph", 128, SoundGroup::WORLD, true);
                if (!shepTollHint) { shepTollHint = true; ui.toast("IT TOLLS", "THE GREAT BELL: A RING ALONG EVERY FLOOR. JUMP IT", POST_LIT * 0.4f, 3.5f); }
            }
            if (shepTollWarn >= 0.f && (shepTollWarn -= dt) < 0.f) {
                shepTollWarn = -1.f; shepTollT = TOLL_EVERY;
                const glm::vec3 g = shep::greatBell();
                for (float y : {0.f, shep::BALC_Y}) wakeRings.push_back({glm::vec3{g.x, y, g.z}, 0.6f, false});
                SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.pitch = 0.32f; o.volume = 1.f; audio.playOpts("ab_ting", o);
                shake(0.6f, 0.03f);
                int strays = 0; for (const auto& e : enemies) strays += e.alive && e.type == EnemyType::STRAY;
                for (int sd = -1; sd <= 1 && strays < 3; sd += 2, ++strays) spawnEnemy(EnemyType::STRAY, {sd * 6.f, 0.f, shep::BEL_S - 6.f});
            }
        }
    }

    // ---- a zone's sheepdog dead: that part of the tine breaks ----
    if (shepBreakT >= 0.f) {
        const float before = shepBreakT;
        shepBreakT += dt;
        if (before == 0.f) {
            static const char* titles[3] = {"THE PLOUGH BREAKS", "THE HERD SCATTERS", "THE BELFRY FALLS SILENT"};
            static const char* subs[3] = {"ON, UP THE TINE", "ON, TO ITS ROOT", "THE COMB IS BROKEN"};
            pushBanner(titles[k], subs[k], SWEEP_ICE * 0.8f, 3.5f);
            shake(1.4f, 0.06f);
            SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.pitch = 0.4f; o.volume = 1.f;
            audio.playOpts("slam", o); o.delay = 0.7f; o.pitch = 0.33f; audio.playOpts("slam", o);
            sweep.reset(); shepBoulders.clear(); shepGrindWarn = -1.f; shepGrindX = -1e9f; shepTollWarn = -1.f;
            for (auto& e : enemies)
                if (e.targetable() && !isSheepdog(e.type) && e.takeDamage(1e6f)) {
                    onEnemyKilled(e, StyleSource::ENVIRONMENT);
                    if (sweptBodies.size() < 24) sweptBodies.push_back({e.position + glm::vec3{0.f, e.height() * 0.5f, 0.f}, glm::vec3{-1.f, 0.f, 0.f}, glm::vec3{e.radius() * 1.6f, e.height(), e.radius() * 1.2f}, e.stats().color, 0.f, (float)(e.uid % 7) - 3.f});
                }
            shepherdDamageT = 1.f;
        }
        if (shepBreakT < 1.6f && std::fmod(gameClock, 0.06f) < dt) {   // cracking up the tine
            const float z = k == 0 ? glm::mix(shep::TIP, shep::PLOUGH_N, shepBreakT / 1.6f) : k == 1 ? glm::mix(shep::PLOUGH_N, shep::KNUCKLE_N, shepBreakT / 1.6f) : shep::BEL_S - 30.f;
            fx.spawnBurst({cosmetic::range(-14.f, 14.f), k == 2 ? cosmetic::range(0.f, 30.f) : 0.4f, z}, glm::vec3{2.f, 1.8f, 1.5f}, 12, 9.f, 0.6f, 3.f);
            shake(0.12f, 0.02f);
        }
        if (before < BREAK_LEN && shepBreakT >= BREAK_LEN) { shepTines = std::max(shepTines, k + 1); shepBreakT = -1.f; if (k == shep::TINES - 1) shepEndT = 0.f; }
    }
}

// The Comb drawn live: capsules in the air and on the deck (gold rings where they'll land), the
// falling plates, the herd rolling and circling, the tooth grinding through, the bells and their chains
inline void GameplayState::gatherComb(std::vector<BoxInstance>& out) {
    using namespace rig;
    using namespace shepcomb;
    const glm::vec3 gold = abGoldTell(glm::vec3{1.9f, 1.15f, 0.15f});
    const glm::vec3 capCol{0.62f, 0.64f, 0.64f}, amber{2.4f, 1.3f, 0.35f}, h = shep::capHalf();
    auto capsule = [&](glm::vec3 c, glm::mat4 rot) {
        push(out, T(c) * rot * S(h * 2.f), capCol);
        push(out, T(c) * rot * T(glm::vec3{0.f, h.y + 0.05f, h.z * 0.6f}) * S(glm::vec3{0.5f, 0.12f, 0.5f}), glm::vec3{0.1f}, amber * 0.9f);
    };
    for (const ShepCap& c : shepCap) {
        if (c.down) { capsule(c.to + glm::vec3{0.f, h.y, 0.f}, glm::mat4(1.f)); continue; }
        if (c.t < 0.f) continue;
        const float u = glm::clamp(c.t / CAP_FLIGHT, 0.f, 1.f);
        const glm::vec3 at = glm::mix(c.from, c.to + glm::vec3{0.f, h.y, 0.f}, u) + glm::vec3{0.f, 16.f * std::sin(3.14159f * u), 0.f};
        capsule(at, RY(c.spin * 4.f * c.t) * RX(c.spin * 6.f * c.t));
        const int n = 20;   // where it will land: a gold ring closing in
        const float r = CAP_HIT * (1.4f - 0.4f * u);
        for (int i = 0; i < n; ++i) {
            const float a = 6.2831853f * i / n;
            push(out, T(c.to + glm::vec3{std::cos(a) * r, 0.06f, std::sin(a) * r}) * RY(-a) * S(glm::vec3{0.2f, 0.05f, 6.2831853f * r / n}), gold * 0.2f, gold * (0.6f + 0.8f * u));
        }
    }
    for (int i = 0; i < (int)level.shepSlabs.size(); ++i) {   // the Stampede's plates
        const Mover& m = level.movers[level.shepSlabs[i]];
        if (m.gone) continue;
        const AABB& bx = level.walls[m.wall].box;
        const float near = shepChase ? glm::clamp(1.f - (shepCollapseZ - shep::slabZ(i)) / SLAB_SHAKE, 0.f, 1.f) : 0.f;
        const glm::vec3 jit = (near > 0.f && shepSlabFall[i] < 0.f) ? glm::vec3{0.f, 0.04f * near * std::sin(gameClock * 60.f + i), 0.f} : glm::vec3{0.f};
        push(out, T((bx.min + bx.max) * 0.5f + jit) * S(bx.max - bx.min), m.color);
        push(out, T(glm::vec3{0.f, bx.max.y + 0.02f, bx.max.z - 0.15f} + jit) * S(glm::vec3{2.f * shep::HALF - 1.f, 0.04f, 0.2f}), glm::vec3{0.05f}, amber * (0.15f + 1.6f * near));   // its seam: glowing as it goes
    }
    for (const ShepBoulder& b : shepBoulders) {
        BoxInstance bi; bi.mesh = (float)meshlib::lumpBall(); bi.color = glm::vec3{0.6f, 0.58f, 0.53f};
        bi.model = T(b.pos) * RX(b.spin) * S(glm::vec3{BOULDER_R * 2.f}); out.push_back(bi);
        for (float z = b.pos.z + 2.f; z < player.position.z + 4.f && z < b.pos.z + 40.f; z += 2.5f)   // its lane, gold, ahead of it
            push(out, T(glm::vec3{b.pos.x, 0.05f, z}) * S(glm::vec3{BOULDER_R * 2.f, 0.04f, 1.2f}), gold * 0.2f, gold * 0.7f);
    }
    if (Enemy* d = shepDog(); d && d->type == EnemyType::DROVER)
        for (int i = 0; i < 4; ++i) {
            if (shepShieldHp[i] <= 0.f || shepShieldOpenT > 0.f) continue;
            BoxInstance bi; bi.mesh = (float)meshlib::lumpBall(); bi.color = glm::vec3{0.66f, 0.64f, 0.6f} * (0.6f + 0.4f * shepShieldHp[i] / SHIELD_HP);
            bi.model = T(shepShieldPos(i, *d)) * RY(gameClock + i) * S(glm::vec3{2.6f, 2.2f, 2.4f}); out.push_back(bi);
        }
    if (shepGrindWarn >= 0.f || shepGrindX > -1e8f) {   // the band, and the tooth through it
        const float u = shepGrindWarn >= 0.f ? 1.f - shepGrindWarn / GRIND_WARN : 1.f;
        push(out, T(glm::vec3{0.f, 0.05f, shepGrindZ}) * S(glm::vec3{2.f * shep::HALF, 0.04f, 2.f * GRIND_HZ}), SWEEP_RED * 0.15f, SWEEP_RED * (0.5f + 1.5f * u * (0.6f + 0.4f * std::sin(gameClock * 20.f))));
        const float x = shepGrindX > -1e8f ? shepGrindX : -shep::HALF - 40.f - (1.f - u) * 120.f;
        push(out, T(glm::vec3{x, 3.f, shepGrindZ}) * S(glm::vec3{60.f, 12.f, 2.f * GRIND_HZ - 1.f}), glm::vec3{0.22f, 0.21f, 0.2f});
        push(out, T(glm::vec3{x + 30.f, 3.f, shepGrindZ}) * S(glm::vec3{0.6f, 12.f, 2.f * GRIND_HZ - 1.f}), glm::vec3{0.1f}, POST_LIT * 0.8f);
    }
    if (shepTines == 2 || (shepTines < 2 && player.position.z < shep::KNUCKLE_S)) {   // the bells
        for (int i = 0; i < 5; ++i) {
            const shep::Bell b = shep::bells()[i];
            float drop = 0.f;
            if (shepBellDown[i]) drop = 1.f;
            else if (shepBellFall[i] >= 0.f) { const float u = shepBellFall[i] / BELL_FALL; drop = u * u; }
            const float topY = glm::mix(b.top.y, b.floorY + shep::BELL_H, drop);
            const glm::vec3 c{b.top.x, topY, b.top.z};
            Enemy* d = shepDog();
            const bool over = !shepBellDown[i] && shepBellFall[i] < 0.f && d && d->type == EnemyType::BELLWETHER && shepHopT < 0.f && shepBellPerch == i;
            const float tilt = shepBellDown[i] ? 0.25f : 0.05f * std::sin(gameClock * 1.3f + i);
            for (int s2 = 0; s2 < 3; ++s2) {   // the bell: three rings, widening down
                const float w = shep::BELL_R * (0.55f + 0.25f * s2);
                push(out, T(c - glm::vec3{0.f, shep::BELL_H * (0.2f + 0.3f * s2), 0.f}) * RZ(tilt) * S(glm::vec3{w * 2.f, shep::BELL_H * 0.34f, w * 2.f}), glm::vec3{0.55f, 0.42f, 0.22f}, over ? amber * (0.25f + 0.2f * std::sin(gameClock * 6.f)) : glm::vec3{0.f});
            }
            if (!shepBellDown[i] && shepBellFall[i] < 0.f) {   // its chain (shoot it)
                const float len = shep::BEL_TOP - b.top.y;
                push(out, T(glm::vec3{b.top.x, b.top.y + len * 0.5f, b.top.z}) * S(glm::vec3{0.35f, len, 0.35f}), glm::vec3{0.3f, 0.3f, 0.32f}, over ? amber * 1.2f : glm::vec3{0.f});
            }
        }
        const glm::vec3 g = shep::greatBell();
        const float flare = shepTollWarn >= 0.f ? 1.f - shepTollWarn / TOLL_WARN : 0.f;
        for (int s2 = 0; s2 < 3; ++s2) {
            const float w = 2.f * shep::BELL_R * (0.55f + 0.25f * s2);
            push(out, T(g - glm::vec3{0.f, 2.f * shep::BELL_H * (0.2f + 0.3f * s2), 0.f}) * S(glm::vec3{w * 2.f, shep::BELL_H * 0.7f, w * 2.f}), glm::vec3{0.3f, 0.28f, 0.25f}, POST_LIT * (0.1f + 2.5f * flare));
        }
    }
}

// Where to look in the Comb
inline void GameplayState::combMarkers(const glm::mat4& view, const glm::mat4& proj) {
    if (shepBreakT >= 0.f) return;
    char buf[64]; float sx, sy;
    Enemy* d = shepDog();
    const glm::vec3 p = player.position;
    if (shepTines == 1 && shepChase && p.z > shep::KNUCKLE_S) {   // the stampede: where to run
        const glm::vec3 at{0.f, 2.f, shep::KNUCKLE_S - 6.f};
        const bool on = projectToScreen(at, view, proj, sx, sy);
        snprintf(buf, sizeof(buf), "RUN  %dM", (int)glm::length(at - p));
        ui.renderMarker(sx, sy, on, glm::vec3{0.4f, 1.f, 0.6f}, buf);
    }
    if (shepTines == 2 && d && d->type == EnemyType::BELLWETHER && shepHopT < 0.f && !shepBellDown[shepBellPerch] && shepBellFall[shepBellPerch] < 0.f) {
        const shep::Bell b = shep::bells()[shepBellPerch];
        const glm::vec3 at{b.top.x, (b.top.y + shep::BEL_TOP) * 0.5f, b.top.z};
        const bool on = projectToScreen(at, view, proj, sx, sy);
        ui.renderMarker(sx, sy, on, glm::vec3{1.f, 0.6f, 0.15f}, "SHOOT THE CHAIN");
    }
    if (d && d->state == EnemyState::ACTIVE) {
        const glm::vec3 at = d->position + glm::vec3{0.f, d->height() + 0.8f, 0.f};
        const bool on = projectToScreen(at, view, proj, sx, sy);
        const char* note = d->type == EnemyType::CROOK && shepPerched ? "  PARRY ITS HOOK" : d->type == EnemyType::DROVER && shepShieldOpenT <= 0.f ? "  SHOOT ITS HERD" : "";
        snprintf(buf, sizeof(buf), "%s %dM%s", d->stats().name, (int)glm::length(at - p), note);
        ui.renderMarker(sx, sy, on, d->shepOpen > 0.f || d->staggered() ? glm::vec3{1.f, 0.75f, 0.2f} : glm::vec3{0.95f, 0.92f, 0.8f}, buf);
    }
}

// The objective line in the Comb
inline const char* GameplayState::combObjective() {
    static char line[110];
    const int k = std::min(shepTines, 2);
    const char* step = "";
    Enemy* d = shepDog();
    if (shepBreakT >= 0.f || level.arenaAt(player.position) < shepTines) step = k == 2 ? "THE COMB IS BROKEN" : "ON, UP THE TINE";
    else if (k == 0) step = sweep.active() ? "OUT OF THE RED CONE, OR BEHIND COVER" : shepPerched ? "THE CROOK IS UP ITS MAST: PARRY ITS WHITE HOOK TO PULL IT DOWN" : "KILL THE CROOK. GOLD RINGS: CAPSULES LANDING";
    else if (k == 1) step = shepGrindWarn >= 0.f || shepGrindX > -1e8f ? "A TOOTH IS COMING THROUGH THE RED BAND: GET OUT" : shepChase && player.position.z > shep::KNUCKLE_S ? "IT IS BREAKING UP BEHIND YOU: RUN" : (d && shepShieldOpenT <= 0.f ? "SHOOT ITS HERD APART, THEN THE DROVER" : "IT'S OPEN: KILL THE DROVER");
    else step = shepTollWarn >= 0.f ? "IT TOLLS: JUMP THE RING" : "DROP A BELL ON THE BELLWETHER: SHOOT THE CHAIN ABOVE IT";
    snprintf(line, sizeof(line), "%d/3  /  %s", k + 1, step);
    return line;
}
