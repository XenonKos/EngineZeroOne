#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>
#include <DirectXCollision.h>
#include "d3dx12.h"

#include <wrl.h>
#include <string>
#include <unordered_map>

#include "D3D12App.h"

using namespace DirectX;
using namespace Microsoft::WRL;

// MeshGeometry可以将多个几何体数据打包到一个顶点缓冲区和一个索引缓冲区中，
// 每个几何体由SubMeshGeometry来指示
struct SubMeshGeometry {
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

	BoundingBox Bounds;
};

struct MeshGeometry {
	std::string Name;

	// 顶点缓冲区及顶点缓冲区资源管理
	ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
	ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

	ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

	ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	UINT VertexBufferSizeInBytes = 0;
	UINT VertexBufferStrideInBytes = 0;

	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
	UINT IndexBufferSizeInBytes = 0;

	std::unordered_map<std::string, SubMeshGeometry> DrawArgs;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const {
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
		vbv.SizeInBytes = VertexBufferSizeInBytes;
		vbv.StrideInBytes = VertexBufferStrideInBytes;

		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW IndexBufferView() const {
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = IndexFormat;
		ibv.SizeInBytes = IndexBufferSizeInBytes;

		return ibv;
	}

	// 还需导入Device和Command List进行操作
	void UploadResource(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
		const void* initData, UINT64 byteSize, 
		ComPtr<ID3D12Resource>& defaultBuffer,
		ComPtr<ID3D12Resource>& uploadBuffer) {

		// 资源创建相关参数
		D3D12_HEAP_PROPERTIES defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		D3D12_HEAP_PROPERTIES uploadHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

		// 创建Default Buffer资源
		ThrowIfFailed(device->CreateCommittedResource(
			&defaultHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(defaultBuffer.GetAddressOf())
		));

		// 创建Upload Buffer资源
		ThrowIfFailed(device->CreateCommittedResource(
			&uploadHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(uploadBuffer.GetAddressOf())
		));

		// 描述待拷贝的资源
		D3D12_SUBRESOURCE_DATA subResourceData;
		subResourceData.pData = initData;
		subResourceData.RowPitch = byteSize;
		subResourceData.SlicePitch = subResourceData.RowPitch;

		// 利用Upload Buffer作为中介进行资源上传
		// 这一过程中需要对Default Buffer的状态做转换
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST
		);
		cmdList->ResourceBarrier(1, &barrier);

		UpdateSubresources<1>(cmdList,
			defaultBuffer.Get(),	// Destination
			uploadBuffer.Get(),		// Intermediate
			0, 0, 1, &subResourceData);

		D3D12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
			defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ
		);
		cmdList->ResourceBarrier(1, &barrier2);

	}
};