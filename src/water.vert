#version 330 core

// Standing water: one flat quad per volume, in world space
layout(location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out vec3 vWorld;

void main()
{
    vWorld      = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
