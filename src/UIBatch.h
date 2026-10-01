#pragma once
// =============================================================================
// UIBatch.h — batched 2D quads for every HUD and menu element.
//
// Text is a 5x7 pixel font where every lit pixel is a small quad, so a screen
// of text is thousands of quads. Drawing each with its own uniform upload and
// glDrawArrays was the single biggest per-frame cost in the browser build
// (every GL call crosses from WebAssembly into JavaScript). Here they are
// appended to one vertex array (position + RGBA per vertex) and drawn with a
// single call per flush.
//
// Usage: begin(); rect(...); text(...); ... end();   (end() flushes)
// Coordinates are pixels, origin top-left, matching SCREEN_W x SCREEN_H.
// =============================================================================
#include "gl.h"
#include "ShaderProgram.h"
#include "PixelFont.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cstring>

class UIBatch {
public:
    int screenW, screenH;

    UIBatch(int w, int h) : screenW(w), screenH(h) {
        shader.loadFiles("src/ui.vert", "src/ui.frag");
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);
        verts.reserve(6 * 6 * 4096);
    }
    ~UIBatch() {
        if (vao) glDeleteVertexArrays(1, &vao);
        if (vbo) glDeleteBuffers(1, &vbo);
    }
    UIBatch(const UIBatch&) = delete;
    UIBatch& operator=(const UIBatch&) = delete;

    void begin() {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        verts.clear();
    }

    void end() {
        flush();
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
    }

    void flush() {
        if (verts.empty()) return;
        shader.use();
        shader.setMat4("projection", glm::ortho(0.f, (float)screenW, (float)screenH, 0.f));
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size() * sizeof(float)), verts.data(), GL_STREAM_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(verts.size() / 6));
        glBindVertexArray(0);
        verts.clear();
    }

    void rect(float x, float y, float w, float h, glm::vec4 c) {
        if (w <= 0.f || h <= 0.f || c.a <= 0.f) return;
        float x1 = x + w, y1 = y + h;
        const float v[36] = {
            x,  y,  c.r,c.g,c.b,c.a,   x1, y,  c.r,c.g,c.b,c.a,   x1, y1, c.r,c.g,c.b,c.a,
            x,  y,  c.r,c.g,c.b,c.a,   x1, y1, c.r,c.g,c.b,c.a,   x,  y1, c.r,c.g,c.b,c.a,
        };
        verts.insert(verts.end(), v, v + 36);
    }

    // Hollow rectangle
    void frame(float x, float y, float w, float h, float t, glm::vec4 c) {
        rect(x, y, w, t, c); rect(x, y + h - t, w, t, c);
        rect(x, y + t, t, h - 2 * t, c); rect(x + w - t, y + t, t, h - 2 * t, c);
    }

    static int textWidth(const char* s, int scale) {
        int len = (int)strlen(s);
        return len > 0 ? len * 6 * scale - scale : 0;
    }

    void text(const char* s, float px, float py, int scale, glm::vec4 c, bool centred = false) {
        int len = (int)strlen(s);
        float x = centred ? px - textWidth(s, scale) / 2 : px;
        float sc = (float)scale;
        for (int i = 0; i < len; ++i, x += 6 * sc) {
            unsigned char ch = (unsigned char)s[i];
            if (ch < 32 || ch > 127) continue;
            const uint8_t* g = PIXEL_FONT[ch - ' '];
            for (int row = 0; row < 7; ++row) {
                uint8_t bits = g[row];
                // Merge horizontal runs of lit pixels into one quad
                int col = 0;
                while (col < 5) {
                    if (!(bits & (0x10 >> col))) { ++col; continue; }
                    int start = col;
                    while (col < 5 && (bits & (0x10 >> col))) ++col;
                    rect(x + start * sc, py + row * sc, (col - start) * sc, sc, c);
                }
            }
        }
    }

    // Right-aligned text ending at px
    void textRight(const char* s, float px, float py, int scale, glm::vec4 c) {
        text(s, px - textWidth(s, scale), py, scale, c, false);
    }

    // Text with a 1-step drop shadow, for HUD readability over bright scenes
    void textShadow(const char* s, float px, float py, int scale, glm::vec4 c, bool centred = false) {
        text(s, px + scale * 0.5f + 1, py + scale * 0.5f + 1, scale, {0.f, 0.f, 0.f, c.a * 0.6f}, centred);
        text(s, px, py, scale, c, centred);
    }

private:
    ShaderProgram      shader;
    GLuint             vao = 0, vbo = 0;
    std::vector<float> verts;
};
