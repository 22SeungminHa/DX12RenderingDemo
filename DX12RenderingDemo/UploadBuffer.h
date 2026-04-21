#pragma once
#include "pch.h"

template<typename T>
class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer)
    {
        elementByteSize_ = sizeof(T);
        elementCount_ = elementCount;

        if (isConstantBuffer)
            elementByteSize_ = d3dUtil::CalcConstantBufferByteSize(sizeof(T));

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(elementByteSize_ * elementCount);

        ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadBuffer_)));

        ThrowIfFailed(uploadBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData_)));
    }

    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
    ~UploadBuffer()
    {
        if (uploadBuffer_ != nullptr)
            uploadBuffer_->Unmap(0, nullptr);

        mappedData_ = nullptr;
    }

    ID3D12Resource* GetResource()const
    {
        return uploadBuffer_.Get();
    }

    void CopyData(UINT elementIndex, const T& data)
    {
        assert(elementIndex < elementCount_);
        memcpy(&mappedData_[elementIndex * elementByteSize_], &data, sizeof(T));
    }

private:
    ComPtr<ID3D12Resource> uploadBuffer_;
    BYTE* mappedData_ = nullptr;

    UINT elementByteSize_ = 0;
    UINT elementCount_ = 0;
};