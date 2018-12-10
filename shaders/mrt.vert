#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 inTangent;

layout (binding = 0) uniform CamInfo 
{
	mat4 projMatrix;
	mat4 viewMatrix;
} camera;

layout (binding = 1) uniform SceneObject 
{
	mat4 modelMatrix;
} model;

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
	// instancing
	vec4 tmpPos = vec4(inPos, 1.f);

	gl_Position = camera.projMatrix * camera.viewMatrix * model.modelMatrix * tmpPos;
	gl_Position.y = -gl_Position.y;
	
	// Vertex position in world space
	outWorldPos = vec3(model.modelMatrix * tmpPos);
	// GL to Vulkan coord space
	// outWorldPos.y = -outWorldPos.y;
	
	// Normal in world space
	mat3 mNormal = transpose(inverse(mat3(model.modelMatrix)));
	outNormal = mNormal * normalize(inNormal);	
	outTangent = mNormal * normalize(inTangent);
	
	outUV = inUV;
	outUV.x = 1.0 - outUV.x;

	outColor = inColor;
}
