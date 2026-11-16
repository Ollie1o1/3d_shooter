#version 330 core

// Per-vertex (mesh geometry — the same unit cube for every instance)
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
// location 3 (per-vertex color) unused — color comes from instance data

// Per-instance (divisor=1, one record per box, uploaded each frame)
layout(location = 4) in mat4 instanceModel;    // occupies locations 4, 5, 6, 7
layout(location = 8) in vec3 instanceColor;
layout(location = 9) in vec3 instanceEmissive;
layout(location = 10) in float instanceSurface;   // a boss's surface (0: none)
layout(location = 11) in vec4 instanceDeform;

uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out vec3 vColor;
out vec3 vEmissive;
flat out float vSurf;
out vec3 vLocal;    // the point on the part, in metres of the part (its own axes)
out vec3 vLocalN;
flat out vec3 vSize;

vec3 spine(vec3 p) {
    float w=smoothstep(-0.9,0.45,p.y);
    vec3 q=p-vec3(0,-0.75,0);
    q.x*=1.0+instanceDeform.z*0.025*w;
    q.z*=1.0+instanceDeform.z*0.06*w;
    float a=instanceDeform.x*w,c=cos(a),s=sin(a);
    q=vec3(q.x,c*q.y-s*q.z,s*q.y+c*q.z);
    a=instanceDeform.y*w;c=cos(a);s=sin(a);
    return vec3(c*q.x+s*q.z,q.y,-s*q.x+c*q.z)+vec3(0,-0.75,0);
}
void main()
{
    vec3 posed=aPos,posedNormal=aNormal;
    if(instanceDeform.w>0.5){
        posed=spine(aPos);
        // The Jacobian keeps the actual curved skin lit correctly while it bends.
        mat3 J=mat3((spine(aPos+vec3(.001,0,0))-posed)*1000.0,
                    (spine(aPos+vec3(0,.001,0))-posed)*1000.0,
                    (spine(aPos+vec3(0,0,.001))-posed)*1000.0);
        posedNormal=transpose(inverse(J))*aNormal;
    }
    vec4 worldPos = instanceModel * vec4(posed, 1.0);
    FragPos     = worldPos.xyz;
    gl_Position = projection * view * worldPos;
    TexCoord    = aTexCoord;
    // Normal matrix: handles non-uniform scale correctly.
    Normal      = transpose(inverse(mat3(instanceModel))) * posedNormal;
    vColor      = instanceColor;
    vEmissive   = instanceEmissive;
    vSurf       = instanceSurface;
    vSize       = vec3(length(instanceModel[0].xyz), length(instanceModel[1].xyz), length(instanceModel[2].xyz));
    vLocal      = aPos * vSize;
    vLocalN     = aNormal;
}
