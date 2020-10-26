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
    mat4 boneTransform = ubo.gBones[aBoneIDs[0]] * aWeights[0];
    boneTransform     += ubo.gBones[aBoneIDs[1]] * aWeights[1];
    boneTransform     += ubo.gBones[aBoneIDs[2]] * aWeights[2];
    boneTransform     += ubo.gBones[aBoneIDs[3]] * aWeights[3];

	boneTransform     += ubo.gBones[aBoneIDs2[0]] * aWeights2[0];
    boneTransform     += ubo.gBones[aBoneIDs2[1]] * aWeights2[1];
    boneTransform     += ubo.gBones[aBoneIDs2[2]] * aWeights2[2];
    boneTransform     += ubo.gBones[aBoneIDs2[3]] * aWeights2[3];

    gl_Position = pushConstant.PVM * boneTransform * vec4(inPosition, 1.0);
    TexCoords = inTexCoord;

    FragPos = vec3(pushConstant.model * vec4(inPosition, 1.0));

    vec4 NormalBone = boneTransform * vec4(inNormals, 0.0);
    Normal = (pushConstant.model * NormalBone).xyz;

    viewPos = ubo.viewPos;
}