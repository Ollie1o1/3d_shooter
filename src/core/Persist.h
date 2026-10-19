#pragma once
// =============================================================================
// Persist.h — tiny key → text store for settings and best times.
//
// Desktop: one file per key next to the binary (settings.cfg, records.cfg).
// Web: the browser's localStorage. The Emscripten build's filesystem is an
// in-memory one that is thrown away on reload, so writing a file there would
// look like it worked and then forget everything; localStorage survives.
// =============================================================================
#include <string>
#include <fstream>
#include <sstream>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif

namespace persist {

#ifdef __EMSCRIPTEN__
// The stored text only ever holds "key value" lines, but escape anyway: it is
// pasted into a JavaScript string literal.
inline std::string jsQuote(const std::string& s) {
    std::string o = "'";
    for (char c : s) {
        if (c == '\\' || c == '\'') { o += '\\'; o += c; }
        else if (c == '\n') o += "\\n";
        else if ((unsigned char)c >= 32) o += c;
    }
    return o + "'";
}
#endif

inline std::string load(const std::string& key) {
#ifdef __EMSCRIPTEN__
    std::string js = "(function(){try{return localStorage.getItem(" + jsQuote("overdrive." + key)
                   + ")||'';}catch(e){return '';}})()";
    const char* r = emscripten_run_script_string(js.c_str());
    return r ? std::string(r) : std::string();
#else
    std::ifstream f(key);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
#endif
}

inline void save(const std::string& key, const std::string& text) {
#ifdef __EMSCRIPTEN__
    std::string js = "try{localStorage.setItem(" + jsQuote("overdrive." + key) + "," + jsQuote(text) + ");}catch(e){}";
    emscripten_run_script(js.c_str());
#else
    std::ofstream f(key, std::ios::trunc);
    if (f) f << text;
#endif
}

} // namespace persist
