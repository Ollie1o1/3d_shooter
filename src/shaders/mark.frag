#version 330 core
// Drawn with a multiply blend (dst * src), as the contact shadows are: white
// leaves the surface as it is, the mark's tint darkens and colours it. Each
// kind has its own shape, ragged by its seed; all fade into the fog.
in vec3 vPos;
in vec4 vUv;
in vec4 vTint;

out vec4 FragColor;

uniform vec3  viewPos;
uniform float uFogDensity;

void main() {
    vec2  p    = vUv.xy;
    float r    = length(p);
    float ang  = atan(p.y, p.x);
    float seed = vUv.z * 6.2831853;
    float rag  = 0.5 * sin(ang * 3.0 + seed) + 0.3 * sin(ang * 7.0 + seed * 2.7) + 0.2 * sin(ang * 13.0 + seed * 4.6);
    float a;
    if (vTint.a < 0.5) {            // a pit: a dark hole, a ring of soot thinning out round it
        float hole = 0.42 + 0.08 * rag;
        a = (1.0 - smoothstep(hole * 0.6, hole, r)) + 0.4 * (1.0 - smoothstep(hole, 0.95, r));
    } else if (vTint.a < 1.5) {     // a scorch: soot, darkest in the middle, thrown out in rays to a ragged edge
        float edge = 0.76 + 0.2 * rag;
        a = 1.0 - smoothstep(edge * 0.2, edge, r);
        a *= 0.8 + 0.2 * sin(ang * 23.0 + seed * 9.0) * smoothstep(0.25, 0.8, r);
    } else {                        // a stain: a pool with an edge
        float edge = 0.8 + 0.17 * rag;
        a = 1.0 - smoothstep(edge * 0.78, edge, r);
    }
    a = clamp(a, 0.0, 1.0) * vUv.w;
    float d = length(viewPos - vPos) * uFogDensity;
    a *= 1.0 - (1.0 - exp(-d * d)) * 0.85;
    FragColor = vec4(mix(vec3(1.0), vTint.rgb, a), 1.0);
}
