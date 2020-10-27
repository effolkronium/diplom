#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 FragPos;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoords;
layout(location = 3) in vec3 viewPos;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D texSpecular;

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);

DirLight dirLight;
DirLight dirLight2;
float shininess;

void main() {

    dirLight.direction = vec3(-0.5, -1.0, -0.3);
    dirLight.ambient = vec3(0.15, 0.15, 0.15);
    dirLight.diffuse = vec3(0.5, 0.5, 0.5);
    dirLight.specular = vec3(1.0, 1.0, 1.0);

    dirLight2.direction = vec3(1.0, 1.0, 1.0);
    dirLight2.ambient = vec3(0.15, 0.15, 0.15);
    dirLight2.diffuse = vec3(0.5, 0.5, 0.5);
    dirLight2.specular = vec3(1.0, 1.0, 1.0);
    
    shininess = 30;


    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = CalcDirLight(dirLight, norm, viewDir);
    result += CalcDirLight(dirLight2, norm, viewDir);

    outColor = vec4(result, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);

    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    // combine results
    vec3 ambient  = light.ambient  * vec3(texture(texSampler, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(texSampler, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(texSpecular, TexCoords));
    return (ambient + diffuse);
}
