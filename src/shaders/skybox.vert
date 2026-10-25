#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aTexCoord;
out vec2 vNDC;
void main() {
    vNDC = aPos;
    gl_Position = vec4(aPos, 1.0, 1.0);   // the far plane: drawn after the world, it fills only what the depth clear left
}
