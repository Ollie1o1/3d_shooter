#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 4) in mat4 instanceModel;
layout(location = 8) in vec3 instanceColor;
layout(location = 9) in vec3 instanceEmissive;
uniform mat4 view;
uniform mat4 projection;
out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out vec3 VertColor;
flat out vec3 objectColor;
flat out vec3 emissiveColor;
void main() {
    vec4 world = instanceModel * vec4(aPos, 1.0);
    FragPos = world.xyz;
    gl_Position = projection * view * world;
    Normal = transpose(inverse(mat3(instanceModel))) * aNormal;
    TexCoord = aTexCoord;
    VertColor = vec3(1.0);
    objectColor = instanceColor;
    emissiveColor = instanceEmissive;
}
