#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>
#include "d3dx12.h"

#include <wrl.h>
#include <windows.h>
#include <windowsx.h>
#include <array>
#include <string>
#include <iostream>

#include <imgui.h>
#include "Editor/imgui_impl_win32.h"
#include "Editor/imgui_impl_dx12.h"

#include "GameTimer.h"
#include "D3D12Exception.h"
#include "Resource.h"

// 链接D3D12库
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

inline XMFLOAT4X4 Identity4X4() {
    return XMFLOAT4X4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}

inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)\
{\
	HRESULT hr__ = (x);\
if (FAILED(hr__)) {\
        throw D3D12Exception(hr__, L#x, __FILEW__, __LINE__);\
    }\
}
#endif


class D3D12App {
public:
    D3D12App(HINSTANCE hInstance);
    ~D3D12App();

    // 宽高比
    float AspectRatio() const;

    static D3D12App* GetApp();
    static D3D12App* mApp;

    int Run(); // 消息循环函数

    virtual bool Init();
    virtual LRESULT CALLBACK MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam); // 向Windows系统注册的消息回调函数
protected:
    bool InitMainWindow(); // 注册、创建、显示、更新窗口
    bool InitD3D12();
    bool InitImGui();

    virtual void OnResize(); // 窗口大小发生改变时，我们要进行一系列操作来进行适配
    virtual void Update(const GameTimer& gt) = 0;
    virtual void Draw(const GameTimer& gt) = 0;

    virtual void OnMouseDown(WPARAM btnState, int x, int y) {}
    virtual void OnMouseUp(WPARAM btnState, int x, int y) {}
    virtual void OnMouseMove(WPARAM btnState, int x, int y) {}
    virtual void OnKeyDown(UINT key) {}
    virtual void OnKeyUp(UINT key) {}

    void CalculateFrameStats();

    void LogAdapters();

    void CreateDevice();
    void CreateFence();
    void CheckMsaaState();
    void CreateCommandQueueAndList();
    void CreateSwapChain();
    void CreateDescriptorHeap();
    void CreateMsaaRenderTargetView();
    virtual void CreateRenderTargetView();
    void CreateDepthStencilView();
    void SetViewPortAndScissorRect();

    // 创建Static Samplers
    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7>
        GetStaticSamplers();

    ID3D12Resource* CurrentBackBuffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

    // FlushCommandQueue()是fence的一个应用，利用它可使CPU强制等待至GPU执行完所有CommandQueue中的命令
    void FlushCommandQueue();

    // Windows窗口设置
    HINSTANCE mAppInstance = nullptr;
    HWND mhMainWindow = nullptr;
    std::wstring mMainWindowCaption = L"EngineZeroOne";
    bool mMaximized;
    bool mMinimized;
    bool mResizing = false;

    // 计时器相关设置
    bool mAppPaused = false;
    GameTimer mTimer;

    ComPtr<IDXGIFactory6> mdxgiFactory;
    ComPtr<IDXGISwapChain> mSwapChain;
    ComPtr<ID3D12Device5> mDevice;
    ComPtr<ID3D12Fence> mFence;

    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12CommandAllocator> mCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList4> mCommandList;

    // 显示缓冲区格式设置
    int mClientWidth = 720;
    int mClientHeight = 1080;

    int mCurrentBackBuffer;
    static const UINT swapChainBufferCount = 2;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    ComPtr<ID3D12Resource> mSwapChainBuffer[swapChainBufferCount];

    // Depth/Stencil设置
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    ComPtr<ID3D12Resource> mDepthStencilBuffer;

    ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    ComPtr<ID3D12DescriptorHeap> mDsvHeap;
    // For ImGui Font Rendering
    ComPtr<ID3D12DescriptorHeap> mImGuiSrvHeap; 

    UINT mRtvDescriptorSize;
    UINT mDsvDescriptorSize;
    UINT mCbvSrvUavDescriptorSize;

    // 视口和裁剪矩形
    D3D12_VIEWPORT mViewPort;
    D3D12_RECT mScissorRect;
    
    // MSAA设置
    bool mMsaaState = true;
    UINT mMsaaQuality = 0;
    ComPtr<ID3D12Resource> mMsaaRenderTarget;
    ComPtr<ID3D12DescriptorHeap> mMsaaRtvHeap;

    // WireFrame
    bool mIsWireFrame = false;
};