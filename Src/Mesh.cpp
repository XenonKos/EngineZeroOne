#include "Mesh.h"

void MeshManager::ImportMesh(const aiScene* pAiScene) {
	// 确定大小
	UINT meshCountBefore = mMeshes.size();
	UINT meshCountIncrement = pAiScene->mNumMeshes;
	UINT meshCountAfter = meshCountBefore + meshCountIncrement;
	mMeshes.resize(meshCountAfter);

	UINT totalVertexCount = 0;
	UINT totalIndexCount = 0;

	const aiMesh* pAiMesh = nullptr;
	// First Loop, construct metadata
	for (UINT i = meshCountBefore; i < meshCountAfter; ++i) {
		pAiMesh = pAiScene->mMeshes[i];

		// 写入SubMeshes描述符
		mMeshes[i].VertexBufferIndex = mVertexBuffer.size();
		mMeshes[i].IndexBufferIndex = mIndexBuffer.size();

		mMeshes[i].VertexCount = pAiMesh->mNumVertices;
		mMeshes[i].IndexCount = pAiMesh->mFaces->mNumIndices * pAiMesh->mNumFaces;

		mMeshes[i].BaseVertexLocation = totalVertexCount;
		mMeshes[i].StartIndexLocation = totalIndexCount;

		if (pAiMesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) {
			mMeshes[i].PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		}

		mMeshes[i].MaterialIndex = pAiMesh->mMaterialIndex;

		// 总计Vertex和Index数量
		totalVertexCount += mMeshes[i].VertexCount;
		totalIndexCount += mMeshes[i].IndexCount;
	}

	// 创建Buffer
	VertexBuffer<Vertex> vertexBuffer(totalVertexCount);
	IndexBuffer<UINT> indexBuffer(totalIndexCount, mIndexFormat);

	mVertexBufferCPU.resize(totalVertexCount);
	mIndexBufferCPU.resize(totalIndexCount);

	// Second Loop, copy data
	for (UINT i = meshCountBefore; i < meshCountAfter; ++i) {
		pAiMesh = pAiScene->mMeshes[i];

		// Vertex Buffer
		for (unsigned int j = 0, k = mMeshes[i].BaseVertexLocation; j < mMeshes[i].VertexCount; ++j, ++k) {
			mVertexBufferCPU[k].position = XMFLOAT3(reinterpret_cast<const float*>(&pAiMesh->mVertices[j]));
			mVertexBufferCPU[k].normal = XMFLOAT3(reinterpret_cast<const float*>(&pAiMesh->mNormals[j]));
			mVertexBufferCPU[k].tangent = XMFLOAT3(reinterpret_cast<const float*>(&pAiMesh->mTangents[j]));
			mVertexBufferCPU[k].textureCoordinate = XMFLOAT2(reinterpret_cast<const float*>(&pAiMesh->mTextureCoords[0][j]));
		}

		// Index Buffer
		unsigned int idx = mMeshes[i].StartIndexLocation;
		for (UINT j = 0; j < pAiMesh->mNumFaces; ++j) {
			for (UINT k = 0; k < pAiMesh->mFaces[j].mNumIndices; ++k) {
				mIndexBufferCPU[idx] = pAiMesh->mFaces[j].mIndices[k];
				idx++;
			}
		}

	}


	// 创建资源
	// TODO WaitFor
	Util::UploadResource(mDevice.Get(), mCommandList.Get(),
		reinterpret_cast<const void*>(mVertexBufferCPU.data()),
		vertexBuffer.SizeInBytes(),
		vertexBuffer.GetBuffer(),
		mVertexBufferUploader);

	Util::UploadResource(mDevice.Get(), mCommandList.Get(),
		reinterpret_cast<const void*>(mIndexBufferCPU.data()),
		indexBuffer.SizeInBytes(),
		indexBuffer.GetBuffer(),
		mIndexBufferUploader);

	mVertexBuffer.emplace_back(vertexBuffer);
	mIndexBuffer.emplace_back(indexBuffer);
}

UINT MeshManager::MeshCount() const {
	return mMeshes.size();
}

//D3D12_VERTEX_BUFFER_VIEW MeshManager::VertexBufferView() const {
//	D3D12_VERTEX_BUFFER_VIEW vbv;
//	vbv.BufferLocation = mVertexBufferGPU->GetGPUVirtualAddress();
//	vbv.SizeInBytes = mVertexBufferSizeInBytes;
//	vbv.StrideInBytes = mVertexBufferStrideInBytes;
//
//	return vbv;
//}
//
//D3D12_INDEX_BUFFER_VIEW MeshManager::IndexBufferView() const {
//	D3D12_INDEX_BUFFER_VIEW ibv;
//	ibv.BufferLocation = mIndexBufferGPU->GetGPUVirtualAddress();
//	ibv.Format = mIndexFormat;
//	ibv.SizeInBytes = mIndexBufferSizeInBytes;
//
//	return ibv;
//}

D3D12_GPU_VIRTUAL_ADDRESS MeshManager::IndexBufferAddress(UINT meshIndex) const {
	D3D12_INDEX_BUFFER_VIEW ibv = mIndexBuffer[mMeshes[meshIndex].IndexBufferIndex].IndexBufferView();
	return ibv.BufferLocation + mMeshes[meshIndex].StartIndexLocation * sizeof(UINT);
}

UINT MeshManager::IndexCount(UINT meshIndex) const {
	return mMeshes[meshIndex].IndexCount;
}

DXGI_FORMAT MeshManager::IndexFormat(UINT meshIndex) const {
	return mIndexFormat;
}

D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE MeshManager::VertexBufferAddressAndStride(UINT meshIndex) const {
	D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE ret = {};
	D3D12_VERTEX_BUFFER_VIEW vbv = mVertexBuffer[mMeshes[meshIndex].VertexBufferIndex].VertexBufferView();

	ret.StartAddress = vbv.BufferLocation + mMeshes[meshIndex].BaseVertexLocation * sizeof(Vertex);
	ret.StrideInBytes = sizeof(Vertex);

	return ret;
}

UINT MeshManager::VertexCount(UINT meshIndex) const {
	return mMeshes[meshIndex].VertexCount;
}

DXGI_FORMAT MeshManager::VertexPositionFormat(UINT meshIndex) const {
	return DXGI_FORMAT_R32G32B32A32_FLOAT;
}
