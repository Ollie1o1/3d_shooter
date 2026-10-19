#version 330 core

in  vec3 vWorld;
out vec4 FragColor;

uniform float uTime;
uniform vec3  uViewPos;
uniform vec3  uDeep;        // the water's own colour, seen from above
uniform vec3  uGlow;        // the red light coming up from beneath
uniform vec3  uFogColor;
uniform float uFogDensity;
uniform vec2  uShaftXZ;     // where the eclipse light falls on it
uniform float uShaftR;

float ripple(vec2 p)
{
    return sin(p.x * 0.9 + uTime * 1.3) * 0.5
         + sin(p.y * 1.3 - uTime * 1.1) * 0.5
         + sin((p.x + p.y) * 2.1 + uTime * 2.2) * 0.25;
}

void main()
{
    float r    = ripple(vWorld.xz);
    vec3  col  = mix(uDeep, uGlow, 0.18 + 0.1 * r);
    float lit  = 1.0 - smoothstep(uShaftR * 0.5, uShaftR, length(vWorld.xz - uShaftXZ));
    col += vec3(0.9, 0.85, 0.75) * lit * (0.55 + 0.15 * r);
    vec3  v    = normalize(uViewPos - vWorld);
    float fres = pow(1.0 - abs(v.y), 3.0);
    col += uGlow * fres * 0.25 + vec3(0.1, 0.25, 0.25) * max(r, 0.0) * 0.12;
    float d    = length(uViewPos - vWorld);
    col = mix(col, uFogColor, 1.0 - exp(-uFogDensity * d));
    FragColor = vec4(col, mix(0.62, 0.9, fres));
}
