#pragma once
#include "gl.h"
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#ifdef __EMSCRIPTEN__
#  include <emscripten/html5.h>
#endif

// =============================================================================
// TextureGen.h — Procedural texture generation (no external image files needed)
//
// All textures are generated as pixel arrays on the CPU, then uploaded to OpenGL.
// Set GL_REPEAT wrapping so they tile naturally across world geometry.
// =============================================================================

namespace TextureGen {

// Simple hash-based noise for procedural patterns
inline float hash(float x, float y) {
    float h = sinf(x * 12.9898f + y * 78.233f) * 43758.5453f;
    return h - floorf(h);
}

inline float smoothNoise(float x, float y) {
    float ix = floorf(x), iy = floorf(y);
    float fx = x - ix, fy = y - iy;
    fx = fx * fx * (3.f - 2.f * fx);
    fy = fy * fy * (3.f - 2.f * fy);
    float a = hash(ix, iy), b = hash(ix+1, iy);
    float c = hash(ix, iy+1), d = hash(ix+1, iy+1);
    return a + (b-a)*fx + (c-a)*fy + (a-b-c+d)*fx*fy;
}

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

// Dark concrete floor with subtle grid lines
inline GLuint generateGridFloor(int size = 128) {
    std::vector<unsigned char> pixels(size * size * 4);
    float gridSize = 16.f; // grid line spacing in pixels
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float noise = smoothNoise(x * 0.08f, y * 0.08f) * 0.12f;
            float noise2 = smoothNoise(x * 0.3f, y * 0.3f) * 0.06f;
            float base = 0.28f + noise + noise2;

            // Grid lines
            float gx = fmodf((float)x, gridSize);
            float gy = fmodf((float)y, gridSize);
            bool gridLine = (gx < 1.2f || gy < 1.2f);
            if (gridLine) base *= 0.65f;

            unsigned char v = (unsigned char)(glm::clamp(base, 0.f, 1.f) * 255.f);
            int i = (y * size + x) * 4;
            pixels[i+0] = v;
            pixels[i+1] = (unsigned char)(v * 0.95f);
            pixels[i+2] = (unsigned char)(v * 0.88f);
            pixels[i+3] = 255;
        }
    }
    return uploadTexture(pixels, size, size);
}

// Brick/panel wall pattern with mortar lines
inline GLuint generateBrickWall(int size = 128) {
    std::vector<unsigned char> pixels(size * size * 4);
    float brickH = 16.f, brickW = 32.f, mortarW = 1.5f;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            int row = (int)floorf(y / brickH);
            float offset = (row % 2 == 1) ? brickW * 0.5f : 0.f;
            float bx = fmodf(x + offset, brickW);
            float by = fmodf((float)y, brickH);

            bool mortar = (bx < mortarW || by < mortarW);

            float noise = smoothNoise(x * 0.15f, y * 0.15f) * 0.08f;
            float base;
            if (mortar) {
                base = 0.15f + noise * 0.5f;
            } else {
                base = 0.38f + noise + smoothNoise(x * 0.4f + row * 7.f, y * 0.4f) * 0.05f;
            }

            unsigned char r = (unsigned char)(glm::clamp(base * 1.05f, 0.f, 1.f) * 255.f);
            unsigned char g = (unsigned char)(glm::clamp(base * 0.95f, 0.f, 1.f) * 255.f);
            unsigned char b = (unsigned char)(glm::clamp(base * 0.85f, 0.f, 1.f) * 255.f);
            int i = (y * size + x) * 4;
            pixels[i+0] = r;
            pixels[i+1] = g;
            pixels[i+2] = b;
            pixels[i+3] = 255;
        }
    }
    return uploadTexture(pixels, size, size);
}

// Dark brushed metal ceiling with seams
inline GLuint generateMetalCeiling(int size = 128) {
    std::vector<unsigned char> pixels(size * size * 4);
    float panelSize = 32.f;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float noise = smoothNoise(x * 0.2f, y * 0.05f) * 0.07f; // directional brushing
            float noise2 = smoothNoise(x * 0.5f, y * 0.5f) * 0.03f;
            float base = 0.20f + noise + noise2;

            // Panel seams
            float px = fmodf((float)x, panelSize);
            float py = fmodf((float)y, panelSize);
            if (px < 1.f || py < 1.f) base *= 0.5f;

            // Rivets at panel corners
            float cx = fmodf((float)x + 3.f, panelSize);
            float cy = fmodf((float)y + 3.f, panelSize);
            if (cx < 3.f && cy < 3.f) base += 0.12f;

            unsigned char v = (unsigned char)(glm::clamp(base, 0.f, 1.f) * 255.f);
            int i = (y * size + x) * 4;
            pixels[i+0] = (unsigned char)(v * 0.90f);
            pixels[i+1] = (unsigned char)(v * 0.92f);
            pixels[i+2] = v;
            pixels[i+3] = 255;
        }
    }
    return uploadTexture(pixels, size, size);
}

// Sci-fi wall panels: big plates with bevelled seams, a recessed strip and
// a few rivets. Tubes, airlocks and the newer rooms use it.
inline GLuint generatePanel(int size = 128) {
    std::vector<unsigned char> pixels(size * size * 4);
    const float P = 64.f;   // two plates per tile edge
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float px = fmodf((float)x, P), py = fmodf((float)y, P);
            float base = 0.36f + smoothNoise(x * 0.06f, y * 0.06f) * 0.06f + smoothNoise(x * 0.5f, y * 0.5f) * 0.02f;
            // Bevel: light top/left edge, dark bottom/right edge, a dark seam
            if (px < 1.5f || py < 1.5f) base = 0.12f;
            else if (px < 3.5f || py < 3.5f) base *= 1.25f;
            else if (px > P - 3.f || py > P - 3.f) base *= 0.7f;
            // A recessed horizontal strip across the middle of each plate
            if (py > P * 0.62f && py < P * 0.70f && px > 8.f && px < P - 8.f) base *= 0.62f;
            // Rivets in the corners
            for (float cx : {7.f, P - 7.f}) for (float cy : {7.f, P - 7.f}) {
                float d = sqrtf((px - cx) * (px - cx) + (py - cy) * (py - cy));
                if (d < 1.8f) base = 0.55f;
            }
            unsigned char v = (unsigned char)(glm::clamp(base, 0.f, 1.f) * 255.f);
            int i = (y * size + x) * 4;
            pixels[i+0] = (unsigned char)(v * 0.94f);
            pixels[i+1] = (unsigned char)(v * 0.97f);
            pixels[i+2] = v;
            pixels[i+3] = 255;
        }
    }
    return uploadTexture(pixels, size, size);
}

// Layered rock: strata, cracks and grain. Cliffs, islands, the canyon.
inline GLuint generateRock(int size = 128) {
    std::vector<unsigned char> pixels(size * size * 4);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float n = smoothNoise(x * 0.05f, y * 0.05f) * 0.5f + smoothNoise(x * 0.13f, y * 0.13f) * 0.3f
                    + smoothNoise(x * 0.4f, y * 0.4f) * 0.2f;
            // Strata: wavy horizontal bands
            float band = sinf(y * 0.35f + smoothNoise(x * 0.04f, y * 0.02f) * 6.f);
            float base = 0.30f + n * 0.22f + band * 0.035f;
            // Cracks where a low-frequency noise crosses a threshold
            float c = fabsf(smoothNoise(x * 0.09f + 3.f, y * 0.09f) - 0.5f);
            if (c < 0.02f) base *= 0.55f;
            unsigned char v = (unsigned char)(glm::clamp(base, 0.f, 1.f) * 255.f);
            int i = (y * size + x) * 4;
            pixels[i+0] = (unsigned char)(v * 1.02f > 255 ? 255 : v * 1.02f);
            pixels[i+1] = (unsigned char)(v * 0.98f);
            pixels[i+2] = (unsigned char)(v * 0.92f);
            pixels[i+3] = 255;
        }
    }
    return uploadTexture(pixels, size, size);
}

// Poured concrete: formwork seams, tie holes and staining
inline GLuint generateConcrete(int size = 128) {
    std::vector<unsigned char> pixels(size * size * 4);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float base = 0.40f + smoothNoise(x * 0.07f, y * 0.07f) * 0.08f + smoothNoise(x * 0.6f, y * 0.6f) * 0.04f;
            base -= smoothNoise(x * 0.02f, y * 0.11f) * 0.08f;          // vertical run-off stains
            float fx = fmodf((float)x, 64.f), fy = fmodf((float)y, 32.f);
            if (fx < 1.f || fy < 1.f) base *= 0.72f;                    // formwork seams
            for (float cx : {16.f, 48.f}) {                             // tie holes
                float d = sqrtf((fx - cx) * (fx - cx) + (fy - 16.f) * (fy - 16.f));
                if (d < 1.6f) base *= 0.45f;
            }
            unsigned char v = (unsigned char)(glm::clamp(base, 0.f, 1.f) * 255.f);
            int i = (y * size + x) * 4;
            pixels[i+0] = v;
            pixels[i+1] = v;
            pixels[i+2] = (unsigned char)(v * 0.96f);
            pixels[i+3] = 255;
        }
    }
    return uploadTexture(pixels, size, size);
}

// Crate/box surface texture
inline GLuint generateCrateSurface(int size = 64) {
    std::vector<unsigned char> pixels(size * size * 4);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float noise = smoothNoise(x * 0.15f, y * 0.15f) * 0.1f;
            float base = 0.42f + noise;

            // Border planks
            float margin = 4.f;
            bool border = (x < (int)margin || x >= size-(int)margin ||
                          y < (int)margin || y >= size-(int)margin);
            if (border) base *= 0.7f;

            // Cross brace
            float cx = fabsf(x - size/2.f), cy = fabsf(y - size/2.f);
            if (cx < 2.f || cy < 2.f) base *= 0.75f;

            unsigned char r = (unsigned char)(glm::clamp(base * 1.1f, 0.f, 1.f) * 255.f);
            unsigned char g = (unsigned char)(glm::clamp(base * 0.85f, 0.f, 1.f) * 255.f);
            unsigned char b = (unsigned char)(glm::clamp(base * 0.55f, 0.f, 1.f) * 255.f);
            int i = (y * size + x) * 4;
            pixels[i+0] = r;
            pixels[i+1] = g;
            pixels[i+2] = b;
            pixels[i+3] = 255;
        }
    }
    return uploadTexture(pixels, size, size);
}

} // namespace TextureGen
