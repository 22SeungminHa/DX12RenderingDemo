#pragma once
#include "DescriptorAllocator.h"

class RenderTexture
{
public:
    RenderTexture() = default;
    ~RenderTexture() = default;

    bool Create(
        ID3D12Device* device,
        UINT width,
        UINT height,
        DXGI_FORMAT format,
        const DescriptorAllocation& srvAllocation,
        const float clearColor[4]);

    void Release();

    void Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES afterState);

    ID3D12Resource* GetResource() const { return resource_.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRtv() const { return rtv_; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrv() const { return srv_; }
    D3D12_RESOURCE_STATES GetState() const { return state_; }

private:
    ComPtr<ID3D12Resource> resource_;
    ComPtr<ID3D12DescriptorHeap> rtvHeap_;

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_{};
    D3D12_GPU_DESCRIPTOR_HANDLE srv_{};

    D3D12_RESOURCE_STATES state_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
};