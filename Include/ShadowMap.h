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
#include <windows.h>
using namespace DirectX;
using namespace Microsoft::WRL;

template <typename LightType>
class ShadowMap {
public:
	ShadowMap(ID3D12Device* device, UINT width, UINT height, LightType* light)
		: mDevice(device),
		mWidth(width),
		mHeight(height),
		mLight(light),
		mCamera(light->Position, light->Direction),
		mViewPort({0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f}),
		mScissorRect({0, 0, static_cast<int>(width), static_cast<int>(height)}) {

		mCamera.SetLens(width, height);

		CreateDescriptorHeap();
		BuildShadowMap();
		BuildDescriptors();
	}

	ShadowMap(const ShadowMap&) = delete;
	ShadowMap& operator=(const ShadowMap&) = delete;
	~ShadowMap() = default;

	UINT Width() const {
		return mWidth;
	}

	UINT Height() const {
		return mHeight;
	}

	D3D12_VIEWPORT ViewPort() const {
		return mViewPort;
	}

	D3D12_RECT ScissorRect() const {
		return mScissorRect;
	}

	ID3D12Resource* Resource() const {
		return mShadowMap.Get();
	}

	const Camera* LightCamera() const {
		return &mCamera;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DsvHandle() {
		return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE SrvHandle() {
		return mSrvHeap->GetGPUDescriptorHandleForHeapStart();
	}

	// Shadow Map纹理变换矩阵
	XMMATRIX ShadowTransformMatrix() {
		XMMATRIX V = mCamera.ViewMatrix();
		XMMATRIX P = mCamera.ProjectionMatrix();

		// 纹理坐标变换矩阵
		const XMMATRIX T(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f
		);

		return V * P * T;
	}

private:
	void CreateDescriptorHeap() {
		// SRV Heap
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		srvHeapDesc.NodeMask = 0;

		ThrowIfFailed(mDevice->CreateDescriptorHeap(
			&srvHeapDesc,
			IID_PPV_ARGS(&mSrvHeap)
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
	}

	void BuildShadowMap() {
		D3D12_RESOURCE_DESC shadowMapDesc = {};
		shadowMapDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		shadowMapDesc.Alignment = 0;
		shadowMapDesc.Width = mWidth;
		shadowMapDesc.Height = mHeight;
		shadowMapDesc.DepthOrArraySize = 1;
		shadowMapDesc.MipLevels = 1;
		shadowMapDesc.Format = mFormat;
		shadowMapDesc.SampleDesc.Count = 1;
		shadowMapDesc.SampleDesc.Quality = 0;
		shadowMapDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		shadowMapDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		// 创建Shadow Map
		D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		// 指定Optimized Clear Value
		D3D12_CLEAR_VALUE optimizedClearValue = { DXGI_FORMAT_D24_UNORM_S8_UINT, { 1.0f, 0 } };
		ThrowIfFailed(mDevice->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&shadowMapDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&optimizedClearValue,
			IID_PPV_ARGS(&mShadowMap)
		));
	}

	void BuildDescriptors() {
		// DSV
		// 当资源的类型为TYPELESS时，必须提供D3D12_DEPTH_STENCIL_VIEW_DESC
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.Texture2D.MipSlice = 0;

		mDevice->CreateDepthStencilView(
			mShadowMap.Get(),
			&dsvDesc,
			mDsvHeap->GetCPUDescriptorHandleForHeapStart()
		);
	}

	ID3D12Device* mDevice;

	// ShadowMap格式
	UINT mWidth;
	UINT mHeight;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R24G8_TYPELESS;

	// 与此Shadow Map相关的Light
	LightType* mLight;

	// 内嵌一个Camera
	Camera mCamera;

	// Rasterizer State
	D3D12_VIEWPORT mViewPort;
	D3D12_RECT mScissorRect;

	// 资源管理
	ComPtr<ID3D12Resource> mShadowMap;
	ComPtr<ID3D12DescriptorHeap> mSrvHeap;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap;
};