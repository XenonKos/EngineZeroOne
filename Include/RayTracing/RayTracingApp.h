#pragma once
#include "Scene.h"
#include "Camera.h"
#include "RayTracing/ShaderTable.h"
#include "RayTracing/RayTracingHLSLCompat.h"

// enum class需强制类型转换，不如namespace方便
namespace GlobalRootParameter {
	enum Value {
		OutputView = 0,
		AccelerationStructure,
		ParameterCount
	};
}

namespace LocalRootParameter {
	enum Value {
		ViewportConstant = 0,
		ParameterCount
	};
}

namespace RayType {
	enum Value {
		PrimaryRay,
		ShadowRay,
		Count
	};
}

namespace LocalRootSignature {
	namespace Triangle {
		struct RootArguments {
			PrimitiveConstantBuffer materialCb;
		};
	}

	inline UINT MaxRootArgumentsSize()
	{
		return sizeof(Triangle::RootArguments);
	}
}

struct AccelerationStructureBuffers
{
	ComPtr<ID3D12Resource> scratch;
	ComPtr<ID3D12Resource> accelerationStructure;
	ComPtr<ID3D12Resource> instanceDesc;    // Used only for top-level AS
	UINT64                 ResultDataMaxSizeInBytes;
};

class RayTracingApp : public D3D12App {
public:
	void CheckRayTracingSupport();
	void CreateDescriptorHeap();
	void CreateRayTracingOutput();
	void CreateRootSignature();
	void CreateStateObject();

	// Complile Shaders
	void CompileShaders();

	// Acceleration Structure
	void BuildGeometryDescForBottomLevelAS(std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& geometryDescs);
	AccelerationStructureBuffers BuildBottomLevelAS(std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& geometryDescs);
	D3D12_GPU_VIRTUAL_ADDRESS BuildInstanceDescForTopLevelAS(D3D12_GPU_VIRTUAL_ADDRESS bottomLevelASAddress);
	AccelerationStructureBuffers BuildTopLevelAS(D3D12_GPU_VIRTUAL_ADDRESS instanceAddress);
	void BuildAccelerationStructures();

	void BuildShaderTable();

	void StartRayTracing();

private:
	// RayTracing Support
	bool bSupportRayTracing;

	// Descriptor Heap
	// Shared by SRVs and UAVs
	ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;

	// UAV and SRV
	// RayTracing Output
	ComPtr<ID3D12Resource> mRayTracingOutput;

	// State Object, Local Root Signature and Global Root Signature
	ComPtr<ID3D12StateObject> mRTPSO;
	ComPtr<ID3D12RootSignature> mLocalRootSignature;
	ComPtr<ID3D12RootSignature> mGlobalRootSignature;

	// Constant Buffer
	RayGenConstantBuffer mRayGenCB;

	// Acceleration Structure
	ComPtr<ID3D12Resource> mAccelerationStructure;
	ComPtr<ID3D12Resource> mBottomLevelAccelerationStructure;
	ComPtr<ID3D12Resource> mTopLevelAccelerationStructure;

	// Shader Tables
	ComPtr<ID3D12Resource> mRayGenerationShaderTable;
	ComPtr<ID3D12Resource> mHitGroupShaderTable;
	UINT mHitGroupShaderTableStrideInBytes;
	ComPtr<ID3D12Resource> mMissShaderTable;
	UINT mMissShaderTableStrideInBytes;

	// Shader Export Names
	static const wchar_t* mHitGroupName;
	static const wchar_t* mRaygenShaderName;
	static const wchar_t* mClosestHitShaderName;
	static const wchar_t* mMissShaderName;

	// Scene
	Scene mScene;
};