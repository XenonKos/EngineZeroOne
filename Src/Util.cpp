#include "Util.h"

void Util::UploadResource(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
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


void Util::UploadTexture2DResource(
	ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, 
	const ScratchImage* scratchImage,
	ComPtr<ID3D12Resource>& defaultBuffer, 
	ComPtr<ID3D12Resource>& uploadBuffer) {
	// Metadata
	TexMetadata metadata = scratchImage->GetMetadata();

	// 资源创建相关参数
	D3D12_HEAP_PROPERTIES defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_HEAP_PROPERTIES uploadHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	D3D12_RESOURCE_DESC textureDesc = 
		CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels);
	// 创建Default Buffer资源
	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf())
	));

	// 用于Upload Buffer
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(defaultBuffer.Get(), 0, metadata.mipLevels);
	D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
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
	//D3D12_SUBRESOURCE_DATA subResourceData;
	//const Image* image = scratchImage->GetImage(0, 0, 0);
	//subResourceData.pData = image->pixels;
	//subResourceData.RowPitch = image->rowPitch;
	//subResourceData.SlicePitch = image->slicePitch;
	std::array<D3D12_SUBRESOURCE_DATA, 16> subResourceDatas;
	for (int mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
		const Image* image = scratchImage->GetImage(mipLevel, 0, 0);
		subResourceDatas[mipLevel].pData = image->pixels;
		subResourceDatas[mipLevel].RowPitch = image->rowPitch;
		subResourceDatas[mipLevel].SlicePitch = image->slicePitch;
	}

	// 利用Upload Buffer作为中介进行资源上传
	// 这一过程中需要对Default Buffer的状态做转换
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_COPY_DEST
	);
	cmdList->ResourceBarrier(1, &barrier);

	UpdateSubresources<16>(cmdList,
		defaultBuffer.Get(),	// Destination
		uploadBuffer.Get(),		// Intermediate
		0, 
		0, 
		metadata.mipLevels, 
		subResourceDatas.data()
	);

	D3D12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
		defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE // 纹理对应的Resource State
	);
	cmdList->ResourceBarrier(1, &barrier2);
}

void Util::UploadTextureCubeResource(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
	const ScratchImage* scratchImage,
	ComPtr<ID3D12Resource>& defaultBuffer,
	ComPtr<ID3D12Resource>& uploadBuffer) {
	// Metadata
	TexMetadata metadata = scratchImage->GetMetadata();

	// 资源创建相关参数
	D3D12_HEAP_PROPERTIES defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_HEAP_PROPERTIES uploadHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// DepthOrArraySize设置为6
	D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, metadata.width, metadata.height, metadata.arraySize, 1);
	// 创建Default Buffer资源
	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf())
	));

	// 计算UploadBuffer所需的缓冲区大小
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(defaultBuffer.Get(), 0, 6);
	D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	// 创建Upload Buffer资源
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf())
	));

	// 利用Upload Buffer作为中介进行资源上传
	// 这一过程中需要对Default Buffer的状态做转换
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_COPY_DEST
	);
	cmdList->ResourceBarrier(1, &barrier);

	// 描述待拷贝的资源
	std::array<D3D12_SUBRESOURCE_DATA, 6> subResourceDatas;
	for (int i = 0; i < 6; ++i) {
		const Image* image = scratchImage->GetImage(0, i, 0);
		subResourceDatas[i].pData = image->pixels;
		subResourceDatas[i].RowPitch = image->rowPitch;
		subResourceDatas[i].SlicePitch = image->slicePitch;
	}

	UpdateSubresources<6>(cmdList,
		defaultBuffer.Get(),	// Destination
		uploadBuffer.Get(),		// Intermediate
		0,						// IntermediateOffset
		0,						// FirstSubresource
		6,						// NumSubresources
		subResourceDatas.data() // pSubresourcedata
	);

	D3D12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
		defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE // 纹理对应的Resource State
	);
	cmdList->ResourceBarrier(1, &barrier2);

}

void Util::AllocateUAVBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, 
	UINT64 byteSize, 
	D3D12_RESOURCE_STATES initialState,
	ComPtr<ID3D12Resource>& defaultBuffer) {
	D3D12_HEAP_PROPERTIES defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		initialState,
		nullptr,
		IID_PPV_ARGS(&defaultBuffer)
	));
}
