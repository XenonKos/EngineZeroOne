#include "BoxApp.h"
#include <array>

BoxApp::BoxApp(HINSTANCE hInstance) 
	: D3D12App(hInstance),
	mCamera(XMFLOAT3(0.0f, 1.3f, -2.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)) {

}

BoxApp::~BoxApp() {

}

bool BoxApp::Init() {
	if (!D3D12App::Init()) {
		return false;
	}

	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	BuildBoxGeometry();
	BuildConstantBuffer();
	BuildShadersAndInputLayout();
	BuildRootSignature();
	BuildPSO();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	return true;
}

void BoxApp::BuildBoxGeometry() {
	/* Box vertices
	*            5-------6              ^ y
	*           /|      /|              |   > z
	*		   1-------2 |              |  /
	*          | 4-----|-7              | /
	*          |/      |/               0-------> x 左手坐标系
	*          0-------3                组织三角面时注意法向量朝向
	*/

	// 创建vertex与index
	std::array<Vertex, 8> vertices = {
		Vertex{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) },
		Vertex{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) },
		Vertex{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) },
		Vertex{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) },
		Vertex{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) },
		Vertex{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) },
		Vertex{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) },
		Vertex{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) }
	};

	std::array<std::uint16_t, 36> indices = {
		// Front
		0, 1, 2,
		0, 2, 3,

		// Back
		4, 6, 5,
		4, 7, 6,

		// Left
		0, 5, 1,
		0, 4, 5,

		// Right
		3, 2, 6,
		3, 6, 7,

		// Up
		1, 5, 6,
		1, 6, 2,

		// Down
		0, 3, 7,
		0, 7, 4
	};

	// 设置Vertex Buffer与Index Buffer
	const UINT vbSizeInBytes = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibSizeInBytes = (UINT)indices.size() * sizeof(std::uint16_t);

	mBoxGeo = std::make_unique<MeshGeometry>();
	mBoxGeo->Name = "BoxGeo";

	mBoxGeo->VertexBufferSizeInBytes = vbSizeInBytes;
	mBoxGeo->VertexBufferStrideInBytes = sizeof(Vertex);
	mBoxGeo->IndexBufferSizeInBytes = ibSizeInBytes;

	SubMeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.BaseVertexLocation = 0;
	submesh.StartIndexLocation = 0;

	mBoxGeo->DrawArgs["box"] = submesh;

	ThrowIfFailed(D3DCreateBlob(vbSizeInBytes, &mBoxGeo->VertexBufferCPU));
	CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbSizeInBytes);

	ThrowIfFailed(D3DCreateBlob(ibSizeInBytes, &mBoxGeo->IndexBufferCPU));
	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibSizeInBytes);

	mBoxGeo->UploadResource(mDevice.Get(), mCommandList.Get(),
		mBoxGeo->VertexBufferCPU->GetBufferPointer(),
		mBoxGeo->VertexBufferCPU->GetBufferSize(),
		mBoxGeo->VertexBufferGPU,
		mBoxGeo->VertexBufferUploader);

	mBoxGeo->UploadResource(mDevice.Get(), mCommandList.Get(),
		mBoxGeo->IndexBufferCPU->GetBufferPointer(),
		mBoxGeo->IndexBufferCPU->GetBufferSize(),
		mBoxGeo->IndexBufferGPU,
		mBoxGeo->IndexBufferUploader);
}

void BoxApp::BuildConstantBuffer() {
	// 创建CBV描述符堆
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;

	ThrowIfFailed(mDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));

	// 创建Constant Buffer资源
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(mDevice.Get(), 1, true);


	// 创建Constant Buffer描述符，并将其加入描述符堆
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = mObjectCB->GetBufferPointer();
	cbvDesc.SizeInBytes = mObjectCB->GetBufferSize();

	mDevice->CreateConstantBufferView(&cbvDesc,
		mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void BoxApp::BuildShadersAndInputLayout() {
	ComPtr<ID3DBlob> ErrorMsgs;

	ThrowIfFailed(D3DCompileFromFile(
		mShaderPath.c_str(),
		nullptr,
		nullptr,
		"VS",
		"vs_5_1",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		mVsByteCode.GetAddressOf(),
		ErrorMsgs.GetAddressOf()
	));

	ThrowIfFailed(D3DCompileFromFile(
		mShaderPath.c_str(),
		nullptr,
		nullptr,
		"PS",
		"ps_5_1",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		mPsByteCode.GetAddressOf(),
		ErrorMsgs.GetAddressOf()
	));

	mInputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void BoxApp::BuildRootSignature() {
	// 创建根参数列表
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	// 将描述符表填入根参数
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	// 序列化根参数
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig;
	ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		errorBlob.GetAddressOf()
	));

	// 创建根签名
	ThrowIfFailed(mDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)
	));

}

void BoxApp::BuildPSO() {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature.Get();

	psoDesc.VS = { 
		reinterpret_cast<BYTE*>(mVsByteCode->GetBufferPointer()), 
		mVsByteCode->GetBufferSize() 
	};
	psoDesc.PS = {
		reinterpret_cast<BYTE*>(mPsByteCode->GetBufferPointer()),
		mPsByteCode->GetBufferSize()
	};

	// Configuration
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = mMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = mMsaaState ? (mMsaaQuality - 1) : 0;

	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;

	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.DSVFormat = mDepthStencilFormat;

	// 创建PSO
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(
		&psoDesc,
		IID_PPV_ARGS(&mPSOs["Solid"])
	));

	// 创建WireFrame PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC wireFramePsoDesc = psoDesc;
	wireFramePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(
		&wireFramePsoDesc,
		IID_PPV_ARGS(&mPSOs["WireFrame"])
	));
}

void BoxApp::OnResize() {
	D3D12App::OnResize();

	float fov = XM_PIDIV4;
	float aspectRatio = AspectRatio();
	float nearPlane = 1.0f;
	float farPlane = 1000.0f;
	XMMATRIX Proj = XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);

	XMStoreFloat4x4(&mProj, Proj);
}

void BoxApp::Update(const GameTimer& gt) {
	// 设置MVP
	XMMATRIX model = XMLoadFloat4x4(&mModel);
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX mvp = model * view * proj;

	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.MVP, XMMatrixTranspose(mvp));
	mObjectCB->Copydata(0, objConstants);

	//ObjectConstants objConstants;
	//objConstants.MVP = mView;
	//mObjectCB->Copydata(0, objConstants);
}

void BoxApp::Draw(const GameTimer& gt) {
	// ImGui
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//DrawUI();

	ImGui::Render();

	// Reset CommandAllocator
	ThrowIfFailed(mCommandAllocator->Reset());

	// Reset CommandList
	if (mIsWireFrame) {
		ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), mPSOs["WireFrame"].Get()));
	}
	else {
		ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), mPSOs["Solid"].Get()));
	}

	mCommandList->RSSetViewports(1, &mViewPort);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// 对当前后台缓冲区的状态做转换
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	mCommandList->ResourceBarrier(1, &barrier);

	// 在绘制前进行清空
	mCommandList->ClearRenderTargetView(
		CurrentBackBufferView(), 
		Colors::LightSteelBlue, 
		0, 
		nullptr);
	mCommandList->ClearDepthStencilView(
		DepthStencilView(), 
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
		1.0f, 
		0, 
		0, 
		nullptr);

	// 设置Output Merge阶段的输出缓冲区
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CurrentBackBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DepthStencilView();
	mCommandList->OMSetRenderTargets(
		1,
		&rtvHandle,
		true,
		&dsvHandle
	);

	// 绑定Shader-visible的描述符堆
	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// 设置Vertex Buffer、Index Buffer、Primitive Topology
	D3D12_VERTEX_BUFFER_VIEW vbv = mBoxGeo->VertexBufferView();
	mCommandList->IASetVertexBuffers(0, 1, &vbv);

	D3D12_INDEX_BUFFER_VIEW ibv = mBoxGeo->IndexBufferView();
	mCommandList->IASetIndexBuffer(&ibv);

	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 设置根签名并绑定资源
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

	// 绘制！
	mCommandList->DrawIndexedInstanced(mBoxGeo->DrawArgs["box"].IndexCount, 1, 0, 0, 0);
	//mCommandList->DrawInstanced(8, 1, 0, 0);

	// ImGui
	ID3D12DescriptorHeap* ImGuiSrvHeap[] = { mImGuiSrvHeap.Get() };
	mCommandList->SetDescriptorHeaps(1, ImGuiSrvHeap);
	ImDrawData* a = ImGui::GetDrawData();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

	// 对当前后台缓冲区的状态做转换 
	D3D12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	mCommandList->ResourceBarrier(1, &barrier2);

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrentBackBuffer = (mCurrentBackBuffer + 1) % swapChainBufferCount;

	FlushCommandQueue();
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y) {
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWindow);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y) {
	ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y) {
	// 左键旋转
	if ((btnState & MK_LBUTTON) != 0) {
		//float rotScale = 0.25f;
		//float dTheta = XMConvertToRadians(rotScale * static_cast<float>(y - mLastMousePos.y));
		//float dPhi = XMConvertToRadians(rotScale * static_cast<float>(x - mLastMousePos.x));

		//mCamera.Update(dTheta, dPhi);
		//mView = mCamera.ViewMatrix();
	}
	// 右键放缩
	else if ((btnState & MK_RBUTTON) != 0) {
		//float magScale = 0.005f;
		//float dx = magScale * static_cast<float>(x - mLastMousePos.x);
		//float dy = magScale * static_cast<float>(y - mLastMousePos.y);

		//mCamera.Update(dx - dy);
		//mView = mCamera.ViewMatrix();
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

