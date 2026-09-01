// header.h: 표준 시스템 포함 파일
// 또는 프로젝트 특정 포함 파일이 들어 있는 포함 파일입니다.
//

#pragma once
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#include "targetver.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <wrl.h>

#include <tchar.h>

#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <filesystem>
#include <memory>
#include <algorithm>
#include <cstdint>

#include <cstdio>
#include <iostream>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>

#include "d3dx12.h"

#include "SimpleMath.h"
#include "SimpleMath.inl"

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

inline constexpr UINT kFrameBufferWidth = 800;
inline constexpr UINT kFrameBufferHeight = 600;

inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

class AssetPath
{
public:
    static std::filesystem::path FBX(const std::wstring& name) { return std::filesystem::path(L"../Assets/Meshes") / (name + L".fbx"); }
    static std::filesystem::path OBJ(const std::wstring& name) { return std::filesystem::path(L"../Assets/Meshes") / (name + L".obj"); }
    static std::filesystem::path Texture(const std::wstring& name) { return std::filesystem::path(L"../Assets/Textures") / (name + L".dds"); }
    static std::filesystem::path Material(const std::wstring& name) { return std::filesystem::path(L"../Assets/Materials") / (name + L".mat"); }
    static std::filesystem::path Data(const std::wstring& name) { return std::filesystem::path(L"../Assets/Data") / (name + L".json"); }
    static std::filesystem::path ShaderFile(const std::wstring& name) { return std::filesystem::path(L"../Shaders") / (name + L".hlsl"); }
    static std::string Key(const std::filesystem::path& path) { return path.lexically_normal().generic_string(); }
};

class D3DUtil
{
public:
    static UINT CalcConstantBufferByteSize(UINT byteSize) { return (byteSize + 255) & ~255; }

    static ComPtr<ID3DBlob> CompileShader(
        const std::filesystem::path& filePath,
        const D3D_SHADER_MACRO* defines,
        const std::string& entrypoint,
        const std::string& target);

    static ComPtr<ID3D12Resource> CreateBufferResource(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* data,
        UINT64 byteSize,
        D3D12_HEAP_TYPE heapType,
        D3D12_RESOURCE_STATES finalState,
        ComPtr<ID3D12Resource>& uploadBuffer);

    static void InitCbv(
        D3D12_ROOT_PARAMETER& parameter,
        UINT shaderRegister,
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL,
        UINT registerSpace = 0);

    static void InitSrvTable(
        D3D12_ROOT_PARAMETER& parameter,
        D3D12_DESCRIPTOR_RANGE& range,
        UINT baseShaderRegister,
        UINT descriptorCount = 1,
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_PIXEL,
        UINT registerSpace = 0);
};

class DxException
{
public:
    DxException() = default;
    DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

    std::wstring ToString() const;

    HRESULT ErrorCode = S_OK;
    std::wstring FunctionName;
    std::wstring Filename;
    int LineNumber = -1;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                                  \
{                                                                         \
    HRESULT hr__ = (x);                                                   \
    std::wstring wfn = AnsiToWString(__FILE__);                           \
    if (FAILED(hr__)) { throw DxException(hr__, L## #x, wfn, __LINE__); } \
}
#endif

#ifndef ThrowIfFailedWithBlob
#define ThrowIfFailedWithBlob(hrExpr, errorBlob)                                           \
{                                                                                          \
    HRESULT hr__ = (hrExpr);                                                               \
    if (FAILED(hr__)) {                                                                    \
        if ((errorBlob)) {                                                                 \
            OutputDebugStringA(static_cast<const char*>((errorBlob)->GetBufferPointer())); \
            OutputDebugStringA("\n");                                                      \
        }                                                                                  \
        std::wstring wfn = AnsiToWString(__FILE__);                                        \
        throw DxException(hr__, L## #hrExpr, wfn, __LINE__);                               \
    }                                                                                      \
}
#endif

#ifdef _DEBUG
#define OPEN_DEBUG_CONSOLE()                         \
{                                                    \
    AllocConsole();                                  \
    FILE* fp;                                        \
    freopen_s(&fp, "CONOUT$", "w", stdout);          \
    freopen_s(&fp, "CONOUT$", "w", stderr);          \
    freopen_s(&fp, "CONIN$",  "r", stdin);           \
    std::ios::sync_with_stdio();                     \
}
#define CLOSE_DEBUG_CONSOLE() FreeConsole()
#define LOG(x) std::cout << x << std::endl

#else
#define OPEN_DEBUG_CONSOLE()
#define CLOSE_DEBUG_CONSOLE()
#define LOG(x)

#endif