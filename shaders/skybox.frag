#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform samplerCube samplerCubeMap;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;
layout (location = 3) out vec4 outMrao;

// void main() 
// {
// 	// outFragColor = texture(samplerCubeMap, inUVW);
// 	// outFragColor = vec4(1.0, 0.0, 0.0, 1.0);
// 	outPosition = vec4(0.0, 0.0, 0.0, 0.0);
// 	outNormal = vec4(0.0, 0.0, 0.0, 0.0);
// 	outAlbedo = texture(samplerCubeMap, inUVW);
// 	outMrao = vec4(0.0, 0.0, 0.0, 0.0);
// }
float exposure = 4.50;
float gamma = 2.20;
// From http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 Uncharted2Tonemap(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

void main() 
{
	vec3 color = texture(samplerCubeMap, inUVW).rgb;

	// Tone mapping
	color = Uncharted2Tonemap(color * exposure);
	color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));	
	// Gamma correction
	color = pow(color, vec3(1.0f / gamma));
	
	outAlbedo = vec4(color, 1.0);
	outPosition = vec4(0.0, 0.0, 0.0, 0.0);
}