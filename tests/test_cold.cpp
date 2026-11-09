// THE COLD STORE (Chapter 5): the planet. The shelf, its ragged lip and the
// Void past it, walking it with the real controller, falling off it, and the
// snow and ice textures. No OpenGL context required. `make test-cold`.
#include "core/Balance.h"
#include "core/MouseCapture.h"
#include "world/LevelCold.h"
#include "world/LevelPlain.h"
#include "game/StorySave.h"
#include "world/ZoneConstraint.h"
#include "render/MaterialTex.h"
#include "enemies/Enemy.h"
#include "render/EnemyModel.h"
#include "world/Interactable.h"
#include "audio/MusicSynth.h"
#include "world/Stuck.h"
#include "player/Player.h"
#include <cstring>
#include <algorithm>
#include <set>
#include <cstdio>
static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { ++failures; std::printf("FAIL: %s\n", m); } else std::printf("ok: %s\n", m); } while (0)
constexpr float DT = 1.f / 60;

// The player as the game moves it: keys, collision, the zone, and the hard floor
// under it (the Void's is far down)
struct Walk {
    LevelData& l; SpatialGrid grid; Player p; float stall = 0.f, clock = 0.f;
    Walk(LevelData& level, glm::vec3 start) : l(level), p(start) { grid.build(l.walls); }
    void step(glm::vec3 goal, bool mayJump, bool still = false) {
        Uint8 keys[SDL_NUM_SCANCODES] = {};
        auto d = goal - p.position; d.y = 0; float n = glm::length(d);
        p.camera.yaw = glm::degrees(std::atan2(d.z, d.x)); keys[SDL_SCANCODE_W] = n > .2f && !still;
        float speed = glm::length(glm::vec2{p.velocity.x, p.velocity.z});
        stall = (n > 1.f && speed < 1.f) ? stall + DT : 0.f;
        if (mayJump && stall > 0.15f && p.onGround) { keys[SDL_SCANCODE_SPACE] = 1; stall = 0.f; }
        int ride = l.moverOfWall(p.groundWall);
        clock += DT; l.updateMovers(clock);
        if (ride >= 0) p.position += l.movers[ride].delta;
        p.dynWalls = l.moverWalls.data(); p.dynCount = (int)l.moverWalls.size();
        p.floorY = l.baseFloor(p.position.x, p.position.z, p.radius);
        p.update(DT, keys, l.walls.data(), (int)l.walls.size(), false, &grid);
        std::vector<const AABB*> zones; for (auto& a : l.arenas) { zones.push_back(&a.zone); for (auto& z : a.extraZones) zones.push_back(&z); }   // (as the game does)
        constrainToZones(p.position, p.velocity, p.height, zones, true);
    }
    bool to(glm::vec3 goal, bool mayJump = false, int frames = 9000, float tol = 1.f) {
        for (int f = 0; f < frames; ++f) {
            if (glm::length(glm::vec2{p.position.x - goal.x, p.position.z - goal.z}) < tol) return true;
            step(goal, mayJump);
        }
        std::printf("  stuck %.1f %.1f %.1f -> %.1f %.1f %.1f\n", p.position.x, p.position.y, p.position.z, goal.x, goal.y, goal.z);
        return false;
    }
};

int main() {
    balanceInit();
    {
        bool held=false;
        CHECK(!pointerLockReleased(false,held),"a denied initial browser mouse request does not pause gameplay");
        CHECK(!pointerLockReleased(true,held)&&held,"acquiring a browser mouse lock does not pause gameplay");
        CHECK(pointerLockReleased(false,held)&&!held,"releasing a held browser mouse lock still pauses gameplay");
        CHECK(!pointerLockReleased(false,held),"repeated inactive browser notifications do not pause again");
    }
    auto L = buildCold();
    const Arena& A = L.arenas[0];

    // ---- the level ----
    CHECK(L.story.built && L.story.chapter == 5 && std::string(L.story.title) == "THE COLD STORE", "Chapter 5 builds as THE COLD STORE");
    CHECK(L.arenas.size() == 10 && A.hasRespawn && A.voidY > cold::VOID_FLOOR + 100.f, "ten sections; falling into the Void respawns you long before its floor");
    const glm::vec3 s = cold::start();
    CHECK(L.baseFloor(s.x, s.z) == 0.f, "the arrival stands on the shelf");
    bool clearStart = true;
    for (const auto& w : L.walls)
        if (std::abs(w.box.min.x + w.box.max.x - 2.f * s.x) < (w.box.max.x - w.box.min.x) + 30.f
            && std::abs(w.box.min.z + w.box.max.z - 2.f * s.z) < (w.box.max.z - w.box.min.z) + 30.f && w.box.max.y > 0.1f) clearStart = false;
    CHECK(clearStart, "nothing solid within 15 m of the arrival");

    {   // Service roofs have physical routes as well as visible ramp meshes.
        bool service=true;
        for(int k=0;k<3;++k) {
            auto d=cold::chiller(k);Walk w(L,d.c+glm::vec3{-2,0,-4.7f});
            bool up=w.to(d.c+glm::vec3{-16.4f,3.2f,-4.7f},false,9000,.3f)
                && w.to(d.c+glm::vec3{-16.4f,3.2f,-1},false,9000,.3f)
                && w.to(d.c+glm::vec3{-14,3.2f,-1},false,9000,.3f);
            for(int f=0;f<30;++f)w.step(d.c+glm::vec3{-14,3.2f,-1},false,true);
            up&=w.p.onGround && std::fabs(w.p.position.y-3.2f)<.1f;
            service&=up && w.to(d.c+glm::vec3{-16.4f,3.2f,-1},false,9000,.3f)
                && w.to(d.c+glm::vec3{-16.4f,3.2f,-4.7f},false,9000,.3f)
                && w.to(d.c+glm::vec3{-2,0,-4.7f},false,9000,.3f);
        }
        CHECK(service,"every Blast Chiller service deck can be climbed, crossed and left without jumping");
    }
    // ---- the lip ----
    {
        bool loop=true,lowerLane=true,recovery=true;
        for(int k=0;k<3;++k) {
            const auto c=cold::chiller(k).c;
            Walk w(L,c+glm::vec3{0,0,14});
            loop&=w.to(c+glm::vec3{0,3.2f,2},false,1800,.25f)
                && w.to(c+glm::vec3{0,3.2f,-.6f},false,1800,.25f)
                && w.to(c+glm::vec3{11,3.2f,-.6f},false,1800,.25f)
                && w.to(c+glm::vec3{-14,3.2f,-.6f},false,1800,.25f);
            for(int f=0;f<30;++f)w.step(w.p.position,false,true);
            loop&=w.p.onGround && std::fabs(w.p.position.y-3.2f)<.1f;
            loop&=w.to(c+glm::vec3{11,3.2f,-.6f},false,1800,.25f)
                && w.to(c+glm::vec3{0,3.2f,-.6f},false,1800,.25f)
                && w.to(c+glm::vec3{0,3.2f,2},false,1800,.25f)
                && w.to(c+glm::vec3{0,0,14},false,1800,.25f);
            Walk ground(L,c+glm::vec3{-5,0,0});
            lowerLane&=ground.to(c+glm::vec3{18,0,0},false,1800,.25f) && ground.p.position.y<.1f;
            recovery&=coldcombat::chillerRecovery(L,c+glm::vec3{-3,0,4.5f},.4f,1.8f)
                && !coldcombat::chillerRecovery(L,cold::vent(k*2),.4f,1.8f)
                && !coldcombat::chillerRecovery(L,c+glm::vec3{21,0,10},.4f,1.8f);
        }
        CHECK(loop,"new Chiller ramps and upper bridges form a reversible walking loop on all three decks");
        CHECK(lowerLane,"the Chiller cross-deck lane stays open underneath the service roofs");
        CHECK(recovery,"Chiller fall recovery accepts safe deck floor and rejects vents and edges");
        bool rim=true;
        for(int k=0;k<3;++k)for(int n=0;n<48;++n) {
            const auto d=cold::chiller(k);const float a=n*6.2831853f/48;
            const auto p=d.c+glm::vec3{std::cos(a)*(d.r-.8f),0,std::sin(a)*(d.r-.8f)};
            rim&=std::fabs(L.groundAt(p.x,p.z,.1f))<.1f;
        }
        CHECK(rim,"the full visible round deck is supported, including its diagonal corners");
        Walk gap(L,{150,0,-830});
        for(int f=0;f<360;++f)gap.step(gap.p.position,false,true);
        CHECK(gap.p.position.y<L.arenas[5].voidY,"the visible gap between Chillers is a real fall, without an invisible floor");
    }
    {
        const auto c=cold::station();Walk w(L,c+glm::vec3{-23.5f,0,-15});
        bool balcony=w.to(c+glm::vec3{-23.5f,3.2f,-5},false,1800,.25f)
            && w.to(c+glm::vec3{-23.5f,3.2f,6},false,1800,.25f);
        for(int f=0;f<30;++f)w.step(w.p.position,false,true);
        balcony&=w.p.onGround && std::fabs(w.p.position.y-3.2f)<.1f;
        balcony&=w.to(c+glm::vec3{-23.5f,0,15.5f},false,1800,.25f)
            && w.to(c+glm::vec3{-23.5f,3.2f,6},false,1800,.25f)
            && w.to(c+glm::vec3{-23.5f,0,-15},false,1800,.25f);
        CHECK(balcony,"Dispatch 2's loading balcony can be walked up, crossed and left from either end");
        Walk entry(L,c+glm::vec3{-31,0,0});
        CHECK(entry.to(c+glm::vec3{-20,0,0},false,1800,.25f)&&entry.p.position.y<.1f,
            "Dispatch 2's west entrance stays open beneath the loading balcony");
        Walk ramp(L,c+glm::vec3{0,0,15.5f});
        bool platform=ramp.to(c+glm::vec3{0,.6f,8},false,1800,.25f);
        for(int f=0;f<30;++f)ramp.step(ramp.p.position,false,true);
        CHECK(platform&&ramp.p.onGround&&std::fabs(ramp.p.position.y-.6f)<.1f,"the central loading ramp climbs the station platform without jumping");
    }
    {
        const auto c=cold::vaultC();glm::vec3 landing;AABB top;
        CHECK(coldcombat::vaultLanding(L,c+glm::vec3{24,0,0},.4f,1.8f,landing)
            && spawnClearance(L,landing,.4f,1.8f,true)
            && glm::length(landing-(c+glm::vec3{24,0,0}))<6.1f,
            "a rollback into new Vault cover resolves to nearby supported, clear floor");
        const auto cover=coldcombat::vaultLowCover()[1];
        CHECK(coldcombat::freezeTop(c+glm::vec3{25,0,0},cover,top)
            && top.min.y>cover.max.y && top.min.x>=cover.min.x && top.max.x<=cover.max.x
            && !coldcombat::freezeTop(c,cover,top),"freeze warnings clip to the dangerous part of a cover top");
        bool drums=true,points=true;
        for(int k=0;k<4;++k)drums&=spawnClearance(L,cold::drum(k)+glm::vec3{0,1.4f,0},.4f,1.8f,true);
        for(int k=0;k<3;++k){const float a=.5f+k*2.0944f;points&=spawnClearance(L,c+glm::vec3{std::cos(a)*18,0,std::sin(a)*18},1,4,true);}
        CHECK(drums&&points,"Vault cover leaves every storage drum and restore point accessible");
    }
    {
        SpatialGrid grid;grid.build(L.walls);EnemyWorld world;
        world.walls=L.walls.data();world.wallCount=(int)L.walls.size();world.grid=&grid;
        world.bounds=L.arenas[5].bounds;world.voidUnder=true;
        world.ramps=L.ramps.data();world.rampCount=(int)L.ramps.size();
        bool pursuit=true,supported=true;
        for(int from=0;from<3;++from)for(int to=0;to<3;++to)if(from!=to) {
            Enemy e(EnemyType::THAWLING,cold::chiller(from).c);e.spawnTimer=0;e.state=EnemyState::ACTIVE;
            world.playerFeet=cold::chiller(to).c;world.playerEye=world.playerFeet+glm::vec3{0,1.7f,0};
            bool reached=false;
            for(int f=0;f<60*70;++f) {
                e.floorY=L.enemyFloor(5,e.position,false,L.groundAt(e.position.x,e.position.z,e.position.y+.5f));
                world.hasMoveVia=coldcombat::chillerRoute(e.position,world.playerFeet,world.moveVia);
                e.update(DT,world);supported&=e.position.y>-.3f;
                if(glm::length(glm::vec2{e.position.x-world.playerFeet.x,e.position.z-world.playerFeet.z})<4){reached=true;break;}
            }
            if(!reached)std::printf("  pursuit %d -> %d stopped at %.1f %.1f %.1f\n",from,to,e.position.x,e.position.y,e.position.z);
            pursuit&=reached;
        }
        CHECK(supported,"Chiller ground enemies stay on visible deck and catwalk support");
        CHECK(pursuit,"Thawlings can pursue between every pair of Chiller decks");
    }
    // ---- the lip ----
    bool lip = true, zoneOver = true, ragged = false;
    float lo = 1e9f, hi = -1e9f;
    for (float x = -cold::HALF_X + 1.5f; x < std::min(cold::HALF_X, cold::SEA_X0 - 1.f); x += 7.f) {   // (off the strips' boundary lines, where either side may answer)
        float ez = cold::edgeZ(x);
        lo = std::min(lo, ez); hi = std::max(hi, ez);
        lip &= L.baseFloor(x, ez + 0.5f) == 0.f && L.baseFloor(x, ez - 0.5f) == cold::VOID_FLOOR;
        zoneOver &= A.zone.min.z < ez - 10.f;
    }
    ragged = hi - lo > 15.f;
    std::printf("      the lip runs from z %.0f to %.0f across the shelf\n", lo, hi);
    CHECK(lip, "the shelf ends at its lip: snow on one side, the Void's long drop on the other");
    CHECK(zoneOver, "the play zone runs past the lip everywhere, so you can walk off it");
    CHECK(ragged, "the lip is ragged, not a ruler line");
    bool floorsOnShelf = true;
    for (const auto& f : L.floors) floorsOnShelf &= f.z0 >= cold::edgeZ(f.x0) - 0.01f;
    CHECK(floorsOnShelf, "no snow is drawn out over the Void");

    // ---- walking it ----
    {
        Walk w(L, s);
        glm::vec3 nearLip{0.f, 0.f, cold::edgeZ(0.f) + 6.f};
        CHECK(w.to(nearLip, true), "you can walk from the arrival to the lip (the marker line is clear)");
        CHECK(w.p.position.y > -0.1f && w.p.onGround, "...and stand there on the snow");
        // Walk on, off the edge: you fall, past the respawn line
        glm::vec3 beyond{0.f, 0.f, cold::edgeZ(0.f) - 40.f};
        float lowest = 0.f;
        for (int f = 0; f < 60 * 8; ++f) { w.step(beyond, false); lowest = std::min(lowest, w.p.position.y); }
        CHECK(lowest < A.voidY, "walk off the lip and you fall into the Void");
    }
    {   // Along the shelf, east to west, near the lip: ridges and blocks are in the way at most for a jump
        Walk w(L, {cold::HALF_X - 20.f, 0.f, cold::edgeZ(cold::HALF_X - 20.f) + 30.f});
        bool ok = true;
        for (float x = cold::HALF_X - 60.f; x > -cold::HALF_X + 20.f && ok; x -= 40.f)
            ok = w.to({x, 0.f, std::max(cold::edgeZ(x), cold::edgeZ(x + 40.f)) + 30.f}, true);
        CHECK(ok && w.p.position.y > -1.f, "you can walk the whole length of the shelf along the lip without falling in");
    }

    // ---- the route: out along the Hook Line on the carriers, into the tower, down to 24-A ----
    {
        auto R = buildCold();
        for (int d = 0; d < (int)R.doors.size(); ++d) { R.doors[d].locked = false; R.setDoorInstant(d, true); }   // (their fights won)
        Walk w(R, cold::start());
        bool ok = w.to({cold::HOOK_X + 3.f, 0.f, cold::lipZ() + 30.f}, true) && w.to(cold::hookStart(), true, 9000, 0.8f);
        CHECK(ok && w.p.position.y > -0.5f, "from the arrival you can walk out onto the Hook Line's gantry");
        // The Hook Line: a chain of hooks, each within an easy grapple of the last, hung where you can hang
        // under them, the first in reach from the gantry and the last over the dock
        bool hooks = R.story.coldHookCount == cold::HOOKS;
        for (int k = 0; k < cold::HOOKS && hooks; ++k) {
            const glm::vec3 h = cold::hookAt(k), hang = cold::hangAt(k);
            hooks &= R.walls[R.story.coldHooks + k].box.min.y > hang.y + 1.9f;   // (your head clears it, hanging)
            if (k > 0) hooks &= glm::length(h - cold::hookAt(k - 1)) < 26.f;
            AABB body{hang + glm::vec3{-0.4f, 0.f, -0.4f}, hang + glm::vec3{0.4f, 1.8f, 0.4f}};
            for (const auto& wl : R.walls) if (body.min.x < wl.box.max.x && body.max.x > wl.box.min.x && body.min.y < wl.box.max.y && body.max.y > wl.box.min.y && body.min.z < wl.box.max.z && body.max.z > wl.box.min.z) hooks = false;
        }
        CHECK(hooks, "the Hook Line's hooks are under 26 m apart, and you hang under each clear of anything");
        CHECK(glm::length(cold::hookAt(0) - (cold::hookStart() + glm::vec3{0.f, 1.7f, 0.f})) < 25.f, "...the first is in reach from the gantry's end");
        const glm::vec3 last = cold::hangAt(cold::HOOKS - 1);
        CHECK(std::fabs(last.x - cold::HOOK_X) < 5.f && last.z < cold::stationZ(cold::STATIONS) + cold::STATION_HALF && last.z > cold::TOWER_FRONT, "...and the last hangs over the dock: let go and you land on it");
        // (the grapple is the game's; the test drops you onto the dock as letting go of the last hook would)
        w.p.position = last; w.p.velocity = glm::vec3{0.f};
        for (int f = 0; f < 120; ++f) w.step(last, false, true);
        bool rode = ok && w.p.onGround && w.p.position.y > -0.5f;
        CHECK(rode, "let go of the last hook and you land on the tower's dock");
        bool inside = rode && w.to({cold::HOOK_X, 0.f, cold::TOWER_FRONT - 5.f}, true) && w.to({cold::HOOK_X, 0.f, cold::THAW_BACK + 3.f}, true)
                   && w.to({cold::HOOK_X, 0.f, cold::INV_BACK + 3.f}, true) && w.to({cold::HOOK_X, 0.f, cold::INV_BACK - 6.f}, true);
        CHECK(inside && w.p.position.y > -0.5f, "through the Thaw Room and Inventory into Rack 24, on the tower's floor");
        bool reach = inside && w.to({cold::rackSlot(0).x + 2.2f, 0.f, cold::rackSlot(0).z}, true);
        CHECK(reach && glm::length(w.p.camera.position - cold::recallAt()) < Interactable::INTERACT_RANGE, "24-A's RECALL module is within reach from the aisle");
        // Out of the back door, over the transfer bridge to the Blast Chillers, across its decks
        bool chill = reach && w.to({cold::HOOK_X, 0.f, cold::TOWER_BACK - 4.f}, true) && w.to({cold::HOOK_X, 0.f, -830.f}, true)
                  && w.to(cold::chiller(0).c+glm::vec3{-18,0,6},true) && w.to(cold::chiller(0).c+glm::vec3{-4,0,6},true)
                  && w.to(cold::chiller(0).c, true) && w.to(cold::chiller(1).c, true, 9000, 0.8f) && w.to(cold::chiller(2).c, true, 9000, 0.8f);
        CHECK(chill && w.p.position.y > -0.5f, "out of Rack 24's back door, over the transfer bridge and across the three cooling-tower decks");
        bool sea = chill && w.to({cold::chiller(2).c.x + 20.f, 0.f, -830.f}, true) && w.to(cold::seaStart(), true);
        CHECK(sea && R.arenaAt(w.p.position) == 6, "through the Chillers' gate and onto the frozen sea");
        bool station = sea && w.to(cold::station() + glm::vec3{0.f, 0.f, 2.f}, true);
        CHECK(station && w.p.position.y > 0.3f, "across the ice and up onto the far shore's station (24-K's)");
        // The Manifest: the causeway out over the sinkhole, to the Vault
        const glm::vec3 vc = cold::vaultC(), st = cold::station();
        const glm::vec3 a = st + glm::vec3{18.f, 0.f, -2.f}, e = vc + glm::normalize(st - vc) * (cold::VAULT_R - 1.f);
        bool walk = station && w.to(a, true) && w.to(glm::mix(a, e, 0.5f), false, 9000, 0.6f) && w.to(e, false, 9000, 0.6f) && w.to(vc, true);
        if (!walk || w.p.position.y < -0.5f) std::printf("  causeway: at %.1f %.1f %.1f\n", w.p.position.x, w.p.position.y, w.p.position.z);
        CHECK(walk && w.p.position.y > -0.5f, "along the Manifest's causeway over the sinkhole and onto the Vault");
        bool inEnd = walk && w.to({vc.x, 0.f, R.story.end.min.z + 3.f}, true) && w.p.position.x > R.story.end.min.x && w.p.position.x < R.story.end.max.x
                     && w.p.position.z > R.story.end.min.z && w.p.position.z < R.story.end.max.z;
        CHECK(inEnd, "...and to its dispatch rail on the north edge, where the chapter ends");
        CHECK(R.baseFloor(vc.x, vc.z) == 0.f && R.baseFloor(vc.x + 50.f, vc.z) == cold::VOID_FLOOR && R.baseFloor(vc.x + 90.f, vc.z) == 0.f,
              "the Vault stands, the sinkhole round it drops into the Void, the sea ice beyond holds");
        // The rooms' spawns are on open floor, and enemies find the tower's floor, not the Void's
        bool open = true;
        for (auto& a : R.arenas) for (auto& wave : a.waves) for (auto& e : wave) for (auto q : e.at) {
            float r = statsOf(e.type).radius, h = statsOf(e.type).height;
            AABB bb{q + glm::vec3{-r, .08f, -r}, q + glm::vec3{r, h, r}};
            for (const auto& wl : R.walls) if (!wl.dynamic && bb.min.x < wl.box.max.x && bb.max.x > wl.box.min.x && bb.min.y < wl.box.max.y && bb.max.y > wl.box.min.y && bb.min.z < wl.box.max.z && bb.max.z > wl.box.min.z)
                { open = false; std::printf("  blocked %s at %.1f %.1f %.1f\n", statsOf(e.type).name, q.x, q.y, q.z); break; }
        }
        CHECK(open, "every placed spawn in the route stands clear of walls");
        bool cycle = true;
        for (int v = 0; v < cold::VENTS; ++v) {
            int prev = cold::ventState(v, 0.f), tells = 0, blasts = 0; bool told = prev == 1, ok = true;
            for (float t = 0.01f; t < cold::VENT_PERIOD * 3.f; t += 0.01f) {
                int st = cold::ventState(v, t);
                if (st == 1 && prev != 1) { ++tells; told = true; }
                if (st == 2 && prev != 2) { ++blasts; ok &= told && prev == 1; told = false; }
                prev = st;
            }
            cycle &= ok && tells >= 2 && blasts >= 2;
        }
        CHECK(cycle, "every Blast Chiller vent tells (red) before each freezing blast, every cycle");
        CHECK(R.baseFloor(500.f, -900.f) == 0.f && R.baseFloor(cold::SEA_X0 - 20.f, -900.f) == cold::VOID_FLOOR, "the frozen sea holds you up; just west of it is still the Void");
        CHECK(R.baseFloor(-40.f, -620.f) == 0.f && R.enemyFloor(2, {-40.f, 0.f, -620.f}, false, 0.f) == 0.f && R.enemyFloor(1, {-40.f, 8.f, -450.f}, true, 0.f) == 0.f,
              "inside the tower the floor is its own (not the Void's), and Pickers over the Hook Line keep to the deck's height");
    }

    // ---- what you see ----
    CHECK(cold::giants().size() >= 4, "frozen giants stand round the horizon");
    bool farEnough = true;
    for (const auto& g : cold::giants()) farEnough &= glm::length(glm::vec2{g.pos.x, g.pos.z}) > 1500.f && glm::length(glm::vec2{g.pos.x, g.pos.z}) + g.height < cold::FAR_PLANE;
    CHECK(farEnough, "...far off the shelf, and inside what the camera can see");
    bool towersOver = true;
    for (const auto& t : cold::towers()) towersOver &= t.base.z + t.half < cold::EDGE_Z - 300.f && t.base.y + t.height > 300.f;
    CHECK(towersOver, "the Store's towers rise out of the Void across the chasm, well above the shelf");
    {   // ...and none stands where you walk (they are scenery: no collision, so one in the way is a black wall you pass through)
        auto R = buildCold();
        bool clear = true;
        for (const auto& t : cold::towers())
            for (int a = 0; a < (int)R.arenas.size(); ++a) {
                AABB z = R.arenas[a].zone;
                if (a == 0) z.max.z = std::min(z.max.z, cold::SOUTH);
                bool hit = t.base.x + t.half + 20.f > z.min.x && t.base.x - t.half - 20.f < z.max.x && t.base.z + t.half + 20.f > z.min.z && t.base.z - t.half - 20.f < z.max.z;
                if (hit && a != 0) { clear = false; std::printf("  tower at %.0f %.0f inside section %d\n", t.base.x, t.base.z, a); }
            }
        CHECK(clear, "no Store tower stands within 20 m of any section you walk through");
    }
    CHECK(glm::dot(cold::giantDir(), glm::vec3{0.f, 0.f, -1.f}) > 0.7f && cold::giantDir().y > 0.2f, "the gas giant hangs over the Store, clear of the horizon");

    // ---- the ground ----
    for (int which = 0; which < 2; ++which) {
        auto px = which ? MaterialTex::ice() : MaterialTex::snow();
        const int S = MaterialTex::SIZE;
        double seamX = 0.0, seamY = 0.0, mean = 0.0;
        for (int y = 0; y < S; ++y) for (int c = 0; c < 3; ++c) seamX += std::abs((int)px[(y * S) * 4 + c] - (int)px[(y * S + S - 1) * 4 + c]);
        for (int x = 0; x < S; ++x) for (int c = 0; c < 3; ++c) seamY += std::abs((int)px[x * 4 + c] - (int)px[((S - 1) * S + x) * 4 + c]);
        for (int i = 0; i < S * S; ++i) mean += px[i * 4];
        mean /= S * S;
        std::printf("      %s: edge difference %.1f / %.1f, mean red %.0f\n", which ? "ice" : "snow", seamX / (S * 3), seamY / (S * 3), mean);
        CHECK(seamX / (S * 3) < 40.0 && seamY / (S * 3) < 40.0, which ? "the ice texture wraps without a hard seam" : "the snow texture wraps without a hard seam");
    }

    // ---- the enemies ----
    {   // THAWLING: crawls, then stands and gets quicker as it warms
        EnemyWorld w; w.playerFeet = {30.f, 0.f, 0.f}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
        Enemy e(EnemyType::THAWLING, {0.f, 0.f, 0.f});
        e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
        float crawlX = 0.f, earlyAttacks = 0;
        for (int f = 0; f < (int)(60 * coldfoe::THAW_RISE) - 2; ++f) { e.update(DT, w); earlyAttacks += e.ev.telegraphStarted; }
        crawlX = e.position.x;
        float x0 = e.position.x; for (int f = 0; f < 60; ++f) e.update(DT, w); float coldStep = e.position.x - x0;
        for (int f = 0; f < 60 * 12; ++f) { e.update(DT, w); if (e.position.x > 20.f) e.position.x = 5.f; }
        float x1 = e.position.x; for (int f = 0; f < 60; ++f) e.update(DT, w); float warmStep = e.position.x - x1;
        std::printf("      thawling: crawled %.1f m, cold %.1f m/s, warm %.1f m/s\n", crawlX, coldStep, warmStep);
        CHECK(earlyAttacks == 0 && crawlX > 0.5f && crawlX < 6.f, "a THAWLING crawls at first, slowly and without attacking");
        CHECK(warmStep > coldStep * 1.4f, "...and is much quicker once it has warmed up");
    }
    {   // PICKER: a white tell over you, then the drop takes a player who stays put
        EnemyWorld w; w.playerFeet = {6.f, 0.f, 0.f}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
        Enemy e(EnemyType::PICKER, {0.f, 4.6f, 0.f}, 0.f);
        e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
        bool tell = false, white = false, grabbed = false;
        for (int f = 0; f < 60 * 15 && !grabbed; ++f) {
            e.update(DT, w);
            tell |= e.attack == AttackKind::SORT_GRAB && e.telegraphTimer > 0.f;
            white |= e.parryWindow();
            grabbed |= e.ev.sortGrab;
        }
        CHECK(tell && white, "a PICKER winds its snatch up with a parry window");
        CHECK(grabbed && e.pickT > 0.f, "...and its drop takes you if you stay under it");
        e.stun(1.f);
        CHECK(e.pickT <= 0.f, "stunned (punched), it lets go");
    }
    {   // Hit registration: a shot at what is drawn lands (the boxes round the models, not a cylinder at the feet)
        auto ray = [](const AABB& b, glm::vec3 at) {   // a level shot from 10 m south at this point
            glm::vec3 o = at + glm::vec3{0.f, 0.f, 10.f}; return rayBoxHit(o, {0.f, 0.f, -1.f}, b) > 0.f;
        };
        Enemy p(EnemyType::PICKER, {0.f, 4.6f, 0.f}, 0.f);
        CHECK(ray(p.getAABB(), p.position + glm::vec3{0.f, 1.25f, 0.f}) && ray(p.getAABB(), p.position + glm::vec3{0.f, 1.85f, 0.f}),
              "a PICKER's body and wheels (drawn above its feet point) take a shot");
        Enemy t(EnemyType::THAWLING, {0.f, 0.f, 0.f}, 0.f); t.yaw = 0.f; t.age = 0.5f;
        CHECK(ray(t.getAABB(), {0.f, 0.3f, 1.4f}) || rayBoxHit({10.f, 0.3f, 1.4f}, {-1.f, 0.f, 0.f}, t.getAABB()) > 0.f, "a crawling THAWLING's body, lying forward on the floor, takes a shot");
        Enemy r(EnemyType::RESTORER, {0.f, 0.f, 0.f}, 0.f); r.yaw = 0.f;
        AABB head; bool hasHead = headBox(r, head);
        const glm::vec3 dome = rsbody::domeWorld(r);
        CHECK(hasHead && ray(head, dome) && ray(r.getAABB(), dome + glm::vec3{0.f, 0.5f, 0.f}), "THE RESTORER's glass dome (its weak point) takes a shot");
        r.rsKneel = 1.f; const glm::vec3 low = rsbody::domeWorld(r);
        CHECK(headBox(r, head) && ray(head, low) && ray(r.getAABB(), low) && low.y < dome.y - 1.5f, "...and still does when it kneels, its dome brought down to you");
    }
    {   // 24-K is drawn at its full frame (bigger than you), and its hitbox covers what is drawn
        Enemy k(EnemyType::UNIT24K, {0.f, 0.f, 0.f}, 0.f); k.spawnTimer = 0.f; k.state = EnemyState::ACTIVE;
        auto headY = [](const Enemy& e) {   // where its head's centre is drawn (its parts' centres: the highest)
            std::vector<BoxInstance> out; buildEnemy(e, 1.f, out);
            float top = 0.f; for (const auto& b : out) top = std::max(top, b.model[3].y);
            return top;
        };
        const float big = headY(k); Enemy one = k; one.scale = 1.f; const float small = headY(one);
        std::printf("      24-K's head drawn at %.2f m (%.2f m at your size); hitbox %.2f m\n", big, small, k.height());
        CHECK(big > small * 1.4f && big > 2.4f && big < k.height(), "24-K is drawn at its full frame, well over your height, inside its hitbox");
    }
    {   // CLERK: never attacks, keeps its distance
        EnemyWorld w; w.playerFeet = {4.f, 0.f, 0.f}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
        Enemy e(EnemyType::CLERK, {0.f, 3.4f, 0.f}, 0.f);
        e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
        bool attacked = false;
        for (int f = 0; f < 60 * 10; ++f) { e.update(DT, w); attacked |= e.ev.telegraphStarted || e.ev.shots > 0 || e.ev.meleeHit; }
        float d = glm::length(glm::vec2{e.position.x - 4.f, e.position.z});
        CHECK(!attacked, "a CLERK never attacks");
        CHECK(d > 9.f, "...and backs off to keep its distance from you");
    }

    {   // UNIT 24-K: its three attacks, each with a tell; the Lancer's round is heavy; the rush can be parried
        EnemyWorld w;
        Enemy e(EnemyType::UNIT24K, {0.f, 0.f, 0.f});
        e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
        bool burst = false, shot = false, dash = false, heavy = false, white = false, enraged = false;
        int shots = 0;
        for (int f = 0; f < 60 * 60; ++f) {
            float dist = (f / 600) % 2 ? 4.f : 12.f;   // close, then far, in turns
            w.playerFeet = e.position + glm::vec3{dist, 0.f, 0.f}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
            if (f == 60 * 40) e.takeDamage(e.maxHealth * 0.55f);
            e.update(DT, w);
            if (e.ev.telegraphStarted) { burst |= e.attack == AttackKind::BURST; shot |= e.attack == AttackKind::SHOT; dash |= e.attack == AttackKind::DASH; }
            heavy |= e.ev.shots > 0 && e.ev.shotHeavy;
            shots += e.ev.shots;
            white |= e.attack == AttackKind::DASH && e.parryWindow();
            enraged |= e.ev.enraged || e.enraged;
        }
        std::printf("      24-K: burst %d, Lancer %d, rush %d, %d rounds fired\n", burst, shot, dash, shots);
        CHECK(isBoss(EnemyType::UNIT24K) && burst && shot && dash, "24-K duels with the revolver, the Lancer and a rush, each told first");
        CHECK(heavy && white, "...its Lancer round is heavy, and its rush has a white parry window");
        CHECK(enraged, "...and half down, it stops holding back");
        auto R = buildCold();
        CHECK(R.arenas[7].waves.size() == 1 && R.arenas[7].waves[0][0].type == EnemyType::UNIT24K,
              "24-K waits at DISPATCH 2, the eighth section");
    }

    {   // THE RESTORER: phase 1 rolls back, snaps and grabs; phase 2 adds frost and racks; every one told first
        EnemyWorld w;
        Enemy e(EnemyType::RESTORER, {0.f, 0.f, 0.f});
        e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
        std::set<int> p1, p2; bool white = false;
        for (int f = 0; f < 60 * 120; ++f) {
            float dist = (f / 300) % 2 ? 6.f : 12.f;
            w.playerFeet = e.position + glm::vec3{dist, 0.f, 0.f}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
            if (f == 60 * 60) e.rsPhase = 2;
            e.update(DT, w);
            if (e.ev.telegraphStarted) (e.rsPhase == 1 ? p1 : p2).insert((int)e.attack);
            white |= e.attack == AttackKind::RS_TONG && e.parryWindow();
        }
        auto has = [](const std::set<int>& s, AttackKind k) { return s.count((int)k) > 0; };
        CHECK(isBoss(EnemyType::RESTORER) && has(p1, AttackKind::RS_ROLLBACK) && has(p1, AttackKind::RS_SNAPSHOT) && has(p1, AttackKind::RS_TONG)
              && !has(p1, AttackKind::RS_FREEZE) && !has(p1, AttackKind::RS_SHELVE), "the Restorer's first phase: ROLLBACK, SNAPSHOT and the tongs, nothing more");
        CHECK(has(p2, AttackKind::RS_FREEZE) && has(p2, AttackKind::RS_SHELVE), "...its second adds FLASH FREEZE and SHELVING");
        CHECK(white, "...and its tongs can be parried");
        Enemy f(EnemyType::RESTORER, {0.f, 0.f, 0.f});
        f.spawnTimer = 0.f; f.state = EnemyState::ACTIVE;
        bool died = f.takeDamage(f.maxHealth * 2.f);
        for (int k = 0; k < 120; ++k) f.update(DT, w);
        CHECK(!died && f.alive && f.rsFalter && f.health > 0.f && f.rsKneel >= 1.f && f.attack == AttackKind::NONE,
              "beaten, the Restorer does not die: it falters onto its knees and waits for the RECALL finisher");
        f.rsFinished = true;
        CHECK(f.takeDamage(f.health + 10.f) && !f.alive, "...and once the module is in its dome, it goes");
        Enemy pt(EnemyType::RS_POINT, {0.f, 0.f, 0.f}), dr(EnemyType::RS_DRUM, {0.f, 0.f, 0.f});
        pt.spawnTimer = dr.spawnTimer = 0.f; pt.state = dr.state = EnemyState::ACTIVE;
        bool still = true;
        for (int f = 0; f < 600; ++f) { pt.update(DT, w); dr.update(DT, w); still &= !pt.ev.telegraphStarted && !dr.ev.telegraphStarted && glm::length(pt.position) < 0.01f && glm::length(dr.position) < 0.01f; }
        CHECK(still, "its restore points and storage drums stand still and never attack");
        auto R = buildCold();
        CHECK(R.story.bossSection == 9 && R.arenas[9].waves[0][0].type == EnemyType::RESTORER && R.story.coldPoolCount == 11,
              "it waits in the Vault (the tenth section), with walls set aside for its statues and racks");
    }

    {   // Chapter 5 is reached from Chapter 4's end, and saves like the others
        auto P = buildPlain();
        CHECK(P.story.nextChapter == 5 && std::string(P.story.nextName) == "THE COLD STORE", "Chapter 4's ending leads on to THE COLD STORE");
        StorySave s; s.chapter = 5; s.checkpoint = 9; StorySave back;
        CHECK(back.parse(s.text()) && back.chapter == 5 && back.checkpoint == 9, "a Chapter 5 save at the Vault round-trips");
        s.checkpoint = 10; CHECK(!back.parse(s.text()), "...and a checkpoint past its last section is rejected");
    }

    {   // THE THAW ROOM (docs/gameplay-standard.md's worked example): a catwalk down each long wall, walked up
        // without a jump from every corner and along its whole length by a real player; the heat by the heaters
        LevelData L = buildCold(); SpatialGrid g; g.build(L.walls);
        struct Climb { glm::vec3 from; float yaw; } climbs[] = {{{-66.f, 0.f, -602.6f}, 180.f}, {{-66.f, 0.f, -648.f}, 180.f}, {{-14.f, 0.f, -602.6f}, 0.f}, {{-14.f, 0.f, -648.f}, 0.f}};
        bool up = true, along = true, clear = true;
        for (const auto& c : climbs) {
            Player p(c.from); p.camera.yaw = c.yaw;
            auto walk = [&](int ticks) {
                for (int i = 0; i < ticks; ++i) {
                    Uint8 k[SDL_NUM_SCANCODES]; std::memset(k, 0, sizeof(k)); k[SDL_SCANCODE_W] = 1;
                    p.floorY = L.baseFloor(p.position.x, p.position.z, p.radius);
                    p.update(DT, k, L.walls.data(), (int)L.walls.size(), false, &g);
                    clear &= !bodyEmbedded(L.walls, L.moverWalls, g, p.position, p.radius, p.height);
                }
            };
            walk(240);
            up &= std::fabs(p.position.y - cold::THAW_DECK_Y) < 0.05f;
            p.camera.yaw = c.from.z > -620.f ? -90.f : 90.f;
            walk(480);
            along &= std::fabs(p.position.y - cold::THAW_DECK_Y) < 0.05f && std::fabs(p.position.z - c.from.z) > 45.f;
        }
        CHECK(up, "the Thaw Room's catwalks: walked up from all four corners, no jump needed");
        CHECK(along && clear, "...and walked end to end, 4.2 m up, catching on nothing");
        CHECK(cold::inThawHeat({-77.f, 0.f, -613.f}) && cold::inThawHeat({-2.f, 0.f, -641.f}) && !cold::inThawHeat({-40.f, 0.f, -625.f}) &&
              !cold::inThawHeat({-77.f, 4.2f, -613.f}), "the heat is by the heaters on the floor, not in the room's middle or up on the catwalk");
    }

    {   // ROLLBACK winds the score back: a short backwards run of what just played, then half a bar earlier
        MusicSynth m; m.setTrack(16); m.setIntensity(1.f);
        std::vector<float> buf(2 * 4410);
        for (int i = 0; i < 30; ++i) m.render(buf.data(), 4410);   // 3 s in
        double before = m.stepPos();
        m.rewind();
        float peak = 0.f; bool finite = true; int blocks = 0;
        while (blocks < 20) { m.render(buf.data(), 4410); ++blocks; for (float v : buf) { finite &= std::isfinite(v); peak = std::max(peak, std::fabs(v)); } if (!m.rewinding()) break; }
        double after = m.stepPos(), moved = before - after; if (moved < 0) moved += 64;
        CHECK(blocks >= 5 && blocks <= 7, "the rewind lasts about 0.6 s");
        CHECK(finite && peak > 0.01f && peak <= 1.f, "...is heard, and stays clean");
        CHECK(moved > 7.f && moved < 9.f, "...and the song plays on from half a bar back");
    }

    std::printf(failures ? "\n%d FAILED\n" : "\nALL PASSED (0 failures)\n", failures);
    return failures ? 1 : 0;
}
