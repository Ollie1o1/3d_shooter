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

float hash3(vec3 p) { return fract(sin(dot(p, vec3(12.9898, 78.233, 37.719))) * 43758.5453); }

void main() {
    vec4 v = uInvProj * vec4(vNDC, 1.0, 1.0);
    vec3 dir = normalize((uInvView * vec4(v.xyz / v.w, 0.0)).xyz);
    float y = dir.y;

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
