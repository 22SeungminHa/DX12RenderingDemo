#pragma once
#include "UploadBuffer.h"
#include "EngineTypes.h"

class FrameResource
{
public:
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();

    ComPtr<ID3D12CommandAllocator> cmdAllocator_;

    std::unique_ptr<UploadBuffer<PassCB>> passCB_ = nullptr;
    std::unique_ptr<UploadBuffer<ObjectCB>> objectCB_ = nullptr;
    std::unique_ptr<UploadBuffer<MaterialCB>> materialCB_ = nullptr;

    UINT64 fenceValue_ = 0;
};

