#pragma once
// =============================================================================
// WorldMesh.h — the static level geometry, batched into one mesh per texture,
// and the 1x1 grey texture untextured things are drawn with.
// =============================================================================
#include "WorldGeometry.h"
#include "Mesh.h"
#include "gl.h"
#include <vector>
#include <algorithm>
#include <cmath>

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

struct WorldMeshes { Mesh byTex[TEX_COUNT]; Mesh neon; };

static WorldMeshes buildWorldMeshes(const LevelData& L) {
    WorldGeometry G = buildWorldGeometry(L);
    WorldMeshes m;
    for (int t = 0; t < TEX_COUNT; ++t) m.byTex[t].upload(G.V[t], G.I[t]);
    m.neon.upload(G.nV, G.nI);
    return m;
}
