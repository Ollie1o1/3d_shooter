#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D scene;
uniform sampler2D bloomBlur;
void main() {
    vec3 sceneColor = texture(scene, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    vec3 result = sceneColor + bloomColor * 0.8;
    // Tone mapping (Reinhard)
    result = result / (result + vec3(1.0));
    // Gamma correction
    result = pow(result, vec3(1.0/2.2));
    FragColor = vec4(result, 1.0);
}
