#version 330 core

// Per-vertex (mesh geometry — the same unit cube for every instance)
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
// location 3 (per-vertex color) unused — color comes from instance data

// Per-instance (divisor=1, one record per box, uploaded each frame)
layout(location = 4) in mat4 instanceModel;    // occupies locations 4, 5, 6, 7
layout(location = 8) in vec3 instanceColor;
layout(location = 9) in vec3 instanceEmissive;
layout(location = 10) in float instanceSurface;   // a boss's surface (0: none)

uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out vec3 vColor;
out vec3 vEmissive;
flat out float vSurf;
out vec3 vLocal;    // the point on the part, in metres of the part (its own axes)
out vec3 vLocalN;
flat out vec3 vSize;

void main()
{
    vec4 worldPos = instanceModel * vec4(aPos, 1.0);
    FragPos     = worldPos.xyz;
    gl_Position = projection * view * worldPos;
    TexCoord    = aTexCoord;
    // Normal matrix: handles non-uniform scale correctly.
    Normal      = transpose(inverse(mat3(instanceModel))) * aNormal;
    vColor      = instanceColor;
    vEmissive   = instanceEmissive;
    vSurf       = instanceSurface;
    vSize       = vec3(length(instanceModel[0].xyz), length(instanceModel[1].xyz), length(instanceModel[2].xyz));
    vLocal      = aPos * vSize;
    vLocalN     = aNormal;
}
