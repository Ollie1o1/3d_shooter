#pragma once
// =============================================================================
// MenuArt.h — the menus' pictures: renders made in Blender (tools/blender/render_art.py)
// in assets/art/, loaded once each (stb_image) and kept as textures. A missing or
// broken picture is just not drawn.
//
//   arena_<level>_<n>   an arena (level act1 / act2, n from 0), 16:9
//   boss_<n>            a boss, in the BOSS page's order (bossEntry), 4:5
// =============================================================================
#include "core/gl.h"
#include <cstdio>
#include <map>
#include <string>

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunused-function"
#  pragma clang diagnostic ignored "-Wmissing-field-initializers"
#  pragma clang diagnostic ignored "-Wsign-compare"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-function"
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#  pragma GCC diagnostic ignored "-Wsign-compare"
#endif
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_STDIO_FAILURE_STRINGS_FALLBACK
#include "third_party/stb/stb_image.h"
#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

namespace menuart {

struct Pic { GLuint tex = 0; int w = 0, h = 0; };

// assets/art/<name>.jpg as a texture (tex 0 if there isn't one)
inline const Pic& get(const std::string& name) {
    static std::map<std::string, Pic> cache;
    auto it = cache.find(name);
    if (it != cache.end()) return it->second;
    Pic p;
    const std::string path = "assets/art/" + name + ".jpg";
    int n = 0;
    unsigned char* px = stbi_load(path.c_str(), &p.w, &p.h, &n, 4);
    if (px) {
        glGenTextures(1, &p.tex);
        glBindTexture(GL_TEXTURE_2D, p.tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p.w, p.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(px);
    }
    return cache.emplace(name, p).first->second;
}
inline const Pic& arena(bool act2, int n) { return get(std::string("arena_") + (act2 ? "act2_" : "act1_") + std::to_string(n)); }
inline const Pic& boss(int n) { return get("boss_" + std::to_string(n)); }

// A picture fading from one to the next (a page's backdrop following what's picked)
struct Fade {
    const Pic* now = nullptr;
    const Pic* was = nullptr;
    float t = 1.f;   // 0..1 into the change
    void show(const Pic* p) { if (p == now) return; was = now; now = p; t = 0.f; }
    void update(float dt, float over = 0.35f) { t = std::min(1.f, t + dt / over); }
};

} // namespace menuart
