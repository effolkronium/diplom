#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

#define MAX_BONE_PER_VERTEX 8

layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in ivec4 aBoneIDs2;

layout (location = 5) in vec4 aWeights;
layout (location = 6) in vec4 aWeights2;

out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;
out vec4 BoneIDs;
out vec4 Weights;

uniform mat4 model;
uniform mat4 PVM;

#define MAX_BONES 100
uniform mat4 gBones[MAX_BONES];

void main()
{

    gl_Position = PVM * vec4(aPos, 1.0);
    FragPos = vec3(model * vec4(aPos, 1.0));

    Normal = aNormal.xyz;

    TexCoords = aTexCoords;
}