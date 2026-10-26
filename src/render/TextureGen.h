#pragma once
#include "core/gl.h"
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include "render/MaterialTex.h"
#ifdef __EMSCRIPTEN__
#  include <emscripten/html5.h>
#endif

// =============================================================================
// TextureGen.h — uploads the procedural textures (no image files): MaterialTex.h
// makes every world surface's pixels on the CPU; here they go to OpenGL,
// repeating (GL_REPEAT) so they tile across the world, mipmapped and filtered.
// =============================================================================

namespace TextureGen {

inline GLuint uploadTexture(const std::vector<unsigned char>& pixels, int w, int h) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    return tex;
}

// Anisotropic filtering: floors and walls stay sharp where they stretch away
// at a glancing angle, instead of blurring into the distance. 1 turns it off.
// Does nothing where the GPU or browser doesn't offer it.
inline void setAnisotropy(GLuint tex, float amount) {
    constexpr GLenum MAX_ANISOTROPY = 0x84FE, MAX_ANISOTROPY_LIMIT = 0x84FF;   // EXT_texture_filter_anisotropic
    static float limit = -1.f;
    if (limit < 0.f) {
#ifdef __EMSCRIPTEN__
        emscripten_webgl_enable_extension(emscripten_webgl_get_current_context(), "EXT_texture_filter_anisotropic");
#endif
        limit = 0.f;
        glGetFloatv(MAX_ANISOTROPY_LIMIT, &limit);
        if (glGetError() != GL_NO_ERROR) limit = 0.f;
    }
    if (limit < 1.f) return;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameterf(GL_TEXTURE_2D, MAX_ANISOTROPY, std::min(std::max(amount, 1.f), limit));
}

// A world material (MaterialTex.h): alpha marks what glows
inline GLuint uploadMaterial(const std::vector<unsigned char>& px) { return uploadTexture(px, MaterialTex::SIZE, MaterialTex::SIZE); }

} // namespace TextureGen
