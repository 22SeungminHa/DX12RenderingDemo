#pragma once
#include "pch.h"

struct DescriptorAllocation
{
    UINT startIndex = UINT_MAX;
    UINT count = 0;

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};

    bool IsValid() const { return startIndex != UINT_MAX && count > 0; }
};

struct RtvDescriptorAllocation
{
    UINT startIndex = UINT_MAX;
    UINT count = 0;

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};

    bool IsValid() const { return startIndex != UINT_MAX && count > 0; }
};

class DescriptorAllocator
{
public:
    DescriptorAllocator() = default;
    ~DescriptorAllocator() = default;

    DescriptorAllocator(const DescriptorAllocator&) = delete;
    DescriptorAllocator& operator=(const DescriptorAllocator&) = delete;

    void Initialize(ID3D12Device* device, UINT maxCount);
    void Shutdown();

    DescriptorAllocation Allocate(UINT count = 1);

    ID3D12DescriptorHeap* GetHeap() const { return heap_.Get(); }
    UINT GetDescriptorSize() const { return descriptorSize_; }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT index) const;

    bool IsValidIndex(UINT index) const { return index < maxCount_; }

private:
    ComPtr<ID3D12DescriptorHeap> heap_;

    UINT descriptorSize_ = 0;
    UINT maxCount_ = 0;
    UINT nextIndex_ = 0;
};

class RtvDescriptorAllocator
{
public:
    RtvDescriptorAllocator() = default;
    ~RtvDescriptorAllocator() = default;

    RtvDescriptorAllocator(const RtvDescriptorAllocator&) = delete;
    RtvDescriptorAllocator& operator=(const RtvDescriptorAllocator&) = delete;

    void Initialize(ID3D12Device* device, UINT maxCount);
    void Shutdown();

    RtvDescriptorAllocation Allocate(UINT count = 1);

    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT index) const;

private:
    ComPtr<ID3D12DescriptorHeap> heap_;

    UINT descriptorSize_ = 0;
    UINT maxCount_ = 0;
    UINT nextIndex_ = 0;
};