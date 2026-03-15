#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D image;
uniform int horizontal;
const float weight[5] = float[](0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);
void main() {
    vec2 texOffset = 1.0 / textureSize(image, 0);
    vec3 result = texture(image, TexCoords).rgb * weight[0];
    if (horizontal == 1) {
        for (int i=1;i<5;++i) {
            result += texture(image, TexCoords + vec2(texOffset.x*i,0.0)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(texOffset.x*i,0.0)).rgb * weight[i];
        }
    } else {
        for (int i=1;i<5;++i) {
            result += texture(image, TexCoords + vec2(0.0,texOffset.y*i)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(0.0,texOffset.y*i)).rgb * weight[i];
        }
    }
    FragColor = vec4(result, 1.0);
}
