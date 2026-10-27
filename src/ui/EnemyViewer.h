#pragma once
// =============================================================================
// EnemyViewer.h — `./shooter --viewer husk`: one enemy on a stand, to work on
// its file (assets/enemies/<name>.glb) in Blender. It watches the file and
// reloads it whenever you save; a broken save keeps the last good model.
//
//   drag     orbit          wheel  zoom          T  turntable on/off
//   W        walk on/off    F      wind up a shot (loops while held on: P)
//   H        hit flash      J      show joints   B  broken (parried) on/off   Esc quit
//
// It's posed by the game's own buildEnemy and drawn with the game's shader
// and bloom, so what you see is what the game will show.
// =============================================================================
#include "core/GameState.h"
#include "core/gl.h"
#include "core/ShaderProgram.h"
#include "core/Display.h"
#include "render/ViewModel.h"
#include "render/PostProcess.h"
#include "render/WorldMesh.h"
#include "render/EnemyModel.h"
#include "render/BoxRenderer.h"
#include "ui/UIBatch.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>
#include <functional>
#include <string>

class EnemyViewerState : public GameState {
public:
    std::function<void()> onQuit;

    EnemyViewerState(int w, int h, EnemyType t) : ui(w, h), screenW(w), screenH(h), type(t), e(t, {0.f, 0.f, 0.f}) {
        shader.loadFiles("src/shaders/shader.vert", "src/shaders/shader.frag");
        whiteTex = makeGreyTexture();
        e.spawnTimer = 0.f;
        std::error_code ec;
        fileTime = std::filesystem::last_write_time(enemyfile::path(type), ec);
        enemyfile::model(type);
        if (g_devOverlay.find("walk") != std::string::npos) walking = true;   // --overlay walk / aim / windup: hold a state (screenshots)
        if (g_devOverlay.find("windup") != std::string::npos) holdWind = 0.75f;
        else if (g_devOverlay.find("aim") != std::string::npos) holdWind = 0.4f;
        if (g_devOverlay.find("joints") != std::string::npos) showJoints = true;
        if (g_devOverlay.find("broken") != std::string::npos) broken = true;
        if (g_devCam) { yaw = g_devCamYaw; pitch = g_devCamPitch; zoom = std::max(0.15f, g_devCamPos.x); turntable = false; }   // --cam ZOOM 0 0 YAW PITCH
    }
    ~EnemyViewerState() override { glDeleteTextures(1, &whiteTex); }

    void handleEvent(const SDL_Event& ev) override {
        if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) { dragging = true; turntable = false; }
        if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT) dragging = false;
        if (ev.type == SDL_MOUSEMOTION && dragging) {
            yaw -= ev.motion.xrel * 0.4f;
            pitch = glm::clamp(pitch + ev.motion.yrel * 0.4f, -85.f, 85.f);
        }
        if (ev.type == SDL_MOUSEWHEEL) zoom = glm::clamp(zoom * std::pow(0.9f, (float)ev.wheel.y), 0.15f, 4.f);
        if (ev.type != SDL_KEYDOWN || ev.key.repeat) return;
        switch (ev.key.keysym.sym) {
            case SDLK_ESCAPE: if (onQuit) onQuit(); break;
            case SDLK_w: walking = !walking; break;
            case SDLK_f: windT = 0.f; winding = true; break;
            case SDLK_p: loop = !loop; winding = loop; windT = 0.f; break;
            case SDLK_h: e.hitFlashTimer = 0.12f; e.flinchAmp = hitreact::FLINCH_MAX * 0.6f; e.flinchAt = clock; break;
            case SDLK_j: showJoints = !showJoints; break;
            case SDLK_b: broken = !broken; break;
            case SDLK_t: turntable = !turntable; break;
            default: break;
        }
    }

    void update(float dt) override {
        clock += dt;
        if (turntable) yaw += dt * 20.f;
        const EnemyStats& st = e.stats();
        e.moveSpeed = walking ? st.speed : 0.f;
        if (walking) e.animPhase += dt * 9.f;
        e.hitFlashTimer = std::max(0.f, e.hitFlashTimer - dt);
        e.staggerTimer = broken ? 1.f : 0.f;
        const float WIND = 0.8f;
        e.attack = AttackKind::NONE; e.telegraphTimer = 0.f;
        if (holdWind >= 0.f) { e.attack = AttackKind::SHOT; e.telegraphDuration = WIND; e.telegraphTimer = WIND * (1.f - holdWind); }
        else if (winding) {
            windT += dt;
            if (windT >= WIND + 0.4f) { windT = 0.f; winding = loop; }
            if (windT < WIND) { e.attack = AttackKind::SHOT; e.telegraphDuration = WIND; e.telegraphTimer = WIND - windT; }
        }
        pollT -= dt;
        if (pollT <= 0.f) {   // the file: reload it when it changes (and once more a moment later if a read failed mid-save)
            pollT = 0.5f;
            std::error_code ec;
            auto t = std::filesystem::last_write_time(enemyfile::path(type), ec);
            bool changed = !ec && t != fileTime;
            if (changed || (retry && clock > retryAt)) {
                if (changed) fileTime = t;
                bool again = retry && !changed;
                retry = false;
                if (enemyfile::load(type, enemyfile::path(type))) { status = "reloaded"; statusT = 2.f; }
                else if (!again) { retry = true; retryAt = clock + 1.f; }
            }
        }
        statusT = std::max(0.f, statusT - dt);
    }

    void render() override {
        post.samples = 4;
        post.beginScene();
        glClearColor(0.05f, 0.05f, 0.07f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        float h = e.height();
        glm::vec3 centre{0.f, h * 0.5f, 0.f};
        float aspect = (float)display::renderW() / (float)std::max(1, display::renderH());
        glm::vec3 dir{std::cos(glm::radians(pitch)) * std::sin(glm::radians(yaw)), std::sin(glm::radians(pitch)),
                      std::cos(glm::radians(pitch)) * std::cos(glm::radians(yaw))};
        glm::vec3 eye = centre + dir * (h * 1.9f * zoom);
        glm::mat4 view = glm::lookAt(eye, centre, {0.f, 1.f, 0.f});
        glm::mat4 proj = glm::perspective(glm::radians(40.f), aspect, 0.02f, 50.f);

        shader.use();
        shader.setMat4("projection", proj);
        shader.setMat4("view", view);
        shader.setVec3("lightDir", glm::normalize(glm::vec3{0.4f, -1.f, 0.3f}));
        shader.setVec3("lightColor", {1.f, 0.9f, 0.8f});
        shader.setVec3("uSkyAmb", {0.32f, 0.32f, 0.38f});
        shader.setVec3("uGroundAmb", {0.12f, 0.11f, 0.10f});
        shader.setFloat("uFogDensity", 0.f);
        shader.setFloat("uPSXStrength", 0.f);
        shader.setFloat("uVertexGlow", 0.f);
        shader.setVec3("uTexGlow", {0.f, 0.f, 0.f});
        shader.setInt("uNeonCount", 0);
        shader.setVec3("viewPos", eye);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, whiteTex);
        shader.setInt("uTexture", 0);

        draw(glm::translate(glm::mat4(1.f), {0.f, -0.02f, 0.f}) * glm::scale(glm::mat4(1.f), {h * 1.2f, 0.04f, h * 1.2f}),
             {0.08f, 0.08f, 0.09f}, glm::vec3{0.f});
        labels.clear();
        if (showJoints) {
            AABB head;
            if (rig::headBox(e, head)) {   // the head hitbox, as a faint red frame
                glm::vec3 c = (head.min + head.max) * 0.5f, s = head.max - head.min;
                for (int a = 0; a < 3; ++a)
                    for (int k = 0; k < 4; ++k) {
                        glm::vec3 o = c, len{0.012f};
                        int b1 = (a + 1) % 3, b2 = (a + 2) % 3;
                        o[b1] += (k & 1 ? 0.5f : -0.5f) * s[b1]; o[b2] += (k & 2 ? 0.5f : -0.5f) * s[b2];
                        len[a] = s[a];
                        draw(glm::translate(glm::mat4(1.f), o) * glm::scale(glm::mat4(1.f), len), {1.f, 0.2f, 0.2f}, {1.5f, 0.2f, 0.2f});
                    }
            }
        }
        shader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
        // the enemy: the game's instanced renderer and shader (boxes and meshes)
        boxes.clear();
        buildEnemy(e, clock, boxes);
        ShaderProgram& bs = boxer.shader;
        bs.use();
        bs.setMat4("projection", proj);
        bs.setMat4("view", view);
        bs.setVec3("lightDir", glm::normalize(glm::vec3{0.4f, -1.f, 0.3f}));
        bs.setVec3("lightColor", {1.f, 0.9f, 0.8f});
        bs.setVec3("uSkyAmb", {0.32f, 0.32f, 0.38f});
        bs.setVec3("uGroundAmb", {0.12f, 0.11f, 0.10f});
        bs.setFloat("uFogDensity", 0.f);
        bs.setInt("uNeonCount", 0);
        bs.setVec3("viewPos", eye);
        bs.setInt("uTexture", 0);
        glBindTexture(GL_TEXTURE_2D, whiteTex);
        boxer.draw(boxes);
        post.endScene();
        overlay();
    }

private:
    ShaderProgram shader;
    PostProcess post{display::renderW(), display::renderH()};
    ViewModel vm;   // (for its unit cube)
    UIBatch ui;
    int screenW, screenH;
    GLuint whiteTex = 0;
    EnemyType type;
    Enemy e;
    std::vector<BoxInstance> boxes;
    BoxRenderer boxer;
    float yaw = 200.f, pitch = 8.f, zoom = 1.f, clock = 0.f;
    bool dragging = false, turntable = true, showJoints = false, walking = false, winding = false, loop = false, broken = false;
    float windT = 0.f, holdWind = -1.f;
    std::filesystem::file_time_type fileTime{};
    float pollT = 0.f, retryAt = 0.f, statusT = 0.f;
    bool retry = false;
    std::string status;
    struct Label { std::string text; glm::vec2 at; };
    std::vector<Label> labels;

    void draw(const glm::mat4& model, glm::vec3 color, glm::vec3 emissive) {
        shader.setMat4("model", model);
        shader.setVec3("objectColor", color);
        shader.setVec3("emissiveColor", emissive);
        vm.cubeMesh.draw();
    }
    void overlay() {
        const auto& m = enemyfile::slot(type);
        ui.begin();
        char buf[200];
        std::snprintf(buf, sizeof buf, "%s   %s", e.stats().name, enemyfile::path(type).c_str());
        ui.textShadow(buf, 20, 18, 3, {0.95f, 0.92f, 0.85f, 1.f});
        int nMesh = 0;
        for (auto& p : m.parts) nMesh += p.mesh > 0;
        std::snprintf(buf, sizeof buf, "%zu parts (%d meshes), %zu joints%s%s", m.parts.size(), nMesh, m.joints.size(),
                      walking ? "   WALKING" : "", e.attack == AttackKind::SHOT ? "   WINDING UP" : "");
        ui.textShadow(buf, 20, 52, 2, {0.7f, 0.72f, 0.78f, 1.f});
        if (!m.error.empty()) {
            ui.rect(14, 80, screenW - 28, 30, {0.35f, 0.05f, 0.05f, 0.85f});
            ui.text(m.error.c_str(), 22, 88, 2, {1.f, 0.85f, 0.8f, 1.f});
        } else if (statusT > 0.f) {
            ui.textShadow(status.c_str(), 20, 84, 2, {0.5f, 1.f, 0.6f, statusT});
        }
        ui.textShadow("W walk   F wind up an attack   P loop it   H hit   B broken   J head hitbox", 20, screenH - 56, 2, {0.6f, 0.62f, 0.68f, 0.9f});
        ui.textShadow("drag orbit   wheel zoom   T turntable   Esc quit", 20, screenH - 30, 2, {0.6f, 0.62f, 0.68f, 0.9f});
        ui.end();
    }
};
