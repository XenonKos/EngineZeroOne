#ifndef CONSTANT_BUFFER_H
#define CONSTANT_BUFFER_H

#include "../Include/Light.h"

#ifdef HLSL
#include "../Shaders/HLSLCompat.hlsli"
#else
#include <DirectXMath.h>
using namespace DirectX;
#endif

struct RenderItemData {
	XMFLOAT4X4 World;
	XMFLOAT4X4 TexTransform;

	UINT MaterialIndex;
};

struct PassData {
	// Camera
	XMFLOAT4X4	View;
	XMFLOAT4X4	InvView;
	XMFLOAT4X4	Proj;
	XMFLOAT4X4	InvProj;
	XMFLOAT4X4	ViewProj;
	XMFLOAT4X4	InvViewProj;
	XMFLOAT3	CameraPosW;
	float		Padding1;
	XMFLOAT2	RenderTargetSize;
	XMFLOAT2	InvRenderTargetSize;
	float		NearZ;
	float		FarZ;

	// Timer
	float		TotalTime;
	float		DeltaTime;

	// Lighting
	Light Lights;
	XMFLOAT4	AmbientLightStrength;

	// Shadow Map
	XMFLOAT4X4 ShadowTransform;
};

struct MaterialData {
	XMFLOAT4	DiffuseAlbedo;
	XMFLOAT3	FresnelR0;
	float		Roughness;
	XMFLOAT4X4	MatTransform;

	UINT DiffuseTextureIndex;
	UINT NormalTextureIndex;
	UINT BumpTextureIndex;
	UINT RoughnessTextureIndex;
	UINT ShininessTextureIndex;
	UINT SpecularTextureIndex;
	UINT MaskTextureIndex;
};

#endif