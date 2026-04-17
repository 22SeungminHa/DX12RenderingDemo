#include "D3DCore.h"

bool D3DCore::Initialize(HWND hwnd, int width, int height)
{
    clientWidth_ = width;
    clientHeight_ = height;

    CreateDirect3DDevice();
    CreateCommandObjects();
    CreateDescriptorHeaps();
    CreateSwapChain(hwnd, width, height);
    CreateRenderTargetViews();
    CreateDepthStencilView();

    return true;
}

void D3DCore::Shutdown()
{
    if (!device_ && !swapChain_ && !cmdQueue_)
        return;

    WaitForGpuComplete();

    ReleaseBackBuffers();

    depthStencilBuffer_.Reset();
    rtvDescriptorHeap_.Reset();
    dsvDescriptorHeap_.Reset();

    cmdList_.Reset();
    for (auto& allocator : cmdAllocators_) {
        allocator.Reset();
    }
    cmdQueue_.Reset();

    fence_.Reset();
    swapChain_.Reset();
    device_.Reset();
    factory_.Reset();

    fenceEvent_.reset();

#if defined(_DEBUG)
    ComPtr<IDXGIDebug1> debug;
    ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(debug.GetAddressOf())));
    ThrowIfFailed(debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL));
#endif
}

void D3DCore::CreateDirect3DDevice()
{
    UINT factoryFlags = 0;

#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())));
    if (debugController)
        debugController->EnableDebugLayer();
    factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(factory_.GetAddressOf())));

    ComPtr<IDXGIAdapter1> selectedAdapter;

    for (UINT adapterIndex = 0; ; ++adapterIndex)
    {
        ComPtr<IDXGIAdapter1> currentAdapter;
        if (factory_->EnumAdapters1(adapterIndex, currentAdapter.GetAddressOf()) == DXGI_ERROR_NOT_FOUND)
            break;

        DXGI_ADAPTER_DESC1 adapterDesc{};
        currentAdapter->GetDesc1(&adapterDesc);

        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        if (SUCCEEDED(D3D12CreateDevice(
            currentAdapter.Get(),
            D3D_FEATURE_LEVEL_12_0,
            IID_PPV_ARGS(device_.GetAddressOf()))))
        {
            selectedAdapter = currentAdapter;
            break;
        }
    }

    if (!device_)
    {
        ThrowIfFailed(factory_->EnumWarpAdapter(IID_PPV_ARGS(selectedAdapter.GetAddressOf())));
        ThrowIfFailed(D3D12CreateDevice(
            selectedAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(device_.GetAddressOf())));
    }

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels{};
    msaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    msaaQualityLevels.SampleCount = 1;
    msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msaaQualityLevels.NumQualityLevels = 0;

    ThrowIfFailed(device_->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msaaQualityLevels,
        sizeof(msaaQualityLevels)));

    msaaQualityLevel_ = msaaQualityLevels.NumQualityLevels;
    msaaEnable_ = false;

    ThrowIfFailed(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.GetAddressOf())));

    for (auto& value : fenceValues_)
        value = 1;

    fenceEvent_.reset(::CreateEvent(nullptr, FALSE, FALSE, nullptr));
    if (!fenceEvent_)
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
}

void D3DCore::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{};
    cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(device_->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(cmdQueue_.GetAddressOf())));
    for (UINT i = 0; i < swapChainBufferCnt_; ++i)
    {
        ThrowIfFailed(device_->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(cmdAllocators_[i].GetAddressOf())));
    }
    ThrowIfFailed(device_->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        cmdAllocators_[0].Get(),
        nullptr,
        IID_PPV_ARGS(cmdList_.GetAddressOf())));
    ThrowIfFailed(cmdList_->Close());
}

void D3DCore::CreateDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.NumDescriptors = swapChainBufferCnt_;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;

    ThrowIfFailed(device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(rtvDescriptorHeap_.GetAddressOf())));
    rtvDescriptorIncrementSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;

    ThrowIfFailed(device_->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvDescriptorHeap_.GetAddressOf())));
    dsvDescriptorIncrementSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}


DXGI_SWAP_CHAIN_DESC D3DCore::CreateSwapChainDesc(HWND hwnd, int width, int height) const
{
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = swapChainBufferCnt_;
    desc.BufferDesc.Width = width;
    desc.BufferDesc.Height = height;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.OutputWindow = hwnd;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Windowed = TRUE;
    desc.Flags = 0;
    return desc;
}

void D3DCore::CreateSwapChain(HWND hwnd, int width, int height)
{
    clientWidth_ = width;
    clientHeight_ = height;

    DXGI_SWAP_CHAIN_DESC swapChainDesc = CreateSwapChainDesc(hwnd, width, height);

    ComPtr<IDXGISwapChain> baseSwapChain;
    ThrowIfFailed(factory_->CreateSwapChain(
        cmdQueue_.Get(),
        &swapChainDesc,
        baseSwapChain.GetAddressOf()));
    ThrowIfFailed(baseSwapChain.As(&swapChain_));

    swapChainBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();
    ThrowIfFailed(factory_->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
}

void D3DCore::ReleaseBackBuffers()
{
    for (auto& buffer : renderTargetBuffers_)
        buffer.Reset();
}

void D3DCore::CreateRenderTargetViews()
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUDescriptorHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < swapChainBufferCnt_; i++)
    {
        renderTargetBuffers_[i].Reset();

        ThrowIfFailed(swapChain_->GetBuffer(i, IID_PPV_ARGS(renderTargetBuffers_[i].GetAddressOf())));
        device_->CreateRenderTargetView(renderTargetBuffers_[i].Get(), nullptr, rtvCPUDescriptorHandle);

        renderTargetStates_[i] = D3D12_RESOURCE_STATE_PRESENT;

        rtvCPUDescriptorHandle.ptr += rtvDescriptorIncrementSize_;
    }
}

D3D12_RESOURCE_DESC D3DCore::CreateDepthStencilResourceDesc() const
{
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = clientWidth_;
    desc.Height = clientHeight_;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    return desc;
}
D3D12_CLEAR_VALUE D3DCore::CreateDepthStencilClearValue() const
{
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;
    return clearValue;
}

void D3DCore::CreateDepthStencilView()
{
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc = CreateDepthStencilResourceDesc();
    D3D12_CLEAR_VALUE clearValue = CreateDepthStencilClearValue();

    ThrowIfFailed(device_->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(depthStencilBuffer_.GetAddressOf())));

    device_->CreateDepthStencilView(
        depthStencilBuffer_.Get(),
        nullptr,
        dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
}

void D3DCore::WaitForGpuComplete()
{
    const UINT64 fenceValue = fenceValues_[swapChainBufferIndex_];
    ThrowIfFailed(cmdQueue_->Signal(fence_.Get(), fenceValue));

    if (fence_->GetCompletedValue() < fenceValue)
    {
        ThrowIfFailed(fence_->SetEventOnCompletion(fenceValue, fenceEvent_.get()));
        ::WaitForSingleObject(fenceEvent_.get(), INFINITE);
    }

    ++fenceValues_[swapChainBufferIndex_];
}

void D3DCore::MoveToNextFrame()
{
    const UINT64 currentFenceValue = fenceValues_[swapChainBufferIndex_];
    ThrowIfFailed(cmdQueue_->Signal(fence_.Get(), currentFenceValue));

    swapChainBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();

    if (fence_->GetCompletedValue() < fenceValues_[swapChainBufferIndex_])
    {
        ThrowIfFailed(fence_->SetEventOnCompletion(
            fenceValues_[swapChainBufferIndex_],
            fenceEvent_.get()));
        ::WaitForSingleObject(fenceEvent_.get(), INFINITE);
    }

    fenceValues_[swapChainBufferIndex_] = currentFenceValue + 1;
}

void D3DCore::ResetCommandList()
{
    auto& allocator = cmdAllocators_[swapChainBufferIndex_];
    ThrowIfFailed(allocator->Reset());
    ThrowIfFailed(cmdList_->Reset(allocator.Get(), nullptr));
}

void D3DCore::ExecuteCommandList()
{
    ThrowIfFailed(cmdList_->Close());

    ID3D12CommandList* commandLists[] = { cmdList_.Get() };
    cmdQueue_->ExecuteCommandLists(_countof(commandLists), commandLists);
}

void D3DCore::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    if (before == after) return;

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    cmdList_->ResourceBarrier(1, &barrier);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DCore::GetCurrentRtvHandle() const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += (swapChainBufferIndex_ * rtvDescriptorIncrementSize_);
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DCore::GetDsvHandle() const
{
    return dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
}

void D3DCore::BeginRender(const float clearColor[4])
{
    ID3D12Resource* currentRenderTarget = GetCurrentRenderTarget();
    UINT currentIndex = swapChainBufferIndex_;

    TransitionResource(
        currentRenderTarget,
        renderTargetStates_[currentIndex],
        D3D12_RESOURCE_STATE_RENDER_TARGET);

    renderTargetStates_[currentIndex] = D3D12_RESOURCE_STATE_RENDER_TARGET;

    const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCurrentRtvHandle();
    const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDsvHandle();

    cmdList_->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    cmdList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    cmdList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
}

void D3DCore::EndRender()
{
    ID3D12Resource* currentRenderTarget = GetCurrentRenderTarget();
    UINT currentIndex = swapChainBufferIndex_;

    TransitionResource(
        currentRenderTarget,
        renderTargetStates_[currentIndex],
        D3D12_RESOURCE_STATE_PRESENT);

    renderTargetStates_[currentIndex] = D3D12_RESOURCE_STATE_PRESENT;
}

void D3DCore::Present(UINT syncInterval, UINT flags)
{
    ThrowIfFailed(swapChain_->Present(syncInterval, flags));
}