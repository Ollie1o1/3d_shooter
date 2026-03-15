#version 330 core

// Per-vertex: world-space position + fade alpha
layout(location = 0) in vec3  aPos;
layout(location = 1) in float aAlpha;

uniform mat4 projection;
uniform mat4 view;

out float vAlpha;

void main()
{
    vAlpha      = aAlpha;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
