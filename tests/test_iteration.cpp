#include "core/Balance.h"
#include "game/DamageFeedback.h"
#include "render/Effects.h"
#include "player/Projectile.h"
#include "world/LevelTraining.h"
#include <cstdio>
#include <limits>
#include <memory>

static int failures=0;
#define CHECK(c,m) do{bool ok=(c);std::printf("%s %s\n",ok?"ok:":"FAIL:",m);failures+=!ok;}while(0)

// The same real AI and gameplay draws, under different presentation workloads.
static std::vector<float> fight(int detail) {
    std::srand(4207);cosmetic::seed(100+detail);
    auto fx=std::make_unique<Effects>();auto level=buildTraining();
    std::vector<Enemy> squad;
    for(auto type:{EnemyType::HUSK,EnemyType::RIPPER,EnemyType::SENTINEL,EnemyType::GRAFTED,
                   EnemyType::MITE,EnemyType::JUGGERNAUT,EnemyType::RAPTOR,EnemyType::SERAPH}) {
        Enemy e(type,{float(squad.size())*3-6,0,-15});e.uid=int(squad.size())+1;
        e.spawnTimer=0;e.state=EnemyState::ACTIVE;squad.push_back(e);
        if(detail)fx->spawnDeathParticles(e.position,{1,.3f,.1f});
    }
    EnemyWorld world;world.bounds={{-40,0,-40},{40,30,40}};
    std::vector<float> trace;
    for(int frame=0;frame<600;++frame) {
        world.playerFeet={std::sin(frame*.03f)*8,0,std::cos(frame*.02f)*4};
        world.playerEye=world.playerFeet+glm::vec3{0,1.7f,0};
        if(detail) {
            fx->spawnBurst(world.playerEye,{1,.4f,.1f},detail*8,5,.3f,9);
            fx->spawnShellCasing(world.playerEye,{1,0,0});
            fx->spawnAmbientParticles(level,world.playerFeet,false,1.f/60,detail,frame/60.f);
            fx->updateParticles(1.f/60);
            (void)cosmetic::range(-1,1);(void)cosmetic::index(4); // camera / sound variation
        }
        for(auto& e:squad) {
            e.update(1.f/60,world);
            for(float v:{e.position.x,e.position.y,e.position.z,e.health,e.attackTimer,e.animPhase,float(e.attack),
                         float(e.ev.shots),float(e.ev.meleeHit),float(e.ev.telegraphStarted)})trace.push_back(v);
        }
        trace.push_back(frand(0,1)); // the weapon spread / loot sequence is unchanged too
    }
    return trace;
}
int main() {
    balanceInit();
    const auto baseline=fight(0);
    CHECK(baseline==fight(1) && baseline==fight(2),"real AI, attack events and gameplay draws are identical with effects off, reduced or full");
    {
        EnemyWorld world;world.playerFeet={0,0,4};world.playerEye={0,1.7f,4};
        Enemy boss(EnemyType::SOVEREIGN,{0,0,0});boss.state=EnemyState::ACTIVE;boss.spawnTimer=0;
        boss.attack=AttackKind::SWEEP;boss.telegraphTimer=.001f;boss.comboStep=1;boss.yaw=0;
        boss.update(1.f/60,world);
        CHECK(boss.ev.meleeHit && boss.ev.meleeAttack==AttackKind::SWEEP && boss.attack==AttackKind::CLEAVE,
              "the real Sovereign's second sweep reports its contact separately from the next queued cleave");
        boss.telegraphTimer=.001f;boss.update(1.f/60,world);
        CHECK(boss.ev.meleeHit && boss.ev.meleeAttack==AttackKind::CLEAVE && boss.attack!=AttackKind::CLEAVE,
              "a real cleave keeps its contact identity after the AI clears its attack");
    }
    std::srand(7);auto a=cosmetic::next();std::srand(7);auto b=cosmetic::next();
    CHECK(a!=b,"reseeding gameplay does not rewind cosmetic randomness");
    DamageHistory history;
    DamageCause cleave{EnemyType::SOVEREIGN,DamageKind::MELEE,AttackKind::CLEAVE};
    history.record(40,30,cleave);history.record(80,0,{EnemyType::HUSK,DamageKind::PROJECTILE});
    CHECK(history.last.attack==AttackKind::CLEAVE,"damage after health reaches zero cannot replace the lethal contact");
    for(float rejected:{0.f,-4.f,std::numeric_limits<float>::quiet_NaN()})history.record(rejected,100,{});
    CHECK(history.last.attack==AttackKind::CLEAVE,"zero, negative and invalid damage cannot overwrite feedback");
    CHECK(damageLabel(history.last)=="SOVEREIGN / CLEAVE" && std::string(damageAdvice(history.last).counter).find("PARRY")!=std::string::npos,
          "a recorded Sovereign cleave names its actual attack and counter");
    history.record(10,100,{});
    CHECK(damageLabel(history.last).empty() && !*damageAdvice(history.last).counter,"an unknown later contact clears old advice instead of guessing");
    history.record(20,10,{EnemyType::COUNT,DamageKind::FALL});
    CHECK(damageLabel(history.last)=="FALL","environmental damage does not inherit an earlier enemy's identity");
    history.clear();CHECK(damageLabel(history.last).empty(),"retries can clear all prior damage feedback");
    for(auto kind:{DamageKind::GOLD_STRIKE,DamageKind::RED_STRIKE})
        CHECK(std::string(damageAdvice({EnemyType::ABSOLUTE,kind}).counter).find("WILL NOT")!=std::string::npos,
              "unparryable and unblockable strikes explain their specific restriction");
    ProjectileSystem shots;std::vector<Enemy> enemies;
    auto hit=[&](EnemyType left,EnemyType right){
        auto* a=shots.fire({-2,1.7f,0},{20,0,0},9,false);a->ownerType=int(left);
        auto* b=shots.fire({2,1.7f,0},{-20,0,0},11,false);b->ownerType=int(right);
        return shots.update(.1f,nullptr,0,enemies,{0,1.7f,0});
    };
    const auto mixed=hit(EnemyType::HUSK,EnemyType::SENTINEL);
    CHECK(mixed.hitPlayer && mixed.playerDamage==20 && mixed.playerHitOwner==-1,"simultaneous projectiles from different enemy types retain damage without false single-enemy attribution");
    const auto same=hit(EnemyType::HUSK,EnemyType::HUSK);
    CHECK(same.hitPlayer && same.playerDamage==20 && same.playerHitOwner==int(EnemyType::HUSK),"same-source projectile batches keep their source");
    std::printf("%d iteration failures\n",failures);return failures?1:0;
}
