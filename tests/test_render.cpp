// Check the actual GPU output, including objects after a dense story scene.
#include <SDL2/SDL.h>
#include "render/BoxRenderer.h"
#include <cstdio>

int main() {
    if(SDL_Init(SDL_INIT_VIDEO)!=0) {std::fprintf(stderr,"SDL: %s\n",SDL_GetError());return 1;}
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    auto* window=SDL_CreateWindow("Render regression",0,0,128,128,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    if(!window) {std::fprintf(stderr,"window: %s\n",SDL_GetError());SDL_Quit();return 1;}
    auto context=SDL_GL_CreateContext(window);
    if(!context) {std::fprintf(stderr,"GL: %s\n",SDL_GetError());SDL_DestroyWindow(window);SDL_Quit();return 1;}
#if !defined(__APPLE__) && !defined(__EMSCRIPTEN__)
    glewExperimental=GL_TRUE;glewInit();glGetError();
#endif
    int failures=0;
    {
        BoxRenderer renderer;
        renderer.shader.use();
        renderer.shader.setMat4("view",glm::mat4(1));
        renderer.shader.setMat4("projection",glm::perspective(glm::radians(60.f),1.f,.05f,100.f));
        renderer.shader.setVec3("lightDir",{0,-1,-1});
        renderer.shader.setVec3("viewPos",{0,0,0});
        renderer.shader.setInt("uTexture",0);
        GLuint texture;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
        const unsigned char white[]={255,255,255,255};
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glViewport(0,0,128,128);glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
        meshlib::MeshData triangle;
        triangle.pos={{-1,-1,0},{1,-1,0},{0,1,0}};
        triangle.nrm=std::vector<glm::vec3>(3,{0,0,1});triangle.idx={0,1,2};
        const int mesh=meshlib::add(std::move(triangle));
        for(int pass=0;pass<3;++pass) {
            // The final object is the only one inside the camera's view.
            std::vector<BoxInstance> instances(pass==2?40001:18001);
            for(auto& b:instances)b.model=glm::translate(glm::mat4(1),glm::vec3{100,0,-3});
            auto& item=instances.back();
            item.model=glm::translate(glm::mat4(1),glm::vec3{0,0,-3});
            item.color={0,0,0};item.emissive={1,0,0};
            if(pass) {instances.front().mesh=(float)mesh;item.mesh=(float)mesh;}
            glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
            renderer.draw(instances);
            unsigned char pixel[4]={};glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
            bool ok=pixel[0]>128 && pixel[1]<30 && pixel[2]<30 && glGetError()==GL_NO_ERROR;
            std::printf("%s: final object is rendered after %zu instances (%s)\n",ok?"ok":"FAIL",instances.size(),pass?"mixed meshes":"cubes");
            failures+=!ok;
        }
        glDeleteTextures(1,&texture);
    }
    SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();
    return failures?1:0;
}
