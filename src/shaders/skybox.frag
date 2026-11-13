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
uniform float uFlare;       // 0..1: a flash of the eclipse (his entrance, his phases), decaying
uniform float uDawn;        // 0..1: the eclipse sliding off the sun (his death)
uniform float uSwallow;     // 0..1: MORNINGSTAR: the eclipse grows over the whole sky, then its ring of fire settles on the horizon
uniform float uLabRealm; // 0 ordinary, 1 nebula, 2 hadal, 3 fractured
uniform float uRealmFade; // 1 = the lab's realm alone; less blends it over the ordinary sky (Chapter 3's arrival)
uniform float uAssessment;
uniform float uMoonLid;     // 0..1: the plain's moon blinking shut (it was an eye; it is the Gleaner's now)
uniform float uPlain;       // 1 = THE BLOOD MOON PLAIN (Chapter 4): the blood moon, burning cloud
uniform float uFlash;       // 0..1: lightning inside the plain's cloud, toward uFlashDir
uniform vec3  uFlashDir;
uniform float uCold;        // 1 = THE COLD STORE (Chapter 5): the ringed gas giant, a small cold sun, aurora, the Void below
uniform float uDark;        // 0..1: TOTALITY: the ring dims to embers
uniform float uRing;        // 1 = THE KEPT RING (Chapter 6): standing in the rings, the giant on the horizon, the Shepherd
uniform vec4  uShepherd;    // the Shepherd: xyz its direction, w its angular radius (it grows level by level)
uniform float uRingFlash;   // 0..1: its ridge lamps (a sweep's flash)
uniform float uRingNight;   // 0..1: the giant's shadow (the Shadow Band): no sun, the giant's night side
uniform float uRingBolt;    // 0..1: lightning in its night side, now (the game times it: the ring lights with it)
uniform vec2  uRingBoltAt;  // where in the cloud (longitude, latitude offsets)
uniform float uRingDawn;    // 0..1: the sunrise out of the Shadow Band: a red arc on its edge, then the diamond
uniform float uRingSwept;   // seconds since a sweep threw the swept off the ring (<0: none lately): they burn up across the giant (13.3)
uniform float uRingBend;
uniform sampler2D uRingGiant;   // the giant's cloud tops, painted once (render/RingDetail.h): x longitude, y latitude angle
uniform sampler2D uRingGalaxy;
uniform sampler2D uRingNebula;  // the nebula's picture (render/RingDetail.h)  // the galaxy's dust: x azimuth, y height    // 0..1: the main rings opening into an arch over you as you near the Shepherd (its pull bends your ringlet under them)

float hash3(vec3 p) { return fract(sin(dot(p, vec3(12.9898, 78.233, 37.719))) * 43758.5453); }
float hash2(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash2(i), hash2(i + vec2(1.0, 0.0)), f.x), mix(hash2(i + vec2(0.0, 1.0)), hash2(i + vec2(1.0, 1.0)), f.x), f.y);
}
float fbm(vec2 p) {
    float s = 0.0, a = 0.5;
    for (int k = 0; k < 4; ++k) { s += a * vnoise(p); p = p * 2.03 + vec2(1.7, 9.2); a *= 0.5; }
    return s;
}

// Inside the eclipse (MORNINGSTAR): black overhead, a ring of white fire low all the way round,
// streamers of flame licking up off it, its light lying on the black below the line
vec3 insideSky(float az, float y) {
    float lit = 1.0 - 0.8 * uDark;
    float lick = vnoise(vec2(az * 14.0 + uTime * 0.6, uTime * 0.4)), lick2 = vnoise(vec2(az * 31.0 - uTime * 0.9, 5.0));
    float flame = (0.5 + 0.8 * pow(lick, 2.0) + 0.5 * pow(lick2, 3.0)) * exp(-max(y, 0.0) * (11.0 - 5.0 * lick));
    vec3 sky = vec3(0.004, 0.001, 0.0);
    sky += vec3(1.7, 0.5, 0.12) * flame * step(0.0, y) * 0.55 * lit;                       // the flames above the line
    sky += vec3(2.6, 2.2, 1.8) * exp(-abs(y - 0.004) * 140.0) * (0.8 + 0.4 * lick) * lit;   // the ring itself, white-hot
    sky += vec3(1.5, 0.45, 0.12) * exp(-abs(y) * 22.0) * 0.5 * lit;                         // its glow
    sky += vec3(0.9, 0.25, 0.06) * exp(y * 30.0) * step(y, 0.0) * 0.35 * lit;               // its light on the black below
    return sky;
}

// The eclipse: a black disc at uSunDir with a burning rim, a corona of
// streamers turning slowly round it, and blood all round the horizon
vec3 eclipseSky(vec3 dir, vec3 col) {
    float y = dir.y;
    vec3 blood = uSunColor;                        // the sunset's colour
    vec3 hot   = vec3(2.4, 1.5, 0.9);              // the rim
    float heat = clamp(uHeat + 0.8 * uFlare, 0.0, 1.6);
    float dawn = uDawn;
    blood = mix(blood, vec3(1.5, 0.75, 0.3), dawn);   // the blood goes gold

    // the horizon: a band of blood all the way round, uneven, brightest at the line
    float az = atan(dir.z, dir.x);
    // swallowed whole: the inside is all there is to see, so skip the sky it covers
    float inside = smoothstep(0.62, 1.0, uSwallow);
    if (inside >= 1.0) return insideSky(az, y);
    float swell = 0.75 + 0.25 * sin(az * 2.0 + 0.6) + 0.15 * sin(az * 5.0 + 2.2);
    float band = exp(-abs(y) * 9.0) * swell;
    col += blood * band * (0.75 + 0.5 * heat);
    col += blood * uFlare * 0.18;                    // the whole sky catches the flare
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
    float grow = clamp(uSwallow * 1.35, 0.0, 1.0);
    float size = uSunSize * (1.0 + 70.0 * grow * grow * grow);   // the swallow: the disc spreads over everything
    float r   = ang / size;                          // 1 at the disc's edge
    float phi = atan(dot(dir, t2), dot(dir, t1));

    // the corona: long streamers, slowly turning, reaching further as it burns
    float reach = 1.6 + 1.4 * heat;
    float s1 = vnoise(vec2(phi * 5.0 + uTime * 0.05, 0.0)), s2 = vnoise(vec2(phi * 13.0 - uTime * 0.09, 3.0));
    float streak = 0.35 + 0.9 * pow(s1, 2.0) + 0.5 * pow(s2, 3.0);
    float fall = exp(-max(r - 1.0, 0.0) * 2.4 / (streak * reach));
    float corona = fall * step(1.0, r);
    vec3 coronaCol = mix(blood * 1.6, vec3(1.9, 1.3, 0.9), exp(-max(r - 1.0, 0.0) * 4.0));
    col += coronaCol * corona * (0.9 + 0.8 * heat) * (1.0 - 0.6 * dawn);
    col += blood * exp(-max(r - 1.0, 0.0) * 1.1) * (0.22 + 0.2 * heat) * step(1.0, r);   // the red bloom round it
    // the rim: a thin ring of fire, beaded where the light breaks through
    float rq = (r - 1.0) / 0.025;
    float rim = exp(-rq * rq);
    float beads = 0.75 + 0.25 * sin(phi * 23.0 + uTime * 0.7) + 0.6 * pow(vnoise(vec2(phi * 9.0, uTime * 0.3)), 6.0);
    col += hot * rim * beads * (1.4 + heat + 2.0 * uFlare) * (1.0 - dawn);
    // the sun behind it, and the moon's disc over it: black, a faint ember on its
    // limb; at the dawn the moon slides off and the sun burns through
    // (Before the dawn the moon sits square on the sun, so rm = r and none of it shows:
    // the whole block is skipped for every pixel of the fight.)
    float rm = r;
    if (dawn > 0.0) {
        vec3 sunCol = vec3(3.2, 2.7, 2.0);
        float off = dawn * 2.4 * size;
        vec3 mdir = normalize(ax * cos(off) + t1 * sin(off));
        rm = acos(clamp(dot(dir, mdir), -1.0, 1.0)) / size;
        float shown = smoothstep(0.0, 0.5, dawn);
        col += sunCol * 0.45 * exp(-max(r - 1.0, 0.0) * 2.6) * shown * step(1.0, r);     // the sun's own glare
        col += vec3(1.4, 0.75, 0.3) * exp(-max(r - 1.0, 0.0) * 0.9) * 0.3 * shown;       // and the warm sky round it
        if (r < 1.0) col = mix(col, sunCol, step(1.0, rm) * shown);
        float ring = dawn < 0.45 ? exp(-(r - 1.0) * (r - 1.0) / 0.0036) * smoothstep(0.98, 1.0, rm) * sin(dawn / 0.45 * 3.14159) : 0.0;
        col += vec3(4.0, 3.6, 3.0) * ring * 2.0;                                           // the diamond ring as it breaks
    }
    if (rm < 1.0 && dawn < 0.999) col = mix(vec3(0.004, 0.0, 0.003), blood * 0.18, smoothstep(0.82, 1.0, rm));

    if (inside > 0.0) col = mix(col, insideSky(az, y), inside);
    return col;
}


// THE BLOOD MOON PLAIN: a red sky, burning cloud lit from the plain's fires,
// and the moon low in the north, huge and mottled (the Gleaner's eye-core)
vec3 plainSky(vec3 dir) {
    float y = dir.y;
    float az = atan(dir.z, dir.x);
    vec3 col = mix(uHorizon, uZenith, pow(clamp(y, 0.0, 1.0), 0.38));
    col += vec3(0.9, 0.22, 0.05) * exp(-abs(y) * 26.0) * 0.55;             // the burning line all round
    if (y < 0.0) col = mix(uHorizon * 0.45, uGround, clamp(-y * 6.0, 0.0, 1.0));

    // The moon: a dark red disc, mottled, a hot limb, a wide bloody halo
    float c0 = dot(dir, uSunDir);
    float ang = acos(clamp(c0, -1.0, 1.0));
    float r = ang / uSunSize;
    float shown = 1.0 - 0.9 * smoothstep(0.6, 1.0, uMoonLid);                 // its glow goes as it shuts
    col += uSunColor * exp(-max(r - 1.0, 0.0) * 1.6) * 0.32 * shown;    // halo (inside too: shut, it leaves no hole)
    col += uSunColor * exp(-max(r - 1.0, 0.0) * 5.0) * 0.5 * shown;
    if (r < 1.0 && uMoonLid < 0.999) {
        vec3 t1 = normalize(cross(uSunDir, vec3(0.0, 1.0, 0.0)));
        vec3 t2 = cross(uSunDir, t1);
        vec2 q = vec2(dot(dir, t1), dot(dir, t2)) / sin(uSunSize);    // -1..1 across the disc
        float sph = sqrt(max(0.0, 1.0 - dot(q, q)));                   // its roundness
        float m = fbm(q * 3.2 + 1.7) * 0.7 + fbm(q * 9.0 + 4.1) * 0.3;
        vec3 face = mix(vec3(0.5, 0.05, 0.03), vec3(1.5, 0.22, 0.07), smoothstep(0.3, 0.75, m));
        face *= 0.55 + 0.6 * sph;
        face += vec3(2.2, 0.5, 0.12) * pow(1.0 - sph, 3.0);            // the hot limb
        // The lids: dark crusted skin closing from above and below, meeting in a seam
        float lidEdge = 1.0 - uMoonLid;
        float inLid = smoothstep(lidEdge - 0.02, lidEdge + 0.02, abs(q.y) + 0.12 * q.x * q.x);
        vec3 skin = mix(vec3(0.05, 0.01, 0.01), uHorizon * 0.35, 0.5 + 0.5 * fbm(q * 6.0));
        face = mix(face, skin, inLid);
        col = mix(col, face, smoothstep(1.0, 0.985, r));
    }

    // Burning cloud: long dark banks drifting overhead, lit orange from below,
    // with fire running along their undersides
    if (y > 0.0) {
        vec2 p = dir.xz / (y + 0.12);
        float drift = uTime * 0.006;
        float c = fbm(p * vec2(0.55, 1.6) + vec2(drift, drift * 0.3));
        float c2 = fbm(p * 1.7 + vec2(-drift * 2.0, 3.0));
        float bank = smoothstep(0.42, 0.72, c) * smoothstep(0.85, 0.05, y);
        float under = smoothstep(0.55, 0.0, y);
        vec3 smoke = mix(vec3(0.05, 0.01, 0.012), vec3(0.6, 0.12, 0.04), under * (0.4 + 0.6 * c2));
        float fire = pow(c2, 3.0) * smoothstep(0.6, 0.8, c) * under;
        smoke += vec3(2.0, 0.6, 0.12) * fire * (0.7 + 0.3 * sin(uTime * 1.3 + c2 * 20.0));
        // Lightning: the bank lit from inside round the strike, a little of it everywhere
        float strike = pow(max(0.0, dot(dir, uFlashDir)), 18.0);
        smoke += vec3(1.6, 0.9, 1.1) * uFlash * (strike * 3.0 + 0.12) * (0.4 + 0.6 * c2);
        // the moon shows through thin cloud, not thick
        float moon = step(ang, uSunSize * 1.05);
        col = mix(col, smoke, bank * (0.9 - 0.5 * moon));
        col += vec3(0.6, 0.3, 0.4) * uFlash * strike * 0.5;   // and the clear sky round it
    }
    // Faint stars where the smoke is thin, high up
    vec3 cell = floor(dir * 260.0);
    col += vec3(1.0, 0.7, 0.6) * step(0.9985, hash3(cell)) * uStars * smoothstep(0.35, 0.8, y);
    return col;
}

// THE COLD STORE (Chapter 5): a black, clear sky over a frozen planet. A ringed
// gas giant hangs low over the Store (a real sphere and ring plane, so the ring
// passes behind it and in front of it), lit from the side by a small white sun.
// Pale green and violet aurora (never cyan: that is UNIT 24's), a drifting
// blizzard haze along the horizon, and below the horizon the Void: black.
const vec3 GIANT_DIR = vec3(0.4257, 0.3065, -0.8514);   // normalize(0.5, 0.36, -1): LevelCold.h giantDir()
vec3 coldSky(vec3 dir) {
    float y = dir.y;
    float az = atan(dir.z, dir.x);
    vec3 col = mix(uHorizon, uZenith, pow(clamp(y, 0.0, 1.0), 0.32));
    // Stars: many, sharp, a few tinted; they thin out into the haze near the horizon
    vec3 cell = floor(dir * 520.0);
    float st = hash3(cell);
    vec3 starCol = mix(vec3(0.75, 0.85, 1.1), vec3(1.1, 0.95, 0.8), hash3(cell + 3.0));
    col += starCol * step(0.9975, st) * (0.5 + 1.2 * hash3(cell + 9.0)) * uStars * smoothstep(0.04, 0.3, y);
    // A faint band of the galaxy across the sky
    float band = exp(-pow((dir.y * 0.8 - dir.x * 0.5 + dir.z * 0.3 - 0.35) * 3.0, 2.0));
    col += vec3(0.05, 0.06, 0.09) * band * fbm(vec2(az * 6.0, y * 8.0)) * smoothstep(0.05, 0.4, y);

    // Aurora: curtains hanging from high up, rippling slowly sideways
    if (y > 0.05) {
        float a1 = vnoise(vec2(az * 5.0 + uTime * 0.02, 1.0)), a2 = vnoise(vec2(az * 13.0 - uTime * 0.035, 7.0));
        float curtain = pow(a1, 2.2) * (0.6 + 0.8 * a2);
        float folds = 0.6 + 0.4 * sin(az * 60.0 + a1 * 12.0 + uTime * 0.25);
        float base = 0.22 + 0.12 * a2, top = base + 0.35 + 0.2 * a1;
        float shape = smoothstep(base, base + 0.04, y) * (1.0 - smoothstep(base + 0.05, top, y));
        vec3 green = vec3(0.25, 0.95, 0.4), violet = vec3(0.6, 0.3, 0.95);
        vec3 ac = mix(green, violet, smoothstep(base + 0.05, top, y));
        float south = smoothstep(-0.6, 0.4, dir.z);   // strongest in the southern sky, away from the giant
        col += ac * curtain * folds * shape * 0.28 * (0.35 + 0.65 * south);
        col += green * curtain * shape * 0.06;
    }

    // The sun: small and white, a tight bright glow, a wider cold one
    float cs = dot(dir, uSunDir);
    float sa = acos(clamp(cs, -1.0, 1.0));
    col += uSunColor * smoothstep(uSunSize, uSunSize * 0.8, sa) * 3.0;
    col += vec3(0.9, 0.95, 1.0) * exp(-sa * 18.0) * 0.45 + vec3(0.5, 0.6, 0.8) * exp(-sa * 4.0) * 0.12;

    // The gas giant: a sphere 10 units off along GIANT_DIR, with a ring plane through it
    vec3 C = GIANT_DIR * 10.0;
    float R = 10.0 * sin(0.27);
    vec3 ringN = normalize(vec3(0.15, 1.0, 0.32));
    float tb = dot(dir, C), disc = tb * tb - (dot(C, C) - R * R);
    float tPlanet = disc > 0.0 ? tb - sqrt(disc) : 1e9;
    float tRing = dot(C, ringN) / dot(dir, ringN);
    vec3 P = dir * tRing;
    float rr = length(P - C) / R;
    vec4 ring = vec4(0.0);
    if (tRing > 0.0 && rr > 1.35 && rr < 2.35) {
        float bands = 0.55 + 0.45 * sin(rr * 55.0) * sin(rr * 13.0 + 1.0);
        float gap = smoothstep(0.0, 0.03, abs(rr - 1.86));   // the dark gap
        float a = bands * gap * smoothstep(1.35, 1.42, rr) * (1.0 - smoothstep(2.25, 2.35, rr));
        // the planet's shadow falls across the ring on its night side
        vec3 toSun = uSunDir;
        vec3 q = P - C;
        float sh = dot(q, toSun) < 0.0 && length(q - toSun * dot(q, toSun)) < R ? 0.25 : 1.0;
        ring = vec4(vec3(0.85, 0.82, 0.76) * (0.35 + 0.65 * bands) * sh, a * 0.85);
    }
    if (ring.a > 0.0 && tRing > tPlanet) {   // behind the planet: it is hidden
        ring.a = 0.0;
    }
    if (tPlanet < 1e8) {
        vec3 hit = dir * tPlanet, nrm = normalize(hit - C);
        float lat = dot(nrm, normalize(vec3(-0.15, 1.0, -0.32)));
        float turb = fbm(vec2(lat * 12.0, atan(nrm.z, nrm.x) * 3.0 + uTime * 0.002));
        float bandsP = 0.5 + 0.5 * sin(lat * 26.0 + turb * 3.5);
        vec3 surf = mix(vec3(0.62, 0.6, 0.56), vec3(0.78, 0.8, 0.84), bandsP);
        surf = mix(surf, vec3(0.45, 0.52, 0.62), smoothstep(0.55, 0.85, turb) * 0.5);
        float lit = clamp(dot(nrm, uSunDir) * 1.2 + 0.08, 0.0, 1.0);
        vec3 body = surf * lit * 0.9 + vec3(0.02, 0.025, 0.04);   // the night side is not quite black
        float limb = pow(1.0 - clamp(dot(-dir, nrm), 0.0, 1.0), 3.0);
        body += vec3(0.4, 0.5, 0.7) * limb * 0.35 * (0.3 + lit);   // its haze at the edge
        col = mix(col, body, smoothstep(0.0, 0.0006, disc / (tb * tb)));   // (a soft edge)
    }
    col = mix(col, ring.rgb, ring.a);

    // The blizzard wall: a pale haze drifting along the horizon
    float storm = fbm(vec2(az * 9.0 + uTime * 0.03, y * 30.0)) * exp(-abs(y - 0.01) * 22.0);
    col = mix(col, uHorizon * 1.15, clamp(storm * 0.9 + exp(-abs(y) * 40.0) * 0.4, 0.0, 0.85));
    // Below the horizon: the Void, black, a little of the haze on its lip
    if (y < 0.0) col = mix(uHorizon * 0.5, uGround, clamp(-y * 9.0, 0.0, 1.0));
    return col;
}

// THE KEPT RING (Chapter 6): you stand in the plane of the rings. The gas giant
// sits on the horizon to the east, half of it below the ring plane, so the ring
// is an edge-on line of light across its middle; the floor of ice runs to that
// line in every direction, and past its far edge there is nothing: black space,
// stars below as well as above. Due north, on the line, the Shepherd: a walnut
// with a ridge of lamps, and the dark gap it has combed out of the ring behind it.
const vec3 RING_GIANT = vec3(0.9899, -0.02, 0.1414);   // normalize(0.99, -0.02, 0.14): LevelRing.h giantDir()
const float RING_GIANT_R = 0.8;                       // its angular radius: it fills a third of the sky
vec3 ringStars(vec3 dir, float k) {
    vec3 cell = floor(dir * 600.0);
    float st = hash3(cell);
    vec3 sc = mix(vec3(0.8, 0.88, 1.1), vec3(1.15, 0.98, 0.82), hash3(cell + 3.0));
    vec3 col = sc * step(0.9978, st) * (0.4 + 1.4 * hash3(cell + 9.0)) * k;
    // The galaxy: a dusty band, brighter and warmer at its core
    float band = exp(-pow((dir.y * 0.7 + dir.x * 0.45 - dir.z * 0.55 + 0.1) * 2.6, 2.0));
    float az = atan(dir.z, dir.x);
    float n = textureLod(uRingGalaxy, vec2(az / 6.28318531 + 0.5, dir.y * 0.5 + 0.5), 0.0).r;   // (painted once: render/RingDetail.h)
    col += mix(vec3(0.03, 0.035, 0.06), vec3(0.09, 0.07, 0.05), n) * band * n * 1.4 * k;
    // The nebula, high in the south-west over the way you came: its own picture, laid flat on the sky round
    // its centre (render/RingDetail.h nebula), squared back from its storage (deep blacks, bright wisps)
    const vec3 NEB_C = vec3(-0.5872, 0.5338, 0.6085);   // normalize(-0.55, 0.5, 0.57)
    float nf = dot(dir, NEB_C);
    if (nf > 0.6) {
        vec3 nr = normalize(cross(NEB_C, vec3(0.0, 1.0, 0.0))), nu = cross(nr, NEB_C);
        vec2 nq = vec2(dot(dir, nr), dot(dir, nu)) / nf / 0.84 * 0.5 + 0.5;
        if (all(greaterThan(nq, vec2(0.0))) && all(lessThan(nq, vec2(1.0)))) {
            vec3 neb = texture(uRingNebula, nq).rgb;
            col += neb * neb * 0.85 * k;
        }
    }
    col += vec3(0.5, 0.5, 0.6) * step(0.9993, hash3(floor(dir * 1400.0))) * band * k;   // its fine stars
    // Now and then a meteor: a short bright streak somewhere up high, gone in under a second
    float slot = floor(uTime / 4.3);
    float mt = fract(uTime / 4.3) * 4.3;
    if (mt < 0.7 && hash2(vec2(slot, 7.0)) > 0.35) {
        vec3 a0 = normalize(vec3(hash2(vec2(slot, 1.0)) - 0.5, 0.25 + 0.6 * hash2(vec2(slot, 2.0)), hash2(vec2(slot, 3.0)) - 0.5));
        vec3 md = normalize(cross(a0, vec3(hash2(vec2(slot, 4.0)) - 0.5, 1.0, hash2(vec2(slot, 5.0)) - 0.5)));
        float u = mt / 0.7;
        vec3 head = normalize(a0 + md * 0.25 * u), tail = normalize(a0 + md * 0.25 * max(0.0, u - 0.35));
        vec3 ab = head - tail; float hh = clamp(dot(dir - tail, ab) / max(dot(ab, ab), 1e-6), 0.0, 1.0);
        float dd = length(dir - tail - ab * hh);
        col += vec3(1.0, 0.95, 0.85) * exp(-dd * 2500.0) * hh * (1.0 - u) * 2.0 * k;
    }
    return col;
}
// The giant's main rings: a plane through its centre (lifted onto the horizon), tilted by `a` so you
// stand just under it. Radii in the sky's units (the giant is 10 away, its radius ~7.2; your ringlet is at 10)
const float RING_IN = 7.6, RING_OUT = 9.75;
float ringDensity(float r) {
    float x = (r - RING_IN) / (RING_OUT - RING_IN);
    if (x <= 0.0 || x >= 1.0) return 0.0;
    float d = 0.22 + 0.18 * smoothstep(0.05, 0.3, x);                                  // the faint inner ring
    d += 0.5 * smoothstep(0.26, 0.34, x) * (1.0 - smoothstep(0.6, 0.64, x));          // the dense middle
    d *= 1.0 - 0.94 * smoothstep(0.635, 0.655, x) * (1.0 - smoothstep(0.7, 0.72, x)); // the wide dark division
    d *= 1.0 - 0.7 * smoothstep(0.9, 0.906, x) * (1.0 - smoothstep(0.914, 0.92, x));  // a thin gap near the edge
    float fine = vnoise(vec2(r * 190.0, 0.5)) * 0.55 + vnoise(vec2(r * 47.0, 3.0)) * 0.45;
    d *= 0.5 + 0.8 * fine;
    d *= smoothstep(0.0, 0.025, x) * (1.0 - smoothstep(0.975, 1.0, x));
    return clamp(d, 0.0, 0.92);
}
bool hitsBall(vec3 o, vec3 d, vec3 c, float r) {
    vec3 oc = o - c; float b = dot(oc, d), q = dot(oc, oc) - r * r;
    return b < 0.0 && b * b - q > 0.0;
}
// Cellular craters for the Shepherd's skin: darker bowls with a bright rim, at one scale
float craters(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    float v = 0.0;
    for (int y = -1; y <= 1; ++y) for (int x = -1; x <= 1; ++x) {
        vec2 g = vec2(float(x), float(y));
        vec2 o = vec2(hash2(i + g), hash2(i + g + 17.3));
        float rad = 0.18 + 0.3 * hash2(i + g + 41.1);
        float dd = length(g + o - f) / rad;
        v += dd < 1.0 ? -0.5 * (1.0 - dd * dd) : 0.25 * exp(-(dd - 1.0) * (dd - 1.0) * 40.0);
    }
    return v;
}

vec3 ringSky(vec3 dir) {
    float y = dir.y;
    float az = atan(dir.z, dir.x);
    float night = uRingNight;
    vec3 sunD = uSunDir;
    vec3 col = vec3(0.0015, 0.0015, 0.003);
    col += ringStars(dir, 1.0 - 0.3 * (1.0 - night));

    // The sun: tiny, white and hard (no air to soften it)
    float sa = acos(clamp(dot(dir, sunD), -1.0, 1.0));
    col += uSunColor * smoothstep(uSunSize, uSunSize * 0.7, sa) * 4.0 * (1.0 - night);
    col += vec3(1.0, 0.97, 0.9) * exp(-sa * 40.0) * 0.6 * (1.0 - night);
    col += vec3(0.7, 0.65, 0.55) * exp(-sa * 9.0) * 0.05 * (1.0 - night);

    // Two small moons, far off: an icy one high in the west, a rust-dark one low over the band to the south.
    // Lit by the sun like the giant (each shows its own phase)
    for (int m = 0; m < 2; ++m) {
        vec3 md = m == 0 ? normalize(vec3(-0.62, 0.42, 0.45)) : normalize(vec3(-0.15, 0.09, 0.98));
        float mr = m == 0 ? 0.021 : 0.011;
        float ma = acos(clamp(dot(dir, md), -1.0, 1.0));
        if (ma < mr * 1.6) {
            vec3 mrt = normalize(cross(md, vec3(0.0, 1.0, 0.0))), mup = cross(mrt, md);
            vec2 mq = vec2(dot(dir, mrt), dot(dir, mup)) / mr;
            float l2 = dot(mq, mq);
            if (l2 < 1.0) {
                vec3 mn = mrt * mq.x + mup * mq.y - md * sqrt(1.0 - l2);
                float ml = clamp(dot(mn, sunD) * 1.1, 0.0, 1.0) * (1.0 - night);
                vec3 msurf = m == 0 ? vec3(0.86, 0.88, 0.9) * (0.8 + 0.25 * vnoise(mq * 6.0 + 3.0)) : vec3(0.55, 0.36, 0.25) * (0.75 + 0.35 * vnoise(mq * 5.0));
                msurf *= 1.0 - 0.35 * smoothstep(0.55, 0.0, craters(mq * 3.0 + float(m) * 5.0) + 0.4);
                vec3 mc = msurf * (ml + 0.012);
                col = mix(col, mc, smoothstep(1.0, 0.96, l2));
            }
        }
    }

    // The main rings' plane (see ringDensity): where this ray meets it, if it does
    float a = mix(0.0004, 0.034, uRingBend);
    vec3 Gp = vec3(RING_GIANT.x, 0.0, RING_GIANT.z) * 10.0;
    vec3 gFlat = normalize(Gp);
    vec3 pn = normalize(vec3(0.0, cos(a), 0.0) + gFlat * sin(a));
    float pd = dot(dir, pn);
    float tRing = pd > 1e-5 ? dot(Gp, pn) / pd : -1.0;
    float rRing = tRing > 0.0 ? length(dir * tRing - Gp) : 0.0;
    float ringA = tRing > 0.0 ? ringDensity(rRing) : 0.0;

    // The gas giant: a sphere along RING_GIANT, its equator on the ring plane
    vec3 C = RING_GIANT * 10.0;
    float R = 10.0 * sin(RING_GIANT_R);
    float tb = dot(dir, C), disc = tb * tb - (dot(C, C) - R * R);
    float tP = 1e9;
    if (disc > 0.0 && tb > 0.0) {
        tP = tb - sqrt(disc);
        vec3 hit = dir * tP, nrm = normalize(hit - C);
        float lat = nrm.y;                                   // its poles are up and down: the ring is its equator
        float lon = atan(nrm.z, nrm.x);
        // The belts drift, each at its own pace (the equator fastest), and curl where they meet
        float lonA = lon + uTime * 0.0035 * (1.0 + 0.9 * sin(lat * 23.0) + 0.6 * (1.0 - abs(lat)));
        // The belts, their curls, the fine lanes, the storm and its wake: painted once (they barely change),
        // read here. Each latitude reads at its own drift, so the belts still slide past each other
        vec3 surf = textureLod(uRingGiant, vec2(lonA / 6.28318531 + 0.5, asin(clamp(lat, -1.0, 1.0)) / 3.14159265 + 0.5), 0.0).rgb;
        surf = max(mix(vec3(dot(surf, vec3(0.3, 0.55, 0.15))), surf, 1.3), 0.0);   // (more colour than the map holds: vivid, not washed out)
        // The pole: a six-sided jet stream round a blue-grey cap
        if (lat > 0.7) {
            vec2 pp = nrm.xz / lat;
            float ang = uTime * 0.01;
            pp = mat2(cos(ang), -sin(ang), sin(ang), cos(ang)) * pp;
            vec2 ap = abs(pp);
            float hex = max(ap.x * 0.866 + ap.y * 0.5, ap.y);
            float edge = exp(-abs(hex - 0.42) * 45.0);
            surf = mix(surf, vec3(0.55, 0.6, 0.66), smoothstep(0.44, 0.36, hex) * 0.7);
            surf += vec3(0.25, 0.22, 0.16) * edge;
        }
        // Light: a soft terminator, reddened where the sun is low through its haze; the limb darkens
        float ndl = dot(nrm, sunD);
        float lit = smoothstep(-0.06, 0.3, ndl) * (0.55 + 0.45 * clamp(ndl, 0.0, 1.0)) * (1.0 - night);
        vec3 term = mix(vec3(1.0, 0.5, 0.25), vec3(1.0), smoothstep(-0.02, 0.28, ndl));
        float mu = clamp(dot(nrm, -dir), 0.0, 1.0);
        // The rings' real shadow: from this point toward the sun, through the rings' plane
        float sp = dot(sunD, pn);
        float ringShadow = 1.0;
        if (abs(sp) > 1e-4) {
            float s = dot(Gp - hit, pn) / sp;
            if (s > 0.0) ringShadow = 1.0 - 0.85 * ringDensity(length(hit + sunD * s - Gp));
        }
        vec3 body = surf * lit * term * ringShadow * pow(mu, 0.22) * 1.08;
        // The night side: ringshine, the rings' light thrown back onto it (faint, warm)
        body += surf * vec3(0.035, 0.03, 0.024) * (1.0 - lit) * (1.0 - 0.7 * night);
        // Night-side lightning in the Shadow Band (silent flickers deep in the cloud)
        if (night > 0.0 && uRingBolt > 0.0) {
            float d = length(vec2(lon - 3.14 + uRingBoltAt.x, lat - uRingBoltAt.y) * vec2(1.0, 2.0));
            body += vec3(0.6, 0.65, 0.9) * uRingBolt * 1.6 * (exp(-d * 9.0) + 0.15 * exp(-d * 2.0));
        }
        float limb = pow(1.0 - mu, 2.5);
        body += vec3(1.0, 0.7, 0.35) * limb * 0.28 * (0.15 + lit);      // its haze at the edge, warm
        body += vec3(0.35, 0.55, 1.0) * pow(1.0 - mu, 6.0) * 0.12 * smoothstep(0.4, 0.9, lat) * (0.2 + lit);   // the north's blue haze
        // Its aurora: a ring of light round the north pole, curtains rippling along it, seen on the night side
        float aur = exp(-pow((acos(clamp(lat, -1.0, 1.0)) - 0.88) * 9.0, 2.0));   // (low enough to show on the limb from the ring)
        float curt = 0.5 + 0.5 * sin(lon * 23.0 + uTime * 0.7 + 3.0 * sin(lon * 5.0 - uTime * 0.3));
        body += mix(vec3(0.25, 1.0, 0.55), vec3(0.6, 0.35, 1.0), curt) * aur * (0.35 + 0.65 * curt) * 1.6 * (1.0 - lit) * (1.0 - night * 0.3);
        body += vec3(0.9, 0.55, 0.3) * pow(1.0 - mu, 8.0) * smoothstep(-0.2, 0.1, ndl) * 0.5 * (1.0 - night);   // the lit limb's thin bright rim
        // The sunrise: a thin red arc along its edge, brightest on the sun's side, then the diamond
        if (uRingDawn > 0.0) {
            float sunSide = clamp(dot(normalize(nrm.xz + vec2(1e-4)), normalize(sunD.xz + vec2(1e-4))) * 0.5 + 0.5, 0.0, 1.0);
            body += vec3(1.6, 0.35, 0.12) * pow(limb, 5.0) * uRingDawn * (0.3 + 1.7 * sunSide);
            body += vec3(3.0, 2.8, 2.5) * pow(limb, 14.0) * pow(sunSide, 8.0) * smoothstep(0.85, 1.0, uRingDawn) * 6.0;
        }
        // Where the swept go (13.3): a few seconds after a sweep, sparks streak down across its face, the
        // ones it threw burning up. Nobody explains it
        if (uRingSwept > 0.0 && uRingSwept < 9.0) {
            for (int k = 0; k < 6; ++k) {
                float fk = float(k);
                float t0 = 1.2 + fk * 0.9 + hash2(vec2(fk, 3.0)) * 0.8, t = uRingSwept - t0;
                if (t < 0.0 || t > 2.2) continue;
                vec2 s0 = vec2(-0.5 + hash2(vec2(fk, 1.0)) * 1.0, 0.25 + 0.5 * hash2(vec2(fk, 2.0)));
                vec2 v = normalize(vec2(0.35 + 0.4 * hash2(vec2(fk, 5.0)), -1.0)) * 0.22;
                vec2 head = s0 + v * t, tail = s0 + v * max(0.0, t - 0.5);
                vec2 p = vec2(mod(lon + 3.0 + 3.14159, 6.28318) - 3.14159, lat);   // (lon about -3.0 faces you)
                vec2 ab = head - tail; float h = clamp(dot(p - tail, ab) / max(dot(ab, ab), 1e-6), 0.0, 1.0);
                float d = length(p - tail - ab * h);
                float fadeK = (1.0 - smoothstep(1.4, 2.2, t)) * smoothstep(0.0, 0.15, t);
                body += vec3(1.6, 0.9, 0.45) * exp(-d * 900.0) * (0.25 + 0.75 * h) * fadeK * 2.5;
                body += vec3(2.5, 2.2, 1.8) * exp(-length(p - head) * 1500.0) * fadeK * 3.0;
            }
        }
        col = mix(col, body, smoothstep(0.0, 0.0008, disc / (tb * tb)));
    }

    // The main rings, over the giant where they pass in front of it, behind it where it is nearer
    if (ringA > 0.0 && tRing < tP) {
        vec3 P = dir * tRing;
        float x = (rRing - RING_IN) / (RING_OUT - RING_IN);
        vec3 rc = mix(vec3(0.5, 0.55, 0.62), vec3(1.0, 0.86, 0.62), smoothstep(0.2, 0.36, x));   // the faint inner ring blue-grey, the dense middle warm gold
        rc = mix(rc, vec3(0.82, 0.7, 0.55), smoothstep(0.7, 0.8, x));                          // the outer, tan past the division
        rc = mix(rc, vec3(0.75, 0.66, 0.52), 0.3 * vnoise(vec2(rRing * 23.0, 9.0)));
        float lightR = 0.45 + 0.55 * abs(dot(pn, sunD));                 // (lit through, from either side)
        float inShadow = hitsBall(P, sunD, C, R) ? 0.06 : 1.0;           // the giant's shadow across them
        vec3 rl = rc * lightR * inShadow * (1.0 - night * 0.9) * 0.95;
        float fade = smoothstep(0.0, 0.0015, a) ;                        // edge-on: nothing (the hairline below is them)
        col = mix(col, rl, ringA * fade);
    }

    // Your ringlet, edge-on: a hair of light on the horizon (its far particles), brightest toward the sun,
    // and broken where the Shepherd has combed the gap
    vec3 sh = normalize(uShepherd.xyz);
    float shAz = atan(sh.z, sh.x);
    float gap = smoothstep(0.012, 0.05, abs(mod(az - shAz + 3.14159, 6.28318) - 3.14159));
    float line = exp(-abs(y - 0.0006) * 1800.0) * 1.4 + exp(-abs(y) * 260.0) * 0.12;
    vec3 ringCol = mix(vec3(0.75, 0.68, 0.55), vec3(1.1, 1.05, 0.96), 0.5 + 0.5 * dot(normalize(vec3(dir.x, 0.0, dir.z)), normalize(vec3(sunD.x, 0.0, sunD.z))));
    col += ringCol * line * mix(0.25, 1.0, gap) * (1.0 - 0.8 * night);

    // The Shepherd: a moonlet on the line, a walnut with a sharp ridge round its middle
    float r = uShepherd.w;
    if (r > 0.0) {
        vec3 f = sh, rt = normalize(cross(f, vec3(0.0, 1.0, 0.0))), up = cross(rt, f);
        vec2 q = vec2(dot(dir, rt), dot(dir, up)) / max(r, 1e-5);
        float fwd = dot(dir, f);
        float shape = length(q * vec2(1.0, 1.55)) - 0.12 * pow(max(0.0, 1.0 - abs(q.y) * 6.0), 2.0);
        // (only near it: the skin's craters and the corona are the sky's costliest work, and it is a small part of it)
        float reach = r < 0.05 ? 1e9 : 1.0 + 0.35 / max(r, 1e-3);
        if (fwd > 0.0 && shape < reach) {
            float edgeW = clamp(0.0025 / r, 0.006, 0.08);                    // (a sharp silhouette however big it gets)
            float body = 1.0 - smoothstep(1.0 - edgeW, 1.0, shape);
            vec3 n = normalize(vec3(q.x, q.y * 1.55, sqrt(max(0.0, 1.0 - dot(q * vec2(1.0, 1.55), q * vec2(1.0, 1.55))))));
            vec3 nw = normalize(rt * n.x + up * n.y - f * n.z);
            float l = clamp(dot(nw, sunD) * 1.2 + 0.04, 0.0, 1.0) * (1.0 - night);
            float giantShine = clamp(dot(nw, RING_GIANT) * 0.8 + 0.2, 0.0, 1.0);   // the giant's light on its east side
            // Its skin: dark regolith, craters at two scales (the near ones only when it is big), the ridge
            float detail = smoothstep(0.02, 0.2, r);
            float cr = 0.0;
            if (shape < 1.0) {
                cr = craters(q * 6.0);
                if (detail > 0.01) cr += detail * 0.6 * craters(q * 23.0 + 7.0) + detail * 0.35 * craters(q * 71.0 + 3.0);
            }
            vec3 skin0 = vec3(0.4, 0.38, 0.35) * (0.82 + 0.25 * fbm(q * 9.0) + 0.3 * cr);
            vec3 skin = skin0 * (0.03 + l) + skin0 * vec3(0.55, 0.45, 0.3) * giantShine * (0.14 + 0.5 * detail) * (1.0 - night);
            skin += skin0 * vec3(0.05, 0.05, 0.06) * detail;   // (up close, the ring's own glow off its face)
            float ridge = exp(-abs(q.y) * 40.0) * step(length(q), 1.15);
            skin += vec3(0.6, 0.58, 0.52) * ridge * (0.12 + l);
            // Its lamps: the ridge's row, and fainter rows above and below it like a lighthouse's;
            // steady and faint, all lit on a flash
            float lamps = step(0.75, fract(q.x * 7.0)) * exp(-abs(q.y) * 60.0) * step(abs(q.x), 0.98);
            for (int k = 1; k <= 3; ++k) {
                float yy = float(k) * 0.14;
                float row = exp(-abs(abs(q.y) - yy) * 140.0 * (1.0 + detail * 2.0)) * step(0.82 + 0.04 * float(k), fract(q.x * (9.0 + 6.0 * float(k)) + float(k) * 0.37));
                lamps += row * (0.6 - 0.12 * float(k)) * step(shape, 0.97);
            }
            skin += vec3(2.6, 2.6, 2.3) * lamps * (0.15 + 1.2 * detail + 3.0 * uRingFlash);
            skin += vec3(0.9, 0.9, 0.85) * exp(-abs(q.y) * 14.0) * 0.05 * detail * step(shape, 0.99);   // the ridge's row lighting the hull round it
            col = mix(col, skin, body * step(0.0006, r));
            // Eclipse: when it covers the sun, its edge burns (the sun's light round its rim) and a corona
            vec2 qs = vec2(dot(sunD, rt), dot(sunD, up)) / max(r, 1e-5);
            float cover = smoothstep(1.05, 0.9, length(qs * vec2(1.0, 1.55))) * step(0.0, dot(sunD, f));
            if (cover > 0.0) {
                float outside = max(shape - 1.0, 0.0) * r;                      // radians past its edge
                float toward = clamp(dot(normalize(q + vec2(1e-4)), normalize(qs + vec2(1e-4))) * 0.5 + 0.5, 0.0, 1.0);
                vec3 corona = vec3(1.0, 0.96, 0.88) * (exp(-outside * 140.0) * 1.6 + exp(-outside * 18.0) * 0.18) * (0.4 + 0.6 * toward);
                corona *= 0.85 + 0.3 * vnoise(vec2(atan(q.y, q.x) * 9.0, uTime * 0.05));
                col += corona * cover * (1.0 - body) * (1.0 - night);
                col += vec3(1.4, 1.3, 1.15) * exp(-abs(shape - 1.0) * r * 400.0) * pow(toward, 3.0) * cover * (1.0 - night);   // the bright rim nearest the sun
            }
            // Far off it is only a point of light (its lamps) and a glow round it on a flash
            float d = length(dir - f);
            col += vec3(1.6, 1.55, 1.4) * exp(-d / max(r, 0.0015) * 2.0) * (0.25 + 2.5 * uRingFlash) * (1.0 - step(0.006, r) * 0.8) * (1.0 - smoothstep(0.05, 0.15, r));
            col += vec3(1.0, 0.98, 0.9) * exp(-d * 18.0) * uRingFlash * 0.35 * (1.0 - smoothstep(0.05, 0.2, r));
        }
    }

    // Below the line: past the ring's far edge, nothing. Black space, the stars under you,
    // and the giant's southern half (above). A faint glow of the ring's underside close under the line.
    if (y < 0.0) col += ringCol * exp(y * 120.0) * 0.05 * (1.0 - night);
    return col;
}

void main() {
    vec4 v = uInvProj * vec4(vNDC, 1.0, 1.0);
    vec3 dir = normalize((uInvView * vec4(v.xyz / v.w, 0.0)).xyz);
    float y = dir.y;

    if (uPlain > 0.5) { FragColor = vec4(plainSky(dir), 1.0); return; }
    if (uCold > 0.5) { FragColor = vec4(coldSky(dir), 1.0); return; }
    if (uRing > 0.5) { FragColor = vec4(ringSky(dir), 1.0); return; }

    vec3 labSky=vec3(0.0);float labW=0.0;
    if (uLabRealm > 0.5) {
        float az=atan(dir.z,dir.x), elevation=asin(clamp(dir.y,-1.0,1.0));
        vec2 p=vec2(az,elevation)*3.0;
        float n=fbm(p+vec2(0.0,uTime*.002));
        float band=exp(-pow((dir.y+dir.x*.5-dir.z*.25)*2.2,2.0));
        vec3 nebula=vec3(.018,.025,.075)+mix(vec3(.12,.15,.46),vec3(.65,.16,.36),n)*band*pow(n*1.8,2.2);
        nebula+=vec3(.2,.45,.65)*pow(fbm(p*2.5+4.0),4.0)*band;
        // Teal cloud banks off the violet band, as on the ward's concept boards
        float tealBand=exp(-pow((dir.y-dir.x*.6+dir.z*.3+.15)*1.8,2.0));
        nebula+=vec3(.03,.32,.36)*pow(fbm(p*1.4+vec2(7.0,2.0)),2.6)*tealBand*2.2;
        vec3 cell=floor(dir*440.0);float star=hash3(cell);
        nebula+=mix(vec3(.6,.8,1.1),vec3(1.1,.8,.6),hash3(cell+7.0))*step(.9984,star)*(0.6+hash3(cell+13.0));
        nebula+=vec3(.8,.7,1.0)*pow(max(0.0,dot(dir,normalize(vec3(-.45,.7,-.55)))),900.0);
        float beams=pow(.5+.5*sin(az*19.0+sin(az*7.0)*2.0+uTime*.018),10.0);
        vec3 hadal=mix(vec3(.004,.022,.045),vec3(.012,.055,.08),smoothstep(-.6,1.0,dir.y));
        hadal+=vec3(.012,.05,.06)*beams*pow(max(0.0,dir.y),.6);
        hadal+=vec3(.12,.42,.38)*step(.9992,hash3(floor(dir*180.0)))*.5;
        vec3 realm=uLabRealm<1.5?nebula:uLabRealm<2.5?hadal:mix(nebula,hadal,step(0.0,sin(az*3.0+dir.y*4.0)));
        if(uLabRealm>2.5){
            // A lensed void beyond the open assessment floor. Its broken
            // crimson accretion bands slowly separate as the chassis heats up.
            vec3 axis=normalize(vec3(-.62,.6,-.5));   // up and to the left, clear of the portal ahead
            float angle=acos(clamp(dot(dir,axis),-1.0,1.0));
            float rim=exp(-pow((angle-.285)*95.0,2.0));
            float band=exp(-pow((angle-.37-.025*sin(az*4.0+uTime*.05))*(25.0-uAssessment*5.0),2.0));
            float fracture=.4+.6*smoothstep(-.3,.45,sin(az*7.0+dir.y*11.0-uTime*.09));
            realm=mix(vec3(.002,.001,.006),realm,smoothstep(.272,.29,angle));
            realm+=vec3(1.1,.08,.25)*rim*(.8+uAssessment*.7);
            realm+=mix(vec3(.35,.13,.65),vec3(1.0,.055,.18),uAssessment)*band*fracture;
        }
        if(uRealmFade>0.999){FragColor=vec4(realm,1.0);return;}
        labSky=realm;labW=uRealmFade;
    }

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

    FragColor = vec4(mix(col, labSky, labW), 1.0);
}
