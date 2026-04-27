// header.h: 표준 시스템 포함 파일
// 또는 프로젝트 특정 포함 파일이 들어 있는 포함 파일입니다.
//

#pragma once
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#pragma comment(lib, "dxguid.lib")

#define FRAME_BUFFER_WIDTH 800
#define FRAME_BUFFER_HEIGHT 600

// 정점 색상 랜덤값.
#define RANDOM_COLOR Vector4(rand() / float(RAND_MAX), rand() / float(RAND_MAX), rand() / float(RAND_MAX), rand() / float(RAND_MAX))

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
#include <windows.h>
#include <windowsx.h>
#include <wrl.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <cmath>

#include <shellapi.h>

#include <Mmsystem.h>
#pragma comment(lib, "winmm.lib")


#include <d3d12.h>
#include <dxgi1_4.h>

#include <D3Dcompiler.h>

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>

#include <DXGIDebug.h>

#include "d3dx12.h"

#include "SimpleMath.h"
#include "SimpleMath.inl"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

extern ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nBytes, D3D12_HEAP_TYPE d3dHeapType, D3D12_RESOURCE_STATES d3dResourceStates, ComPtr<ID3D12Resource>& pd3dUploadBuffer);

inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

class d3dUtil
{
public:
    static bool IsKeyDown(int vkeyCode);
    static std::string ToString(HRESULT hr);

    // 상수 버퍼의 크기는 반드시 최소 하드웨어 할당 크기(흔히 256바이트)의 배수여야 한다.
    static UINT CalcConstantBufferByteSize(UINT byteSize) { return (byteSize + 255) & ~255; }

    static ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename);
    static ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer);
    static ComPtr<ID3DBlob> CompileShader(const std::wstring& filename, const D3D_SHADER_MACRO* defines, const std::string& entrypoint, const std::string& target);
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

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
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