#pragma once
#include "D3D12App.h"

template <typename T>
class UploadBuffer {
public:
	UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer)
		: mElementCount(elementCount),
		mIsConstantBuffer(isConstantBuffer) {
		mElementByteSize = sizeof(T);
		if (mIsConstantBuffer) {
			UINT alignedSize = 256;
			mElementByteSize = (mElementByteSize + alignedSize - 1) & ~(alignedSize - 1);
		}

		D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(elementCount * mElementByteSize);

		// 创建UploadBuffer资源
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mUploadBuffer)
		));

		// 进行内存映射
		mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedBuffer));
	}

	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
	~UploadBuffer() {
		// 释放映射内存
		if (mUploadBuffer != nullptr) {
			mUploadBuffer->Unmap(0, nullptr);
		}

		mMappedBuffer = nullptr;
	}

	ID3D12Resource* Resource() {
		return mUploadBuffer.Get();
	}

	template<typename Type>
	void Copydata(int elementIndex, const Type& data) {
		std::memcpy(&mMappedBuffer[elementIndex * mElementByteSize], &data, sizeof(Type));
	}

	UINT GetElementSizeInBytes() const {
		return mElementByteSize;
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetBufferPointer() const {
		return mUploadBuffer->GetGPUVirtualAddress();
	}

	UINT GetBufferSize() const {
		return mElementByteSize * mElementCount;
	}

private:
	ComPtr<ID3D12Resource> mUploadBuffer;
	BYTE* mMappedBuffer = nullptr;
	UINT mElementCount = 0;
	UINT mElementByteSize = 0;
	bool mIsConstantBuffer = false;
};