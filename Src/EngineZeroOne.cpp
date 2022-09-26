// EngineZeroOne.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "EngineZeroOne.h"
#include "BoxApp.h"
#include "SceneApp.h"

// Dear ImGui: standalone example application for DirectX 12
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// Important: to compile on 32-bit systems, the DirectX12 backend requires code to be compiled with '#define ImTextureID ImU64'.
// This is because we need ImTextureID to carry a 64-bit value and by default ImTextureID is defined as void*.
// This define is set in the example .vcxproj file and need to be replicated in your app or by adding it to your imconfig.h file.


#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    //BoxApp theApp(hInstance);
    //try
    //{
    //    if (!theApp.Init())
    //        return 0;

    //    return theApp.Run();
    //}
    SceneApp app(hInstance);
    try {
        if (!app.Init())
            return 1;

        // 加载模型
        const std::string rockPath = "C:\\Users\\Lenovo\\Desktop\\EngineZeroOne\\Models\\rock\\rock.obj";
        const std::string bustPath = "C:\\Users\\Lenovo\\Desktop\\EngineZeroOne\\Models\\marble_bust_01_4k.fbx\\marble_bust_01_4k.fbx";
        const std::string floorPath = "C:\\Users\\Lenovo\\Desktop\\EngineZeroOne\\Models\\floor\\floor.obj";
        const std::string tablePath = "C:\\Users\\Lenovo\\Desktop\\EngineZeroOne\\Models\\round_wooden_table_01_4k.fbx\\round_wooden_table_01_4k.fbx";
        
        //const std::string sponzaPath = "C:\\Users\\Lenovo\\Desktop\\EngineZeroOne\\Models\\Intel Graphics Sample\\Main.1_Sponza\\NewSponza_Main_Yup_002.fbx";
        const std::string sponzaPath = "C:\\Users\\Lenovo\\Desktop\\EngineZeroOne\\Models\\sponza\\sponza.obj";
        const std::string environmentPath = "C:\\Users\\Lenovo\\Desktop\\EngineZeroOne\\Models\\CubeMap\\snowcube1024.dds";

        app.LoadModel(bustPath);
        app.LoadModel(tablePath);

        //app.LoadModel(sponzaPath);
        app.LoadCubeMap(environmentPath);

        //app.LoadModel(floorPath);
        //app.LoadModel(rockPath);

        return app.Run();
    }
    catch (D3D12Exception& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}
