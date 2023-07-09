#include "SceneApp.h"

#include <fstream>
#include <sstream>

SceneApp::SceneApp(HINSTANCE hInstance)
	: D3D12App(hInstance),
	mCamera(XMFLOAT3(0.0f, 1.3f, -2.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)) {
	//mCamera(XMFLOAT3(1.0f, 1.3f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)) {

}

SceneApp::~SceneApp() {

}

bool SceneApp::Init() {
	if (!D3D12App::Init()) {
		return false;
	}

	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	ConfigLights();

	BuildConstantBuffer();
	BuildRootSignature();
	BuildShadowMap();

	BuildSrvHeap();

	// ����Scene
	// SRV Heapǰ����ΪEnvironment Mapping��Shadow Mapping
	mScene.Init(mDevice, mCommandList, mSrvHeap, 2);

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	return true;
}

void SceneApp::LoadModel(const std::string& path) {
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	mScene.ImportModel(path);

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();
}

void SceneApp::LoadCubeMap(const std::string& path) {
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	mScene.LoadCubeMap(path);

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();
}

void SceneApp::ConfigLights() {
	// Directional Lights
	mLights.NumDirectionalLights = 0;

	mLights.DirectionalLights[0].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mLights.DirectionalLights[0].Strength = { 1.0f, 1.0f, 1.0f };
	mLights.DirectionalLights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mLights.DirectionalLights[1].Strength = { 0.8f, 0.8f, 0.8f };
	mLights.DirectionalLights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mLights.DirectionalLights[2].Strength = { 0.15f, 0.15f, 0.15f };

	// Point Lights
	mLights.NumPointLights = 0;
	
	// Spot Lights
	mLights.NumSpotLights = 0;

	// Rect Lights
	// ���
	mLights.NumRectLights = 1;
	mLights.RectLights[0].Direction = { 1.0f, 0.0f, 0.0f };
	mLights.RectLights[0].Position = { -1.0f, 1.3f, 0.0f };
	mLights.RectLights[0].Strength = { 1.0f, 1.0f, 1.0f };
	mLights.RectLights[0].AttenuationRange = 1000.0f;

	// Ambient Lighting
	mAmbientLightStrength = { 0.25f, 0.25f, 0.35f, 1.0f };
}

void SceneApp::BuildConstantBuffer() {
	// ����UploadBuffer

	//mObjectCBGPU = std::make_unique<UploadBuffer<cbPerObject>>(mDevice.Get(), maximumItemNum, true);
	//mMaterialCBGPU = std::make_unique<UploadBuffer<cbMaterial>>(mDevice.Get(), maximumItemNum, true);
	
	// 0: PassCB, 1: Shadow Map PassCB
	mPassCBGPU = std::make_unique<UploadBuffer<PassData>>(mDevice.Get(), 2, true);

}

void SceneApp::BuildShaders(PipelineStateFlags pipelineStateFlags) {
	// �������������߱���HLSL�ļ���ErrorMsgs�ܼ���ذ������ǽ���Debug
	HRESULT hr = S_OK;
	ComPtr<ID3DBlob> ErrorMsgs;

	// �˴������˼·�ǣ�
	// ���ݼ���ģ��ʱ�Ƿ��������Դ��������صĺ꣬Ȼ����Щ�긽��Shader����Ŀ�ͷλ��
	// ������ֻ��������ʱ����Shader����

	// Ӳ����·��
	std::string shaderPath = "C:\\Users\\Lenovo\\Desktop\\EngineZeroOne\\Shaders\\Light.hlsl";
	if (pipelineStateFlags & ShadowMapping) {
		shaderPath = "C:\\Users\\Lenovo\\Desktop\\EngineZeroOne\\Shaders\\ShadowMapping.hlsl";
	}
	if (pipelineStateFlags & EnvironmentMapping) {
		shaderPath = "C:\\Users\\Lenovo\\Desktop\\EngineZeroOne\\Shaders\\EnvironmentMapping.hlsl";
	}
	std::ifstream file(shaderPath);
	std::stringstream buffer;
	buffer << file.rdbuf();

	std::string macros;
	if (pipelineStateFlags & DiffuseTexture) {
		macros += "#define HAS_DIFFUSE_TEXTURE\n";
	}
	if (pipelineStateFlags & NormalTexture) {
		macros += "#define HAS_NORMAL_TEXTURE\n";
	}
	if (pipelineStateFlags & BumpTexture) {
		macros += "#define HAS_BUMP_TEXTURE\n";
	}
	if (pipelineStateFlags & RoughnessTexture) {
		macros += "#define HAS_ROUGHNESS_TEXTURE\n";
	}
	if (pipelineStateFlags & ShininessTexture) {
		macros += "#define HAS_SHININESS_TEXTURE\n";
	}
	if (pipelineStateFlags & SpecularTexture) {
		macros += "#define HAS_SPECULAR_TEXTURE\n";
	}
	if (pipelineStateFlags & MaskTexture) {
		macros += "#define HAS_MASK_TEXTURE\n";
	}

	std::string shader = macros + buffer.str();

	// Vertex Shader
	hr = D3DCompile(
		shader.c_str(),
		shader.length(),
		shaderPath.c_str(), // Ϊ�˽���#include�ļ������·��
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VS",
		"vs_5_1",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		mVsByteCode[pipelineStateFlags].GetAddressOf(),
		ErrorMsgs.GetAddressOf()
	);

	if (ErrorMsgs != nullptr) {
		OutputDebugStringA((LPCSTR)(ErrorMsgs->GetBufferPointer()));
	}

	ThrowIfFailed(hr);

	// Pixel Shader
	hr = D3DCompile(
		shader.c_str(),
		shader.length(),
		shaderPath.c_str(),
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"PS",
		"ps_5_1",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		mPsByteCode[pipelineStateFlags].GetAddressOf(),
		ErrorMsgs.GetAddressOf()
	);

	if (ErrorMsgs != nullptr) {
		OutputDebugStringA((LPCSTR)(ErrorMsgs->GetBufferPointer()));
	}

	ThrowIfFailed(hr);

	//hr = D3DCompileFromFile(
	//	mShaderPath.c_str(),
	//	nullptr,
	//	D3D_COMPILE_STANDARD_FILE_INCLUDE,
	//	"VS",
	//	"vs_5_1",
	//	D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
	//	0,
	//	mVsByteCode.GetAddressOf(),
	//	ErrorMsgs.GetAddressOf()
	//);

	//if (ErrorMsgs != nullptr) {
	//	OutputDebugStringA((LPCSTR)(ErrorMsgs->GetBufferPointer()));
	//}

	//ThrowIfFailed(hr);

	//hr = D3DCompileFromFile(
	//	mShaderPath.c_str(),
	//	nullptr,
	//	D3D_COMPILE_STANDARD_FILE_INCLUDE,
	//	"PS",
	//	"ps_5_1",
	//	D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
	//	0,
	//	mPsByteCode.GetAddressOf(),
	//	ErrorMsgs.GetAddressOf()
	//);

	//if (ErrorMsgs != nullptr) {
	//	OutputDebugStringA((LPCSTR)(ErrorMsgs->GetBufferPointer()));
	//}

	//ThrowIfFailed(hr);
}

void SceneApp::BuildRootSignature() {
	// �����������б�
	CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignatureParameter::ParameterCount];
	
	// Parameter[0]: PerObjectCB
	// Parameter[1]: PerPassCB
	// Parameter[2]: MaterialCB
	// Parameter[3]: TextureTable
	slotRootParameter[RootSignatureParameter::PerObjectCB].InitAsConstantBufferView(0);
	slotRootParameter[RootSignatureParameter::PerPassCB].InitAsConstantBufferView(1);

	// MaterialCB
	slotRootParameter[RootSignatureParameter::MaterialCB].InitAsShaderResourceView(0, 1);

	// TextureTable
	CD3DX12_DESCRIPTOR_RANGE srvTable;
	UINT textureNum = mScene.mMaxTextureNum; // �̶���������
	srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2 + textureNum, 0, 0);
	slotRootParameter[RootSignatureParameter::TextureTable].InitAsDescriptorTable(1, &srvTable, D3D12_SHADER_VISIBILITY_PIXEL);


	// ����Static Samplers�б�
	auto staticSamplers = GetStaticSamplers();
	UINT numStaticSamplers = staticSamplers.size();
	const D3D12_STATIC_SAMPLER_DESC* pStaticSamplerDesc = staticSamplers.data();

	// ���л�������
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		ARRAYSIZE(slotRootParameter),
		slotRootParameter,
		numStaticSamplers, 
		pStaticSamplerDesc,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	HRESULT hr = S_OK;
	ComPtr<ID3DBlob> serializedRootSig;
	ComPtr<ID3DBlob> ErrorMsgs;
	hr = D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		ErrorMsgs.GetAddressOf()
	);

	if (ErrorMsgs != nullptr) {
		OutputDebugStringA((LPCSTR)(ErrorMsgs->GetBufferPointer()));
	}

	ThrowIfFailed(hr);

	// ������ǩ��
	ThrowIfFailed(mDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)
	));
}

void SceneApp::BuildShadowMap() {
	// Ŀǰ�����賡����ֻ��һ�����Դ
	mShadowMap = std::make_unique<ShadowMap<RectLight>>(mDevice.Get(), 2048, 2048, &mLights.RectLights[0]);
}

void SceneApp::BuildSrvHeap() {
	// SRV Heap
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.NumDescriptors = 128;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NodeMask = 0;

	ThrowIfFailed(mDevice->CreateDescriptorHeap(
		&srvHeapDesc,
		IID_PPV_ARGS(&mSrvHeap)
	));

	// SRV
	auto srvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC environmentMapDesc = {};
	environmentMapDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	environmentMapDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	environmentMapDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	environmentMapDesc.Texture2D.MostDetailedMip = 0;
	environmentMapDesc.Texture2D.MipLevels = 1;
	environmentMapDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	// Environment Cube Mapռλ��
	mDevice->CreateShaderResourceView(
		nullptr,
		&environmentMapDesc,
		srvHandle
	);

	srvHandle.Offset(1, mCbvSrvUavDescriptorSize);

	// �˴���FormatҲ��ShadowMap��Format��ͬ
	D3D12_SHADER_RESOURCE_VIEW_DESC shadowMapDesc = {};
	shadowMapDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	shadowMapDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	shadowMapDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	shadowMapDesc.Texture2D.MostDetailedMip = 0;
	shadowMapDesc.Texture2D.MipLevels = 1;
	shadowMapDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	shadowMapDesc.Texture2D.PlaneSlice = 0;

	mDevice->CreateShaderResourceView(
		mShadowMap->Resource(),
		&shadowMapDesc,
		srvHandle
	);
}

void SceneApp::BuildPSO(PipelineStateFlags pipelineStateFlags) {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	psoDesc.InputLayout = VertexPositionNormalTangentTexture::InputLayout;
	psoDesc.pRootSignature = mRootSignature.Get();

	// �����ڶ�ӦtexFlags��Shader���룬���ֳ�����
	if (mVsByteCode.count(pipelineStateFlags) == 0) {
		BuildShaders(pipelineStateFlags);
	}

	psoDesc.VS = {
		reinterpret_cast<BYTE*>(mVsByteCode[pipelineStateFlags]->GetBufferPointer()),
		mVsByteCode[pipelineStateFlags]->GetBufferSize()
	};
	psoDesc.PS = {
		reinterpret_cast<BYTE*>(mPsByteCode[pipelineStateFlags]->GetBufferPointer()),
		mPsByteCode[pipelineStateFlags]->GetBufferSize()
	};

	// Configuration
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	psoDesc.SampleMask = UINT_MAX;
	// �˴���Swap Chain�����ñ���һ��
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;

	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.NumRenderTargets = 1;
	psoDesc.DSVFormat = mDepthStencilFormat;
	
	// -----------------------------------Specialization--------------------------------------------
	if (pipelineStateFlags & WireFrame) {
		// Fill Mode
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	}
	if (pipelineStateFlags & MSAA) {
		// 4X MSAA
		psoDesc.SampleDesc.Count = 4;
		psoDesc.SampleDesc.Quality = mMsaaQuality - 1;
	}
	if (pipelineStateFlags & ShadowMapping) {
		// Shadow Map Bias
		psoDesc.RasterizerState.DepthBias = 10000;
		psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
		psoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;

		// No Render Targets
		psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
		psoDesc.NumRenderTargets = 0;
		psoDesc.DSVFormat = DXGI_FORMAT_R24G8_TYPELESS;
	}
	if (pipelineStateFlags & EnvironmentMapping) {
		// �����λ��������ڣ����ñ����޳�
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

		// ��Ȳ��Ժ�������LESS_EQUAL
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	}

	if (pipelineStateFlags & MaskTexture) {
		// ҶƬ��ģ�ͣ����ñ����޳�
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	}

	// ����PSO
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(
		&psoDesc,
		IID_PPV_ARGS(&mPSOs[pipelineStateFlags])
	));

}

void SceneApp::OnResize() {
	D3D12App::OnResize();

	mCamera.SetLens(mClientWidth, mClientHeight);
	mCamera.UpdateViewMatrix();
	mCamera.UpdateProjectionMatrix();
}

void SceneApp::Update(const GameTimer& gt) {

	UpdateRenderItemCB(gt);
	UpdatePassCB(gt);
}

void SceneApp::UpdateRenderItemCB(const GameTimer& gt) {
	mScene.SetProperties("marble_bust_01_4k",
		XMFLOAT3(1.0f, 1.0f, 1.0f),
		XM_PIDIV2, XMFLOAT3(1.0f, 0.0f, 0.0f),
		XMFLOAT3(0.0f, 1.02f, 0.0f));

	mScene.SetProperties("round_wooden_table_01_4k",
		XMFLOAT3(1.0f, 1.0f, 1.0f),
		XM_PIDIV2, XMFLOAT3(1.0f, 0.0f, 0.0f));

	mScene.SetProperties("sponza",
		XMFLOAT3(0.01f, 0.01f, 0.01f),
		0, XMFLOAT3(1.0f, 0.0f, 0.0f));

	// ��������������ʼ��Ϊ�����
	mScene.SetProperties("sky",
		XMFLOAT3(1.0f, 1.0f, 1.0f),
		0, XMFLOAT3(1.0f, 0.0f, 0.0f),
		mCamera.CartesianPos());
}

void SceneApp::UpdatePassCB(const GameTimer& gt) {
	// Rendering Pass
	// Camera
	XMMATRIX view = mCamera.ViewMatrix();
	XMMATRIX proj = mCamera.ProjectionMatrix();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(nullptr, view);
	XMMATRIX invProj = XMMatrixInverse(nullptr, proj);
	XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProj);

	XMStoreFloat4x4(&mPassCBCPU.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mPassCBCPU.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mPassCBCPU.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mPassCBCPU.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mPassCBCPU.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mPassCBCPU.InvViewProj, XMMatrixTranspose(invViewProj));
	mPassCBCPU.CameraPosW = mCamera.CartesianPos();
	mPassCBCPU.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mPassCBCPU.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mPassCBCPU.NearZ = mCamera.mNearZ;
	mPassCBCPU.FarZ = mCamera.mFarZ;

	// Timer
	mPassCBCPU.TotalTime = mTimer.TotalTime();
	mPassCBCPU.DeltaTime = mTimer.DeltaTime();

	// Lights
	CopyMemory(&mPassCBCPU.Lights, &mLights, sizeof(Light));
	mPassCBCPU.AmbientLightStrength = mAmbientLightStrength;

	//UpdateShadowTransform();
	XMStoreFloat4x4(&mPassCBCPU.ShadowTransform, XMMatrixTranspose(mShadowMap->ShadowTransformMatrix()));
	
	mPassCBGPU->Copydata(0, mPassCBCPU);

	// Shadow Mapping Pass
	const Camera* pLightCamera = mShadowMap->LightCamera(); 
	view = pLightCamera->ViewMatrix();
	proj = pLightCamera->ProjectionMatrix();

	viewProj = XMMatrixMultiply(view, proj);
	invView = XMMatrixInverse(nullptr, view);
	invProj = XMMatrixInverse(nullptr, proj);
	invViewProj = XMMatrixInverse(nullptr, viewProj);

	XMStoreFloat4x4(&mPassCBCPU.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mPassCBCPU.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mPassCBCPU.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mPassCBCPU.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mPassCBCPU.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mPassCBCPU.InvViewProj, XMMatrixTranspose(invViewProj));
	mPassCBCPU.CameraPosW = pLightCamera->CartesianPos();
	mPassCBCPU.RenderTargetSize = XMFLOAT2((float)mShadowMap->Width(), (float)mShadowMap->Height());
	mPassCBCPU.InvRenderTargetSize = XMFLOAT2(1.0f / mShadowMap->Width(), 1.0f / mShadowMap->Height());
	mPassCBCPU.NearZ = mCamera.mNearZ;
	mPassCBCPU.FarZ = mCamera.mFarZ;

	// Timer
	mPassCBCPU.TotalTime = mTimer.TotalTime();
	mPassCBCPU.DeltaTime = mTimer.DeltaTime();

	mPassCBGPU->Copydata(1, mPassCBCPU);
}

void SceneApp::UpdateShadowTransform() {
	XMMATRIX transform = mShadowMap->ShadowTransformMatrix();
}

void SceneApp::Draw(const GameTimer& gt) {
	// Reset CommandAllocator
	ThrowIfFailed(mCommandAllocator->Reset());

	// Reset CommandList
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	// ImGui
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	DrawUI();

	ImGui::Render();

	// ----------------------------- Command List Starts-----------------------------------

	// ��Shader-visible��SRV Descriptor Heap
	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// ���Pass����һ��Root Signature��ʽ
	// ���ø�ǩ��������Դ
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	mCommandList->SetGraphicsRootShaderResourceView(RootSignatureParameter::MaterialCB, 
		mScene.mMaterialCBGPU->Resource()->GetGPUVirtualAddress()); // StructuredBuffer
	// Texture Table
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	mCommandList->SetGraphicsRootDescriptorTable(RootSignatureParameter::TextureTable, srvHandle);

	// PASS 1: ShadowMapping
	DrawShadowMap(gt);

	// PASS 2: 
	mCommandList->RSSetViewports(1, &mViewPort);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// �״�״̬ת��
	if (mMsaaState) {
		// MSAA Render Target: RESOLVE_SOURCE -> RENDER_TARGET
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			mMsaaRenderTarget.Get(),
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		mCommandList->ResourceBarrier(1, &barrier);
	}
	else {
		// Back Buffer Render Target: PRESENT -> RENDER_TARGET
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		mCommandList->ResourceBarrier(1, &barrier);
	}
	
	// �ڻ���ǰ�������
	if (mMsaaState) {
		// MSAA Render Target
		mCommandList->ClearRenderTargetView(
			mMsaaRtvHeap->GetCPUDescriptorHandleForHeapStart(),
			Colors::LightSteelBlue,
			0,
			nullptr
		);
	}
	else {
		// Back Buffer Render Target
		mCommandList->ClearRenderTargetView(
			CurrentBackBufferView(),
			Colors::LightSteelBlue,
			0,
			nullptr
		);
	}
	
	mCommandList->ClearDepthStencilView(
		DepthStencilView(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f,
		0,
		0,
		nullptr
	);

	// ����Output Merge�׶ε����������������Դ��
	if (mMsaaState) {
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mMsaaRtvHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DepthStencilView();
		mCommandList->OMSetRenderTargets(
			1,
			&rtvHandle,
			true,
			&dsvHandle
		);
	}
	else {
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CurrentBackBufferView();
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DepthStencilView();
		mCommandList->OMSetRenderTargets(
			1,
			&rtvHandle,
			true,
			&dsvHandle
		);
	}
	

	// ��Render Item��ͨ����Դ��PassData
	mCommandList->SetGraphicsRootConstantBufferView(RootSignatureParameter::PerPassCB, 
		mPassCBGPU->Resource()->GetGPUVirtualAddress());
	

	// ����Pipeline State Flags
	PipelineStateFlags pipelineStateFlags = 0;
	if (mMsaaState) {
		pipelineStateFlags |= MSAA;
	}
	if (mIsWireFrame) {
		pipelineStateFlags |= WireFrame;
	}
	// ���ƣ�
	DrawRenderItems(gt, pipelineStateFlags);

	// ������ͼ
	DrawEnvironmentMap(gt, pipelineStateFlags);

	// ImGui
	ID3D12DescriptorHeap* ImGuiSrvHeap[] = { mImGuiSrvHeap.Get() };
	mCommandList->SetDescriptorHeaps(1, ImGuiSrvHeap);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

	// �ڶ���״̬ת��
	if (mMsaaState) {
		// MSAA Render Target: RENDER_TARGET -> RESOLVE_SOURCE
		// Back Buffer Render Target : PRESENT -> RESOLVE_DEST
		D3D12_RESOURCE_BARRIER barriers[2] = {
			CD3DX12_RESOURCE_BARRIER::Transition(
				mMsaaRenderTarget.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(
				CurrentBackBuffer(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RESOLVE_DEST),
		};

		mCommandList->ResourceBarrier(2, barriers);

		// ��MSAA Render Target������Back Buffer Render Target��
		mCommandList->ResolveSubresource(CurrentBackBuffer(), 0, mMsaaRenderTarget.Get(), 0, mBackBufferFormat);

		// Back Buffer Render Target : RESOLVE_DEST -> PRESENT
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RESOLVE_DEST,
			D3D12_RESOURCE_STATE_PRESENT
		);
	}
	else {
		// Back Buffer Render Target : RENDER_TARGET -> PRESENT
		D3D12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
			CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		);
		mCommandList->ResourceBarrier(1, &barrier2);
	}

	// ----------------------------- Command List Ends-----------------------------------

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrentBackBuffer = (mCurrentBackBuffer + 1) % swapChainBufferCount;

	FlushCommandQueue();
}

void SceneApp::DrawRenderItems(const GameTimer& gt, PipelineStateFlags pipelineStateFlags) {
	// ����Render Item�е���������
	for (auto& [textureFlags, itemList] : mScene.mRenderItems) {
		// ���ô��б���Render Item�����PSO
		PipelineStateFlags flags = pipelineStateFlags | textureFlags;

		// ���ܵ��Ż���
		//if (flags & ShadowMapping) {
		//	flags = pipelineStateFlags;
		//}

		// �������ֻ�ᱻ����һ��
		// �������Է�ֹԤ����׶����ɹ����PSO��ͬʱ�ֿ��Զ�̬�ؼ���ģ��
		// TODO: ����Ҳ���Խ�BuildPSO�Ĺ���������Դ�����߳�
		if (mPSOs.count(flags) == 0) {
			BuildPSO(flags);
		}
		mCommandList->SetPipelineState(mPSOs[flags].Get());

		// �����б�����������
		UINT objCBByteSize = mScene.mObjectCBGPU->GetElementSizeInBytes();
		auto objCB = mScene.mObjectCBGPU->Resource();
		for (int itemIndex = 0; itemIndex < itemList.size(); ++itemIndex) {
			RenderItem& item = itemList[itemIndex];
			// ����Vertex Buffer��Index Buffer��Primitive Topology
			D3D12_VERTEX_BUFFER_VIEW vbv = mScene.mMeshes[item.MeshIndex].VertexBufferView();
			mCommandList->IASetVertexBuffers(0, 1, &vbv);
			D3D12_INDEX_BUFFER_VIEW ibv = mScene.mMeshes[item.MeshIndex].IndexBufferView();
			mCommandList->IASetIndexBuffer(&ibv);
			mCommandList->IASetPrimitiveTopology(item.PrimitiveTopology);

			// ����Դ
			// Constant Buffer
			D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objCB->GetGPUVirtualAddress() + item.RenderItemIndex * objCBByteSize;
			mCommandList->SetGraphicsRootConstantBufferView(RootSignatureParameter::PerObjectCB, objCBAddress);

			// ���ƣ�
			mCommandList->DrawIndexedInstanced(item.NumIndices, 1, item.StartIndexLocation, item.BaseVertexLocation, 0);
		}
	}
}

void SceneApp::DrawUI() {
	// Demo
	bool show_demo_window = false;
	//ImGui::ShowDemoWindow(&show_demo_window);

	bool menu_active = true;
	ImGui::Begin("Menu", &menu_active, ImGuiWindowFlags_MenuBar);

	// Load Assets
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open..", "Ctrl+O")) {

			}
			if (ImGui::MenuItem("Save", "Ctrl+S")) {

			}
			if (ImGui::MenuItem("Close", "Ctrl+W")) {
				menu_active = false;
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	ImGui::Text("Settings:");
	ImGui::Checkbox("Wire Frame", &mIsWireFrame);
	ImGui::Checkbox("4X MSAA", &mMsaaState);

	// Show Current Position
	XMFLOAT3 cameraPos = mCamera.CartesianPos();
	ImGui::Text("Camera Position\n X: %f\n Y: %f\n Z: %f\n", cameraPos.x, cameraPos.y, cameraPos.z);

	ImGui::End();
}

void SceneApp::DrawShadowMap(const GameTimer& gt) {
	// ViewPort and SciccorRect
	D3D12_VIEWPORT viewPort = mShadowMap->ViewPort();
	D3D12_RECT scissorRect = mShadowMap->ScissorRect();
	mCommandList->RSSetViewports(1, &viewPort);
	mCommandList->RSSetScissorRects(1, &scissorRect);

	// ShadowMap״̬ת��
	// ShadowMap: PIXEL_SHADER_RESOURCE -> DEPTH_WRITE
	D3D12_RESOURCE_BARRIER barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
		mShadowMap->Resource(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE
	);
	mCommandList->ResourceBarrier(1, &barrier1);

	// ���ShadowMap
	mCommandList->ClearDepthStencilView(
		mShadowMap->DsvHandle(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f,
		0,
		0,
		nullptr
	);

	// ��Render Target����Ϊ0
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = mShadowMap->DsvHandle();
	mCommandList->OMSetRenderTargets(0, nullptr, false, &dsvHandle);

	// ��Root Signature Parameter
	D3D12_GPU_VIRTUAL_ADDRESS passCBAdress = mPassCBGPU->GetBufferPointer() + mPassCBGPU->GetElementSizeInBytes();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCBAdress);

	// ����Pipeline State Flags
	PipelineStateFlags pipelineStateFlags = ShadowMapping;
	DrawRenderItems(gt, pipelineStateFlags);


	// ShadowMap: DEPTH_WRITE -> PIXEL_SHADER_RESOURCE
	D3D12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
		mShadowMap->Resource(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	mCommandList->ResourceBarrier(1, &barrier2);
}

void SceneApp::DrawEnvironmentMap(const GameTimer& gt, PipelineStateFlags pipelineStateFlags) {
	PipelineStateFlags flags = EnvironmentMapping | pipelineStateFlags;

	// Pipeline State Object
	if (mPSOs.count(flags) == 0) {
		BuildPSO(flags);
	}
	mCommandList->SetPipelineState(mPSOs[flags].Get());

	UINT objCBByteSIze = mScene.mObjectCBGPU->GetElementSizeInBytes();
	auto objCB = mScene.mObjectCBGPU->Resource();

	RenderItem& skySphere = mScene.mSkySphere;
	// ����Vertex Buffer��Index Buffer��Primitive Topology
	D3D12_VERTEX_BUFFER_VIEW vbv = mScene.mMeshes[skySphere.MeshIndex].VertexBufferView();
	mCommandList->IASetVertexBuffers(0, 1, &vbv);
	D3D12_INDEX_BUFFER_VIEW ibv = mScene.mMeshes[skySphere.MeshIndex].IndexBufferView();
	mCommandList->IASetIndexBuffer(&ibv);
	mCommandList->IASetPrimitiveTopology(skySphere.PrimitiveTopology);

	// ����Դ
	// Constant Buffer
	D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objCB->GetGPUVirtualAddress() + skySphere.RenderItemIndex * objCBByteSIze;
	mCommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);

	// ���ƣ�
	mCommandList->DrawIndexedInstanced(skySphere.NumIndices, 1, skySphere.StartIndexLocation, skySphere.BaseVertexLocation, 0);
}

void SceneApp::OnMouseDown(WPARAM btnState, int x, int y) {
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWindow);
}

void SceneApp::OnMouseUp(WPARAM btnState, int x, int y) {
	ReleaseCapture();
}

void SceneApp::OnMouseMove(WPARAM btnState, int x, int y) {
	// �����ת
	if ((btnState & MK_LBUTTON) != 0) {
		float rotScale = 0.25f;
		float dTheta = XMConvertToRadians(rotScale * static_cast<float>(y - mLastMousePos.y));
		float dPhi = XMConvertToRadians(rotScale * static_cast<float>(x - mLastMousePos.x));

		mCamera.Pitch(dTheta);
		mCamera.Yaw(dPhi);
	}
	// �Ҽ�����
	else if ((btnState & MK_RBUTTON) != 0) {
		//float magScale = 0.005f;
		//float dx = magScale * static_cast<float>(x - mLastMousePos.x);
		//float dy = magScale * static_cast<float>(y - mLastMousePos.y);

		//mCamera.Update(dx - dy);
		//mView = mCamera.ViewMatrix();
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;

	mCamera.UpdateViewMatrix();
}

void SceneApp::OnKeyDown(UINT key) {
	switch (key) {
	case VK_SPACE:
		mCamera.MoveUp();
		break;
	case VK_CONTROL:
		mCamera.MoveDown();
		break;
	case 'W':
		mCamera.MoveForward();
		break;
	case 'S':
		mCamera.MoveBackward();
		break;
	case 'A':
		mCamera.MoveLeft();
		break;
	case 'D':
		mCamera.MoveRight();
		break;
	default:
		break;
	}

	mCamera.UpdateViewMatrix();
}

