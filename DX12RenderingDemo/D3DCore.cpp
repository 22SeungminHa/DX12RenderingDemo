#include "D3DCore.h"
//
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

	return(true);
}

void CD3DCore::Shutdown()
{
	::CloseHandle(mFenceEvent);

	for (int i = 0; i < mSwapChainBufferCount; i++) if (mRenderTargetBuffers[i]) mRenderTargetBuffers[i]->Release();
	if (mRtvDescriptorHeap) mRtvDescriptorHeap->Release();
	if (mDepthStencilBuffer) mDepthStencilBuffer->Release();
	if (mDsvDescriptorHeap) mDsvDescriptorHeap->Release();
	if (mCommandAllocator) mCommandAllocator->Release();
	if (mCommandQueue) mCommandQueue->Release();
	if (mCommandList) mCommandList->Release();
	if (mFence) mFence->Release();
	if (mSwapChain) mSwapChain->SetFullscreenState(FALSE, NULL);
	if (mSwapChain) mSwapChain->Release();
	if (mD3DDevice) mD3DDevice->Release();
	if (mDXGIFactory) mDXGIFactory->Release();

#if defined(_DEBUG)
	IDXGIDebug1* debug = NULL;
	DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), (void**)&debug);
	HRESULT hResult = debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
	debug->Release();
#endif
}

void CD3DCore::CreateSwapChain(HWND hWnd, int width, int height)
{
	mClientWidth = width;
	mClientHeight = height;

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	::ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	swapChainDesc.BufferCount = mSwapChainBufferCount;
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

	//전체화면 모드에서 바탕화면의 해상도를 스왑체인(후면버퍼)의 크기에 맞게 변경한다.
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	HRESULT hResult = mDXGIFactory->CreateSwapChain(mCommandQueue, &swapChainDesc, (IDXGISwapChain**)&mSwapChain);

	mSwapChainBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
	hResult = mDXGIFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE
	CreateRenderTargetViews();
#endif
}

void CD3DCore::CreateDirect3DDevice()
{
	HRESULT hResult;
	UINT DXGIFactoryFlags = 0;
#if defined(_DEBUG)
	ID3D12Debug* debugController = NULL;
	hResult = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&debugController);
	if (debugController)
	{
		debugController->EnableDebugLayer();
		debugController->Release();
	}
	DXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	//모든 하드웨어 어댑터 대하여 특성 레벨 12.0을 지원하는 하드웨어 디바이스를 생성한다. 
	hResult = ::CreateDXGIFactory2(DXGIFactoryFlags, __uuidof(IDXGIFactory4), (void**)&mDXGIFactory);
	IDXGIAdapter1* DXGIAdapter = NULL;
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != mDXGIFactory->EnumAdapters1(i, &DXGIAdapter); i++) {
		DXGI_ADAPTER_DESC1 DXGIAdapterDesc;
		DXGIAdapter->GetDesc1(&DXGIAdapterDesc);
		if (DXGIAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(DXGIAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void**)&mD3DDevice))) break;
	}

	//특성 레벨 12.0을 지원하는 하드웨어 디바이스를 생성할 수 없으면 WARP 디바이스를 생성한다. 
	if (!DXGIAdapter)
	{
		mDXGIFactory->EnumWarpAdapter(_uuidof(IDXGIAdapter1), (void**)&DXGIAdapter);
		D3D12CreateDevice(DXGIAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), (void**)&mD3DDevice);
	}

	//디바이스가 지원하는 다중 샘플의 품질 수준을 확인한다. 
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS MSAAQualityLevels;
	MSAAQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	MSAAQualityLevels.SampleCount = 4; //Msaa4x 다중 샘플링
	MSAAQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	MSAAQualityLevels.NumQualityLevels = 0;
	mD3DDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &MSAAQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	mMSAAQualityLevels = MSAAQualityLevels.NumQualityLevels;

	//다중 샘플의 품질 수준이 1보다 크면 다중 샘플링을 활성화한다.
	mMSAAEnable = (mMSAAQualityLevels > 1) ? true : false;

	hResult = mD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&mFence);

	// 펜스와 동기화를 위한 이벤트 객체를 생성한다(이벤트 객체의 초기값을 FALSE이다). 이벤트가 실행되면(Signal) 이벤트의 값을 자동적으로 FALSE가 되도록 생성한다.
	mFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	if (DXGIAdapter) DXGIAdapter->Release();
}

void CD3DCore::CreateDescriptorHeaps()
{
	//렌더 타겟 서술자 힙(서술자의 개수는 스왑체인 버퍼의 개수)을 생성한다.
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc;
	::ZeroMemory(&descriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	descriptorHeapDesc.NumDescriptors = mSwapChainBufferCount;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDesc.NodeMask = 0;
	HRESULT hResult = mD3DDevice->CreateDescriptorHeap(&descriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&mRtvDescriptorHeap);

	//렌더 타겟 서술자 힙의 원소의 크기를 저장한다. 
	mRtvDescriptorIncrementSize = mD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//깊이-스텐실 서술자 힙(서술자의 개수는 1)을 생성한다.
	descriptorHeapDesc.NumDescriptors = 1;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = mD3DDevice->CreateDescriptorHeap(&descriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&mDsvDescriptorHeap);
	//깊이-스텐실 서술자 힙의 원소의 크기를 저장한다.
	mDsvDescriptorIncrementSize = mD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void CD3DCore::CreateCommandObjects()
{
	//직접(Direct) 명령 큐를 생성한다. 
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
	::ZeroMemory(&commandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	HRESULT hResult = mD3DDevice->CreateCommandQueue(&commandQueueDesc, _uuidof(ID3D12CommandQueue), (void**)&mCommandQueue);

	//직접(Direct) 명령 할당자를 생성한다. 
	hResult = mD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&mCommandAllocator);

	//직접(Direct) 명령 리스트를 생성한다.
	hResult = mD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&mCommandList);

	//명령 리스트는 생성되면 열린(Open) 상태이므로 닫힌(Closed) 상태로 만든다. 
	hResult = mCommandList->Close();
}

//스왑체인의 각 후면 버퍼에 대한 렌더 타겟 뷰를 생성한다. 
void CD3DCore::CreateRenderTargetViews()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUDescriptorHandle = mRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < mSwapChainBufferCount; i++)
	{
		mSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&mRenderTargetBuffers[i]);
		mD3DDevice->CreateRenderTargetView(mRenderTargetBuffers[i], NULL, rtvCPUDescriptorHandle);
		rtvCPUDescriptorHandle.ptr += mRtvDescriptorIncrementSize;
	}
}

void CD3DCore::CreateDepthStencilView()
{
	//깊이-스텐실 버퍼를 생성한다.
	D3D12_RESOURCE_DESC resourceDesc;
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
	D3D12_HEAP_PROPERTIES heapProperties;
	::ZeroMemory(&heapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;
	mD3DDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, __uuidof(ID3D12Resource), (void**)&mDepthStencilBuffer);

	//깊이-스텐실 버퍼 뷰를 생성한다.
	D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUDescriptorHandle = mDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	mD3DDevice->CreateDepthStencilView(mDepthStencilBuffer, NULL, dsvCPUDescriptorHandle);
}

void CD3DCore::ChangeSwapChainState()
{
	WaitForGpuComplete();
	BOOL fullScreenState = FALSE;
	mSwapChain->GetFullscreenState(&fullScreenState, NULL);
	mSwapChain->SetFullscreenState(!fullScreenState, NULL);
	DXGI_MODE_DESC targetParameters;
	targetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	targetParameters.Width = mClientWidth;
	targetParameters.Height = mClientHeight;
	targetParameters.RefreshRate.Numerator = 60;
	targetParameters.RefreshRate.Denominator = 1;
	targetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	targetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	mSwapChain->ResizeTarget(&targetParameters);

	for (int i = 0; i < mSwapChainBufferCount; i++) if (mRenderTargetBuffers[i])
		mRenderTargetBuffers[i]->Release();

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	mSwapChain->GetDesc(&swapChainDesc);
	mSwapChain->ResizeBuffers(mSwapChainBufferCount, mClientWidth, mClientHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags);

	mSwapChainBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

	CreateRenderTargetViews();
}

void CD3DCore::WaitForGpuComplete()
{
	UINT64 fenceValue = ++mFenceValues[mSwapChainBufferIndex];
	HRESULT hResult = mCommandQueue->Signal(mFence, fenceValue);
	if (mFence->GetCompletedValue() < fenceValue)
	{
		hResult = mFence->SetEventOnCompletion(fenceValue, mFenceEvent);
		::WaitForSingleObject(mFenceEvent, INFINITE);
	}
}

void CD3DCore::MoveToNextFrame()
{
	mSwapChainBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
	UINT64 fenceValue = ++mFenceValues[mSwapChainBufferIndex];

	HRESULT hResult = mCommandQueue->Signal(mFence, fenceValue);

	if (mFence->GetCompletedValue() < fenceValue)
	{
		hResult = mFence->SetEventOnCompletion(fenceValue, mFenceEvent);
		::WaitForSingleObject(mFenceEvent, INFINITE);
	}
}

void CD3DCore::ResetCommandList()
{
	HRESULT hResult = mCommandAllocator->Reset();
	hResult = mCommandList->Reset(mCommandAllocator, NULL);
}

void CD3DCore::ExecuteCommandList()
{
	//씬 객체를 생성하기 위하여 필요한 그래픽 명령 리스트들을 명령 큐에 추가한다.
	HRESULT hResult = mCommandList->Close();
	ID3D12CommandList* commandLists[] = { mCommandList };
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
	D3D12_RESOURCE_BARRIER resourceBarrier;
	::ZeroMemory(&resourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));

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

	mCommandList->ClearRenderTargetView(rtvCPUDescriptorHandle, clearColor, 0, NULL);
	mCommandList->ClearDepthStencilView(dsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
}

void CD3DCore::EndRender()
{
	D3D12_RESOURCE_BARRIER resourceBarrier;
	::ZeroMemory(&resourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));

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
	mSwapChain->Present(syncInterval, flags);
}