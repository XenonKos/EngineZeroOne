#pragma once
//#include <wrl.h>
//#include <windows.h>
//#include <windowsx.h>
//using Microsoft::WRL::ComPtr;
#include "D3D12App.h"

#include "DirectXTK12/VertexTypes.h"


struct VertexPositionNormalTangentTexture {
	VertexPositionNormalTangentTexture() = default;

	VertexPositionNormalTangentTexture(const VertexPositionNormalTangentTexture&) = default;
	VertexPositionNormalTangentTexture& operator=(const VertexPositionNormalTangentTexture&) = default;

	VertexPositionNormalTangentTexture(VertexPositionNormalTangentTexture&&) = default;
	VertexPositionNormalTangentTexture& operator=(VertexPositionNormalTangentTexture&&) = default;

	VertexPositionNormalTangentTexture(
		const DirectX::XMFLOAT3& iposition,
		const DirectX::XMFLOAT3& inormal,
		const DirectX::XMFLOAT3& itangent,
		const DirectX::XMFLOAT2& itexcoord
	) noexcept 
		: position(iposition),
		normal(inormal),
		tangent(itangent),
		textureCoordinate(itexcoord) 
	{}

	VertexPositionNormalTangentTexture(
		DirectX::FXMVECTOR iposition,
		DirectX::FXMVECTOR inormal,
		DirectX::FXMVECTOR itangent,
		DirectX::CXMVECTOR itexcoord
	) noexcept {
		DirectX::XMStoreFloat3(&this->position, iposition);
		DirectX::XMStoreFloat3(&this->normal, inormal);
		DirectX::XMStoreFloat3(&this->tangent, itangent);
		DirectX::XMStoreFloat2(&this->textureCoordinate, itexcoord);
	}

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT3 tangent;
	DirectX::XMFLOAT2 textureCoordinate;

	static const D3D12_INPUT_LAYOUT_DESC InputLayout;

private:
	static constexpr unsigned int InputElementCount = 4;
	static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};


template <typename VertexType>
class VertexBuffer {
public:
	VertexBuffer(UINT vertexCount);
	ComPtr<ID3D12Resource>& GetBuffer();
	UINT SizeInBytes() const;
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const;

private:
	UINT mVertexCount;
	UINT mVertexBufferStrideInBytes;
	UINT mVertexBufferSizeInBytes;

	ComPtr<ID3D12Resource> mVertexBufferGPU;
};


template <typename IndexType>
class IndexBuffer {
public:
	IndexBuffer(UINT indexCount, DXGI_FORMAT indexFormat);
	ComPtr<ID3D12Resource>& GetBuffer();
	UINT SizeInBytes() const;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView() const;

private:
	UINT mIndexCount;
	DXGI_FORMAT mIndexFormat;
	UINT mIndexBufferSizeInBytes;

	ComPtr<ID3D12Resource> mIndexBufferGPU;
};

template <typename VertexType>
VertexBuffer<VertexType>::VertexBuffer(UINT vertexCount)
	: mVertexCount(vertexCount),
	mVertexBufferStrideInBytes(sizeof(VertexType)),
	mVertexBufferSizeInBytes(vertexCount * sizeof(VertexType)) {

}

template <typename VertexType>
ComPtr<ID3D12Resource>& VertexBuffer<VertexType>::GetBuffer() {
	return mVertexBufferGPU;
}

template <typename VertexType>
D3D12_VERTEX_BUFFER_VIEW VertexBuffer<VertexType>::VertexBufferView() const {
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = mVertexBufferGPU->GetGPUVirtualAddress();
	vbv.SizeInBytes = mVertexBufferSizeInBytes;
	vbv.StrideInBytes = mVertexBufferStrideInBytes;

	return vbv;
}

template <typename VertexType>
UINT VertexBuffer<VertexType>::SizeInBytes() const {
	return mVertexBufferSizeInBytes;
}

template <typename IndexType>
IndexBuffer<IndexType>::IndexBuffer(UINT indexCount, DXGI_FORMAT indexFormat)
	: mIndexCount(indexCount),
	mIndexFormat(indexFormat),
	mIndexBufferSizeInBytes(indexCount * sizeof(IndexType)) {

}

template <typename IndexType>
ComPtr<ID3D12Resource>& IndexBuffer<IndexType>::GetBuffer() {
	return mIndexBufferGPU;
}

template <typename IndexType>
UINT IndexBuffer<IndexType>::SizeInBytes() const {
	return mIndexBufferSizeInBytes;
}


template <typename IndexType>
D3D12_INDEX_BUFFER_VIEW IndexBuffer<IndexType>::IndexBufferView() const {
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = mIndexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = mIndexFormat;
	ibv.SizeInBytes = mIndexBufferSizeInBytes;

	return ibv;
}