#pragma once
#include "EngineTypes.h"
#include "D3DCore.h"

class Scene;
class GameObject;
class Camera;
class MeshRenderer;
class Texture;
class Material;
class FrameResource;
class SkyboxShader;
class SkyboxMesh;
class AssetManager;
class PostProcessShader;
class BrightPassShader;
class HorizontalBlurShader;

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
    static constexpr DXGI_FORMAT kSceneColorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

private:
    D3DCore d3dCore_;

    ComPtr<ID3D12RootSignature> rootSignature_;

    std::vector<std::unique_ptr<FrameResource>> frameResources_;
    FrameResource* currentFrameResource_ = nullptr;
    UINT currentFrameResourceIndex_ = 0;

    ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
    UINT srvDescriptorSize_ = 0;
    UINT nextSrvDescriptorIndex_ = 0;

    std::vector<RenderItem> opaqueQueue_;
    std::vector<RenderItem> transparentQueue_;

    UINT nextMaterialCBIndex_ = 0;
    std::unordered_map<std::string, UINT> materialCBIndexTable_;
    std::unordered_map<std::string, MaterialGpuBinding> materialGpuBindingTable_;

    std::unique_ptr<SkyboxShader> skyboxShader_;
    std::unique_ptr<SkyboxMesh> skyboxMesh_;

    std::shared_ptr<Texture> skyboxTexture_;
    D3D12_GPU_DESCRIPTOR_HANDLE skyboxGpuHandle_{};
    UINT skyboxDescriptorIndex_ = UINT_MAX;
    std::filesystem::path loadedSkyboxPath_;

    ComPtr<ID3D12Resource> sceneColorBuffer_;
    ComPtr<ID3D12DescriptorHeap> sceneColorRtvHeap_;
    D3D12_CPU_DESCRIPTOR_HANDLE sceneColorRtv_{};
    D3D12_GPU_DESCRIPTOR_HANDLE sceneColorSrv_{};
    UINT sceneColorSrvDescriptorIndex_ = UINT_MAX;
    D3D12_RESOURCE_STATES sceneColorState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    std::unique_ptr<PostProcessShader> postProcessShader_;

    ComPtr<ID3D12Resource> brightColorBuffer_;
    ComPtr<ID3D12DescriptorHeap> brightColorRtvHeap_;
    D3D12_CPU_DESCRIPTOR_HANDLE brightColorRtv_{};
    D3D12_GPU_DESCRIPTOR_HANDLE brightColorSrv_{};
    UINT brightColorSrvDescriptorIndex_ = UINT_MAX;
    D3D12_RESOURCE_STATES brightColorState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    std::unique_ptr<BrightPassShader> brightPassShader_;

    ComPtr<ID3D12Resource> blurTempBuffer_;
    ComPtr<ID3D12DescriptorHeap> blurTempRtvHeap_;
    D3D12_CPU_DESCRIPTOR_HANDLE blurTempRtv_{};
    D3D12_GPU_DESCRIPTOR_HANDLE blurTempSrv_{};
    UINT blurTempSrvDescriptorIndex_ = UINT_MAX;
    D3D12_RESOURCE_STATES blurTempState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    std::unique_ptr<HorizontalBlurShader> horizontalBlurShader_;

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

    void RenderSkybox(Scene* scene, Camera* camera);
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
    void BindSkyboxTexture();
    
    // Material GPU Resources
    UINT GetOrCreateMaterialCBIndex(Material* material);
    MaterialGpuBinding GetOrCreateMaterialGpuBinding(Material* material);
    bool CreateTextureSrvDescriptor(Texture* texture, UINT descriptorIndex);

    // Draw
    void DrawMeshRenderer(const GameObject* object, const MeshRenderer* meshRenderer, Camera* camera);

    bool PrepareSkyboxResources(const SkyboxDesc& skybox, AssetManager& assetManager);
    bool CreateSkyboxSrvDescriptor(Texture* texture, UINT descriptorIndex);

    void CreateSceneRenderTexture(UINT width, UINT height);
    void ReleaseSceneRenderTexture();

    void BeginSceneRender(Camera* camera);
    void EndSceneRender();
    void TransitionSceneColor(D3D12_RESOURCE_STATES afterState);
    
    void RenderPostProcess(Camera* camera);

    void CreateBrightPassTexture(UINT width, UINT height);
    void ReleaseBrightPassTexture();
    void TransitionBrightColor(D3D12_RESOURCE_STATES afterState);
    void RenderBrightPass(Camera* camera);

    void CreateBlurTempTexture(UINT width, UINT height);
    void ReleaseBlurTempTexture();
    void TransitionBlurTemp(D3D12_RESOURCE_STATES afterState);
    void RenderHorizontalBlur(Camera* camera);
};