// Common.hlsli
#define HLSL
#include "Light.hlsli"
#include "../Include/ConstantBuffer.h"


// Constant Buffer
ConstantBuffer<RenderItemData>  gRenderItemData : register(b0);
ConstantBuffer<PassData>        gPassData : register(b1);

// Texture
TextureCube gCubeMap : register(t0);
Texture2D   gShadowMap : register(t1);
Texture2D   gTextures[128] : register(t2);

// MaterialData
StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

// Static Samplers
SamplerState gSamPointWrap : register(s0);
SamplerState gSamPointClamp : register(s1);
SamplerState gSamLinearWrap : register(s2);
SamplerState gSamLinearClamp : register(s3);
SamplerState gSamAnisotropicWrap : register(s4);
SamplerState gSamAnisotropicClamp : register(s5);
SamplerComparisonState gSamShadow : register(s6);


// Normal Texture Sampling Helper
float3 NormalSampleToWorldSpace(float3 normalTextureSample, float3 unitNormalW, float3 tangentW)
{
    float3 normalT = 2.0f * normalTextureSample - 1.0f;
    
    // TBN Space
    float3 N = unitNormalW;
    float3 T = normalize(tangentW - N * dot(N, tangentW));
    float3 B = cross(N, T);
    
    float3x3 TBN = float3x3(T, B, N);
    
    // Tangent Space -> World Space
    float3 bumpedNormalW = mul(normalT, TBN);
    
    return bumpedNormalW;
}

// Bump Texture Sampling Helper
float3 BumpSampleToNormal(float3 bumpTextureSamples, float dx, float dy, float3 unitNormalW, float3 tangentW)
{
    static const float heightScale = 0.05f;
    float3 bumpT = 2.0f * bumpTextureSamples - 1.0f;
    
    // TBN Space
    float3 N = unitNormalW;
    float3 T = normalize(tangentW - N * dot(N, tangentW));
    float3 B = cross(N, T);
    
    float3x3 TBN = float3x3(T, B, N);
    
    float3 slopeU = float3(dx, 0.0f, heightScale * (bumpT[1] - bumpT[0]));
    float3 slopeV = float3(0.0f, dy, heightScale * (bumpT[2] - bumpT[0]));
    float3 bumpedNormalT = cross(slopeU, slopeV);
    
    float3 bumpedNormalW = normalize(mul(bumpedNormalT, TBN));
    return bumpedNormalW;
}


float CalcShadowFactor(float4 shadowPosH)
{
    // Perspective Division
    shadowPosH.xyz /= shadowPosH.w;
    
    // NDC Depth
    float depth = shadowPosH.z;
    
    uint width, height, numMips;
    gShadowMap.GetDimensions(0, width, height, numMips);
    
    float dx = 1.0f / (float) width;
    float lit = 0.0f;
    
    // 3X3
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx, dx), float2(0.0f, dx), float2(dx, dx)
    };
    
    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        lit += gShadowMap.SampleCmpLevelZero(gSamShadow, shadowPosH.xy + offsets[i], depth).r;
    }

    return lit / 9.0f;
}