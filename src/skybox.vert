#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aTexCoord;
out vec2 vUV;
uniform mat4 view;
uniform mat4 projection;
void main() {
    vUV = aTexCoord;
    gl_Position = vec4(aPos, 0.9999, 1.0);
}
