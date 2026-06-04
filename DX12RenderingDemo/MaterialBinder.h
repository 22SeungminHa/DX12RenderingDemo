#pragma once
#include "EngineTypes.h"
#include "DescriptorAllocator.h"

class Camera;
class Texture;
class Material;
class FrameResource;
class SkyboxRenderer;

struct MaterialGpuBinding
{
    UINT startDescriptorIndex = UINT_MAX;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
    bool valid = false;
};

class MaterialBinder
{
public:
    void Initialize(ID3D12Device* device, DescriptorAllocator* srvAllocator);
    void Shutdown();

    bool Bind(
        ID3D12GraphicsCommandList* cmdList,
        Material* material,
        Camera* camera,
        FrameResource* frameResource,
        SkyboxRenderer* skyboxRenderer);

private:
    UINT GetOrCreateMaterialCBIndex(Material* material);
    MaterialGpuBinding GetOrCreateMaterialGpuBinding(Material* material);
    bool CreateTextureSrvDescriptor(Texture* texture, UINT descriptorIndex);

    void BindMaterialTextures(ID3D12GraphicsCommandList* cmdList, Material* material);
    void BindMaterialData(ID3D12GraphicsCommandList* cmdList, const Material* material, UINT materialIndex, FrameResource* frameResource);

private:
    ID3D12Device* device_ = nullptr;
    DescriptorAllocator* srvAllocator_ = nullptr;

    UINT nextMaterialCBIndex_ = 0;
    std::unordered_map<std::string, UINT> materialCBIndexTable_;
    std::unordered_map<std::string, MaterialGpuBinding> materialGpuBindingTable_;
};