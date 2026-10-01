#version 330 core

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec3 vColor;
in vec3 vEmissive;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform vec3 uSkyAmb;      // hemisphere ambient, per arena
uniform vec3 uGroundAmb;
uniform vec3 uFogColor;
uniform float uFogDensity;
uniform vec3 pointLightPos[4];
uniform vec3 pointLightColor[4];

void main()
{
    vec4 texColor = texture(uTexture, TexCoord) * vec4(vColor, 1.0);

    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 ambient = mix(uGroundAmb, uSkyAmb, norm.y * 0.5 + 0.5);

    // Directional light — Blinn-Phong
    vec3  L    = normalize(-lightDir);
    float diff = max(dot(norm, L), 0.0) * 0.85 + 0.08;
    vec3  H    = normalize(L + viewDir);
    float spec = pow(max(dot(norm, H), 0.0), 48.0);
    vec3  specular = lightColor * spec * 0.18;

    vec3 pointContrib = vec3(0.0);
    for (int i = 0; i < 4; ++i) {
        vec3  toLight = pointLightPos[i] - FragPos;
        float dist    = length(toLight);
        float atten   = 1.0 / (1.0 + 0.25*dist + 0.07*dist*dist);
        vec3  lDir    = normalize(toLight);
        float pDiff   = max(dot(norm, lDir), 0.0);
        vec3  pH      = normalize(lDir + viewDir);
        float pSpec   = pow(max(dot(norm, pH), 0.0), 24.0) * 0.35;
        pointContrib += pointLightColor[i] * (pDiff + pSpec) * atten;
    }

    // Rim light in the sky colour keeps silhouettes readable against the floor
    float rim     = pow(1.0 - max(dot(norm, viewDir), 0.0), 3.0);
    vec3 rimColor = (uSkyAmb * 1.4 + vec3(0.06)) * rim;

    // Dark seams at face edges make every box in a rig read as a separate part
    float eu = min(TexCoord.x, 1.0 - TexCoord.x);
    float ev = min(TexCoord.y, 1.0 - TexCoord.y);
    float edgeFactor = smoothstep(0.0, 0.06, min(eu, ev));

    vec3 lighting = ambient + diff * lightColor + pointContrib;
    vec3 result   = (lighting * texColor.rgb + specular + rimColor) * mix(0.35, 1.0, edgeFactor)
                  + vEmissive;

    float d   = length(viewPos - FragPos) * uFogDensity;
    float fog = 1.0 - exp(-d * d);
    result = mix(result, uFogColor, fog * 0.85);

    FragColor = vec4(result, texColor.a);
}
