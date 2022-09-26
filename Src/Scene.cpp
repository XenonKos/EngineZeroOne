#include "Scene.h"

void Scene::Init(ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> cmdList,
	ComPtr<ID3D12DescriptorHeap> srvHeap, UINT srvHeapOffset) {
	mDevice = device;
	mCommandList = cmdList;
	mSrvHeap = srvHeap;
	mSrvHeapOffset = srvHeapOffset;

	BuildConstantBuffer();

	// 创建天空球
	GenerateSkySphere();
}

bool Scene::ImportModel(const std::string& path) {
	// PBRT Format
	if (path.find(".pbrt") != std::string::npos) {
		//ImportPBRT(path);
	}
	// Other Formats
	else {
		ImportAssimp(path);
	}

	return true;
}

bool Scene::LoadCubeMap(const std::string& path)
{
	//if (mRenderItems[EnvironmentMapping].size() == 0) {
	//	// 创建天空球
	//	GenerateSkySphere();
	//}

	// 创建新的Texture及Descriptor
	mTextures.emplace_back(mDevice, mCommandList);
	ID3D12Resource* tex = mTextures.back().LoadTexture(path);

	// CubeMap的Descriptor固定在SRV Heap的开头，类型为D3D12_SRV_DIMENSION_TEXTURECUBE
	CreateShaderResourceView(tex, 0, D3D12_SRV_DIMENSION_TEXTURECUBE);  

	return true;
}

void Scene::SetProperties(const std::string& name, XMFLOAT3 scale, float rotationAngle, XMFLOAT3 rotationAxis, XMFLOAT3 pos) {
	// SRT Matrix
	XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&rotationAxis), rotationAngle);
	XMMATRIX T = XMMatrixTranslation(pos.x, pos.y, pos.z);

	// World Matrix
	XMMATRIX World = XMMatrixTranspose(S * R * T);

	std::vector<UINT>& indexList = mNameIndexMap[name];
	for (int i = 0; i < indexList.size(); ++i) {
		mObjectCBGPU->Copydata(indexList[i], World);
	}
}

UINT Scene::MeshCount() const {
	return mMeshManager->MeshCount();
}

D3D12_GPU_VIRTUAL_ADDRESS Scene::IndexBufferAddress(UINT meshIndex) const {
	return mMeshManager->IndexBufferAddress(meshIndex);
}

UINT Scene::IndexCount(UINT meshIndex) const {
	return mMeshManager->IndexCount(meshIndex);
}

DXGI_FORMAT Scene::IndexFormat(UINT meshIndex) const {
	return mMeshManager->IndexFormat(meshIndex);
}

D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE Scene::VertexBufferAddressAndStride(UINT meshIndex) const {
	return mMeshManager->VertexBufferAddressAndStride(meshIndex);
}

UINT Scene::VertexCount(UINT meshIndex) const {
	return mMeshManager->VertexCount(meshIndex);
}

DXGI_FORMAT Scene::VertexPositionFormat(UINT meshIndex) const {
	return mMeshManager->VertexPositionFormat(meshIndex);
}

void Scene::GenerateSkySphere() {
	const float skySphereRadius = 5000.0f;

	Mesh mesh(mDevice, mCommandList);
	mesh.GenerateSphere(skySphereRadius);
	mMeshes.push_back(std::move(mesh));

	// 设置Mesh信息
	mSkySphere.MeshIndex = mMeshes.size() - 1;
	mSkySphere.NumVertices = mMeshes[mSkySphere.MeshIndex].NumVertices;
	mSkySphere.NumIndices = mMeshes[mSkySphere.MeshIndex].NumIndices;
	mSkySphere.BaseVertexLocation = 0;
	mSkySphere.StartIndexLocation = 0;
	mSkySphere.PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	mSkySphere.RenderItemIndex = mRenderItemNum++;

	// 拷贝Object Constant Buffer
	RenderItemData objectCBCPU;
	objectCBCPU.World = Identity4X4();
	objectCBCPU.TexTransform = Identity4X4();
	objectCBCPU.MaterialIndex = 0;

	mObjectCBGPU->Copydata(mSkySphere.RenderItemIndex, objectCBCPU);

	mNameIndexMap["sky"].push_back(mSkySphere.RenderItemIndex);
}

void Scene::BuildConstantBuffer() {
	mObjectCBGPU = std::make_unique<UploadBuffer<RenderItemData>>(mDevice.Get(), mMaximumItemNum, true);
	mMaterialCBGPU = std::make_unique<UploadBuffer<MaterialData>>(mDevice.Get(), mMaximumItemNum, false);
}

bool Scene::ImportAssimp(const std::string& path) {
	unsigned int flags =
		aiProcess_Triangulate |				// 多边形三角化
		aiProcess_FixInfacingNormals |		// 修正三角形朝向
		aiProcess_JoinIdenticalVertices |	// 去除相同顶点
		aiProcess_CalcTangentSpace |		// 自动为顶点附加Tangent属性
		aiProcess_ConvertToLeftHanded;		// 转换为左手坐标系

	bool ret = false;

	// ReadFile的参数只支持string
	// 这意味着我们需要保持全英文路径
	const aiScene* pAiScene = nullptr;
	try {
		pAiScene = mAiImporter.ReadFile(path, flags);
	}
	catch (std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
	}
	if (pAiScene == nullptr) {
		OutputDebugStringA((LPCSTR)mAiImporter.GetErrorString());
	}
	else {
		ret = InitFromAiScene(pAiScene, path);
	}

	return ret;
}

bool Scene::InitFromAiScene(const aiScene* pAiScene, const std::string& path) {
	// 路径转换，提取父文件夹的绝对路径
	const std::string directory = path.substr(0, path.find_last_of('\\') + 1);

	// 解析模型名称
	const std::string name = path.substr(path.find_last_of('\\') + 1,
		path.find_last_of('.') - path.find_last_of('\\') - 1);

	// 1. 处理网格
	if (pAiScene->HasMeshes()) {
		Mesh mesh(mDevice, mCommandList);
		mesh.InitFromAssimp(pAiScene);
		mMeshes.push_back(std::move(mesh));
	}

	// 记录当前Material列表中的元素数量，以将相对的MaterialIndex转化为绝对的MaterialIndex
	unsigned int baseMaterialIndex = mMaterials.size();

	// 2.处理材质
	// Textures Supported:
	//   [Diffuse Texture]
	//   [Normal Texture]
	//   [Bump Texture]
	//   [Roughness Texture]
	//   [Specular Texture]
	//   [Mask Texture]
	if (pAiScene->HasMaterials()) {
		unsigned int NumMaterials = pAiScene->mNumMaterials;
		for (unsigned int i = 0; i < NumMaterials; ++i) {
			// 创建一种新材质
			Material mat;
			const aiMaterial* pAiMaterial = pAiScene->mMaterials[i];

			// Specular Color
			aiColor3D specular;
			if (pAiMaterial->Get(AI_MATKEY_SPECULAR_FACTOR, specular) == AI_SUCCESS) {
				mat.FresnelR0 = XMFLOAT3(specular.r, specular.g, specular.b);
			}

			// Lambda
			auto LoadTexture = [&](aiTextureType textureType) {
				aiString relativePath;
				pAiMaterial->GetTexture(textureType, 0, &relativePath);

				std::string absolutePath = directory + relativePath.data;

				// 创建新的Texture及Descriptor
				mTextures.emplace_back(mDevice, mCommandList);
				ID3D12Resource* tex = mTextures.back().LoadTexture(absolutePath);
				CreateShaderResourceView(tex, mSrvHeapOffset++);
			};

			// 目前假设每种材质内每种纹理只含一张
			// ----------------------------------- Diffuse Texture -----------------------------------
			if (pAiMaterial->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
				LoadTexture(aiTextureType_DIFFUSE);

				mat.DiffuseTextureIndex = mTextureNum++;
				mat.ItemType |= TextureType::DiffuseTexture;
			}


			// ----------------------------------- Normal Texture -----------------------------------
			if (pAiMaterial->GetTextureCount(aiTextureType_NORMALS) != 0) {
				LoadTexture(aiTextureType_NORMALS);

				mat.NormalTextureIndex = mTextureNum++;
				mat.ItemType |= TextureType::NormalTexture;
			}

			// ----------------------------------- Bump Texture --------------------------------------
			if (pAiMaterial->GetTextureCount(aiTextureType_HEIGHT) != 0) {
				LoadTexture(aiTextureType_HEIGHT);

				mat.BumpTextureIndex = mTextureNum++;
				mat.ItemType |= TextureType::BumpTexture;
			}

			// ----------------------------------- Roughness Texture ---------------------------------
			if (pAiMaterial->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) != 0) {
				LoadTexture(aiTextureType_DIFFUSE_ROUGHNESS);

				mat.RoughnessTextureIndex = mTextureNum++;
				mat.ItemType |= TextureType::RoughnessTexture;
			}

			// ----------------------------------- Shininess Texture ---------------------------------
			if (pAiMaterial->GetTextureCount(aiTextureType_SHININESS) != 0) {
				LoadTexture(aiTextureType_SHININESS);

				mat.RoughnessTextureIndex = mTextureNum++;
				mat.ItemType |= TextureType::RoughnessTexture;
			}

			// ----------------------------------- Specular Texture ---------------------------------
			if (pAiMaterial->GetTextureCount(aiTextureType_SPECULAR) != 0) {
				LoadTexture(aiTextureType_SPECULAR);

				mat.SpecularTextureIndex = mTextureNum++;
				mat.ItemType |= TextureType::SpecularTexture;
			}

			// ----------------------------------- Mask Texture -----------------------------------
			if (pAiMaterial->GetTextureCount(aiTextureType_OPACITY) != 0) {
				LoadTexture(aiTextureType_OPACITY);

				mat.MaskTextureIndex = mTextureNum++;
				mat.ItemType |= TextureType::MaskTexture;
			}

			mMaterials.push_back(mat);


			// 拷贝Material Constant Buffer
			UINT materialIndex = mMaterials.size() - 1;

			MaterialData materialCBCPU;
			materialCBCPU.DiffuseAlbedo = mMaterials[materialIndex].DiffuseAlbedo;
			materialCBCPU.FresnelR0 = mMaterials[materialIndex].FresnelR0;
			materialCBCPU.Roughness = mMaterials[materialIndex].Roughness;
			materialCBCPU.MatTransform = Identity4X4();
			materialCBCPU.DiffuseTextureIndex = mMaterials[materialIndex].DiffuseTextureIndex;
			materialCBCPU.NormalTextureIndex = mMaterials[materialIndex].NormalTextureIndex;
			materialCBCPU.BumpTextureIndex = mMaterials[materialIndex].BumpTextureIndex;
			materialCBCPU.RoughnessTextureIndex = mMaterials[materialIndex].RoughnessTextureIndex;
			materialCBCPU.ShininessTextureIndex = mMaterials[materialIndex].ShininessTextureIndex;
			materialCBCPU.SpecularTextureIndex = mMaterials[materialIndex].SpecularTextureIndex;
			materialCBCPU.MaskTextureIndex = mMaterials[materialIndex].MaskTextureIndex;

			mMaterialCBGPU->Copydata(materialIndex, materialCBCPU);
		}
	}

	// 3.填入RenderItem列表
	std::vector<SubMesh>& submeshes = mMeshes.back().SubMeshes;
	for (unsigned int i = 0; i < submeshes.size(); ++i) {
		RenderItem item;

		// 设置Mesh信息
		item.MeshIndex = mMeshes.size() - 1;
		item.NumVertices = submeshes[i].NumVertices;
		item.NumIndices = submeshes[i].NumIndices;
		item.BaseVertexLocation = submeshes[i].BaseVertexLocation;
		item.StartIndexLocation = submeshes[i].StartIndexLocation;
		item.PrimitiveTopology = submeshes[i].PrimitiveTopology;

		item.RenderItemIndex = mRenderItemNum;
		UINT materialIndex = baseMaterialIndex + submeshes[i].MaterialIndex;

		// 拷贝Object Constant Buffer
		RenderItemData objectCBCPU;
		objectCBCPU.World = Identity4X4();
		objectCBCPU.TexTransform = Identity4X4();
		objectCBCPU.MaterialIndex = materialIndex;

		mObjectCBGPU->Copydata(item.RenderItemIndex, objectCBCPU);

		TextureFlags type = mMaterials[materialIndex].ItemType;
		mRenderItems[type].push_back(item);

		// 填入NameIndexMap
		mNameIndexMap[name].push_back(item.RenderItemIndex);

		// 更新计数
		mRenderItemNum++;
	}

	mModelNum++;
	return true;
}

void Scene::CreateShaderResourceView(ID3D12Resource* tex, UINT srvHeapOffset, D3D12_SRV_DIMENSION viewDimension) {
	static UINT srvSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 创建SRV Descriptor
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = tex != nullptr ? tex->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = viewDimension;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = tex != nullptr ? tex->GetDesc().MipLevels : 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	// 将SRV Descriptor填入SRV Descriptor Heap
	CD3DX12_CPU_DESCRIPTOR_HANDLE descHandle(mSrvHeap->GetCPUDescriptorHandleForHeapStart());
	descHandle.Offset(srvHeapOffset, srvSize);

	mDevice->CreateShaderResourceView(
		tex,
		&srvDesc,
		descHandle
	);

}