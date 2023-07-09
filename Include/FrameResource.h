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

// ÿ֡�Ļ�����GPU�������Դ
class FrameResource {

};

struct RenderItem {
	RenderItem() {

	}

	// ������Ϣ
	// Mesh
	UINT MeshIndex;
	UINT NumVertices;
	UINT NumIndices;

	INT BaseVertexLocation;
	UINT StartIndexLocation;

	D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// �ɱ���Ϣ����
	// Render Item Index
	// ���World Matrix, Texture Transformation Matrix, Material Index
	UINT RenderItemIndex;

	// Dirtyλ

};