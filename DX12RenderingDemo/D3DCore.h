#pragma once
#include "stdafx.h"

class CD3DCore
{
public:
    // 스왑 체인 후면 버퍼의 개수
    static const UINT swapChainBufferCount = 2;

public:
    CD3DCore();
    ~CD3DCore();

    bool Initialize(HWND hWnd, int width, int height);
    void Shutdown();

    // 스왑 체인, 디바이스, 서술자 힙, 명령 큐/할당자/리스트를 생성하는 함수이다.
    void CreateSwapChain(HWND hWnd, int width, int height);
    void CreateDirect3DDevice();
    void CreateDescriptorHeaps();
    void CreateCommandObjects();
    // 렌더 타겟 뷰와 깊이-스텐실 뷰를 생성하는 함수이다.
    void CreateRenderTargetViews();
    void CreateDepthStencilView();

    void ChangeSwapChainState();

    void WaitForGpuComplete(); // CPU GPU 동기화
    void MoveToNextFrame();
    void Present(UINT syncInterval = 0, UINT flags = 0);

    void ResetCommandList();
    void ExecuteCommandList();

    void BeginRender(const float clearColor[4]);
    void EndRender();

    void Resize(UINT width, UINT height) { mClientWidth = width; mClientHeight = height; };

public:
    UINT GetClientWidth() const { return mClientWidth; }
    UINT GetClientHeight() const { return mClientHeight; }

    ID3D12Device* GetDevice() const { return mD3DDevice; }
    IDXGISwapChain3* GetSwapChain() const { return mSwapChain; }
    ID3D12GraphicsCommandList* GetCommandList() const { return mCommandList; }
    ID3D12CommandQueue* GetCommandQueue() const { return mCommandQueue; }

    ID3D12Resource* GetCurrentRenderTarget() const { return mRenderTargetBuffers[mSwapChainBufferIndex]; }
    UINT GetCurrentBackBufferIndex() const { return mSwapChainBufferIndex; }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRtvHandle() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() const;

    UINT GetRtvDescriptorIncrementSize() const { return mRtvDescriptorIncrementSize; }
    UINT GetDsvDescriptorIncrementSize() const { return mDsvDescriptorIncrementSize; }

private:
    // DXGI 팩토리 인터페이스에 대한 포인터이다.
    IDXGIFactory4* mDXGIFactory = nullptr;
    // 스왑 체인 인터페이스에 대한 포인터이다. 주로 디스플레이를 제어하기 위해 필요
    IDXGISwapChain3* mSwapChain = nullptr;
    // Direct3D 디바이스 인터페이스에 대한 포인터이다. 주로 리소스를 생성하기 위해서 필요
    ID3D12Device* mD3DDevice = nullptr;

    // MSAA 다중 샘플링 활성화, 다중 샘플링 레벨 설정
    bool mMSAAEnable = false;
    UINT mMSAAQualityLevels = 0;

    // 현재 스왑 체인의 후면 버퍼 인덱스이다.
    UINT mSwapChainBufferIndex = 0;

    // 렌더 타겟 버퍼, 서술자 힙 인터페이스 포인터, 렌더 타겟 서술자 원소의 크기이다.
    ID3D12Resource* mRenderTargetBuffers[swapChainBufferCount] = {};
    ID3D12DescriptorHeap* mRtvDescriptorHeap = nullptr;
    UINT mRtvDescriptorIncrementSize = 0;

    // 깊이-스텐실 버퍼, 서술자 힙 인터페이스 포인터, 깊이-스텐실 서술자 원소의 크기이다.
    ID3D12Resource* mDepthStencilBuffer = nullptr;
    ID3D12DescriptorHeap* mDsvDescriptorHeap = nullptr;
    UINT mDsvDescriptorIncrementSize = 0;

    // 명령 큐, 명령 할당자, 명령 리스트 인터페이스 포인터이다.
    ID3D12CommandQueue* mCommandQueue = nullptr;
    ID3D12CommandAllocator* mCommandAllocator = nullptr;
    ID3D12GraphicsCommandList* mCommandList = nullptr;

    // 펜스 인터페이스 포인터, 펜스의 값, 이벤트 핸들이다.
    ID3D12Fence* mFence = nullptr;
    UINT64 mFenceValues[swapChainBufferCount] = {}; // 후면 버퍼 마다 현재의 펜스 값을 관리
    HANDLE mFenceEvent = nullptr;

    UINT mClientWidth = 0;
    UINT mClientHeight = 0;
};