#version 330 core

out vec4 FragColor;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
}; 

struct DirLight {
    vec3 direction;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};  

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords;

DirLight dirLight;
DirLight dirLight2;
float shininess;

uniform Material material;

uniform vec3 viewPos;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);

void main()
{

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

    FragColor = vec4(result, 1.0);
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
    vec3 ambient  = light.ambient  * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));
    //FragColor = vec4(specular, 1.0);
    return (ambient + diffuse);
}
