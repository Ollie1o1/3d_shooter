#pragma once
#include "gl.h"
#include "ShaderProgram.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class PostProcess {
public:
    int W, H;
    GLuint sceneFBO = 0, sceneTex = 0, sceneDepth = 0;
    GLuint bloomFBO[2] = {0,0};
    GLuint bloomTex[2] = {0,0};

    ShaderProgram brightShader;
    ShaderProgram blurShader;
    ShaderProgram compositeShader;

    GLuint quadVAO = 0, quadVBO = 0;

    PostProcess(int w, int h) : W(w), H(h) {
        float verts[] = {
            -1,-1,0,0,  1,-1,1,0,  1,1,1,1,
            -1,-1,0,0,  1,1,1,1,  -1,1,0,1
        };
        glGenVertexArrays(1,&quadVAO);
        glGenBuffers(1,&quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER,quadVBO);
        glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
        glBindVertexArray(0);

        glGenFramebuffers(1,&sceneFBO);
        glBindFramebuffer(GL_FRAMEBUFFER,sceneFBO);
        glGenTextures(1,&sceneTex);
        glBindTexture(GL_TEXTURE_2D,sceneTex);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,W,H,0,GL_RGBA,GL_FLOAT,nullptr);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,sceneTex,0);
        glGenRenderbuffers(1,&sceneDepth);
        glBindRenderbuffer(GL_RENDERBUFFER,sceneDepth);
        glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,W,H);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,sceneDepth);
        glBindFramebuffer(GL_FRAMEBUFFER,0);

        for (int i=0;i<2;++i) {
            glGenFramebuffers(1,&bloomFBO[i]);
            glBindFramebuffer(GL_FRAMEBUFFER,bloomFBO[i]);
            glGenTextures(1,&bloomTex[i]);
            glBindTexture(GL_TEXTURE_2D,bloomTex[i]);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,W/2,H/2,0,GL_RGBA,GL_FLOAT,nullptr);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,bloomTex[i],0);
            glBindFramebuffer(GL_FRAMEBUFFER,0);
        }

        brightShader.loadFiles("src/postprocess.vert","src/bloom_bright.frag");
        blurShader.loadFiles("src/postprocess.vert","src/bloom_blur.frag");
        compositeShader.loadFiles("src/postprocess.vert","src/bloom_composite.frag");
    }

    ~PostProcess() {
        glDeleteFramebuffers(1,&sceneFBO);
        glDeleteTextures(1,&sceneTex);
        glDeleteRenderbuffers(1,&sceneDepth);
        for(int i=0;i<2;++i) {
            glDeleteFramebuffers(1,&bloomFBO[i]);
            glDeleteTextures(1,&bloomTex[i]);
        }
        if(quadVAO) glDeleteVertexArrays(1,&quadVAO);
        if(quadVBO) glDeleteBuffers(1,&quadVBO);
    }

    void beginScene() {
        glBindFramebuffer(GL_FRAMEBUFFER,sceneFBO);
        glViewport(0,0,W,H);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    }

    void endScene() {
        glBindFramebuffer(GL_FRAMEBUFFER,bloomFBO[0]);
        glViewport(0,0,W/2,H/2);
        glDisable(GL_DEPTH_TEST);
        brightShader.use();
        brightShader.setInt("scene",0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,sceneTex);
        drawQuad();

        bool horizontal = true;
        for (int i=0;i<10;++i) {
            glBindFramebuffer(GL_FRAMEBUFFER,bloomFBO[horizontal?1:0]);
            blurShader.use();
            blurShader.setInt("image",0);
            blurShader.setInt("horizontal",horizontal?1:0);
            glBindTexture(GL_TEXTURE_2D,bloomTex[horizontal?0:1]);
            drawQuad();
            horizontal = !horizontal;
        }

        glBindFramebuffer(GL_FRAMEBUFFER,0);
        glViewport(0,0,W,H);
        compositeShader.use();
        compositeShader.setInt("scene",0);
        compositeShader.setInt("bloomBlur",1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,sceneTex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D,bloomTex[0]);
        drawQuad();
        glEnable(GL_DEPTH_TEST);
    }

private:
    void drawQuad() {
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES,0,6);
        glBindVertexArray(0);
    }
};
