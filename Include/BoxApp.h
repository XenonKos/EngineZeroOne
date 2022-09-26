#pragma once
#include <array>
#include <vector>
#include <memory>

#include "D3D12App.h"
#include "UploadBuffer.h"
#include "MeshGeometry.h"
#include "Camera.h"


struct Vertex {
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

struct ObjectConstants {
	XMFLOAT4X4 MVP = Identity4X4();
};

class BoxApp : public D3D12App {
public:
	BoxApp(HINSTANCE hInstance);
	BoxApp(const BoxApp& rhs) = delete;
	BoxApp& operator=(const BoxApp& rhs) = delete;
	~BoxApp();

	bool Init() override;

private:
	void BuildBoxGeometry();
	void BuildConstantBuffer();
	void BuildShadersAndInputLayout();
	void BuildRootSignature();
	void BuildPSO();

	void OnResize() override;
	void Update(const GameTimer& gt) override;
	void Draw(const GameTimer& gt) override;

	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;

	// RootSignature与PSO
	ComPtr<ID3D12RootSignature> mRootSignature;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs; // Solid and WireFrame
	//ComPtr<ID3D12PipelineState> mPSO;

	// 用于每帧MVP矩阵的常量描述符堆和常量缓冲区
	ComPtr<ID3D12DescriptorHeap> mCbvHeap;
	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB;

	// 几何数据管理
	std::unique_ptr<MeshGeometry> mBoxGeo;

	// Shaders代码
    ComPtr<ID3DBlob> mVsByteCode;
    ComPtr<ID3DBlob> mPsByteCode;
    std::wstring mShaderPath = L"C:\\Users\\Lenovo\\Desktop\\EngineZeroOne\\Shaders\\BoxShader.hlsl";

	// Input Layout
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	// Camera
	Camera mCamera;
	POINT mLastMousePos;

	// 变换矩阵
	XMFLOAT4X4 mModel = Identity4X4();
	XMFLOAT4X4 mView = Identity4X4();
	XMFLOAT4X4 mProj = Identity4X4();
};