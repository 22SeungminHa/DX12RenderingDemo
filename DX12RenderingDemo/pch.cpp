#include "pch.h"
#include <comdef.h>
#include <fstream>

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
    ErrorCode(hr),
    FunctionName(functionName),
    Filename(filename),
    LineNumber(lineNumber)
{
}

std::wstring DxException::ToString() const
{
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}

ComPtr<ID3D12Resource> D3DUtil::CreateBufferResource(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* data,
    UINT64 byteSize,
    D3D12_HEAP_TYPE heapType,
    D3D12_RESOURCE_STATES finalState,
    ComPtr<ID3D12Resource>& uploadBuffer) 
{
    if (!device || byteSize == 0)
        return nullptr;

    if (heapType == D3D12_HEAP_TYPE_DEFAULT && data && !cmdList)
        return nullptr;

    ComPtr<ID3D12Resource> buffer;
    uploadBuffer.Reset();

    D3D12_HEAP_PROPERTIES heapDesc{};
    heapDesc.Type = heapType;
    heapDesc.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapDesc.CreationNodeMask = 1;
    heapDesc.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = byteSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_COMMON;

    if (heapType == D3D12_HEAP_TYPE_UPLOAD)
        resourceStates = D3D12_RESOURCE_STATE_GENERIC_READ;
    else if (heapType == D3D12_HEAP_TYPE_READBACK)
        resourceStates = D3D12_RESOURCE_STATE_COPY_DEST;

    ThrowIfFailed(device->CreateCommittedResource(&heapDesc, D3D12_HEAP_FLAG_NONE, &resourceDesc, resourceStates, nullptr, IID_PPV_ARGS(buffer.GetAddressOf())));

    if (data) {
        switch (heapType) {
        case D3D12_HEAP_TYPE_DEFAULT: {
            heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;

            ThrowIfFailed(device->CreateCommittedResource(
                &heapDesc,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

            D3D12_RANGE range{ 0, 0 };
            UINT8* bufferDataBegin = nullptr;
            ThrowIfFailed(uploadBuffer->Map(0, &range, reinterpret_cast<void**>(&bufferDataBegin)));
            memcpy(bufferDataBegin, data, byteSize);
            uploadBuffer->Unmap(0, nullptr);

            D3D12_RESOURCE_BARRIER copyDestBarrier{};
            copyDestBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            copyDestBarrier.Transition.pResource = buffer.Get();
            copyDestBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
            copyDestBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
            copyDestBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            cmdList->ResourceBarrier(1, &copyDestBarrier);

            cmdList->CopyResource(buffer.Get(), uploadBuffer.Get());

            D3D12_RESOURCE_BARRIER resourceBarrier{};
            resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            resourceBarrier.Transition.pResource = buffer.Get();
            resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            resourceBarrier.Transition.StateAfter = finalState;
            resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            cmdList->ResourceBarrier(1, &resourceBarrier);
            break;
        }
        case D3D12_HEAP_TYPE_UPLOAD: {
            D3D12_RANGE range{ 0, 0 };
            UINT8* bufferDataBegin = nullptr;
            ThrowIfFailed(buffer->Map(0, &range, reinterpret_cast<void**>(&bufferDataBegin)));
            memcpy(bufferDataBegin, data, byteSize);
            buffer->Unmap(0, nullptr);
            break;
        }
        case D3D12_HEAP_TYPE_READBACK:
            break;
        }
    }

    return buffer;
}

ComPtr<ID3DBlob> D3DUtil::CompileShader(
    const std::filesystem::path& filePath,
    const D3D_SHADER_MACRO* defines,
    const std::string& entrypoint,
    const std::string& target)
{
    UINT compileFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> byteCode;
    ComPtr<ID3DBlob> errors;

    HRESULT hr = D3DCompileFromFile(
        filePath.c_str(),
        defines,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entrypoint.c_str(),
        target.c_str(),
        compileFlags,
        0,
        byteCode.GetAddressOf(),
        errors.GetAddressOf()
    );

    if (errors)
        OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer()));

    ThrowIfFailed(hr);

    return byteCode;
}

void D3DUtil::InitCbv(
    D3D12_ROOT_PARAMETER& parameter,
    UINT shaderRegister,
    D3D12_SHADER_VISIBILITY visibility,
    UINT registerSpace)
{
    parameter = {};

    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameter.Descriptor.ShaderRegister = shaderRegister;
    parameter.Descriptor.RegisterSpace = registerSpace;
    parameter.ShaderVisibility = visibility;
}

void D3DUtil::InitSrvTable(
    D3D12_ROOT_PARAMETER& parameter,
    D3D12_DESCRIPTOR_RANGE& range,
    UINT baseShaderRegister,
    UINT descriptorCount,
    D3D12_SHADER_VISIBILITY visibility,
    UINT registerSpace)
{
    range = {};

    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = descriptorCount;
    range.BaseShaderRegister = baseShaderRegister;
    range.RegisterSpace = registerSpace;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    parameter = {};

    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

    parameter.DescriptorTable.NumDescriptorRanges = 1;
    parameter.DescriptorTable.pDescriptorRanges = &range;
    parameter.ShaderVisibility = visibility;
}