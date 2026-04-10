#include "pch.h"
#include <comdef.h>
#include <fstream>

ID3D12Resource* CreateBufferResource(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nBytes, D3D12_HEAP_TYPE d3dHeapType, D3D12_RESOURCE_STATES d3dResourceStates, ID3D12Resource** ppd3dUploadBuffer)
{
    ID3D12Resource* pd3dBuffer = NULL;

    D3D12_HEAP_PROPERTIES d3dHeapPropertiesDesc{};
    d3dHeapPropertiesDesc.Type = d3dHeapType;
    d3dHeapPropertiesDesc.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    d3dHeapPropertiesDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    d3dHeapPropertiesDesc.CreationNodeMask = 1;
    d3dHeapPropertiesDesc.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC d3dResourceDesc{};
    d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    d3dResourceDesc.Alignment = 0;
    d3dResourceDesc.Width = nBytes;
    d3dResourceDesc.Height = 1;
    d3dResourceDesc.DepthOrArraySize = 1;
    d3dResourceDesc.MipLevels = 1;
    d3dResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    d3dResourceDesc.SampleDesc.Count = 1;
    d3dResourceDesc.SampleDesc.Quality = 0;
    d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_RESOURCE_STATES d3dResourceInitialStates = D3D12_RESOURCE_STATE_COPY_DEST;
    if (d3dHeapType == D3D12_HEAP_TYPE_UPLOAD)
        d3dResourceInitialStates = D3D12_RESOURCE_STATE_GENERIC_READ;
    else if (d3dHeapType == D3D12_HEAP_TYPE_READBACK)
        d3dResourceInitialStates = D3D12_RESOURCE_STATE_COPY_DEST;
    HRESULT hResult = pd3dDevice->CreateCommittedResource(
        &d3dHeapPropertiesDesc,
        D3D12_HEAP_FLAG_NONE,
        &d3dResourceDesc,
        d3dResourceInitialStates,
        NULL,
        __uuidof(ID3D12Resource),
        (void**)&pd3dBuffer);
    if (pData)
    {
        switch (d3dHeapType)
        {
        case D3D12_HEAP_TYPE_DEFAULT:
        {
            if (ppd3dUploadBuffer)
            {
                //업로드 버퍼를 생성한다.
                d3dHeapPropertiesDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
                pd3dDevice->CreateCommittedResource(
                    &d3dHeapPropertiesDesc,
                    D3D12_HEAP_FLAG_NONE,
                    &d3dResourceDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    NULL,
                    __uuidof(ID3D12Resource),
                    (void**)ppd3dUploadBuffer);
                //업로드 버퍼를 매핑하여 초기화 데이터를 업로드 버퍼에 복사한다.
                D3D12_RANGE d3dReadRange = { 0, 0 };
                UINT8* pBufferDataBegin = NULL;
                (*ppd3dUploadBuffer)->Map(0, &d3dReadRange, (void**)&pBufferDataBegin);
                memcpy(pBufferDataBegin, pData, nBytes);
                (*ppd3dUploadBuffer)->Unmap(0, NULL);
                //업로드 버퍼의 내용을 디폴트 버퍼에 복사한다.
                pd3dCommandList->CopyResource(pd3dBuffer, *ppd3dUploadBuffer);
                D3D12_RESOURCE_BARRIER d3dResourceBarrier{};
                d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                d3dResourceBarrier.Transition.pResource = pd3dBuffer;
                d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                d3dResourceBarrier.Transition.StateAfter = d3dResourceStates;
                d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);
            }
            break;
        }
        case D3D12_HEAP_TYPE_UPLOAD:
        {
            D3D12_RANGE d3dReadRange = { 0, 0 };
            UINT8* pBufferDataBegin = NULL;
            pd3dBuffer->Map(0, &d3dReadRange, (void**)&pBufferDataBegin);
            memcpy(pBufferDataBegin, pData, nBytes);
            pd3dBuffer->Unmap(0, NULL);
            break;
        }
        case D3D12_HEAP_TYPE_READBACK:
            break;
        }
    }
    return(pd3dBuffer);
}

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
    ErrorCode(hr),
    FunctionName(functionName),
    Filename(filename),
    LineNumber(lineNumber)
{
}

bool d3dUtil::IsKeyDown(int vkeyCode)
{
    return (GetAsyncKeyState(vkeyCode) & 0x8000) != 0;
}

// 컴파일된 셰이더 바이트코드를 C++ 표준 입출력 라이브러리를 이용해서 적재한다.
ComPtr<ID3DBlob> d3dUtil::LoadBinary(const std::wstring& filename)
{
    std::ifstream fin(filename, std::ios::binary);

    fin.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = (int)fin.tellg();
    fin.seekg(0, std::ios_base::beg);

    ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

    fin.read((char*)blob->GetBufferPointer(), size);
    fin.close();

    return blob;
}

// 편의용 기본 버퍼 리소스 초기화 함수
Microsoft::WRL::ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC defaultBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

    // 실제 기본 버퍼 자원을 생성한다.
    ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &defaultBufferDesc,
		D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

    // CPU 메모리의 리소스를 기본 버퍼에 복사하려면 임시 업로드 힙을 만들어야 한다.
    ThrowIfFailed(device->CreateCommittedResource(
        &uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


    // 기본 버퍼에 복사할 자료를 서술한다.
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // 기본 버퍼 리소스로의 리소스 복사를 요청한다.
    // 개략적으로 말하자면, 보조 함수 UpdateSubresources는 CPU 메모리를 임시 업로드 힙에 복사하고,
    // ID3D12CommandList::CopySubresourceRegion을 이용해서 임시 업로드 힙의 자료를 mBuffer에 복사한다.
    CD3DX12_RESOURCE_BARRIER barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
        defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->ResourceBarrier(1, &barrier1);

    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

    CD3DX12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
        defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_GENERIC_READ);
    cmdList->ResourceBarrier(1, &barrier2);

    // 주의: 위의 함수 호출 이후에도 uploadBuffer를 계속 유지해야 한다.
    // 실제로 복사를 수행하는 커멘드 리스트가 아직 실행되지 않았기 때문이다.
    // 복사가 완료되었음이 확실해진 후에 호출자가 uploadBuffer를 해제하면 된다.

    return defaultBuffer;
}

// 실행 시점에서 셰이더 프로그램을 좀 더 손쉽게 컴파일하기 위한 보조 함수
ComPtr<ID3DBlob> d3dUtil::CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
    // 디버그 모드에서는 디버깅 관련 플래그들을 사용한다.
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(
        filename.c_str(),
        defines,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entrypoint.c_str(),
        target.c_str(),
        compileFlags, 0,
        &byteCode,
        &errors);

    // 오류 메시지를 디버그 창에 출력한다.
	if(errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	ThrowIfFailed(hr);

	return byteCode;
}

std::wstring DxException::ToString() const
{
    // Get the string description of the error code.
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}
