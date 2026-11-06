#version 330 core

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec3 vColor;
in vec3 vEmissive;
flat in float vSurf;
in vec3 vLocal;
in vec3 vLocalN;
flat in vec3 vSize;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform vec3 uSkyAmb;      // hemisphere ambient, per arena
uniform vec3 uGroundAmb;
uniform vec3 uFogColor;
uniform int uSurfaces;     // the bosses' surfaces drawn (QUALITY above its lowest)
uniform float uFogDensity;
uniform vec3 pointLightPos[4];
uniform vec3 pointLightColor[4];

// Light from the neon and lava near this point (NeonLights.h picks them):
// each is a box of light; distance to its nearest point, a soft wrap so the
// surface a strip is mounted on glows around it, nothing past 7 m
const int MAX_NEON = 8;
uniform int  uNeonCount;
uniform vec3 uNeonMin[MAX_NEON];
uniform vec3 uNeonMax[MAX_NEON];
uniform vec3 uNeonColor[MAX_NEON];

vec3 neonLight(vec3 P, vec3 N) {
    vec3 sum = vec3(0.0);
    if (uNeonCount == 0) return sum;
    for (int i = 0; i < MAX_NEON; ++i) {   // a fixed count, so it unrolls: unused slots sit far away
        vec3 d = clamp(P, uNeonMin[i], uNeonMax[i]) - P;
        float d2 = dot(d, d);
        if (d2 > 49.0) continue;
        float dist = sqrt(d2);
        float ndl  = dist > 0.02 ? dot(N, d / dist) : 0.0;
        float wrap = clamp(ndl * 0.5 + 0.5, 0.0, 1.0);
        float fall = (1.0 - smoothstep(3.5, 7.0, dist)) / (1.0 + d2 * 0.8);
        sum += uNeonColor[i] * (wrap * fall);
    }
    // Soft ceiling: right up against a big panel (a jump pad) it glows, it doesn't blow out
    return sum / (1.0 + 0.35 * max(max(sum.r, sum.g), sum.b));
}

// ---- a boss's surfaces (BoxInstance::surface): patterns made here, in the part's own metres ----
float hash2(vec2 p) { vec3 q = fract(vec3(p.xyx) * 0.1031); q += dot(q, q.yzx + 33.33); return fract((q.x + q.y) * q.z); }
float vnoise2(vec2 x) {
    vec2 i = floor(x), f = fract(x); f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash2(i), hash2(i + vec2(1, 0)), f.x), mix(hash2(i + vec2(0, 1)), hash2(i + vec2(1, 1)), f.x), f.y);
}
float fbm2(vec2 p) { return 0.57 * vnoise2(p) + 0.29 * vnoise2(p * 2.03 + vec2(1.7, 9.2)) + 0.14 * vnoise2(p * 4.1 + vec2(5.3, 2.8)); }
vec2 faceUV(vec3 p, vec3 n) { vec3 a = abs(n); return a.x > a.y && a.x > a.z ? p.zy : a.y > a.z ? p.xz : p.xy; }
// How near a crack: the noise's midline, wandering (0 on a crack)
float crackDist(vec2 p) { return abs(vnoise2(p) - 0.5) + 0.25 * abs(vnoise2(p * 2.7 + vec2(4.1, 1.3)) - 0.5); }
// One noise per pixel (n), shared by everything a surface does; the fine work only near enough to see it
vec3 bossSurface(int s, vec3 base, out vec3 emi, float dist) {
    emi = vec3(0.0);
    vec3 p = vLocal;
    float big = max(vSize.x, max(vSize.y, vSize.z));
    vec2 uv = faceUV(p, vLocalN) + vec2(dot(sign(vLocalN), vec3(31.0, 57.0, 13.0)));   // (each face its own pattern)
    if (dist > 900.0) return base * (0.8 + 0.4 * vnoise2(uv * 0.04));     // (far off: tone only)
    float detail = 1.0 - smoothstep(90.0, 700.0, dist);
    float n = fbm2(uv * 0.25);
    if (s == 1) {   // riveted plate, rusting: panels, their seams, rivets along them, rust blooms, runoff streaks
        float panel = clamp(big / 5.0, 1.4, 18.0);
        vec2 g = abs(fract(uv / panel) - 0.5) * panel;
        float edge = panel * 0.5 - max(g.x, g.y);
        float seam = 1.0 - smoothstep(0.015 * panel, 0.045 * panel, edge);
        vec2 rv = fract(uv / (panel * 0.125)) - 0.5;
        float riv = (1.0 - smoothstep(0.16, 0.26, length(rv))) * (1.0 - smoothstep(0.05 * panel, 0.1 * panel, edge)) * (1.0 - seam);
        float streak = detail > 0.0 ? vnoise2(vec2((p.x + p.z) * 1.4, p.y * 0.07)) : 0.5;
        float rust = smoothstep(0.45, 0.72, n);
        vec3 c = base * (0.75 + 0.5 * n);
        c = mix(c, vec3(0.42, 0.16, 0.06) * (0.7 + 0.5 * fract(n * 7.0)), rust * 0.7);
        c *= 1.0 - 0.35 * smoothstep(0.55, 0.85, streak);
        c *= 1.0 - 0.6 * seam * detail;
        c += base * 1.1 * riv * detail;
        return c;
    }
    if (s == 2) {   // pitted cast iron, its casting seams, a cooler cast to break the red
        float pits = detail > 0.0 ? smoothstep(0.64, 0.78, vnoise2(uv * 2.6)) * detail : 0.0;
        vec3 c = base * (0.7 + 0.6 * n) * (1.0 - 0.45 * pits);
        float band = 1.0 - smoothstep(0.0, 0.05, abs(fract(p.y / clamp(big / 4.0, 2.0, 16.0)) - 0.5) - 0.44);
        c *= 1.0 - 0.3 * band * detail;
        return mix(c, c * vec3(0.85, 0.97, 1.2), 0.4 * smoothstep(0.3, 0.7, n));
    }
    if (s == 3) {   // bone: grain, yellowing, a web of fine cracks
        float grain = 0.5 + 0.5 * sin(dot(p, vec3(0.3, 2.2, 0.4)) * 3.0 + n * 6.0);
        float cr = detail > 0.0 ? 1.0 - smoothstep(0.0, 0.035, crackDist(uv * 0.3)) : 0.0;
        vec3 c = base * (0.82 + 0.3 * grain * detail);
        c = mix(c, c * vec3(1.12, 0.95, 0.7), smoothstep(0.5, 0.8, n));
        return c * (1.0 - 0.6 * cr * detail);
    }
    if (s == 4) {   // char: soot, ash, a few embers still alight
        vec3 c = base * (0.55 + 0.9 * n);
        c = mix(c, vec3(0.16, 0.15, 0.14), 0.35 * smoothstep(0.6, 0.8, fract(n * 5.0)));
        if (detail > 0.0) emi = vec3(1.6, 0.4, 0.1) * step(0.93, vnoise2(uv * 2.2)) * detail * 0.6;
        return c;
    }
    if (s == 5) {   // heat-cracked iron: the furnace's light in its cracks
        float crack = 1.0 - smoothstep(0.0, 0.04, crackDist(uv * clamp(0.6 / max(big * 0.04, 0.3), 0.06, 1.2)));
        emi = vec3(2.2, 0.55, 0.12) * crack * (0.5 + 0.5 * n) * 0.9;
        return base * (0.7 + 0.6 * n) * (1.0 - 0.5 * crack);
    }
    if (s == 7) {   // Yard: staggered brick on the walls, worn square pavers underfoot
        vec2 tile = abs(vLocalN.y) > 0.5 ? vec2(1.5) : vec2(1.2, 0.48);
        vec2 grid = uv / tile;
        if (abs(vLocalN.y) < 0.5) grid.x += mod(floor(grid.y), 2.0) * 0.5;
        vec2 f = fract(grid);
        vec2 edge = min(f, 1.0 - f) * tile;
        float mortar = 1.0 - smoothstep(0.018, 0.018 + max(fwidth(uv.x), fwidth(uv.y)) + 0.015, min(edge.x, edge.y));
        float variation = hash2(floor(grid));
        return mix(base * (0.77 + 0.32 * variation + 0.12 * n), base * 0.32, mortar * detail);
    }
    if (s == 8) {   // Spire: pale dressed stone with subtle mineral veins
        float vein = 1.0 - smoothstep(0.025, 0.09, abs(sin(uv.x * 0.75 + uv.y * 0.34 + n * 8.0)));
        vec2 seam = abs(fract(uv / 2.5) - 0.5);
        float joint = smoothstep(0.485, 0.5, max(seam.x, seam.y));
        return base * (0.9 + 0.18 * n) * (1.0 - 0.16 * vein * detail - 0.32 * joint * detail);
    }
    if (s == 9) {   // Core: recessed circuit panels, sparse cyan traces
        vec2 cell = fract(uv / 2.4);
        vec2 edge = min(cell, 1.0 - cell);
        float seam = 1.0 - smoothstep(0.012, 0.035, min(edge.x, edge.y));
        float trace = (1.0 - smoothstep(0.008, 0.018, abs(cell.x - 0.2))) * step(0.2, cell.y) * step(cell.y, 0.8);
        trace += (1.0 - smoothstep(0.008, 0.018, abs(cell.y - 0.8))) * step(0.2, cell.x) * step(cell.x, 0.65);
        emi = vec3(0.02, 0.24, 0.32) * trace * detail;
        return base * (0.8 + 0.25 * n) * (1.0 - 0.55 * seam * detail);
    }
    if (s == 10) {   // debris: coarse fractured basalt, kept quiet against the landing edges
        float crack = 1.0 - smoothstep(0.025, 0.075, crackDist(uv * 0.65));
        return base * (0.65 + 0.55 * n) * (1.0 - 0.35 * crack * detail);
    }
    return base;
}

void main()
{
    vec3 baseColor = vColor, surfEmi = vec3(0.0);
    // 6: open ground seen from far off (Chapter 3's arrival): ash grain, no rim light or seams,
    // which a plain seen at a glancing angle would otherwise turn white with
    bool ground = vSurf > 5.5 && vSurf < 6.5;
    if (ground) baseColor *= 0.82 + 0.18 * sin(FragPos.x * 0.07 + sin(FragPos.z * 0.05) * 3.0) * sin(FragPos.z * 0.09 + 1.3)
                           + 0.12 * fract(sin(dot(floor(FragPos.xz * 0.7), vec2(12.9898, 78.233))) * 43758.5453);
    else if (vSurf > 0.5 && uSurfaces != 0) baseColor = bossSurface(int(vSurf + 0.5), vColor, surfEmi, length(viewPos - FragPos));
    vec4 texColor = texture(uTexture, TexCoord) * vec4(baseColor, 1.0);

    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 ambient = mix(uGroundAmb, uSkyAmb, norm.y * 0.5 + 0.5);

    // Directional light — Blinn-Phong
    vec3  L    = normalize(-lightDir);
    float diff = max(dot(norm, L), 0.0) * 0.85 + 0.08;
    vec3  H    = normalize(L + viewDir);
    float spec = pow(max(dot(norm, H), 0.0), 48.0);
    vec3  specular = lightColor * spec * 0.18;

    vec3 pointContrib = vec3(0.0);
    for (int i = 0; i < 4; ++i) {
        if (all(equal(pointLightColor[i], vec3(0.0)))) continue;
        vec3  toLight = pointLightPos[i] - FragPos;
        float dist    = length(toLight);
        float atten   = 1.0 / (1.0 + 0.25*dist + 0.07*dist*dist);
        vec3  lDir    = normalize(toLight);
        float pDiff   = max(dot(norm, lDir), 0.0);
        vec3  pH      = normalize(lDir + viewDir);
        float pSpec   = pow(max(dot(norm, pH), 0.0), 24.0) * 0.35;
        pointContrib += pointLightColor[i] * (pDiff + pSpec) * atten;
    }

    // Rim light in the sky colour keeps silhouettes readable against the floor
    float rim     = pow(1.0 - max(dot(norm, viewDir), 0.0), 3.0);
    vec3 rimColor = ground ? vec3(0.0) : (uSkyAmb * 1.4 + vec3(0.06)) * rim;

    // Dark seams at face edges make every box in a rig read as a separate part
    float eu = min(TexCoord.x, 1.0 - TexCoord.x);
    float ev = min(TexCoord.y, 1.0 - TexCoord.y);
    float edgeFactor = ground ? 1.0 : smoothstep(0.0, 0.06, min(eu, ev));

    vec3 lighting = ambient + diff * lightColor + pointContrib + neonLight(FragPos, norm);
    vec3 result   = (lighting * texColor.rgb + specular + rimColor) * mix(0.35, 1.0, edgeFactor)
                  + vEmissive + surfEmi;

    float d   = length(viewPos - FragPos) * uFogDensity;
    float fog = 1.0 - exp(-d * d);
    result = mix(result, uFogColor, fog * 0.85);

    FragColor = vec4(result, texColor.a);
}
