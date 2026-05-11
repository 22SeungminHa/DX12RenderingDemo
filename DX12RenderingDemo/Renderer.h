#pragma once
#include "D3DCore.h"
#include "FrameResource.h"
#include "Material.h"

class Scene;
class GameObject;
class Camera;
class MeshRenderer;
class Texture;

struct TextureSrvInfo
{
    UINT descriptorIndex = UINT_MAX;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
};

struct MaterialGpuBinding
{
    UINT startDescriptorIndex = UINT_MAX;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
    bool valid = false;
};

class Renderer
{
private:
    static constexpr UINT kNumFrameResources = 3;
    static constexpr UINT kMaxObjectCount = 1000;
    static constexpr UINT kMaxSrvDescriptorCount = 128;

private:
    D3DCore d3dCore_;

    ComPtr<ID3D12RootSignature> rootSignature_;

    std::vector<std::unique_ptr<FrameResource>> frameResources_;
    FrameResource* currentFrameResource_ = nullptr;
    UINT currentFrameResourceIndex_ = 0;

    ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
    UINT srvDescriptorSize_ = 0;
    UINT nextSrvDescriptorIndex_ = 0;

    std::unordered_map<std::string, TextureSrvInfo> textureSrvTable_;
    std::unordered_map<std::string, MaterialGpuBinding> materialGpuBindingTable_;

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

    void CreateSrvDescriptorHeap();
    void ReleaseSrvDescriptorHeap();

    MaterialGpuBinding CreateMaterialGpuBinding(Material* material);
    TextureSrvInfo CreateTextureSrv(Texture* texture);
    void BindMaterialTextures(Material* material);
    bool CopyTextureSrvToDescriptor(Texture* texture, UINT descriptorIndex);

    // getters
    ID3D12Device* GetDevice() const { return d3dCore_.GetDevice(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return d3dCore_.GetCommandList(); }
    ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }

private:
    void RenderObjects(Scene* scene, Camera* camera);
    void RenderObject(GameObject* object, Camera* camera);
    void DrawMeshRenderer(const MeshRenderer* meshRenderer, Camera* camera);

    void CreateRootSignature();
    void ReleaseRootSignature();

    void CreateFrameResources();
    void ReleaseFrameResources();

    void AdvanceFrameResource();
    void WaitForCurrentFrameResource();

    void UpdateCameraData(Camera* camera);
    void SetViewportsAndScissorRects(Camera* camera);
};