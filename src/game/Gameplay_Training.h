#pragma once
#include "ui/TextWrap.h"

inline void GameplayState::trainingSuccess(int lesson) {
    if(!training() || trainingLesson!=lesson || trainingPassed[lesson])return;
    trainingPassed[lesson]=true;
    ui.toast("LESSON COMPLETE", "REPEAT IT OR CHOOSE ANOTHER LESSON IN THE FIELD GUIDE", {.3f,1.f,.65f}, 3.f);
    audio.play("upgrade",100,SoundGroup::UI,true);
}

inline void GameplayState::startTrainingLesson(int lesson) {
    trainingLesson=std::clamp(lesson,0,lessons::COUNT-1);
    enterArena(0); director.events.clear();director.queue.clear();
    explosionFlashTimer=muzzleFlashTimer=0;hitStopFrames=0;punchCooldown=0;
    fanLeft=0;fanning=doubleShot=false;finishHint=0;
    ui.damageVignette=ui.shootFlashTimer=ui.hitmarkerTimer=ui.parryFlash=ui.overdriveFlash=0;
    director.phase=WaveDirector::Phase::APPROACH;director.hold=true;
    if(prog.level==1) {prog.level=10;prog.points=32;}
    trainingRespawn=.15f;ranked=false;
    if(trainingLesson==3)activeWeapon=1;
    if(trainingLesson==1){styleSystem.health=55;weapons[activeWeapon].ammo=1;}
    viewModel.reset(activeWeapon);captureMouse(true);
}

inline void GameplayState::updateTraining(float dt) {
    if(grenadeTimer<=0)grenadeCount=grenadeMax;
    if(trainingLesson==5 && styleSystem.overdrive)trainingSuccess(5);
    if(trainingLesson==6 && player.position.x>12 && player.position.z<-10 && player.position.y>=4.9f)trainingSuccess(6);
    int alive=0;for(const auto& e:enemies)alive+=e.alive;
    if(alive || trainingLesson==3 || trainingLesson==6)return;
    trainingRespawn-=dt;if(trainingRespawn>0)return;
    // Remove dead targets between repetitions, keeping the sandbox bounded.
    enemies.clear();trainingRespawn=1.5f;
    static const EnemyType choices[]={EnemyType::HUSK,EnemyType::RIPPER,EnemyType::SENTINEL,
        EnemyType::SHIELDBEARER,EnemyType::BRUTE,EnemyType::RAPTOR};
    EnemyType type=trainingLesson==7?choices[trainingEnemy]:EnemyType::HUSK;
    int count=trainingLesson==4 || trainingLesson==5?3:1;
    for(int i=0;i<count;++i) {
        glm::vec3 at={float(i-(count/2))*3.f,0,-7};
        if(trainingLesson==1)at={0,0,7};
        if(trainingLesson==2)at={0,4,-14};
        if(type==EnemyType::RAPTOR)at.y=6;
        spawnEnemy(type,at);
        auto& e=enemies.back();
        if(trainingLesson==1){e.health=e.maxHealth*.3f;e.staggerTimer=999.f;}
        if(trainingLesson==4 || trainingLesson==2)e.staggerTimer=999.f;
    }
}

inline void GameplayState::openGuide() {
    guideWasPaused=paused;guideOpen=true;paused=true;pauseSettings=false;
    guideSelected=training()?trainingLesson:0;
    captureMouse(false);
}

inline void GameplayState::handleGuideEvent(const SDL_Event& e) {
    auto close=[&](){guideOpen=false;paused=guideWasPaused;captureMouse(!paused);};
    if(e.type==SDL_KEYDOWN) {
        auto k=e.key.keysym.sym;
        if(k==SDLK_ESCAPE || k==SDLK_F4 || k==SDLK_p){close();return;}
        if(k==SDLK_UP || k==SDLK_LEFT)guideSelected=(guideSelected+lessons::COUNT-1)%lessons::COUNT;
        if(k==SDLK_DOWN || k==SDLK_RIGHT)guideSelected=(guideSelected+1)%lessons::COUNT;
        if(training() && guideSelected==7 && k==SDLK_RIGHTBRACKET)trainingEnemy=(trainingEnemy+1)%6;
        if(training() && (k==SDLK_RETURN || k==SDLK_SPACE)) {
            guideOpen=false;startTrainingLesson(guideSelected);return;
        }
    }
    auto row=[&](int x,int y){return x>=100 && x<400 && y>=170 && y<170+lessons::COUNT*42 ? (y-170)/42 : -1;};
    if(e.type==SDL_MOUSEMOTION) {int i=row(e.motion.x,e.motion.y);if(i>=0)guideSelected=i;}
    if(e.type==SDL_MOUSEBUTTONDOWN && e.button.button==SDL_BUTTON_LEFT) {
        int i=row(e.button.x,e.button.y);
        if(i>=0) {guideSelected=i;if(training()){guideOpen=false;startTrainingLesson(i);}}
        else if(e.button.y>=580 && e.button.y<625 && e.button.x>=100 && e.button.x<1180)close();
    }
}

inline void GameplayState::renderGuide() {
    ui.begin2D();auto& b=ui.ui;
    b.rect(0,0,SCREEN_W,SCREEN_H,{.015f,.025f,.04f,.96f});
    b.text("FIELD GUIDE",SCREEN_W/2,65,5,{.35f,.85f,1,1},true);
    b.text(training()?"PICK A LESSON. ENTER OR CLICK TO PRACTISE IT.":"COMBAT REFERENCE / PRACTICE IS AVAILABLE FROM THE MAIN MENU",SCREEN_W/2,125,1,{.75f,.8f,.85f,1},true);
    for(int i=0;i<lessons::COUNT;++i) {
        float y=170+i*42.f;b.rect(100,y,300,36,i==guideSelected?glm::vec4{.13f,.25f,.29f,1}:glm::vec4{.05f,.08f,.11f,1});
        b.text(lessons::title(i),116,y+10,2,{.85f,.9f,.95f,1});
        if(trainingPassed[i])b.textRight("OK",385,y+12,1,{.3f,1,.6f,1});
    }
    b.text(lessons::title(guideSelected),445,178,3,{1,.8f,.35f,1});
    auto lines=ui_text::wrap(lessons::action(guideSelected,binds()),58);
    float y=242;for(const auto& line:lines){b.text(line.c_str(),445,y,2,{.9f,.94f,.96f,1});y+=25;}
    if(training() && guideSelected==7) {
        static const EnemyType types[]={EnemyType::HUSK,EnemyType::RIPPER,EnemyType::SENTINEL,EnemyType::SHIELDBEARER,EnemyType::BRUTE,EnemyType::RAPTOR};
        std::string target=std::string("PAD X / ] TARGET: ")+statsOf(types[trainingEnemy]).name;
        b.text(target.c_str(),445,y+10,1,{1,.8f,.4f,1});y+=25;
    }
    y+=30;for(const auto& line:ui_text::wrap(lessons::why(guideSelected),58)) {
        b.text(line.c_str(),445,y,2,{.6f,.73f,.8f,1});y+=25;
    }
    b.rect(100,580,1080,45,{.08f,.14f,.18f,1});
    b.text("ESC / F4 OR CLICK HERE TO RETURN",SCREEN_W/2,596,2,{.7f,.83f,.88f,1},true);
    ui.end2D();
}

inline void GameplayState::renderTraining() {
    ui.begin2D();auto& b=ui.ui;
    b.rect(210,12,860,92,{.015f,.035f,.055f,.94f});
    std::string title=std::string(trainingPassed[trainingLesson]?"COMPLETE / ":"PRACTISE / ")+lessons::title(trainingLesson);
    b.text(title.c_str(),SCREEN_W/2,23,2,{.4f,1.f,.75f,1},true);
    float y=48;for(const auto& line:ui_text::wrap(lessons::action(trainingLesson,binds()),112)) {
        b.text(line.c_str(),SCREEN_W/2,y,1,{.9f,.93f,.96f,1},true);y+=15;
    }
    b.text("F4 FIELD GUIDE / PGUP-PGDN LESSON / BACKSPACE REPEAT / TAB ARMORY",SCREEN_W/2,84,1,{.6f,.78f,.85f,1},true);
    if(trainingLesson==7) {
        static const EnemyType types[]={EnemyType::HUSK,EnemyType::RIPPER,EnemyType::SENTINEL,EnemyType::SHIELDBEARER,EnemyType::BRUTE,EnemyType::RAPTOR};
        std::string target=std::string("[ / ] TARGET: ")+statsOf(types[trainingEnemy]).name;
        b.text(target.c_str(),SCREEN_W/2,115,2,{1,.8f,.4f,1},true);
    }
    ui.end2D();
}
