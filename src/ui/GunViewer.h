#pragma once
// =============================================================================
// GunViewer.h — `./shooter --viewer [gun]`: one gun on a stand, to work on its
// file (assets/guns/<name>.glb) in Blender. It watches the file and reloads it
// whenever you save; a broken save keeps the last good gun and shows why.
//
//   drag     orbit          wheel  zoom          T  turntable on/off
//   1-5      pick a gun (5: the sword)            F  fire (sword: a riposte)  R  reload
//   C        pump / bolt    H      heat 0/half/full
//   [ ]      magazine size (sword: guard meter)   J  show joints   P  demo loop (fire, fire... reload)
//   B        the sword's guard broken / whole
//   Esc      quit
//
// It draws with the game's own shader and bloom, and moves the gun with the
// game's GunMotion, so what you see is what the game will show.
// =============================================================================
#include "core/GameState.h"
#include "core/gl.h"
#include "core/ShaderProgram.h"
#include "core/Display.h"
#include "render/ViewModel.h"
#include "render/PostProcess.h"
#include "render/WorldMesh.h"
#include "ui/UIBatch.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>
#include <functional>
#include <string>

class GunViewerState : public GameState {
public:
    std::function<void()> onQuit;

    GunViewerState(int w, int h, int gun) : ui(w, h), screenW(w), screenH(h) {
        shader.loadFiles("src/shaders/shader.vert", "src/shaders/shader.frag");
        whiteTex = makeGreyTexture();
        pick(gun);
        if (g_devOverlay.find("broken") != std::string::npos) broken = true;    // --overlay broken / charged / riposte: hold a sword state (screenshots)
        if (g_devOverlay.find("charged") != std::string::npos) charge = 1.f;
        if (g_devOverlay.find("riposte") != std::string::npos) riposteT = 1e9f;
        if (g_devCam) { yaw = g_devCamYaw; pitch = g_devCamPitch; zoom = std::max(0.15f, g_devCamPos.x); turntable = false; }   // --cam ZOOM 0 0 YAW PITCH: a fixed view (screenshots)
    }
    ~GunViewerState() override { glDeleteTextures(1, &whiteTex); }

    void handleEvent(const SDL_Event& e) override {
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) { dragging = true; turntable = false; }
        if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) dragging = false;
        if (e.type == SDL_MOUSEMOTION && dragging) {
            yaw -= e.motion.xrel * 0.4f;
            pitch = glm::clamp(pitch + e.motion.yrel * 0.4f, -85.f, 85.f);
        }
        if (e.type == SDL_MOUSEWHEEL) zoom = glm::clamp(zoom * std::pow(0.9f, (float)e.wheel.y), 0.15f, 4.f);
        if (e.type != SDL_KEYDOWN || e.key.repeat) return;
        switch (e.key.keysym.sym) {
            case SDLK_ESCAPE: if (onQuit) onQuit(); break;
            case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4: case SDLK_5: pick(e.key.keysym.sym - SDLK_1); break;
            case SDLK_b: broken = !broken; break;
            case SDLK_f: fire(); break;
            case SDLK_r: reload(); break;
            case SDLK_c: motion.cycle(gun == 1 ? 0.45f : 0.7f); break;
            case SDLK_h:
                if (sword()) { charge = charge > 0.5f ? 0.f : 1.f; break; }
                heatHold = heatHold < 0.25f ? 0.5f : heatHold < 0.75f ? 1.f : 0.f; heat = heatHold; break;
            case SDLK_LEFTBRACKET:  mag = std::max(sword() ? 0 : 1, mag - 1);  ammo = std::min(ammo, mag); break;
            case SDLK_RIGHTBRACKET: mag = std::min(sword() ? 8 : 16, mag + 1); ammo = mag; break;
            case SDLK_j: showJoints = !showJoints; break;
            case SDLK_t: turntable = !turntable; break;
            case SDLK_p: demo = !demo; demoT = 0.f; break;
            default: break;
        }
    }

    void update(float dt) override {
        clock += dt;
        if (turntable) yaw += dt * 20.f;
        bool wasReloading = motion.reloading();
        motion.update(dt);
        motion.cues.clear();
        if (wasReloading && !motion.reloading()) ammo = refillTo;
        flashT = std::max(0.f, flashT - dt);
        riposteT = std::max(0.f, riposteT - dt);
        heat = std::max(heatHold, heat - dt * 0.7f);
        breathPh += dt * (0.3f + 1.4f * heat);
        breathPh -= std::floor(breathPh);
        cylSpin *= std::exp(-dt * 22.f);
        if (demo && !motion.reloading()) {
            demoT += dt;
            if (demoT > 0.9f) { demoT = 0.f; if (ammo > 0) fire(); else reload(); }
        }
        // the file: reload it when it changes (and once more a moment after a failed read: Blender may still be writing)
        pollT -= dt;
        if (pollT <= 0.f) {
            pollT = 0.5f;
            std::error_code ec;
            auto t = std::filesystem::last_write_time(gunkit::gunPath(gun), ec);
            bool changed = !ec && t != fileTime;
            if (changed || (retry && clock > retryAt)) {
                if (changed) fileTime = t;
                bool again = retry && !changed;   // (one more try, a moment after a failed read: Blender may still have been writing)
                retry = false;
                if (gunkit::loadGunModel(gun, gunkit::gunPath(gun))) { status = "reloaded"; statusT = 2.f; frame(); }
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

        float aspect = (float)display::renderW() / (float)std::max(1, display::renderH());
        glm::vec3 dir{std::cos(glm::radians(pitch)) * std::sin(glm::radians(yaw)), std::sin(glm::radians(pitch)),
                      std::cos(glm::radians(pitch)) * std::cos(glm::radians(yaw))};
        glm::vec3 eye = centre + dir * (radius * 2.6f * zoom);
        glm::mat4 view = glm::lookAt(eye, centre, {0.f, 1.f, 0.f});
        glm::mat4 proj = glm::perspective(glm::radians(40.f), aspect, 0.005f, 20.f);

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

        GunLook L = look();
        GunPose P = motion.pose(false);
        parts.clear();
        build(L, P, parts);
        for (const auto& p : parts) draw(p.xf, p.color, p.emissive, p.mesh);
        // the stand: a dark plinth under the gun
        draw(glm::translate(glm::mat4(1.f), {centre.x, lowest - 0.012f, centre.z}) * glm::scale(glm::mat4(1.f), {radius * 1.6f, 0.01f, radius * 2.2f}),
             {0.08f, 0.08f, 0.09f}, glm::vec3{0.f});
        if (showJoints) {   // each joint: a small white marker, an axis tick along its own +Z
            glm::mat4 W[gunkit::J_COUNT];
            const auto& m = gunkit::gunModel(gun);
            gunkit::jointWorlds(m, sword() ? gunkit::rigSword(swordLook()) : gunkit::rigOf(gun, L, P), W);
            glDisable(GL_DEPTH_TEST);
            for (const auto& J : m.joints) {
                draw(W[J.id] * glm::scale(glm::mat4(1.f), glm::vec3{0.006f}), {1.f, 1.f, 1.f}, {1.2f, 1.2f, 1.2f});
                draw(W[J.id] * glm::translate(glm::mat4(1.f), {0.f, 0.f, 0.012f}) * glm::scale(glm::mat4(1.f), {0.002f, 0.002f, 0.024f}),
                     {1.f, 1.f, 1.f}, {0.4f, 1.2f, 0.4f});
            }
            glEnable(GL_DEPTH_TEST);
            jointLabels(m, W, proj * view);
        }
        shader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
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
    std::vector<GunPart> parts;

    int gun = 0, ammo = 8, mag = 8, refillTo = 8;
    GunMotion motion;
    float flashT = 0.f, heat = 0.f, heatHold = 0.f, breathPh = 0.f, cylSpin = 0.f;
    float riposteT = 0.f, charge = 0.f;   // the sword's
    bool broken = false;
    float yaw = 120.f, pitch = 12.f, zoom = 1.f, clock = 0.f;
    bool dragging = false, turntable = true, showJoints = false, demo = false;
    float demoT = 0.f;
    glm::vec3 centre{0.f, 0.f, 0.15f};
    float radius = 0.3f, lowest = -0.1f;
    std::filesystem::file_time_type fileTime{};
    float pollT = 0.f, retryAt = 0.f, statusT = 0.f;
    bool retry = false;
    std::string status;
    struct Label { std::string text; glm::vec2 at; };
    std::vector<Label> labels;

    static int magOf(int g) { static const int M[4] = {8, 6, 5, 5}; return M[std::clamp(g, 0, 3)]; }
    static float reloadTime(int g) { static const float T[4] = {1.4f, 2.f, 1.8f, 2.2f}; return T[std::clamp(g, 0, 3)]; }

    bool sword() const { return gun == gunkit::GUN_SWORD; }
    SwordLook swordLook() const {
        SwordLook S; S.guard01 = mag / 8.f; S.broken = broken; S.charge = charge; S.riposte = riposteT > 0.f ? 1.f : 0.f;
        return S;
    }
    void build(const GunLook& L, const GunPose& P, std::vector<GunPart>& out) const {
        if (sword()) gunkit::buildSword(swordLook(), out);
        else gunkit::buildGun(gun, L, P, out);
    }
    void pick(int g) {
        gun = std::clamp(g, 0, gunkit::GUN_FILES - 1);
        mag = ammo = refillTo = sword() ? 8 : magOf(gun);
        broken = false; charge = 0.f;
        motion.reset(std::min(gun, 3));
        std::error_code ec;
        fileTime = std::filesystem::last_write_time(gunkit::gunPath(gun), ec);
        gunkit::gunModel(gun);   // (first use loads it)
        frame();
    }
    // Centre the camera on the gun at rest
    void frame() {
        std::vector<GunPart> rest;
        build(GunLook{}, GunPose{}, rest);
        if (rest.empty()) return;
        glm::vec3 lo{1e9f}, hi{-1e9f};
        for (auto& p : rest) { glm::vec3 c{p.xf[3]}; lo = glm::min(lo, c); hi = glm::max(hi, c); }
        centre = (lo + hi) * 0.5f;
        radius = std::max(0.05f, glm::length(hi - lo) * 0.5f);
        lowest = lo.y - 0.03f;
    }
    void fire() {
        if (sword()) { riposteT = 0.35f; return; }
        if (motion.reloading()) return;
        motion.fire();
        flashT = 0.06f; heat = std::min(1.f, heat + 0.22f); cylSpin = 45.f;
        ammo = std::max(0, ammo - 1);
        if (gun >= 1) motion.cycle(gun == 1 ? 0.45f : 0.7f);
    }
    void reload() {
        if (sword() || motion.reloading() || ammo == mag) return;
        int refill = gun == 1 ? mag - ammo : gun == 2 ? std::min(5, mag - ammo) : mag;
        refillTo = gun == 0 || gun == 3 ? mag : std::min(mag, ammo + refill);
        motion.reload(reloadTime(gun), ammo, mag, refill);
    }
    GunLook look() const {
        GunLook L;
        L.ammo = ammo; L.mag = mag; L.lit = motion.litCells(ammo, mag);
        L.flash = glm::clamp(flashT / 0.06f, 0.f, 1.f); L.heat = heat; L.cylSpin = cylSpin;
        L.refill = std::max(1, refillTo - ammo);
        L.breath = 0.5f + 0.5f * std::sin(breathPh * 6.2831853f);
        return L;
    }
    void draw(const glm::mat4& model, glm::vec3 color, glm::vec3 emissive, int mesh = 0) {
        shader.setMat4("model", model);
        shader.setVec3("objectColor", color);
        shader.setVec3("emissiveColor", emissive);
        vm.drawShape(mesh);   // (a mesh part whole, as the game draws it)
    }
    void jointLabels(const gunkit::GunModel& m, const glm::mat4* W, const glm::mat4& vp) {
        labels.clear();
        for (const auto& J : m.joints) {
            glm::vec4 c = vp * W[J.id][3];
            if (c.w <= 0.f) continue;
            glm::vec2 ndc{c.x / c.w, c.y / c.w};
            labels.push_back({gunkit::jointName(J.id), {(ndc.x * 0.5f + 0.5f) * screenW + 8.f, (0.5f - ndc.y * 0.5f) * screenH - 6.f}});
        }
    }
    void overlay() {
        const auto& m = gunkit::gunModelSlot(gun);
        ui.begin();
        char buf[160];
        std::snprintf(buf, sizeof buf, "%s   %s", gunkit::gunName(gun), gunkit::gunPath(gun).c_str());
        ui.textShadow(buf, 20, 18, 3, {0.95f, 0.92f, 0.85f, 1.f});
        if (sword())
            std::snprintf(buf, sizeof buf, "%zu boxes, %zu joints   guard %d/8%s%s", m.parts.size(), m.joints.size(), mag,
                          broken ? "   BROKEN" : "", charge > 0.5f ? "   CHARGED" : "");
        else
            std::snprintf(buf, sizeof buf, "%zu boxes, %zu joints   ammo %d/%d   heat %.1f%s", m.parts.size(), m.joints.size(), ammo, mag, heat,
                          demo ? "   DEMO" : "");
        ui.textShadow(buf, 20, 52, 2, {0.7f, 0.72f, 0.78f, 1.f});
        if (!m.error.empty()) {   // a broken save: what's wrong (the last good gun stays up)
            ui.rect(14, 80, screenW - 28, 30, {0.35f, 0.05f, 0.05f, 0.85f});
            ui.text(m.error.c_str(), 22, 88, 2, {1.f, 0.85f, 0.8f, 1.f});
        } else if (statusT > 0.f) {
            ui.textShadow(status.c_str(), 20, 84, 2, {0.5f, 1.f, 0.6f, statusT});
        }
        if (showJoints) for (auto& l : labels) ui.textShadow(l.text.c_str(), l.at.x, l.at.y, 2, {1.f, 1.f, 1.f, 0.95f});
        ui.textShadow("F fire   R reload   C pump/bolt   H heat   [ ] mag size   P demo loop",
                      20, screenH - 56, 2, {0.6f, 0.62f, 0.68f, 0.9f});
        if (sword()) ui.textShadow("SWORD:  [ ] guard meter   H charge   F riposte   B broken",
                                   20, screenH - 82, 2, {0.95f, 0.55f, 0.7f, 0.9f});
        ui.textShadow("drag orbit   wheel zoom   T turntable   1-5 gun (5 sword)   J joints   Esc quit",
                      20, screenH - 30, 2, {0.6f, 0.62f, 0.68f, 0.9f});
        ui.end();
    }
};
