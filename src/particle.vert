#version 330 core

// Per-particle: world position, RGB colour, fade alpha
layout(location = 0) in vec3  aPos;
layout(location = 1) in vec3  aColor;
layout(location = 2) in float aAlpha;

uniform mat4  projection;
uniform mat4  view;
uniform float uPointScale;  // world-size factor: pixels at 1 m distance

out vec3  vColor;
out float vAlpha;

void main()
{
    vec4 viewPos = view * vec4(aPos, 1.0);
    gl_Position  = projection * viewPos;
    // Perspective-correct size so near sparks read big and far ones shrink
    gl_PointSize = clamp(uPointScale / max(-viewPos.z, 0.1), 2.0, 24.0);
    vColor = aColor;
    vAlpha = aAlpha;
}
