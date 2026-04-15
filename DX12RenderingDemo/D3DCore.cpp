#include "D3DCore.h"

CD3DCore::CD3DCore()
{
    clientWidth_ = FRAME_BUFFER_WIDTH;
    clientHeight_ = FRAME_BUFFER_HEIGHT;
}

CD3DCore::~CD3DCore()
{
}

bool CD3DCore::Initialize(HWND hwnd, int width, int height)
{
	//Direct3D 디바이스, 명령 큐와 명령 리스트, 스왑 체인 등을 생성하는 함수를 호출한다. 
    CreateDirect3DDevice();
    CreateCommandObjects();
    CreateDescriptorHeaps();
    CreateSwapChain(hwnd, width, height);
    CreateRenderTargetViews();
    CreateDepthStencilView();

    return true;
}

void CD3DCore::Shutdown()
{
    if (!device_ && !swapChain_ && !cmdQueue_)
        return;

    WaitForGpuComplete();

    if (swapChain_)
        swapChain_->SetFullscreenState(FALSE, nullptr);

    for (auto& buffer : renderTargetBuffers_)
        buffer.Reset();

    depthStencilBuffer_.Reset();
    rtvDescriptorHeap_.Reset();
    dsvDescriptorHeap_.Reset();
    cmdAllocator_.Reset();
    cmdQueue_.Reset();
    cmdList_.Reset();
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

void CD3DCore::CreateSwapChain(HWND hwnd, int width, int height)
{
    clientWidth_ = width;
    clientHeight_ = height;

    DXGI_SWAP_CHAIN_DESC swapChainDesc{};

    swapChainDesc.BufferCount = swapChainBufferCnt_;
    swapChainDesc.BufferDesc.Width = clientWidth_;
    swapChainDesc.BufferDesc.Height = clientHeight_;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.SampleDesc.Count = (msaaEnable_) ? 4 : 1;
    swapChainDesc.SampleDesc.Quality = (msaaEnable_) ? (msaaQualityLevel_ - 1) : 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ComPtr<IDXGISwapChain> swapChain;
    ThrowIfFailed(factory_->CreateSwapChain(cmdQueue_.Get(), &swapChainDesc, swapChain.GetAddressOf()));
    ThrowIfFailed(swapChain.As(&swapChain_));

    swapChainBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();
    ThrowIfFailed(factory_->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE
    CreateRenderTargetViews();
#endif
}

void CD3DCore::CreateDirect3DDevice()
{
    UINT factoryFlags = 0;

#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())));
    if (debugController)
        debugController->EnableDebugLayer();
    factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(::CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(factory_.GetAddressOf())));

    ComPtr<IDXGIAdapter1> adapter;

    for (UINT i = 0; ; ++i) {
        ComPtr<IDXGIAdapter1> currentAdapter;
        if (factory_->EnumAdapters1(i, currentAdapter.GetAddressOf()) == DXGI_ERROR_NOT_FOUND)
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
            adapter = currentAdapter;
            break;
        }
    }

    if (!device_)
    {
        ThrowIfFailed(factory_->EnumWarpAdapter(IID_PPV_ARGS(adapter.GetAddressOf())));
        ThrowIfFailed(D3D12CreateDevice(
            adapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(device_.GetAddressOf())));
    }

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels{};
    msaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    msaaQualityLevels.SampleCount = 4;
    msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msaaQualityLevels.NumQualityLevels = 0;

    ThrowIfFailed(device_->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msaaQualityLevels,
        sizeof(msaaQualityLevels)));

    msaaQualityLevel_ = msaaQualityLevels.NumQualityLevels;
    msaaEnable_ = (msaaQualityLevel_ > 1);

    ThrowIfFailed(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.GetAddressOf())));

    for (UINT i = 0; i < swapChainBufferCnt_; ++i)
        fenceValues_[i] = 1;

    fenceEvent_.reset(::CreateEvent(nullptr, FALSE, FALSE, nullptr));
    if (!fenceEvent_)
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
}

void CD3DCore::CreateDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
    descriptorHeapDesc.NumDescriptors = swapChainBufferCnt_;
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    descriptorHeapDesc.NodeMask = 0;

    ThrowIfFailed(device_->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(rtvDescriptorHeap_.GetAddressOf())));

    rtvDescriptorIncrementSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    descriptorHeapDesc.NumDescriptors = 1;
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

    ThrowIfFailed(device_->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(dsvDescriptorHeap_.GetAddressOf())));

    dsvDescriptorIncrementSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void CD3DCore::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(cmdQueue_.GetAddressOf())));
    ThrowIfFailed(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(cmdAllocator_.GetAddressOf())));
    ThrowIfFailed(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator_.Get(), nullptr, IID_PPV_ARGS(cmdList_.GetAddressOf())));
    ThrowIfFailed(cmdList_->Close());
}

void CD3DCore::CreateRenderTargetViews()
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUDescriptorHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < swapChainBufferCnt_; i++) {
        renderTargetBuffers_[i].Reset();

        ThrowIfFailed(swapChain_->GetBuffer(i, IID_PPV_ARGS(renderTargetBuffers_[i].GetAddressOf())));
        device_->CreateRenderTargetView(renderTargetBuffers_[i].Get(), nullptr, rtvCPUDescriptorHandle);

        rtvCPUDescriptorHandle.ptr += rtvDescriptorIncrementSize_;
    }
}

void CD3DCore::CreateDepthStencilView()
{
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = clientWidth_;
    resourceDesc.Height = clientHeight_;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    resourceDesc.SampleDesc.Count = (msaaEnable_) ? 4 : 1;
    resourceDesc.SampleDesc.Quality = (msaaEnable_) ? (msaaQualityLevel_ - 1) : 0;
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

    ThrowIfFailed(device_->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(depthStencilBuffer_.GetAddressOf())));

    D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUDescriptorHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    device_->CreateDepthStencilView(depthStencilBuffer_.Get(), nullptr, dsvCPUDescriptorHandle);
}

void CD3DCore::ChangeSwapChainState()
{
    WaitForGpuComplete();

    BOOL fullScreenState = FALSE;
    ThrowIfFailed(swapChain_->GetFullscreenState(&fullScreenState, nullptr));
    ThrowIfFailed(swapChain_->SetFullscreenState(!fullScreenState, nullptr));

    DXGI_MODE_DESC targetParameters{};
    targetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    targetParameters.Width = clientWidth_;
    targetParameters.Height = clientHeight_;
    targetParameters.RefreshRate.Numerator = 60;
    targetParameters.RefreshRate.Denominator = 1;
    targetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    targetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

    ThrowIfFailed(swapChain_->ResizeTarget(&targetParameters));

    for (auto& buffer : renderTargetBuffers_)
        buffer.Reset();

    DXGI_SWAP_CHAIN_DESC swapChainDesc{};
    ThrowIfFailed(swapChain_->GetDesc(&swapChainDesc));
    ThrowIfFailed(swapChain_->ResizeBuffers(swapChainBufferCnt_, clientWidth_, clientHeight_, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

    swapChainBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();
    CreateRenderTargetViews();
}

void CD3DCore::WaitForGpuComplete()
{
    const UINT64 fenceValue = fenceValues_[swapChainBufferIndex_];
    ThrowIfFailed(cmdQueue_->Signal(fence_.Get(), fenceValue));

    if (fence_->GetCompletedValue() < fenceValue) {
        ThrowIfFailed(fence_->SetEventOnCompletion(fenceValue, fenceEvent_.get()));
        ::WaitForSingleObject(fenceEvent_.get(), INFINITE);
    }

    fenceValues_[swapChainBufferIndex_]++;
}

void CD3DCore::MoveToNextFrame()
{
    const UINT64 currentFenceValue = fenceValues_[swapChainBufferIndex_];
    ThrowIfFailed(cmdQueue_->Signal(fence_.Get(), currentFenceValue));
    swapChainBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();

    if (fence_->GetCompletedValue() < fenceValues_[swapChainBufferIndex_]) {
        ThrowIfFailed(fence_->SetEventOnCompletion(fenceValues_[swapChainBufferIndex_], fenceEvent_.get()));
        ::WaitForSingleObject(fenceEvent_.get(), INFINITE);
    }

    fenceValues_[swapChainBufferIndex_] = currentFenceValue + 1;
}

void CD3DCore::ResetCommandList()
{
    ThrowIfFailed(cmdAllocator_->Reset());
    ThrowIfFailed(cmdList_->Reset(cmdAllocator_.Get(), nullptr));
}

void CD3DCore::ExecuteCommandList()
{
    ThrowIfFailed(cmdList_->Close());
    ID3D12CommandList* commandLists[] = { cmdList_.Get() };
    cmdQueue_->ExecuteCommandLists(_countof(commandLists), commandLists);
}

D3D12_CPU_DESCRIPTOR_HANDLE CD3DCore::GetCurrentRtvHandle() const
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUDescriptorHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    rtvCPUDescriptorHandle.ptr += (swapChainBufferIndex_ * rtvDescriptorIncrementSize_);
    return rtvCPUDescriptorHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE CD3DCore::GetDsvHandle() const
{
    return dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
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

    cmdList_->ResourceBarrier(1, &resourceBarrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUDescriptorHandle = GetCurrentRtvHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUDescriptorHandle = GetDsvHandle();

    cmdList_->OMSetRenderTargets(1, &rtvCPUDescriptorHandle, FALSE, &dsvCPUDescriptorHandle);
    cmdList_->ClearRenderTargetView(rtvCPUDescriptorHandle, clearColor, 0, nullptr);
    cmdList_->ClearDepthStencilView(dsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
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

    cmdList_->ResourceBarrier(1, &resourceBarrier);
}

void CD3DCore::Present(UINT syncInterval, UINT flags)
{
    ThrowIfFailed(swapChain_->Present(syncInterval, flags));
}