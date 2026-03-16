#version 330 core

// Per-vertex (mesh geometry — same cube for every enemy)
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
// location 3 (per-vertex color) unused — color comes from instance data

// Per-instance (divisor=1, one record per enemy, uploaded each frame)
layout(location = 4) in mat4 instanceModel;    // occupies locations 4, 5, 6, 7
layout(location = 8) in vec3 instanceColor;
layout(location = 9) in vec3 instanceEmissive;

uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out vec3 vColor;
out vec3 vEmissive;

void main()
{
    vec4 worldPos = instanceModel * vec4(aPos, 1.0);
    FragPos     = worldPos.xyz;
    gl_Position = projection * view * worldPos;
    TexCoord    = aTexCoord;
    // Normal matrix: handles non-uniform scale correctly.
    Normal      = mat3(transpose(inverse(instanceModel))) * aNormal;
    vColor      = instanceColor;
    vEmissive   = instanceEmissive;
}
