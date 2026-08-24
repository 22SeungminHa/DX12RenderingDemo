#include "DescriptorAllocator.h"

void DescriptorAllocator::Initialize(ID3D12Device* device, UINT maxCount)
{
    Shutdown();

    if (!device || maxCount == 0)
        return;

    maxCount_ = maxCount;

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.NumDescriptors = maxCount_;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.NodeMask = 0;

    ThrowIfFailed(device->CreateDescriptorHeap(
        &heapDesc,
        IID_PPV_ARGS(heap_.GetAddressOf())));

    descriptorSize_ =
        device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    nextIndex_ = 0;
}

void DescriptorAllocator::Shutdown()
{
    heap_.Reset();

    descriptorSize_ = 0;
    maxCount_ = 0;
    nextIndex_ = 0;
}

DescriptorAllocation DescriptorAllocator::Allocate(UINT count)
{
    DescriptorAllocation allocation{};

    if (!heap_ || count == 0)
        return allocation;

    if (nextIndex_ + count > maxCount_)
    {
        LOG("Descriptor heap is full");
        return allocation;
    }

    allocation.startIndex = nextIndex_;
    allocation.count = count;
    allocation.cpuHandle = GetCpuHandle(nextIndex_);
    allocation.gpuHandle = GetGpuHandle(nextIndex_);

    nextIndex_ += count;

    return allocation;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::GetCpuHandle(UINT index) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};

    if (!heap_ || index >= maxCount_)
        return handle;

    handle = heap_->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * descriptorSize_;

    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorAllocator::GetGpuHandle(UINT index) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle{};

    if (!heap_ || index >= maxCount_)
        return handle;

    handle = heap_->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * descriptorSize_;

    return handle;
}

void RtvDescriptorAllocator::Initialize(ID3D12Device* device, UINT maxCount)
{
    Shutdown();

    if (!device || maxCount == 0)
        return;

    maxCount_ = maxCount;

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.NumDescriptors = maxCount_;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask = 0;

    ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(heap_.GetAddressOf())));

    descriptorSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    nextIndex_ = 0;
}

void RtvDescriptorAllocator::Shutdown()
{
    heap_.Reset();

    descriptorSize_ = 0;
    maxCount_ = 0;
    nextIndex_ = 0;
}

RtvDescriptorAllocation
RtvDescriptorAllocator::Allocate(UINT count)
{
    RtvDescriptorAllocation allocation{};

    if (!heap_ || count == 0)
        return allocation;

    if (nextIndex_ + count > maxCount_)
    {
        LOG("RTV descriptor heap is full");
        return allocation;
    }

    allocation.startIndex = nextIndex_;
    allocation.count = count;
    allocation.cpuHandle = GetCpuHandle(nextIndex_);

    nextIndex_ += count;

    return allocation;
}

D3D12_CPU_DESCRIPTOR_HANDLE
RtvDescriptorAllocator::GetCpuHandle(UINT index) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};

    if (!heap_ || index >= maxCount_)
        return handle;

    handle = heap_->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * descriptorSize_;

    return handle;
}