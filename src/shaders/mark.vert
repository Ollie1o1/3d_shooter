#version 330 core
// Marks left on the world (render/Marks.h): a quad laid on the surface it marks
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aUvSeedStrength;   // across the mark -1..1, its outline's seed, how strong it is now
layout(location = 2) in vec4 aTintKind;         // what it does to the surface's colour, and which kind (0 pit, 1 scorch, 2 stain)

out vec3 vPos;
out vec4 vUv;
out vec4 vTint;

uniform mat4 view;
uniform mat4 projection;

void main() {
    vPos = aPos; vUv = aUvSeedStrength; vTint = aTintKind;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
