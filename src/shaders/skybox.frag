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
uniform float uAssessment;
uniform float uMoonLid;     // 0..1: the plain's moon blinking shut (it was an eye; it is the Gleaner's now)
uniform float uPlain;       // 1 = THE BLOOD MOON PLAIN (Chapter 4): the blood moon, burning cloud
uniform float uFlash;       // 0..1: lightning inside the plain's cloud, toward uFlashDir
uniform vec3  uFlashDir;
uniform float uCold;        // 1 = THE COLD STORE (Chapter 5): the ringed gas giant, a small cold sun, aurora, the Void below
uniform float uDark;        // 0..1: TOTALITY: the ring dims to embers

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

void main() {
    vec4 v = uInvProj * vec4(vNDC, 1.0, 1.0);
    vec3 dir = normalize((uInvView * vec4(v.xyz / v.w, 0.0)).xyz);
    float y = dir.y;

    if (uPlain > 0.5) { FragColor = vec4(plainSky(dir), 1.0); return; }
    if (uCold > 0.5) { FragColor = vec4(coldSky(dir), 1.0); return; }

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
        FragColor=vec4(realm,1.0);return;
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

    FragColor = vec4(col, 1.0);
}
