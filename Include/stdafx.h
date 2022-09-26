#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif


// Use the C++ standard templated min/max
#define NOMINMAX

#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>

#include <list>
#include <string>
#include <wrl.h>
#include <shellapi.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <atlbase.h>
#include <assert.h>
#include <array>
#include <unordered_map>
#include <functional>
#include <random>
#include <numeric>
#include <iterator>
#include <sal.h>
#include <stack>

#include <stdint.h>
#include <float.h>
#include <map>
#include <set>

#include <dxgi1_6.h>
#include <d3d12.h>
#include <atlbase.h>
#include "d3dx12.h"	
//#include <pix3.h>

#include <DirectXMath.h>
//#include <WICTextureLoader.h>
//#include <DDSTextureLoader.h>
//#include "ResourceUploadBatch.h"
#include <DirectXTK12/ResourceUploadBatch.h>

using namespace DirectX;

class HrException : public std::runtime_error
{
    inline std::string HrToString(HRESULT hr)
    {
        char s_str[64] = {};
        sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
        return std::string(s_str);
    }
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }
private:
    const HRESULT m_hr;
};

#define SAFE_RELEASE(p) if (p) (p)->Release()


inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        DebugBreak();
        throw HrException(hr);
    }
}

inline void ThrowIfFailed(HRESULT hr, const wchar_t* msg)
{
    if (FAILED(hr))
    {
        OutputDebugString(msg);
        DebugBreak();
        throw HrException(hr);
    }
}

template<typename... Args>
inline void ThrowIfFailed(HRESULT hr, const wchar_t* format, Args... args)
{
    if (FAILED(hr))
    {
        WCHAR msg[128];
        swprintf_s(msg, format, args...);
        OutputDebugString(msg);
        DebugBreak();
        throw HrException(hr);
    }
}

inline void ThrowIfFalse(bool value)
{
    ThrowIfFailed(value ? S_OK : E_FAIL);
}

inline void ThrowIfFalse(bool value, const wchar_t* msg)
{
    ThrowIfFailed(value ? S_OK : E_FAIL, msg);
}


template<typename... Args>
inline void ThrowIfFalse(bool value, const wchar_t* format, Args... args)
{
    ThrowIfFailed(value ? S_OK : E_FAIL, format, args...);
}

inline void ThrowIfTrue(bool value)
{
    ThrowIfFalse(!value);
}

inline void ThrowIfTrue(bool value, const wchar_t* msg)
{
    ThrowIfFalse(!value, msg);
}