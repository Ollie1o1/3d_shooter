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
uniform vec3 emissiveColor;
uniform vec3 objectColor;

uniform vec3 pointLightPos[4];
uniform vec3 pointLightColor[4];

uniform vec3  uSkyAmb;       // hemisphere ambient, per arena
uniform vec3  uGroundAmb;
uniform vec3  uFogColor;
uniform float uFogDensity;   // 0 = no fog (view model, HUD-ish geometry)
uniform float uVertexGlow;   // > 0: per-vertex colour is also emitted (neon strips)
uniform vec3  uTexGlow;      // what the texture marks as glowing (alpha < 1) gives off this
uniform float uNoSeams;      // 1: no dark seam every 4 m (the newer textures carry their own joints)

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

void main()
{
    // Surface colour = texture * per-vertex tint * per-object tint
    vec4 texColor = texture(uTexture, TexCoord) * vec4(VertColor * objectColor, 1.0);

    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // Hemisphere ambient: sky colour from above, bounce colour from below
    vec3 ambient = mix(uGroundAmb, uSkyAmb, norm.y * 0.5 + 0.5);

    // Directional light — Blinn-Phong
    vec3  L    = normalize(-lightDir);
    float diff = max(dot(norm, L), 0.0) * 0.85 + 0.08;
    vec3  H    = normalize(L + viewDir);
    float spec = pow(max(dot(norm, H), 0.0), 48.0);
    vec3  specular = lightColor * spec * 0.18;

    // Point lights
    vec3 pointContrib = vec3(0.0);
    for (int i = 0; i < 4; ++i) {
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
    float rim    = pow(1.0 - max(dot(norm, viewDir), 0.0), 4.0);
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

    vec3 lighting = ambient + diff * lightColor + pointContrib + neonLight(FragPos, norm);
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
}
