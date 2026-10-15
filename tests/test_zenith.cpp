// Headless tests for the encore: the sword (Sword.h), the Zenith, THE
// ABSOLUTE's mind and hazards, and his fight against scripted players.
// Built and run by `make test` after test_game.
#include "../src/Sword.h"
#include "../src/GunKit.h"
#include "../src/LevelAct2.h"
#include "../src/MusicSynth.h"
#include "../src/Progression.h"
#include "../src/WaveDirector.h"
#include "../src/AbsoluteHazards.h"
#include <string>
#include <cstdio>
#include <cmath>

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++failures; } \
                              else { std::printf("ok:   %s\n", msg); } } while (0)
static constexpr float DT = 1.f / 60.f;

// Hold an input for t seconds (a press only on the first tick)
static Sword::Out run(Sword& s, SwordInput in, float t) {
    Sword::Out last; bool first = true;
    for (float u = 0.f; u < t - 1e-5f; u += DT) {
        Sword::Out o = s.update(DT, in);
        if (o.stroke >= 0) last.stroke = o.stroke;
        if (o.thrust) last.thrust = true;
        if (first) { in.swingPressed = in.guardPressed = false; first = false; }
    }
    return last;
}

int main() {
    // ---------------------------------------------------------------- the sword: strokes
    {
        Sword s;
        SwordInput click{true, true, false, false};
        Sword::Out o = s.update(DT, click);
        CHECK(o.stroke == 0 && s.swinging(), "a click starts the first stroke at once");
        CHECK(std::fabs(s.strokeDamage(0) - 70.f) < 1e-4f && std::fabs(s.strokeDamage(2) - 120.f) < 1e-4f,
              "strokes do 70, 70 and 120");
        run(s, {false, false, false, false}, Sword::SWING_TIME);
        o = s.update(DT, click);
        CHECK(o.stroke == 1, "a second click inside the combo window is the second stroke");
        run(s, {false, false, false, false}, Sword::SWING_TIME + Sword::COMBO_RESET + 0.05f);
        o = s.update(DT, click);
        CHECK(o.stroke == 0, "after 0.6 s without a swing the combo starts over");
    }
    {
        Sword s;
        s.update(DT, {true, true, false, false});
        Sword::Out o = s.update(DT, {true, true, false, false});   // clicked again mid-stroke
        CHECK(o.stroke < 0, "a click mid-stroke doesn't cut the stroke short");
        Sword::Out later = run(s, {false, false, false, false}, Sword::SWING_TIME);
        CHECK(later.stroke == 1, "...it's buffered: the next stroke follows when this one ends");
    }
    // ---------------------------------------------------------------- the sword: thrust
    {
        Sword s;
        run(s, {true, true, false, false}, Sword::CHARGE_TIME + 0.05f);
        Sword::Out o = s.update(DT, {false, false, false, false});
        CHECK(o.thrust && std::fabs(s.thrustDamage() - 220.f) < 1e-4f, "hold 0.5 s and release: a thrust for 220");
        Sword q;
        run(q, {true, true, false, false}, 0.3f);
        o = q.update(DT, {false, false, false, false});
        CHECK(!o.thrust, "released before 0.5 s: no thrust");
        Sword h;
        run(h, {true, true, false, false}, Sword::CHARGE_TIME + 0.05f);
        h.holster();
        o = h.update(DT, {false, false, false, false});
        CHECK(!o.thrust, "holstering mid-charge never fires a thrust");
    }
    // ---------------------------------------------------------------- the sword: parry, spam lock, riposte
    {
        Sword s;
        s.update(DT, {false, false, true, true});
        CHECK(s.parryOpen(), "raising the guard opens a parry window");
        CHECK(s.receive(Sword::Strike::WHITE, true) == Sword::Guarded::PARRIED, "a white strike in the window is parried");
        CHECK(std::fabs(s.strokeDamage(0) - 70.f * Sword::RIPOSTE_MULT) < 1e-3f, "the next stroke is a riposte at x2.5");
        CHECK(std::fabs(s.strokeDamage(0) - 70.f) < 1e-3f, "...and only that one");
    }
    {
        Sword s;
        s.update(DT, {false, false, true, true});
        run(s, {false, false, false, true}, 0.12f);   // 8 more ticks: 0.133 s raised
        CHECK(s.parryOpen(), "the window is still open at 0.13 s");
        run(s, {false, false, false, true}, 0.03f);   // 0.167 s
        CHECK(!s.parryOpen() && s.blocking(), "closed by 0.17 s: still blocking");
    }
    {
        Sword s;
        s.update(DT, {false, false, true, true});
        run(s, {false, false, false, true}, 0.2f);          // missed
        run(s, {false, false, false, false}, 0.1f);         // lowered
        s.update(DT, {false, false, true, true});           // raised again 0.1 s after the miss
        CHECK(!s.parryOpen() && s.blocking(), "a raise within 0.4 s of a missed parry blocks but can't parry");
        Sword t;
        t.update(DT, {false, false, true, true});
        run(t, {false, false, false, true}, 0.2f);
        run(t, {false, false, false, false}, 0.45f);
        t.update(DT, {false, false, true, true});
        CHECK(t.parryOpen(), "after 0.4 s the window opens again");
    }
    {
        Sword s;
        s.update(DT, {false, false, true, true});
        CHECK(s.receive(Sword::Strike::GOLD, true) == Sword::Guarded::BLOCKED, "a gold strike can't be parried: blocked");
        CHECK(s.receive(Sword::Strike::WHITE, false) == Sword::Guarded::NONE, "a strike from behind isn't guarded");
        Sword p;
        p.update(DT, {false, false, true, true});
        CHECK(p.tryParry() && !p.tryParry(), "one window parries one thing (a projectile or a ray)");
    }
    // ---------------------------------------------------------------- the sword: block and guard break
    {
        Sword s;
        s.update(DT, {false, false, true, true});
        run(s, {false, false, false, true}, 0.2f);
        s.receive(Sword::Strike::WHITE, true);
        CHECK(std::fabs(s.guard - 75.f) < 1e-3f, "a blocked white strike drains 25");
        s.receive(Sword::Strike::GOLD, true);
        CHECK(std::fabs(s.guard - 15.f) < 1e-3f, "a blocked gold strike drains 60");
        CHECK(s.receive(Sword::Strike::WHITE, true) == Sword::Guarded::BROKE && s.broken() && !s.blocking(),
              "the meter empties: guard break");
        run(s, {false, false, false, true}, 0.5f);
        CHECK(!s.blocking(), "broken: the guard can't come up for a second");
        run(s, {false, false, false, false}, Sword::BREAK_TIME);
        CHECK(!s.broken(), "...and then it can");
        float g = s.guard;
        run(s, {false, false, false, false}, 1.0f + 1.0f);
        CHECK(s.guard > g + 25.f, "the guard refills (30/s after a second down)");
        CHECK(std::fabs(Sword::BLOCK_TAKEN - 0.25f) < 1e-6f && std::fabs(s.moveMult() - 1.f) < 1e-6f,
              "a block takes 25 %; lowered, you move at full speed");
        Sword b; b.update(DT, {false, false, true, true});
        b.receive(Sword::Strike::BEAM, true, 1.f);
        CHECK(std::fabs(b.guard - 85.f) < 1e-3f && std::fabs(b.moveMult() - Sword::BLOCK_SPEED) < 1e-6f,
              "a beam drains 15 a second; blocking you move at 60 %");
        Sword n; n.update(DT, {false, false, true, true});
        Sword::Out o = n.update(DT, {true, true, false, true});
        CHECK(o.stroke < 0, "no swings while blocking");
    }
    // ---------------------------------------------------------------- the sword: reach, arc, thrust path, lock-on
    {
        glm::vec3 eye{0, 1.7f, 0}, fwd{0, 0, -1};
        CHECK(Sword::inArc(eye, fwd, {0, 1.2f, -3.f}, 0.5f), "a target 3 m ahead is in reach");
        CHECK(!Sword::inArc(eye, fwd, {0, 1.2f, -4.5f}, 0.5f), "4.5 m ahead is out of reach");
        CHECK(Sword::inArc(eye, fwd, {1.8f, 1.2f, -1.8f}, 0.5f), "45 degrees off the crosshair is in the arc");
        CHECK(!Sword::inArc(eye, fwd, {2.5f, 1.2f, 0.5f}, 0.5f), "beside and behind is not");
        CHECK(Sword::thrustHits({0, 1, 0}, {0, 1, -8}, {0.8f, 1, -5}, 0.5f), "a thrust hits what's along its path");
        CHECK(!Sword::thrustHits({0, 1, 0}, {0, 1, -8}, {3.f, 1, -5}, 0.5f), "...not what's 3 m to the side");
        glm::vec3 st = Sword::lockStep({0, 0, 0}, fwd, {0, 0, -5}, 0.8f);
        CHECK(std::fabs(glm::length(st) - Sword::STEP_DIST) < 1e-3f && st.z < 0.f, "lock-on: a stroke carries you 2 m toward him");
        st = Sword::lockStep({0, 0, 0}, fwd, {0, 0, -2.f}, 0.8f);
        CHECK(glm::length(st) < 0.25f, "...but never into him");
        st = Sword::lockStep({0, 0, 0}, fwd, {0, 0, 5}, 0.8f);
        CHECK(glm::length(st) < 1e-4f, "...and not toward something behind you");
    }

    // ---------------------------------------------------------------- the sword: its look and motion
    {
        std::vector<GunPart> parts;
        auto lit = [&](float g, bool broken) {
            parts.clear(); SwordLook L; L.guard01 = g; L.broken = broken;
            gunkit::buildSword(L, parts);
            int n = 0; for (auto& p : parts) n += p.tag == 1; return n;
        };
        CHECK(lit(1.f, false) == 8 && lit(0.5f, false) == 4 && lit(0.01f, false) == 1, "the fuller's 8 cells read the guard meter");
        CHECK(lit(1.f, true) == 0, "a broken guard: the fuller goes dark");
        parts.clear(); gunkit::buildSword(SwordLook{}, parts);
        glm::vec3 lo{1e9f}, hi{-1e9f};
        for (auto& p : parts) { glm::vec3 c{p.xf[3]}; lo = glm::min(lo, c); hi = glm::max(hi, c); }
        CHECK(hi.z - lo.z > 0.7f && hi.z - lo.z < 1.1f && hi.x - lo.x < 0.25f, "a blade about 0.9 m long and slim");
        bool glows = false;
        for (auto& p : parts) if (p.tag == 1 && glm::length(p.emissive - gunkit::SWORD_GLOW * 1.6f) < 0.05f) glows = true;
        CHECK(glows, "its cells glow rose-red");
        SwordPose a = gunkit::swordPose(0, 0.f, false, 0.f, 0.f), b = gunkit::swordPose(0, 1.f, false, 0.f, 0.f);
        CHECK(a.offset.x > 0.1f && b.offset.x < -0.1f, "stroke 0 sweeps from right to left");
        a = gunkit::swordPose(1, 0.f, false, 0.f, 0.f); b = gunkit::swordPose(1, 1.f, false, 0.f, 0.f);
        CHECK(a.offset.x < -0.1f && b.offset.x > 0.1f, "stroke 1 sweeps from left to right");
        a = gunkit::swordPose(2, 0.f, false, 0.f, 0.f); b = gunkit::swordPose(2, 1.f, false, 0.f, 0.f);
        CHECK(a.offset.y > b.offset.y + 0.2f, "stroke 2 comes down overhead");
        SwordPose g = gunkit::swordPose(-1, -1.f, true, 0.f, 0.f), r = gunkit::swordPose(-1, -1.f, false, 0.f, 0.f);
        CHECK(std::fabs(g.offset.x) < std::fabs(r.offset.x) && std::fabs(g.roll) > 60.f, "raised: the blade across the middle of the screen");
    }

    // ---------------------------------------------------------------- the sword: where it can be drawn
    {
        CHECK(swordKeyAllowed(false, true, 5, 5), "key 5 in the Zenith draws the sword");
        CHECK(!swordKeyAllowed(false, true, 4, 5) && !swordKeyAllowed(false, false, 5, 5),
              "key 5 outside the Zenith keeps the gun");
        CHECK(swordKeyAllowed(true, false, 0, 5), "--sword allows it anywhere (dev)");
    }

    // ---------------------------------------------------------------- the Zenith
    LevelData N = buildAct2Level();
    SpatialGrid g; g.build(N.walls);
    const glm::vec3 ZC = zenithCentre();
    {
        CHECK(N.arenas.size() == 6 && std::string(N.arenas[ZENITH_ARENA].name) == "THE ZENITH", "a sixth Act II arena: the Zenith");
        const Arena& Z = N.arenas[ZENITH_ARENA];
        CHECK(Z.waves.size() == 1 && Z.waves[0][0].type == EnemyType::ABSOLUTE && Z.maxAlive == 1 && Z.space == ReverbSpace::OPEN,
              "one wave, THE ABSOLUTE, alone, under an open sky");
        CHECK(std::fabs(Z.voidY - (ZC.y - 40.f)) < 1e-3f && Z.hasRespawn, "the void below, a respawn on the floor");
        CHECK(N.arenaAt(Z.playerStart) == ZENITH_ARENA && N.arenaAt(zenithShard(3)) == ZENITH_ARENA,
              "the Zenith's start and shards belong to the Zenith (arenaAt is XZ-only: it must not overlap the Maw)");
        auto floorAt = [&](glm::vec3 p) { return N.groundAt(p.x, p.z, p.y + 3.f); };
        bool floorOk = true;
        for (int k = 0; k < 16; ++k) for (float r : {12.f, 25.f, 40.f}) {
            float a = k * 0.3926991f;
            glm::vec3 p = ZC + glm::vec3{std::cos(a) * r, 0.5f, std::sin(a) * r};
            if (N.zenithPillarAt(p, 0.5f) >= 0) continue;
            if (std::fabs(floorAt(p) - ZC.y) > 0.05f) floorOk = false;
        }
        CHECK(floorOk, "the floor is at the centre's height from r 9 to 44, all the way round");
        CHECK(std::fabs(floorAt(ZC + glm::vec3{0.f, 2.f, 3.f}) - (ZC.y + ZEN_DAIS_UP)) < 0.05f, "the Dais is raised 1.2 m");
        CHECK(floorAt(ZC + glm::vec3{48.f, 2.f, 0.f}) < ZC.y - 30.f, "a gap past the floor's edge (the void)");
        CHECK(N.zenith.pillarWalls.size() == 8 && N.zenith.shardWalls.size() == 6, "eight pillars, six shards");
        bool shardsOk = true;
        for (int k = 0; k < 6; ++k) { glm::vec3 s = zenithShard(k); if (std::fabs(floorAt(s + glm::vec3{0, 3, 0}) - s.y) > 0.05f || !N.onZenithShard(s)) shardsOk = false; }
        CHECK(shardsOk, "each shard is a floor you can stand on");
        N.parkZenithPiece(false, 0);
        CHECK(N.zenithPillarAt(zenithPillar(0), 0.5f) < 0 && !N.zenith.pillarUp[0], "a shattered pillar is gone: no collision");
        N.parkZenithPiece(true, 2);
        CHECK(!N.onZenithShard(zenithShard(2)), "a fallen shard is gone");
        N.restoreZenith();
        CHECK(N.zenithPillarAt(zenithPillar(0), 0.5f) == 0 && N.onZenithShard(zenithShard(2)), "a retry puts them back");
        CHECK(MUSIC_TRACKS == 11 && std::string(musicTrack(10).name) == "ZENITH" && musicTrack(10).bpm >= 165.f, "the Zenith's own fast track");
    }

    // ---------------------------------------------------------------- the hand-off and the records
    {
        Records r;
        recordAct2Clear(r, 1500.f);
        CHECK(r.act2Cleared && std::fabs(r.bestAct2 - 1500.f) < 1e-3f, "the Leviathan's death saves the Act II clear and its time");
        recordAct2Clear(r, 1600.f);
        CHECK(std::fabs(r.bestAct2 - 1500.f) < 1e-3f, "a slower clear doesn't replace the best");
        CHECK(recordZenith(r, 1700.f) && !recordZenith(r, 1800.f) && std::fabs(r.bestZenith - 1700.f) < 1e-3f, "ACT II + ZENITH keeps its own best");
        CHECK(zenithHandoff(true, 4, 6) && !zenithHandoff(true, 3, 6) && !zenithHandoff(false, 4, 6) && !zenithHandoff(true, 4, 5),
              "only the Maw's clear in Act II hands off to the Zenith");
        CHECK(retryTarget(WaveDirector::Phase::CLEARED, 4) == ZENITH_ARENA && retryTarget(WaveDirector::Phase::ACTIVE, ZENITH_ARENA) == ZENITH_ARENA,
              "a death during the ascension retries the Zenith; a death in it, the Zenith again");
        Records w; w.act2Cleared = true; w.bestAct2 = 1234.5f; w.bestZenith = 1500.25f;
        std::string text = w.serialise();
        Records back; back.parse(text);
        CHECK(back.act2Cleared && std::fabs(back.bestAct2 - 1234.5f) < 0.01f && std::fabs(back.bestZenith - 1500.25f) < 0.01f,
              "the clear and both times survive a save and a load");
    }

    // ---------------------------------------------------------------- THE ABSOLUTE: phase 1
    const Arena& ZA = N.arenas[ZENITH_ARENA];
    std::vector<AABB> standing;
    auto zworld = [&](glm::vec3 feet) {
        standing.clear();
        for (size_t i = 0; i < N.zenith.pillarWalls.size(); ++i) if (N.zenith.pillarUp[i]) standing.push_back(N.walls[N.zenith.pillarWalls[i]].box);
        EnemyWorld w; w.playerFeet = feet; w.playerEye = feet + glm::vec3{0, 1.7f, 0};
        w.walls = N.walls.data(); w.wallCount = (int)N.walls.size(); w.grid = &g; w.bounds = ZA.bounds;
        w.hasZenith = true; w.zenCentre = ZC; w.zenFloor = ZC.y;
        w.zenPillars = standing.data(); w.zenPillarCount = (int)standing.size();
        return w;
    };
    auto freshAbs = [&]() { Enemy e(EnemyType::ABSOLUTE, ZA.bossSpawn); e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE; return e; };
    {
        bool rule = true;
        for (int id = 0; id < AB_STRINGS; ++id) {
            const AbString& s = abString(id);
            for (int k = 0; k < s.n; ++k)
                if (s.steps[k].damage > 0.f && abTellOf(s.steps[k].k) == AbTell::NONE) rule = false;
        }
        CHECK(rule, "every damaging move in every string has a tell colour");
        CHECK(abTellOf(AttackKind::AJAB) == AbTell::WHITE && abTellOf(AttackKind::AHOOK) == AbTell::WHITE &&
              abTellOf(AttackKind::AUPPER) == AbTell::GOLD && abTellOf(AttackKind::ADROP) == AbTell::GOLD &&
              abTellOf(AttackKind::ARAY) == AbTell::RED && abTellOf(AttackKind::ASTOMP) == AbTell::RED &&
              abTellOf(AttackKind::ASERPENT) == AbTell::WHITE && abTellOf(AttackKind::ASWEEP) == AbTell::WHITE &&
              abTellOf(AttackKind::ALUNGE) == AbTell::GOLD && abTellOf(AttackKind::ACRESCENT) == AbTell::WHITE,
              "white: jab, hook, serpent, sweep, crescent; gold: uppercut, drop-kick, lunge; red: ray, stomp");
        float full = 0.f;
        for (int id : {AB_TOO_SLOW, AB_KNEEL, AB_JUDGED}) {
            float sum = 0.f; const AbString& st = abString(id);
            for (int k = 0; k < st.n; ++k) if (st.steps[k].minPhase <= 1) sum += st.steps[k].damage;
            full = std::max(full, sum);
            for (int k = 0; k < st.n; ++k) if (st.steps[k].damage > 0.f && (st.steps[k].damage < 18.f || st.steps[k].damage > 30.f)) full = 1e9f;
        }
        CHECK(full >= 60.f && full <= 70.f, "a whole phase 1 string taken costs 60-70; single hits 18-30");
        CHECK(statsOf(EnemyType::ABSOLUTE).health == 9000.f && isBoss(EnemyType::ABSOLUTE), "9000 hp, a boss");
        CHECK(ZEN_DAIS == Enemy::AB_DAIS_R && ZEN_DAIS_UP == Enemy::AB_DAIS_UP, "the mind and the map agree on the Dais");
    }
    {
        Enemy e = freshAbs();
        glm::vec3 me = ZC + glm::vec3{0, 0, 20};
        e.update(DT, zworld(me));
        CHECK(!e.targetable(), "arriving: untargetable");
        for (int i = 0; i < 125; ++i) e.update(DT, zworld(me));
        CHECK(e.targetable(), "after 2 s he's in the fight");
        bool started = false;
        for (int i = 0; i < 120 && !started; ++i) { e.update(DT, zworld(me)); started = e.abCur >= 0; }
        CHECK(started, "...and opens a string within 2 s");
    }
    {
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = AB_TOO_SLOW;
        glm::vec3 me = ZC + glm::vec3{0, 0, 20};
        bool blinked = false; int whites = 0, parried = 0; float reeled = 0.f;
        bool firstOk = false, secondOk = false;
        for (int i = 0; i < 600 && parried < 2; ++i) {
            e.update(DT, zworld(me));
            if (e.ev.blinked) blinked = true;
            if (e.ev.abTell == (int)AbTell::WHITE) ++whites;
            if (e.parryWindow()) {
                int r = e.abParried(false);
                ++parried;
                if (parried == 1) { firstOk = r == 1 && e.staggered() && e.abReeling && std::fabs(e.armorMult() - 1.5f) < 1e-4f; reeled = e.staggerTimer; }
                if (parried == 2) secondOk = r == 2 && e.staggered() && !e.abReeling && std::fabs(e.armorMult() - 2.f) < 1e-4f && std::fabs(e.staggerTimer - 3.f) < 0.05f;
            }
        }
        CHECK(blinked, "TOO SLOW opens with a teleport (an afterimage)");
        CHECK(firstOk && whites >= 2 && std::fabs(reeled - Enemy::AB_REEL) < 0.05f, "the first parry: he reels 0.8 s (x1.5), and the string goes on");
        CHECK(secondOk, "the second in one string breaks him: 3 s at x2");
        CHECK(e.abParried(false) == 0, "a broken Absolute can't be parried again");
    }
    {
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = AB_KNEEL;
        glm::vec3 me = ZC + glm::vec3{0, 0, 12};
        bool goldWindow = false, launched = false;
        for (int i = 0; i < 400; ++i) {
            e.update(DT, zworld(me));
            if (abTellOf(e.attack) == AbTell::GOLD && e.parryWindow()) goldWindow = true;
            if (e.ev.abLaunch) launched = true;
        }
        CHECK(!goldWindow, "no parry window ever opens on a gold strike");
        CHECK(launched, "KNEEL's uppercut launches you when it lands");
    }
    {
        float least = 1e9f, leastFollow = 1e9f;
        for (int d = 0; d < DIFFICULTY_LEVELS; ++d)
            for (int id = 0; id < AB_STRINGS; ++id)
                for (int ph = 1; ph <= 3; ++ph) {
                    Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = id; e.abPhase = ph; e.abBlade = ph == 3;
                    EnemyWorld w = zworld(ZC + glm::vec3{0, 0, 14}); w.tune = &difficulty(d);
                    for (int i = 0; i < 400; ++i) {
                        e.update(DT, w);
                        if (e.ev.telegraphStarted && abTellOf(e.attack) != AbTell::NONE) {
                            if (e.tellFollow) leastFollow = std::min(leastFollow, e.telegraphDuration);
                            else least = std::min(least, e.telegraphDuration);
                        }
                    }
                }
        CHECK(least >= Enemy::TELL_FLOOR - 1e-4f && leastFollow >= Enemy::AB_FOLLOW_FLOOR - 1e-4f,
              "every Absolute tell is at least 0.35 s (0.22 s within a string), every difficulty and phase");
    }
    {
        // He starts beside the camper, mid-rotation, in a long string: only the
        // camp clock can bring TOO SLOW (a teleport onto you) before 5 s
        auto campTest = [&](glm::vec3 me, bool shard) {
            Enemy e = freshAbs(); e.abArrive = 0.f; e.abRot = 1; e.abForce = AB_JUDGED;
            glm::vec3 in = glm::normalize(glm::vec3{ZC.x - me.x, 0.f, ZC.z - me.z});
            e.position = me + in * 8.f; e.position.y = ZC.y;
            for (int i = 0; i < 5 * 60; ++i) {
                EnemyWorld w = zworld(me); w.onShard = shard;
                e.update(DT, w);
                if (e.ev.blinked && e.abCur == AB_TOO_SLOW && glm::length(glm::vec2{e.position.x - me.x, e.position.z - me.z}) < 6.f) return i * DT;
            }
            return 99.f;
        };
        float far = campTest(ZC + glm::vec3{0, 0, 42.f}, false);
        glm::vec3 pil = zenithPillar(0), out = glm::normalize(glm::vec3{pil.x - ZC.x, 0, pil.z - ZC.z});
        float behind = campTest(pil + out * 2.f, false);
        std::printf("      anti-camp: far %.1f s, behind a pillar %.1f s\n", far, behind);
        CHECK(far < 5.f && behind < 5.f, "far away or behind a pillar: within 5 s he's on you (TOO SLOW)");
    }
    {
        srand(77);
        int bad = 0;
        EnemyWorld w = zworld(ZC);
        for (int i = 0; i < 500; ++i) {
            float a = frand(0.f, 6.283f), r = frand(0.f, 48.f);
            glm::vec3 want = ZC + glm::vec3{std::cos(a) * r, 0.f, std::sin(a) * r};
            glm::vec3 p = absSafeSpot(w, want, ZC);
            float pr = glm::length(glm::vec2{p.x - ZC.x, p.z - ZC.z});
            if (pr > Enemy::AB_FLOOR_R + 0.01f || N.zenithPillarAt(p, 0.7f) >= 0) ++bad;
            if (pr < Enemy::AB_DAIS_R - 0.5f && std::fabs(p.y - (ZC.y + Enemy::AB_DAIS_UP)) > 1e-3f) ++bad;
        }
        CHECK(bad == 0, "500 teleports, at the edge, past it or into pillars, all land on open floor (on the Dais at its height)");
    }
    {
        Enemy e = freshAbs(); e.abArrive = 0.f; e.yaw = 0.f;
        CHECK(e.deflects({0, 0, -1}, 20.f) && !e.deflects({0, 0, -1}, 10.f), "beyond 14 m his guard turns shots aside; closer they land");
        e.stagger(1.f);
        CHECK(!e.deflects({0, 0, -1}, 20.f), "staggered, he's open from anywhere");
    }

    // ---------------------------------------------------------------- the Absolute's hazards
    {
        using AH = AbsoluteHazards;
        const glm::vec3 c = ZC;
        auto total = [](const std::vector<AH::Hit>& hs) { float s = 0; for (auto& h : hs) s += h.damage; return s; };
        { AH h; glm::vec3 me = c + glm::vec3{0, 0, 12};
          h.addRay(c + glm::vec3{0, 1.6f, 0}, me + glm::vec3{0, 1, 0});
          float got = 0; for (float t = 0; t < 1.2f; t += DT) got += total(h.update(DT, me, 1.8f, true, &N, c));
          CHECK(got > 20.f && got < 27.f, "standing in a ray: about 25 over its second");
          AH r; glm::vec3 p = me + glm::vec3{7.f * 0.6f, 0, 0}; float got2 = 0;   // strafed through its 0.6 s tell
          r.addRay(c + glm::vec3{0, 1.6f, 0}, me + glm::vec3{0, 1, 0});
          for (float t = 0; t < 1.2f; t += DT) { p.x += 7.f * DT; got2 += total(r.update(DT, p, 1.8f, true, &N, c)); }
          CHECK(got2 == 0.f, "strafing at a run outpaces it: it fires where its red line was, then chases at 5.5 m/s");
          AH q; glm::vec3 pil = zenithPillar(0), out = glm::normalize(glm::vec3{pil.x - c.x, 0, pil.z - c.z});
          glm::vec3 hide = pil + out * 2.f;
          q.addRay(c + glm::vec3{0, 1.6f, 0}, hide + glm::vec3{0, 1, 0});
          float got3 = 0; for (float t = 0; t < 1.2f; t += DT) got3 += total(q.update(DT, hide, 1.8f, true, &N, c));
          CHECK(got3 < 1e-3f, "a pillar between you stops it");
          AH pr; pr.addRay(c, me); pr.update(0.1f, me, 1.8f, true, &N, c);
          CHECK(pr.rayParry(0) && pr.rays.empty(), "a ray caught in its first 0.15 s is parried and ends");
          AH late; late.addRay(c, me); late.update(0.2f, me, 1.8f, true, &N, c);
          CHECK(!late.rayParry(0), "...not after"); }
        { glm::vec3 me = c + glm::vec3{0, 0, 9};
          AH h; h.addStomp(c);
          float grounded = 0, jumped = 0;
          for (float t = 0; t < 1.f; t += DT) grounded += total(h.update(DT, me, 1.8f, true, &N, c));
          AH j; j.addStomp(c);
          for (float t = 0; t < 1.f; t += DT) jumped += total(j.update(DT, me + glm::vec3{0, 1.4f, 0}, 1.8f, false, &N, c));
          CHECK(std::fabs(grounded - AH::STOMP_DAMAGE) < 1e-3f && jumped == 0.f, "the stomp ring hits once on the ground; jump it"); }
        { glm::vec3 me = c + glm::vec3{0, 0, 15}, p = me;
          AH h; h.addSerpent(c + glm::vec3{0, 1.8f, 0}, {0, 0, 1});
          float got = 0;
          for (float t = 0; t < AH::SERPENT_LIFE + 0.5f; t += DT) { p.x += 7.f * DT; got += total(h.update(DT, p, 1.8f, true, &N, c)); }
          CHECK(got == 0.f, "a running player outruns a serpent");
          AH s; s.addSerpent(c + glm::vec3{0, 1.8f, 0}, {0, 0, 1});
          float hit = 0; for (float t = 0; t < 4.f; t += DT) hit += total(s.update(DT, me, 1.8f, true, &N, c));
          CHECK(std::fabs(hit - AH::SERPENT_DAMAGE) < 1e-3f, "standing still, it reaches you once");
          AH r; r.addSerpent(me + glm::vec3{0, 1.5f, -1.f}, {0, 0, 1});
          int k = r.serpentNear(me + glm::vec3{0, 1.5f, 0}, 2.f);
          CHECK(k == 0, "a serpent within reach can be found to parry");
          r.reflectSerpent(k);
          int back = 0; for (float t = 0; t < 4.f && !back; t += DT) { r.update(DT, me, 1.8f, true, &N, c); back = r.reflectedHits(c + glm::vec3{0, 1.3f, 0}, 1.4f); }
          CHECK(back == 1, "parried, it flies back into him"); }
        { glm::vec3 a = c + glm::vec3{3, 0, 3}, b = c + glm::vec3{9, 0, 3};
          CHECK(AH::seamParity(c, a) != AH::seamParity(c, b), "neighbouring cells alternate");
          glm::vec3 in0 = AH::seamParity(c, a) == 0 ? a : b, in1 = AH::seamParity(c, a) == 0 ? b : a;
          AH h0; h0.addSeams(c, 0); float d0 = 0; for (float t = 0; t < AH::SEAM_WARN + 0.05f; t += DT) d0 += total(h0.update(DT, in0, 1.8f, true, &N, c));
          AH h1; h1.addSeams(c, 0); float d1 = 0; for (float t = 0; t < AH::SEAM_WARN + 0.05f; t += DT) d1 += total(h1.update(DT, in1, 1.8f, true, &N, c));
          CHECK(d0 == AH::SEAM_DAMAGE && d1 == 0.f, "the first wave burns one parity; the dark cells are safe");
          AH h2; h2.addSeams(c, 0); float d2 = 0; for (float t = 0; t < AH::SEAM_WARN + AH::SEAM_GAP + 0.05f; t += DT) d2 += total(h2.update(DT, in1, 1.8f, true, &N, c));
          CHECK(d2 == AH::SEAM_DAMAGE, "...then the other, 0.7 s later: step across"); }
        { glm::vec3 me = c + glm::vec3{0, 0, 20}, p = me + glm::vec3{7.f * 0.6f, 0, 0};
          AH h; h.addZenith(c + glm::vec3{0, 40, 0}, me);
          float got = 0; for (float t = 0; t < AH::ZRAY_TIME; t += DT) { p.x += 7.f * DT; got += total(h.update(DT, p, 1.8f, true, &N, c)); }
          AH s; s.addZenith(c + glm::vec3{0, 40, 0}, me);
          float still = 0; for (float t = 0; t < AH::ZRAY_TIME; t += DT) still += total(s.update(DT, me, 1.8f, true, &N, c));
          CHECK(got == 0.f && still > 40.f, "the zenith ray burns where you stand, and turns slower than you run"); }
        { glm::vec3 me = c + glm::vec3{0, 0, 20};
          AH h; h.addHalo(c, c.y);
          float stand = 0; for (float t = 0; t < AH::HALO_TIME + 0.1f; t += DT) stand += total(h.update(DT, me, 1.8f, true, &N, c));
          AH s; s.addHalo(c, c.y);
          float slide = 0; for (float t = 0; t < AH::HALO_TIME + 0.1f; t += DT) slide += total(s.update(DT, me, 0.9f, true, &N, c));
          CHECK(stand == AH::HALO_DAMAGE && slide == 0.f, "the 360 sweep hits you standing; slide under it");
          AH m; m.addHalo(c, c.y); int mask = 0;
          for (float t = 0; t < AH::HALO_TIME + 0.1f; t += DT) { m.update(DT, me, 1.8f, true, &N, c); mask |= m.haloShatterMask(); }
          CHECK(mask == 0x55, "it shatters four pillars (every other one)"); }
        { AH h; h.addJudge(c, c); AH f; f.addJudge(c, c);
          float near = 0, far = 0;
          for (float t = 0; t < AH::JUDGE_WARN + 0.1f; t += DT) {
              near += total(h.update(DT, c + glm::vec3{0, 0, 4}, 1.8f, true, &N, c));
              far += total(f.update(DT, c + glm::vec3{0, 6, 20}, 1.8f, false, &N, c));
          }
          CHECK(near == 0.f && far == AH::JUDGE_DAMAGE, "the last judgment: beside him you're safe; anywhere else (even in the air) it's 60"); }
        { glm::vec3 me = c + glm::vec3{0, 0, 10};
          AH h; h.addMirage(me + glm::vec3{2, 0, 0});
          CHECK(h.mirageAlong(me + glm::vec3{0, 1.5f, 0}, glm::vec3{1, 0, 0}, 10.f) == 0, "a mirage can be shot");
          h.killMirage(0);
          float got = 0; for (float t = 0; t < 1.f; t += DT) got += total(h.update(DT, me, 1.8f, true, &N, c));
          CHECK(h.mirages.empty() && got == 0.f, "one hit and it's gone");
          AH k; k.addMirage(me + glm::vec3{2, 0, 0}); float hit = 0;
          for (float t = 0; t < 1.f; t += DT) hit += total(k.update(DT, me, 1.8f, true, &N, c));
          CHECK(hit == AH::MIRAGE_DAMAGE, "left alone its jab lands for half (8)"); }
        { AH h; h.addRay(c, c); h.addStomp(c); h.addSerpent(c, {0, 0, 1}); h.addSeams(c, 0); h.addZenith(c, c); h.addHalo(c, c.y); h.addMirage(c); h.addJudge(c, c);
          h.clear();
          CHECK(h.rays.empty() && h.stomps.empty() && h.serpents.empty() && h.seams.empty() && h.zrays.empty() && h.halos.empty() && h.mirages.empty() && h.judges.empty(),
                "clear() empties every hazard (a retry)"); }
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
