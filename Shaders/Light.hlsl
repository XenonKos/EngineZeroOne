// Light.hlsl
#include "Common.hlsl"

// Vertex Shader Input/Output Format
struct VertexIn
{
    float3 PosL     : SV_POSITION;
    float3 NormalL  : NORMAL;
    float3 TangentU : TANGENT;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
    float4 PosH     : SV_POSITION;
    float3 PosW     : POSITION;
    float3 NormalW  : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexCoord : TEXCOORD0;
    float4 ShadowMapTexCoord : TEXCOORD1;
};

// Vertex Shader
VertexOut VS( VertexIn vin )
{
    VertexOut vout;
    
    MaterialData matData = gMaterialData[gRenderItemData.MaterialIndex];
    
    // World Matrix Transformation
    float4 posW = mul(float4(vin.PosL, 1.0f), gRenderItemData.World);
    vout.PosW = posW.xyz;
    
    // Shadow Mapping
    vout.ShadowMapTexCoord = mul(float4(vout.PosW, 1.0f), gPassData.ShadowTransform);
    
    // Normal Transformation
    vout.NormalW = mul(vin.NormalL, (float3x3) gRenderItemData.World);
    // Tangent Transformation
    vout.TangentW = mul(vin.TangentU, (float3x3) gRenderItemData.World);
    
    // Homogeneous
    vout.PosH = mul(posW, gPassData.ViewProj);
    
    // Texture Transformation
    float4 texC = mul(float4(vin.TexCoord, 0.0f, 1.0f), gRenderItemData.TexTransform);
    vout.TexCoord = mul(texC, matData.MatTransform).xy;
    
    return vout;
}

// Pixel Shader
float4 PS(VertexOut pin) : SV_TARGET
{
    // Normalization
    pin.NormalW = normalize(pin.NormalW);
    
    // Get Material
    MaterialData matData = gMaterialData[gRenderItemData.MaterialIndex];
    
    // Sampling
    // Early Clipping
#ifdef HAS_MASK_TEXTURE
    float alpha = gTextures[matData.MaskTextureIndex].Sample(gSamPointWrap, pin.TexCoord).r;
    clip(alpha - 0.1);
#endif

// Normal and Height 
#ifdef HAS_DIFFUSE_TEXTURE
    float4 diffuseTextureSample = gTextures[matData.DiffuseTextureIndex].Sample(gSamLinearWrap, pin.TexCoord) * matData.DiffuseAlbedo;
    float4 diffuseAlbedo = diffuseTextureSample;
#else
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
#endif
    
#ifdef HAS_NORMAL_TEXTURE
    float3 normalTextureSample = gTextures[matData.NormalTextureIndex].Sample(gSamLinearWrap, pin.TexCoord);
    // OpenGL -> DirectX Normal Format Convert
    normalTextureSample.g = 1.0f - normalTextureSample.g;
    float3 bumpedNormalW = NormalSampleToWorldSpace(normalTextureSample, pin.NormalW, pin.TangentW);
#else
#ifdef HAS_BUMP_TEXTURE
    float mipLevel = gTextures[matData.BumpTextureIndex].CalculateLevelOfDetail(gSamLinearWrap, pin.TexCoord);
    uint lowerMip = (uint) floor(mipLevel), higherMip = (uint) ceil(mipLevel);
    
    uint lowerWidth, lowerHeight, higherWidth, higherHeight, numMips;
    gTextures[matData.BumpTextureIndex].GetDimensions(lowerMip, lowerWidth, lowerHeight, numMips);
    gTextures[matData.BumpTextureIndex].GetDimensions(higherMip, higherWidth, higherHeight, numMips);
    float lowerDx = 1.0f / (float) lowerWidth, lowerDy = 1.0f / (float) lowerHeight;
    float higherDx = 1.0f / (float) higherWidth, higherDy = 1.0f / (float) higherHeight;
    float dx = lerp(lowerDx, higherDx, frac(mipLevel)), dy = lerp(lowerDy, higherDy, frac(mipLevel));
    
    float3 bumpTextureSamples;
    bumpTextureSamples[0] = gTextures[matData.BumpTextureIndex].Sample(gSamLinearWrap, pin.TexCoord);
    bumpTextureSamples[1] = gTextures[matData.BumpTextureIndex].Sample(gSamLinearWrap, pin.TexCoord + float2(dx, 0.0f));
    bumpTextureSamples[2] = gTextures[matData.BumpTextureIndex].Sample(gSamLinearWrap, pin.TexCoord + float2(0.0f, dy));
    
    float3 bumpedNormalW = BumpSampleToNormal(bumpTextureSamples, dx, dy, pin.NormalW, pin.TangentW); 
#else
    float3 bumpedNormalW = pin.NormalW;
#endif
#endif

// Roughness and Glossiness   
#ifdef HAS_ROUGHNESS_TEXTURE
    float roughnessTextureSample = gTextures[matData.RoughnessTextureIndex].Sample(gSamLinearWrap, pin.TexCoord);
    float roughness = roughnessTextureSample;
#else
#ifdef HAS_SHININESS_TEXTURE
    float shininessTextureSample = gTextures[matData.ShininessTextureIndex].Sample(gSamLinearWrap, pin.TexCoord);
    float roughness = 1.0f - shininessTextureSample;
#else
    float roughness = matData.Roughness;
#endif
#endif
    
#ifdef HAS_SPECULAR_TEXTURE
    // sponza场景中的specular贴图为灰度图，一般应为rgb图
    float3 specularTextureSample = gTextures[matData.SpecularTextureIndex].Sample(gSamLinearWrap, pin.TexCoord).xxx;
    float3 fresnelR0 = specularTextureSample;
#else
    float3 fresnelR0 = matData.FresnelR0;
#endif
    
    float3 toCamera = normalize(gPassData.CameraPosW - pin.PosW);
    
    Material mat = { diffuseAlbedo, fresnelR0, roughness };

    float3 shadowFactor = CalcShadowFactor(pin.ShadowMapTexCoord); 
    // Direct and Ambient Lighting
    float4 directLight = ComputeLighting(
        gPassData.Lights,
        mat,
        bumpedNormalW,
        pin.PosW,
        toCamera,
        shadowFactor);
    float4 ambientLight = gPassData.AmbientLightStrength * diffuseAlbedo;

    float4 litColor = directLight + ambientLight;
    litColor.a = diffuseAlbedo.a;
    
    return litColor;
}
