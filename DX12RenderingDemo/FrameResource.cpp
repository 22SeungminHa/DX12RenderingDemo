#include "FrameResource.h"

FrameResource::FrameResource(
    ID3D12Device* device,
    UINT passCount,
    UINT objectCount,
    UINT materialCount) 
{
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(cmdAllocator_.GetAddressOf())));

    passCB_ = std::make_unique<UploadBuffer<PassCB>>(device, passCount, true);
    objectCB_ = std::make_unique<UploadBuffer<ObjectCB>>(device, objectCount, true);
    materialCB_ = std::make_unique<UploadBuffer<MaterialCB>>(device, materialCount, true);

    D3D12_QUERY_HEAP_DESC queryHeapDesc{};
    queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    queryHeapDesc.Count = 2;
    queryHeapDesc.NodeMask = 0;

    ThrowIfFailed(device->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(glassTimestampQueryHeap_.GetAddressOf())));

    const auto readbackHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    const auto readbackDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT64) * 2);

    ThrowIfFailed(
        device->CreateCommittedResource(
            &readbackHeap,
            D3D12_HEAP_FLAG_NONE,
            &readbackDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(glassTimestampReadback_.GetAddressOf())
        )
    );
}

FrameResource::~FrameResource()
{
}