#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;

layout (constant_id = 0) const uint morphOffsets = 0;

layout (binding = 0) uniform UBO 
{
	mat4 MVP;
	mat4 model;
} ubo;

#define hardTest 6
layout(binding = 1) buffer MorphTargets {
   vec3 morphTargets[ ];
};

#define maxMorphCount 8

layout(push_constant) uniform PushConsts {
	float morphWeights[maxMorphCount];
} pushConsts;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outLightVec;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
    vec3 lightPos = vec3(1.0, -1.0, 3.0);

//    vec3 morphPos = inPos + (inPos_1 * pushConsts.morphWeights[0]) + (inPos_2 * pushConsts.morphWeights[1]);
    vec3 morphPos = inPos + (morphTargets[hardTest * gl_VertexIndex] * pushConsts.morphWeights[0]) + (morphTargets[hardTest * gl_VertexIndex + 1] * pushConsts.morphWeights[1]);
//    vec3 morphNormal = inNormal + (inNormal_1 * pushConsts.morphWeights[0]) + (inNormal_2 * pushConsts.morphWeights[1]);
    vec3 morphNormal = inNormal;

	gl_Position = ubo.MVP * vec4(morphPos, 1.0);

    outNormal = transpose(inverse(mat3(ubo.model))) * morphNormal;

    vec4 pos = ubo.model * vec4(inPos, 1.0);
    outNormal = mat3(inverse(transpose(ubo.model))) * inNormal;
    vec3 lPos = mat3(ubo.model) * lightPos;
    outLightVec = lPos - pos.xyz;
}
