#pragma once
#include <DirectXMath.h>
using namespace DirectX;

using TextureFlags = UINT;

enum TextureType {
	DiffuseTexture		= 1 << 0,
	NormalTexture		= 1 << 1,
	RoughnessTexture	= 1 << 2,
	ShininessTexture	= 1 << 3,
	SpecularTexture		= 1 << 4,
	EnvironmentMapping	= 1 << 5,
	BumpTexture			= 1 << 6,
	MaskTexture			= 1 << 7,
};

class Material {
public:
	Material() 
		: DiffuseAlbedo(1.0f, 1.0f, 1.0f, 1.0f),
		FresnelR0(0.05f, 0.05f, 0.05f),
		Roughness(0.3f),
		ItemType(0) {

	}

	XMFLOAT4 DiffuseAlbedo;
	XMFLOAT3 FresnelR0;
	float Roughness;

	UINT DiffuseTextureIndex;
	UINT NormalTextureIndex;
	UINT BumpTextureIndex;
	UINT RoughnessTextureIndex;
	UINT ShininessTextureIndex;
	UINT SpecularTextureIndex;
	UINT MaskTextureIndex;

	TextureFlags ItemType;
};