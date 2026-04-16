#pragma once
#include "pch.h"

class D3DCore {
public:
    static const UINT swapChainBufferCnt_ = 2;

    D3DCore() = default;
    ~D3DCore() = default;

    bool Initialize(HWND hWnd, int width, int height);
    void Shutdown();

    void ChangeSwapChainState();

    void WaitForGpuComplete();
    void MoveToNextFrame();
    void Present(UINT syncInterval = 0, UINT flags = 0);

    void ResetCommandList();
    void ExecuteCommandList();

    void BeginRender(const float clearColor[4]);
    void EndRender();

    void Resize(UINT width, UINT height);

public:
    UINT GetClientWidth() const { return clientWidth_; }
    UINT GetClientHeight() const { return clientHeight_; }

    ID3D12Device* GetDevice() const { return device_.Get(); }
    IDXGISwapChain3* GetSwapChain() const { return swapChain_.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return cmdList_.Get(); }
    ID3D12CommandQueue* GetCommandQueue() const { return cmdQueue_.Get(); }

    ID3D12Resource* GetCurrentRenderTarget() const { return renderTargetBuffers_[swapChainBufferIndex_].Get(); }
    UINT GetCurrentBackBufferIndex() const { return swapChainBufferIndex_; }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRtvHandle() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() const;

    UINT GetRtvDescriptorIncrementSize() const { return rtvDescriptorIncrementSize_; }
    UINT GetDsvDescriptorIncrementSize() const { return dsvDescriptorIncrementSize_; }

private:
    struct HandleCloser
    {
        void operator()(HANDLE h) const noexcept { if (h) ::CloseHandle(h); }
    };
    using unique_handle = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleCloser>;

private:
    void CreateSwapChain(HWND hWnd, int width, int height);
    void CreateDirect3DDevice();
    void CreateDescriptorHeaps();
    void CreateCommandObjects();
    void CreateRenderTargetViews();
    void CreateDepthStencilView();

    void ReleaseBackBuffers();
    void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

    DXGI_SWAP_CHAIN_DESC CreateSwapChainDesc(HWND hwnd, int width, int height) const;
    D3D12_RESOURCE_DESC CreateDepthStencilResourceDesc() const;
    D3D12_CLEAR_VALUE CreateDepthStencilClearValue() const;

private:
    ComPtr<IDXGIFactory4> factory_;
    ComPtr<IDXGISwapChain3> swapChain_;
    ComPtr<ID3D12Device> device_;

    bool msaaEnable_ = false;
    UINT msaaQualityLevel_ = 0;

    UINT swapChainBufferIndex_ = 0;

    std::array<ComPtr<ID3D12Resource>, swapChainBufferCnt_> renderTargetBuffers_;
    std::array<D3D12_RESOURCE_STATES, swapChainBufferCnt_> renderTargetStates_{};
    
    ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
    UINT rtvDescriptorIncrementSize_ = 0;

    ComPtr<ID3D12Resource> depthStencilBuffer_;
    ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;
    UINT dsvDescriptorIncrementSize_ = 0;

    ComPtr<ID3D12CommandQueue> cmdQueue_;
    ComPtr<ID3D12CommandAllocator> cmdAllocator_;
    ComPtr<ID3D12GraphicsCommandList> cmdList_;

    ComPtr<ID3D12Fence> fence_;
    std::array<UINT64, swapChainBufferCnt_> fenceValues_{};
    unique_handle fenceEvent_{ nullptr };

    UINT clientWidth_ = 0;
    UINT clientHeight_ = 0;
};