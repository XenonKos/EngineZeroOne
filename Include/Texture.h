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

		// 创建WICFactory
		ThrowIfFailed(CoCreateInstance(
			CLSID_WICImagingFactory, 
			nullptr, 
			CLSCTX_INPROC_SERVER, 
			IID_PPV_ARGS(&pIWICFactory)
		));
	}


	ID3D12Resource* LoadTexture(const std::string& path) {
		// 此处需注释ScratchImage的构成
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
		// DDS格式自带mipmap
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
//	// 对path有2点要求：
//	// 1. path需要是绝对路径，否则会找不到文件
//	// 2. path需转换成wstring
//	std::wstring wpath(path.begin(), path.end());
//	ThrowIfFailed(pIWICFactory->CreateDecoderFromFilename(
//		(LPCWSTR)(wpath.c_str()),
//		nullptr,
//		GENERIC_READ,
//		WICDecodeMetadataCacheOnDemand,
//		&pIWICDecoder
//	));
//
//	// 获取第一帧图片(因为GIF等格式文件可能会有多帧图片，其他的格式一般只有一帧图片)
//	// 实际解析出来的往往是位图格式数据
//	ThrowIfFailed(pIWICDecoder->GetFrame(0, &pIWICFrame));
//
//	WICPixelFormatGUID wpf = {};
//	// 获取WIC图片格式
//	ThrowIfFailed(pIWICFrame->GetPixelFormat(&wpf));
//	GUID tgFormat = {};
//
//	// 通过第一道转换之后获取DXGI的等价格式
//	if (Util::GetTargetPixelFormat(&wpf, &tgFormat))
//	{
//		mFormat = Util::GetDXGIFormatFromPixelFormat(&tgFormat);
//	}
//
//	// 不支持的图片格式
//	if (mFormat == DXGI_FORMAT_UNKNOWN) {
//		throw;
//	}
//
//	// 定义一个位图格式的图片数据对象接口
//	ComPtr<IWICBitmapSource> pIBMP;
//
//	if (!InlineIsEqualGUID(wpf, tgFormat))
//	{// 这个判断很重要，如果原WIC格式不是直接能转换为DXGI格式的图片时
//		// 我们需要做的就是转换图片格式为能够直接对应DXGI格式的形式
//		//创建图片格式转换器
//		ComPtr<IWICFormatConverter> pIConverter;
//		ThrowIfFailed(pIWICFactory->CreateFormatConverter(&pIConverter));
//
//		//初始化一个图片转换器，实际也就是将图片数据进行了格式转换
//		ThrowIfFailed(pIConverter->Initialize(
//			pIWICFrame.Get(),                // 输入原图片数据
//			tgFormat,						 // 指定待转换的目标格式
//			WICBitmapDitherTypeNone,         // 指定位图是否有调色板，现代都是真彩位图，不用调色板，所以为None
//			NULL,                            // 指定调色板指针
//			0.f,                             // 指定Alpha阀值
//			WICBitmapPaletteTypeCustom       // 调色板类型，实际没有使用，所以指定为Custom
//		));
//		// 调用QueryInterface方法获得对象的位图数据源接口
//		ThrowIfFailed(pIConverter.As(&pIBMP));
//	}
//	else
//	{
//		//图片数据格式不需要转换，直接获取其位图数据源接口
//		ThrowIfFailed(pIWICFrame.As(&pIBMP));
//	}
//
//	// 获得图片大小（单位：像素）
//	ThrowIfFailed(pIBMP->GetSize(&mWidth, &mHeight));
//
//	//获取图片像素的位大小的BPP（Bits Per Pixel）信息，用以计算图片行数据的真实大小（单位：字节）
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
//	// 到这里终于可以得到BPP了，这也是我看的比较吐血的地方，为了BPP居然绕了这么多环节
//	ThrowIfFailed(pIWICPixelinfo->GetBitsPerPixel(&mBPP));
//
//	// 计算图片实际的行大小（单位：字节）
//	mRowPitch = (uint64_t(mWidth) * uint64_t(mBPP) + 7u) / 8u;
//
//	// 开辟一片内存中的空间准备存储图片
//	ThrowIfFailed(D3DCreateBlob(
//		mRowPitch * mHeight,
//		&mTextureCPU
//	));
//
//	// 将图片拷贝到内存空间中
//	ThrowIfFailed(pIBMP->CopyPixels(
//		nullptr, // 拷贝整张图片，设为nullptr
//		mRowPitch,
//		mTextureCPU->GetBufferSize(),
//		reinterpret_cast<BYTE*>(mTextureCPU->GetBufferPointer())
//	));
//
//	// 上传资源
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