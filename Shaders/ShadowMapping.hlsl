// ShadowMapping.hlsl
#include "Common.hlsl"


// 输入可以只使用一部分Vertex Buffer中的数据
struct VertexIn
{
    float3 PosL : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    MaterialData matData = gMaterialData[gRenderItemData.MaterialIndex];
	
    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gRenderItemData.World);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gPassData.ViewProj);
	
	// Output vertex attributes for interpolation across triangle.
    float4 texC = mul(float4(vin.TexCoord, 0.0f, 1.0f), gRenderItemData.TexTransform);
    vout.TexCoord = mul(texC, matData.MatTransform).xy;
	
    return vout;
}

void PS(VertexOut pin)
{
    MaterialData matData = gMaterialData[gRenderItemData.MaterialIndex];
#ifdef HAS_MASK_TEXTURE
    float alpha = gTextures[matData.MaskTextureIndex].Sample(gSamPointWrap, pin.TexCoord).r;
    clip(alpha - 0.1);
#endif
}