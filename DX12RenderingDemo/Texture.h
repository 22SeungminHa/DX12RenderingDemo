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

    void SetSrvIndex(UINT index) { srvIndex_ = index; }
    UINT GetSrvIndex() const { return srvIndex_; }
    bool HasSrvIndex() const { return srvIndex_ != UINT_MAX; }

    void ReleaseUploadBuffer()
    {
        uploadBuffer_.Reset();
    }

private:
    ComPtr<ID3D12Resource> texture_;
    ComPtr<ID3D12Resource> uploadBuffer_;

    UINT srvIndex_ = UINT_MAX;
};