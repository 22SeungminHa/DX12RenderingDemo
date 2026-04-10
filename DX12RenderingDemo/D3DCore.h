#pragma once
#include "pch.h"

class CD3DCore
{
public:
    static const UINT swapChainBufferCount = 2;

private:
    struct HandleCloser
    {
        void operator()(HANDLE h) const noexcept { if (h) ::CloseHandle(h); }
    };
    using unique_handle = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleCloser>;

public:
    CD3DCore();
    ~CD3DCore();

    bool Initialize(HWND hWnd, int width, int height);
    void Shutdown();

    void CreateSwapChain(HWND hWnd, int width, int height);
    void CreateDirect3DDevice();
    void CreateDescriptorHeaps();
    void CreateCommandObjects();
    void CreateRenderTargetViews();
    void CreateDepthStencilView();

    void ChangeSwapChainState();

    void WaitForGpuComplete();
    void MoveToNextFrame();
    void Present(UINT syncInterval = 0, UINT flags = 0);

    void ResetCommandList();
    void ExecuteCommandList();

    void BeginRender(const float clearColor[4]);
    void EndRender();

    void Resize(UINT width, UINT height) { mClientWidth = width; mClientHeight = height; }

public:
    UINT GetClientWidth() const { return mClientWidth; }
    UINT GetClientHeight() const { return mClientHeight; }

    ID3D12Device* GetDevice() const { return mD3DDevice.Get(); }
    IDXGISwapChain3* GetSwapChain() const { return mSwapChain.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return mCommandList.Get(); }
    ID3D12CommandQueue* GetCommandQueue() const { return mCommandQueue.Get(); }

    ID3D12Resource* GetCurrentRenderTarget() const { return mRenderTargetBuffers[mSwapChainBufferIndex].Get(); }
    UINT GetCurrentBackBufferIndex() const { return mSwapChainBufferIndex; }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRtvHandle() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() const;

    UINT GetRtvDescriptorIncrementSize() const { return mRtvDescriptorIncrementSize; }
    UINT GetDsvDescriptorIncrementSize() const { return mDsvDescriptorIncrementSize; }

private:
    ComPtr<IDXGIFactory4> mDXGIFactory;
    ComPtr<IDXGISwapChain3> mSwapChain;
    ComPtr<ID3D12Device> mD3DDevice;

    bool mMSAAEnable = false;
    UINT mMSAAQualityLevels = 0;

    UINT mSwapChainBufferIndex = 0;

    std::array<ComPtr<ID3D12Resource>, swapChainBufferCount> mRenderTargetBuffers;
    ComPtr<ID3D12DescriptorHeap> mRtvDescriptorHeap;
    UINT mRtvDescriptorIncrementSize = 0;

    ComPtr<ID3D12Resource> mDepthStencilBuffer;
    ComPtr<ID3D12DescriptorHeap> mDsvDescriptorHeap;
    UINT mDsvDescriptorIncrementSize = 0;

    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12CommandAllocator> mCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;

    ComPtr<ID3D12Fence> mFence;
    UINT64 mFenceValues[swapChainBufferCount] = {};
    unique_handle mFenceEvent{ nullptr };

    UINT mClientWidth = 0;
    UINT mClientHeight = 0;
};