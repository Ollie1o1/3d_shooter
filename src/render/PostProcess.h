#pragma once
#include "core/gl.h"
#include "core/ShaderProgram.h"
#include "core/Display.h"
#include <algorithm>
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Scene → bloom → (CRT) → the display canvas. The scene targets follow the
// render resolution (resized on the fly); bloom always works at 640x360, so
// the glow looks the same at any resolution. With anti-aliasing on, the scene
// is drawn into a multisampled target and resolved into sceneTex before bloom.
class PostProcess {
public:
    int W, H;
    static constexpr int BW = 640, BH = 360;
    GLuint sceneFBO = 0, sceneTex = 0, sceneDepth = 0;
    int samples = 0;         // MSAA asked for (0 or 1 = off); set every frame from the settings
    int activeSamples = 0;   // what the targets were built with (0 if the GPU said no)
    GLuint msFBO = 0, msColor = 0, msDepth = 0;
    static constexpr GLenum SCENE_FORMAT = GL_R11F_G11F_B10F;
    GLuint bloomFBO[2] = {0,0};
    GLuint bloomTex[2] = {0,0};

    ShaderProgram brightShader;
    ShaderProgram blurShader;
    ShaderProgram compositeShader;
    ShaderProgram crtShader;
    bool crtEnabled = false;
    GLuint crtFBO = 0, crtTex = 0;  // intermediate FBO for CRT pass

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

        createTargets();
        brightShader.loadFiles("src/shaders/postprocess.vert","src/shaders/bloom_bright.frag");
        blurShader.loadFiles("src/shaders/postprocess.vert","src/shaders/bloom_blur.frag");
        compositeShader.loadFiles("src/shaders/postprocess.vert","src/shaders/bloom_composite.frag");
        crtShader.loadFiles("src/shaders/postprocess.vert","src/shaders/crt.frag");

        // CRT intermediate FBO
    }

    ~PostProcess() {
        destroyTargets();
        if(quadVAO) glDeleteVertexArrays(1,&quadVAO);
        if(quadVBO) glDeleteBuffers(1,&quadVBO);
    }

    void createTargets() {
        glGenFramebuffers(1,&sceneFBO);
        glBindFramebuffer(GL_FRAMEBUFFER,sceneFBO);
        glGenTextures(1,&sceneTex);
        glBindTexture(GL_TEXTURE_2D,sceneTex);
        // Packed HDR (4 bytes a pixel, no alpha: nothing reads it): half the memory traffic of
        // RGBA16F, which with 4x MSAA was most of its cost. Banding checked: same as before
        glTexImage2D(GL_TEXTURE_2D,0,SCENE_FORMAT,W,H,0,GL_RGB,GL_FLOAT,nullptr);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,sceneTex,0);
        glGenRenderbuffers(1,&sceneDepth);
        glBindRenderbuffer(GL_RENDERBUFFER,sceneDepth);
        glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,W,H);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,sceneDepth);
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        createMultisample();

        for (int i=0;i<2;++i) {
            glGenFramebuffers(1,&bloomFBO[i]);
            glBindFramebuffer(GL_FRAMEBUFFER,bloomFBO[i]);
            glGenTextures(1,&bloomTex[i]);
            glBindTexture(GL_TEXTURE_2D,bloomTex[i]);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,BW,BH,0,GL_RGBA,GL_FLOAT,nullptr);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,bloomTex[i],0);
            glBindFramebuffer(GL_FRAMEBUFFER,0);
        }

        glGenFramebuffers(1,&crtFBO);
        glBindFramebuffer(GL_FRAMEBUFFER,crtFBO);
        glGenTextures(1,&crtTex);
        glBindTexture(GL_TEXTURE_2D,crtTex);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,W,H,0,GL_RGBA,GL_FLOAT,nullptr);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,crtTex,0);
        glBindFramebuffer(GL_FRAMEBUFFER,0);
    }

    // The multisampled scene, if asked for: same formats as the scene, `samples` per pixel
    void createMultisample() {
        activeSamples = 0;
        if (samples <= 1) return;
        GLint maxS = 0;
        glGetIntegerv(GL_MAX_SAMPLES, &maxS);
        int n = std::min(samples, (int)maxS);
        if (n <= 1) return;
        glGenFramebuffers(1, &msFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, msFBO);
        glGenRenderbuffers(1, &msColor);
        glBindRenderbuffer(GL_RENDERBUFFER, msColor);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, n, SCENE_FORMAT, W, H);   // (a resolve needs matching formats)
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msColor);
        glGenRenderbuffers(1, &msDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, msDepth);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, n, GL_DEPTH_COMPONENT24, W, H);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, msDepth);
        bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (ok) activeSamples = n;
        else destroyMultisample();   // (no multisampled float targets here: render without)
        if (activeSamples != loggedSamples) {
            std::fprintf(stderr, ok ? "anti-aliasing: %dx MSAA\n" : "anti-aliasing: %dx MSAA not available, off\n", ok ? activeSamples : n);
            loggedSamples = activeSamples;
        }
    }
    void destroyMultisample() {
        if (msFBO) glDeleteFramebuffers(1, &msFBO);
        if (msColor) glDeleteRenderbuffers(1, &msColor);
        if (msDepth) glDeleteRenderbuffers(1, &msDepth);
        msFBO = msColor = msDepth = 0;
    }

    void destroyTargets() {
        destroyMultisample();
        glDeleteFramebuffers(1,&sceneFBO);
        glDeleteTextures(1,&sceneTex);
        glDeleteRenderbuffers(1,&sceneDepth);
        glDeleteFramebuffers(1,&crtFBO);
        glDeleteTextures(1,&crtTex);
        for(int i=0;i<2;++i) {
            glDeleteFramebuffers(1,&bloomFBO[i]);
            glDeleteTextures(1,&bloomTex[i]);
        }
    }

    void beginScene() {
        // The render resolution changed (window resized, fullscreen, scale setting)
        if (W != display::renderW() || H != display::renderH()) {
            destroyTargets();
            W = display::renderW(); H = display::renderH();
            createTargets();
        }
        int want = samples > 1 ? samples : 0;
        if (want != wantedSamples) {   // the setting changed: rebuild just the multisampled target
            wantedSamples = want;
            destroyMultisample();
            createMultisample();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, activeSamples ? msFBO : sceneFBO);
        glViewport(0,0,W,H);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    }

    int wantedSamples = -1;   // the samples the targets were last built for
    int loggedSamples = 0;
    bool skipBloom = false;   // dev (--skip bloom): composite without the bright pass and blurs

    void endScene() {
        if (activeSamples) {   // resolve: average each pixel's samples into the scene texture
            glBindFramebuffer(GL_READ_FRAMEBUFFER, msFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sceneFBO);
            glBlitFramebuffer(0, 0, W, H, 0, 0, W, H, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        if (!skipBloom) bloomPasses();
        compositeToCanvas();
        glEnable(GL_DEPTH_TEST);
    }

    // Screen effects in the final composite (bloom_composite.frag): set by the game each frame, 0 = off
    struct ScreenFx { float speed = 0.f, low = 0.f, pulse = 0.f, hurt = 0.f, time = 0.f, drive = 0.f, kick = 0.f; };
    ScreenFx fx;

private:
    void setScreenFx() {
        compositeShader.setFloat("uSpeed", fx.speed);
        compositeShader.setFloat("uLow",   fx.low);
        compositeShader.setFloat("uPulse", fx.pulse);
        compositeShader.setFloat("uHurt",  fx.hurt);
        compositeShader.setFloat("uTime",  fx.time);
        compositeShader.setFloat("uDrive", fx.drive);
        compositeShader.setFloat("uKick",  fx.kick);
        compositeShader.setFloat("uAspect", H > 0 ? (float)W / (float)H : 16.f / 9.f);
    }

    void bloomPasses() {
        glBindFramebuffer(GL_FRAMEBUFFER,bloomFBO[0]);
        glViewport(0,0,BW,BH);
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
    }

    void compositeToCanvas() {
        glDisable(GL_DEPTH_TEST);
        if (crtEnabled) {
            // Composite bloom to CRT intermediate FBO
            glBindFramebuffer(GL_FRAMEBUFFER,crtFBO);
            glViewport(0,0,W,H);
            compositeShader.use();
            compositeShader.setInt("scene",0);
            compositeShader.setInt("bloomBlur",1);
            setScreenFx();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,sceneTex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D,bloomTex[0]);
            drawQuad();

            // Apply CRT filter into the display canvas
            glBindFramebuffer(GL_FRAMEBUFFER,display::canvas().fbo);
            glViewport(0,0,W,H);
            crtShader.use();
            crtShader.setInt("scene",0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,crtTex);
            drawQuad();
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER,display::canvas().fbo);
            glViewport(0,0,W,H);
            compositeShader.use();
            compositeShader.setInt("scene",0);
            compositeShader.setInt("bloomBlur",1);
            setScreenFx();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,sceneTex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D,bloomTex[0]);
            drawQuad();
        }
    }

    void drawQuad() {
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES,0,6);
        glBindVertexArray(0);
    }
};
