#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D scene;
void main() {
    vec3 color = texture(scene, TexCoords).rgb;
    float lum = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (lum > 0.8)
        FragColor = vec4(color, 1.0);
    else
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
