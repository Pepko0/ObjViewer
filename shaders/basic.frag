#version 330 core

struct Material {
    vec3 diffuseColor;
    sampler2D diffuseMap;
    int hasTexture;
};

uniform Material uMaterial;
uniform vec3 uLightDir;
uniform vec3 uViewPos;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-uLightDir);

    float diff = max(dot(norm, lightDir), 0.0);

    vec3 baseColor = uMaterial.diffuseColor;
    if (uMaterial.hasTexture == 1) {
        baseColor *= texture(uMaterial.diffuseMap, TexCoord).rgb;
    }

    vec3 ambient = 0.1 * baseColor;
    vec3 diffuse = diff * baseColor;

    FragColor = vec4(ambient + diffuse, 1.0);
}
