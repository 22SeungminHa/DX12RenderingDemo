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
#include "AssetManager.h"

Renderer::Renderer() {}
Renderer::~Renderer() {}

void Renderer::Initialize(HWND hwnd, UINT width, UINT height)
{
    d3dCore_.Initialize(hwnd, width, height);

    CreateRootSignature();
    CreateSrvDescriptorHeap();
    CreateSceneRenderTexture(width, height);
    CreateBrightPassTexture(width, height);
    CreateBlurTempTexture(width, height);
    CreateFrameResources();

    skyboxShader_ = std::make_unique<SkyboxShader>();
    skyboxShader_->CreateShader(d3dCore_.GetDevice(), rootSignature_.Get());

    postProcessShader_ = std::make_unique<PostProcessShader>();
    postProcessShader_->CreateShader(d3dCore_.GetDevice(), rootSignature_.Get());

    brightPassShader_ = std::make_unique<BrightPassShader>();
    brightPassShader_->CreateShader(d3dCore_.GetDevice(), rootSignature_.Get());

    horizontalBlurShader_ = std::make_unique<HorizontalBlurShader>();
    horizontalBlurShader_->CreateShader(d3dCore_.GetDevice(), rootSignature_.Get());
    
    verticalBlurShader_ = std::make_unique<VerticalBlurShader>();
    verticalBlurShader_->CreateShader(d3dCore_.GetDevice(), rootSignature_.Get());
}

void Renderer::Shutdown()
{
    skyboxMesh_.reset();
    skyboxShader_.reset();
    skyboxTexture_.reset();
    loadedSkyboxPath_.clear();
    skyboxDescriptorIndex_ = UINT_MAX;
    skyboxGpuHandle_ = {};

    postProcessShader_.reset();

    brightPassShader_.reset();
    ReleaseBrightPassTexture();

    horizontalBlurShader_.reset();
    verticalBlurShader_.reset();
    ReleaseBlurTempTexture();

    ReleaseFrameResources();
    ReleaseSceneRenderTexture();
    ReleaseSrvDescriptorHeap();
    ReleaseRootSignature();

    d3dCore_.Shutdown();
}

void Renderer::CreateRootSignature()
{
    D3D12_ROOT_PARAMETER rootParameters[5]{};

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

    // t2 : Skybox cubemap
    D3D12_DESCRIPTOR_RANGE skyboxSrvRange{};
    skyboxSrvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    skyboxSrvRange.NumDescriptors = 1;
    skyboxSrvRange.BaseShaderRegister = 2;
    skyboxSrvRange.RegisterSpace = 0;
    skyboxSrvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[4].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[4].DescriptorTable.pDescriptorRanges = &skyboxSrvRange;
    rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

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
            1,
            kMaxObjectCount,
            kMaxMaterialCount));
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

    sceneColorSrvDescriptorIndex_ = UINT_MAX;
    sceneColorSrv_ = {};

    brightColorSrvDescriptorIndex_ = UINT_MAX;
    brightColorSrv_ = {};

    blurTempSrvDescriptorIndex_ = UINT_MAX;
    blurTempSrv_ = {};
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

    if (auto iter = materialGpuBindingTable_.find(materialKey); iter != materialGpuBindingTable_.end())
        return iter->second;

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
    binding.gpuHandle.ptr += static_cast<SIZE_T>(startIndex) * srvDescriptorSize_;
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

    D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    dstHandle.ptr += static_cast<SIZE_T>(descriptorIndex) * srvDescriptorSize_;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texture->GetResource()->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = texture->GetResource()->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    d3dCore_.GetDevice()->CreateShaderResourceView(texture->GetResource(), &srvDesc, dstHandle);

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

    const UINT objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectCB));
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
    CreateSceneRenderTexture(width, height);
    CreateBrightPassTexture(width, height);
    CreateBlurTempTexture(width, height);
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

    BeginSceneRender(camera);

    auto* cmdList = d3dCore_.GetRenderCommandList();
    cmdList->SetGraphicsRootSignature(rootSignature_.Get());

    BindCameraData(camera);

    RenderSkybox(scene, camera);

    BuildRenderQueues(scene, camera);

    RenderItems(opaqueQueue_, camera);
    RenderTransparentQueue(camera);

    EndSceneRender();

    RenderBrightPass(camera);
    RenderHorizontalBlur(camera);
    RenderVerticalBlur(camera);

    d3dCore_.BeginRender();
    RenderPostProcess(camera);
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
        if (!object) continue;
        CollectRenderItems(object.get(), camera);
    }
}

void Renderer::CollectRenderItems(GameObject* object, Camera* camera)
{
    if (!object)
        return;

    object->OnPrepareRender();

    MeshRenderer* meshRenderer = object->GetComponent<MeshRenderer>();

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
        CollectRenderItems(child.get(), camera);
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
    materialCB.baseColorTint = material->GetBaseColorTint();
    materialCB.alpha = material->GetAlpha();
    materialCB.fresnelPower = material->GetFresnelPower();
    materialCB.specularStrength = material->GetSpecularStrength();
    materialCB.reflectionStrength = material->GetReflectionStrength();

    currentFrameResource_->materialCB_->CopyData(materialIndex, materialCB);

    const UINT matCBByteSize =
        D3DUtil::CalcConstantBufferByteSize(sizeof(MaterialCB));

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

    if (material->UseEnvironmentReflection())
        BindSkyboxTexture();

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

void Renderer::RenderSkybox(Scene* scene, Camera* camera)
{
    if (!scene || !camera)
        return;

    const SkyboxDesc& skybox = scene->GetSkybox();

    if (!skybox.enabled)
        return;

    if (!skyboxShader_)
        return;

    if (loadedSkyboxPath_ != skybox.cubemapPath)
        return;

    if (!skyboxMesh_ || !skyboxTexture_)
        return;

    auto* cmdList = d3dCore_.GetRenderCommandList();

    ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap_.Get() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootDescriptorTable(4, skyboxGpuHandle_);

    skyboxShader_->Render(cmdList, camera, RenderMode::Opaque);
    skyboxMesh_->Render(cmdList);
}

bool Renderer::PrepareSkyboxResources(const SkyboxDesc& skybox, AssetManager& assetManager) 
{
    if (!skybox.enabled || skybox.cubemapPath.empty())
        return false;

    if (loadedSkyboxPath_ == skybox.cubemapPath && skyboxTexture_ && skyboxMesh_)
        return true;

    auto* device = d3dCore_.GetDevice();
    auto* uploadCmdList = d3dCore_.GetUploadCommandList();

    if (!device || !uploadCmdList)
        return false;

    skyboxTexture_ = assetManager.LoadTexture(device, uploadCmdList, skybox.cubemapPath);

    if (!skyboxMesh_)
        skyboxMesh_ = std::make_unique<SkyboxMesh>(device, uploadCmdList);

    if (skyboxDescriptorIndex_ == UINT_MAX)
    {
        if (nextSrvDescriptorIndex_ + 1 > kMaxSrvDescriptorCount)
        {
            LOG("SRV descriptor heap is full. Skybox SRV failed.");
            return false;
        }

        skyboxDescriptorIndex_ = nextSrvDescriptorIndex_++;
    }

    if (!CreateSkyboxSrvDescriptor(skyboxTexture_.get(), skyboxDescriptorIndex_))
        return false;

    skyboxGpuHandle_ = srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();
    skyboxGpuHandle_.ptr += static_cast<SIZE_T>(skyboxDescriptorIndex_) * srvDescriptorSize_;

    loadedSkyboxPath_ = skybox.cubemapPath;

    return true;
}

bool Renderer::CreateSkyboxSrvDescriptor(Texture* texture, UINT descriptorIndex)
{
    if (!texture || !texture->GetResource() || !srvDescriptorHeap_)
        return false;

    if (descriptorIndex >= kMaxSrvDescriptorCount)
        return false;

    D3D12_CPU_DESCRIPTOR_HANDLE dstHandle =
        srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    dstHandle.ptr += static_cast<SIZE_T>(descriptorIndex) * srvDescriptorSize_;

    const D3D12_RESOURCE_DESC resourceDesc = texture->GetResource()->GetDesc();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = resourceDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels = resourceDesc.MipLevels;
    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

    d3dCore_.GetDevice()->CreateShaderResourceView(
        texture->GetResource(),
        &srvDesc,
        dstHandle
    );

    return true;
}

bool Renderer::PrepareSkybox(const SkyboxDesc& skybox, AssetManager& assetManager)
{
    return PrepareSkyboxResources(skybox, assetManager);
}

void Renderer::ReleaseSkyboxUploadResources()
{
    if (skyboxMesh_)
        skyboxMesh_->ReleaseUploadResources();
}

void Renderer::BindSkyboxTexture()
{
    if (!srvDescriptorHeap_ || skyboxDescriptorIndex_ == UINT_MAX)
        return;

    auto* cmdList = d3dCore_.GetRenderCommandList();

    ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap_.Get() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootDescriptorTable(4, skyboxGpuHandle_);
}

void Renderer::CreateSceneRenderTexture(UINT width, UINT height)
{
    if (width == 0 || height == 0)
        return;

    ReleaseSceneRenderTexture();

    ID3D12Device* device = d3dCore_.GetDevice();
    if (!device || !srvDescriptorHeap_)
        return;

    D3D12_RESOURCE_DESC textureDesc{};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Alignment = 0;
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = 1;
    textureDesc.Format = kSceneColorFormat;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = kSceneColorFormat;
    clearValue.Color[0] = 0.0f;
    clearValue.Color[1] = 0.0f;
    clearValue.Color[2] = 0.0f;
    clearValue.Color[3] = 1.0f;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clearValue,
        IID_PPV_ARGS(sceneColorBuffer_.GetAddressOf())
    ));

    sceneColorState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.NumDescriptors = 1;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ThrowIfFailed(device->CreateDescriptorHeap(
        &rtvHeapDesc,
        IID_PPV_ARGS(sceneColorRtvHeap_.GetAddressOf())
    ));

    sceneColorRtv_ = sceneColorRtvHeap_->GetCPUDescriptorHandleForHeapStart();

    device->CreateRenderTargetView(
        sceneColorBuffer_.Get(),
        nullptr,
        sceneColorRtv_
    );

    if (sceneColorSrvDescriptorIndex_ == UINT_MAX)
    {
        if (nextSrvDescriptorIndex_ + 1 > kMaxSrvDescriptorCount)
        {
            LOG("SRV descriptor heap is full. Scene color SRV failed.");
            return;
        }

        sceneColorSrvDescriptorIndex_ = nextSrvDescriptorIndex_++;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle =
        srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    srvCpuHandle.ptr +=
        static_cast<SIZE_T>(sceneColorSrvDescriptorIndex_) * srvDescriptorSize_;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = kSceneColorFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    device->CreateShaderResourceView(
        sceneColorBuffer_.Get(),
        &srvDesc,
        srvCpuHandle
    );

    sceneColorSrv_ = srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();
    sceneColorSrv_.ptr +=
        static_cast<SIZE_T>(sceneColorSrvDescriptorIndex_) * srvDescriptorSize_;
}

void Renderer::ReleaseSceneRenderTexture()
{
    sceneColorBuffer_.Reset();
    sceneColorRtvHeap_.Reset();

    sceneColorRtv_ = {};
    sceneColorState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}

void Renderer::TransitionSceneColor(D3D12_RESOURCE_STATES afterState)
{
    if (!sceneColorBuffer_)
        return;

    if (sceneColorState_ == afterState)
        return;

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = sceneColorBuffer_.Get();
    barrier.Transition.StateBefore = sceneColorState_;
    barrier.Transition.StateAfter = afterState;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    d3dCore_.GetRenderCommandList()->ResourceBarrier(1, &barrier);

    sceneColorState_ = afterState;
}

void Renderer::BeginSceneRender(Camera* camera)
{
    if (!camera || !sceneColorBuffer_)
        return;

    auto* cmdList = d3dCore_.GetRenderCommandList();

    TransitionSceneColor(D3D12_RESOURCE_STATE_RENDER_TARGET);

    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = d3dCore_.GetDsvHandle();

    cmdList->OMSetRenderTargets(1, &sceneColorRtv_, FALSE, &dsvHandle);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    cmdList->ClearRenderTargetView(sceneColorRtv_, clearColor, 0, nullptr);
    cmdList->ClearDepthStencilView(
        dsvHandle,
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f,
        0,
        0,
        nullptr
    );
}

void Renderer::EndSceneRender()
{
    TransitionSceneColor(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void Renderer::RenderPostProcess(Camera* camera)
{
    if (!camera || !postProcessShader_ || !sceneColorBuffer_)
        return;

    auto* cmdList = d3dCore_.GetRenderCommandList();

    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    cmdList->SetGraphicsRootSignature(rootSignature_.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap_.Get() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootDescriptorTable(3, brightColorSrv_);

    postProcessShader_->Render(cmdList, camera, RenderMode::Opaque);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);
}

void Renderer::CreateBrightPassTexture(UINT width, UINT height)
{
    if (width == 0 || height == 0)
        return;

    ReleaseBrightPassTexture();

    ID3D12Device* device = d3dCore_.GetDevice();
    if (!device || !srvDescriptorHeap_)
        return;

    D3D12_RESOURCE_DESC textureDesc{};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = 1;
    textureDesc.Format = kSceneColorFormat;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = kSceneColorFormat;
    clearValue.Color[0] = 0.0f;
    clearValue.Color[1] = 0.0f;
    clearValue.Color[2] = 0.0f;
    clearValue.Color[3] = 1.0f;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clearValue,
        IID_PPV_ARGS(brightColorBuffer_.GetAddressOf())
    ));

    brightColorState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.NumDescriptors = 1;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ThrowIfFailed(device->CreateDescriptorHeap(
        &rtvHeapDesc,
        IID_PPV_ARGS(brightColorRtvHeap_.GetAddressOf())
    ));

    brightColorRtv_ = brightColorRtvHeap_->GetCPUDescriptorHandleForHeapStart();

    device->CreateRenderTargetView(
        brightColorBuffer_.Get(),
        nullptr,
        brightColorRtv_
    );

    if (brightColorSrvDescriptorIndex_ == UINT_MAX)
    {
        if (nextSrvDescriptorIndex_ + 1 > kMaxSrvDescriptorCount)
        {
            LOG("SRV descriptor heap is full. Bright color SRV failed.");
            return;
        }

        brightColorSrvDescriptorIndex_ = nextSrvDescriptorIndex_++;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle =
        srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    srvCpuHandle.ptr +=
        static_cast<SIZE_T>(brightColorSrvDescriptorIndex_) * srvDescriptorSize_;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = kSceneColorFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(
        brightColorBuffer_.Get(),
        &srvDesc,
        srvCpuHandle
    );

    brightColorSrv_ = srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();
    brightColorSrv_.ptr +=
        static_cast<SIZE_T>(brightColorSrvDescriptorIndex_) * srvDescriptorSize_;
}

void Renderer::ReleaseBrightPassTexture()
{
    brightColorBuffer_.Reset();
    brightColorRtvHeap_.Reset();

    brightColorRtv_ = {};
    brightColorState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}

void Renderer::TransitionBrightColor(D3D12_RESOURCE_STATES afterState)
{
    if (!brightColorBuffer_)
        return;

    if (brightColorState_ == afterState)
        return;

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = brightColorBuffer_.Get();
    barrier.Transition.StateBefore = brightColorState_;
    barrier.Transition.StateAfter = afterState;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    d3dCore_.GetRenderCommandList()->ResourceBarrier(1, &barrier);

    brightColorState_ = afterState;
}

void Renderer::RenderBrightPass(Camera* camera)
{
    if (!camera || !brightPassShader_ || !sceneColorBuffer_ || !brightColorBuffer_)
        return;

    auto* cmdList = d3dCore_.GetRenderCommandList();

    TransitionBrightColor(D3D12_RESOURCE_STATE_RENDER_TARGET);

    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    cmdList->OMSetRenderTargets(1, &brightColorRtv_, FALSE, nullptr);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    cmdList->ClearRenderTargetView(brightColorRtv_, clearColor, 0, nullptr);

    cmdList->SetGraphicsRootSignature(rootSignature_.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap_.Get() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootDescriptorTable(3, sceneColorSrv_);

    brightPassShader_->Render(cmdList, camera, RenderMode::Opaque);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    TransitionBrightColor(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void Renderer::CreateBlurTempTexture(UINT width, UINT height)
{
    if (width == 0 || height == 0)
        return;

    ReleaseBlurTempTexture();

    ID3D12Device* device = d3dCore_.GetDevice();
    if (!device || !srvDescriptorHeap_)
        return;

    D3D12_RESOURCE_DESC textureDesc{};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = 1;
    textureDesc.Format = kSceneColorFormat;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = kSceneColorFormat;
    clearValue.Color[0] = 0.0f;
    clearValue.Color[1] = 0.0f;
    clearValue.Color[2] = 0.0f;
    clearValue.Color[3] = 1.0f;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clearValue,
        IID_PPV_ARGS(blurTempBuffer_.GetAddressOf())
    ));

    blurTempState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.NumDescriptors = 1;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ThrowIfFailed(device->CreateDescriptorHeap(
        &rtvHeapDesc,
        IID_PPV_ARGS(blurTempRtvHeap_.GetAddressOf())
    ));

    blurTempRtv_ = blurTempRtvHeap_->GetCPUDescriptorHandleForHeapStart();

    device->CreateRenderTargetView(
        blurTempBuffer_.Get(),
        nullptr,
        blurTempRtv_
    );

    if (blurTempSrvDescriptorIndex_ == UINT_MAX)
    {
        if (nextSrvDescriptorIndex_ + 1 > kMaxSrvDescriptorCount)
        {
            LOG("SRV descriptor heap is full. Blur temp SRV failed.");
            return;
        }

        blurTempSrvDescriptorIndex_ = nextSrvDescriptorIndex_++;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle =
        srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    srvCpuHandle.ptr +=
        static_cast<SIZE_T>(blurTempSrvDescriptorIndex_) * srvDescriptorSize_;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = kSceneColorFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(
        blurTempBuffer_.Get(),
        &srvDesc,
        srvCpuHandle
    );

    blurTempSrv_ = srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();
    blurTempSrv_.ptr +=
        static_cast<SIZE_T>(blurTempSrvDescriptorIndex_) * srvDescriptorSize_;
}

void Renderer::ReleaseBlurTempTexture()
{
    blurTempBuffer_.Reset();
    blurTempRtvHeap_.Reset();

    blurTempRtv_ = {};
    blurTempState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}

void Renderer::TransitionBlurTemp(D3D12_RESOURCE_STATES afterState)
{
    if (!blurTempBuffer_)
        return;

    if (blurTempState_ == afterState)
        return;

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = blurTempBuffer_.Get();
    barrier.Transition.StateBefore = blurTempState_;
    barrier.Transition.StateAfter = afterState;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    d3dCore_.GetRenderCommandList()->ResourceBarrier(1, &barrier);

    blurTempState_ = afterState;
}

void Renderer::RenderHorizontalBlur(Camera* camera)
{
    if (!camera || !horizontalBlurShader_ || !brightColorBuffer_ || !blurTempBuffer_)
        return;

    auto* cmdList = d3dCore_.GetRenderCommandList();

    TransitionBlurTemp(D3D12_RESOURCE_STATE_RENDER_TARGET);

    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    cmdList->OMSetRenderTargets(1, &blurTempRtv_, FALSE, nullptr);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    cmdList->ClearRenderTargetView(blurTempRtv_, clearColor, 0, nullptr);

    cmdList->SetGraphicsRootSignature(rootSignature_.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap_.Get() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootDescriptorTable(3, brightColorSrv_);

    horizontalBlurShader_->Render(cmdList, camera, RenderMode::Opaque);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    TransitionBlurTemp(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void Renderer::RenderVerticalBlur(Camera* camera)
{
    if (!camera || !verticalBlurShader_ || !blurTempBuffer_ || !brightColorBuffer_)
        return;

    auto* cmdList = d3dCore_.GetRenderCommandList();

    TransitionBrightColor(D3D12_RESOURCE_STATE_RENDER_TARGET);

    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    cmdList->OMSetRenderTargets(1, &brightColorRtv_, FALSE, nullptr);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    cmdList->ClearRenderTargetView(brightColorRtv_, clearColor, 0, nullptr);

    cmdList->SetGraphicsRootSignature(rootSignature_.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap_.Get() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootDescriptorTable(3, blurTempSrv_);

    verticalBlurShader_->Render(cmdList, camera, RenderMode::Opaque);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    TransitionBrightColor(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}