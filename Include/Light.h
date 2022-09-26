#ifndef LIGHT_H
#define LIGHT_H

#ifdef HLSL
#include "Shaders/HLSLCompat.hlsli"
#else
#include <DirectXMath.h>
using namespace DirectX;
#endif

// Maximum Lights Amount
#define MAX_DIRECTIONAL_LIGHT 16
#define MAX_POINT_LIGHT 16
#define MAX_RECT_LIGHT 16
#define MAX_SPOT_LIGHT 16 

struct DirectionalLight {
	XMFLOAT3	Strength;
	float		Padding1;
	XMFLOAT3	Direction;
	float		Padding2;
};

struct PointLight {
	XMFLOAT3	Strength;
	float		AttenuationRange;
	XMFLOAT3	Position;
	float		Padding;
};

struct RectLight {
	XMFLOAT3	Strength;
	float		AttenuationRange;
	XMFLOAT3	Direction;
	float		Padding;
	XMFLOAT3	Position;
	float		Padding2;
	XMFLOAT2	Rect;
	XMFLOAT2	Padding3;
};

struct SpotLight {
	XMFLOAT3	Strength;
	float		AttenuationRange;
	XMFLOAT3	Direction;
	float		MaxAngle;
	XMFLOAT3	Position;
	float		Padding;
};

struct Light {
	DirectionalLight    DirectionalLights[MAX_DIRECTIONAL_LIGHT];
	PointLight          PointLights[MAX_POINT_LIGHT];
	RectLight			RectLights[MAX_RECT_LIGHT];
	SpotLight           SpotLights[MAX_SPOT_LIGHT];
	UINT        NumDirectionalLights;
	UINT        NumPointLights;
	UINT		NumRectLights;
	UINT        NumSpotLights;
};

#endif