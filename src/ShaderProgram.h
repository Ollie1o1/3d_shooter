#pragma once
#include <string>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include "gl.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// =============================================================================
// ShaderProgram
// =============================================================================
// Compiles a vertex + fragment shader pair into a GL program and exposes
// helpers to set uniform variables (the values you pass from CPU to GPU).
//
// GLSL shaders live in src/shader.vert and src/shader.frag.
// They are loaded from disk at runtime so you can edit and re-run without
// recompiling the C++ code.
//
// UNIFORMS explained:
//   Uniforms are variables set from C++ that stay constant for every vertex/
//   fragment in a single draw call. The three core uniforms are:
//     - "model"      — where in the world this object sits (translation, rotation, scale)
//     - "view"       — the camera transform (inverse of camera position/rotation)
//     - "projection" — perspective distortion (makes far things look small)
//   Together these make the MVP matrix pipeline: clip_pos = projection * view * model * vertex_pos
//
// HOW TO ADD A NEW UNIFORM:
//   1. Declare it in the shader:   uniform float myValue;
//   2. Add a helper here:          void setFloat(const char* name, float v) const { ... }
//   3. Call it in main.cpp before drawing: shader.setFloat("myValue", 0.5f);
//
// HOW TO ADD A NEW SHADER (e.g., for post-processing or particles):
//   Just create another ShaderProgram instance and load different .vert/.frag files.
//   Switch between shaders with shader.use() before each draw call.
// =============================================================================

class ShaderProgram {
public:
    GLuint id = 0; // OpenGL program handle — 0 means not compiled yet

    ShaderProgram() = default;
    ShaderProgram(const ShaderProgram&) = delete;
    ~ShaderProgram() { if (id) glDeleteProgram(id); }

    // Load vertex and fragment shaders from disk, compile, and link.
    // Throws std::runtime_error with the GL error log if anything fails.
    void loadFiles(const std::string& vertPath, const std::string& fragPath) {
        std::string vs = readFile(vertPath);
        std::string fs = readFile(fragPath);
        compile(vs.c_str(), fs.c_str());
    }

    // Bind this shader program — all subsequent draw calls will use it until
    // another shader calls use() or glUseProgram(0) is called.
    void use() const { glUseProgram(id); }

    // -------------------------------------------------------------------------
    // Uniform setters — call these after use() and before draw().
    // glGetUniformLocation looks up the variable name in the compiled shader.
    // -------------------------------------------------------------------------

    // Set a 4x4 matrix (e.g., model, view, projection)
    void setMat4(const char* name, const glm::mat4& m) const {
        glUniformMatrix4fv(loc(name), 1, GL_FALSE, glm::value_ptr(m));
    }

    // Set a vec3 (e.g., light direction, color, camera position)
    void setVec3(const char* name, const glm::vec3& v) const {
        glUniform3fv(loc(name), 1, glm::value_ptr(v));
    }

    void setVec2(const char* name, const glm::vec2& v) const {
        glUniform2fv(loc(name), 1, glm::value_ptr(v));
    }

    // Set an integer (e.g., texture slot index: 0 for GL_TEXTURE0)
    void setInt(const char* name, int v) const {
        glUniform1i(loc(name), v);
    }

    // Set a float (e.g., time, opacity, fog density)
    void setFloat(const char* name, float v) const {
        glUniform1f(loc(name), v);
    }

    // Set a vec4
    void setVec4(const char* name, const glm::vec4& v) const {
        glUniform4fv(loc(name), 1, glm::value_ptr(v));
    }

    // Set an array of vec3 (name is the array's base name, e.g. "pointLightPos")
    void setVec3Array(const char* name, const glm::vec3* arr, int count) const {
        glUniform3fv(loc(name), count, glm::value_ptr(arr[0]));
    }

    // Uniform locations are looked up once and cached: glGetUniformLocation is
    // a string search in the driver, and in the browser every GL call is a
    // trip from WebAssembly into JavaScript.
    GLint loc(const char* name) const {
        auto it = locations.find(name);
        if (it != locations.end()) return it->second;
        GLint l = glGetUniformLocation(id, name);
        locations.emplace(name, l);
        return l;
    }

private:
    mutable std::unordered_map<std::string, GLint> locations;

    static std::string readFile(const std::string& path) {
        std::ifstream f(path);
        if (!f) throw std::runtime_error("Cannot open shader: " + path);
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    void compile(const char* vs, const char* fs) {
        GLuint vert = compileStage(GL_VERTEX_SHADER,   vs);
        GLuint frag = compileStage(GL_FRAGMENT_SHADER, fs);

        id = glCreateProgram();
        glAttachShader(id, vert);
        glAttachShader(id, frag);
        glLinkProgram(id);

        GLint ok;
        glGetProgramiv(id, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetProgramInfoLog(id, sizeof log, nullptr, log);
            glDeleteProgram(id);
            id = 0;
            std::fprintf(stderr, "Shader link error:\n%s\n", log);
            throw std::runtime_error(std::string("Shader link error:\n") + log);
        }

        // Shader objects are no longer needed after linking
        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    static GLuint compileStage(GLenum type, const char* src) {
#ifdef __EMSCRIPTEN__
        // Shaders are written as desktop GLSL 3.30 core; WebGL2 wants GLSL
        // ES 3.00. The dialects are close enough that swapping the version
        // line and declaring default precision is all that's needed.
        std::string es = src;
        auto nl = es.find('\n');
        es = "#version 300 es\nprecision highp float;\nprecision highp int;\n"
           + (nl == std::string::npos ? std::string() : es.substr(nl + 1));
        src = es.c_str();
#endif
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);

        GLint ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetShaderInfoLog(s, sizeof log, nullptr, log);
            glDeleteShader(s);
            std::fprintf(stderr, "Shader compile error:\n%s\n", log); // visible even where exceptions abort
            throw std::runtime_error(std::string("Shader compile error:\n") + log);
        }
        return s;
    }
};
