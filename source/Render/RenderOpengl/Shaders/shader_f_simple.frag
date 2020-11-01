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

void main()
{
    FragColor = texture(material.texture_diffuse1, TexCoords);
}
