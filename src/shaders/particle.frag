#version 330 core

in  vec3  vColor;
in  float vAlpha;
out vec4  FragColor;

void main()
{
    // Round, soft-edged points instead of hard squares
    vec2  d = gl_PointCoord - vec2(0.5);
    float r = dot(d, d) * 4.0;          // 0 at centre, 1 at edge
    if (r > 1.0) discard;
    float falloff = 1.0 - r;
    // Additive blending in the caller; hot centre, slightly over-bright for bloom
    FragColor = vec4(vColor * (1.0 + falloff), vAlpha * falloff);
}
