#include "Renderer.h"
#include "Scene.h"
#include "GameObject.h"
#include "Camera.h"
#include "MeshRenderer.h"
#include "Material.h"
#include "Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include "FrameResource.h"

Renderer::Renderer() {}
Renderer::~Renderer() {}

void Renderer::Initialize(HWND hwnd, UINT width, UINT height)
{
    d3dCore_.Initialize(hwnd, width, height);

    CreateRootSignature();
    CreateSrvDescriptorHeap();
    CreateFrameResources();
}

void Renderer::Shutdown()
{
    ReleaseFrameResources();
    ReleaseSrvDescriptorHeap();
    ReleaseRootSignature();

    d3dCore_.Shutdown();
}

void Renderer::CreateRootSignature()
{
    D3D12_ROOT_PARAMETER rootParameters[4]{};

    // b0 : ObjectCB
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].Descriptor.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // b1 : PassCB
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].Descriptor.ShaderRegister = 1;
    rootParameters[1].Descriptor.RegisterSpace = 0;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // b2 : MaterialCB
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[2].Descriptor.ShaderRegister = 2;
    rootParameters[2].Descriptor.RegisterSpace = 0;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // t0 ~ TextureType::end - 1 : Material textures
    D3D12_DESCRIPTOR_RANGE materialTextureSrvRange{};
    materialTextureSrvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    materialTextureSrvRange.NumDescriptors = static_cast<UINT>(TextureType::End);
    materialTextureSrvRange.BaseShaderRegister = 0;
    materialTextureSrvRange.RegisterSpace = 0;
    materialTextureSrvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[3].DescriptorTable.pDescriptorRanges = &materialTextureSrvRange;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MipLODBias = 0.0f;
    sampler.MaxAnisotropy = 1;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.NumParameters = _countof(rootParameters);
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumStaticSamplers = 1;
    rootSignatureDesc.pStaticSamplers = &sampler;
    rootSignatureDesc.Flags = rootSignatureFlags;

    ComPtr<ID3DBlob> signatureBlob;
    ComPtr<ID3DBlob> errorBlob;

    ThrowIfFailedWithBlob(
        D3D12SerializeRootSignature(
            &rootSignatureDesc,
            D3D_ROOT_SIGNATURE_VERSION_1,
            signatureBlob.GetAddressOf(),
            errorBlob.GetAddressOf()),
            errorBlob.Get()
    );

    ThrowIfFailed(
        d3dCore_.GetDevice()->CreateRootSignature(
            0,
            signatureBlob->GetBufferPointer(),
            signatureBlob->GetBufferSize(),
            IID_PPV_ARGS(rootSignature_.GetAddressOf()))
    );
}

void Renderer::ReleaseRootSignature()
{
    rootSignature_.Reset();
}

void Renderer::CreateFrameResources()
{
    frameResources_.clear();
    frameResources_.reserve(kNumFrameResources);

    for (UINT i = 0; i < kNumFrameResources; ++i)
    {
        frameResources_.push_back(std::make_unique<FrameResource>(
            d3dCore_.GetDevice(),
            1,                  // pass count
            kMaxObjectCount));  // object count
    }

    currentFrameResourceIndex_ = 0;
    currentFrameResource_ = frameResources_[0].get();
}

void Renderer::ReleaseFrameResources()
{
    currentFrameResource_ = nullptr;
    frameResources_.clear();

    materialCBIndexTable_.clear();
    nextMaterialCBIndex_ = 0;
}

void Renderer::CreateSrvDescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.NumDescriptors = kMaxSrvDescriptorCount;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.NodeMask = 0;

    ThrowIfFailed(
        d3dCore_.GetDevice()->CreateDescriptorHeap(
            &heapDesc,
            IID_PPV_ARGS(srvDescriptorHeap_.GetAddressOf()))
    );

    srvDescriptorSize_ =
        d3dCore_.GetDevice()->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void Renderer::ReleaseSrvDescriptorHeap()
{
    materialGpuBindingTable_.clear();

    srvDescriptorHeap_.Reset();
    srvDescriptorSize_ = 0;
    nextSrvDescriptorIndex_ = 0;
}

MaterialGpuBinding Renderer::GetOrCreateMaterialGpuBinding(Material* material)
{
    MaterialGpuBinding empty{};

    if (!material || !srvDescriptorHeap_)
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

    if (nextSrvDescriptorIndex_ + materialTextureCount > kMaxSrvDescriptorCount)
    {
        LOG("SRV descriptor heap is full");
        return empty;
    }

    const UINT startIndex = nextSrvDescriptorIndex_;

    for (UINT i = 0; i < materialTextureCount; ++i)
    {
        if (!CreateTextureSrvDescriptor(materialTextures[i], startIndex + i))
        {
            LOG("Failed to copy texture SRV. Material: " << materialKey << ", Slot: " << i);
            return empty;
        }
    }

    MaterialGpuBinding binding{};
    binding.startDescriptorIndex = startIndex;
    binding.gpuHandle = srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();
    binding.gpuHandle.ptr +=
        static_cast<SIZE_T>(startIndex) * srvDescriptorSize_;
    binding.valid = true;

    materialGpuBindingTable_[materialKey] = binding;

    nextSrvDescriptorIndex_ += materialTextureCount;

    return binding;
}

bool Renderer::CreateTextureSrvDescriptor(Texture* texture, UINT descriptorIndex)
{
    if (!texture || !texture->GetResource() || !srvDescriptorHeap_)
        return false;

    if (descriptorIndex >= kMaxSrvDescriptorCount)
        return false;

    D3D12_CPU_DESCRIPTOR_HANDLE dstHandle =
        srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    dstHandle.ptr +=
        static_cast<SIZE_T>(descriptorIndex) *
        srvDescriptorSize_;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texture->GetResource()->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = texture->GetResource()->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    d3dCore_.GetDevice()->CreateShaderResourceView(
        texture->GetResource(),
        &srvDesc,
        dstHandle
    );

    return true;
}

void Renderer::BindMaterialTextures(Material* material)
{
    if (!material || !srvDescriptorHeap_)
        return;

    MaterialGpuBinding binding = GetOrCreateMaterialGpuBinding(material);

    if (!binding.valid)
        return;

    auto* cmdList = d3dCore_.GetRenderCommandList();

    ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap_.Get() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootDescriptorTable(
        3,
        binding.gpuHandle
    );
}

void Renderer::AdvanceFrameResource()
{
    currentFrameResourceIndex_ = (currentFrameResourceIndex_ + 1) % kNumFrameResources;
    currentFrameResource_ = frameResources_[currentFrameResourceIndex_].get();
}

void Renderer::WaitForCurrentFrameResource()
{
    if (!currentFrameResource_) return;
    if (currentFrameResource_->fenceValue_ == 0) return;
    if (d3dCore_.GetCompletedFrameFenceValue() >= currentFrameResource_->fenceValue_) return;

    d3dCore_.WaitForFrameFence(currentFrameResource_->fenceValue_);
}

void Renderer::BindCameraData(Camera* camera)
{
    if (!camera || !currentFrameResource_ || !currentFrameResource_->passCB_)
        return;

    auto* cmdList = d3dCore_.GetRenderCommandList();

    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    PassCB passCB = camera->BuildPassCB();
    currentFrameResource_->passCB_->CopyData(0, passCB);

    cmdList->SetGraphicsRootConstantBufferView(1, currentFrameResource_->passCB_->GetResource()->GetGPUVirtualAddress());
}

void Renderer::BindObjectData(const GameObject* object)
{
    if (!object || !currentFrameResource_ || !currentFrameResource_->objectCB_)
        return;

    Matrix world = object->GetWorldMatrix();

    ObjectCB objectCB{};
    objectCB.world = world.Transpose();

    Matrix worldInvTranspose = world;
    worldInvTranspose.Translation(Vector3::Zero);
    worldInvTranspose = worldInvTranspose.Invert();

    objectCB.worldInvTranspose = worldInvTranspose.Transpose();

    const UINT objectIndex = object->GetObjectCBIndex();
    currentFrameResource_->objectCB_->CopyData(objectIndex, objectCB);

    const UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectCB));
    D3D12_GPU_VIRTUAL_ADDRESS objCBAddress =
        currentFrameResource_->objectCB_->GetResource()->GetGPUVirtualAddress()
        + (static_cast<UINT64>(objectIndex) * objCBByteSize);

    d3dCore_.GetRenderCommandList()->SetGraphicsRootConstantBufferView(0, objCBAddress);
}

void Renderer::Resize(UINT width, UINT height)
{
    if (width == 0 || height == 0)
        return;
    
    d3dCore_.Resize(width, height);
}

void Renderer::ResetUploadCmdList()
{
    d3dCore_.ResetUploadCmdList();
}

UINT64 Renderer::ExecuteUploadCmdList()
{
    return d3dCore_.ExecuteUploadCmdList();
}

bool Renderer::IsSceneLoadComplete(UINT64 fenceValue) const
{
    return d3dCore_.IsUploadFenceComplete(fenceValue);
}

void Renderer::WaitForSceneLoad(UINT64 fenceValue)
{
    d3dCore_.WaitForUploadFence(fenceValue);
}

void Renderer::Render(Scene* scene)
{
    if (!scene) return;

    Camera* camera = scene->GetActiveCamera();
    if (!camera) return;

    AdvanceFrameResource();
    WaitForCurrentFrameResource();

    d3dCore_.ResetCommandList(currentFrameResource_->cmdAllocator_.Get());
    d3dCore_.BeginRender();

    auto* cmdList = d3dCore_.GetRenderCommandList();
    cmdList->SetGraphicsRootSignature(rootSignature_.Get());

    BindCameraData(camera);

    BuildRenderQueues(scene, camera);

    RenderItems(opaqueQueue_, camera);
    RenderTransparentQueue(camera);

    d3dCore_.EndRender();
    d3dCore_.ExecuteCommandList();
    d3dCore_.Present(0, 0);

    currentFrameResource_->fenceValue_ = d3dCore_.SignalFrameFence();
    d3dCore_.MoveToNextFrame();
}

void Renderer::WaitForGpuComplete()
{
    d3dCore_.WaitForGpuComplete();
}

void Renderer::DrawMeshRenderer(
    const GameObject* object,
    const MeshRenderer* meshRenderer,
    Camera* camera)
{
    if (!object || !meshRenderer || !meshRenderer->IsRenderable())
        return;

    Mesh* mesh = meshRenderer->GetMesh();
    Material* material = meshRenderer->GetMaterial();

    if (!mesh || !material)
        return;

    if (!BindMaterial(material, camera))
        return;

    mesh->Render(d3dCore_.GetRenderCommandList());
}

void Renderer::BuildRenderQueues(Scene* scene, Camera* camera)
{
    opaqueQueue_.clear();
    transparentQueue_.clear();

    if (!scene || !camera)
        return;

    const auto& objects = scene->GetObjects();

    for (const auto& object : objects)
    {
        if (!object)
            continue;

        CollectRenderItems(object.get(), camera);
    }
}

void Renderer::CollectRenderItems(GameObject* object, Camera* camera)
{
    if (!object)
        return;

    object->OnPrepareRender();

    MeshRenderer* meshRenderer = object->GetMeshRenderer();

    if (meshRenderer && meshRenderer->IsRenderable())
    {
        Material* material = meshRenderer->GetMaterial();

        RenderItem item{};
        item.object = object;
        item.meshRenderer = meshRenderer;

        Vector3 objectPos = object->GetWorldMatrix().Translation();
        Vector3 cameraPos = camera->GetPosition();

        item.distanceToCamera = Vector3::DistanceSquared(objectPos, cameraPos);

        if (material && material->GetRenderMode() == RenderMode::Transparent)
            transparentQueue_.push_back(item);
        else
            opaqueQueue_.push_back(item);
    }

    for (const auto& child : object->GetChildren())
    {
        CollectRenderItems(child.get(), camera);
    }
}

void Renderer::RenderItems(const std::vector<RenderItem>& queue, Camera* camera)
{
    for (const RenderItem& item : queue)
    {
        if (!item.object || !item.meshRenderer)
            continue;

        BindObjectData(item.object);
        DrawMeshRenderer(item.object, item.meshRenderer, camera);
    }
}

void Renderer::RenderTransparentQueue(Camera* camera)
{
    std::sort(transparentQueue_.begin(), transparentQueue_.end(),
        [](const RenderItem& a, const RenderItem& b) {
            return a.distanceToCamera > b.distanceToCamera;
        }
    );

    RenderItems(transparentQueue_, camera);
}

void Renderer::BindMaterialData(const Material* material, UINT materialIndex)
{
    if (!material || !currentFrameResource_ || !currentFrameResource_->materialCB_)
        return;

    MaterialCB materialCB{};
    materialCB.baseColor = material->GetBaseColor();
    materialCB.alpha = material->GetAlpha();
    materialCB.fresnelPower = material->GetFresnelPower();
    materialCB.specularStrength = material->GetSpecularStrength();

    currentFrameResource_->materialCB_->CopyData(materialIndex, materialCB);

    const UINT matCBByteSize =
        d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialCB));

    D3D12_GPU_VIRTUAL_ADDRESS matCBAddress =
        currentFrameResource_->materialCB_->GetResource()->GetGPUVirtualAddress()
        + static_cast<UINT64>(materialIndex) * matCBByteSize;

    d3dCore_.GetRenderCommandList()->SetGraphicsRootConstantBufferView(
        2,
        matCBAddress
    );
}

UINT Renderer::GetOrCreateMaterialCBIndex(Material* material)
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

bool Renderer::BindMaterial(Material* material, Camera* camera)
{
    if (!material)
        return false;

    Shader* shader = material->GetShader();
    if (!shader)
        return false;

    BindMaterialTextures(material);

    const UINT materialCBIndex = GetOrCreateMaterialCBIndex(material);
    if (materialCBIndex == UINT_MAX)
        return false;

    BindMaterialData(material, materialCBIndex);

    shader->Render(
        d3dCore_.GetRenderCommandList(),
        camera,
        material->GetRenderMode()
    );

    return true;
}