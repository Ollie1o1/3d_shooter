#version 330 core
// THE SWEEP's wall (Gameplay_Ring.h renderSweepWall): a curtain of lifted ice across the ring, drawn on one
// tall quad per layer. Ragged churning crest, billows streaming upward, dusty at the foot and glittering
// white higher up, a bright lip along the crest. Translucent: what is behind it shows through its thin parts
in vec3 vWorld;
out vec4 FragColor;
uniform vec3  uOrigin;   // a point on its foot
uniform vec3  uSide;     // across the ring, along the wall
uniform float uGround, uHeight, uTime, uSeen, uLayer;
uniform vec3  uViewPos;
float h21(vec2 p) { vec3 q = fract(vec3(p.xyx) * 0.1031); q += dot(q, q.yzx + 33.33); return fract((q.x + q.y) * q.z); }
float vn(vec2 p) {
    vec2 i = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
    return mix(mix(h21(i), h21(i + vec2(1.0, 0.0)), f.x), mix(h21(i + vec2(0.0, 1.0)), h21(i + vec2(1.0, 1.0)), f.x), f.y);
}
float fb(vec2 p) { return vn(p) * 0.5 + vn(p * 2.07 + 3.1) * 0.3 + vn(p * 4.3 + 7.7) * 0.2; }
void main() {
    float x = dot(vWorld - uOrigin, uSide);
    float y = (vWorld.y - uGround) / uHeight;
    float t = uTime;
    float crest = 0.5 + 0.32 * fb(vec2(x * 0.04 + uLayer * 7.0, t * 0.5)) + 0.1 * sin(x * 0.13 + t * 2.3 + uLayer * 2.0);
    float bill = fb(vec2(x * 0.08 + uLayer * 11.0, y * 2.4 - t * 1.4));
    float streak = fb(vec2(x * 0.55, y * 0.7 - t * 2.6));
    float body = smoothstep(crest, crest - 0.32, y);
    float a = body * (0.62 + 0.5 * bill) * (0.78 + 0.22 * streak);
    float lip = exp(-abs(y - crest + 0.03) * 28.0) * smoothstep(0.25, 0.6, bill) * step(y, crest + 0.06);
    vec3 dust = vec3(0.5, 0.43, 0.34), ice = vec3(1.45, 1.4, 1.3);
    vec3 col = mix(dust, ice, smoothstep(0.04, 0.5, y)) * (0.55 + 0.65 * bill);
    col += ice * lip * 1.6;
    // glints of ice turning in it: small round points, gone within a few metres of you (they'd be blobs)
    vec2 gp = vec2(x * 6.0, y * uHeight * 6.0 - t * 40.0);
    float sp = step(0.975, h21(floor(gp))) * (1.0 - smoothstep(0.12, 0.3, length(fract(gp) - 0.5))) * body;
    sp *= smoothstep(6.0, 20.0, length(vWorld - uViewPos));
    col += vec3(2.6, 2.5, 2.3) * sp;
    a = clamp(a * (uLayer > 0.5 ? 0.6 : 0.97) + lip * 0.7 + sp * 0.5, 0.0, 1.0);
    a *= smoothstep(0.0, 0.03, y + 0.01);
    FragColor = vec4(col * uSeen, a * uSeen);
}
