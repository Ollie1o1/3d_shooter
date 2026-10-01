#version 330 core
// Sniper scope: black outside a circular lens, a dark ring at its rim, a
// faint blue tint and vignette inside. The reticle is drawn on top as HUD.
in vec2 TexCoords;
out vec4 FragColor;
uniform vec2  uRes;      // screen size in pixels
uniform vec2  uCenter;   // lens centre in pixels (sways slightly)
uniform float uRadius;   // lens radius in pixels
uniform float uAlpha;    // fades the whole overlay in as the scope comes up
void main() {
    vec2 p = TexCoords * uRes;
    float d = length(p - uCenter) / uRadius;
    float outside = smoothstep(0.985, 1.0, d);
    float rim     = smoothstep(0.80, 1.0, d) * 0.85;
    vec3  tint    = vec3(0.02, 0.04, 0.06);
    float a = max(outside, rim);
    // A soft lens reflection along the upper-left edge
    float glint = smoothstep(0.6, 0.95, d) * max(0.0, dot(normalize(p - uCenter + 0.001), normalize(vec2(-1.0, 1.0)))) * 0.12;
    FragColor = vec4(mix(tint, vec3(0.25, 0.35, 0.45), glint), a * uAlpha);
}
