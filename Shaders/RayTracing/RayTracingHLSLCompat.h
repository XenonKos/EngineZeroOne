#ifndef RAYTRACINGHLSLCOMPAT_H
#define RAYTRACINGHLSLCOMPAT_H

#ifdef HLSL
#include "../Shaders/HLSLCompat.hlsli"
#else
#include <DirectXMath.h>
using namespace DirectX;
#endif

#include "Include/Light.h"

struct Ray {
    XMFLOAT3 Origin;
    XMFLOAT3 Direction;
};

struct RadianceRayPayload {
    XMFLOAT4 color;
    UINT recursionDepth;
};

struct ShadowRayPayload {

};

struct Viewport {
	float left, top, right, bottom;
};


struct RayGenConstantBuffer {
	Viewport viewport;
	Viewport Stencil;
};


struct SceneConstantBuffer {
    XMFLOAT4X4 InvViewProj;
    XMFLOAT3 CameraPosW;
    float Padding;
    RectLight light;
};


// Attributes per primitive type.
struct PrimitiveConstantBuffer
{
    XMFLOAT4 albedo;
    float reflectanceCoef;
    float diffuseCoef;
    float specularCoef;
    float specularPower;
    float stepScale;                      // Step scale for ray marching of signed distance primitives. 
                                          // - Some object transformations don't preserve the distances and 
                                          //   thus require shorter steps.
    XMFLOAT3 padding;
};


struct Vertex {
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT3 tangent;
    XMFLOAT2 textureCoordinate;
};

#endif