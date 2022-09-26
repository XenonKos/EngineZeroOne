#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>
#include <DirectXCollision.h>
#include "d3dx12.h"

using namespace DirectX;

// 每帧的绘制中GPU所需的资源
class FrameResource {

};

struct RenderItem {
	RenderItem() {

	}

	// 不变信息
	// Mesh
	UINT MeshIndex;
	UINT NumVertices;
	UINT NumIndices;

	INT BaseVertexLocation;
	UINT StartIndexLocation;

	D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// 可变信息（）
	// Render Item Index
	// 存放World Matrix, Texture Transformation Matrix, Material Index
	UINT RenderItemIndex;

	// Dirty位

};