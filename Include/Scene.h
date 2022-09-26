#pragma once
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/Exceptional.h" // Assimp Exceptions

#include <DirectXTK12/ResourceUploadBatch.h> // texture uploading

#include "D3D12App.h"
#include "Mesh.h"
#include "Texture.h"
#include "Material.h"
#include "ConstantBuffer.h"
#include "FrameResource.h"
#include "UploadBuffer.h"


	

// Scene代表当前显示的场景，我们可以不断地向场景中添加资产，
// 或是进行场景切换，
// Scene的资源交由Mesh和Texture管理
class Scene {
public:
	Scene() {

		// Initialization For WICTextureLoader.
		ThrowIfFailed(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED));
	}

	void Init(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> cmdList,
		ComPtr<ID3D12DescriptorHeap> srvHeap, UINT srvHeapOffset);

	bool ImportModel(const std::string& path);
	bool LoadCubeMap(const std::string& path);

	void SetProperties(const std::string& name,
		XMFLOAT3 scale,
		float rotationAngle, XMFLOAT3 rotationAxis,
		XMFLOAT3 pos = XMFLOAT3(0.0f, 0.0f, 0.0f));

	// Mesh MetaData Getters
	UINT MeshCount() const;

	D3D12_GPU_VIRTUAL_ADDRESS IndexBufferAddress(UINT meshIndex) const;
	UINT IndexCount(UINT meshIndex) const;
	DXGI_FORMAT IndexFormat(UINT meshIndex) const;

	D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE VertexBufferAddressAndStride(UINT meshIndex) const;
	UINT VertexCount(UINT meshIndex) const;
	DXGI_FORMAT VertexPositionFormat(UINT meshIndex) const; // For Ray Tracing

public:
	// 资源列表
	std::vector<Mesh> mMeshes;
	std::vector<Texture> mTextures;
	std::vector<Material> mMaterials;

	// 重新设计
	// SRV Heap的管理交由外部类
	ComPtr<ID3D12DescriptorHeap> mSrvHeap;
	UINT mSrvHeapOffset = 0;

	static const UINT mMaxTextureNum = 128;
	UINT mTextureNum = 0;

	// Render Item管理
	// 为减少PSO切换次数，使用相同Shader的Render Item将被放在一起
	std::unordered_map<TextureFlags, std::vector<RenderItem>> mRenderItems;
	std::unordered_map<std::string, std::vector<UINT>> mNameIndexMap;

	// 目前只能静态地确定资源的最大可容纳大小，不能动态地改变
	static const UINT mMaximumItemNum = 512;
	UINT mRenderItemNum = 0;
	UINT mModelNum = 0;

	// GPU侧的Constant Buffer
	// RenderItemData: 其中包括
	std::unique_ptr<UploadBuffer<RenderItemData>> mObjectCBGPU;
	std::unique_ptr<UploadBuffer<MaterialData>> mMaterialCBGPU;

	// 天空球
	RenderItem mSkySphere;

private:
	void BuildConstantBuffer();

	void GenerateSkySphere();

	bool ImportAssimp(const std::string& path);

	bool InitFromAiScene(const aiScene* pAiScene, const std::string& path);

	void CreateShaderResourceView(ID3D12Resource* tex, UINT srvHeapOffset, D3D12_SRV_DIMENSION viewDimension = D3D12_SRV_DIMENSION_TEXTURE2D);

	ComPtr<ID3D12Device> mDevice;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;
	Assimp::Importer mAiImporter;

	// 资源列表2.0
	std::unique_ptr<MeshManager> mMeshManager;
};