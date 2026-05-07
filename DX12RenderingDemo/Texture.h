#pragma once
#include "Asset.h"

class Texture : public Asset
{
public:
    Texture() = default;
    virtual ~Texture() = default;

    void LoadDDS(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const std::wstring& filePath
    );

    ID3D12Resource* GetResource() const { return texture_.Get(); }
    ID3D12Resource* GetUploadBuffer() const { return uploadBuffer_.Get(); }

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
    {
        return texture_ ? texture_->GetGPUVirtualAddress() : 0;
    }

    void ReleaseUploadBuffer()
    {
        uploadBuffer_.Reset();
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> texture_;
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer_;
};