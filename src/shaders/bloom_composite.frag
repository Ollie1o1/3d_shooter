#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D scene;
uniform sampler2D bloomBlur;

// Screen effects (PostProcess::ScreenFx, set by the game every frame; all 0 = off)
uniform float uSpeed;    // 0..1 a dash / high speed: zoom blur toward the centre, speed lines, edge fringe
uniform float uLow;      // 0..1 low health: the colour drains, red closes in from the edges
uniform float uPulse;    // 0..1 the heartbeat's throb (on its beats)
uniform float uHurt;     // 0..1 just hit: a jolt of colour fringe
uniform float uTime;     // seconds (the speed lines' flicker)
uniform float uDrive;    // 0..1 OVERDRIVE: richer colour, a gold heat breathing in from the edges
uniform float uKick;     // 0..1 a kill: a quick pop of light and fringe
uniform float uAspect;   // width / height
uniform float uWake;     // 0..1 STORY: UNIT 24's optics booting (1 black; then grey, soft and tearing; 0 clear)

// ACES filmic curve (Narkowicz 2015 fit). Keeps darks dark and colours
// saturated, rolling highlights off smoothly; scene colours are authored as
// display values, so no separate gamma step.
vec3 aces(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

vec3 hdrAt(vec2 uv) {
    return texture(scene, uv).rgb + texture(bloomBlur, uv).rgb * 0.7;
}

float hash1(float n) { return fract(sin(n * 127.1) * 43758.5453); }

void main() {
    vec2 uv = TexCoords;
    vec2 c = uv - 0.5;
    vec2 ca = vec2(c.x * uAspect, c.y);          // round on screen, not stretched
    float r = length(ca) / (0.5 * uAspect);      // 0 centre .. ~1 at the side edges

    if (uWake > 0.001) {   // the optics booting: rows of the image torn sideways, a beat at a time
        float row = floor(uv.y * 90.0), beat = floor(uTime * 18.0);
        float tear = step(1.0 - 0.35 * uWake, hash1(row * 1.7 + beat * 3.1));
        uv.x += (hash1(row + beat * 7.0) - 0.5) * 0.08 * uWake * tear;
    }
    vec3 hdr;
    if (uWake > 0.05) {    // and out of focus
        float rr = 0.012 * smoothstep(0.1, 0.8, uWake);
        hdr = hdrAt(uv);
        for (int i = 0; i < 8; ++i) {
            float a = float(i) * 0.7853982;
            hdr += hdrAt(uv + vec2(cos(a), sin(a) * uAspect) * rr);
        }
        hdr /= 9.0;
    } else if (uSpeed > 0.01) {
        // Zoom blur: taps toward the centre, longer further out (the centre stays sharp)
        float len = 0.07 * uSpeed * smoothstep(0.1, 0.9, r);
        hdr = hdrAt(uv);
        for (int i = 1; i <= 6; ++i) hdr += hdrAt(uv - c * len * (float(i) / 6.0));
        hdr /= 7.0;
    } else {
        hdr = hdrAt(uv);
    }

    // Colour fringe at the edges: red out, blue in (a hit, a dash, a heartbeat)
    float fringe = 0.006 * uHurt + 0.004 * uSpeed + 0.003 * uPulse * uLow + 0.004 * uKick;
    if (fringe > 0.0002) {
        vec2 off = c * fringe * (0.4 + r);
        hdr.r = mix(hdr.r, hdrAt(uv + off).r, 0.85);
        hdr.b = mix(hdr.b, hdrAt(uv - off).b, 0.85);
    }

    vec3 col = aces(hdr * (1.35 + 0.12 * uKick));

    if (uSpeed > 0.01) {   // speed lines: thin streaks radiating out, flickering, only past mid-screen
        float ang = atan(ca.y, ca.x) / 6.2831853 + 0.5;
        float slice = floor(ang * 220.0);
        float h = hash1(slice + floor(uTime * 24.0) * 13.0);
        float line = step(0.9, h) * smoothstep(0.45, 0.95, r);
        col = mix(col, vec3(0.92, 0.97, 1.0), line * 0.22 * uSpeed);
    }

    if (uDrive > 0.001) {
        float g = dot(col, vec3(0.299, 0.587, 0.114));
        col = max(mix(vec3(g), col, 1.0 + 0.3 * uDrive), 0.0);              // more colour
        col *= mix(vec3(1.0), vec3(1.05, 0.99, 0.92), uDrive);               // warmer
        float heat = smoothstep(0.62, 1.25, r) * (0.75 + 0.25 * sin(uTime * 8.0));
        col += vec3(1.0, 0.5, 0.1) * heat * 0.3 * uDrive;                    // gold at the edges, breathing
    }

    if (uLow > 0.001) {
        // The colour drains (a warm grey, not a cold one) as health runs out
        float g = dot(col, vec3(0.299, 0.587, 0.114));
        col = mix(col, vec3(g) * vec3(1.08, 0.94, 0.92), uLow * 0.6);
        // Red closes in from the edges, and throbs inward on each heartbeat
        float edge = smoothstep(0.55 - 0.25 * uLow - 0.12 * uPulse, 1.15, r);
        vec3 blood = col * vec3(0.45, 0.06, 0.05) + vec3(0.16, 0.0, 0.0);
        col = mix(col, blood, edge * clamp(uLow * 0.85 + uPulse * 0.3 * uLow, 0.0, 0.95));
    }

    if (uWake > 0.001) {
        float g = dot(col, vec3(0.299, 0.587, 0.114));
        col = mix(col, vec3(g) * vec3(0.85, 1.0, 1.1), smoothstep(0.15, 0.6, uWake));     // grey, a sensor's cool cast
        col *= 0.82 + 0.18 * sin(uv.y * 900.0);                                             // scan lines
        float vig = smoothstep(1.25 - 0.9 * uWake, 0.2, r);                                 // the view opening from the centre
        col *= mix(1.0, vig, smoothstep(0.0, 0.5, uWake));
        col *= 1.0 - smoothstep(0.65, 1.0, uWake);                                          // black
    }
    FragColor = vec4(col, 1.0);
}
