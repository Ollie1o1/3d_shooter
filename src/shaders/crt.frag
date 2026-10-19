#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D scene;

void main()
{
    vec2 uv = TexCoords;

    // Barrel distortion
    vec2 centered = uv - 0.5;
    float r2 = dot(centered, centered);
    float distortion = 1.0 + r2 * 0.15 + r2 * r2 * 0.05;
    uv = centered * distortion + 0.5;

    // Clip to screen bounds
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        FragColor = vec4(0.0);
        return;
    }

    // Chromatic aberration — offset R and B channels slightly
    float caStrength = 0.002;
    vec2 caOffset = centered * caStrength;
    float r = texture(scene, uv + caOffset).r;
    float g = texture(scene, uv).g;
    float b = texture(scene, uv - caOffset).b;

    vec3 color = vec3(r, g, b);

    // Scanlines — horizontal dark lines
    float scanline = 0.85 + 0.15 * sin(gl_FragCoord.y * 3.14159 * 1.0);
    color *= scanline;

    // Vignette — darken edges
    float vignette = 1.0 - r2 * 1.5;
    vignette = clamp(vignette, 0.0, 1.0);
    color *= vignette;

    // Slight brightness boost to compensate for darkening
    color *= 1.1;

    FragColor = vec4(color, 1.0);
}
