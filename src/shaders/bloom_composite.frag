#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D scene;
uniform sampler2D bloomBlur;

// ACES filmic curve (Narkowicz 2015 fit). Keeps darks dark and colours
// saturated, rolling highlights off smoothly; scene colours are authored as
// display values, so no separate gamma step.
vec3 aces(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
    vec3 sceneColor = texture(scene, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    vec3 result = (sceneColor + bloomColor * 0.7) * 1.35;
    FragColor = vec4(aces(result), 1.0);
}
