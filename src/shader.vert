#version 330 core

// =============================================================================
// Vertex Shader
// =============================================================================
// Runs once per vertex. Transforms each vertex from local object space all the
// way to clip space (what the GPU uses to rasterize triangles on screen).
//
// PIPELINE:
//   local space  --(model)-->  world space  --(view)-->  camera space  --(projection)-->  clip space
//
// The "layout(location = N)" must match the glVertexAttribPointer calls in Mesh::upload().
// =============================================================================

// Per-vertex inputs (from the VBO via the VAO layout)
layout(location = 0) in vec3 aPos;      // vertex position in local/object space
layout(location = 1) in vec2 aTexCoord; // UV coordinates for texture sampling
layout(location = 2) in vec3 aNormal;   // surface normal for lighting
layout(location = 3) in vec3 aColor;    // per-vertex surface tint

// Outputs passed to the fragment shader (interpolated across the triangle)
out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos; // world-space position, used for lighting in the fragment shader
out vec3 VertColor;

// Uniform matrices — set once per draw call from ShaderProgram::setMat4()
uniform mat4  model;        // object-to-world transform
uniform mat4  view;         // world-to-camera transform (inverse of camera pose)
uniform mat4  projection;   // camera-to-clip transform (perspective)

// PSX retro aesthetic: vertex snapping.
// 0.0 = off (clean, default). 1.0 = maximum jitter.
// Good range: 0.05 – 0.20 for a subtle retro look.
uniform float uPSXStrength; // default 0

void main()
{
    // Transform vertex to world space (needed for lighting)
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    // Full MVP transform: puts the vertex in clip space for rasterization
    vec4 clipPos = projection * view * worldPos;

    // PSX-style vertex snapping: quantise clip-space XY to a coarse grid.
    // The grid is expressed in NDC (divide by w, snap, multiply back by w)
    // so snap resolution is uniform across the screen.
    if (uPSXStrength > 0.0) {
        float snapRes = mix(512.0, 64.0, uPSXStrength); // higher = finer = less jitter
        clipPos.xy = floor(clipPos.xy / clipPos.w * snapRes + 0.5) / snapRes * clipPos.w;
    }

    gl_Position = clipPos;

    TexCoord = aTexCoord;

    // Transform normal to world space so lighting is correct when the model
    // is rotated or scaled. The transpose(inverse(model)) handles non-uniform scaling.
    // For a pure translation (no scale/rotation), this equals mat3(model).
    Normal = mat3(transpose(inverse(model))) * aNormal;

    VertColor = aColor;
}
