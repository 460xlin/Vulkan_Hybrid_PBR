#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// #define SHOW_ALBEDO
// #define SHOW_METALLIC
// #define SHOW_ROUGHNESS
// #define SHOW_AO
// #define SHOW_NORMAL
// #define SHOW_POSITION
// #define SHOW_MRAO
// #define DEBUG_RAYTRACE
#define USE_SHADOW_MAP


layout (binding = 0) uniform UBO 
{
	vec3 eyePos;
    vec3 lightPos;
	mat4 modelView;
} ubo;

vec3 u_LightColor = vec3(1.f, 1.f, 1.f);
vec3 u_ScaleIBLAmbient = vec3(1.5f);
float u_Exposure = 4.50;
float u_Gamma = 2.20;
vec3 shadow;
float lightShadow;
float IBLSpecularShadow;
float IBLDiffuseShadow;



layout (binding = 1) uniform sampler2D samplerPosition;
layout (binding = 2) uniform sampler2D samplerNormal;
layout (binding = 3) uniform sampler2D samplerAlbedo;
layout (binding = 4) uniform sampler2D samplerMrao;
layout (binding = 5) uniform samplerCube samplerCubemap;
layout (binding = 6) uniform sampler2D samplerBrdfLUT;
layout (binding = 7) uniform sampler2D samplerShadowMap;


// in
layout (location = 0) in vec2 inUV;

// out
layout (location = 0) out vec4 outFragColor;



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

vec3 CubeMapToneAndGamma(vec3 c) {
	vec3 color = c;
	color = Uncharted2Tonemap(color * u_Exposure);
	color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));	
	// Gamma correction
	color = pow(color, vec3(1.0f / u_Gamma));
	return color;
}

// Encapsulate the various inputs used by the various functions in the shading equation
// We store values in this struct to simplify the integration of alternative implementations
// of the shading terms, outlined in the Readme.MD Appendix.
struct PBRInfo
{
    float NdotL;                  // cos angle between normal and light direction
    float NdotV;                  // cos angle between normal and view direction
    float NdotH;                  // cos angle between normal and half vector
    float LdotH;                  // cos angle between light direction and half vector
    float VdotH;                  // cos angle between view direction and half vector
    float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
    float metalness;              // metallic value at the surface
    vec3 reflectance0;            // full reflectance color (normal incidence angle)
    vec3 reflectance90;           // reflectance color at grazing angle
    float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
    vec3 diffuseColor;            // color contribution from diffuse lighting
    vec3 specularColor;           // color contribution from specular lighting
};

const float M_PI = 3.141592653589793;
const float c_MinRoughness = 0.04;

// #define MANUAL_SRGB
// #define SRGB_FAST_APPROXIMATION
vec4 SRGBtoLINEAR(vec4 srgbIn)
{
    #ifdef MANUAL_SRGB
    #ifdef SRGB_FAST_APPROXIMATION
    vec3 linOut = pow(srgbIn.xyz,vec3(2.2));
    #else //SRGB_FAST_APPROXIMATION
    vec3 bLess = step(vec3(0.04045),srgbIn.xyz);
    vec3 linOut = mix( srgbIn.xyz/vec3(12.92), pow((srgbIn.xyz+vec3(0.055))/vec3(1.055),vec3(2.4)), bLess );
    #endif //SRGB_FAST_APPROXIMATION
    return vec4(linOut,srgbIn.w);;
    #else //MANUAL_SRGB
    return srgbIn;
    #endif //MANUAL_SRGB
}

vec3 getIBLContribution(PBRInfo pbrInputs, vec3 n, vec3 reflection)
{
    float mipCount = 9.0; // resolution of 512x512
    float lod = (pbrInputs.perceptualRoughness * mipCount);
    // retrieve a scale and bias to F0. See [1], Figure 3
    vec3 brdf = SRGBtoLINEAR(texture(samplerBrdfLUT, vec2(pbrInputs.NdotV, 1.0 - pbrInputs.perceptualRoughness))).rgb;

    // vec3 diffuseLight = SRGBtoLINEAR(texture(samplerCubemap, -n)).rgb;
    // vec3 specularLight = SRGBtoLINEAR(texture(samplerCubemap, -reflection)).rgb;

	vec3 diffuseLight = CubeMapToneAndGamma(texture(samplerCubemap, -n, lod).rgb);
    vec3 specularLight = CubeMapToneAndGamma(texture(samplerCubemap, -reflection).rgb);

    vec3 diffuse = diffuseLight * pbrInputs.diffuseColor;
    vec3 specular = specularLight * (pbrInputs.specularColor * brdf.x + brdf.y);

    // For presentation, this allows us to disable IBL terms
    diffuse *= u_ScaleIBLAmbient.x;
    specular *= u_ScaleIBLAmbient.y;

#ifdef USE_SHADOW_MAP
    if (IBLDiffuseShadow > 0.f) {
        diffuse *= log(IBLDiffuseShadow + 1);
        specular *= log(IBLSpecularShadow + 1);
    }
#endif
    // return specular;
    return diffuse + specular;
}

// Basic Lambertian diffuse
// Implementation from Lambert's Photometria https://archive.org/details/lambertsphotome00lambgoog
// See also [1], Equation 1
vec3 diffuse(PBRInfo pbrInputs)
{
    return pbrInputs.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(PBRInfo pbrInputs)
{
    return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
}

// This calculates the specular geometric attenuation (aka G()),
// where rougher material will reflect less light back to the viewer.
// This implementation is based on [1] Equation 4, and we adopt their modifications to
// alphaRoughness as input as originally proposed in [2].
float geometricOcclusion(PBRInfo pbrInputs)
{
    float NdotL = pbrInputs.NdotL;
    float NdotV = pbrInputs.NdotV;
    float r = pbrInputs.alphaRoughness;

    float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
    float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
    return attenuationL * attenuationV;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(PBRInfo pbrInputs)
{
    float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;
    float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;
    return roughnessSq / (M_PI * f * f);
}

void main() {
#ifdef DEBUG_RAYTRACE
    vec3 temp =  texture(samplerShadowMap, inUV).xyz;
    float distanceT = texture(samplerShadowMap, inUV).z;
	// vec3 myColor = texture(samplerAlbedo, inUV).rgb;
    // if(distanceT > 0)
    // {
    //     myColor *= 0.5;
    // }
	// outFragColor = vec4(myColor, 1.f);   

    outFragColor = vec4(vec3(distanceT) , 1.0f);
    outFragColor = vec4(temp, 1.0f);
    outFragColor = vec4(temp.xyz, 1.0f);
    return;
#endif

	vec4 fragPosV4 = texture(samplerPosition, inUV);
	vec3 fragColor = texture(samplerAlbedo, inUV).rgb;
	if (fragPosV4.w < 1.0f) {
#ifndef SHOW_AO
#ifndef SHOW_METALLIC
#ifndef SHOW_ROUGHNESS
#ifndef SHOW_NORMAL
#ifndef SHOW_POSITION
#ifndef SHOW_MRAO
        outFragColor = vec4(fragColor, 1.0f);
        return;
#endif
#endif
#endif
#endif
#endif
#endif
		outFragColor = vec4(1.0f);
		return;
	}

    shadow = texture(samplerShadowMap, inUV).xyz;
#ifdef USE_SHADOW_MAP
    lightShadow = shadow.x;
    IBLSpecularShadow = shadow.y;
    IBLDiffuseShadow = shadow.z;
#endif

	vec3 fragPos = vec3(fragPosV4.x, fragPosV4.y, fragPosV4.z);
	vec3 mrao = texture(samplerMrao, inUV).xyz;
    float metallic = mrao.x;
	float perceptualRoughness = mrao.y;

	perceptualRoughness = clamp(perceptualRoughness, c_MinRoughness, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);

	// Roughness is authored as perceptual roughness; as is convention,
    // convert to material roughness by squaring the perceptual roughness [2].
    float alphaRoughness = perceptualRoughness * perceptualRoughness;

	vec4 baseColor = SRGBtoLINEAR(texture(samplerAlbedo, inUV));

	vec3 f0 = vec3(0.04);
    vec3 diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
    diffuseColor *= 1.0 - metallic;
    vec3 specularColor = mix(f0, baseColor.rgb, metallic);

	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

	// For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
    // For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
    float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
    vec3 specularEnvironmentR0 = specularColor.rgb;
    vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

	vec3 n = texture(samplerNormal, inUV).xyz; 			  // normal at surface point

	vec3 v = normalize(ubo.eyePos - fragPos);        // Vector from surface point to camera

	vec3 l = normalize(ubo.lightPos - fragPos);             // directional light

	vec3 h = normalize(l+v);                          // Half vector between both l and v
    vec3 reflection = -normalize(reflect(v, n));

	float NdotL = clamp(dot(n, l), 0.001, 1.0);
    float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
    float NdotH = clamp(dot(n, h), 0.0, 1.0);
    float LdotH = clamp(dot(l, h), 0.0, 1.0);
    float VdotH = clamp(dot(v, h), 0.0, 1.0);

	PBRInfo pbrInputs = PBRInfo(
        NdotL,
        NdotV,
        NdotH,
        LdotH,
        VdotH,
        perceptualRoughness,
        metallic,
        specularEnvironmentR0,
        specularEnvironmentR90,
        alphaRoughness,
        diffuseColor,
        specularColor
    );

	// Calculate the shading terms for the microfacet specular shading model
    vec3 F = specularReflection(pbrInputs);
    float G = geometricOcclusion(pbrInputs);
    float D = microfacetDistribution(pbrInputs);

	// Calculation of analytical lighting contribution
    vec3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);
    vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);
    // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
    vec3 color = vec3(0.f);
    vec3 pointLightContribution =
        NdotL * u_LightColor * (diffuseContrib + specContrib);
    #ifdef USE_SHADOW_MAP
    if (lightShadow >= 0.f) {
        pointLightContribution *= log(lightShadow + 1);
    }
    #endif
    color += pointLightContribution;


	vec3 IBLContribution = getIBLContribution(pbrInputs, n, reflection);
	color += IBLContribution;

	// IMPT: ao is used here
	// raytrace is used here
	float ao = mrao.z;
	color = mix(color, color * ao, 0.5);

	// This section uses mix to override final color for reference app visualization
    // of various parameters in the lighting equation.
    // color = mix(color, F, u_ScaleFGDSpec.x);
    // color = mix(color, vec3(G), u_ScaleFGDSpec.y);
    // color = mix(color, vec3(D), u_ScaleFGDSpec.z);
    // color = mix(color, specContrib, u_ScaleFGDSpec.w);

    // color = mix(color, diffuseContrib, u_ScaleDiffBaseMR.x);
    // color = mix(color, baseColor.rgb, 0.3f);
    // color = mix(color, vec3(metallic), u_ScaleDiffBaseMR.z);
    // color = mix(color, vec3(perceptualRoughness), u_ScaleDiffBaseMR.w);
	outFragColor = vec4(color, 1.f);
	// outFragColor = vec4(fragPos, 1.f);
#ifdef SHOW_ALBEDO
	outFragColor = texture(samplerAlbedo, inUV);
#endif

#ifdef SHOW_METALLIC
	outFragColor = vec4(vec3(mrao.x), 1.f);
#endif

#ifdef SHOW_ROUGHNESS
	outFragColor = vec4(vec3(mrao.y), 1.f);
#endif

#ifdef SHOW_AO
	outFragColor = vec4(vec3(mrao.z), 1.f);
#endif

#ifdef SHOW_NORMAL
    outFragColor = vec4(n, 1.f);
#endif

#ifdef SHOW_POSITION
    outFragColor = vec4(fragPos, 1.f);
#endif

#ifdef SHOW_MRAO
    outFragColor = vec4(mrao, 1.f);
#endif
}