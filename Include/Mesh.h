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

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "D3D12App.h"
#include "Util.h"
#include "VertexType.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

//struct Vertex {
//	XMFLOAT3 Pos;
//	XMFLOAT2 TexCoord;
//};

struct SubMesh {
	UINT NumVertices = 0;
	UINT NumIndices = 0;

	INT BaseVertexLocation = 0;
	UINT StartIndexLocation = 0;

	D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	UINT MaterialIndex = 0;

	BoundingBox Bounds;
};

class Mesh {
public:
	// 指定Vertex类型为DirectXTK12/VertexTypes中的类型
	using Vertex = VertexPositionNormalTangentTexture;
	Mesh(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> cmdList) 
		: mDevice(device),
		mCommandList(cmdList) {

	}

	void InitFromAssimp(const aiScene* pAiScene) {
		// 确定大小
		unsigned int numSubMeshes = pAiScene->mNumMeshes;
		SubMeshes.resize(numSubMeshes);

		const aiMesh* pAiSubmesh = nullptr;
		// First Loop, construct metadata
		for (unsigned int i = 0; i < numSubMeshes; ++i) {
			pAiSubmesh = pAiScene->mMeshes[i];
			
			// 写入SubMeshes描述符
			SubMeshes[i].NumVertices = pAiSubmesh->mNumVertices;
			SubMeshes[i].NumIndices = pAiSubmesh->mFaces->mNumIndices * pAiSubmesh->mNumFaces;

			SubMeshes[i].StartIndexLocation = NumIndices;
			SubMeshes[i].BaseVertexLocation = NumVertices;

			if (pAiSubmesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) {
				SubMeshes[i].PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			}

			SubMeshes[i].MaterialIndex = pAiSubmesh->mMaterialIndex;

			// 总计Vertex和Index数量
			NumVertices += SubMeshes[i].NumVertices;
			NumIndices += SubMeshes[i].NumIndices;
		}

		VertexBufferSizeInBytes = sizeof(Vertex) * NumVertices;
		IndexBufferSizeInBytes = sizeof(UINT) * NumIndices;

		VertexBufferCPU.resize(NumVertices);
		IndexBufferCPU.resize(NumIndices);

		// Second Loop, copy data
		for (unsigned int i = 0; i < numSubMeshes; ++i) {
			pAiSubmesh = pAiScene->mMeshes[i];

			// Vertex Buffer
			for (unsigned int j = 0, k = SubMeshes[i].BaseVertexLocation; j < SubMeshes[i].NumVertices; ++j, ++k) {
				VertexBufferCPU[k].position = XMFLOAT3(reinterpret_cast<const float*>(&pAiSubmesh->mVertices[j]));
				VertexBufferCPU[k].normal = XMFLOAT3(reinterpret_cast<const float*>(&pAiSubmesh->mNormals[j]));
				VertexBufferCPU[k].tangent = XMFLOAT3(reinterpret_cast<const float*>(&pAiSubmesh->mTangents[j]));
				VertexBufferCPU[k].textureCoordinate = XMFLOAT2(reinterpret_cast<const float*>(&pAiSubmesh->mTextureCoords[0][j]));
			}

			// Index Buffer
			unsigned int idx = SubMeshes[i].StartIndexLocation;
			for (UINT j = 0; j < pAiSubmesh->mNumFaces; ++j) {
				for (UINT k = 0; k < pAiSubmesh->mFaces[j].mNumIndices; ++k) {
					IndexBufferCPU[idx] = pAiSubmesh->mFaces[j].mIndices[k];
					idx++;
				}
			}

		}
		

		// 创建资源
		Util::UploadResource(mDevice.Get(), mCommandList.Get(),
			reinterpret_cast<const void*>(VertexBufferCPU.data()),
			VertexBufferSizeInBytes,
			VertexBufferGPU,
			VertexBufferUploader);

		Util::UploadResource(mDevice.Get(), mCommandList.Get(),
			reinterpret_cast<const void*>(IndexBufferCPU.data()),
			IndexBufferSizeInBytes,
			IndexBufferGPU,
			IndexBufferUploader);
	}


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

	UINT IndexCount() const {
		return NumIndices;
	}

	void GenerateSphere(float radius) {
		UINT numSubdivision = 6u;

		// 构建正十二面体
		const float X = 0.525731f;
		const float Z = 0.850651f;

		XMFLOAT3 pos[12] =
		{
			XMFLOAT3(-X, 0.0f, Z),  XMFLOAT3(X, 0.0f, Z),
			XMFLOAT3(-X, 0.0f, -Z), XMFLOAT3(X, 0.0f, -Z),
			XMFLOAT3(0.0f, Z, X),   XMFLOAT3(0.0f, Z, -X),
			XMFLOAT3(0.0f, -Z, X),  XMFLOAT3(0.0f, -Z, -X),
			XMFLOAT3(Z, X, 0.0f),   XMFLOAT3(-Z, X, 0.0f),
			XMFLOAT3(Z, -X, 0.0f),  XMFLOAT3(-Z, -X, 0.0f)
		};

		UINT k[60] =
		{
			1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
			1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
			3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
			10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
		};

		VertexBufferCPU.resize(12);
		for (UINT i = 0; i < 12; ++i) {
			VertexBufferCPU[i].position = pos[i];
		}
		IndexBufferCPU.assign(&k[0], &k[60]);

		// 细分
		for (UINT i = 0; i < numSubdivision; ++i) {
			Subdivide();
		}

		// 修正细分后的Position
		for (UINT i = 0; i < VertexBufferCPU.size(); ++i) {
			XMStoreFloat3(&VertexBufferCPU[i].position, radius * XMVector3Normalize(XMLoadFloat3(&VertexBufferCPU[i].position)));
		}

		// 填写元数据
		NumVertices = VertexBufferCPU.size();
		NumIndices = IndexBufferCPU.size();
		VertexBufferSizeInBytes = sizeof(Vertex) * NumVertices;
		IndexBufferSizeInBytes = sizeof(UINT) * NumIndices;

		// 创建资源
		Util::UploadResource(mDevice.Get(), mCommandList.Get(),
			reinterpret_cast<const void*>(VertexBufferCPU.data()),
			VertexBufferSizeInBytes,
			VertexBufferGPU,
			VertexBufferUploader);

		Util::UploadResource(mDevice.Get(), mCommandList.Get(),
			reinterpret_cast<const void*>(IndexBufferCPU.data()),
			IndexBufferSizeInBytes,
			IndexBufferGPU,
			IndexBufferUploader);
	}

	// 曲面细分
	void Subdivide() {
		//       v1
		//       *
		//      / \
		//     /   \
		//  m0*-----*m1
		//   / \   / \
		//  /   \ /   \
		// *-----*-----*
		// v0    m2     v2

		// 拷贝一份细分前的Mesh，同时清空当前Mesh
		Mesh inputCopy = *this;
		this->VertexBufferCPU.resize(0);
		this->IndexBufferCPU.resize(0);

		// 对每个三角形依次处理
		UINT numTriangles = inputCopy.IndexBufferCPU.size() / 3;
		for (UINT i = 0; i < numTriangles; ++i) {
			Vertex v0 = inputCopy.VertexBufferCPU[inputCopy.IndexBufferCPU[i * 3 + 0]];
			Vertex v1 = inputCopy.VertexBufferCPU[inputCopy.IndexBufferCPU[i * 3 + 1]];
			Vertex v2 = inputCopy.VertexBufferCPU[inputCopy.IndexBufferCPU[i * 3 + 2]];

			Vertex m0 = MidPoint(v0, v1);
			Vertex m1 = MidPoint(v1, v2);
			Vertex m2 = MidPoint(v0, v2);

			this->VertexBufferCPU.push_back(v0); // 0
			this->VertexBufferCPU.push_back(v1); // 1
			this->VertexBufferCPU.push_back(v2); // 2
			this->VertexBufferCPU.push_back(m0); // 3
			this->VertexBufferCPU.push_back(m1); // 4
			this->VertexBufferCPU.push_back(m2); // 5

			// 左下
			this->IndexBufferCPU.push_back(i * 6 + 0);
			this->IndexBufferCPU.push_back(i * 6 + 3);
			this->IndexBufferCPU.push_back(i * 6 + 5);

			// 中上
			this->IndexBufferCPU.push_back(i * 6 + 3);
			this->IndexBufferCPU.push_back(i * 6 + 1);
			this->IndexBufferCPU.push_back(i * 6 + 4);

			// 右下
			this->IndexBufferCPU.push_back(i * 6 + 5);
			this->IndexBufferCPU.push_back(i * 6 + 4);
			this->IndexBufferCPU.push_back(i * 6 + 2);

			// 中下
			this->IndexBufferCPU.push_back(i * 6 + 3);
			this->IndexBufferCPU.push_back(i * 6 + 4);
			this->IndexBufferCPU.push_back(i * 6 + 5);

		}
	}

	// 中点插值
	Vertex MidPoint(const Vertex& v0, const Vertex& v1) {
		XMVECTOR position0 = XMLoadFloat3(&v0.position);
		XMVECTOR position1 = XMLoadFloat3(&v1.position);

		XMVECTOR normal0 = XMLoadFloat3(&v0.normal);
		XMVECTOR normal1 = XMLoadFloat3(&v1.normal);

		XMVECTOR tangent0 = XMLoadFloat3(&v0.tangent);
		XMVECTOR tangent1 = XMLoadFloat3(&v1.tangent);

		XMVECTOR texCoord0 = XMLoadFloat2(&v0.textureCoordinate);
		XMVECTOR texCoord1 = XMLoadFloat2(&v1.textureCoordinate);

		Vertex ret;
		// 注意单位向量取平均之后模长可能不再为1
		XMStoreFloat3(&ret.position, 0.5 * (position0 + position1));
		XMStoreFloat3(&ret.normal, XMVector3Normalize(0.5 * (normal0 + normal1)));
		XMStoreFloat3(&ret.tangent, XMVector3Normalize(0.5 * (tangent0 + tangent1)));
		XMStoreFloat2(&ret.textureCoordinate, 0.5 * (texCoord0 + texCoord1));

		return ret;
	}

	// 顶点缓冲区及顶点缓冲区资源管理
	std::vector<Vertex> VertexBufferCPU;
	std::vector<UINT> IndexBufferCPU;

	ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

	ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	UINT NumVertices = 0;
	UINT NumIndices = 0;

	UINT VertexBufferSizeInBytes = 0;
	UINT VertexBufferStrideInBytes = sizeof(Vertex);

	UINT IndexBufferSizeInBytes = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R32_UINT;

	std::vector<SubMesh> SubMeshes;

	ComPtr<ID3D12Device> mDevice;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;
};


// Refactoring
struct MeshDescriptor {
	UINT VertexBufferIndex = 0;
	UINT IndexBufferIndex = 0;

	UINT VertexCount = 0;
	UINT IndexCount = 0;

	INT BaseVertexLocation = 0;
	UINT StartIndexLocation = 0;

	//D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
	//D3D12_INDEX_BUFFER_VIEW IndexBufferView;
	D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	UINT MaterialIndex = 0;

	BoundingBox Bounds;
};


class MeshManager {
public:
	using Vertex = VertexPositionNormalTangentTexture;
	MeshManager(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> cmdList)
		: mDevice(device),
		mCommandList(cmdList),
		//mVertexCount(0),
		//mVertexBufferStrideInBytes(sizeof(Vertex)),
		//mVertexBufferSizeInBytes(0),
		//mIndexCount(0),
		mIndexFormat(DXGI_FORMAT_R32_UINT)
		/*mIndexBufferSizeInBytes(0)*/ {

	}

	void ImportMesh(const aiScene* pAiScene);

	UINT MeshCount() const;

	//D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const;
	//D3D12_INDEX_BUFFER_VIEW IndexBufferView() const;

	D3D12_GPU_VIRTUAL_ADDRESS IndexBufferAddress(UINT meshIndex) const;
	UINT IndexCount(UINT meshIndex) const;
	DXGI_FORMAT IndexFormat(UINT meshIndex) const;

	D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE VertexBufferAddressAndStride(UINT meshIndex) const;
	UINT VertexCount(UINT meshIndex) const;
	DXGI_FORMAT VertexPositionFormat(UINT meshIndex) const; // For Ray Tracing

private:
	ComPtr<ID3D12Device> mDevice;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;

	// 顶点缓冲区及顶点缓冲区资源管理
	// 临时缓存空间
	std::vector<Vertex> mVertexBufferCPU;
	std::vector<UINT> mIndexBufferCPU;

	ComPtr<ID3D12Resource> mVertexBufferUploader;
	ComPtr<ID3D12Resource> mIndexBufferUploader;

	// GPU端 VertexBuffer IndexBuffer
	std::vector<VertexBuffer<Vertex>> mVertexBuffer;
	std::vector<IndexBuffer<UINT>> mIndexBuffer;

	//std::vector<ComPtr<ID3D12Resource>> mVertexBufferGPU;
	//std::vector<ComPtr<ID3D12Resource>> mIndexBufferGPU;

	//UINT mVertexCount;
	//UINT mVertexBufferStrideInBytes;
	//UINT mVertexBufferSizeInBytes;

	//UINT mIndexCount;
	DXGI_FORMAT mIndexFormat;
	//UINT mIndexBufferSizeInBytes;

	std::vector<MeshDescriptor> mMeshes;
};


class MeshGenerator {
public:

private:

};