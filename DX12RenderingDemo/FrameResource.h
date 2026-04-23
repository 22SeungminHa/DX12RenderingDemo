#pragma once
#include "UploadBuffer.h"
#include "ShaderTypes.h"

class FrameResource
{
public:
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();

    // 명령 할당자는 GPU가 명령들을 다 처리한 후 재설정해야 한다.
    // 따라서 프레임마다 할당자가 필요하다.
    ComPtr<ID3D12CommandAllocator> cmdAllocator_;

    // 상수 버퍼는 그것을 참조하는 명령들을 GPU가 다 처리한 후에 갱신해야 한다.
    // 따라서 프레임마다 상수 버퍼를 새로 만들어야 한다.
    std::unique_ptr<UploadBuffer<PassCB>> passCB_ = nullptr;
    std::unique_ptr<UploadBuffer<ObjectCB>> objectCB_ = nullptr;

    // Fence는 현재 울타리 지점까지의 명령들을 표시하는 값이다.
    // 이 값은 GPU가 아직 이 프레임 자원들을 사용하고 있는지 판정하는 용도로 쓰인다.
    UINT64 fenceValue_ = 0;
};

