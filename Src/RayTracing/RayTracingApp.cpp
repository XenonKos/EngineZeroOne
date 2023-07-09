#include "RayTracing/RayTracingApp.h"

#define SizeInUINT32(obj) ((sizeof(obj) + sizeof(UINT32) - 1) / sizeof(UINT32))

const wchar_t* RayTracingApp::mHitGroupName = L"MyHitGroup";
const wchar_t* RayTracingApp::mRaygenShaderName = L"MyRaygenShader";
const wchar_t* RayTracingApp::mClosestHitShaderName = L"MyClosestShader";
const wchar_t* RayTracingApp::mMissShaderName = L"MyMissShader";

void RayTracingApp::CheckRayTracingSupport() {
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 rayTracingSupportData = {};

	// RayTracing��֧�ְ�����D3D12_FEATURE_D3D12_OPTIONS5������
	HRESULT hr = mDevice->CheckFeatureSupport(
		D3D12_FEATURE_D3D12_OPTIONS5,
		&rayTracingSupportData,
		sizeof(rayTracingSupportData)
	);

	bSupportRayTracing = SUCCEEDED(hr) && (rayTracingSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED);
}

void RayTracingApp::CreateDescriptorHeap() {
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.NumDescriptors = 128;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NodeMask = 0;

	ThrowIfFailed(mDevice->CreateDescriptorHeap(
		&descriptorHeapDesc,
		IID_PPV_ARGS(&mDescriptorHeap)
	));
}

void RayTracingApp::CreateRayTracingOutput() {
	// 1. ��Դ����
	D3D12_RESOURCE_DESC rayTracingOutputDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		mBackBufferFormat,
		mClientWidth,
		mClientHeight,
		1,
		1,
		1,
		0,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	);

	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(mDevice->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&rayTracingOutputDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&mRayTracingOutput)
	));

	// 2. ���������� + д����������
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	// �˴���UAV����Descriptor Heap�����
	CD3DX12_CPU_DESCRIPTOR_HANDLE descHandle(
		mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		mDescriptorHeap->GetDesc().NumDescriptors - 1,
		mCbvSrvUavDescriptorSize);
	mDevice->CreateUnorderedAccessView(
		mRayTracingOutput.Get(),
		nullptr, // CounterResource
		&uavDesc,
		descHandle
	);
}

void RayTracingApp::CreateRootSignature() {
	// Global Root Signature
	// �˴����밴˳��ִ�У���ǰ��û��������ϵ
	// �ɵ��������������Թ��������ͻ
	{
		CD3DX12_ROOT_PARAMETER rootParameters[GlobalRootParameter::ParameterCount];

		CD3DX12_DESCRIPTOR_RANGE UAVDescriptor;
		UAVDescriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
		rootParameters[GlobalRootParameter::OutputView].InitAsDescriptorTable(1, &UAVDescriptor);
		rootParameters[GlobalRootParameter::AccelerationStructure].InitAsShaderResourceView(0);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);

		HRESULT hr = S_OK;
		ComPtr<ID3DBlob> serializedRootSignature;
		ComPtr<ID3DBlob> ErrorMsgs;
		hr = D3D12SerializeRootSignature(
			&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&serializedRootSignature,
			&ErrorMsgs
		);

		if (ErrorMsgs != nullptr) {
			OutputDebugStringA((LPCSTR)(ErrorMsgs->GetBufferPointer()));
		}

		ThrowIfFailed(hr);

		ThrowIfFailed(mDevice->CreateRootSignature(
			0,
			serializedRootSignature->GetBufferPointer(),
			serializedRootSignature->GetBufferSize(),
			IID_PPV_ARGS(&mGlobalRootSignature)
		));
	}

	// Local Root Signature
	{
		CD3DX12_ROOT_PARAMETER rootParameters[LocalRootParameter::ParameterCount];
		// �׸�����Ϊ��32-bitΪ��λ�����Ĵ�С
		rootParameters[LocalRootParameter::ViewportConstant].InitAsConstants(SizeInUINT32(mRayGenCB), 0);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
		// Local Signature����ָ����Flag
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		HRESULT hr = S_OK;
		ComPtr<ID3DBlob> serializedRootSignature;
		ComPtr<ID3DBlob> ErrorMsgs;
		hr = D3D12SerializeRootSignature(
			&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&serializedRootSignature,
			&ErrorMsgs
		);

		if (ErrorMsgs != nullptr) {
			OutputDebugStringA((LPCSTR)(ErrorMsgs->GetBufferPointer()));
		}

		ThrowIfFailed(hr);

		ThrowIfFailed(mDevice->CreateRootSignature(
			0,
			serializedRootSignature->GetBufferPointer(),
			serializedRootSignature->GetBufferSize(),
			IID_PPV_ARGS(&mLocalRootSignature)
		));
	}
}

void RayTracingApp::CreateStateObject() {
	CD3DX12_STATE_OBJECT_DESC rayTracingPipeline(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

	// 1. ����Subobject
	// Subobject��State Object�еĴ洢��ʽΪһƬ�������ڴ�

	// DXIL Library
	// Shader���뱻fxcԤ�ȱ���ã�������ֽ��������ʽ����װ��һ��ͷ�ļ���
	// �˴�RayTracing.hlsl����װ��RayTracing.hlsl.h�е�gpRayTracing��
	auto lib = rayTracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	//CD3DX12_SHADER_BYTECODE libDXIL = CD3DX12_SHADER_BYTECODE((void*)gpRayTracing, ARRAYSIZE(gpRayTracing));
	//lib->SetDXILLibrary(&libDXIL);
	// ����������Entrypoint(Shader Export)
	{
		lib->DefineExport(mRaygenShaderName);
		lib->DefineExport(mClosestHitShaderName);
		lib->DefineExport(mMissShaderName);
	}

	// Hit Group
	// ���ҵ���⣬Hit Group�൱�ڽӿڵ�������
	auto hitGroup = rayTracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
	hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
	hitGroup->SetClosestHitShaderImport(mClosestHitShaderName);
	hitGroup->SetHitGroupExport(mHitGroupName);

	// Local Root Signature
	{
		auto localRootSignature = rayTracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
		localRootSignature->SetRootSignature(mLocalRootSignature.Get());
		// Explicit Association to Ray Generation Shader
		auto rootSignatureAssociation = rayTracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
		rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
		rootSignatureAssociation->AddExport(mRaygenShaderName);
	}

	// Global Root Signature
	auto globalRootSignature = rayTracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	globalRootSignature->SetRootSignature(mGlobalRootSignature.Get());

	// Shader Config
	// ���ڶ���Shader�Ĳ�������
	// �˴��������payload��attribute�Ĵ�С
	auto shaderConfig = rayTracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	UINT payloadSize = 4 * sizeof(float); // float4 color
	UINT attributeSize = 2 * sizeof(float); // float2 barycentric
	shaderConfig->Config(payloadSize, attributeSize);

	// Pipeline Config
	// �涨��Shader�����ݹ����
	auto pipelineConfig = rayTracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	UINT maxRecursionDepth = 1; // only primary ray
	pipelineConfig->Config(maxRecursionDepth);

	// 2. ����State Object
	// CD3DX12_STATE_OBJECT������CD3DX12_STATE_OBJECT -> const D3D12_STATE_OBJECT *����ת������
	ThrowIfFailed(mDevice->CreateStateObject(
		rayTracingPipeline, 
		IID_PPV_ARGS(&mRTPSO)
	));
}

void RayTracingApp::BuildGeometryDescForBottomLevelAS(std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& geometryDescs) {
	// ONLY For Triangles
	// �����ڹ������ٽṹ֮ǰ���ģ�͵���
	UINT meshCount = mScene.MeshCount();
	geometryDescs.resize(meshCount);

	for (UINT i = 0; i < meshCount; ++i) {
		geometryDescs[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometryDescs[i].Triangles.IndexBuffer = mScene.IndexBufferAddress(i);
		geometryDescs[i].Triangles.IndexCount = mScene.IndexCount(i);
		geometryDescs[i].Triangles.IndexFormat = mScene.IndexFormat(i);
		geometryDescs[i].Triangles.Transform3x4 = 0;	// D3D12_GPU_VIRTUAL_ADDRESS
		geometryDescs[i].Triangles.VertexBuffer = mScene.VertexBufferAddressAndStride(i);
		geometryDescs[i].Triangles.VertexCount = mScene.VertexCount(i);
		geometryDescs[i].Triangles.VertexFormat = mScene.VertexPositionFormat(i);

		// Ϊ��������ܣ������ڲ�Ӱ���������ǰ���½�����primitive��Ϊopaque��
		geometryDescs[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	}
}

AccelerationStructureBuffers RayTracingApp::BuildBottomLevelAS(std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& geometryDescs) {
	// 1. ����Ԥ������Դ��С
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};
	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	bottomLevelInputs.NumDescs = static_cast<UINT>(geometryDescs.size());
	bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	bottomLevelInputs.pGeometryDescs = geometryDescs.data();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
	mDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);

	// 2. Allocate for ScratchBuffer
	ComPtr<ID3D12Resource> scratch;
	Util::AllocateUAVBuffer(mDevice.Get(), mCommandList.Get(),
		bottomLevelPrebuildInfo.ScratchDataSizeInBytes,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		scratch
	);

	// 3. Allocate for BottomLevelAS
	ComPtr<ID3D12Resource> bottomLevelAS;
	Util::AllocateUAVBuffer(mDevice.Get(), mCommandList.Get(),
		bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		bottomLevelAS
	);

	// 4. Build BottomLevelAS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelDesc = {};
	bottomLevelDesc.DestAccelerationStructureData = bottomLevelAS->GetGPUVirtualAddress();
	bottomLevelDesc.Inputs = bottomLevelInputs;
	bottomLevelDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();

	mCommandList->BuildRaytracingAccelerationStructure(&bottomLevelDesc, 0, nullptr);


	AccelerationStructureBuffers bottomLevelBuffers;
	bottomLevelBuffers.accelerationStructure = bottomLevelAS;
	bottomLevelBuffers.ResultDataMaxSizeInBytes = bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes;
	bottomLevelBuffers.scratch = scratch;
	return bottomLevelBuffers;
}

D3D12_GPU_VIRTUAL_ADDRESS RayTracingApp::BuildInstanceDescForTopLevelAS(D3D12_GPU_VIRTUAL_ADDRESS bottomLevelASAddress) {
	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
	instanceDescs.resize(1);

	instanceDescs[0].AccelerationStructure = bottomLevelASAddress;
	instanceDescs[0].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	instanceDescs[0].InstanceContributionToHitGroupIndex = 0;
	instanceDescs[0].InstanceID = 0;
	instanceDescs[0].InstanceMask = 1;

	XMMATRIX transform = XMMatrixIdentity();
	XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDescs[0].Transform), transform);	// FLOAT

	// ��ʵ���������ϴ���GPU
	UploadBuffer<D3D12_RAYTRACING_INSTANCE_DESC> buffer(mDevice.Get(), instanceDescs.size(), false);
	for (UINT i = 0; i < instanceDescs.size(); ++i) {
		buffer.Copydata(i, instanceDescs[i]);
	}

	return buffer.GetBufferPointer();
}

AccelerationStructureBuffers RayTracingApp::BuildTopLevelAS(D3D12_GPU_VIRTUAL_ADDRESS instanceAddress) {
	// 1. ����Ԥ������Դ��С
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	topLevelInputs.NumDescs = 1;
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.InstanceDescs = instanceAddress;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
	mDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);

	// 2. Allocate for ScratchBuffer
	ComPtr<ID3D12Resource> scratch;
	Util::AllocateUAVBuffer(mDevice.Get(), mCommandList.Get(),
		topLevelPrebuildInfo.ScratchDataSizeInBytes,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		scratch
	);

	// 3. Allocate for TopLevelAS
	ComPtr<ID3D12Resource> topLevelAS;
	Util::AllocateUAVBuffer(mDevice.Get(), mCommandList.Get(),
		topLevelPrebuildInfo.ResultDataMaxSizeInBytes,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		topLevelAS
	);

	// 4. Build TopLevelAS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelDesc = {};
	topLevelDesc.DestAccelerationStructureData = topLevelAS->GetGPUVirtualAddress();
	topLevelDesc.Inputs = topLevelInputs;
	topLevelDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();

	mCommandList->BuildRaytracingAccelerationStructure(&topLevelDesc, 0, nullptr);

	// 5. ret
	AccelerationStructureBuffers topLevelBuffers;
	topLevelBuffers.accelerationStructure = topLevelAS;
	topLevelBuffers.ResultDataMaxSizeInBytes = topLevelPrebuildInfo.ResultDataMaxSizeInBytes;
	topLevelBuffers.scratch = scratch;
	return topLevelBuffers;
}

void RayTracingApp::BuildAccelerationStructures() {
	// Reset CommandList
	mCommandList->Reset(mCommandAllocator.Get(), nullptr);

	// 1. Build Bottom Level Acceleration Structure
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
	BuildGeometryDescForBottomLevelAS(geometryDescs);
	AccelerationStructureBuffers bottomLevelBuffers =  BuildBottomLevelAS(geometryDescs);


	// 2. Build Top Level Acceleration Structure
	D3D12_GPU_VIRTUAL_ADDRESS instanceAddress = BuildInstanceDescForTopLevelAS(bottomLevelBuffers.accelerationStructure->GetGPUVirtualAddress());
	AccelerationStructureBuffers topLevelBuffers = BuildTopLevelAS(instanceAddress);

	// 3. ������ɣ���Resource StateתΪUAV����׷ʹ��
	// Ϊʲôֻת��BottomLevelAS��
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(bottomLevelBuffers.accelerationStructure.Get());
	mCommandList->ResourceBarrier(1, &barrier);

	// 4. ��GPUִ�й������裬���ȴ������
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();
}

void RayTracingApp::BuildShaderTable() {
	ComPtr<ID3D12StateObjectProperties> rtpsoProp;
	mRTPSO->QueryInterface(IID_PPV_ARGS(&rtpsoProp));

	const UINT shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	// Ray Generation Shader Table
	{
		const UINT shaderRecordSize = shaderIdentifierSize;
		typedef ShaderRecord<shaderRecordSize> RaygenShaderRecord;
		ShaderTable<RaygenShaderRecord> rayGenTable();

		RaygenShaderRecord record;
		//rayGenTable.Copydata(0, record);
	}

	// Miss Shader Table
	{
		const UINT shaderRecordSize = shaderIdentifierSize;
		typedef ShaderRecord<shaderRecordSize> MissShaderRecord;
		ShaderTable<MissShaderRecord> missTable();

		MissShaderRecord record;
	}

	// Hit Group Table 
	{
		UINT size = 0;
		for (UINT i = 0; i < size; ++i) {
			ShaderRecord<D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES> record;
			record.SetIdentifier(rtpsoProp->GetShaderIdentifier(mRaygenShaderName));
		}
	}

}
