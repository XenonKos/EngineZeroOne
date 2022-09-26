// RayTracing.hlsl
#define HLSL
#include "RayTracing.hlsli"

// Shader Resources

// Global Resources
RaytracingAccelerationStructure gScene : register(t0, space0);
RWTexture2D<float4> gRenderTarget : register(u0);
ConstantBuffer<SceneConstantBuffer> gSceneCB : register(b0);

// Index and Vertex Buffer
ByteAddressBuffer gIndices : register(t1, space0);
StructuredBuffer<Vertex> gVertices : register(t2, space0);

float4 TraceRadianceRay(Ray ray, uint currentRecursionDepth)
{
    // 递归深度限制
    if (currentRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    
    RayDesc rayDesc;
    rayDesc.Origin = ray.Origin;
    rayDesc.Direction = ray.Direction;
    rayDesc.TMin = 0;
    rayDesc.TMax = 10000;
    
    RadianceRayPayload payload =
    {
        float4(0.0f, 0.0f, 0.0f, 0.0f),
        currentRecursionDepth + 1
    };
    
    TraceRay(
        gScene,
        RAY_FLAG_CULL_BACK_FACING_TRANGLES,
        0xFF, 0, 0, 0,
        rayDesc, payload
    );

    return payload.color;
}

[shader("raygeneration")]
void raygen_main()
{
    Ray ray = GenerateCameraRay(DispatchRaysIndex().xy, gSceneCB.CameraPosW, gSceneCB.InvViewProj);
    
    
}

[shader("closesthit")]
void closesthit_main()
{
    
}

[shader("miss")]
void miss_main()
{
    
}