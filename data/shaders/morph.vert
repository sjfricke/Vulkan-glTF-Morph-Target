#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inPos_1;
layout (location = 2) in vec3 inPos_2;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
	vec3 camPos;
	float flipUV;
} ubo;

#define morphCount 2

layout(push_constant) uniform PushConsts {
	float morphWeights[morphCount];
} pushConsts;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
    vec3 morphPos = inPos + (inPos_1 * pushConsts.morphWeights[0]) + (inPos_2 * pushConsts.morphWeights[1]);

	gl_Position =  ubo.projection * ubo.view * ubo.model * vec4(morphPos, 1.0);
}
