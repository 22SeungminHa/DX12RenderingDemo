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
	CreateDescHeaps();
	CreateSwapChain(hWnd, width, height);
	CreateRenderTargetViews();
	CreateDepthStencilView();

	return(true);
}

void CD3DCore::Shutdown()
{
	//GPU가 모든 명령 리스트를 실행할 때 까지 기다린다.
	WaitForGpuComplete();

	::CloseHandle(mFenceEvent);

	for (int i = 0; i < mSwapChainBufferCount; i++) if (mRenderTargetBuffers[i]) mRenderTargetBuffers[i]->Release();
	if (mRtvDescriptorHeap) mRtvDescriptorHeap->Release();
	if (mDepthStencilBuffer) mDepthStencilBuffer->Release();
	if (mDsvDescriptorHeap) mDsvDescriptorHeap->Release();
	if (mCommandAllocator) mCommandAllocator->Release();
	if (mCommandQueue) mCommandQueue->Release();
	if (mCommandList) mCommandList->Release();
	if (mFence) mFence->Release();
	mSwapChain->SetFullscreenState(FALSE, NULL);
	if (mSwapChain) mSwapChain->Release();
	if (mD3DDevice) mD3DDevice->Release();
	if (mDXGIFactory) mDXGIFactory->Release();

#if defined(_DEBUG)
	IDXGIDebug1* pdxgiDebug = NULL;
	DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), (void**)&pdxgiDebug);
	HRESULT hResult = pdxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
	pdxgiDebug->Release();
#endif
}

void CD3DCore::CreateSwapChain(HWND hWnd, int width, int height)
{
	mClientWidth = width;
	mClientHeight = height;

	DXGI_SWAP_CHAIN_DESC SwapChainDesc;
	::ZeroMemory(&SwapChainDesc, sizeof(SwapChainDesc));

	SwapChainDesc.BufferCount = mSwapChainBufferCount;
	SwapChainDesc.BufferDesc.Width = mClientWidth;
	SwapChainDesc.BufferDesc.Height = mClientHeight;

	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	SwapChainDesc.OutputWindow = hWnd;
	SwapChainDesc.SampleDesc.Count = (mMSAAEnable) ? 4 : 1;
	SwapChainDesc.SampleDesc.Quality = (mMSAAEnable) ? (mMSAAQualityLevels - 1) : 0;
	SwapChainDesc.Windowed = TRUE;

	//전체화면 모드에서 바탕화면의 해상도를 스왑체인(후면버퍼)의 크기에 맞게 변경한다.
	SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	HRESULT hResult = mDXGIFactory->CreateSwapChain(mCommandQueue, &SwapChainDesc, (IDXGISwapChain**)&mSwapChain);

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
	ID3D12Debug* DebugController = NULL;
	hResult = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&DebugController);
	if (DebugController)
	{
		DebugController->EnableDebugLayer();
		DebugController->Release();
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

void CD3DCore::CreateDescHeaps()
{
	//렌더 타겟 서술자 힙(서술자의 개수는 스왑체인 버퍼의 개수)을 생성한다.
	D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc;
	::ZeroMemory(&DescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	DescriptorHeapDesc.NumDescriptors = mSwapChainBufferCount;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	HRESULT hResult = mD3DDevice->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&mRtvDescriptorHeap);

	//렌더 타겟 서술자 힙의 원소의 크기를 저장한다. 
	mRtvDescriptorIncrementSize = mD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//깊이-스텐실 서술자 힙(서술자의 개수는 1)을 생성한다.
	DescriptorHeapDesc.NumDescriptors = 1;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = mD3DDevice->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&mDsvDescriptorHeap);
	//깊이-스텐실 서술자 힙의 원소의 크기를 저장한다.
	mDsvDescriptorIncrementSize = mD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void CD3DCore::CreateCommandObjects()
{
	//직접(Direct) 명령 큐를 생성한다. 
	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc;
	::ZeroMemory(&CommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	HRESULT hResult = mD3DDevice->CreateCommandQueue(&CommandQueueDesc, _uuidof(ID3D12CommandQueue), (void**)&mCommandQueue);

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
	D3D12_CPU_DESCRIPTOR_HANDLE RtvCPUDescriptorHandle = mRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < mSwapChainBufferCount; i++)
	{
		mSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&mRenderTargetBuffers[i]);
		mD3DDevice->CreateRenderTargetView(mRenderTargetBuffers[i], NULL, RtvCPUDescriptorHandle);
		RtvCPUDescriptorHandle.ptr += mRtvDescriptorIncrementSize;
	}
}

void CD3DCore::CreateDepthStencilView()
{
	//깊이-스텐실 버퍼를 생성한다.
	D3D12_RESOURCE_DESC ResourceDesc;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Alignment = 0;
	ResourceDesc.Width = mClientWidth;
	ResourceDesc.Height = mClientHeight;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ResourceDesc.SampleDesc.Count = (mMSAAEnable) ? 4 : 1;
	ResourceDesc.SampleDesc.Quality = (mMSAAEnable) ? (mMSAAQualityLevels - 1) : 0;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	D3D12_HEAP_PROPERTIES HeapProperties;
	::ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.CreationNodeMask = 1;
	HeapProperties.VisibleNodeMask = 1;
	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;
	mD3DDevice->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue, __uuidof(ID3D12Resource), (void**)&mDepthStencilBuffer);

	//깊이-스텐실 버퍼 뷰를 생성한다.
	D3D12_CPU_DESCRIPTOR_HANDLE DsvCPUDescriptorHandle = mDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	mD3DDevice->CreateDepthStencilView(mDepthStencilBuffer, NULL, DsvCPUDescriptorHandle);
}

void CD3DCore::WaitForGpuComplete()
{
	UINT64 nFenceValue = ++mFenceValues[mSwapChainBufferIndex];
	HRESULT hResult = mCommandQueue->Signal(mFence, nFenceValue);
	if (mFence->GetCompletedValue() < nFenceValue)
	{
		hResult = mFence->SetEventOnCompletion(nFenceValue, mFenceEvent);
		::WaitForSingleObject(mFenceEvent, INFINITE);
	}
}

void CD3DCore::MoveToNextFrame()
{
	mSwapChainBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
	UINT64 nFenceValue = ++mFenceValues[mSwapChainBufferIndex];

	HRESULT hResult = mCommandQueue->Signal(mFence, nFenceValue);

	if (mFence->GetCompletedValue() < nFenceValue)
	{
		hResult = mFence->SetEventOnCompletion(nFenceValue, mFenceEvent);
		::WaitForSingleObject(mFenceEvent, INFINITE);
	}
}