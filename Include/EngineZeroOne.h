#pragma once
// D3D12
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>
#include "d3dx12.h"

// Windows
#include <wrl.h>
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <iostream>

// ImGui
#include <imgui.h>
#include "Editor/imgui_impl_win32.h"
#include "Editor/imgui_impl_dx12.h"

// Headers
#include "GameTimer.h"
#include "D3D12Exception.h"
#include "resource.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

// Vertex
//struct Vertex {
//	XMFLOAT3 Position;
//	XMFLOAT4 Color;
//	XMFLOAT3 Normal;
//	XMFLOAT3 Tangent;
//	XMFLOAT3 Bitangent;
//};
