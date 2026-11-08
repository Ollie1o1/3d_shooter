#pragma once
#include <cstdint>
#include <limits>
#include <string>

// OD1 identifies the wave generator/rules version. A code shares the wave
// sequence and difficulty, not enemy positions, AI choices or a full replay.
struct EndlessChallenge {
    bool active = false;
    uint32_t seed = 0;
    int difficulty = 1;
    std::string code() const {return "OD1-"+std::to_string(seed)+"-"+std::to_string(difficulty);}
    bool parse(const std::string& text) {
        if(text.rfind("OD1-",0)!=0)return false;
        const auto dash=text.find('-',4);
        if(dash==std::string::npos || dash==4 || dash+2!=text.size())return false;
        uint64_t n=0;
        for(size_t i=4;i<dash;++i){
            if(text[i]<'0'||text[i]>'9')return false;
            n=n*10+(text[i]-'0');
            if(n>std::numeric_limits<uint32_t>::max())return false;
        }
        const int d=text.back()-'0';if(d<0||d>3)return false;
        active=true;seed=(uint32_t)n;difficulty=d;return true;
    }
};
