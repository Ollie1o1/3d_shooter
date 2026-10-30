#pragma once
#include "bosses/ballast/BallastHazards.h"
#include "bosses/absolute/AbsoluteHazards.h"   // THE ABSOLUTE's hazards (and the sword's block)
// A scripted player against a boss's real AI, in its real arena: the boss's
// events resolved the way the game resolves them (melee, slams you can jump,
// projectiles, the Sovereign's blades, the Warden's hazards and conduits).
// CORNER/PERCH/KITE/RANGED are cheese strategies (they never shoot); SOLID
// circles at 10 m, shoots at a fixed effective DPS, cuts conduits first, hits
// open cores, and parries every other parry window.
struct BossSim {
    enum Policy { CORNER, PERCH, KITE, RANGED, SOLID, DODGER, STILL };   // DODGER: SOLID's feet, guns only, no parries; STILL never moves
    static constexpr float SOLID_DPS = 55.f;   // a solid player's effective damage a second (misses and reloads included)
    const LevelData& L; const SpatialGrid& grid; const Arena& A; Policy policy;
    Enemy boss; ProjectileSystem proj; SovereignHazards sov; WardenHazards ward; ConduitClock clock; LeviathanHazards lev;
    BallastHazards ball;
    std::vector<Enemy> others;
    LevelData lv; ArenaShifts rings;            // the reactor's rings, driven by the Warden as in the game
    glm::vec3 feet{0.f}; float damageTaken = 0.f, t = 0.f, kite = 0.f; int conduits = 4; float conduitHp = 250.f; bool parryNext = true;
    static constexpr float REACT = 0.3f, AFTER = 0.4f;   // SOLID: reacts to a tell after 0.3 s, dodging (not shooting) until 0.4 s after it lands
    float dodgeFrom = -1.f, dodgeUntil = -1.f; bool wasTelegraphing = false;
    glm::vec3 vel{0.f};   // the player's velocity (enemies lead their shots with it, as in the game)
    bool prevWindow = false;
    // THE ABSOLUTE (the Zenith): his hazards, as Gameplay_Absolute.h resolves them
    AbsoluteHazards abs;
    std::vector<AABB> pillarsUp;
    float dmgMelee = 0.f, dmgBeam = 0.f, dmgGround = 0.f, dmgWhite = 0.f, dmgShot = 0.f;   // where the hurt came from (tuning)
    float formAt = -1.f, lowest2 = 100.f, taken2 = 0.f;
    float msBy[5] = {};   // MORNINGSTAR's damage by source (AbsoluteHazards::Hit::src; 0: his body)   // MORNINGSTAR: when his second form began; the lowest health and the damage taken in it
    float health = 100.f, lowest = 100.f;   // SOLID's health with the sword's healing (6 a hit, 10 a parry), capped at 100
    int dashes = 2;   // two dash charges, 1.2 s each to come back (as the game)
    int parries = 0; float dashCd = 0.f; bool sliding = false, airborne = false, blocking = false;
    bool zenith() const { return boss.type == EnemyType::ABSOLUTE && L.zenith.built; }

    BossSim(const LevelData& l, const SpatialGrid& g, const Arena& a, EnemyType type, Policy p)
        : L(l), grid(g), A(a), policy(p), boss(type, a.bossSpawn), lv(l) {
        rings.capture(lv); rings.reset(lv);
        boss.spawnTimer = 0.f; boss.state = EnemyState::ACTIVE;
        boss.maxHealth *= difficulty(DIFFICULTY_DEFAULT).health; boss.health = boss.maxHealth;
        proj.floorY = L.lowestFloor();
        feet = a.playerStart;
        if (type == EnemyType::LEVIATHAN && l.hasLair) feet.y = l.lairFloor;   // (the Maw's start is in the air)
        if (p == CORNER) {   // the arena's corner, standing on whatever is there
            feet = glm::vec3{a.bounds.min.x + 2.f, feet.y, a.bounds.min.z + 2.f};
            for (auto& w : L.walls)
                if (!w.dynamic && feet.x > w.box.min.x && feet.x < w.box.max.x && feet.z > w.box.min.z && feet.z < w.box.max.z &&
                    w.box.max.y < a.bounds.max.y - 2.f && w.box.max.y > feet.y) feet.y = w.box.max.y;
        }
        if (p == RANGED) {   // already holding its distance: 30 m out, toward where the fight came in
            glm::vec3 d{a.playerStart.x - boss.position.x, 0.f, a.playerStart.z - boss.position.z};
            float l = glm::length(d);
            feet = glm::vec3{boss.position.x, feet.y, boss.position.z} + (l > 0.1f ? d / l : glm::vec3{0, 0, 1}) * 30.f;
        }
        if (type == EnemyType::ABSOLUTE && l.zenith.built) {   // the Zenith's cheeses: its rim, a pillar's top (PERCH below)
            glm::vec3 c = l.zenith.centre;
            if (p == CORNER) feet = c + glm::vec3{-30.f, 0.f, 30.f};   // r 42.4: the rim
            if (p == RANGED) feet = c + glm::vec3{0.f, 0.f, 30.f};
            if (p == SOLID || p == DODGER || p == STILL) feet = c + glm::vec3{0.f, 0.f, 14.f};
        }
        if (p == PERCH) {   // the highest walkable top inside the arena
            float best = -1e9f;
            for (auto& w : L.walls) {
                const AABB& b = w.box;
                if (w.dynamic || b.max.x - b.min.x < 1.5f || b.max.z - b.min.z < 1.5f) continue;
                glm::vec3 c{(b.min.x + b.max.x) * 0.5f, b.max.y, (b.min.z + b.max.z) * 0.5f};
                if (c.x < a.bounds.min.x || c.x > a.bounds.max.x || c.z < a.bounds.min.z || c.z > a.bounds.max.z || c.y > a.bounds.max.y - 2.f) continue;
                if (c.y > best) { best = c.y; feet = c; }
            }
        }
    }
    bool bossAlive() const { return boss.alive; }
    float groundY() const { return boss.type == EnemyType::LEVIATHAN && L.hasLair ? L.lairFloor : A.playerStart.y; }
    EnemyWorld world() const {
        EnemyWorld w;
        w.playerFeet = feet; w.playerEye = feet + glm::vec3{0, 1.7f, 0};
        w.walls = L.walls.data(); w.wallCount = (int)L.walls.size(); w.grid = &grid; w.bounds = A.bounds;
        w.reactor = L.reactorPos; w.hasReactor = L.hasReactor; w.playerVel = vel;
        w.hasLair = L.hasLair; w.lair = L.lair; w.lairFloor = L.lairFloor; w.wells = L.wells.data(); w.wellCount = (int)L.wells.size();
        if (boss.type == EnemyType::ABSOLUTE && lv.zenith.built) {
            w.hasZenith = true; w.zenCentre = lv.zenith.centre; w.zenFloor = lv.zenith.centre.y;
            w.zenPillars = pillarsUp.data(); w.zenPillarCount = (int)pillarsUp.size(); w.onShard = lv.onZenithShard(feet);
        }
        return w;
    }
    void hurt(float d) { damageTaken += d * A.damageScale * difficulty(DIFFICULTY_DEFAULT).damage; }
    void movePlayer(float dt) {
        glm::vec3 c = (A.bounds.min + A.bounds.max) * 0.5f;
        if (policy == KITE) {   // round the arena's edge, 2 m in, at a run
            kite += dt * 7.f;
            float w = A.bounds.max.x - A.bounds.min.x - 4.f, d = A.bounds.max.z - A.bounds.min.z - 4.f, per = 2.f * (w + d);
            float s = std::fmod(kite, per);
            glm::vec3 o{A.bounds.min.x + 2.f, groundY(), A.bounds.min.z + 2.f};
            if (s < w) feet = o + glm::vec3{s, 0, 0};
            else if (s < w + d) feet = o + glm::vec3{w, 0, s - w};
            else if (s < 2 * w + d) feet = o + glm::vec3{w - (s - w - d), 0, d};
            else feet = o + glm::vec3{0, 0, d - (s - 2 * w - d)};
        }
        if (policy == SOLID && boss.type == EnemyType::LEVIATHAN) { leviathanMove(dt); return; }
        if (policy == RANGED || policy == SOLID) {
            float want = policy == RANGED ? 30.f : 10.f;
            glm::vec3 bc = boss.type == EnemyType::LEVIATHAN ? boss.levRoot : boss.position;   // (the Leviathan's head sways; its root stays put)
            glm::vec3 from{feet.x - bc.x, 0.f, feet.z - bc.z};
            float l = glm::length(from);
            glm::vec3 dir = l > 0.1f ? from / l : glm::vec3{1, 0, 0};
            glm::vec3 side{-dir.z, 0.f, dir.x};
            glm::vec3 goal = glm::vec3{bc.x, groundY(), bc.z} + (policy == RANGED && boss.type == EnemyType::LEVIATHAN ? dir : glm::normalize(dir + side * 0.3f)) * want;
            goal.x = glm::clamp(goal.x, A.bounds.min.x + 1.5f, A.bounds.max.x - 1.5f);
            goal.z = glm::clamp(goal.z, A.bounds.min.z + 1.5f, A.bounds.max.z - 1.5f);
            glm::vec3 mv = goal - feet; mv.y = 0.f;
            float ml = glm::length(mv);
            if (ml > 0.05f) feet += mv / ml * std::min(ml, 8.f * dt);
            feet.y = groundY();
        }
        (void)c;
    }
    void shoot(float dt) {
        if (policy != SOLID || !boss.targetable()) return;
        if (t >= dodgeFrom && t < dodgeUntil) return;   // busy getting out of the way
        float dmg = SOLID_DPS * dt;
        if (boss.type == EnemyType::WARDEN && boss.wardenPhase == 1 && conduits > 0 && !boss.coreOpen()) {   // cut the conduits first
            conduitHp -= dmg;
            if (conduitHp <= 0.f) { --conduits; conduitHp = 250.f; if (conduits == 0) { boss.stagger(4.f); clock.allCut(); } }
            return;
        }
        if (boss.type == EnemyType::LEVIATHAN) {   // the throat while it inhales, the eye when it's open, else the head (some plates)
            float m = boss.levInhale > 0.f ? 3.f : boss.levEyeOpen() ? 0.4f + 0.6f * boss.levEyeMult() : 0.85f;
            float hit = dmg * m * boss.armorMult();
            if (boss.levInhale > 0.f) boss.levThroatHit(hit);
            boss.takeDamage(hit);
            return;
        }
        float m = boss.armorMult();
        // SOLID aims at exposed machinery rather than gaining the old whole-body bonus.
        if(!boss.staggered()&&((boss.type==EnemyType::SORTER&&boss.sortOpen>0)||(boss.type==EnemyType::BALLAST&&boss.ballOpen>0)))m*=1.6f;
        if (boss.coreOpen()) m = std::max(m, 1.f) * (0.4f + 0.6f * boss.woundMult());   // most shots into the open core
        boss.takeDamage(dmg * m);
    }
    // THE LEVIATHAN: its events resolved as Gameplay_Leviathan.h does
    float lvFloor() const { return L.hasLair ? L.lairFloor : A.playerStart.y; }
    bool jumping = false;   // SOLID: in the air over a tide right now
    float circle = 1.f;     // SOLID: which way it circles the Leviathan
    void leviathanEvents(float dt, const EnemyEvents& ev) {
        if (ev.lvCrash) { lev.landCrash(); if (LeviathanHazards::inStrip(ev.lvFrom, ev.lvTo, feet, lvFloor())) hurt(LeviathanHazards::CRASH_DAMAGE); }
        if (ev.lvCrashMark) lev.markCrash(ev.lvFrom, ev.lvMarkTo);
        if (ev.lvTide) lev.addTide(ev.lvTideAt, boss.levAtPool() ? Enemy::LV_POOL : 3.5f, ev.lvTideR);
        if (ev.lvSpit) lev.addSpit(ev.lvSpitAt);
        if (ev.lvBreachTell) lev.addBoil(ev.lvSite);
        if (ev.lvBreach && LeviathanHazards::breachHits(ev.lvSite, feet)) hurt(LeviathanHazards::BREACH_DAMAGE);
        if (boss.staggered() || boss.attack != AttackKind::CRASH) lev.dropMarks();
        if (boss.levInhale > 0.f) {
            glm::vec3 m = boss.levMouth();
            feet += LeviathanHazards::pull(m, feet) * dt;
            if (LeviathanHazards::bites(m, feet)) { hurt(LeviathanHazards::BITE_DAMAGE); boss.levEndInhale(); }
        }
        glm::vec3 f = feet;
        jumping = false;
        if (policy == SOLID)   // it jumps a tide as it reaches it
            for (auto& t : lev.tides) { float d = glm::length(glm::vec2{feet.x - t.centre.x, feet.z - t.centre.z}); if (d > t.radius - 0.5f && d < t.radius + 3.f) jumping = true; }
        if (jumping) f.y += 1.6f;
        for (auto& h : lev.update(dt, f, lvFloor())) hurt(h.damage);
    }
    // SOLID against THE LEVIATHAN: circles its root at ~22 m; reads a crash's
    // strip and steps out of it, keeps off a spit's marker and a boiling well,
    // runs out of an inhale
    void leviathanMove(float dt) {
        glm::vec3 root{boss.levRoot.x, feet.y, boss.levRoot.z};
        glm::vec3 from = feet - root; from.y = 0.f;
        float l = glm::length(from);
        glm::vec3 dir = l > 0.1f ? from / l : glm::vec3{0, 0, 1}, side{-dir.z, 0.f, dir.x};
        if (boss.ev.telegraphStarted && boss.attack == AttackKind::TORRENT) circle = -circle;   // orbs coming: change direction, as a player would
        glm::vec3 mv = side * (0.6f * circle) + dir * glm::clamp((22.f - l) * 0.3f, -1.f, 1.f);
        if (t >= dodgeFrom) {
            for (auto& s : lev.strips)
                if (!s.landed) {   // well clear of it (2 m past its edge), not just out
                    glm::vec2 d{s.to.x - s.from.x, s.to.z - s.from.z}; d = glm::normalize(d);
                    glm::vec2 r{feet.x - s.from.x, feet.z - s.from.z};
                    float cr = r.x * d.y - r.y * d.x, along = glm::dot(r, d);
                    if (std::fabs(cr) < LeviathanHazards::STRIP_HALF + 2.f && along > -2.f && along < glm::length(glm::vec2{s.to.x - s.from.x, s.to.z - s.from.z}) + LeviathanHazards::STRIP_PAST + 2.f)
                        mv = glm::vec3{d.y, 0.f, -d.x} * (cr >= 0.f ? 1.f : -1.f) * 3.f;
                }
            for (auto& s : lev.spits) if (!s.burst) { glm::vec3 a = feet - s.at; a.y = 0.f; if (glm::length(a) < LeviathanHazards::SPIT_RADIUS + 1.f) mv = glm::normalize(a + glm::vec3{0.01f, 0, 0}) * 3.f; }
            for (auto& b : lev.boils) { glm::vec3 a = feet - b.at; a.y = 0.f; if (glm::length(a) < LeviathanHazards::BREACH_RADIUS + 1.f) mv = glm::normalize(a + glm::vec3{0.01f, 0, 0}) * 3.f; }
        }
        if (boss.levInhale > 0.f || (boss.attack == AttackKind::SWALLOW && t >= dodgeFrom)) mv = dir * 3.f;   // the jaw unhinges: run
        float ml = glm::length(mv);
        if (ml > 1e-3f) feet += mv / ml * std::min(8.f, 8.f * ml) * dt;
        glm::vec3 c{L.lair.x, feet.y, L.lair.z}; glm::vec3 o = feet - c; o.y = 0.f;   // stay on the ring
        float ol = glm::length(o);
        if (ol > 44.f) feet = c + o / ol * 44.f;
        if (ol < Enemy::LV_POOL + 1.f && ol > 0.01f) feet = c + o / ol * (Enemy::LV_POOL + 1.f);
        feet.y = groundY();
    }
    void step(float dt) {
        if(boss.type==EnemyType::BALLAST){ballastStep(dt);return;}
        if(boss.type==EnemyType::SORTER){sorterStep(dt);return;}
        if (zenith()) { absoluteStep(dt); return; }
        t += dt;
        glm::vec3 was = feet;
        movePlayer(dt);
        vel = (feet - was) / dt;
        if (boss.type == EnemyType::WARDEN) {
            boss.conduitsLeft = boss.wardenPhase == 1 ? conduits : 0;
            if (boss.wardenPhase == 1 && clock.running()) { int want = clock.update(dt); if (conduits < want) conduits = want; if (conduits >= 4) clock.reset(); }
        }
        EnemyWorld w = world();
        boss.update(dt, w);
        const EnemyEvents& ev = boss.ev;
        // SOLID dodges every tell it sees: from REACT after it starts until AFTER past when it lands
        bool tel = boss.telegraphTimer > 0.f;
        if (ev.telegraphStarted) { dodgeFrom = t + REACT; dodgeUntil = 1e9f; }
        if (wasTelegraphing && !tel) dodgeUntil = t + AFTER;
        wasTelegraphing = tel;
        if (boss.type == EnemyType::WARDEN && boss.wardenPhase >= 2) {
            glm::vec3 c = L.hasReactor ? L.reactorPos : boss.position; c.y = boss.floorY;
            rings.bossPulse(boss.wardenPhase == 2 ? 3.5f : 5.f, c);
        }
        rings.update(dt, lv, -1, true);
        if (rings.ringHits(feet)) hurt(ArenaShifts::PULSE_DAMAGE);
        glm::vec3 bp = boss.position;
        float flat = glm::length(glm::vec2(feet.x - bp.x, feet.z - bp.z));
        bool grounded = feet.y < bp.y + 0.9f && feet.y > bp.y - 1.5f;
        if (ev.meleeHit) hurt(ev.meleeDamage);
        if (ev.slam && flat < ev.slamRadius && grounded) hurt(ev.slamDamage);
        for (int k = 0; k < ev.shots; ++k)
            if (Projectile* pr = proj.fire(ev.shotOrigin, ev.shotDir[k] * ev.shotSpeed, ev.shotDamage, false, {1, 1, 1}, false, 0.f, ev.shotSize, ev.shotHeavy))
                pr->owner = boss.uid;
        auto r = proj.update(dt, L.walls.data(), (int)L.walls.size(), others, feet + glm::vec3{0, 1.7f, 0}, &grid);   // as the game: the eye
        if (r.hitPlayer) hurt(r.playerDamage);
        for (int k = 0; k < ev.strikes; ++k) sov.addStrike(ev.strikePos[k], ev.strikeKind[k], ev.strikeDelay[k]);
        for (int k = 0; k < ev.phantoms; ++k) sov.addPhantom(ev.phantomPos[k], ev.phantomDir[k]);
        for (auto& h : sov.update(dt, feet)) hurt(h.damage);
        if (ev.wLance) ward.addLance(ev.wLanceFrom, ev.wLanceYaw, ev.wLanceSign);
        if (ev.wSeeker) ward.addSeeker(ev.wSeekerAt);
        if (ev.wDetonate) hurt(WardenHazards::DETONATE_DAMAGE);
        for (auto& h : ward.update(dt, feet, 1.8f, L.walls.data(), (int)L.walls.size(), bp, boss.ventTimer > 0.f)) hurt(h.damage);
        if (boss.type == EnemyType::LEVIATHAN) leviathanEvents(dt, ev);
        // SOLID parries every other window it sees open
        bool window = boss.parryWindow();
        if (policy == SOLID && window && !prevWindow) { if (parryNext) boss.stagger(boss.staggerTime()); parryNext = !parryNext; }
        prevWindow = window;
        shoot(dt);
    }

    // Ballast: shared AI and crosswind geometry, real projectile collisions.
    // SOLID reads committed lanes and answers alternating white cable weights.
    void ballastStep(float dt) {
        t+=dt;glm::vec3 was=feet;movePlayer(dt);
        if(policy==SOLID && boss.telegraphTimer>0 && t>=dodgeFrom) {
            glm::vec3 d=boss.ballAim-boss.ballFrom;d.y=0;float n=glm::length(d);
            if(n>.01f && (boss.attack==AttackKind::BALL_GUST||boss.attack==AttackKind::BALL_GRADE)) {
                d/=n;glm::vec3 side{-d.z,0,d.x};
                float sign=glm::dot(feet-boss.ballFrom,side)>=0?1.f:-1.f;
                feet+=side*sign*8.f*dt;
            }
            if((boss.attack==AttackKind::BALL_ANCHOR||boss.attack==AttackKind::BALL_WINCH) && !parryNext) {
                glm::vec3 out=feet-boss.ballAim;out.y=0;
                if(glm::length(out)<.1f)out={1,0,0};
                feet+=glm::normalize(out)*8.f*dt;
            }
        }
        if(policy==SOLID&&boss.ballDrive>0) {
            glm::vec3 d=boss.ballAim-boss.ballFrom;d.y=0;
            if(glm::length(d)>.01f) {
                d=glm::normalize(d);glm::vec3 side{-d.z,0,d.x};
                if(BallastHazards::across(boss.ballFrom,d,feet)<4.2f) {
                    float sign=glm::dot(feet-boss.ballFrom,side)>=0?1.f:-1.f;
                    feet+=side*sign*8.f*dt; // keep clear until the visible blade pass ends
                }
            }
        }
        if(policy==SOLID)for(const auto& f:ball.fronts) {
            float a=BallastHazards::along(f.from,f.dir,feet)-f.distance;
            if(a>0&&a<10.f&&BallastHazards::across(f.from,f.dir,feet)<BallastHazards::HALF_WIDTH+1.f) {
                glm::vec3 side{-f.dir.z,0,f.dir.x};float sign=glm::dot(feet-f.from,side)>=0?1.f:-1.f;
                feet+=side*sign*8.f*dt;
            }
        }
        feet.x=glm::clamp(feet.x,A.bounds.min.x+.6f,A.bounds.max.x-.6f);
        feet.z=glm::clamp(feet.z,A.bounds.min.z+.6f,A.bounds.max.z-.6f);
        bool window=boss.parryWindow();
        if(policy==SOLID&&window&&!prevWindow&&glm::length(feet-boss.ballAim)<Enemy::BALL_ANCHOR_R+1.f) {
            if(parryNext){boss.stagger(boss.staggerTime());++parries;}
            parryNext=!parryNext;
        }
        prevWindow=window;vel=(feet-was)/dt;boss.update(dt,world());const auto& ev=boss.ev;
        if(ev.telegraphStarted){dodgeFrom=t+REACT;dodgeUntil=t+boss.telegraphDuration+AFTER;}
        if(ev.meleeHit)hurt(ev.meleeDamage);
        if(ev.ballGust)ball.add(ev.ballFrom,ev.ballTo);
        if(ev.ballRip)ball.addRip(ev.ballFrom,ev.ballTo);
        if(ev.ballYank){auto pull=boss.position-feet;pull.y=0;if(glm::length(pull)>1)feet+=glm::normalize(pull)*6.f*dt;}
        for(const auto& hit:ball.update(dt,feet,L.walls))hurt(hit.damage);
        for(int k=0;k<ev.shots;++k)proj.fire(ev.shotOrigin,ev.shotDir[k]*ev.shotSpeed,ev.shotDamage,false,{1,1,1},false,0,ev.shotSize,true);
        auto hit=proj.update(dt,L.walls.data(),(int)L.walls.size(),others,feet+glm::vec3{0,1.7f,0},&grid);
        if(hit.hitPlayer)hurt(hit.playerDamage);
        if(policy==SOLID)shoot(dt);
    }

    // THE SORTER: the actual AI and projectile collisions; SOLID reads its locked
    // floor target after REACT and answers alternating claw windows with a parry.
    void sorterStep(float dt) {
        t+=dt;glm::vec3 was=feet;
        movePlayer(dt);
        if(policy==SOLID && boss.telegraphTimer>0 && t>dodgeFrom &&
           (boss.attack==AttackKind::SORT_STAMP || boss.attack==AttackKind::SORT_GRAB)) {
            glm::vec3 out=feet-boss.sortAim;out.y=0;
            if(glm::length(out)<.1f)out={1,0,0};
            feet+=glm::normalize(out)*8.f*dt;
        }
        feet.x=glm::clamp(feet.x,A.bounds.min.x+.6f,A.bounds.max.x-.6f);
        feet.z=glm::clamp(feet.z,A.bounds.min.z+.6f,A.bounds.max.z-.6f);
        vel=(feet-was)/dt;
        bool window=boss.parryWindow();
        if(policy==SOLID && window && !prevWindow && glm::length(feet-boss.sortAim)<Enemy::SORT_GRAB_R+1.f) {
            if(parryNext){boss.stagger(boss.staggerTime());boss.sortMark=0;++parries;}
            parryNext=!parryNext;
        }
        prevWindow=window;boss.update(dt,world());const auto& ev=boss.ev;
        if(ev.telegraphStarted){dodgeFrom=t+REACT;dodgeUntil=t+.8f;}
        if(ev.meleeHit)hurt(ev.meleeDamage);
        for(int k=0;k<ev.shots;++k)proj.fire(ev.shotOrigin,ev.shotDir[k]*ev.shotSpeed,ev.shotDamage,false,{1,1,1},false,0,ev.shotSize,true);
        auto hit=proj.update(dt,L.walls.data(),(int)L.walls.size(),others,feet+glm::vec3{0,1.7f,0},&grid);
        if(hit.hitPlayer)hurt(hit.playerDamage);
        if(policy==SOLID)shoot(dt);
    }

    // ---- THE ABSOLUTE --------------------------------------------------------
    float zGround(glm::vec3 p) const { return lv.groundAt(p.x, p.z, p.y + 0.5f); }
    // SOLID and DODGER: hold their range, read each tell and answer it
    void absoluteMove(float dt) {
        const glm::vec3 c = lv.zenith.centre;
        if (policy == STILL || policy == CORNER || policy == PERCH) { feet.y = zGround(feet + glm::vec3{0.f, 0.3f, 0.f}); return; }
        if (policy == KITE) {   // round the floor's edge at a run
            kite += dt * 7.f / 40.f;
            feet = c + glm::vec3{std::cos(kite) * 40.f, 0.f, std::sin(kite) * 40.f};
            feet.y = zGround(feet + glm::vec3{0.f, 1.f, 0.f});
            return;
        }
        const glm::vec3 start = feet;   // where this tick began (a step onto no floor is taken back, below)
        glm::vec3 bp{boss.position.x, feet.y, boss.position.z};
        glm::vec3 from = feet - bp; from.y = 0.f;
        float d = glm::length(from);
        glm::vec3 dir = d > 0.1f ? from / d : glm::vec3{0.f, 0.f, 1.f}, side{-dir.z, 0.f, dir.x};
        const bool flight = boss.abAirborne() && boss.attack != AttackKind::ATHUNDER;   // (THUNDERCLAP's hover: stay in and jump the flame)
        float want = policy == RANGED ? 30.f : policy == DODGER || flight ? (policy == DODGER ? 12.f : 10.f) : 2.4f;   // (aloft: SOLID stands off and shoots)
        glm::vec3 mv = side * 0.5f + dir * glm::clamp((want - d) * 0.6f, -1.f, 1.f);
        if (policy == RANGED) mv = dir * glm::clamp((want - d) * 0.6f, -1.f, 1.f);
        sliding = !abs.halos.empty() && policy != RANGED;
        airborne = false; blocking = false;
        if (policy == SOLID || policy == DODGER) {
            bool reacting = t >= dodgeFrom;
            AbTell tell = boss.telegraphTimer > 0.f ? abTellOf(boss.attack) : AbTell::NONE;
            if (reacting && (boss.attack == AttackKind::ARAY || !abs.rays.empty())) mv = side * 2.f;   // strafe out of the ray
            if (!abs.zrays.empty()) { glm::vec3 a = feet - abs.zrays.front().at; a.y = 0.f; if (glm::length(a) > 0.01f) mv = glm::normalize(a) * 2.f + side; }
            for (auto& sp : abs.serpents) if (!sp.back && policy == DODGER) mv = glm::normalize(glm::cross(sp.dir, glm::vec3{0, 1, 0}) + glm::vec3{1e-4f}) * 2.f;
            for (auto& st : abs.stomps) { float r = glm::length(glm::vec2{feet.x - st.at.x, feet.z - st.at.z}); if (r > st.r - 2.5f && r < st.r + 0.8f) airborne = true; }
            for (auto& fn : abs.fans) for (int k = 0; k < AbsoluteHazards::FAN_N; ++k)   // jump a flame as it reaches you
                if (!fn.out[k]) { glm::vec3 p = AbsoluteHazards::fanFlame(fn, k, &lv); if (glm::length(glm::vec2{feet.x - p.x, feet.z - p.z}) < 2.5f) airborne = true; }
            if (reacting && boss.attack == AttackKind::ASTOMP) { float r = glm::length(glm::vec2{feet.x - boss.position.x, feet.z - boss.position.z}); if (r < 2.f) airborne = true; }
            if (!abs.seams.empty()) {   // stand in a cell that isn't burning next, near its edge
                auto& sm = abs.seams.front();
                int safe = sm.done0 ? sm.first : (sm.first ^ 1);
                glm::vec3 best = feet; float bd = 1e9f;
                for (int dx = -1; dx <= 1; ++dx) for (int dz = -1; dz <= 1; ++dz) {
                    glm::vec3 cc = feet + glm::vec3{dx * 6.f, 0.f, dz * 6.f};
                    if (AbsoluteHazards::seamParity(c, cc) != safe) continue;
                    float cx = std::floor((cc.x - c.x) / 6.f) * 6.f + c.x, cz = std::floor((cc.z - c.z) / 6.f) * 6.f + c.z;
                    glm::vec3 q{glm::clamp(feet.x, cx + 0.6f, cx + 5.4f), feet.y, glm::clamp(feet.z, cz + 0.6f, cz + 5.4f)};
                    float qd = glm::length(q - feet);
                    if (qd < bd && glm::length(glm::vec2{q.x - c.x, q.z - c.z}) < 41.f) { bd = qd; best = q; }
                }
                if (bd > 0.05f) mv = (best - feet) / std::max(bd, 1e-3f) * 2.f;
                else mv = glm::vec3{0.f};
            }
            if (!abs.judges.empty()) mv = d > 3.f ? -dir * 2.f : glm::vec3{0.f};   // be beside him
            if (reacting && boss.attack == AttackKind::ACLAP && d < Enemy::AB_CLAP_R + 1.f) mv = side * 2.f;   // step out of the clap's cone
            if (boss.attack == AttackKind::ASKYFALL && boss.telegraphTimer < 0.25f && reacting && dashes > 0 &&
                glm::length(glm::vec2{feet.x - boss.abAim.x, feet.z - boss.abAim.z}) < Enemy::AB_SKY_R + 0.5f) {   // out of the circle
                glm::vec3 out = feet - boss.abAim; out.y = 0.f;
                feet += (glm::length(out) > 0.1f ? glm::normalize(out) : side) * 5.f; if (dashes-- == 2) dashCd = 1.2f;
            }
            if (boss.morning()) {   // MORNINGSTAR: read the floor
                auto flat = [](glm::vec3 a, glm::vec3 b) { return glm::length(glm::vec2{a.x - b.x, a.z - b.z}); };
                for (auto& w : abs.waves) {   // jump a ring as it reaches you
                    if (w.delay > 0.f) continue;
                    float r = flat(feet, w.at);
                    if (w.inward ? (r < w.r + 2.5f && r > w.r - 0.8f) : (r > w.r - 2.5f && r < w.r + 0.8f)) airborne = true;
                }
                if (reacting && boss.attack == AttackKind::MSUNDER && boss.telegraphTimer < 0.15f) {   // his fists landing near you: be in the air
                    glm::vec3 f{std::sin(boss.yaw), 0.f, std::cos(boss.yaw)};
                    if (flat(feet, boss.position + f * (1.4f * boss.scale)) < 3.f) airborne = true;
                }
                auto offLine = [&](glm::vec3 from, glm::vec3 dir, float len) {   // on a line: step off it, sideways
                    glm::vec3 rel = feet - from; rel.y = 0.f;
                    float along = glm::dot(rel, dir);
                    glm::vec3 s2{-dir.z, 0.f, dir.x};
                    float across = glm::dot(rel, s2);
                    if (along > -1.f && along < len && std::fabs(across) < AbsoluteHazards::FIS_HALF + 1.2f) mv = s2 * (across >= 0.f ? 2.f : -2.f);
                };
                for (auto& f : abs.fissures) offLine(f.from, f.dir, f.len);
                if (reacting && boss.attack == AttackKind::MFISSURE && boss.telegraphProgress() > 0.6f) {
                    glm::vec3 l = boss.abAim - boss.position; l.y = 0.f;
                    if (glm::length(l) > 0.1f) offLine(boss.position, glm::normalize(l), 44.f);
                }
                for (auto& L : abs.lanes) {   // in a lit lane: to the gap beside it
                    if (AbsoluteHazards::laneFront(L) > glm::dot(feet - L.centre, L.dir) + 1.5f) continue;   // (past you)
                    float across = glm::dot(feet - L.centre, L.side);
                    for (int k = 0; k < AbsoluteHazards::LANE_N; ++k)
                        if (std::fabs(across - L.off[k]) < AbsoluteHazards::LANE_HALF + 0.8f) mv = L.side * (across >= L.off[k] ? 2.f : -2.f);
                }
                if (reacting && (boss.attack == AttackKind::MGRASP || boss.attack == AttackKind::MDROP)) {   // out of the hand's circle, the shadow
                    float R = boss.attack == AttackKind::MGRASP ? Enemy::MS_GRASP_R : Enemy::MS_DROP_R, dd = flat(feet, boss.abAim);
                    glm::vec3 out = feet - boss.abAim; out.y = 0.f;
                    out = glm::length(out) > 0.1f ? glm::normalize(out) : side;
                    if (dd < R + 1.5f) mv = out * 2.f;
                    if (dd < R + 0.5f && boss.telegraphTimer < 0.3f && dashes > 0) { feet += out * 5.f; if (dashes-- == 2) dashCd = 1.2f; }
                }
            }
            // gold (and the white strikes it won't parry): a dash out of reach as it lands, else block it
            bool strike = tell == AbTell::GOLD || (tell == AbTell::WHITE && Enemy::abMeleeWhite(boss.attack));   // (a parried one never lands: he's staggered)
            float sd = boss.attack == AttackKind::AMETEOR ? glm::length(boss.position - feet) : d;
            const bool floorRead = boss.attack == AttackKind::ASKYFALL || boss.attack == AttackKind::MGRASP || boss.attack == AttackKind::MDROP;
            if (strike && !floorRead && boss.telegraphTimer < 0.22f && sd < (boss.morning() ? 7.5f : 6.f)) {
                if (dashes > 0) { feet += side * 5.f; if (dashes-- == 2) dashCd = 1.2f; }
                else if (policy == SOLID) blocking = true;   // the sword up: a white strike or a gold one, from the front
            }
        }
        if (dashes < 2) { dashCd -= dt; if (dashCd <= 0.f) { ++dashes; dashCd = dashes < 2 ? 1.2f : 0.f; } }
        float ml = glm::length(mv);
        if (ml > 1e-3f) feet += mv / ml * std::min(7.f, 7.f * ml) * dt;
        glm::vec3 o = feet - c; o.y = 0.f;
        float ol = glm::length(o);
        if (ol > 41.5f) feet = c + o / ol * 41.5f;
        // The floor under you, looked for from the floor's own height (not your feet's: one bad step
        // would leave you under the world for good). No floor there (a gap at the rim, a broken
        // shard): a player doesn't step into the void, so the move is taken back
        float gy = lv.groundAt(feet.x, feet.z, std::max(feet.y, c.y) + 2.f);
        if (gy < c.y - 1.f) feet = start;
        else feet.y = gy;
    }
    void heal(float h) { health = std::min(100.f, health + h); }
    void absoluteStep(float dt) {
        t += dt;
        float before = damageTaken;
        glm::vec3 was = feet;
        pillarsUp.clear();
        for (size_t i = 0; i < lv.zenith.pillarWalls.size(); ++i) if (lv.zenith.pillarUp[i]) pillarsUp.push_back(lv.walls[lv.zenith.pillarWalls[i]].box);
        absoluteMove(dt);
        vel = (feet - was) / dt;
        // SOLID: the sword's parry on every other white strike, as it lands
        bool window = boss.parryWindow();
        if (policy == SOLID && window && !prevWindow) {   // (only within the game's reach: a METEOR met from too far off isn't parried)
            glm::vec3 to = boss.position - feet; to.y = 0.f;
            bool inReach = glm::length(to) <= abSwordParryReach(boss);
            if (parryNext && inReach && boss.abParried(true)) { ++parries; heal(Sword::HEAL_PARRY); }
            parryNext = !parryNext;
        }
        prevWindow = window;
        boss.update(dt, world());
        const EnemyEvents& ev = boss.ev;
        if (ev.telegraphStarted) { dodgeFrom = t + REACT; dodgeUntil = 1e9f; }
        const glm::vec3 c = lv.zenith.centre;
        if (ev.abHalo) abs.addHalo(c, c.y);
        if (ev.abShardBreak) { int k = lv.zenithShardAt(ev.abTo); if (k >= 0) lv.parkZenithPiece(true, k); }
        if (ev.abShards) for (int k = 0; k < (int)lv.zenith.shardWalls.size(); ++k) lv.parkZenithPiece(true, k);
        if (ev.abRay) abs.addRay(ev.abFrom, ev.abTo);
        if (ev.abStomp) abs.addStomp(ev.abFrom);
        if (ev.abFan) abs.addFan(ev.abFrom, ev.abTo);
        if (ev.abSerpent) abs.addSerpent(ev.abFrom, feet + glm::vec3{0.f, 1.7f, 0.f} - ev.abFrom);
        if (ev.abSeams) abs.addSeams(c, ev.abSeamFirst);
        if (ev.abZenith) abs.addZenith(ev.abFrom, ev.abTo);
        for (int k = 0; k < ev.abMirages; ++k) abs.addMirage(ev.phantomPos[k]);
        if (ev.abJudgeMark) abs.addJudge(c, boss.position);
        abs.morningEvents(ev, c, feet, boss.yaw);
        if (ev.msForm) formAt = t;
        if (ev.meleeHit) { float b = damageTaken; bool redBlow = abTellOf(boss.lastAbAttack) == AbTell::RED; hurt(ev.meleeDamage * (blocking && !redBlow ? Sword::BLOCK_TAKEN : 1.f)); dmgMelee += damageTaken - b; if (boss.morning()) msBy[0] += damageTaken - b; }
        for (int k = 0; k < ev.shots; ++k)
            if (Projectile* pr = proj.fire(ev.shotOrigin, ev.shotDir[k] * ev.shotSpeed, ev.shotDamage, false, {1, 1, 1}, false, 0.f, ev.shotSize, false))
                pr->owner = boss.uid;
        auto r = proj.update(dt, lv.walls.data(), (int)lv.walls.size(), others, feet + glm::vec3{0, 1.7f, 0}, &grid);
        if (r.hitPlayer) { float b = damageTaken; hurt(r.playerDamage); dmgShot += damageTaken - b; }
        glm::vec3 hf = feet + glm::vec3{0.f, airborne ? 1.4f : 0.f, 0.f};
        for (auto& h : abs.update(dt, hf, sliding ? 0.9f : 1.8f, !airborne, &lv, boss.position)) {
            float b = damageTaken; hurt(h.damage);
            msBy[h.src] += h.src ? damageTaken - b : 0.f;
            (h.kind == Sword::Strike::BEAM ? dmgBeam : h.kind == Sword::Strike::GOLD ? dmgGround : dmgWhite) += damageTaken - b;
        }
        if (int m = abs.haloShatterMask()) for (int k = 0; k < 8; ++k) if (m & (1 << k)) lv.parkZenithPiece(false, k);
        if (policy == SOLID) {   // serpents parried back, mirages cut down
            int sp = abs.serpentNear(feet + glm::vec3{0.f, 1.7f, 0.f}, 2.4f);
            if (sp >= 0) abs.reflectSerpent(sp);
        }
        if (policy == SOLID || policy == DODGER)
            for (int m = (int)abs.mirages.size() - 1; m >= 0; --m)
                if (policy == DODGER || glm::length(abs.mirages[m].at - feet) < 3.5f) abs.killMirage(m);
        for (int n = abs.reflectedHits(boss.position + glm::vec3{0.f, 1.3f, 0.f}, 1.6f); n > 0; --n)
            boss.takeDamage(AbsoluteHazards::SERPENT_PARRY * boss.armorMult());
        health -= damageTaken - before; lowest = std::min(lowest, health);
        if (formAt >= 0.f) { lowest2 = std::min(lowest2, health); taken2 += damageTaken - before; }
        // the damage: SOLID's sword up close; DODGER's guns (his guard turns most beyond 14 m)
        if (!boss.targetable()) return;
        float d = glm::length(glm::vec2{feet.x - boss.position.x, feet.z - boss.position.z});
        if (policy == SOLID && boss.abAirborne()) boss.takeDamage(SOLID_DPS * dt * boss.armorMult());   // aloft: his guard's down, the guns land
        else if (policy == SOLID && d < Sword::REACH + 0.3f) {   // strokes of ~80: each heals 6
            float dmg = SOLID_DPS * dt * boss.armorMult();
            boss.takeDamage(dmg);
            heal(dmg / 80.f * Sword::HEAL_HIT);
        }
        if (policy == DODGER) boss.takeDamage((d < Enemy::AB_DEFLECT || boss.abAirborne() || boss.telegraphTimer > 0.f || boss.staggered() ? 40.f : 6.f) * dt * boss.armorMult());
    }
};
