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
        const std::filesystem::path& filePath
    );

    ID3D12Resource* GetResource() const { return texture_.Get(); }
    ID3D12Resource* GetUploadBuffer() const { return uploadBuffer_.Get(); }

    void ReleaseUploadBuffer()
    {
        uploadBuffer_.Reset();
    }

private:
    ComPtr<ID3D12Resource> texture_;
    ComPtr<ID3D12Resource> uploadBuffer_;
};