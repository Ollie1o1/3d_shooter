#pragma once
#include "ui/BalancePanel.h"
#include "game/GameplayState.h"

namespace encounterpanel {
#ifdef OVERDRIVE_BALANCE_PANEL
struct State {
    int room=-1,wave=0,entry=0,point=0;
    std::string status;
    std::vector<EncounterDraft> authored;
};
inline State& st(){static State s;return s;}
inline std::string mapKey(const GameplayState& g){return std::to_string((int)g.mode)+"-"+std::to_string(g.level.story.chapter);}
inline void bind(GameplayState& g) {
    st()=State{};
    for(int r=0;r<(int)g.level.arenas.size();++r){EncounterDraft d;d.capture(g.level,r,mapKey(g));st().authored.push_back(d);}
}
inline void build(GameplayState& g) {
    ImGui::SetNextWindowPos({16,16},ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({530,600},ImGuiCond_FirstUseEver);
    ImGui::Begin("ENCOUNTER WORKSHOP",nullptr,ImGuiWindowFlags_NoCollapse);
    if(ImGui::Button("BACK TO BALANCE"))balancepanel::showEncounter=false;
    if(g.endless() || g.training() || g_bossRun>=0) {
        ImGui::TextWrapped("Start an authored arena or Story encounter to edit its waves.");ImGui::End();return;
    }
    if(!g_practice) {
        ImGui::TextWrapped("Enable editing for this run to change waves and preview spawn positions. Edited runs cannot save Story progress or records.");
        if(ImGui::Button("ENABLE ENCOUNTER EDITING")){g_practice=true;g.ranked=g.act2FromStart=g.storyFromStart=false;}
        ImGui::End();return;
    }
    auto& s=st();
    if(s.authored.empty())bind(g);
    if(s.room<0){s.room=std::clamp(g.director.arena,0,(int)g.level.arenas.size()-1);g.encounterDraft.capture(g.level,s.room,mapKey(g));}
    if(ImGui::BeginCombo("Room",g.level.arenas[s.room].name)) {
        for(int r=0;r<(int)g.level.arenas.size();++r)if(ImGui::Selectable(g.level.arenas[r].name,s.room==r)) {
            s.room=r;s.wave=s.entry=s.point=0;s.status.clear();g.encounterDraft.capture(g.level,r,mapKey(g));
        }
        ImGui::EndCombo();
    }
    auto& d=g.encounterDraft;
    if(!EncounterDraft::editable(g.level.arenas[s.room])) {
        ImGui::TextWrapped("Bosses, quiet sections and scripted encounters keep their authored setup.");ImGui::End();return;
    }
    ImGui::SliderInt("Concurrent cap",&d.maxAlive,1,40);
    ImGui::SliderInt("Wave",&s.wave,0,(int)d.waves.size()-1);
    auto& wave=d.waves[s.wave];s.entry=std::clamp(s.entry,0,std::max(0,(int)wave.size()-1));
    if(!wave.empty()) {
        ImGui::SliderInt("Entry",&s.entry,0,(int)wave.size()-1);
        auto& entry=wave[s.entry];
        if(ImGui::BeginCombo("Enemy",statsOf(entry.type).name)) {
            for(int t=0;t<(int)EnemyType::COUNT;++t)if(!isBoss((EnemyType)t) && (EnemyType)t!=EnemyType::CONDUIT)
                if(ImGui::Selectable(statsOf((EnemyType)t).name,(int)entry.type==t)){entry.type=(EnemyType)t;entry.variant=Hollow::NONE;}
            ImGui::EndCombo();
        }
        if(entry.at.empty())ImGui::SliderInt("Count (automatic placement)",&entry.count,1,64);
        else {
            s.point=std::clamp(s.point,0,(int)entry.at.size()-1);
            ImGui::SliderInt("Spawn",&s.point,0,(int)entry.at.size()-1);
            ImGui::DragFloat3("Feet X / Y / Z",&entry.at[s.point].x,.25f);
            if(ImGui::Button("MOVE SPAWN TO PLAYER"))entry.at[s.point]=g.player.position;
            ImGui::SameLine();if(ImGui::Button("REMOVE SPAWN"))entry.at.erase(entry.at.begin()+s.point);
        }
        if(ImGui::Button("ADD SPAWN AT PLAYER")){entry.at.push_back(g.player.position);s.point=(int)entry.at.size()-1;}
        ImGui::SameLine();if(ImGui::Button("USE AUTOMATIC PLACEMENT")){entry.count=std::max(1,(int)entry.at.size());entry.at.clear();}
        ImGui::TextDisabled("Escorts: %zu (kept from the authored entry)",entry.escort.size());
    }
    if(ImGui::Button("ADD ENEMY ENTRY")){wave.emplace_back(EnemyType::HUSK,1);s.entry=(int)wave.size()-1;}
    ImGui::SameLine();if(!wave.empty() && ImGui::Button("REMOVE ENTRY"))wave.erase(wave.begin()+s.entry);
    ImGui::Checkbox("Preview spawn bounds in the world",&g.encounterPreview);
    const auto error=d.validate(g.level);
    if(!error.empty())ImGui::TextWrapped("%s",error.c_str());
    ImGui::BeginDisabled(!error.empty());
    if(ImGui::Button("APPLY AND RESTART ROOM")) {
        if(d.apply(g.level,s.status)) {
            g.enterArena(s.room);s.status="Applied. Close F1 to play the encounter.";
            // F1 retains the frozen editor and cursor until the user closes it.
            g.captureMouse(false);
        }
    }
    ImGui::SameLine();if(ImGui::Button("SAVE PATCH")){persist::save("encounter.v1.cfg",d.text());s.status="Saved encounter.v1.cfg";}
    ImGui::EndDisabled();
    if(ImGui::Button("LOAD PATCH")) {
        if(d.read(persist::load("encounter.v1.cfg"),g.level,s.status)){s.wave=s.entry=s.point=0;s.status="Loaded. Apply to play it.";}
    }
    ImGui::SameLine();if(ImGui::Button("RESTORE AUTHORED")){d=s.authored[s.room];s.wave=s.entry=s.point=0;s.status="Authored waves restored in draft. Apply to play them.";}
    if(!s.status.empty())ImGui::TextWrapped("%s",s.status.c_str());
    ImGui::TextWrapped("F1 closes the workshop and resumes play. Objectives stay authored. Patches apply only when loaded here.");
    ImGui::End();
}
#else
inline void bind(GameplayState&){}
inline void build(GameplayState&){}
#endif
}
