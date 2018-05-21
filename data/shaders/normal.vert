#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;

layout (binding = 0) uniform UBO
{
	mat4 MVP;
	mat4 model;
	vec4 camera;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outLightVec;
layout (location = 2) out vec3 outViewVec;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
    vec3 lightPos = vec3(2.0, -0.5, 7.0);

	gl_Position = ubo.MVP * vec4(inPos, 1.0);

    vec4 pos = ubo.model * vec4(inPos, 1.0);
    outNormal = mat3(inverse(transpose(ubo.model))) * inNormal;
    vec3 lPos = mat3(ubo.model) * lightPos;
    outLightVec = lPos - pos.xyz;
    outViewVec = ubo.camera.xyz - pos.xyz;
}
