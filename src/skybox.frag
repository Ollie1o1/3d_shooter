#version 330 core
in vec2 vNDC;
out vec4 FragColor;

// Procedural sky, set per arena (and blended down corridors) by GameplayState.
// Each pixel is turned back into a world-space view direction, so the sun,
// stars and mountains stay put as you look around.
// (No uniform initializers: GLSL ES, used by the web build, forbids them.)
uniform mat4  uInvProj;
uniform mat4  uInvView;
uniform vec3  uZenith;
uniform vec3  uHorizon;
uniform vec3  uGround;
uniform vec3  uSunDir;
uniform vec3  uSunColor;
uniform float uSunSize;     // angular radius, radians
uniform float uSunStripes;  // 1 = synthwave bands cut through the lower half
uniform vec3  uMountain;
uniform float uStars;
uniform float uEclipse;     // 1 = the sun is an eclipse with a corona, a blood horizon all round, a sea of cloud below
uniform float uHeat;        // 0..1: the eclipse burns hotter (the Zenith: as the boss's health falls)
uniform float uTime;

float hash3(vec3 p) { return fract(sin(dot(p, vec3(12.9898, 78.233, 37.719))) * 43758.5453); }
float hash2(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash2(i), hash2(i + vec2(1.0, 0.0)), f.x), mix(hash2(i + vec2(0.0, 1.0)), hash2(i + vec2(1.0, 1.0)), f.x), f.y);
}
float fbm(vec2 p) {
    float s = 0.0, a = 0.5;
    for (int k = 0; k < 5; ++k) { s += a * vnoise(p); p = p * 2.03 + vec2(1.7, 9.2); a *= 0.5; }
    return s;
}

// The eclipse: a black disc at uSunDir with a burning rim, a corona of
// streamers turning slowly round it, and blood all round the horizon
vec3 eclipseSky(vec3 dir, vec3 col) {
    float y = dir.y;
    vec3 blood = uSunColor;                        // the sunset's colour
    vec3 hot   = vec3(2.4, 1.5, 0.9);              // the rim
    float heat = uHeat;

    // the horizon: a band of blood all the way round, uneven, brightest at the line
    float az = atan(dir.z, dir.x);
    float swell = 0.75 + 0.25 * sin(az * 2.0 + 0.6) + 0.15 * sin(az * 5.0 + 2.2);
    float band = exp(-abs(y) * 9.0) * swell;
    col += blood * band * (0.75 + 0.5 * heat);
    col += vec3(1.8, 0.7, 0.25) * exp(-abs(y) * 40.0) * swell * 0.55;   // the burning line itself

    // above: thin cloud streaks low in the sky, dark, lit red from beneath
    if (y > 0.0 && y < 0.45) {
        vec2 p = dir.xz / (y + 0.08);
        float c = fbm(p * vec2(0.9, 2.6) + vec2(uTime * 0.01, 0.0));
        float m = smoothstep(0.46, 0.7, c) * smoothstep(0.45, 0.04, y) * smoothstep(0.0, 0.02, y);
        vec3 lit = mix(vec3(0.07, 0.02, 0.05), blood * 1.4, smoothstep(0.2, 0.0, y) * 0.8 + 0.2 * c);
        col = mix(col, lit, m * 0.85);
    }
    // below: a sea of cloud far beneath the ruin, glowing where it meets the horizon
    if (y < 0.0) {
        vec2 p = dir.xz / (-y + 0.02);
        float c = fbm(p * 0.35 + vec2(0.0, uTime * 0.008));
        float glow = exp(y * 9.0);                          // brightest toward the horizon
        vec3 sea = mix(vec3(0.05, 0.01, 0.03), blood * 0.9, glow * 0.85) * (0.55 + 0.75 * c);
        sea += vec3(1.2, 0.35, 0.1) * pow(c, 4.0) * glow * 0.8;   // the tops of the billows catch the light
        col = mix(col, sea, smoothstep(0.0, -0.03, y));
    }

    // the eclipse
    vec3 ax = uSunDir;
    vec3 t1 = normalize(cross(ax, abs(ax.y) < 0.9 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0)));
    vec3 t2 = cross(ax, t1);
    float c0  = dot(dir, ax);
    float ang = acos(clamp(c0, -1.0, 1.0));
    float r   = ang / uSunSize;                      // 1 at the disc's edge
    float phi = atan(dot(dir, t2), dot(dir, t1));

    // the corona: long streamers, slowly turning, reaching further as it burns
    float reach = 1.6 + 1.4 * heat;
    float s1 = vnoise(vec2(phi * 5.0 + uTime * 0.05, 0.0)), s2 = vnoise(vec2(phi * 13.0 - uTime * 0.09, 3.0));
    float streak = 0.35 + 0.9 * pow(s1, 2.0) + 0.5 * pow(s2, 3.0);
    float fall = exp(-max(r - 1.0, 0.0) * 2.4 / (streak * reach));
    float corona = fall * step(1.0, r);
    vec3 coronaCol = mix(blood * 1.6, vec3(1.9, 1.3, 0.9), exp(-max(r - 1.0, 0.0) * 4.0));
    col += coronaCol * corona * (0.9 + 0.8 * heat);
    col += blood * exp(-max(r - 1.0, 0.0) * 1.1) * (0.22 + 0.2 * heat) * step(1.0, r);   // the red bloom round it
    // the rim: a thin ring of fire, beaded where the light breaks through
    float rim = exp(-pow((r - 1.0) / 0.025, 2.0));
    float beads = 0.75 + 0.25 * sin(phi * 23.0 + uTime * 0.7) + 0.6 * pow(vnoise(vec2(phi * 9.0, uTime * 0.3)), 6.0);
    col += hot * rim * beads * (1.4 + heat);
    // the disc: black, a faint ember along its limb
    if (r < 1.0) col = mix(vec3(0.004, 0.0, 0.003), blood * 0.18, smoothstep(0.82, 1.0, r));
    return col;
}

void main() {
    vec4 v = uInvProj * vec4(vNDC, 1.0, 1.0);
    vec3 dir = normalize((uInvView * vec4(v.xyz / v.w, 0.0)).xyz);
    float y = dir.y;

    if (uEclipse > 0.5) {
        vec3 col = mix(uHorizon, uZenith, pow(clamp(y, 0.0, 1.0), 0.45));
        if (y < 0.0) col = mix(uHorizon * 0.5, uGround, clamp(-y * 8.0, 0.0, 1.0));
        vec3 cell = floor(dir * 220.0);
        float s = hash3(cell);
        col += vec3(step(0.9972, s) * uStars * smoothstep(0.25, 0.6, y) * (0.5 + 0.5 * hash3(cell + 7.0)));
        FragColor = vec4(eclipseSky(dir, col), 1.0);
        return;
    }

    // Gradient: horizon → zenith above, horizon → ground below
    vec3 col = mix(uHorizon, uZenith, pow(clamp(y, 0.0, 1.0), 0.5));
    if (y < 0.0) col = mix(uHorizon * 0.5, uGround, clamp(-y * 8.0, 0.0, 1.0));

    // Stars, fading out toward the horizon haze
    vec3 cell = floor(dir * 220.0);
    float s = hash3(cell);
    col += vec3(step(0.9972, s) * uStars * smoothstep(0.04, 0.35, y) * (0.5 + 0.5 * hash3(cell + 7.0)));

    // Sun / moon: a disc with a wide glow
    float c   = dot(dir, uSunDir);
    float ang = acos(clamp(c, -1.0, 1.0));
    float disc = 1.0 - smoothstep(uSunSize * 0.97, uSunSize, ang);
    if (uSunStripes > 0.5) {
        // Position within the disc along its local "up", -1 (bottom) .. 1 (top)
        vec3 up  = normalize(vec3(0.0, 1.0, 0.0) - uSunDir * uSunDir.y);
        float rel = dot(dir - uSunDir * c, up) / sin(uSunSize);
        if (rel < 0.15) {
            float band = fract(rel * 7.0);
            disc *= step(0.18 + (0.15 - rel) * 0.22, band);
        }
        // Vertical gradient across the disc: yellow top, hot pink bottom
        vec3 sunCol = mix(vec3(1.6, 0.25, 0.55), uSunColor, clamp(rel * 0.5 + 0.5, 0.0, 1.0));
        col = mix(col, sunCol, disc);
    } else {
        col = mix(col, uSunColor, disc);
    }
    col += uSunColor * exp(-ang * ang / (uSunSize * uSunSize * 30.0)) * 0.25;

    // Two ridges of mountains along the horizon (in front of the sun)
    float az = atan(dir.z, dir.x);
    float ridgeFar  = 0.085 + 0.035 * sin(az * 3.0 + 1.3) + 0.018 * sin(az * 7.0 + 0.4) + 0.008 * sin(az * 19.0);
    float ridgeNear = 0.055 + 0.025 * sin(az * 5.0 + 2.1) + 0.012 * sin(az * 11.0 + 1.7) + 0.005 * sin(az * 29.0);
    if (y < ridgeFar && y > -0.05)  col = mix(col, uMountain * 1.4 + uHorizon * 0.15, 0.92);
    if (y < ridgeNear && y > -0.05) col = mix(col, uMountain, 0.96);

    FragColor = vec4(col, 1.0);
}
