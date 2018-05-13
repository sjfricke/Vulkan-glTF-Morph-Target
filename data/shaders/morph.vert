#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 1) in vec3 inTangent;

layout (binding = 0) uniform UBO
{
	mat4 MVP;
	mat4 model;
	vec4 camera;
} ubo;

// Tried having the morphTargets.buf a vec3[], but there must be some inheirent padding issues not aware of
layout(binding = 1) readonly buffer MorphTargets {
   float buf[];
} morphTargets;

layout (constant_id = 0) const uint morphBufStride = 1;
layout (constant_id = 1) const uint morphNormalOffset = 1;
layout (constant_id = 2) const uint morphTangentOffset = 1;

#define maxMorphCount 8

layout(push_constant) uniform PushConsts {
	float morphWeights[maxMorphCount];
} pushConsts;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outLightVec;
layout (location = 2) out vec3 outViewVec;

out gl_PerVertex
{
	vec4 gl_Position;
};

uint pIndex;
void main()
{
    vec3 lightPos = vec3(2.0, -0.5, 2.0);
    vec3 morphPos = inPos;

    for (uint i = 0, pIndex = 0; i < morphNormalOffset; i++, pIndex++) {
        morphPos += vec3(morphTargets.buf[(morphBufStride * gl_VertexIndex * 3) + (i * 3) + 0],
                         morphTargets.buf[(morphBufStride * gl_VertexIndex * 3) + (i * 3) + 1],
                         morphTargets.buf[(morphBufStride * gl_VertexIndex * 3) + (i * 3) + 2])
                         * pushConsts.morphWeights[pIndex];
    }

    vec3 morphNormal = inNormal;
    for (uint i = morphNormalOffset, pIndex = 0; i < morphTangentOffset; i++, pIndex++) {
        morphNormal += vec3(morphTargets.buf[(morphBufStride * gl_VertexIndex * 3) + (i * 3) + 0],
                            morphTargets.buf[(morphBufStride * gl_VertexIndex * 3) + (i * 3) + 1],
                            morphTargets.buf[(morphBufStride * gl_VertexIndex * 3) + (i * 3) + 2])
                          * pushConsts.morphWeights[pIndex];
    }

    // unused at the moment
    vec3 morphTagent = inTangent;
    for (uint i = morphTangentOffset, pIndex = 0; i < morphBufStride; i++, pIndex++) {
        morphTagent += vec3(morphTargets.buf[(morphBufStride * gl_VertexIndex * 3) + (i * 3) + 0],
                            morphTargets.buf[(morphBufStride * gl_VertexIndex * 3) + (i * 3) + 1],
                            morphTargets.buf[(morphBufStride * gl_VertexIndex * 3) + (i * 3) + 2])
                          * pushConsts.morphWeights[pIndex];
    }

	gl_Position = ubo.MVP * vec4(morphPos, 1.0);

    vec4 pos = ubo.model * vec4(inPos, 1.0);
    outNormal = mat3(inverse(transpose(ubo.model))) * morphNormal;
    vec3 lPos = mat3(ubo.model) * lightPos;
    outLightVec = lPos - pos.xyz;
    outViewVec = ubo.camera.xyz - pos.xyz;
}
