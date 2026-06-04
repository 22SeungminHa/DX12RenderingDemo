#include "MaterialBinder.h"
#include "Material.h"
#include "Texture.h"
#include "Shader.h"
#include "FrameResource.h"
#include "Camera.h"
#include "SkyboxRenderer.h"

void MaterialBinder::Initialize(ID3D12Device* device, DescriptorAllocator* srvAllocator)
{
    device_ = device;
    srvAllocator_ = srvAllocator;
}

void MaterialBinder::Shutdown()
{
    materialCBIndexTable_.clear();
    materialGpuBindingTable_.clear();

    nextMaterialCBIndex_ = 0;

    device_ = nullptr;
    srvAllocator_ = nullptr;
}

bool MaterialBinder::Bind(
    ID3D12GraphicsCommandList* cmdList,
    Material* material,
    Camera* camera,
    FrameResource* frameResource,
    SkyboxRenderer* skyboxRenderer)
{
    if (!cmdList || !material || !frameResource)
        return false;

    Shader* shader = material->GetShader();
    if (!shader)
        return false;

    BindMaterialTextures(cmdList, material);

    if (material->UseEnvironmentReflection() && skyboxRenderer)
        skyboxRenderer->BindSkyboxTexture(cmdList);

    const UINT materialCBIndex = GetOrCreateMaterialCBIndex(material);
    if (materialCBIndex == UINT_MAX)
        return false;

    BindMaterialData(cmdList, material, materialCBIndex, frameResource);

    shader->Render(cmdList, camera, material->GetRenderMode());

    return true;
}

UINT MaterialBinder::GetOrCreateMaterialCBIndex(Material* material)
{
    if (!material)
        return UINT_MAX;

    const std::string& materialKey = material->GetKey();
    if (materialKey.empty())
        return UINT_MAX;

    if (material->GetMaterialCBIndex() != UINT_MAX)
        return material->GetMaterialCBIndex();

    if (auto iter = materialCBIndexTable_.find(materialKey);
        iter != materialCBIndexTable_.end())
    {
        material->SetMaterialCBIndex(iter->second);
        return iter->second;
    }

    const UINT index = nextMaterialCBIndex_++;
    materialCBIndexTable_[materialKey] = index;
    material->SetMaterialCBIndex(index);

    return index;
}

MaterialGpuBinding MaterialBinder::GetOrCreateMaterialGpuBinding(Material* material)
{
    MaterialGpuBinding empty{};

    if (!material || !srvAllocator_ || !srvAllocator_->GetHeap())
        return empty;

    const std::string& materialKey = material->GetKey();

    if (materialKey.empty())
    {
        LOG("Material key is empty");
        return empty;
    }

    if (auto iter = materialGpuBindingTable_.find(materialKey);
        iter != materialGpuBindingTable_.end())
    {
        return iter->second;
    }

    constexpr UINT materialTextureCount = static_cast<UINT>(TextureType::End);

    std::array<Texture*, materialTextureCount> materialTextures{};

    for (UINT i = 0; i < materialTextureCount; ++i)
    {
        TextureType type = static_cast<TextureType>(i);
        Texture* texture = material->GetTexture(type);

        if (!texture || !texture->GetResource())
        {
            LOG("Material texture slot is empty. Material: " << materialKey << ", Slot: " << i);
            return empty;
        }

        materialTextures[i] = texture;
    }

    DescriptorAllocation allocation = srvAllocator_->Allocate(materialTextureCount);

    if (!allocation.IsValid())
    {
        LOG("SRV descriptor heap is full");
        return empty;
    }

    for (UINT i = 0; i < materialTextureCount; ++i)
    {
        if (!CreateTextureSrvDescriptor(materialTextures[i], allocation.startIndex + i))
        {
            LOG("Failed to copy texture SRV. Material: " << materialKey << ", Slot: " << i);
            return empty;
        }
    }

    MaterialGpuBinding binding{};
    binding.startDescriptorIndex = allocation.startIndex;
    binding.gpuHandle = allocation.gpuHandle;
    binding.valid = true;

    materialGpuBindingTable_[materialKey] = binding;

    return binding;
}

bool MaterialBinder::CreateTextureSrvDescriptor(Texture* texture, UINT descriptorIndex)
{
    if (!device_ || !texture || !texture->GetResource() || !srvAllocator_)
        return false;

    if (!srvAllocator_->IsValidIndex(descriptorIndex))
        return false;

    D3D12_CPU_DESCRIPTOR_HANDLE dstHandle =
        srvAllocator_->GetCpuHandle(descriptorIndex);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texture->GetResource()->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = texture->GetResource()->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    device_->CreateShaderResourceView(
        texture->GetResource(),
        &srvDesc,
        dstHandle);

    return true;
}

void MaterialBinder::BindMaterialTextures(
    ID3D12GraphicsCommandList* cmdList,
    Material* material)
{
    if (!cmdList || !material || !srvAllocator_ || !srvAllocator_->GetHeap())
        return;

    MaterialGpuBinding binding = GetOrCreateMaterialGpuBinding(material);

    if (!binding.valid)
        return;

    ID3D12DescriptorHeap* descriptorHeaps[] = { srvAllocator_->GetHeap() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootDescriptorTable(
        static_cast<UINT>(RootParam::MaterialTextures),
        binding.gpuHandle);
}

void MaterialBinder::BindMaterialData(
    ID3D12GraphicsCommandList* cmdList,
    const Material* material,
    UINT materialIndex,
    FrameResource* frameResource)
{
    if (!cmdList || !material || !frameResource || !frameResource->materialCB_)
        return;

    MaterialCB materialCB{};
    materialCB.baseColorTint = material->GetBaseColorTint();
    materialCB.alpha = material->GetAlpha();
    materialCB.fresnelPower = material->GetFresnelPower();
    materialCB.specularStrength = material->GetSpecularStrength();
    materialCB.reflectionStrength = material->GetReflectionStrength();

    frameResource->materialCB_->CopyData(materialIndex, materialCB);

    const UINT matCBByteSize =
        D3DUtil::CalcConstantBufferByteSize(sizeof(MaterialCB));

    D3D12_GPU_VIRTUAL_ADDRESS matCBAddress =
        frameResource->materialCB_->GetResource()->GetGPUVirtualAddress()
        + static_cast<UINT64>(materialIndex) * matCBByteSize;

    cmdList->SetGraphicsRootConstantBufferView(
        static_cast<UINT>(RootParam::MaterialCB),
        matCBAddress);
}