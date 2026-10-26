#pragma once
// =============================================================================
// WorldMesh.h — the static level geometry, batched into one mesh per texture,
// and the 1x1 grey texture untextured things are drawn with.
// =============================================================================
#include "render/WorldGeometry.h"
#include "core/Mesh.h"
#include "core/gl.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstddef>

static GLuint makeGreyTexture() {
    GLuint tex;
    glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D,tex);
    unsigned char grey[4]={200,200,200,255};
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,grey);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    return tex;
}

// Contact shadows (contact.vert): the static ones built once, and a second
// buffer refilled each frame for whatever moves (enemies)
struct ContactMesh {
    GLuint vao = 0, vbo = 0;
    int count = 0, capacity = 0;
    // Leaves GL_ARRAY_BUFFER as it found it: other passes fill whatever is bound
    void upload(const std::vector<ContactVert>& v, bool dynamic) {
        GLint prevBuf = 0;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevBuf);
        if (!vao) {
            glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            const GLsizei st = sizeof(ContactVert);
            glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, st, (void*)offsetof(ContactVert, pos));
            glEnableVertexAttribArray(1); glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, st, (void*)offsetof(ContactVert, rect));
            glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, st, (void*)offsetof(ContactVert, strengthReach));
        }
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        if ((int)v.size() > capacity || !dynamic) {
            capacity = std::max((int)v.size(), dynamic ? 1536 : 0);
            glBufferData(GL_ARRAY_BUFFER, capacity * sizeof(ContactVert), nullptr, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        }
        if (!v.empty()) glBufferSubData(GL_ARRAY_BUFFER, 0, v.size() * sizeof(ContactVert), v.data());
        count = (int)v.size();
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, (GLuint)prevBuf);
    }
    void draw() const { if (count > 0) { glBindVertexArray(vao); glDrawArrays(GL_TRIANGLES, 0, count); glBindVertexArray(0); } }
    void release() { if (vbo) glDeleteBuffers(1, &vbo); if (vao) glDeleteVertexArrays(1, &vao); vao = vbo = 0; count = capacity = 0; }
};

struct WorldMeshes { Mesh byTex[TEX_COUNT]; Mesh neon; ContactMesh contact; };

static WorldMeshes buildWorldMeshes(const LevelData& L) {
    WorldGeometry G = buildWorldGeometry(L);
    WorldMeshes m;
    for (int t = 0; t < TEX_COUNT; ++t) m.byTex[t].upload(G.V[t], G.I[t]);
    m.neon.upload(G.nV, G.nI);
    m.contact.upload(G.contact, false);
    return m;
}
