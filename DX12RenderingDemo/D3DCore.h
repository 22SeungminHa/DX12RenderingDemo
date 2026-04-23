#pragma once
#include "pch.h"

class D3DCore
{
public:
	static constexpr UINT kSwapChainBufferCount = 2;

public:
	D3DCore() = default;
	~D3DCore() = default;

	void Initialize(HWND hWnd, int width, int height);
	void Shutdown();

	void ResetCommandList(ID3D12CommandAllocator* allocator);
	void ExecuteCommandList();

	void Resize(UINT width, UINT height);

	// scene load / upload path
	void ResetUploadCommandList();
	UINT64 ExecuteUploadCommandList();
	bool IsUploadFenceComplete(UINT64 fenceValue) const;
	void WaitForUploadFence(UINT64 fenceValue);

	void BeginRender();
	void EndRender();

	void Present(UINT syncInterval = 0, UINT flags = 0);
	void MoveToNextFrame();
	void WaitForGpuComplete();

	UINT64 Signal();
	UINT64 GetCompletedFenceValue() const;
	void WaitForFenceValue(UINT64 fenceValue);

public:
	UINT GetClientWidth() const { return clientWidth_; }
	UINT GetClientHeight() const { return clientHeight_; }

	ID3D12Device* GetDevice() const { return device_.Get(); }
	IDXGISwapChain3* GetSwapChain() const { return swapChain_.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() const { return cmdList_.Get(); }
	ID3D12CommandQueue* GetCommandQueue() const { return cmdQueue_.Get(); }

	ID3D12Resource* GetCurrentRenderTarget() const { return renderTargetBuffers_[currentBackBufferIndex_].Get(); }
	UINT GetCurrentBackBufferIndex() const { return currentBackBufferIndex_; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRtvHandle() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() const;

	UINT GetRtvDescriptorIncrementSize() const { return rtvDescriptorSize_; }
	UINT GetDsvDescriptorIncrementSize() const { return dsvDescriptorSize_; }

private:
	struct HandleCloser
	{
		void operator()(HANDLE h) const noexcept
		{
			if (h) ::CloseHandle(h);
		}
	};

	using unique_handle = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleCloser>;

private:
	void CreateDirect3DDevice();
	void CreateCommandObjects();
	void CreateDescriptorHeaps();

	void CreateSwapChain(HWND hWnd, int width, int height);
	DXGI_SWAP_CHAIN_DESC1 CreateSwapChainDesc1(int width, int height) const;

	void CreateRenderTargetViews();
	void ReleaseBackBuffers();

	void CreateDepthStencilObjects();
	D3D12_RESOURCE_DESC CreateDepthStencilResourceDesc() const;
	D3D12_CLEAR_VALUE CreateDepthStencilClearValue() const;

	void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

private:
	// Device / DXGI core
	ComPtr<IDXGIFactory4> factory_;
	ComPtr<ID3D12Device> device_;
	ComPtr<IDXGISwapChain3> swapChain_;

	// Command system
	ComPtr<ID3D12CommandQueue> cmdQueue_;
	ComPtr<ID3D12CommandAllocator> uploadCmdAllocator_;
	ComPtr<ID3D12GraphicsCommandList> cmdList_;

	// Render targets
	std::array<ComPtr<ID3D12Resource>, kSwapChainBufferCount> renderTargetBuffers_;
	std::array<D3D12_RESOURCE_STATES, kSwapChainBufferCount> renderTargetStates_{};
	UINT currentBackBufferIndex_ = 0;

	ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
	UINT rtvDescriptorSize_ = 0;

	// Depth stencil
	ComPtr<ID3D12Resource> depthStencilBuffer_;
	ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;
	UINT dsvDescriptorSize_ = 0;

	// Synchronization
	// frame fence
	ComPtr<ID3D12Fence> fence_;
	UINT64 nextFenceValue_ = 1;
	unique_handle fenceEvent_{ nullptr };
	// upload fence
	ComPtr<ID3D12Fence> uploadFence_;
	UINT64 uploadFenceValue_ = 0;
	unique_handle uploadFenceEvent_{ nullptr };

	// View / configuration
	UINT clientWidth_ = 0;
	UINT clientHeight_ = 0;

	bool isMsaaEnabled_ = false;
	UINT msaaQualityLevel_ = 0;
};