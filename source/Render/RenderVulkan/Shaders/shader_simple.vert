#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    #define MAX_BONES 100
    mat4 gBones[MAX_BONES];
    vec3 viewPos;
} ubo;

layout( push_constant ) uniform PushConstantBufferObject {
  mat4 PVM;
  mat4 model;
} pushConstant;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormals;
layout(location = 2) in vec2 inTexCoord;


layout (location = 3) in uvec4 aBoneIDs;
layout (location = 4) in uvec4 aBoneIDs2;

layout (location = 5) in vec4 aWeights;
layout (location = 6) in vec4 aWeights2;


layout(location = 0) out vec3 FragPos;
layout(location = 1) out vec3 Normal;
layout(location = 2) out vec2 TexCoords;
layout(location = 3) out vec3 viewPos;


void main() {
    gl_Position = pushConstant.PVM * vec4(inPosition, 1.0);
    TexCoords = inTexCoord;

    Normal = inNormals;
    viewPos = ubo.viewPos;
}