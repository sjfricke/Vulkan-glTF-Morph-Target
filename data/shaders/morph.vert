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
	vec4 lightPos;
} ubo;

// Tried having the morphTargets.buf a vec3[], but there must be some inheirent padding issues not aware of
layout(binding = 1) readonly buffer MorphTargets {
   float buf[];
} morphTargets;

#define MAX_WEIGHTS 8

layout(push_constant) uniform PushConsts {
    uint  bufferOffset;
	uint  normalOffset;
	uint  tangentOffset;
	uint  vertexStride;
	float weights[MAX_WEIGHTS];
} push;

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
    vec3 morphPos = inPos;
    uint vertexOffset = (push.vertexStride * gl_VertexIndex * 3);

    for (uint i = 0, pIndex = 0; i < push.normalOffset; i++, pIndex++) {
        morphPos += vec3(morphTargets.buf[(vertexOffset + (i * 3) + 0) + push.bufferOffset],
                         morphTargets.buf[(vertexOffset + (i * 3) + 1) + push.bufferOffset],
                         morphTargets.buf[(vertexOffset + (i * 3) + 2) + push.bufferOffset])
                         * push.weights[pIndex];
    }

    vec3 morphNormal = inNormal;
    for (uint i = push.normalOffset, pIndex = 0; i < push.tangentOffset; i++, pIndex++) {
        morphNormal += vec3(morphTargets.buf[(vertexOffset + (i * 3) + 0) + push.bufferOffset],
                            morphTargets.buf[(vertexOffset + (i * 3) + 1) + push.bufferOffset],
                            morphTargets.buf[(vertexOffset + (i * 3) + 2) + push.bufferOffset])
                          * push.weights[pIndex];
    }

    // unused at the moment
    vec3 morphTagent = inTangent;
    for (uint i = push.tangentOffset, pIndex = 0; i < push.vertexStride; i++, pIndex++) {
        morphTagent += vec3(morphTargets.buf[(vertexOffset + (i * 3) + 0) + push.bufferOffset],
                            morphTargets.buf[(vertexOffset + (i * 3) + 1) + push.bufferOffset],
                            morphTargets.buf[(vertexOffset + (i * 3) + 2) + push.bufferOffset])
                          * push.weights[pIndex];
    }

	gl_Position = ubo.MVP * vec4(morphPos, 1.0);

    vec4 pos = ubo.model * vec4(inPos, 1.0);
    outNormal = mat3(inverse(transpose(ubo.model))) * morphNormal;
    vec3 lPos = mat3(ubo.model) * ubo.lightPos.xyz;
    outLightVec = lPos - pos.xyz;
    outViewVec = ubo.camera.xyz - pos.xyz;
}
