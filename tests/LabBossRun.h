#pragma once
#include "player/Weapons.h"
#include "player/Projectile.h"
struct LabBossResult {float seconds=0,damage=0,health=100,lowest=100;int hits=0,parries=0;bool won=false;std::set<AttackKind> moves;};
// A fixed, reproducible input policy. It uses real controller acceleration,
// collision, ammo/reload and occluded hitscan. Its deliberately imperfect aim
// is a tuning instrument, not a claim about a human player's skill.
inline LabBossResult labBossRun(LevelData level,bool reactive,WeaponId weapon,float opening=.3f){
    const auto& a=level.arenas[9];SpatialGrid grid;grid.build(level.walls);Player p(a.playerStart);
    Enemy e(EnemyType::SUCCESSOR,a.bossSpawn,a.playerStart.y);e.spawnTimer=0;e.state=EnemyState::ACTIVE;
    // Do not let unrelated earlier tests consume rand() and change this input
    // rehearsal. Exercise the live constructor's clock range explicitly.
    e.attackTimer=e.stats().attackEvery*opening;e.strafeDir=1;e.animPhase=0;
    const auto& tune=difficulty(DIFFICULTY_DEFAULT);e.maxHealth*=tune.health;e.health=e.maxHealth;
    WeaponUpgrades upgrades;WeaponState gun;gun.ammo=weaponMag(weapon,upgrades);
    ProjectileSystem projectiles;projectiles.floorY=level.lowestFloor();std::vector<Enemy> none;
    SuccessorHazards hazards;LabBossResult result;float immunity=0,dashClock=0;int attempts=0;
    auto hurt=[&](float d){if(immunity>0)return;d*=tune.damage*a.damageScale;result.damage+=d;result.health-=d;result.lowest=std::min(result.lowest,result.health);immunity=.35f;};
    for(int frame=0;frame<60*280 && e.alive;++frame){
        float t=frame*DT;result.seconds=t;immunity=std::max(0.f,immunity-DT);dashClock=std::max(0.f,dashClock-DT);
        auto to=e.position-p.position;to.y=0;float distance=glm::length(to);auto forward=distance>.01f?to/distance:glm::vec3{0,0,-1};glm::vec3 side{-forward.z,0,forward.x};
        auto move=side+(distance<9?-forward*.8f:distance>16?forward*.8f:glm::vec3{0});bool dodge=false;
        if(reactive){
            if(e.attack==AttackKind::SU_INCISION && e.telegraphTimer>0 && distance<7){move=-forward+side*.5f;dodge=true;}
            if((e.attack==AttackKind::SU_VECTOR&&e.telegraphTimer<.4f)||e.suBurst>0){auto d=e.suAim-e.suFrom;d.y=0;if(glm::length(d)>.1f&&machineryDistance(p.position,e.suFrom,e.suAim)<4){d=glm::normalize(d);glm::vec3 normal{-d.z,0,d.x};float sign=glm::dot(p.position-e.suFrom,normal)>=0?1.f:-1.f;move=normal*sign;dodge=true;}}
            for(const auto& mark:hazards.marks)if(mark.wait>0 && mark.wait<.65f && machineryDistance(p.position,mark.a,mark.b)<mark.radius+2){move=p.position-mark.a;move.y=0;dodge=true;}
            if(e.parryWindow()&&distance<5.1f){e.stagger(1.35f);++result.parries;result.health=std::min(100.f,result.health+8*tune.heal);hazards.clear();immunity=.5f;}
        }
        if(glm::length(move)>.01f)move=glm::normalize(move);
        if(reactive&&dodge&&dashClock<=0){p.velocity=move*26.f;dashClock=1.2f;immunity=.15f;}
        Uint8 keys[SDL_NUM_SCANCODES]={};keys[SDL_SCANCODE_W]=1;p.camera.yaw=glm::degrees(std::atan2(move.z,move.x));applyWater(p,level);
        auto before=p.position;p.update(DT,keys,level.walls.data(),(int)level.walls.size(),false,&grid);
        p.position.x=glm::clamp(p.position.x,a.bounds.min.x+1,a.bounds.max.x-1);p.position.z=glm::clamp(p.position.z,a.bounds.min.z+1,a.bounds.max.z-1);
        EnemyWorld w;w.bounds=a.bounds;w.walls=level.walls.data();w.wallCount=(int)level.walls.size();w.grid=&grid;w.tune=&tune;
        w.playerFeet=p.position;w.playerEye=p.position+glm::vec3{0,1.7f,0};w.playerVel=(p.position-before)/DT;
        e.floorY=level.groundAt(e.position.x,e.position.z,e.position.y+.5f);e.update(DT,w);
        if(e.attack!=AttackKind::NONE)result.moves.insert(e.attack);
        const auto& ev=e.ev;if(hazards.epoch!=e.suEpoch){hazards.clear();hazards.epoch=e.suEpoch;}
        if(ev.meleeHit)hurt(ev.meleeDamage);
        if(ev.suSuture){auto a=ev.suAt,b=ev.suTo;a.y=b.y=e.floorY;auto d=glm::normalize(b-a);hazards.line(a,a+d*23.f,.8f);hazards.line(b,b-d*23.f,.8f);}
        if(ev.suRecall||ev.suEcho)hazards.circle(ev.suAt,1.05f,ev.suEcho?18:24);
        float damage=0;hazards.update(DT,p.position,damage);if(damage>0)hurt(damage);
        for(int k=0;k<ev.shots;++k)projectiles.fire(ev.shotOrigin,ev.shotDir[k]*ev.shotSpeed,ev.shotDamage,false,{1,1,1},false,0,ev.shotSize,false);
        if(ev.suCore){auto from=e.position+glm::vec3{0,1.9f,0};projectiles.fire(from,glm::normalize(w.playerEye-from)*15.f,18,false,{1,1,1},false,0,1.1f,true);}
        auto shots=projectiles.update(DT,level.walls.data(),(int)level.walls.size(),none,w.playerEye,&grid);if(shots.hitPlayer)hurt(shots.playerDamage);
        gun.tick(DT,weaponMag(weapon,upgrades));if(gun.ammo==0&&!gun.reloading)gun.startReload(weaponReload(weapon,upgrades));
        if(gun.ready()&&!dodge && e.suPause<=0){
            --gun.ammo;gun.cooldown=weaponCooldown(weapon,upgrades);++attempts;
            // 40% successful shots, with the same misses in each run; no hidden fixed DPS.
            if(attempts%5<2){
                auto hull=e.getAABB();auto target=(hull.min+hull.max)*.5f;auto d=target-w.playerEye;float length=glm::length(d);d/=std::max(.001f,length);
                float wall=weaponDef(weapon).range;for(const auto& b:level.walls){float hit=rayBoxHit(w.playerEye,d,b.box);if(hit>0)wall=std::min(wall,hit);}
                if(length<wall+.05f){e.takeDamage(weaponDamage(weapon,upgrades)*e.armorMult());++result.hits;result.health=std::min(100.f,result.health+2*tune.heal);}
            }
        }
    }
    result.won=!e.alive;return result;
}
