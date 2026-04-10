#include "D3DCore.h"

CD3DCore::CD3DCore()
{
    mClientWidth = FRAME_BUFFER_WIDTH;
    mClientHeight = FRAME_BUFFER_HEIGHT;
}

CD3DCore::~CD3DCore()
{
}

bool CD3DCore::Initialize(HWND hWnd, int width, int height)
{
	//Direct3D 디바이스, 명령 큐와 명령 리스트, 스왑 체인 등을 생성하는 함수를 호출한다. 
    CreateDirect3DDevice();
    CreateCommandObjects();
    CreateDescriptorHeaps();
    CreateSwapChain(hWnd, width, height);
    CreateRenderTargetViews();
    CreateDepthStencilView();

    return true;
}

void CD3DCore::Shutdown()
{
    if (!mD3DDevice && !mSwapChain && !mCommandQueue)
        return;

    WaitForGpuComplete();

    if (mSwapChain)
        mSwapChain->SetFullscreenState(FALSE, nullptr);

    for (auto& buffer : mRenderTargetBuffers)
        buffer.Reset();

    mDepthStencilBuffer.Reset();
    mRtvDescriptorHeap.Reset();
    mDsvDescriptorHeap.Reset();
    mCommandAllocator.Reset();
    mCommandQueue.Reset();
    mCommandList.Reset();
    mFence.Reset();
    mSwapChain.Reset();
    mD3DDevice.Reset();
    mDXGIFactory.Reset();

    mFenceEvent.reset();

#if defined(_DEBUG)
    ComPtr<IDXGIDebug1> debug;
    ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(debug.GetAddressOf())));
    ThrowIfFailed(debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL));
#endif
}

void CD3DCore::CreateSwapChain(HWND hWnd, int width, int height)
{
    mClientWidth = width;
    mClientHeight = height;

    DXGI_SWAP_CHAIN_DESC swapChainDesc{};

    swapChainDesc.BufferCount = swapChainBufferCount;
    swapChainDesc.BufferDesc.Width = mClientWidth;
    swapChainDesc.BufferDesc.Height = mClientHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.SampleDesc.Count = (mMSAAEnable) ? 4 : 1;
    swapChainDesc.SampleDesc.Quality = (mMSAAEnable) ? (mMSAAQualityLevels - 1) : 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ComPtr<IDXGISwapChain> swapChain;
    ThrowIfFailed(mDXGIFactory->CreateSwapChain(
        mCommandQueue.Get(),
        &swapChainDesc,
        swapChain.GetAddressOf()));

    ThrowIfFailed(swapChain.As(&mSwapChain));

    mSwapChainBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
    ThrowIfFailed(mDXGIFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE
    CreateRenderTargetViews();
#endif
}

void CD3DCore::CreateDirect3DDevice()
{
    UINT DXGIFactoryFlags = 0;

#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())));
    if (debugController)
        debugController->EnableDebugLayer();
    DXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(::CreateDXGIFactory2(DXGIFactoryFlags, IID_PPV_ARGS(mDXGIFactory.GetAddressOf())));

    ComPtr<IDXGIAdapter1> dxgiAdapter;

    for (UINT i = 0; ; ++i) {
        ComPtr<IDXGIAdapter1> currentAdapter;
        if (mDXGIFactory->EnumAdapters1(i, currentAdapter.GetAddressOf()) == DXGI_ERROR_NOT_FOUND)
            break;

        DXGI_ADAPTER_DESC1 adapterDesc{};
        currentAdapter->GetDesc1(&adapterDesc);

        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        if (SUCCEEDED(D3D12CreateDevice(
            currentAdapter.Get(),
            D3D_FEATURE_LEVEL_12_0,
            IID_PPV_ARGS(mD3DDevice.GetAddressOf()))))
        {
            dxgiAdapter = currentAdapter;
            break;
        }
    }

    if (!mD3DDevice)
    {
        ThrowIfFailed(mDXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(dxgiAdapter.GetAddressOf())));
        ThrowIfFailed(D3D12CreateDevice(
            dxgiAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(mD3DDevice.GetAddressOf())));
    }

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels{};
    msaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    msaaQualityLevels.SampleCount = 4;
    msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msaaQualityLevels.NumQualityLevels = 0;

    ThrowIfFailed(mD3DDevice->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msaaQualityLevels,
        sizeof(msaaQualityLevels)));

    mMSAAQualityLevels = msaaQualityLevels.NumQualityLevels;
    mMSAAEnable = (mMSAAQualityLevels > 1);

    ThrowIfFailed(mD3DDevice->CreateFence(
        0,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(mFence.GetAddressOf())));

    for (UINT i = 0; i < swapChainBufferCount; ++i)
        mFenceValues[i] = 1;

    mFenceEvent.reset(::CreateEvent(nullptr, FALSE, FALSE, nullptr));
    if (!mFenceEvent)
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
}

void CD3DCore::CreateDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
    descriptorHeapDesc.NumDescriptors = swapChainBufferCount;
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    descriptorHeapDesc.NodeMask = 0;

    ThrowIfFailed(mD3DDevice->CreateDescriptorHeap(
        &descriptorHeapDesc,
        IID_PPV_ARGS(mRtvDescriptorHeap.GetAddressOf())));

    mRtvDescriptorIncrementSize = mD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    descriptorHeapDesc.NumDescriptors = 1;
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

    ThrowIfFailed(mD3DDevice->CreateDescriptorHeap(
        &descriptorHeapDesc,
        IID_PPV_ARGS(mDsvDescriptorHeap.GetAddressOf())));

    mDsvDescriptorIncrementSize = mD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void CD3DCore::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(mD3DDevice->CreateCommandQueue(
        &commandQueueDesc,
        IID_PPV_ARGS(mCommandQueue.GetAddressOf())));

    ThrowIfFailed(mD3DDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(mCommandAllocator.GetAddressOf())));

    ThrowIfFailed(mD3DDevice->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        mCommandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(mCommandList.GetAddressOf())));

    ThrowIfFailed(mCommandList->Close());
}

void CD3DCore::CreateRenderTargetViews()
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUDescriptorHandle = mRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < swapChainBufferCount; i++)
    {
        mRenderTargetBuffers[i].Reset();

        ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(mRenderTargetBuffers[i].GetAddressOf())));

        mD3DDevice->CreateRenderTargetView(
            mRenderTargetBuffers[i].Get(),
            nullptr,
            rtvCPUDescriptorHandle);

        rtvCPUDescriptorHandle.ptr += mRtvDescriptorIncrementSize;
    }
}

void CD3DCore::CreateDepthStencilView()
{
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = mClientWidth;
    resourceDesc.Height = mClientHeight;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    resourceDesc.SampleDesc.Count = (mMSAAEnable) ? 4 : 1;
    resourceDesc.SampleDesc.Quality = (mMSAAEnable) ? (mMSAAQualityLevels - 1) : 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    ThrowIfFailed(mD3DDevice->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

    D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUDescriptorHandle = mDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    mD3DDevice->CreateDepthStencilView(
        mDepthStencilBuffer.Get(),
        nullptr,
        dsvCPUDescriptorHandle);
}

void CD3DCore::ChangeSwapChainState()
{
    WaitForGpuComplete();

    BOOL fullScreenState = FALSE;
    ThrowIfFailed(mSwapChain->GetFullscreenState(&fullScreenState, nullptr));
    ThrowIfFailed(mSwapChain->SetFullscreenState(!fullScreenState, nullptr));

    DXGI_MODE_DESC targetParameters{};
    targetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    targetParameters.Width = mClientWidth;
    targetParameters.Height = mClientHeight;
    targetParameters.RefreshRate.Numerator = 60;
    targetParameters.RefreshRate.Denominator = 1;
    targetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    targetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

    ThrowIfFailed(mSwapChain->ResizeTarget(&targetParameters));

    for (auto& buffer : mRenderTargetBuffers)
        buffer.Reset();

    DXGI_SWAP_CHAIN_DESC swapChainDesc{};
    ThrowIfFailed(mSwapChain->GetDesc(&swapChainDesc));
    ThrowIfFailed(mSwapChain->ResizeBuffers(
        swapChainBufferCount,
        mClientWidth,
        mClientHeight,
        swapChainDesc.BufferDesc.Format,
        swapChainDesc.Flags));

    mSwapChainBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
    CreateRenderTargetViews();
}

void CD3DCore::WaitForGpuComplete()
{
    const UINT64 fenceValue = mFenceValues[mSwapChainBufferIndex];
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), fenceValue));

    if (mFence->GetCompletedValue() < fenceValue)
    {
        ThrowIfFailed(mFence->SetEventOnCompletion(fenceValue, mFenceEvent.get()));
        ::WaitForSingleObject(mFenceEvent.get(), INFINITE);
    }

    mFenceValues[mSwapChainBufferIndex]++;
}

void CD3DCore::MoveToNextFrame()
{
    const UINT64 currentFenceValue = mFenceValues[mSwapChainBufferIndex];
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), currentFenceValue));
    mSwapChainBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

    if (mFence->GetCompletedValue() < mFenceValues[mSwapChainBufferIndex])
    {
        ThrowIfFailed(mFence->SetEventOnCompletion(
            mFenceValues[mSwapChainBufferIndex],
            mFenceEvent.get()));
        ::WaitForSingleObject(mFenceEvent.get(), INFINITE);
    }

    mFenceValues[mSwapChainBufferIndex] = currentFenceValue + 1;
}

void CD3DCore::ResetCommandList()
{
    ThrowIfFailed(mCommandAllocator->Reset());
    ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));
}

void CD3DCore::ExecuteCommandList()
{
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* commandLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
}

D3D12_CPU_DESCRIPTOR_HANDLE CD3DCore::GetCurrentRtvHandle() const
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUDescriptorHandle = mRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    rtvCPUDescriptorHandle.ptr += (mSwapChainBufferIndex * mRtvDescriptorIncrementSize);
    return rtvCPUDescriptorHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE CD3DCore::GetDsvHandle() const
{
    return mDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

void CD3DCore::BeginRender(const float clearColor[4])
{
    D3D12_RESOURCE_BARRIER resourceBarrier{};
    resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resourceBarrier.Transition.pResource = GetCurrentRenderTarget();
    resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    mCommandList->ResourceBarrier(1, &resourceBarrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUDescriptorHandle = GetCurrentRtvHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUDescriptorHandle = GetDsvHandle();

    mCommandList->OMSetRenderTargets(1, &rtvCPUDescriptorHandle, FALSE, &dsvCPUDescriptorHandle);
    mCommandList->ClearRenderTargetView(rtvCPUDescriptorHandle, clearColor, 0, nullptr);
    mCommandList->ClearDepthStencilView(
        dsvCPUDescriptorHandle,
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f,
        0,
        0,
        nullptr);
}

void CD3DCore::EndRender()
{
    D3D12_RESOURCE_BARRIER resourceBarrier{};
    resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resourceBarrier.Transition.pResource = GetCurrentRenderTarget();
    resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    mCommandList->ResourceBarrier(1, &resourceBarrier);
}

void CD3DCore::Present(UINT syncInterval, UINT flags)
{
    ThrowIfFailed(mSwapChain->Present(syncInterval, flags));
}