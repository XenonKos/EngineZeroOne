#include "D3D12App.h"

// ImGui
extern
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	return D3D12App::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}


// �����ڡ������Ϣ�������
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

D3D12App::D3D12App(HINSTANCE hInstance) 
	: mAppInstance(hInstance) {
	assert(mApp == nullptr);
	mApp = this;
}

D3D12App::~D3D12App() {
	if (mDevice != nullptr) {
		FlushCommandQueue();
	}

	// ImGui
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

float D3D12App::AspectRatio() const {
	return static_cast<float>(mClientWidth) / mClientHeight;
}

D3D12App* D3D12App::mApp = nullptr;
D3D12App* D3D12App::GetApp() {
	return mApp;
}

// ��Ϣѭ��
int D3D12App::Run() {
	MSG msg = { 0 };

	// ������Ϣѭ��ʱ�����ü�ʱ��
	mTimer.Reset();

	while (msg.message != WM_QUIT) {
		if (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		else {
			mTimer.Tick();

			if (!mAppPaused) {
				CalculateFrameStats();
				Update(mTimer);
				Draw(mTimer);
			}
			else {
				Sleep(100);
			}
		}
	}
	return (int)msg.wParam;
}

bool D3D12App::Init() {
	if (!InitMainWindow()) {
		return false;
	}
	if (!InitD3D12()) {
		return false;
	}

	if (!InitImGui()) {
		return false;
	}

	OnResize();
	return true;
}

LRESULT D3D12App::MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	// ImGui
	ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

	switch (msg)
	{
	// �����ȡ������ʱ
	case WM_ACTIVATE: { 
		if (LOWORD(wParam) == WA_INACTIVE) {
			//mAppPaused = true;
			mTimer.Pause();
		}
		else {
			//mAppPaused = true;
			mTimer.Start();
		}
	}
	break;
	// ���ڳߴ����
	case WM_ENTERSIZEMOVE: {
		mAppPaused = true;
		mResizing = true;
		mTimer.Pause();
	}
	break;
	case WM_EXITSIZEMOVE: {
		mAppPaused = false;
		mResizing = false;
		mTimer.Start();
		OnResize();
	}
	break;
	case WM_SIZE: {
		mClientWidth = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if (mDevice) {
			if (wParam == SIZE_MAXIMIZED) {
				mAppPaused = false;
				mMaximized = true;
				mMinimized = false;
				OnResize();
			}
			else if (wParam == SIZE_MINIMIZED) {
				mAppPaused = true;
				mMaximized = false;
				mMinimized = true;
				mTimer.Pause();
			}
			else if (wParam == SIZE_RESTORED) {
				if (mMinimized) {
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}
				else if (mMaximized) {
					mMaximized = false;
					OnResize();
				}
				else if (!mResizing) {
					OnResize();
				}
			}
		}
	}
	break;
	// ����ƶ�����
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	// �������봦��
	case WM_KEYDOWN:
		OnKeyDown(static_cast<UINT>(wParam));
		break;
	case WM_KEYUP:
		OnKeyUp(static_cast<UINT>(wParam));
		break;
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// �����˵�ѡ��:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(mAppInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: �ڴ˴����ʹ�� hdc ���κλ�ͼ����...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

bool D3D12App::InitMainWindow() {
	// Registration
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = mAppInstance;
	wcex.hIcon = LoadIcon(mAppInstance, MAKEINTRESOURCE(IDI_ENGINEZEROONE));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_ENGINEZEROONE);
	wcex.lpszClassName = L"MainWindow";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	if (!RegisterClassExW(&wcex))
	{
		//MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}
	
	// Creation
	// ����Ӧ�ó���ʹ�õķ�Χ��С���㴰�ڴ�С
	RECT R = { 0, 0, mClientWidth, mClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false); // ���һ������Ϊ�Ƿ�ӵ�в˵���
	int width = R.right - R.left, height = R.bottom - R.top;

	mhMainWindow = CreateWindowW(
		L"MainWindow", 
		mMainWindowCaption.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 
		CW_USEDEFAULT, 
		width, 
		height, 
		nullptr, 
		nullptr, 
		mAppInstance, 
		nullptr);

	if (!mhMainWindow)
	{
		//MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	// Show and Update
	ShowWindow(mhMainWindow, SW_SHOW);
	UpdateWindow(mhMainWindow);

	return true;
}

bool D3D12App::InitD3D12() {
#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	CreateDevice();
	CreateFence();
	CheckMsaaState();
	CreateCommandQueueAndList();
	CreateSwapChain();
	CreateDescriptorHeap();

	OnResize();

	return true;
}

bool D3D12App::InitImGui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsLight();

	
	if (!ImGui_ImplWin32_Init(mhMainWindow)) {
		return false;
	}
	if (!ImGui_ImplDX12_Init(
		mDevice.Get(),
		swapChainBufferCount,
		mBackBufferFormat,
		mImGuiSrvHeap.Get(),
		mImGuiSrvHeap->GetCPUDescriptorHandleForHeapStart(),
		mImGuiSrvHeap->GetGPUDescriptorHandleForHeapStart()
	)) {
		return false;
	}

	return true;
}

void D3D12App::OnResize() {
	assert(mDevice);
	assert(mSwapChain);
	assert(mCommandAllocator);
	// �ڸı�����ǰ�ȵȴ�CommandQueue�е�����ִ�����
	FlushCommandQueue();

	// ��ִ��ExecuteCommandLists��Reset�����ڴ棬ע�������������ʱCommandList���봦��Close״̬
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	// �ͷ�֮ǰ�Ļ�������Դ
	for (int i = 0; i < swapChainBufferCount; ++i) {
		mSwapChainBuffer[i].Reset();
	}
	mDepthStencilBuffer.Reset();

	// ����BackBuffer��С
	ThrowIfFailed(mSwapChain->ResizeBuffers(
		swapChainBufferCount,
		mClientWidth,
		mClientHeight,
		mBackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
	));

	mCurrentBackBuffer = 0;

	CreateRenderTargetView();
	CreateDepthStencilView();
	CreateMsaaRenderTargetView();

	// �ر�CommandList������CommandQueue
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	// �����µ��ӿںͲü����� 
	SetViewPortAndScissorRect();
}

void D3D12App::CalculateFrameStats() {
	// ÿ��ͳ��һ��FPS
	static int frameCount = 0;
	static float timeElapsed = 0.0f;

	// ÿ����һ�λ��ƣ��ͻ����˺���
	frameCount++;

	if (mTimer.TotalTime() - timeElapsed >= 1.0f) {
		float fps = static_cast<float>(frameCount);
		float mspf = 1000.0f / fps;

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);

		std::wstring windowText = mMainWindowCaption +
			L"  FPS: " + fpsStr +
			L"  mspf: " + mspfStr;
		SetWindowText(mhMainWindow, windowText.c_str());

		// Reset
		frameCount = 0;
		timeElapsed += 1.0f;
	}
}

void D3D12App::LogAdapters() {
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;

	while (mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());

		++i;
	}
}

void D3D12App::CreateDevice() {
#if defined(DEBUG) || defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	ThrowIfFailed(CreateDXGIFactory2(
		DXGI_CREATE_FACTORY_DEBUG, 
		IID_PPV_ARGS(&mdxgiFactory)
	));

	// ��ʾ�����Կ���Ϣ
	LogAdapters();

	// IDXGIFactory6�ṩ��EnumAdapterByGpuPreference����,
	// ���Ը�������ѡ����ʵ��Կ�
	ComPtr<IDXGIAdapter> pAdapter = nullptr;
	ThrowIfFailed(mdxgiFactory->EnumAdapterByGpuPreference(
		0,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
		IID_PPV_ARGS(&pAdapter)
	));


	HRESULT hardwareResult = D3D12CreateDevice(
		pAdapter.Get(),
		D3D_FEATURE_LEVEL_12_0,
		IID_PPV_ARGS(&mDevice)
	);
}

void D3D12App::CreateFence() {
	ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

	// ����descriptor��С
	mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void D3D12App::CheckMsaaState() {
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;

	ThrowIfFailed(mDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)
	));

	mMsaaQuality = msQualityLevels.NumQualityLevels;
}


void D3D12App::CreateCommandQueueAndList() {
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(mDevice->CreateCommandQueue(
		&queueDesc, 
		IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfFailed(mDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, 
		IID_PPV_ARGS(&mCommandAllocator)));

	ThrowIfFailed(mDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mCommandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(&mCommandList)
	));

	// ���뱣֤������CommandList֮���䴦��Close״̬���������ܵ���Reset����
	ThrowIfFailed(mCommandList->Close());
}

void D3D12App::CreateSwapChain() {
	// �ڴ����½�����ǰ�������پɵĽ�����
	mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	swapChainDesc.BufferDesc.Width = mClientWidth;
	swapChainDesc.BufferDesc.Height = mClientHeight;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Format = mBackBufferFormat;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// DirectX 12��֧��MSAA���������˴�Count������Ϊ1
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = swapChainBufferCount;
	swapChainDesc.OutputWindow = mhMainWindow;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&swapChainDesc,
		&mSwapChain
	));
}

void D3D12App::CreateDescriptorHeap() {
	// RTV Heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = swapChainBufferCount;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;

	ThrowIfFailed(mDevice->CreateDescriptorHeap(
		&rtvHeapDesc,
		IID_PPV_ARGS(&mRtvHeap)
	));

	// DSV Heap
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;

	ThrowIfFailed(mDevice->CreateDescriptorHeap(
		&dsvHeapDesc,
		IID_PPV_ARGS(&mDsvHeap)
	));

	// ImGui SRV Heap
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NodeMask = 0;

	ThrowIfFailed(mDevice->CreateDescriptorHeap(
		&srvHeapDesc,
		IID_PPV_ARGS(&mImGuiSrvHeap)
	));
}

void D3D12App::CreateMsaaRenderTargetView() {
	// MSAA Render Target
	D3D12_RESOURCE_DESC msaaRtvDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		mBackBufferFormat,
		mClientWidth,
		mClientHeight,
		1,
		1,
		4  // 4X MSAA
	);
	msaaRtvDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE optimizedClearValue = {};
	optimizedClearValue.Format = mBackBufferFormat;
	CopyMemory(optimizedClearValue.Color, Colors::LightSteelBlue, sizeof(float) * 4);
	ThrowIfFailed(mDevice->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&msaaRtvDesc,
		D3D12_RESOURCE_STATE_RESOLVE_SOURCE, // ��ʼ״̬������ΪRESOLVE_SOURCE
		&optimizedClearValue,
		IID_PPV_ARGS(&mMsaaRenderTarget)
	));


	// MSAA RTV Heap
	D3D12_DESCRIPTOR_HEAP_DESC msaaRtvHeapDesc;
	msaaRtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	msaaRtvHeapDesc.NumDescriptors = 1;
	msaaRtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	msaaRtvHeapDesc.NodeMask = 0;

	ThrowIfFailed(mDevice->CreateDescriptorHeap(
		&msaaRtvHeapDesc,
		IID_PPV_ARGS(&mMsaaRtvHeap)
	));

	// MSAA RTV
	CD3DX12_CPU_DESCRIPTOR_HANDLE msaaRtvHeapHandle(mMsaaRtvHeap->GetCPUDescriptorHandleForHeapStart());
	mDevice->CreateRenderTargetView(
		mMsaaRenderTarget.Get(),
		nullptr,
		msaaRtvHeapHandle
	);
}

void D3D12App::CreateRenderTargetView() {
	// Swap Chain RTV
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	
	for (UINT i = 0; i < swapChainBufferCount; ++i) {
		ThrowIfFailed(mSwapChain->GetBuffer(
			i,
			IID_PPV_ARGS(&mSwapChainBuffer[i])
		));

		// return void
		mDevice->CreateRenderTargetView(
			mSwapChainBuffer[i].Get(), 
			nullptr, 
			rtvHeapHandle
		);

		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

}

void D3D12App::CreateDepthStencilView() {
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mClientWidth;
	depthStencilDesc.Height = mClientHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = mMsaaState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = mMsaaState ? (mMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// ������Ȼ�������Դ
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	// ָ��Optimized Clear Value
	D3D12_CLEAR_VALUE optimizedClearValue = { mDepthStencilFormat, { 1.0f, 0 } };
	ThrowIfFailed(mDevice->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optimizedClearValue,
		IID_PPV_ARGS(&mDepthStencilBuffer)
	));

	// ������Ȼ�����������
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
	mDevice->CreateDepthStencilView(
		mDepthStencilBuffer.Get(), 
		nullptr, 
		dsvHeapHandle
	);

	// ����Դת��Ϊ��д״̬
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_DEPTH_WRITE
	);
	mCommandList->ResourceBarrier(
		1,
		&barrier
	);
}

void D3D12App::SetViewPortAndScissorRect() {
	mViewPort.TopLeftX = 0.0f;
	mViewPort.TopLeftY = 0.0f;
	mViewPort.Width = static_cast<float>(mClientWidth);
	mViewPort.Height = static_cast<float>(mClientHeight);
	mViewPort.MinDepth = 0.0f;
	mViewPort.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> D3D12App::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);
	
	// Comparison Sampler
	const CD3DX12_STATIC_SAMPLER_DESC shadow(
		6,
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		0.0f,
		16,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE
	);

	return { pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWrap, anisotropicClamp, shadow };
}

ID3D12Resource* D3D12App::CurrentBackBuffer() const {
	return mSwapChainBuffer[mCurrentBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12App::CurrentBackBufferView() const {
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrentBackBuffer,
		mRtvDescriptorSize
	);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12App::DepthStencilView() const {
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3D12App::FlushCommandQueue() {
	UINT64 newValue = mFence->GetCompletedValue() + 1;

	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), newValue));

	if (mFence->GetCompletedValue() < newValue) {
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

		ThrowIfFailed(mFence->SetEventOnCompletion(newValue, eventHandle));

		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}
