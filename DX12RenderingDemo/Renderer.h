#pragma once
#include "D3DCore.h"
#include "Camera.h"
#include "FrameResource.h"

class Scene;
class GameObject;

class Renderer
{
private:
    static constexpr UINT kNumFrameResources = 3;
    static constexpr UINT kMaxObjectCount = 1000;

private:
    D3DCore d3dCore_;

    std::vector<std::unique_ptr<FrameResource>> frameResources_;
    FrameResource* currentFrameResource_ = nullptr;
    UINT currentFrameResourceIndex_ = 0;

public:
    Renderer() = default;
    ~Renderer() = default;

public:
    // lifecycle
    void Initialize(HWND hwnd, UINT width, UINT height);
    void Shutdown();
    void Resize(UINT width, UINT height);

    // render
    void Render(Scene* scene);

    // load
    void BeginSceneLoad();
    UINT64 EndSceneLoad();
    bool IsSceneLoadComplete(UINT64 fenceValue) const;
    void WaitForSceneLoad(UINT64 fenceValue);
    
    // sync
    void WaitForGpuComplete();

    void UpdateObjectData(const GameObject* object);

    // getters
    ID3D12Device* GetDevice() const { return d3dCore_.GetDevice(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return d3dCore_.GetCommandList(); }

private:
    void CreateFrameResources();
    void ReleaseFrameResources();

    void AdvanceFrameResource();
    void WaitForCurrentFrameResource();

    void UpdateCameraData(Camera* camera);
    void SetViewportsAndScissorRects(Camera* camera);
};