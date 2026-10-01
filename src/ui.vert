#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec4 aColor;
uniform mat4 projection;
out vec4 vColor;
void main() {
    vColor = aColor;
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
}
