#include "core/Balance.h"
#include "player/Player.h"
#include "player/Projectile.h"
#include "world/LevelStory.h"
#include "world/LevelWasteland.h"
#include "world/StoryBossArena.h"
#include "bosses/ballast/BallastHazards.h"
#include "render/EnemyModel.h"
#include <cstdio>
#include <set>
static int failures=0;
#define CHECK(c,m) do{if(!(c)){++failures;std::printf("FAIL: %s\n",m);}else std::printf("ok: %s\n",m);}while(0)
constexpr float STEP=1.f/60.f;

struct GunRun { float seconds=0,hp=100,taken=0,closest=1e9f;int hits=0,parries=0;bool won=false; };
// Real acceleration, jump/gravity/collision, hitscan occlusion, ammo/reloads,
// enemy projectiles, damage immunity and on-hit healing. The passive policy
// circles/backpedals and dashes periodically; the reactive one reads tool cues.
// Boss potion rewards are granted immediately (a generous survival assumption).
static GunRun gunRun(bool road,bool reactive,WeaponId weapon) {
    LevelData level=road?buildWasteland():buildCradle();int section=level.story.bossSection;
    const auto arena=level.arenas[section];SpatialGrid grid;grid.build(level.walls);
    Enemy boss(road?EnemyType::BALLAST:EnemyType::SORTER,arena.bossSpawn);boss.spawnTimer=0;boss.state=EnemyState::ACTIVE;
    boss.maxHealth*=difficulty(DIFFICULTY_DEFAULT).health;boss.health=boss.maxHealth;
    Player player(arena.playerStart);const float floor=arena.playerStart.y;
    ProjectileSystem shots;shots.floorY=floor;std::vector<Enemy> others;BallastHazards fronts;
    WeaponState gun;WeaponUpgrades upgrades;gun.ammo=weaponMag(weapon,upgrades);GunRun run;
    float immunity=0,recharge=0,lastDash=-10;int charges=2,potions=0;
    auto hurt=[&](float amount){if(immunity>0)return;float damage=amount*difficulty(DIFFICULTY_DEFAULT).damage;run.hp-=damage;run.taken+=damage;immunity=.32f;};
    for(int frame=0;frame<60*220&&run.hp>0&&boss.alive;++frame) {
        float t=frame*STEP;run.seconds=t;immunity=std::max(0.f,immunity-STEP);
        if(charges<2){recharge+=STEP;if(recharge>=1.2f){++charges;recharge=0;}}
        glm::vec3 delta=boss.position-player.position;delta.y=0;float distance=glm::length(delta);
        glm::vec3 toward=distance>.01f?delta/distance:glm::vec3{0,0,-1},side{-toward.z,0,toward.x};
        glm::vec3 move=distance<22?-toward:distance>32?toward:side;
        bool jump=false,dodge=false;
        if(reactive) {
            if(boss.type==EnemyType::SORTER) {
                glm::vec3 danger=boss.sortAim;float radius=Enemy::SORT_STAMP_R;
                bool press=(boss.attack==AttackKind::SORT_STAMP||boss.attack==AttackKind::SORT_GRAB)&&boss.telegraphTimer>0&&boss.telegraphTimer<.6f;
                for(const auto& head:boss.sortPresses)if(head.remaining>0&&head.remaining<.65f&&glm::length(head.at-player.position)<radius+2){danger=head.at;press=true;}
                if(press&&glm::length(danger-player.position)<radius+2){move=player.position-danger;move.y=0;dodge=true;}
                if((boss.attack==AttackKind::SORT_RUSH&&boss.telegraphTimer<.6f)||boss.sortRush>0) {
                    glm::vec3 dir=boss.sortRushTo-boss.position;dir.y=0;
                    if(glm::length(dir)>.01f&&machineryDistance(player.position,boss.position,boss.sortRushTo)<5){dir=glm::normalize(dir);move={-dir.z,0,dir.x};dodge=true;}
                }
                if((boss.attack==AttackKind::SORT_PURGE||boss.sortFire>0)&&boss.sortVentCount>0)for(int i=0;i<boss.sortVentCount;++i) {
                    const auto& b=boss.sortVents[i];auto p=player.position;
                    if(p.x>b.min.x-1&&p.x<b.max.x+1&&p.z>b.min.z-1&&p.z<b.max.z+1){move={0,0,p.z>(b.min.z+b.max.z)*.5f?1.f:-1.f};dodge=true;}
                }
            } else {
                if((boss.attack==AttackKind::BALL_GRADE&&boss.telegraphTimer<.6f)||boss.ballDrive>0) {
                    auto dir=boss.ballAim-boss.ballFrom;dir.y=0;
                    if(glm::length(dir)>.01f&&machineryDistance(player.position,boss.ballFrom,boss.ballAim)<5){dir=glm::normalize(dir);move={-dir.z,0,dir.x};dodge=true;jump=true;}
                }
                if(boss.attack==AttackKind::BALL_ANCHOR&&boss.telegraphTimer<.55f&&glm::length(boss.ballAim-player.position)<4.5f){move=player.position-boss.ballAim;move.y=0;dodge=true;}
                if(boss.attack==AttackKind::BALL_SWEEP&&distance<8&&boss.telegraphTimer<.6f)jump=true;
                for(const auto& front:fronts.fronts) {
                    float along=BallastHazards::along(front.from,front.dir,player.position)-front.distance;
                    if(along>0&&along<9&&BallastHazards::across(front.from,front.dir,player.position)<BallastHazards::halfWidth(front)+1){
                        move={-front.dir.z,0,front.dir.x};jump|=front.rubble;dodge=true;
                    }
                }
            }
            glm::vec3 catchAt=road?boss.ballAim:boss.sortAim;
            if(boss.parryWindow()&&glm::length(catchAt-player.position)<3.8f){boss.stagger(boss.staggerTime());++run.parries;}
        }
        if(glm::length(move)<.01f)move=side;move=glm::normalize(move);
        if(charges>0&&t-lastDash>(reactive?.4f:1.6f)&&(dodge||!reactive)) {
            player.velocity.x=move.x*26;player.velocity.z=move.z*26;--charges;lastDash=t;immunity=.25f;
        }
        Uint8 keys[SDL_NUM_SCANCODES]={};keys[SDL_SCANCODE_W]=1;keys[SDL_SCANCODE_SPACE]=jump;
        player.camera.yaw=glm::degrees(std::atan2(move.z,move.x));player.floorY=floor;
        auto before=player.position;player.update(STEP,keys,level.walls.data(),(int)level.walls.size(),false,&grid);
        player.position.x=glm::clamp(player.position.x,arena.bounds.min.x+.6f,arena.bounds.max.x-.6f);
        player.position.z=glm::clamp(player.position.z,arena.bounds.min.z+.6f,arena.bounds.max.z-.6f);
        EnemyWorld w;w.bounds=arena.bounds;w.playerFeet=player.position;w.playerEye=player.position+glm::vec3{0,1.7f,0};w.playerVel=(player.position-before)/STEP;
        w.walls=level.walls.data();w.wallCount=(int)level.walls.size();w.grid=&grid;w.tune=&difficulty(DIFFICULTY_DEFAULT);boss.floorY=floor;
        if((road&&boss.ballDrive>0)||(!road&&boss.sortRush>0)) {
            auto dir=road?boss.ballDir:boss.sortRushDir;
            if(!shatterStoryCover(level,boss.position+glm::vec3{0,1,0},boss.position+glm::vec3{0,1,0}+dir*(36*STEP),4).empty())grid.build(level.walls);
        }
        boss.update(STEP,w);run.closest=std::min(run.closest,glm::length(glm::vec2{boss.position.x-player.position.x,boss.position.z-player.position.z}));
        const auto& ev=boss.ev;if(ev.meleeHit)hurt(ev.meleeDamage);
        if(ev.ballGust)fronts.add(ev.ballFrom,ev.ballTo);if(ev.ballRip)fronts.addRip(ev.ballFrom,ev.ballTo);
        for(const auto& hit:fronts.update(STEP,player.position,level.walls))hurt(hit.damage);
        for(int k=0;k<ev.shots;++k)shots.fire(ev.shotOrigin,ev.shotDir[k]*ev.shotSpeed,ev.shotDamage,false,{1,1,1},false,0,ev.shotSize,true);
        auto result=shots.update(STEP,level.walls.data(),(int)level.walls.size(),others,w.playerEye,&grid);
        if(result.hitPlayer)hurt(result.playerDamage);
        if(run.hp<=0)break;
        gun.tick(STEP,weaponMag(weapon,upgrades));
        if(gun.ammo==0&&!gun.reloading)gun.startReload(weaponReload(weapon,upgrades));
        if(gun.ready()) {
            --gun.ammo;gun.cooldown=weaponCooldown(weapon,upgrades);
            auto hull=boss.getAABB();glm::vec3 target=(hull.min+hull.max)*.5f;
            AABB weak;bool aimWeak=reactive&&woundBox(boss,weak);if(aimWeak)target=(weak.min+weak.max)*.5f;
            glm::vec3 ray=target-w.playerEye;float length=glm::length(ray);ray/=std::max(.001f,length);
            float wall=weaponDef(weapon).range;for(const auto& b:level.walls){float hit=rayBoxHit(w.playerEye,ray,b.box);if(hit>0)wall=std::min(wall,hit);}
            if(length<wall+.05f){float woundT=0;float mult=woundShot(boss,w.playerEye,ray,woundT)?boss.woundMult():1.f;
                boss.takeDamage(weaponDamage(weapon,upgrades)*boss.armorMult()*mult);run.hp=std::min(100.f,run.hp+2*difficulty(DIFFICULTY_DEFAULT).heal);++run.hits;}
            int due=std::min(2,int((1-boss.health/boss.maxHealth)/(1.f/3.f)+.0001f));
            while(potions<due){++potions;run.hp=std::min(100.f,run.hp+40.f);}
        }
    }
    run.won=!boss.alive;return run;
}
static void toolChecks() {
    EnemyWorld w;w.bounds={{-60,-2,-60},{60,30,60}};w.playerFeet={0,0,25};w.playerEye={0,1.7f,25};w.playerVel={8,0,0};
    auto ready=[](EnemyType type){Enemy e(type,{0,type==EnemyType::SORTER?2.f:0.f,0});e.state=EnemyState::ACTIVE;e.spawnTimer=0;e.sortPhase=e.ballPhase=1;e.sortHome=e.position;e.sortHomeSet=true;return e;};
    for(bool road:{false,true}) {
        auto boss=ready(road?EnemyType::BALLAST:EnemyType::SORTER);
        if(road)boss.ballFollow=AttackKind::BALL_GRADE;else boss.sortNext=AttackKind::SORT_RUSH;
        boss.update(STEP,w);
        for(int i=0;i<120&&boss.telegraphTimer>.4f;++i)boss.update(STEP,w);
        auto locked=road?boss.ballAim:boss.sortRushTo;
        w.playerFeet={-30,0,12};w.playerEye=w.playerFeet+glm::vec3{0,1.7f,0};w.playerVel={-14,0,0};
        for(int i=0;i<120&&boss.telegraphTimer>0;++i)boss.update(STEP,w);
        auto committed=road?boss.ballAim:boss.sortRushTo;
        CHECK(glm::length(locked-committed)<.001f,"a rush commits before release and cannot track a late reversal");
        CHECK(road?boss.ballDrive>0:boss.sortRush>0,"the committed warning produces an actual pursuing rush");
        boss.stagger(1);CHECK(boss.ballDrive==0&&boss.sortRush==0&&boss.ballFollow==AttackKind::NONE&&boss.sortNext==AttackKind::NONE,"a parry cancels both pursuit and its queued follow-up");
        w.playerFeet={0,0,25};w.playerEye={0,1.7f,25};w.playerVel={8,0,0};
    }
    {
        auto boss=ready(EnemyType::SORTER);boss.sortNext=AttackKind::SORT_ROW;boss.update(STEP,w);
        CHECK(boss.sortPresses.size()==3,"the press row warns all three physical heads before the first impact");
        boss.stagger(1);for(int i=0;i<120;++i)boss.update(STEP,w);
        CHECK(boss.sortPresses.empty()&&!boss.ev.sortStamp,"parrying cancels pending press heads throughout the stagger");
        CHECK(machinerySweepHits({0,0,5},{0,0,0},{0,0,10},2,0,1.7f)&&!machinerySweepHits({0,2,5},{0,0,0},{0,0,10},2,0,1.7f),"a real jump clears the low machinery sweep");
    }
    for(bool road:{false,true}) {
        auto boss=ready(road?EnemyType::BALLAST:EnemyType::SORTER);boss.sortStep=1;boss.attackTimer=100;
        EnemyWorld high=w;high.playerFeet={0,9,25};high.playerEye=high.playerFeet+glm::vec3{0,1.7f,0};high.playerVel={7,4,0};
        boss.update(STEP,high);
        CHECK(boss.attack==(road?AttackKind::BALL_WINCH:AttackKind::SORT_THROW),"an elevated ranged target gets an airborne-capable tool rather than a grounded sweep");
        if(road) {
            for(int i=0;i<100&&boss.telegraphTimer>.2f;++i)boss.update(STEP,high);
            CHECK(boss.ballAim.y>9&&boss.parryWindow(),"the harpoon reaches flight height and provides its advertised white parry window");
            boss.stagger(1);CHECK(boss.ballTether==0&&boss.ballFollow==AttackKind::NONE,"parrying the harpoon cancels the cable and follow-up");
        } else {
            bool upward=false;for(int i=0;i<100&&!upward;++i){boss.update(STEP,high);if(boss.ev.shots)upward=boss.ev.shotDir[0].y>0;}
            CHECK(upward,"the scrap ejector actually fires upward at elevated movement");
        }
    }
    {
        BallastHazards ground,air,covered;ground.addRip({0,0,0},{0,0,40});air=ground;covered=ground;
        std::vector<Wall> empty,walls;Wall shield;shield.box={{-10,0,5},{10,4,6}};walls.push_back(shield);
        int hits=0,airHits=0,coverHits=0;
        for(int i=0;i<50;++i){hits+=(int)ground.update(STEP,{0,0,12},empty).size();airHits+=(int)air.update(STEP,{0,2.2f,12},empty).size();coverHits+=(int)covered.update(STEP,{0,0,12},walls).size();}
        CHECK(hits>0&&airHits==0&&coverHits==0,"road slabs hit a grounded target, can be jumped and stop at intact cover");
    }
    for(bool road:{false,true}) {
        auto level=road?buildWasteland():buildCradle();const auto cover=level.story.bossCover.front();
        auto center=(cover.home.min+cover.home.max)*.5f;auto from=center-glm::vec3{0,0,8};glm::vec3 dir{0,0,1};
        CHECK(rayBoxHit(from,dir,level.walls[cover.wall].box)>0,"an intact authored shield blocks real rays");
        auto broken=shatterStoryCover(level,from,center+glm::vec3{0,0,8},.5f);
        CHECK(!broken.empty()&&rayBoxHit(from,dir,level.walls[cover.wall].box)<0,"a machinery impact removes the same shield from collision and line of sight");
        CHECK(shatterStoryCover(level,from,center+glm::vec3{0,0,8},.5f).empty(),"destroyed cover cannot spawn debris repeatedly");
        if(road){auto last=level.story.bossCover.back();auto c=(last.home.min+last.home.max)*.5f;CHECK(shatterStoryCover(level,c,c,1,true).empty(),"an active gust retains its final authored shelter");}
    }
    for(bool road:{false,true}) {
        auto boss=ready(road?EnemyType::BALLAST:EnemyType::SORTER);boss.sortOpen=boss.ballOpen=2;AABB weak;woundBox(boss,weak);
        auto center=(weak.min+weak.max)*.5f;glm::vec3 correct=center+glm::vec3{0,0,road?-15.f:15.f},wrong=center+glm::vec3{0,0,road?15.f:-15.f};float t=0;
        CHECK(woundShot(boss,correct,glm::normalize(center-correct),t)&&!woundShot(boss,wrong,glm::normalize(center-wrong),t),"precision damage requires the visible front strip or rear turbine, from its exposed side");
        CHECK(boss.armorMult()==1&&boss.woundMult()==2,"opening machinery rewards precision without doubling ordinary body damage");
        boss.sortOpen=boss.ballOpen=0;CHECK(!woundBox(boss,weak)&&boss.armorMult()>.8f,"closed machinery remains damageable without a hidden precision bonus");
    }
}
int main() {
    balanceInit();toolChecks();
    for(bool road:{false,true})for(auto weapon:{WeaponId::REVOLVER,WeaponId::LONGSHOT}) {
        auto passive=gunRun(road,false,weapon),reactive=gunRun(road,true,weapon);
        for(bool readsCues:{false,true}) {
            const auto& r=readsCues?reactive:passive;
            std::printf("%s %s %s: %.1fs, hp %.0f, damage %.0f, closest %.1fm, hits %d, parries %d, %s\n",road?"Ballast":"Sorter",weaponDef(weapon).name,readsCues?"reactive":"kite",r.seconds,r.hp,r.taken,r.closest,r.hits,r.parries,r.won?"WIN":"LOSS");
            CHECK(r.taken>=1,"sustained gunfire and movement remain exposed to real attacks");
            CHECK(r.closest<15,"boss pursuit closes long-range firing distance");
        }
        CHECK(reactive.won&&reactive.seconds<90,"reading commitments and attacking openings wins within a reasonable weapon budget");
        CHECK(reactive.taken<passive.taken,"reading the machinery is safer than constant circling and fire");
        CHECK(!passive.won||passive.hp<50,"passive kiting cannot finish untouched even with generous healing");
    }
    std::printf("%s (%d failures)\n",failures?"FAILED":"ALL PASSED",failures);return failures?1:0;
}
