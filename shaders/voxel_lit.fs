#version 330 core
#define NR_POINT_LIGHTS 0

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D dirt;
    sampler2D grassTop;
    sampler2D grassSide;
    float     shininess;
};

struct DirLight {
    vec3 direction; // ray direction (light -> scene)
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float cutOff;
    float outerCutOff;
};

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

out vec4 FragColor;

uniform vec3 viewPos;
uniform Material material;

// Two directional lights
uniform DirLight sunLight;
uniform DirLight moonLight;

// Keep point light structure (disabled when NR_POINT_LIGHTS == 0)
#if NR_POINT_LIGHTS > 0
uniform PointLight pointLights[NR_POINT_LIGHTS];
#endif

// Flashlight
uniform SpotLight spotLight;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in float TexLayer;

vec3 sampleAlbedo(vec2 uv) {
    if (TexLayer < 0.5) {
        return vec3(texture(material.diffuse, uv));
    }
    if (TexLayer < 1.5) {
        return vec3(texture(material.dirt, uv));
    }
    if (TexLayer < 2.5) {
        return vec3(texture(material.grassTop, uv));
    }
    return vec3(texture(material.grassSide, uv));
}

vec3 sampleSpecular(vec2 uv) {
    if (TexLayer < 0.5) {
        return vec3(texture(material.specular, uv));
    }
    return sampleAlbedo(uv) * 0.15;
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = vec3(0.0);

    // Sun + Moon directional lights
    result += CalcDirLight(sunLight,  norm, viewDir);
    result += CalcDirLight(moonLight, norm, viewDir);

    // Point lights (disabled)
#if NR_POINT_LIGHTS > 0
    for (int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
#endif

    // Flashlight
    result += CalcSpotLight(spotLight, norm, FragPos, viewDir);

    FragColor = vec4(result, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    // light.direction is ray direction (light -> scene),
    // for shading we need direction from fragment to light:
    vec3 lightDir = normalize(-light.direction);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 albedo  = sampleAlbedo(TexCoords);
    vec3 specMap = sampleSpecular(TexCoords);

    vec3 ambient  = light.ambient  * albedo;
    vec3 diffuse  = light.diffuse  * diff * albedo;
    vec3 specular = light.specular * spec * specMap;

    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    // Kept for "original structure", but unused when NR_POINT_LIGHTS == 0.
    vec3 lightDir = normalize(light.position - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
                               light.quadratic * (distance * distance));

    vec3 albedo  = sampleAlbedo(TexCoords);
    vec3 specMap = sampleSpecular(TexCoords);

    vec3 ambient  = light.ambient  * albedo;
    vec3 diffuse  = light.diffuse  * diff * albedo;
    vec3 specular = light.specular * spec * specMap;

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 ambient = light.ambient * sampleAlbedo(TexCoords);

    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * sampleAlbedo(TexCoords);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * sampleSpecular(TexCoords);

    // soft edges
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = (light.cutOff - light.outerCutOff);
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    diffuse  *= intensity;
    specular *= intensity;

    // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
                               light.quadratic * (distance * distance));

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}