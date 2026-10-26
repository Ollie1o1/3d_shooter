#version 330 core
// Drawn with a multiply blend (dst * src): white leaves the floor as it is,
// darker darkens it. Darkest at the footprint's edge, gone `reach` away,
// and fading into the fog with the floor under it.
in vec3 vPos;
in vec4 vRect;
in vec2 vSR;

out vec4 FragColor;

uniform vec3  viewPos;
uniform float uFogDensity;

void main() {
    vec2 p  = vPos.xz;
    vec2 dd = max(max(vRect.xy - p, p - vRect.zw), vec2(0.0));
    float k = 1.0 - clamp(length(dd) / vSR.y, 0.0, 1.0);
    float a = vSR.x * k * k;
    float d = length(viewPos - vPos) * uFogDensity;
    a *= 1.0 - (1.0 - exp(-d * d)) * 0.85;
    FragColor = vec4(vec3(1.0 - a), 1.0);
}
