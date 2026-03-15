#version 330 core

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec3 VertColor;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec3 viewPos;
uniform vec3 emissiveColor;
uniform vec3 objectColor;

uniform vec3 pointLightPos[4];
uniform vec3 pointLightColor[4];

void main()
{
    // Surface colour = texture * per-vertex tint * per-object tint
    vec4 texColor = texture(uTexture, TexCoord) * vec4(VertColor * objectColor, 1.0);

    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // Hemisphere ambient
    vec3 skyColor    = vec3(0.18, 0.22, 0.38);
    vec3 groundColor = vec3(0.07, 0.06, 0.05);
    float hemi = norm.y * 0.5 + 0.5;
    vec3 ambient = mix(groundColor, skyColor, hemi);

    // Directional light — Blinn-Phong
    vec3  L    = normalize(-lightDir);
    float diff = max(dot(norm, L), 0.0) * 0.85 + 0.08;
    vec3  H    = normalize(L + viewDir);
    float spec = pow(max(dot(norm, H), 0.0), 48.0);
    vec3  specular = lightColor * spec * 0.18;

    // Point lights
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

    // Rim light
    float rim    = pow(1.0 - max(dot(norm, viewDir), 0.0), 4.0);
    vec3 rimColor = vec3(0.10, 0.14, 0.30) * rim;

    // UV-based edge lines — darkens pixels near the boundary of each polygon face.
    // Makes box edges visually readable without extra geometry.
    float eu = min(TexCoord.x, 1.0 - TexCoord.x);
    float ev = min(TexCoord.y, 1.0 - TexCoord.y);
    float edgeFactor = smoothstep(0.0, 0.035, min(eu, ev));
    // edgeFactor = 0 at edges (dark), 1 at face centre (full colour)

    vec3 lighting = ambient + diff * lightColor + pointContrib;
    vec3 result   = (lighting * texColor.rgb + specular + rimColor + emissiveColor)
                  * mix(0.15, 1.0, edgeFactor);  // darken at face edges

    FragColor = vec4(result, texColor.a);
}
