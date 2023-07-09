#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>
#include <DirectXCollision.h>
#include "d3dx12.h"

#include "DirectXTK12/DDSTextureLoader.h"
#include "DirectXTK12/WICTextureLoader.h"

#include <DirectXTex.h>
#include <DirectXTexEXR.h>

#include <wincodec.h>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "D3D12App.h"

struct SubTexture {
	UINT Width = 0;
	UINT Height = 0;
	UINT BPP = 0;

	DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
};

class Texture {
public:
	Texture(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> cmdList)
		: mDevice(device),
		mCommandList(cmdList) {

		// ����WICFactory
		ThrowIfFailed(CoCreateInstance(
			CLSID_WICImagingFactory, 
			nullptr, 
			CLSCTX_INPROC_SERVER, 
			IID_PPV_ARGS(&pIWICFactory)
		));
	}


	ID3D12Resource* LoadTexture(const std::string& path) {
		// �˴���ע��ScratchImage�Ĺ���
		ScratchImage baseImage;
		std::wstring wpath(path.begin(), path.end());

		// DDS File
		if (path.find(".dds") != std::string::npos) {
			ThrowIfFailed(LoadFromDDSFile(
				wpath.c_str(),
				DDS_FLAGS_FORCE_RGB,
				nullptr,
				baseImage
			));
		}
		// HDR File
		else if (path.find(".hdr") != std::string::npos) {
			ThrowIfFailed(LoadFromHDRFile(
				wpath.c_str(),
				nullptr,
				baseImage
			));
		}
		// TGA File
		else if (path.find(".tga") != std::string::npos) {
			ThrowIfFailed(LoadFromTGAFile(
				wpath.c_str(),
				nullptr,
				baseImage
			));
		}
		// EXR File
		else if (path.find(".exr") != std::string::npos) {
			ThrowIfFailed(LoadFromEXRFile(
				wpath.c_str(),
				nullptr,
				baseImage
			));
		}
		// WIC Supported File
		else {
			ThrowIfFailed(LoadFromWICFile(
				wpath.c_str(),
				WIC_FLAGS_FORCE_RGB,
				nullptr,
				baseImage
			));
		}

		ScratchImage mipChain;
		// DDS��ʽ�Դ�mipmap
		if (path.find(".dds") == std::string::npos) {
			ThrowIfFailed(GenerateMipMaps(baseImage.GetImages(), baseImage.GetImageCount(), baseImage.GetMetadata(),
				TEX_FILTER_DEFAULT, 0, mipChain));
		}
		else {
			mipChain = std::move(baseImage);
		}

		// Uploading
		if (mipChain.GetMetadata().IsCubemap()) {
			Util::UploadTextureCubeResource(
				mDevice.Get(), mCommandList.Get(),
				&mipChain,
				mTextureGPU,
				mTextureUploader
			);
		}
		else {
			Util::UploadTexture2DResource(
				mDevice.Get(), mCommandList.Get(),
				&mipChain,
				mTextureGPU,
				mTextureUploader
			);
		}

		return mTextureGPU.Get();
	}

	ID3D12Resource* Resource() const {
		return mTextureGPU.Get();
	}

private:
	ComPtr<ID3D12Resource> mTextureGPU;
	ComPtr<ID3D12Resource> mTextureUploader;

	UINT mWidth = 0;
	UINT mHeight = 0;
	UINT mBPP = 0;
	UINT mRowPitch = 0;

	DXGI_FORMAT mFormat = DXGI_FORMAT_UNKNOWN;

	// WIC
	ComPtr<IWICImagingFactory>			pIWICFactory;
	ComPtr<IWICBitmapDecoder>			pIWICDecoder;
	ComPtr<IWICBitmapFrameDecode>		pIWICFrame;

	// Device and CommandList
	ComPtr<ID3D12Device> mDevice;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;
};


// Old Version
//ID3D12Resource* LoadTexture(const std::string& path, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) {
//	// Decoder
//	// ��path��2��Ҫ��
//	// 1. path��Ҫ�Ǿ���·����������Ҳ����ļ�
//	// 2. path��ת����wstring
//	std::wstring wpath(path.begin(), path.end());
//	ThrowIfFailed(pIWICFactory->CreateDecoderFromFilename(
//		(LPCWSTR)(wpath.c_str()),
//		nullptr,
//		GENERIC_READ,
//		WICDecodeMetadataCacheOnDemand,
//		&pIWICDecoder
//	));
//
//	// ��ȡ��һ֡ͼƬ(��ΪGIF�ȸ�ʽ�ļ����ܻ��ж�֡ͼƬ�������ĸ�ʽһ��ֻ��һ֡ͼƬ)
//	// ʵ�ʽ���������������λͼ��ʽ����
//	ThrowIfFailed(pIWICDecoder->GetFrame(0, &pIWICFrame));
//
//	WICPixelFormatGUID wpf = {};
//	// ��ȡWICͼƬ��ʽ
//	ThrowIfFailed(pIWICFrame->GetPixelFormat(&wpf));
//	GUID tgFormat = {};
//
//	// ͨ����һ��ת��֮���ȡDXGI�ĵȼ۸�ʽ
//	if (Util::GetTargetPixelFormat(&wpf, &tgFormat))
//	{
//		mFormat = Util::GetDXGIFormatFromPixelFormat(&tgFormat);
//	}
//
//	// ��֧�ֵ�ͼƬ��ʽ
//	if (mFormat == DXGI_FORMAT_UNKNOWN) {
//		throw;
//	}
//
//	// ����һ��λͼ��ʽ��ͼƬ���ݶ���ӿ�
//	ComPtr<IWICBitmapSource> pIBMP;
//
//	if (!InlineIsEqualGUID(wpf, tgFormat))
//	{// ����жϺ���Ҫ�����ԭWIC��ʽ����ֱ����ת��ΪDXGI��ʽ��ͼƬʱ
//		// ������Ҫ���ľ���ת��ͼƬ��ʽΪ�ܹ�ֱ�Ӷ�ӦDXGI��ʽ����ʽ
//		//����ͼƬ��ʽת����
//		ComPtr<IWICFormatConverter> pIConverter;
//		ThrowIfFailed(pIWICFactory->CreateFormatConverter(&pIConverter));
//
//		//��ʼ��һ��ͼƬת������ʵ��Ҳ���ǽ�ͼƬ���ݽ����˸�ʽת��
//		ThrowIfFailed(pIConverter->Initialize(
//			pIWICFrame.Get(),                // ����ԭͼƬ����
//			tgFormat,						 // ָ����ת����Ŀ���ʽ
//			WICBitmapDitherTypeNone,         // ָ��λͼ�Ƿ��е�ɫ�壬�ִ��������λͼ�����õ�ɫ�壬����ΪNone
//			NULL,                            // ָ����ɫ��ָ��
//			0.f,                             // ָ��Alpha��ֵ
//			WICBitmapPaletteTypeCustom       // ��ɫ�����ͣ�ʵ��û��ʹ�ã�����ָ��ΪCustom
//		));
//		// ����QueryInterface������ö����λͼ����Դ�ӿ�
//		ThrowIfFailed(pIConverter.As(&pIBMP));
//	}
//	else
//	{
//		//ͼƬ���ݸ�ʽ����Ҫת����ֱ�ӻ�ȡ��λͼ����Դ�ӿ�
//		ThrowIfFailed(pIWICFrame.As(&pIBMP));
//	}
//
//	// ���ͼƬ��С����λ�����أ�
//	ThrowIfFailed(pIBMP->GetSize(&mWidth, &mHeight));
//
//	//��ȡͼƬ���ص�λ��С��BPP��Bits Per Pixel����Ϣ�����Լ���ͼƬ�����ݵ���ʵ��С����λ���ֽڣ�
//	ComPtr<IWICComponentInfo> pIWICmntinfo;
//	ThrowIfFailed(pIWICFactory->CreateComponentInfo(tgFormat, pIWICmntinfo.GetAddressOf()));
//
//	WICComponentType type;
//	ThrowIfFailed(pIWICmntinfo->GetComponentType(&type));
//
//	if (type != WICPixelFormat)
//	{
//		throw;
//	}
//
//	ComPtr<IWICPixelFormatInfo> pIWICPixelinfo;
//	ThrowIfFailed(pIWICmntinfo.As(&pIWICPixelinfo));
//
//	// ���������ڿ��Եõ�BPP�ˣ���Ҳ���ҿ��ıȽ���Ѫ�ĵط���Ϊ��BPP��Ȼ������ô�໷��
//	ThrowIfFailed(pIWICPixelinfo->GetBitsPerPixel(&mBPP));
//
//	// ����ͼƬʵ�ʵ��д�С����λ���ֽڣ�
//	mRowPitch = (uint64_t(mWidth) * uint64_t(mBPP) + 7u) / 8u;
//
//	// ����һƬ�ڴ��еĿռ�׼���洢ͼƬ
//	ThrowIfFailed(D3DCreateBlob(
//		mRowPitch * mHeight,
//		&mTextureCPU
//	));
//
//	// ��ͼƬ�������ڴ�ռ���
//	ThrowIfFailed(pIBMP->CopyPixels(
//		nullptr, // ��������ͼƬ����Ϊnullptr
//		mRowPitch,
//		mTextureCPU->GetBufferSize(),
//		reinterpret_cast<BYTE*>(mTextureCPU->GetBufferPointer())
//	));
//
//	// �ϴ���Դ
//	D3D12_SUBRESOURCE_FOOTPRINT footPrint = { mFormat, mWidth, mHeight, 1, mRowPitch };
//	Util::UploadTexture2DResource(device, cmdList,
//		mTextureCPU->GetBufferPointer(),
//		footPrint,
//		mTextureGPU,
//		mTextureUploader
//	);
//
//	return mTextureGPU.Get();
//}