#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inLightVec;

layout (location = 0) out vec4 outFragColor;

void main()
{
	vec4 color = vec4(1.0, 1.0, 1.0, 1.0);

	vec3 N = normalize(inNormal);
    vec3 L = normalize(inLightVec);
    vec3 diffuse = max(dot(N, L), 0.7) * vec3(1.0);

    outFragColor = vec4(diffuse * color.rgb, 1.0);
}