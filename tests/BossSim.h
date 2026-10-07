#pragma once
// A scripted player against a boss's real AI, in its real arena: the boss's
// events resolved the way the game resolves them (melee, slams you can jump,
// projectiles, the Sovereign's blades, the Warden's hazards and conduits).
// CORNER/PERCH/KITE/RANGED are cheese strategies (they never shoot); SOLID
// circles at 10 m, shoots at a fixed effective DPS, cuts conduits first, hits
// open cores, and parries every other parry window.
struct BossSim {
    enum Policy { CORNER, PERCH, KITE, RANGED, SOLID };
    static constexpr float SOLID_DPS = 55.f;   // a solid player's effective damage a second (misses and reloads included)
    const LevelData& L; const SpatialGrid& grid; const Arena& A; Policy policy;
    Enemy boss; ProjectileSystem proj; SovereignHazards sov; WardenHazards ward; ConduitClock clock;
    std::vector<Enemy> others;
    LevelData lv; ArenaShifts rings;            // the reactor's rings, driven by the Warden as in the game
    glm::vec3 feet{0.f}; float damageTaken = 0.f, t = 0.f, kite = 0.f; int conduits = 4; float conduitHp = 250.f; bool parryNext = true;
    static constexpr float REACT = 0.3f, AFTER = 0.4f;   // SOLID: reacts to a tell after 0.3 s, dodging (not shooting) until 0.4 s after it lands
    float dodgeFrom = -1.f, dodgeUntil = -1.f; bool wasTelegraphing = false;
    glm::vec3 vel{0.f};   // the player's velocity (enemies lead their shots with it, as in the game)
    bool prevWindow = false;

    BossSim(const LevelData& l, const SpatialGrid& g, const Arena& a, EnemyType type, Policy p)
        : L(l), grid(g), A(a), policy(p), boss(type, a.bossSpawn), lv(l) {
        rings.capture(lv); rings.reset(lv);
        boss.spawnTimer = 0.f; boss.state = EnemyState::ACTIVE;
        boss.maxHealth *= difficulty(DIFFICULTY_DEFAULT).health; boss.health = boss.maxHealth;
        proj.floorY = L.lowestFloor();
        feet = a.playerStart;
        if (p == CORNER) {   // the arena's corner, standing on whatever is there
            feet = glm::vec3{a.bounds.min.x + 2.f, a.playerStart.y, a.bounds.min.z + 2.f};
            for (auto& w : L.walls)
                if (!w.dynamic && feet.x > w.box.min.x && feet.x < w.box.max.x && feet.z > w.box.min.z && feet.z < w.box.max.z &&
                    w.box.max.y < a.bounds.max.y - 2.f && w.box.max.y > feet.y) feet.y = w.box.max.y;
        }
        if (p == RANGED) {   // already holding its distance: 30 m out, toward where the fight came in
            glm::vec3 d{a.playerStart.x - boss.position.x, 0.f, a.playerStart.z - boss.position.z};
            float l = glm::length(d);
            feet = glm::vec3{boss.position.x, a.playerStart.y, boss.position.z} + (l > 0.1f ? d / l : glm::vec3{0, 0, 1}) * 30.f;
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
    EnemyWorld world() const {
        EnemyWorld w;
        w.playerFeet = feet; w.playerEye = feet + glm::vec3{0, 1.7f, 0};
        w.walls = L.walls.data(); w.wallCount = (int)L.walls.size(); w.grid = &grid; w.bounds = A.bounds;
        w.reactor = L.reactorPos; w.hasReactor = L.hasReactor; w.playerVel = vel;
        return w;
    }
    void hurt(float d) { damageTaken += d * A.damageScale * difficulty(DIFFICULTY_DEFAULT).damage; }
    void movePlayer(float dt) {
        glm::vec3 c = (A.bounds.min + A.bounds.max) * 0.5f;
        if (policy == KITE) {   // round the arena's edge, 2 m in, at a run
            kite += dt * 7.f;
            float w = A.bounds.max.x - A.bounds.min.x - 4.f, d = A.bounds.max.z - A.bounds.min.z - 4.f, per = 2.f * (w + d);
            float s = std::fmod(kite, per);
            glm::vec3 o{A.bounds.min.x + 2.f, A.playerStart.y, A.bounds.min.z + 2.f};
            if (s < w) feet = o + glm::vec3{s, 0, 0};
            else if (s < w + d) feet = o + glm::vec3{w, 0, s - w};
            else if (s < 2 * w + d) feet = o + glm::vec3{w - (s - w - d), 0, d};
            else feet = o + glm::vec3{0, 0, d - (s - 2 * w - d)};
        }
        if (policy == RANGED || policy == SOLID) {
            float want = policy == RANGED ? 30.f : 10.f;
            glm::vec3 from{feet.x - boss.position.x, 0.f, feet.z - boss.position.z};
            float l = glm::length(from);
            glm::vec3 dir = l > 0.1f ? from / l : glm::vec3{1, 0, 0};
            glm::vec3 side{-dir.z, 0.f, dir.x};
            glm::vec3 goal = glm::vec3{boss.position.x, A.playerStart.y, boss.position.z} + glm::normalize(dir + side * 0.3f) * want;
            goal.x = glm::clamp(goal.x, A.bounds.min.x + 1.5f, A.bounds.max.x - 1.5f);
            goal.z = glm::clamp(goal.z, A.bounds.min.z + 1.5f, A.bounds.max.z - 1.5f);
            glm::vec3 mv = goal - feet; mv.y = 0.f;
            float ml = glm::length(mv);
            if (ml > 0.05f) feet += mv / ml * std::min(ml, 8.f * dt);
            feet.y = A.playerStart.y;
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
        float m = boss.armorMult();
        if (boss.coreOpen()) m = std::max(m, 1.f) * (0.4f + 0.6f * boss.woundMult());   // most shots into the open core
        boss.takeDamage(dmg * m);
    }
    void step(float dt) {
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
        // SOLID parries every other window it sees open
        bool window = boss.parryWindow();
        if (policy == SOLID && window && !prevWindow) { if (parryNext) boss.stagger(boss.staggerTime()); parryNext = !parryNext; }
        prevWindow = window;
        shoot(dt);
    }
};
