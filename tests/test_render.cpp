// Check the actual GPU output, including objects after a dense story scene.
#include <SDL2/SDL.h>
#include "render/BoxRenderer.h"
#include "render/ViewModel.h"
#include <cstdio>

int main() {
    if(SDL_Init(SDL_INIT_VIDEO)!=0) {std::fprintf(stderr,"SDL: %s\n",SDL_GetError());return 1;}
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
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
            glClear(GL_COLOR_BUFFER_BIT);
            const auto submitted=renderer.drawVisible(instances,glm::perspective(glm::radians(60.f),1.f,.05f,100.f));
            glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
            ok=submitted==1 && pixel[0]>128 && pixel[1]<30 && pixel[2]<30 && glGetError()==GL_NO_ERROR;
            std::printf("%s: culling keeps the final object and submits only %zu of %zu instances\n",ok?"ok":"FAIL",submitted,instances.size());
            failures+=!ok;
        }
        // Compare every pixel to the unculled path. Include a mesh whose
        // vertices are offset from its origin, thin/large boxes crossing clip
        // planes, and transformed parts whose centres are outside the view.
        meshlib::MeshData offset;
        offset.pos={{3,-1,0},{5,-1,0},{4,1,0}};
        offset.nrm=std::vector<glm::vec3>(3,{0,0,1});offset.idx={0,1,2};
        const int offsetMesh=meshlib::add(std::move(offset));
        std::vector<BoxInstance> scene;
        auto part=[&](glm::vec3 at,glm::vec3 scale,int shape=0){
            BoxInstance b;
            b.model=glm::translate(glm::mat4(1),at)*glm::rotate(glm::mat4(1),.6f,glm::vec3{0,0,1})*glm::scale(glm::mat4(1),scale);
            b.color={0,0,0};b.emissive={.8f,.3f,.6f};b.mesh=(float)shape;
            scene.push_back(b);
        };
        part({100,0,-3},{1,1,1});part({0,0,3},{1,1,1});
        part({2,0,-3},{5,.04f,.5f});part({-2,0,-4},{-2,.4f,.7f});
        part({0,0,-.06f},{.04f,.04f,.1f});part({0,1,-6},{1,2,1},mesh);
        part({-4,0,-4},{1,1,1},offsetMesh);
        scene.back().model[1][0]+=.4f; // shear is valid too
        std::vector<unsigned char> reference(128*128*4),culled(reference.size());
        const glm::mat4 projection=glm::perspective(glm::radians(60.f),1.f,.05f,100.f);
        for(float yaw:{0.f,.7f,1.6f,3.14f}){
            const glm::mat4 view=glm::rotate(glm::mat4(1),yaw,glm::vec3{0,1,0});
            renderer.shader.setMat4("view",view);
            glClear(GL_COLOR_BUFFER_BIT);renderer.draw(scene);
            glReadPixels(0,0,128,128,GL_RGBA,GL_UNSIGNED_BYTE,reference.data());
            glClear(GL_COLOR_BUFFER_BIT);renderer.drawVisible(scene,projection*view);
            glReadPixels(0,0,128,128,GL_RGBA,GL_UNSIGNED_BYTE,culled.data());
            const bool ok=reference==culled && glGetError()==GL_NO_ERROR;
            std::printf("%s: culled frame matches every reference pixel at yaw %.2f\n",ok?"ok":"FAIL",yaw);
            failures+=!ok;
        }
        {
            ShaderProgram legacy;legacy.loadFiles("src/shaders/shader.vert","src/shaders/shader.frag");
            ViewModel referenceVm,batchedVm;
            ShaderProgram& instanced=batchedVm.batchShader();
            Camera camera({0,0,0});camera.aspectRatio=1;
            auto lighting=[&](ShaderProgram& sh){
                sh.use();sh.setInt("uTexture",0);
                sh.setInt("uNormalMap",1);sh.setInt("uBake",3);sh.setInt("uGlowBake",4);
                sh.setMat4("view",camera.viewMatrix());sh.setMat4("projection",camera.projectionMatrix());
                sh.setVec3("lightDir",glm::normalize(glm::vec3{.4f,-1,.3f}));sh.setVec3("lightColor",{1,.9f,.8f});
                sh.setVec3("uSkyAmb",{.32f,.32f,.38f});sh.setVec3("uGroundAmb",{.12f,.11f,.1f});sh.setVec3("viewPos",{0,0,0});
                sh.setFloat("uFogDensity",0);
            };
            glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture);
            glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);glDisable(GL_CULL_FACE);
            for(int pose=0;pose<8;++pose){
                Sword sword;sword.raised=true;
                if(pose==6){sword.raised=false;sword.update(.01f,{true,true,false,false});sword.update(.12f,{});}
                for(auto* vm:{&referenceVm,&batchedVm}){
                    vm->reset(pose<4?pose:0);
                    if(pose==4)vm->triggerParry(true);
                    vm->update(.12f,0,true);vm->tickSword(.12f,sword);
                }
                auto draw=[&](ViewModel& vm,ShaderProgram& sh){
                    lighting(sh);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
                    if(pose==5 || pose==6)vm.drawSword(sh,camera,sword);
                    else if(pose==7){ViewModel::FreeArm arm;arm.shoulder={.3f,-.6f,0};arm.fist=glm::translate(glm::mat4(1),glm::vec3{.25f,-.25f,.35f});vm.drawFreeArms(sh,camera,&arm,1);}
                    else vm.draw(sh,camera,pose<4?pose:0,0,pose==2?1.f:0.f,8,8);
                };
                draw(referenceVm,legacy);glReadPixels(0,0,128,128,GL_RGBA,GL_UNSIGNED_BYTE,reference.data());
                batchedVm.resetBatchStats();draw(batchedVm,instanced);
                glReadPixels(0,0,128,128,GL_RGBA,GL_UNSIGNED_BYTE,culled.data());
                size_t differences=0;int largest=0;
                for(size_t i=0;i<reference.size();++i){const int d=std::abs((int)reference[i]-(int)culled[i]);differences+=d>2;largest=std::max(largest,d);}
                const GLenum error=glGetError();
                const bool visible=std::count_if(reference.begin(),reference.end(),[](unsigned char c){return c>0 && c<255;})>100;
                const bool ok=visible && differences<reference.size()/1000 && largest<24 && batchedVm.batchPartCount>0 &&
                              batchedVm.batchDrawCalls<batchedVm.batchPartCount && error==GL_NO_ERROR;
                std::printf("%s: view-model pose %d preserves pixels (%zu channels >2, max %d), %d parts / %d draws (GL %x)\n",
                            ok?"ok":"FAIL",pose,differences,largest,batchedVm.batchPartCount,batchedVm.batchDrawCalls,error);
                failures+=!ok;
            }
        }
        glDeleteTextures(1,&texture);
    }
    SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();
    return failures?1:0;
}
