#version 330 core

// Minimal passthrough shader for the grapple rope and anchor cross.
// Only position is needed — colour and dither are handled in the frag.
layout(location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * vec4(aPos, 1.0);
}
