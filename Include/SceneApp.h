#pragma once
#include "Scene.h"
#include "Camera.h"
#include "ConstantBuffer.h"
#include "UploadBuffer.h"
#include "VertexType.h"
#include "ShadowMap.h"

#include <DirectXTK12/BufferHelpers.h>
using namespace DirectX;

using PipelineStateFlags = UINT;
enum PIPE_STATE_FLAG_TYPE {
	WireFrame			= 1 << 31,
	MSAA				= 1 << 30,
	ShadowMapping		= 1 << 29,
};

// 该方法出自DirectX Samples，可用于清晰地指明一块连续的内存中不同区域代表的内容
namespace RootSignatureParameter {
	enum Value {
		PerObjectCB = 0,
		PerPassCB,
		MaterialCB,
		TextureTable,
		ParameterCount
	};
}

class SceneApp : public D3D12App {
public:
	SceneApp(HINSTANCE hInstance);
	SceneApp(const SceneApp& rhs) = delete;
	SceneApp& operator=(const SceneApp& rhs) = delete;
	~SceneApp();

	bool Init() override;

	void LoadModel(const std::string& path);
	void LoadCubeMap(const std::string& path);

private:
	void ConfigLights();

	void BuildConstantBuffer();
	void BuildRootSignature();
	void BuildShadowMap();
	void BuildSrvHeap();

	void BuildShaders(PipelineStateFlags pipelineStateFlags);
	void BuildPSO(PipelineStateFlags pipelineStateFlags);

	void OnResize() override;
	void Update(const GameTimer& gt) override;
	void UpdateRenderItemCB(const GameTimer& gt);
	void UpdatePassCB(const GameTimer& gt);
	void UpdateShadowTransform();

	void Draw(const GameTimer& gt) override;
	// Advanced Features
	void DrawUI();
	void DrawShadowMap(const GameTimer& gt); // Pass 0
	void DrawScene(const GameTimer& gt); // Pass 1
	
	void DrawRenderItems(const GameTimer& gt, PipelineStateFlags pipelineStateFlags);
	void DrawEnvironmentMap(const GameTimer& gt, PipelineStateFlags pipelineStateFlags);

	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;
	void OnKeyDown(UINT key) override;


	// RootSignature与PSO
	ComPtr<ID3D12RootSignature> mRootSignature;
	std::unordered_map<PipelineStateFlags, ComPtr<ID3D12PipelineState>> mPSOs; // Multiple PSOs

	// SRV Heap 存放场景中所有物体共用的纹理，如ShadowMap和CubeMap
	// 物体独有的纹理存放在mScene的SRV Heap中
	ComPtr<ID3D12DescriptorHeap> mSrvHeap;

	// CPU侧的Constant Buffer
	PassData mPassCBCPU;
	// GPU侧的Constant Buffer
	std::unique_ptr<UploadBuffer<PassData>> mPassCBGPU;

	// Shaders代码
	// unordered_map 封装
	std::unordered_map<PipelineStateFlags, ComPtr<ID3DBlob>> mVsByteCode;
	std::unordered_map<PipelineStateFlags, ComPtr<ID3DBlob>> mPsByteCode;

	// Camera
	Camera mCamera;
	POINT mLastMousePos;

	// 场景
	Scene mScene;

	// 灯光
	Light mLights;
	XMFLOAT4	mAmbientLightStrength;

	// ShadowMap
	// 目前假定光源数量为1且为面光源
	std::unique_ptr<ShadowMap<RectLight>> mShadowMap;
};