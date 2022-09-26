#ifndef _LIGHT_HLSLI
#define _LIGHT_HLSLI
#include "../Include/Light.h"

struct Material {
	float4	DiffuseAlbedo;
	float3	FresnelR0;
	float	Roughness;
};

// Helper Functions
float AttenuationFactor(float distance, float AttenuationRange) {
	// 平方衰减
    return saturate(1 - distance * distance / (AttenuationRange * AttenuationRange));
}

float3 SchlickApprox(float3 R0, float3 normal, float3 toLight)
{
    float cosTheta = max(dot(normal, toLight), 0);
    float3 ret = R0 + (1 - R0) * pow(1 - cosTheta, 5);
    return ret;
}

float LambertFactor(float3 normal, float3 toLight)
{
    float ret = max(dot(normal, toLight), 0);
    return ret;
}

float3 FresnelFactor(float3 R0, float3 normal, float3 toLight)
{
    float3 ret = SchlickApprox(R0, normal, toLight);
    return ret;
}

float NormalDistribution(float roughness, float3 normal, float3 halfwayVec)
{
    float m = (1 - roughness) * 256.0f;
    float normalizationFactor = (m + 8.0f) / 8.0f;
	
    float ret = normalizationFactor * pow(max(dot(normal, halfwayVec), 0), m);
    //float ret = pow(max(dot(normal, halfwayVec), 0), m);
    //float ret = pow(dot(normal, halfwayVec), m);
    return ret;
}
    

float GGXNDF(float3 halfwayVec, float3 normal, float roughness)
{
    static const float PI = 3.14159265f;
    float alpha_g = roughness * roughness;
    float alpha_g_sq = alpha_g * alpha_g;
    float NdotH = dot(halfwayVec, normal);
    float d = (1 + NdotH * NdotH * (alpha_g_sq - 1));
        
    float ret = alpha_g_sq / PI * d * d;
        
    return ret;
}
    
float Geometry(float NdotL, float NdotC, float roughness)
{
    float k = (roughness + 1) * (roughness + 1) / 8.0f;    
    float G1 = NdotL / (NdotL * (1 - k) + k), G2 = NdotC / (NdotC * (1 - k) + k);
        
    float ret = G1 * G2;

    return ret;
}
    

// Only Direct Lighting
float3 BlinnPhong(float3 lightStrength, 
	Material mat,
	float3 normal, float3 toLight, float3 toCamera)
{
    float3 halfwayVec = normalize(toCamera + toLight);
	
    float3 directLighting = lightStrength * LambertFactor(normal, toLight) *
		(mat.DiffuseAlbedo.rgb + FresnelFactor(mat.FresnelR0, normal, toLight) * NormalDistribution(mat.Roughness, normal, halfwayVec));
    return directLighting;
}

float3 PBR(float3 lightStrength,
    Material mat, 
    float3 normal, float3 toLight, float3 toCamera)
{
    float PI = 3.1415926f;
    float3 halfwayVec = normalize(toCamera + toLight);
        
    float NdotL = dot(normal, toLight); 
    float NdotC = dot(normal, toCamera);
    float3 F = FresnelFactor(mat.FresnelR0, normal, toLight);
    //float D = GGXNDF(halfwayVec, normal, mat.Roughness);
    float D = NormalDistribution(mat.Roughness, normal, halfwayVec);
    float G = Geometry(NdotL, NdotC, mat.Roughness);
        
    float3 diffuse = lightStrength * (float3(1.0f, 1.0f, 1.0f) - F) * mat.DiffuseAlbedo.rgb / PI;
    float3 specular = lightStrength * F * D * G / (4.0f * NdotL * NdotC);
    float3 ret = diffuse + specular;
        
    return ret;
}

float3 ComputeDirectionalLight(DirectionalLight light, 
	Material mat,
	float3 normal,
	float3 toCamera)
{
    float3 toLight = -light.Direction;
    float3 lighting = BlinnPhong(light.Strength, mat, normal, toLight, toCamera);
	
    return lighting;
}

float3 ComputePointLight(PointLight light, 
	Material mat,
	float3 normal,
	float3 pos,
	float3 toCamera)
{
    float3 toLight = light.Position - pos;
	
	// Distance
    float distance = length(toLight);
	// Normalization
    toLight /= distance;
	
	// Distance Attenuation
    float3 lightStrength = light.Strength * AttenuationFactor(distance, light.AttenuationRange);
	
    return BlinnPhong(lightStrength, mat, normal, toLight, toCamera);
}

// 点光源模拟面光源
float3 ComputeRectLight(RectLight light,
	Material mat,
	float3 normal,
	float3 pos,
	float3 toCamera)
{
    float3 toLight = light.Position - pos;
    // Distance
    float distance = length(toLight);
	// Normalization
    toLight /= distance;
	
	// Distance Attenuation
    float3 lightStrength = light.Strength * AttenuationFactor(distance, light.AttenuationRange);
	
    return BlinnPhong(lightStrength, mat, normal, toLight, toCamera);
}

float3 ComputeSpotLight(SpotLight light,
	Material mat,
	float3 normal,
	float3 pos,
	float3 toCamera)
{
    float3 toLight = light.Position - pos;
	
	// Distance
    float distance = length(toLight);
	// Normalization
    toLight /= distance;
	
    float3 lightStrength = light.Strength * AttenuationFactor(distance, light.AttenuationRange);
	
	// Spotlight Angle Attenuation
    float angle = acos(dot(light.Direction, -toLight));
    lightStrength *= saturate(1.0f - angle / light.MaxAngle);
	
    return BlinnPhong(lightStrength, mat, normal, toLight, toCamera);
}
	
// Lighting
float4 ComputeLighting(
    Light lights,
    Material mat,
    float3 normal,
    float3 pos,
    float3 toCamera,
    float3 shadowFactor)
{
// Shadow Factor用于Shadow mapping
    float3 result = 0.0f;
    
// Directional Light
    for (uint directionalLightIndex = 0; directionalLightIndex < lights.NumDirectionalLights; ++directionalLightIndex)
    {
        result += shadowFactor * ComputeDirectionalLight(lights.DirectionalLights[directionalLightIndex], mat, normal, toCamera);
    }
    
// Point Light
    for (uint pointLightIndex = 0; pointLightIndex < lights.NumPointLights; ++pointLightIndex)
    {
        result += ComputePointLight(lights.PointLights[pointLightIndex], mat, normal, pos, toCamera);
    }

// Rect Light
    for (uint rectLightIndex = 0; rectLightIndex < lights.NumRectLights; ++rectLightIndex)
    {
        result += shadowFactor * ComputeRectLight(lights.RectLights[rectLightIndex], mat, normal, pos, toCamera);
    }
    
// Spot Light
    for (uint i = 0; i < lights.NumSpotLights; ++i)
    {
        result += ComputeSpotLight(lights.SpotLights[i], mat, normal, pos, toCamera);
    }

    return float4(result, 0.0f);
}

#endif