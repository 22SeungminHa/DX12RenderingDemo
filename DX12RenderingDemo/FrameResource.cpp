#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount)
{
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(cmdAllocator_.GetAddressOf())));

    passCB_ = std::make_unique<UploadBuffer<PassCB>>(device, passCount, true);
    objectCB_ = std::make_unique<UploadBuffer<ObjectCB>>(device, objectCount, true);
}

FrameResource::~FrameResource()
{

}