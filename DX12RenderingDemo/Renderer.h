#pragma once
#include "EngineTypes.h"
#include "D3DCore.h"
#include "PostProcessRenderer.h"
#include "SkyboxRenderer.h"
#include "DescriptorAllocator.h"
#include "MaterialBinder.h"
#include "RenderQueueBuilder.h"
#include "GlassRenderer.h"

class Scene;
class GameObject;
class Camera;
class MeshRenderer;
class Texture;
class Material;
class FrameResource;
class AssetManager;

class Renderer
{
private:
    static constexpr UINT kNumFrameResources = 3;
    static constexpr UINT kMaxObjectCount = 1000;
    static constexpr UINT kMaxMaterialCount = 1000;
    static constexpr UINT kMaxSrvDescriptorCount = 1024;
    static constexpr UINT kMaxRtvDescriptorCount = 64;

private:
    D3DCore d3dCore_;

    ComPtr<ID3D12RootSignature> rootSignature_;

    std::vector<std::unique_ptr<FrameResource>> frameResources_;
    FrameResource* currentFrameResource_ = nullptr;
    UINT currentFrameResourceIndex_ = 0;

    DescriptorAllocator srvDescriptorAllocator_;
    RtvDescriptorAllocator rtvDescriptorAllocator_;
    RenderQueueBuilder renderQueueBuilder_;
    MaterialBinder materialBinder_;

    std::unique_ptr<SkyboxRenderer> skyboxRenderer_;
    std::unique_ptr<PostProcessRenderer> postProcessRenderer_;
    std::unique_ptr<GlassRenderer> glassRenderer_;

    RefractionMode refractionMode_ = RefractionMode::SingleCapture;

public:
    Renderer();
    ~Renderer();

public:
    // Lifecycle
    void Initialize(HWND hwnd, UINT width, UINT height);
    void Shutdown();
    void Resize(UINT width, UINT height);

    // Frame Rendering
    void Render(Scene* scene);

    // Scene Loading / Upload
    void ResetUploadCmdList();
    UINT64 ExecuteUploadCmdList();
    bool IsSceneLoadComplete(UINT64 fenceValue) const;
    void WaitForSceneLoad(UINT64 fenceValue);
    
    // GPU Synchronization
    void WaitForGpuComplete();

    bool PrepareSkybox(const SkyboxDesc& skybox, AssetManager& assetManager);
    void ReleaseSkyboxUploadResources();

    // Accessors
    ID3D12Device* GetDevice() const { return d3dCore_.GetDevice(); }
    ID3D12GraphicsCommandList* GetRenderCommandList() const { return d3dCore_.GetRenderCommandList(); }
    ID3D12GraphicsCommandList* GetUploadCommandList() const { return d3dCore_.GetUploadCommandList(); }
    ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }

private:
    // Core Resources
    void CreateRootSignature();
    void ReleaseRootSignature();
    void CreateSrvDescriptorHeap();
    void ReleaseSrvDescriptorHeap();
    void CreateRtvDescriptorHeap();
    void ReleaseRtvDescriptorHeap();

    // Frame Resources
    void CreateFrameResources();
    void ReleaseFrameResources();
    void AdvanceFrameResource();
    void WaitForCurrentFrameResource();

    // Render Queue
    void RenderItem(const RenderItemDesc& item, Camera* camera, RenderPass renderPass = RenderPass::Default);
    void RenderItems(const std::vector<RenderItemDesc>& queue, Camera* camera, RenderPass renderPass = RenderPass::Default);

    void RenderGlassItems(const std::vector<RenderItemDesc>& queue, Camera* camera);
    void RenderSingleCapture(const std::vector<RenderItemDesc>& queue, Camera* camera);
    void RenderPerGlassCapture(const std::vector<RenderItemDesc>& queue, Camera* camera);
    void RenderAccumulation(const std::vector<RenderItemDesc>& queue, Camera* camera);

    // GPU Binding
    void BindCameraData(Scene* scene, Camera* camera);
    void BindObjectData(const GameObject* object);

    // Draw
    void DrawMeshRenderer(const GameObject* object, const MeshRenderer* meshRenderer, Camera* camera, RenderPass renderPass = RenderPass::Default);

    Camera* BeginFrame(Scene* scene);
    void RenderSceneToTexture(Scene* scene, Camera* camera);
    void RenderToBackBuffer(Camera* camera);
    void EndFrame();
};