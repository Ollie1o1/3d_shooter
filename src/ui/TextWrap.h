#pragma once
#include <algorithm>
#include <string>
#include <vector>
namespace ui_text {
// Wrap complete records at word boundaries without truncating their contents.
inline std::vector<std::string> wrap(const std::string& text,size_t columns) {
    columns=std::max<size_t>(1,columns);
    std::vector<std::string> lines;
    size_t start=0;
    while(start<text.size()) {
        while(start<text.size()&&text[start]==' ')++start;
        if(start==text.size())break;
        size_t end=std::min(text.size(),start+columns);
        if(end<text.size()) {
            size_t space=text.rfind(' ',end);
            if(space!=std::string::npos&&space>start)end=space;
        }
        lines.push_back(text.substr(start,end-start));start=end;
    }
    return lines;
}
}
