#ifndef RAYTRACING_HLSLI
#define RAYTRACING_HLSLI

#define HLSL
#include "RayTracingHLSLCompat.h"

#define MAX_RAY_RECURSION_DEPTH 3

Ray GenerateCameraRay(float2 pixelIndex, float3 cameraPosW, float4x4 invViewProj)
{
    // Center of Pixel
    float2 xy = pixelIndex + 0.5f;
    float2 screenPosH = xy / DispatchRaysDimensions().xy * 2.0f - 1.0f;
    
    float4 screenPosW = mul(float4(screenPosH, 0, 1), invViewProj);
    screenPosW.xyz /= screenPosW.w; // ???
    
    Ray ret;
    ret.Origin = cameraPosW;
    ret.Direction = normalize(screenPosW.xyz - cameraPosW);
    return ret;
}


#endif