#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform UBO 
{
	vec3 eyePos;
    vec3 lightPos;
	mat4 modelView;
} ubo;

layout (binding = 1) uniform sampler2D samplerPosition;
layout (binding = 2) uniform sampler2D samplerNormal;
layout (binding = 3) uniform sampler2D samplerAlbedo;
layout (binding = 4) uniform sampler2D samplerMrao;
layout (binding = 5) uniform samplerCube samplerCubemap;
layout (binding = 6) uniform sampler2D samplerBrdfLUT;

// in
layout (location = 0) in vec2 inUV;

// out
layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec3 fragPos = texture(samplerPosition, inUV).xyz;
	vec3 normal = texture(samplerNormal, inUV).xyz;
	vec3 eye2Frag = fragPos - ubo.eyePos;
	vec3 sampDir = reflect(normalize(eye2Frag), normal);
	vec3 fragColor = texture(samplerCubemap, sampDir).rgb;

	// -------------------------------
	vec3 v = normalize(ubo.eyePos - fragPos);
	vec3 reflection = -normalize(reflect(v, normal));
	fragColor = texture(samplerCubemap, reflection).rgb;
	fragColor = texture(samplerAlbedo, inUV).rgb;
	fragColor = normal;
	// fragColor = normal;
	// fragColor = fragPos;
	outFragColor = vec4(fragColor, 1.f);
}