#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform UBO 
{
	vec3 eyePos;
    vec3 lightPos;
} ubo;

layout (binding = 1) uniform sampler2D samplerposition;
layout (binding = 2) uniform sampler2D samplerNormal;
layout (binding = 3) uniform sampler2D samplerAlbedo;
layout (binding = 4) uniform sampler2D samplerMrao;
layout (binding = 5) uniform samplerCube samplerCubemap;
layout (binding = 6) uniform sampler2D samplerBrdfLUT;

// in
layout (location = 0) in vec2 inUV;

// out
layout (location = 0) out vec4 outFragcolor;

struct Light {
	vec4 position;
	vec3 color;
	float radius;
};

void main()  
{
	Light light;
	light.position = vec4(20.f, 20.f, 20.f, 1.f);
	light.color = vec3(1.f, 1.f, 1.f);
	light.radius = 500.f;

	// Get G-Buffer values
	vec3 fragPos = texture(samplerposition, inUV).rgb;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	vec4 albedo = texture(samplerAlbedo, inUV);
	
	#define lightCount 1
	#define ambient 0.0
	
	// Ambient part
	vec3 fragcolor  = albedo.rgb * ambient;
	// Vector to light
	vec3 L = light.position.xyz - fragPos;
	// Distance from light to fragment position
	float dist = length(L);

	// Viewer to fragment
	vec3 V = ubo.eyePos - fragPos;
	V = normalize(V);
	
	// Light to fragment
	L = normalize(L);

	// Attenuation
	float atten = light.radius / (pow(dist, 2.0) + 1.0);

	// Diffuse part
	vec3 N = normalize(normal);
	float NdotL = max(0.0, dot(N, L));
	vec3 diff = light.color * albedo.rgb * NdotL * atten;

	// Specular part
	// Specular map values are stored in alpha of albedo mrtmrt
	vec3 R = reflect(-L, N);
	float NdotR = max(0.0, dot(R, V));
	// vec3 spec = light.color * albedo.a * pow(NdotR, 16.0) * atten;
	fragcolor += diff;	
	fragcolor = texture(samplerCubemap, R, 0.0f).xyz;
	// fragcolor = texture(samplerBrdfLUT, inUV).rgb;
	fragcolor = R;


	outFragcolor = vec4(normalize(vec3(texture(samplerNormal, inUV))), 1.f);
	// outFragcolor = vec4((normal + vec3(1.f)) / 2.f, 1.f);
	outFragcolor = vec4(R, 1.0);
	outFragcolor = vec4(fragcolor, 1.0f);
}