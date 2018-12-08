#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 inTangent;

layout (binding = 0) uniform UBO 
{
	mat4 projMatrix;
	mat4 viewMatrix;
	mat4 modelMatrix;
	float lodBias;
} ubo;

layout (location = 0) out vec3 outUVW;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	outUVW = inPos.xyz;
	outUVW.x *= -1.0;
	// gl_Position = ubo.projection * ubo.model * vec4(inPos.xyz, 1.0);

	gl_Position =  vec4(inPos.xyz, 1.0);

	gl_Position = ubo.projMatrix * ubo.modelMatrix * vec4(inPos, 1.f);

	// gl_Position = ubo.projMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos, 1.f);
}
