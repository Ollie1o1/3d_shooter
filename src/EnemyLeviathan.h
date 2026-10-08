#pragma once
// =============================================================================
// EnemyLeviathan.h — THE LEVIATHAN's mind (Enemy::thinkLeviathan), and its
// body: the curve out of its root up to its head, and the zones you can hit.
// Included at the end of Enemy.h. It only decides and emits events
// (EnemyEvents::lv*); LeviathanHazards.h and Gameplay_Leviathan.h turn them
// into what hurts you.
//
//   THE DEEP    (100-65 %)  towers out of the pool. CRASH: rears, then slams
//                           its head along a strip to where you stood (dash
//                           out; it lies BEACHED 3 s, the eye open x3; or
//                           punch as it lands: staggered 4 s). TORRENT: orbs
//                           to parry into its eye. TIDE: a wave to jump.
//                           SPIT for anyone keeping away, hiding or perching.
//   THE HUNT    (65-30 %)   two attacks from a root, then it SUBMERGEs and
//                           BREACHes out of the well (or the pool) nearest you.
//   THE ECLIPSE (< 30 %)    back in the pool, the Maw floods, the eye open x2,
//                           everything 25 % sooner; SWALLOW every third attack:
//                           it inhales and drags you in. Shoot down its throat
//                           (450) and it chokes.
// =============================================================================

inline float levSmooth(float t) { t = glm::clamp(t, 0.f, 1.f); return t * t * (3.f - 2.f * t); }

// The sites it can breach from in THE HUNT: the pool, then the wells
inline int levSiteCount(const EnemyWorld& w) { return 1 + (w.hasLair ? w.wellCount : 0); }
inline glm::vec3 levSite(const Enemy& e, const EnemyWorld& w, int i) {
    return i == 0 || !w.hasLair ? e.levHome : glm::vec3{w.wells[i - 1].x, w.lairFloor, w.wells[i - 1].z};
}

inline void Enemy::thinkLeviathan(float dt, const EnemyWorld& w, bool resolve) {
    if (!levInit) {   // where it lives: the pool it was spawned in
        levInit = true;
        levHome = levRoot = w.hasLair ? w.lair : position;
        levFloor = w.hasLair ? w.lairFloor : position.y;
        levStage = LevStage::RISE; levStageT = LV_RISE; ev.lvRise = true;
        position = levRoot - glm::vec3{0.f, 8.f, 0.f};
        attackTimer = 0.f;
    }
    const float hp = health / maxHealth;
    glm::vec3 feet = w.playerFeet;
    glm::vec2 rel{feet.x - levRoot.x, feet.z - levRoot.z};
    const float d = glm::length(rel);
    const float rootR = levAtPool() ? LV_POOL : 3.5f;   // its root's edge

    // ---- phases ----
    if (levPhase == 1 && hp <= LV_PHASE2) {
        levPhase = 2; ev.lvPhase = 2; ev.enraged = true; phaseHold = PHASE_PAUSE;
        attack = AttackKind::NONE; telegraphTimer = 0.f; levBeached = 0.f; levSiteAttacks = 2;   // straight under
    }
    if (levPhase == 2 && hp <= LV_PHASE3) {
        levPhase = 3; ev.lvPhase = 3; ev.enraged = true; phaseHold = PHASE_PAUSE;
        attack = AttackKind::NONE; telegraphTimer = 0.f; levBeached = 0.f; levInhale = 0.f;
        levSiteAttacks = levAtPool() ? 0 : 2;   // away from home: back to the pool first
        levSeq = 0;
    }
    const float quick = levPhase == 3 ? 0.75f : 1.f;
    if (phaseHold > 0.f) phaseHold -= dt;
    if (levBeached > 0.f) levBeached -= dt;

    // ---- rising out of the pool, or hidden under the ring ----
    if (levStage == LevStage::RISE) {
        levStageT -= dt;
        if (levStageT <= 0.f) { levStage = LevStage::FIGHT; phaseHold = PHASE_PAUSE; }
        levPose(dt, w);
        return;
    }
    if (levStage == LevStage::HIDDEN) {
        levStageT -= dt;
        if (levStageT <= 0.f) {   // up through the floor: the well boils first
            levStage = LevStage::FIGHT;
            ev.lvBreachTell = true; ev.lvSite = levTarget;
            startAttack(AttackKind::BREACH, 1.2f);
            telegraphTimer = telegraphDuration = 1.2f;   // a place to get away from: the same on every difficulty
        }
        levPose(dt, w);
        return;
    }

    // ---- an inhale in progress ----
    if (levInhale > 0.f) {
        levInhale -= dt;
        if (levInhale <= 0.f) levEndInhale();
        yaw += glm::clamp(std::remainder(std::atan2(rel.x, rel.y) - yaw, 6.2831853f), -1.2f * dt, 1.2f * dt);
        levPose(dt, w);
        return;
    }

    // ---- what it wound up lands ----
    if (resolve) {
        AttackKind k = attack;
        attack = AttackKind::NONE;
        switch (k) {
            case AttackKind::CRASH:
                ev.lvCrash = true; ev.lvFrom = levRoot; ev.lvTo = levTarget;
                levBeached = LV_BEACHED; ev.lvBeached = true;
                ++levSiteAttacks;
                if (levPhase == 2) levSiteAttacks = 2;   // a beaching spends its time at this root
                break;
            case AttackKind::TORRENT:
                fireAt(w.playerEye, tune_->windup < 1.f ? 11 : 9, 1.1f, 16.f, 12.f, 1.7f);
                ev.shotParry = 180.f;   // parried back, it homes into the eye (Gameplay_Leviathan.h)
                ++levSiteAttacks;
                break;
            case AttackKind::TIDE:
                ev.lvTide = true; ev.lvTideAt = glm::vec3{levRoot.x, levFloor, levRoot.z}; ev.lvTideR = levAtPool() ? 46.f : 20.f;
                ++levSiteAttacks;
                break;
            case AttackKind::SPIT: break;   // its marker went down when it began
            case AttackKind::SWALLOW:
                levInhale = LV_INHALE; levChoke = 0.f; ev.lvInhale = true;
                ++levSiteAttacks;
                break;
            case AttackKind::SUBMERGE: {   // under, and off toward the site nearest you
                levStage = LevStage::HIDDEN; levStageT = LV_HIDDEN;
                levWakeFrom = levRoot;
                int best = 0; float bd = 1e9f;
                for (int i = 0; i < levSiteCount(w); ++i) {
                    glm::vec3 s = levSite(*this, w, i);
                    float sd = glm::length(glm::vec2{feet.x - s.x, feet.z - s.z});
                    bool same = glm::length(glm::vec2{s.x - levRoot.x, s.z - levRoot.z}) < 1.f;
                    if (levPhase == 3) sd = i == 0 ? 0.f : 1e9f;          // the eclipse: home
                    else if (same && levSiteCount(w) > 1) continue;      // never where it just was
                    if (sd < bd) { bd = sd; best = i; }
                }
                levTarget = levSite(*this, w, best);
                break;
            }
            case AttackKind::BREACH:
                levRoot = levTarget; levSiteAttacks = 0;
                ev.lvBreach = true; ev.lvSite = levRoot;
                recoverTimer = 0.8f;
                position = levRoot + glm::vec3{0.f, 6.f, 0.f};
                break;
            default: break;
        }
    }

    // ---- facing you (not while it's committed to a strike) ----
    if (telegraphTimer <= 0.f && levBeached <= 0.f)
        yaw += glm::clamp(std::remainder(std::atan2(rel.x, rel.y) - yaw, 6.2831853f), -1.6f * dt, 1.6f * dt);

    // ---- the spit: keeping away, hiding or perching doesn't last ----
    glm::vec3 eye = position + glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)} * 2.f;
    bool away = d > LV_SPIT_FAR || feet.y > levFloor + LV_SPIT_HIGH || !lineOfSight(eye, w);
    farTimer = away && attack != AttackKind::BREACH ? farTimer + dt : 0.f;

    levPose(dt, w);
    if (telegraphTimer > 0.f || phaseHold > 0.f || levBeached > 0.f || recoverTimer > 0.f) return;
    if (farTimer >= LV_SPIT_AFTER) {
        farTimer = 0.f;
        ev.lvSpit = true; ev.lvSpitAt = feet + w.playerVel * 1.f; ev.lvSpitAt.y = feet.y;   // where you'll be when it lands
        startAttack(AttackKind::SPIT, 1.f);
        return;
    }

    // ---- what it starts next ----
    if (levPhase >= 2 && levSiteAttacks >= 2) { startAttack(AttackKind::SUBMERGE, 0.8f); return; }
    if (!attackReady(dt / quick)) return;
    ++attackCount;
    auto crash = [&]() {   // locked on where you stand now, within its reach
        glm::vec2 to = d > LV_REACH ? rel / d * LV_REACH : rel;
        levTarget = glm::vec3{levRoot.x + to.x, levFloor, levRoot.z + to.y};
        ev.lvCrashMark = true; ev.lvFrom = levRoot; ev.lvTo = levTarget;
        startAttack(AttackKind::CRASH, 1.1f * quick);
    };
    if (d < rootR + 10.f) { crash(); return; }   // hugging its root
    int s = levSeq++;
    if (levPhase == 1) {
        switch (s % 4) { case 0: case 2: crash(); break; case 1: startAttack(AttackKind::TORRENT, 0.8f); break;
                         default: startAttack(AttackKind::TIDE, 0.9f); break; }
    } else if (levPhase == 2) {
        if (levSiteAttacks == 0) crash();
        else if (d < 18.f && !levAtPool()) startAttack(AttackKind::TIDE, 0.9f);
        else startAttack(AttackKind::TORRENT, 0.8f);
    } else {
        if (s % 3 == 2) startAttack(AttackKind::SWALLOW, 0.9f * quick);
        else if (s % 6 == 0 || s % 6 == 4) crash();
        else if (s % 6 == 1) startAttack(AttackKind::TORRENT, 0.8f * quick);
        else startAttack(AttackKind::TIDE, 0.9f * quick);
    }
}

// Staggered (a parried crash, a choke): the head down on the ring, reeling
inline void Enemy::levReel(float dt) {
    glm::vec3 fwd{std::sin(yaw), 0.f, std::cos(yaw)};
    glm::vec3 want = levTarget + glm::vec3{0.f, 1.6f, 0.f};
    if (glm::length(glm::vec2{levTarget.x - levRoot.x, levTarget.z - levRoot.z}) < 4.f)   // no strike to fall along: in front of its root
        want = glm::vec3{levRoot.x, levFloor + 1.6f, levRoot.z} + fwd * (levAtPool() ? LV_POOL + 3.f : 6.f);
    position += (want - position) * std::min(1.f, dt * 8.f);
    levPitch += (-0.1f - levPitch) * std::min(1.f, dt * 6.f);
    levJaw += (0.6f - levJaw) * std::min(1.f, dt * 4.f);
    animPhase += dt;
}

// Where its head is this tick, from what it's doing
inline void Enemy::levPose(float dt, const EnemyWorld& w) {
    (void)w;
    animPhase += dt;
    glm::vec3 up{0.f, 1.f, 0.f}, fwd{std::sin(yaw), 0.f, std::cos(yaw)};
    glm::vec3 base{levRoot.x, levFloor, levRoot.z};
    float sway = std::sin(animPhase * 0.7f) * 1.2f;
    glm::vec3 side{fwd.z, 0.f, -fwd.x};
    glm::vec3 idle = base + up * 11.f + fwd * 4.f + side * sway;
    glm::vec3 want = idle; float pitch = -0.15f, jaw = 0.1f, rate = 5.f;
    float tp = telegraphProgress();
    if (levStage == LevStage::RISE) {
        float t = levSmooth(1.f - levStageT / LV_RISE);
        position = glm::mix(base - up * 8.f, idle, t);
        levPitch = 0.6f * (1.f - t); levJaw = 0.5f * std::sin(t * 3.1416f);
        return;
    }
    if (levStage == LevStage::HIDDEN || attack == AttackKind::BREACH) {   // under the floor
        position = base - up * 8.f; levJaw = 0.f;
        return;
    }
    if (levBeached > 0.f) {   // lying where it struck
        position = levTarget + up * 1.6f; levPitch = -0.05f; levJaw = 0.25f;
        return;
    }
    if (levInhale > 0.f) {   // low over the edge of its root, the jaw unhinged
        want = base + fwd * (levAtPool() ? LV_POOL + 1.5f : 5.f) + up * 2.2f; pitch = 0.f; jaw = 1.f; rate = 6.f;
    } else switch (attack) {
        case AttackKind::CRASH: {   // rearing back, then down along the strip
            const float slam = 0.15f;
            glm::vec3 reared = base + up * 15.f - fwd * 3.f;
            if (tp < 1.f - slam) { want = glm::mix(idle, reared, levSmooth(tp / (1.f - slam))); pitch = 0.5f; jaw = 0.4f; rate = 7.f; }
            else {
                float s = (tp - (1.f - slam)) / slam;
                position = glm::mix(reared, levTarget + up * 1.6f, s * s);
                levPitch = glm::mix(0.5f, -0.05f, s); levJaw = 0.3f;
                return;
            }
            break;
        }
        case AttackKind::TORRENT: want = base + up * 8.f + fwd * (4.f + 3.f * tp); pitch = -0.3f; jaw = 0.2f + 0.8f * tp; break;
        case AttackKind::TIDE:    want = base + up * 12.f - fwd * 2.f; pitch = 0.3f; jaw = 0.5f * tp; break;
        case AttackKind::SPIT:    want = idle + up * 1.5f; pitch = 0.2f + 0.3f * tp; jaw = 0.6f * tp; break;
        case AttackKind::SWALLOW: want = base + fwd * (levAtPool() ? LV_POOL + 1.5f : 5.f) + up * (6.f - 3.8f * tp); pitch = 0.f; jaw = tp; rate = 4.f; break;
        case AttackKind::SUBMERGE: want = base - up * 8.f * tp; pitch = -0.5f; jaw = 0.f; rate = 4.f; break;
        default: break;
    }
    position += (want - position) * std::min(1.f, dt * rate);
    levPitch += (pitch - levPitch) * std::min(1.f, dt * 6.f);
    levJaw += (jaw - levJaw) * std::min(1.f, dt * 8.f);
}

// ---- its body: segment centres from the root up to behind the head ----
static constexpr int LV_SEGMENTS = 14;
struct LevSegment { glm::vec3 c; float r; };
inline void leviathanBody(const Enemy& e, LevSegment out[LV_SEGMENTS]) {
    glm::vec3 up{0.f, 1.f, 0.f}, fwd{std::sin(e.yaw), 0.f, std::cos(e.yaw)};
    glm::vec3 base{e.levRoot.x, e.levFloor, e.levRoot.z};
    float reach = glm::length(glm::vec2{e.position.x - base.x, e.position.z - base.z});
    glm::vec3 p0 = base - up * 6.f, p1 = base + up * std::max(6.f, 4.f + reach * 0.35f);
    glm::vec3 p3 = e.position - fwd * 2.4f, p2 = p3 - fwd * 5.f + up * 3.f;
    for (int i = 0; i < LV_SEGMENTS; ++i) {
        float t = (i + 0.5f) / LV_SEGMENTS, u = 1.f - t;
        out[i].c = u * u * u * p0 + 3.f * u * u * t * p1 + 3.f * u * t * t * p2 + t * t * t * p3;
        out[i].r = 2.5f - 0.8f * t;
    }
}
// The eye on its brow, its throat behind the open jaw
inline AABB leviathanEye(const Enemy& e) {
    glm::vec3 fwd{std::sin(e.yaw), 0.f, std::cos(e.yaw)};
    glm::vec3 c = e.position + fwd * 2.1f + glm::vec3{0.f, 0.7f + e.levPitch * 1.5f, 0.f};
    glm::vec3 h{0.75f};
    return {c - h, c + h};
}
inline AABB leviathanThroat(const Enemy& e) {
    glm::vec3 fwd{std::sin(e.yaw), 0.f, std::cos(e.yaw)};
    glm::vec3 c = e.position + fwd * 1.6f - glm::vec3{0.f, 0.9f, 0.f};
    glm::vec3 h{1.0f};
    return {c - h, c + h};
}

// What a ray along d from o hits on it first: 0 nothing, 1 body, 2 head,
// 3 eye, 4 throat (only from in front while it inhales); t: where
enum class LevZone { NONE, BODY, HEAD, EYE, THROAT };
inline LevZone leviathanRay(const Enemy& e, glm::vec3 o, glm::vec3 d, float& t) {
    t = 1e9f; LevZone z = LevZone::NONE;
    auto test = [&](const AABB& b, LevZone k, float bias = 0.f) {
        float h = rayBoxHit(o, d, b);
        if (h > 0.f && h - bias < t) { t = h; z = k; }
    };
    LevSegment seg[LV_SEGMENTS];
    leviathanBody(e, seg);
    for (const auto& s : seg)
        if (s.c.y > e.levFloor - 2.f) test({s.c - glm::vec3{s.r * 0.8f}, s.c + glm::vec3{s.r * 0.8f}}, LevZone::BODY);
    test(e.getAABB(), LevZone::HEAD, 0.3f);
    test(leviathanEye(e), LevZone::EYE, 0.6f);   // the eye wins a near-tie with the head it sits on
    if (e.levInhale > 0.f && glm::dot(glm::vec2{d.x, d.z}, glm::vec2{std::sin(e.yaw), std::cos(e.yaw)}) < 0.f)
        test(leviathanThroat(e), LevZone::THROAT, 1.0f);
    return z;
}
// The damage multiplier for a zone (before armorMult's x2 while staggered)
inline float leviathanZoneMult(const Enemy& e, LevZone z) {
    switch (z) {
        case LevZone::BODY:   return 0.2f;
        case LevZone::EYE:    return e.levEyeMult();
        case LevZone::THROAT: return e.levInhale > 0.f ? 3.f : 0.f;
        case LevZone::HEAD:   return 1.f;
        default:              return 0.f;
    }
}
