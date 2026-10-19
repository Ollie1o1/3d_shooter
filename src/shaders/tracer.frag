#version 330 core

in  float vAlpha;
out vec4  FragColor;

uniform vec3 uColor;

void main()
{
    // Additive blending in the caller — just output colour * fade
    FragColor = vec4(uColor, vAlpha);
}
