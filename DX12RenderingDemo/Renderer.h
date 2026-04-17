#pragma once
#include "D3DCore.h"
#include "Camera.h"

class Scene;

class Renderer
{
private:
    D3DCore d3dCore_;
    std::unique_ptr<Camera> camera_;

public:
    Renderer() = default;
    ~Renderer() = default;

public:
    // lifecycle
    void Initialize(HWND hwnd, UINT width, UINT height);
    void Shutdown();

    // setup
    void InitializeCamera(UINT width, UINT height);

    // render
    void Render(Scene* scene);

    // load
    void BeginSceneLoad();
    UINT64 EndSceneLoad();                      // º¯°æ
    bool IsSceneLoadComplete(UINT64 fenceValue) const;
    void WaitForSceneLoad(UINT64 fenceValue);
    
    // sync
    void WaitForGpuComplete();

    // getters
    ID3D12Device* GetDevice() const { return d3dCore_.GetDevice(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return d3dCore_.GetCommandList(); }
    Camera* GetCamera() const { return camera_.get(); }
};