#version 330 core
// Contact shadows: soft dark patches on the ground around whatever stands on
// it (WorldGeometry.h builds the static ones; enemies add theirs each frame)
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aRect;            // the footprint, x0 z0 x1 z1
layout(location = 2) in vec2 aStrengthReach;   // darkest, and how far it reaches past the footprint (m)

out vec3 vPos;
out vec4 vRect;
out vec2 vSR;

uniform mat4 view;
uniform mat4 projection;

void main() {
    vPos = aPos; vRect = aRect; vSR = aStrengthReach;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
