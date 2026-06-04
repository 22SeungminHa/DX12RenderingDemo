#pragma once
#include "EngineTypes.h"
#include "D3DCore.h"
#include "PostProcessRenderer.h"
#include "SkyboxRenderer.h"
#include "DescriptorAllocator.h"

class Scene;
class GameObject;
class Camera;
class MeshRenderer;
class Texture;
class Material;
class FrameResource;
class AssetManager;

struct MaterialGpuBinding
{
    UINT startDescriptorIndex = UINT_MAX;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
    bool valid = false;
};

struct RenderItem
{
    GameObject* object = nullptr;
    MeshRenderer* meshRenderer = nullptr;
    float distanceToCamera = 0.0f;
};

class Renderer
{
private:
    static constexpr UINT kNumFrameResources = 3;
    static constexpr UINT kMaxObjectCount = 1000;
    static constexpr UINT kMaxMaterialCount = 1000;
    static constexpr UINT kMaxSrvDescriptorCount = 1024;

private:
    D3DCore d3dCore_;

    ComPtr<ID3D12RootSignature> rootSignature_;

    std::vector<std::unique_ptr<FrameResource>> frameResources_;
    FrameResource* currentFrameResource_ = nullptr;
    UINT currentFrameResourceIndex_ = 0;

    DescriptorAllocator srvDescriptorAllocator_;

    std::vector<RenderItem> opaqueQueue_;
    std::vector<RenderItem> transparentQueue_;

    UINT nextMaterialCBIndex_ = 0;
    std::unordered_map<std::string, UINT> materialCBIndexTable_;
    std::unordered_map<std::string, MaterialGpuBinding> materialGpuBindingTable_;

    std::unique_ptr<SkyboxRenderer> skyboxRenderer_;
    std::unique_ptr<PostProcessRenderer> postProcessRenderer_;

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

    // Frame Resources
    void CreateFrameResources();
    void ReleaseFrameResources();
    void AdvanceFrameResource();
    void WaitForCurrentFrameResource();

    // Render Queue
    void BuildRenderQueues(Scene* scene, Camera* camera);
    void CollectRenderItems(GameObject* object, Camera* camera);
    void RenderItems(const std::vector<RenderItem>& queue, Camera* camera);
    void RenderTransparentQueue(Camera* camera);

    // GPU Binding
    void BindCameraData(Camera* camera);
    void BindObjectData(const GameObject* object);
    bool BindMaterial(Material* material, Camera* camera);
    void BindMaterialData(const Material* material, UINT materialIndex);
    void BindMaterialTextures(Material* material);
    
    // Material GPU Resources
    UINT GetOrCreateMaterialCBIndex(Material* material);
    MaterialGpuBinding GetOrCreateMaterialGpuBinding(Material* material);
    bool CreateTextureSrvDescriptor(Texture* texture, UINT descriptorIndex);

    // Draw
    void DrawMeshRenderer(const GameObject* object, const MeshRenderer* meshRenderer, Camera* camera);

    Camera* BeginFrame(Scene* scene);
    void RenderSceneToTexture(Scene* scene, Camera* camera);
    void RenderToBackBuffer(Camera* camera);
    void EndFrame();
};