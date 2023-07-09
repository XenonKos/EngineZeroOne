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

// �÷�������DirectX Samples��������������ָ��һ���������ڴ��в�ͬ������������
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


	// RootSignature��PSO
	ComPtr<ID3D12RootSignature> mRootSignature;
	std::unordered_map<PipelineStateFlags, ComPtr<ID3D12PipelineState>> mPSOs; // Multiple PSOs

	// SRV Heap ��ų������������干�õ�������ShadowMap��CubeMap
	// ������е���������mScene��SRV Heap��
	ComPtr<ID3D12DescriptorHeap> mSrvHeap;

	// CPU���Constant Buffer
	PassData mPassCBCPU;
	// GPU���Constant Buffer
	std::unique_ptr<UploadBuffer<PassData>> mPassCBGPU;

	// Shaders����
	// unordered_map ��װ
	std::unordered_map<PipelineStateFlags, ComPtr<ID3DBlob>> mVsByteCode;
	std::unordered_map<PipelineStateFlags, ComPtr<ID3DBlob>> mPsByteCode;

	// Camera
	Camera mCamera;
	POINT mLastMousePos;

	// ����
	Scene mScene;

	// �ƹ�
	Light mLights;
	XMFLOAT4	mAmbientLightStrength;

	// ShadowMap
	// Ŀǰ�ٶ���Դ����Ϊ1��Ϊ���Դ
	std::unique_ptr<ShadowMap<RectLight>> mShadowMap;
};