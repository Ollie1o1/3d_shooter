#version 330 core
in vec2 vUV;
out vec4 FragColor;
void main() {
    // Vertical gradient: dark indigo at bottom, deep blue-black at top
    float t = vUV.y;
    vec3 bottom = vec3(0.04, 0.02, 0.08);
    vec3 top    = vec3(0.01, 0.01, 0.04);
    vec3 col    = mix(bottom, top, t);
    // Add some stars (pseudo-random based on position)
    float starX = fract(vUV.x * 127.3 + vUV.y * 311.7);
    float starY = fract(vUV.x * 269.5 + vUV.y * 183.3);
    float star  = step(0.998, starX * starY);
    col += vec3(star * 0.6);
    FragColor = vec4(col, 1.0);
}
