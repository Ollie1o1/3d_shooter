#version 330 core

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec3 VertColor;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
#ifdef VIEWMODEL_INSTANCED
flat in vec3 emissiveColor;
flat in vec3 objectColor;
#else
uniform vec3 emissiveColor;
uniform vec3 objectColor;
#endif

uniform vec3 pointLightPos[4];
uniform vec3 pointLightColor[4];

uniform vec3  uSkyAmb;       // hemisphere ambient, per arena
uniform vec3  uGroundAmb;
uniform vec3  uFogColor;
uniform float uFogDensity;   // 0 = no fog (view model, HUD-ish geometry)
uniform float uVertexGlow;   // > 0: per-vertex colour is also emitted (neon strips)
uniform vec3  uTexGlow;      // what the texture marks as glowing (alpha < 1) gives off this
uniform float uNoSeams;      // 1: no dark seam every 4 m (the newer textures carry their own joints)
uniform float uMacro;        // > 0: soft variation over metres of world (the 4 m tile stops reading as a repeat)
uniform sampler2D uNormalMap; // the texture's relief (MaterialTex::normalMap), on unit 1
uniform float uBump;         // > 0: light by it (how strongly); 0 the flat face

// The surface's own axes from how its position and texture coordinates change across the screen
// (no tangents stored: the world's UVs are projected per face, props' too), so the relief map can tilt
// the normal the way the texture runs
mat3 cotangentFrame(vec3 N, vec3 p, vec2 uv) {
    vec3 dp1 = dFdx(p), dp2 = dFdy(p);
    vec2 duv1 = dFdx(uv), duv2 = dFdy(uv);
    vec3 dp2perp = cross(dp2, N), dp1perp = cross(N, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
    float m = max(dot(T, T), dot(B, B));
    if (m < 1e-20) return mat3(vec3(0.0), vec3(0.0), N);
    float inv = inversesqrt(m);
    return mat3(T * inv, B * inv, N);
}

// Value noise from the world position (no texture, no sin: steady far from the origin)
float hash12(vec2 p) {
    vec3 q = fract(vec3(p.xyx) * 0.1031);
    q += dot(q, q.yzx + 33.33);
    return fract((q.x + q.y) * q.z);
}
float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash12(i), hash12(i + vec2(1.0, 0.0)), f.x),
               mix(hash12(i + vec2(0.0, 1.0)), hash12(i + vec2(1.0, 1.0)), f.x), f.y);
}
// Brightness and a faint warm/cool shift over the world: floors vary across X and Z, walls along
// themselves and up them. 1 on average
vec3 macroTint(vec3 P) {
    vec2 p = P.xz + P.y * vec2(0.37, 0.61);
    float a = vnoise(p / 9.0) * 0.65 + vnoise(p / 3.1 + 17.0) * 0.35;
    float h = vnoise(p / 14.0 + 41.0);
    float k = 1.0 + (a - 0.5) * 0.34 * uMacro;
    return k * vec3(1.0 + (h - 0.5) * 0.08 * uMacro, 1.0, 1.0 - (h - 0.5) * 0.08 * uMacro);
}

// Light from the neon and lava near this point (NeonLights.h picks them):
// each is a box of light; distance to its nearest point, a soft wrap so the
// surface a strip is mounted on glows around it, nothing past 7 m
const int MAX_NEON = 8;
uniform int  uNeonCount;
uniform vec3 uNeonMin[MAX_NEON];
uniform vec3 uNeonMax[MAX_NEON];
uniform vec3 uNeonColor[MAX_NEON];

// The level's baked light (LightBake.h): per 1 m cell, R how open it is (ambient occlusion: 1 as
// open as flat ground) and G whether its arena's sun reaches it. Regions packed in one 3D texture;
// outside them, nothing changes. Only the static world turns it on
const int MAX_BAKE = 16;
uniform float     uBakeOn;
uniform float     uBakeDebug;            // 1: draw what's baked (R open, G sun, B how much the sun counts)
uniform sampler3D uBake;
uniform int       uBakeCount;
uniform vec3      uBakeAtlas;            // the texture's size, cells
uniform vec3      uBakeMin[MAX_BAKE];    // a region's corner (world)
uniform vec3      uBakeDims[MAX_BAKE];   // its size, cells
uniform vec3      uBakeOff[MAX_BAKE];    // where it sits in the texture, cells
uniform vec3      uBakeSun[MAX_BAKE];    // the sun it was baked for (the way its light travels)
// ...and at half the resolution, the light from everything that glows (RGB, stored as sqrt(light / 4)) and
// how much sunlit surface is round a point (A: the sun's bounce, coloured by its region's ground)
uniform float     uGlowOn;
uniform sampler3D uGlowBake;
uniform vec3      uGlowAtlas;            // its size, glow cells (2 cells each)
uniform vec3      uBakeAlbedo[MAX_BAKE]; // a region's ground colour
vec4  gGlow   = vec4(0.0);               // baked(): the glow light (rgb) and the bounce (a) where it looked
vec3  gAlbedo = vec3(0.0);

// x: openness, y: sun (1 = lit), z: how much the baked sun counts (0 outside every region, or the
// light has turned from the one it was baked for)
vec3 baked(vec3 P, vec3 N) {
    if (uBakeOn < 0.5) return vec3(1.0, 1.0, 0.0);
    vec3 q = P + N * 0.55;   // off the surface: a cell's centre is in the open, not in the wall
    for (int i = 0; i < MAX_BAKE; ++i) {
        if (i >= uBakeCount) break;
        vec3 c = (q - uBakeMin[i]);   // (cells are 1 m)
        if (any(lessThan(c, vec3(0.0))) || any(greaterThanEqual(c, uBakeDims[i]))) continue;
        c = clamp(c, vec3(0.5), uBakeDims[i] - 0.5);   // (never filter into the next region)
        vec2 v = texture(uBake, (uBakeOff[i] + c) / uBakeAtlas).rg;
        if (uGlowOn > 0.5) {
            vec3 cg = clamp(c, vec3(1.0), uBakeDims[i] - 1.0);   // (half a glow cell in: never into the gap)
            vec4 g = texture(uGlowBake, (uBakeOff[i] + cg) / 2.0 / uGlowAtlas);
            gGlow = vec4(g.rgb * g.rgb * 4.0, g.a);
            gAlbedo = uBakeAlbedo[i];
        }
        float turned = smoothstep(0.92, 0.99, dot(normalize(-lightDir), normalize(-uBakeSun[i])));
        // at a region's edge it fades to nothing, so walking out of one isn't a line on the floor
        vec3 e = min(c, uBakeDims[i] - c);
        float edge = smoothstep(0.5, 3.0, min(e.x, min(e.y, e.z)));
        gGlow *= edge;
        gGlow.a *= turned;   // (the sun it bounced has gone round: so has its bounce)
        return vec3(mix(1.0, v.r, edge), v.g, turned * edge);
    }
    return vec3(1.0, 1.0, 0.0);
}

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

void main()
{
    // Surface colour = texture * per-vertex tint * per-object tint
    vec4 texColor = texture(uTexture, TexCoord) * vec4(VertColor * objectColor, 1.0);
    if (uMacro > 0.0) texColor.rgb *= macroTint(FragPos);

    vec3 face    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 norm    = face;
    if (uBump > 0.0) {   // the texture's relief: grout, seams and cracks catch the light and shade
        vec3 tn = texture(uNormalMap, TexCoord).xyz * 2.0 - 1.0;
        tn.xy *= uBump;
        float lean = length(tn.xy);   // (a sharp seam leans no further than about 40 degrees)
        if (lean > 0.85 * tn.z) tn.xy *= 0.85 * tn.z / lean;
        vec3 n = cotangentFrame(face, FragPos, TexCoord) * tn;
        if (dot(n, n) > 1e-6) norm = normalize(n);
    }

    // Hemisphere ambient: sky colour from above, bounce colour from below; dimmed where the bake
    // says little of the sky gets in
    vec3 bk = baked(FragPos, face);
    float ao = clamp((bk.x - 0.3) / 0.68, 0.0, 1.0);
    ao = ao * ao * (3.0 - 2.0 * ao);   // (eased: open floor stays as it was, corners and undersides go deep)
    // (in a cast shadow, part of the sky is behind the caster too: the ambient dims a little with it)
    // (the relief shades the sky's light only a little: a seam on a ceiling doesn't open to the sky)
    vec3 ambN = normalize(mix(face, norm, 0.35));
    vec3 ambient = mix(uGroundAmb, uSkyAmb, ambN.y * 0.5 + 0.5) * mix(0.25, 1.0, ao) * mix(1.0, mix(0.62, 1.0, bk.y), bk.z);

    // Directional light — Blinn-Phong, out of the baked sun's shadow
    float sunVis = mix(1.0, mix(0.22, 1.0, bk.y), bk.z);
    vec3  L    = normalize(-lightDir);
    // (a face turned from the sun stays out of it, whatever its relief leans toward)
    float faceLit = uBump > 0.0 ? clamp(dot(face, L) * 4.0 + 0.25, 0.0, 1.0) : 1.0;
    float diff = (max(dot(norm, L), 0.0) * 0.85 * sunVis * faceLit + 0.08) * mix(0.6, 1.0, ao);
    vec3  H    = normalize(L + viewDir);
    float spec = pow(max(dot(norm, H), 0.0), 48.0) * faceLit;
    vec3  specular = lightColor * spec * 0.18 * sunVis;

    // Point lights
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

    // Rim light
    float rim    = pow(1.0 - max(dot(face, viewDir), 0.0), 4.0);
    vec3 rimColor = uSkyAmb * rim * 0.5;

    // UV-based edge lines — darkens pixels near the boundary of each polygon face.
    // Makes box edges visually readable without extra geometry.
    // World UVs run 1 per 4 m, so fract() puts a thin seam at every tile edge.
    // (Without it, every UV outside 0..1 read as "on an edge" and the whole
    // level outside one 4 m tile rendered at 15% brightness.)
    vec2  tuv = fract(TexCoord);
    float eu = min(tuv.x, 1.0 - tuv.x);
    float ev = min(tuv.y, 1.0 - tuv.y);
    float edgeFactor = smoothstep(0.0, 0.02, min(eu, ev));
    // edgeFactor = 0 at edges (dark), 1 at face centre (full colour)

    // The baked glow light (lava, neon, lamps: soft, reaching across rooms, blocked by walls) and the
    // sun's bounce off the ground round it. The live neon light stays for the close, sharp part of it
    vec3 lit = normalize(mix(face, norm, 0.5));
    vec3 glowLight = gGlow.rgb * 0.7 + lightColor * gAlbedo * gGlow.a * 0.55 * max(0.0, 0.6 + 0.4 * lit.y);
    float liveNeon = uGlowOn > 0.5 ? 0.6 : 1.0;
    vec3 lighting = ambient + diff * lightColor + pointContrib + neonLight(FragPos, lit) * liveNeon + glowLight * mix(0.45, 1.0, ao);
    vec3 result   = (lighting * texColor.rgb + specular + rimColor + emissiveColor)
                  * mix(0.6, 1.0, max(edgeFactor, uNoSeams));   // darken at tile seams
    float glowMask = 1.0 - texture(uTexture, TexCoord).a;
    result += uTexGlow * glowMask;
    result += VertColor * uVertexGlow;

    // Exponential-squared distance fog toward the arena's horizon colour
    float d   = length(viewPos - FragPos) * uFogDensity;
    float fog = 1.0 - exp(-d * d);
    result = mix(result, uFogColor, fog * 0.85);

    FragColor = vec4(result, dot(uTexGlow, uTexGlow) > 0.0 ? 1.0 : texColor.a);
    if (uBakeDebug > 0.5) FragColor = vec4(bk, 1.0);
}
