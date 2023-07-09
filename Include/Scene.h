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


	

// Scene����ǰ��ʾ�ĳ��������ǿ��Բ��ϵ��򳡾�������ʲ���
// ���ǽ��г����л���
// Scene����Դ����Mesh��Texture����
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
	// ��Դ�б�
	std::vector<Mesh> mMeshes;
	std::vector<Texture> mTextures;
	std::vector<Material> mMaterials;

	// �������
	// SRV Heap�Ĺ������ⲿ��
	ComPtr<ID3D12DescriptorHeap> mSrvHeap;
	UINT mSrvHeapOffset = 0;

	static const UINT mMaxTextureNum = 128;
	UINT mTextureNum = 0;

	// Render Item����
	// Ϊ����PSO�л�������ʹ����ͬShader��Render Item��������һ��
	std::unordered_map<TextureFlags, std::vector<RenderItem>> mRenderItems;
	std::unordered_map<std::string, std::vector<UINT>> mNameIndexMap;

	// Ŀǰֻ�ܾ�̬��ȷ����Դ���������ɴ�С�����ܶ�̬�ظı�
	static const UINT mMaximumItemNum = 512;
	UINT mRenderItemNum = 0;
	UINT mModelNum = 0;

	// GPU���Constant Buffer
	// RenderItemData: ���а���
	std::unique_ptr<UploadBuffer<RenderItemData>> mObjectCBGPU;
	std::unique_ptr<UploadBuffer<MaterialData>> mMaterialCBGPU;

	// �����
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

	// ��Դ�б�2.0
	std::unique_ptr<MeshManager> mMeshManager;
};