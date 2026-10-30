// Real WebAssembly regression: Story previews must not allocate all of the
// legacy procedural rigs' temporary arrays before drawing a file-based boss.
#include "render/EnemyModel.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>

int main(int argc,char** argv) {
    if(argc!=2)return 1;
    const auto type=static_cast<EnemyType>(std::atoi(argv[1]));
    if(type!=EnemyType::SORTER&&type!=EnemyType::BALLAST)return 1;
    const auto* model=enemyfile::modelOf(static_cast<int>(type));
    if(!model||model->parts.empty())return 1;
    Enemy boss(type,{0,0,0});boss.spawnTimer=0;boss.state=EnemyState::ACTIVE;
    std::vector<BoxInstance> parts;
    for(int frame=0;frame<3;++frame) {
        parts.clear();buildEnemy(boss,frame/60.f,parts);
        if(parts.empty())return 1;
        for(const auto& part:parts)for(int c=0;c<4;++c)for(int r=0;r<4;++r)
            if(!std::isfinite(part.model[c][r]))return 1;
    }
    std::printf("ok: %s preview renders %zu parts within a 32 KiB WebAssembly stack\n",boss.stats().name,parts.size());
    return 0;
}
