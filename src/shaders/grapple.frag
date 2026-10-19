#version 330 core

out vec4 FragColor;

uniform vec3  uColor;
uniform float uAlpha; // 0.0 – 1.0 — controls dither density (0 = invisible, 1 = solid)

// 4×4 ordered Bayer matrix for PSX-style dithered transparency.
// Each fragment is discarded if its alpha threshold is below the Bayer value
// at its screen-space position, producing a checkerboard-like dissolve pattern.
// At uAlpha = 1.0 all 16 cells pass (solid); at 0.5 half pass (50% screen dots).
void main()
{
    const float bayer[16] = float[16](
         0.0/16.0,  8.0/16.0,  2.0/16.0, 10.0/16.0,
        12.0/16.0,  4.0/16.0, 14.0/16.0,  6.0/16.0,
         3.0/16.0, 11.0/16.0,  1.0/16.0,  9.0/16.0,
        15.0/16.0,  7.0/16.0, 13.0/16.0,  5.0/16.0
    );

    int bx = int(mod(gl_FragCoord.x, 4.0));
    int by = int(mod(gl_FragCoord.y, 4.0));

    if (uAlpha <= bayer[by * 4 + bx]) discard;

    FragColor = vec4(uColor, 1.0);
}
