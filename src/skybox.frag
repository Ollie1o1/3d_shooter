#version 330 core
in vec2 vUV;
out vec4 FragColor;

// Per-room sky tone — set every frame from GameplayState so each room reads
// visually distinct. (No initializers: GLSL ES, used by the web build, forbids them.)
uniform vec3  uBottom;
uniform vec3  uTop;
uniform float uStarIntensity;

void main() {
    // Vertical gradient: room-tinted horizon to a darker zenith
    float t   = vUV.y;
    vec3  col = mix(uBottom, uTop, t);
    // Add some stars (pseudo-random based on position)
    float starX = fract(vUV.x * 127.3 + vUV.y * 311.7);
    float starY = fract(vUV.x * 269.5 + vUV.y * 183.3);
    float star  = step(0.998, starX * starY);
    col += vec3(star * uStarIntensity);
    FragColor = vec4(col, 1.0);
}
