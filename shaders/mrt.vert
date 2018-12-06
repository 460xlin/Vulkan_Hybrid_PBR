#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 inTangent;
layout (location = 5) in vec3 inEx;


// layout (binding = 0) uniform UBO 
// {
// 	mat4 projection;
// 	mat4 model;
// 	mat4 view;
// 	vec4 instancePos[3];
// } ubo;

layout (binding = 0) uniform UBO 
{
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 deferredProj;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outColor;
layout (location = 3) out vec3 outWorldPos;
layout (location = 4) out vec3 outTangent;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{

	vec4 deltaPos = vec4(ubo.deferredProj[gl_InstanceIndex][0],
	ubo.deferredProj[gl_InstanceIndex][1],
	ubo.deferredProj[gl_InstanceIndex][2], 0.f);
	// instancing
	vec4 tmpPos = inPos + deltaPos;

	gl_Position = ubo.proj * ubo.view * ubo.model * tmpPos;
	
	// Vertex position in world space
	outWorldPos = vec3(ubo.model * tmpPos);
	// GL to Vulkan coord space
	outWorldPos.y = -outWorldPos.y;
	
	// Normal in world space
	mat3 mNormal = transpose(inverse(mat3(ubo.model)));
	outNormal = mNormal * normalize(inNormal);	
	outTangent = mNormal * normalize(inTangent);
	
	outUV = inUV;
	outUV.x = 1.0 - outUV.x;

	outColor = inColor;
}
