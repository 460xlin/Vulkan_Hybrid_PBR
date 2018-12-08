#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform samplerCube samplerCubeMap;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;
layout (location = 3) out vec4 outMrao;

void main() 
{
	// outFragColor = texture(samplerCubeMap, inUVW);
	// outFragColor = vec4(1.0, 0.0, 0.0, 1.0);
	outPosition = vec4(0.0, 0.0, 0.0, 0.0);
	outNormal = vec4(0.0, 0.0, 0.0, 0.0);
	outAlbedo = texture(samplerCubeMap, inUVW);
	outMrao = vec4(0.0, 0.0, 0.0, 0.0);
}