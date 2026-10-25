#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D image;
uniform int horizontal;
// A 9-tap Gaussian (weights 0.2270, 0.1946, 0.1216, 0.0541, 0.0162 at 0..4 texels)
// read in 5 fetches: each pair of neighbouring taps is one linear-filtered read
// placed between them by their weights (offset = (1*w1 + 2*w2) / (w1 + w2), ...)
const float weight[3] = float[](0.2270270270, 0.3162162162, 0.0702702703);
const float offset[3] = float[](0.0, 1.3846153846, 3.2307692308);
void main() {
    vec2 texel = 1.0 / vec2(textureSize(image, 0)); // explicit: GLSL ES has no implicit int->float
    vec2 dir = horizontal == 1 ? vec2(texel.x, 0.0) : vec2(0.0, texel.y);
    vec3 result = texture(image, TexCoords).rgb * weight[0];
    for (int i = 1; i < 3; ++i) {
        result += texture(image, TexCoords + dir * offset[i]).rgb * weight[i];
        result += texture(image, TexCoords - dir * offset[i]).rgb * weight[i];
    }
    FragColor = vec4(result, 1.0);
}
