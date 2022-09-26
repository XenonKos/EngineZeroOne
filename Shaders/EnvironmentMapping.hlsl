// EnvironmentMapping.hlsl
#include "Common.hlsl"

struct VertexIn
{
    float3 PosL : SV_POSITION;
};

struct VertexOut
{
    float3 PosL : POSITION;
    float4 PosH : SV_POSITION;
};

VertexOut VS( VertexIn vin )
{
    VertexOut vout;
    vout.PosL = vin.PosL;
    
    float4 posW = mul(float4(vin.PosL, 1.0f), gRenderItemData.World);
    
    // 令天空球始终位于远平面
    vout.PosH = mul(posW, gPassData.ViewProj).xyww;
    
	return vout;
}

float4 PS(VertexOut pin) : SV_TARGET
{
    return gCubeMap.Sample(gSamLinearWrap, pin.PosL);
}