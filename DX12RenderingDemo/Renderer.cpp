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
    CreateFrameResources();

    materialBinder_.Initialize(d3dCore_.GetDevice(), &srvDescriptorAllocator_);

    skyboxRenderer_ = std::make_unique<SkyboxRenderer>();
    skyboxRenderer_->Initialize(d3dCore_.GetDevice(), rootSignature_.Get(), &srvDescriptorAllocator_);

    postProcessRenderer_ = std::make_unique<PostProcessRenderer>();
    postProcessRenderer_->Initialize(d3dCore_.GetDevice(), rootSignature_.Get(), &srvDescriptorAllocator_, width, height);
}

void Renderer::Shutdown()
{
    if (skyboxRenderer_)
        skyboxRenderer_->Shutdown();

    skyboxRenderer_.reset();

    if (postProcessRenderer_)
        postProcessRenderer_->Shutdown();

    postProcessRenderer_.reset();

    materialBinder_.Shutdown();

    ReleaseFrameResources();
    ReleaseSrvDescriptorHeap();
    ReleaseRootSignature();

    d3dCore_.Shutdown();
}

void Renderer::CreateRootSignature()
{
    D3D12_ROOT_PARAMETER rootParameters[7]{};

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

    // t3 : Bloom texture
    D3D12_DESCRIPTOR_RANGE bloomSrvRange{};
    bloomSrvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    bloomSrvRange.NumDescriptors = 1;
    bloomSrvRange.BaseShaderRegister = 3;
    bloomSrvRange.RegisterSpace = 0;
    bloomSrvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[5].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[5].DescriptorTable.pDescriptorRanges = &bloomSrvRange;
    rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // t4 : Scene color texture for refraction
    D3D12_DESCRIPTOR_RANGE sceneColorSrvRange{};
    sceneColorSrvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    sceneColorSrvRange.NumDescriptors = 1;
    sceneColorSrvRange.BaseShaderRegister = 4;
    sceneColorSrvRange.RegisterSpace = 0;
    sceneColorSrvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[6].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[6].DescriptorTable.pDescriptorRanges = &sceneColorSrvRange;
    rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

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
            errorBlob.Get());

    ThrowIfFailed(
        d3dCore_.GetDevice()->CreateRootSignature(
            0,
            signatureBlob->GetBufferPointer(),
            signatureBlob->GetBufferSize(),
            IID_PPV_ARGS(rootSignature_.GetAddressOf())));
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
}

void Renderer::CreateSrvDescriptorHeap()
{
    srvDescriptorAllocator_.Initialize(d3dCore_.GetDevice(), kMaxSrvDescriptorCount);
}

void Renderer::ReleaseSrvDescriptorHeap()
{
    srvDescriptorAllocator_.Shutdown();
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

    cmdList->SetGraphicsRootConstantBufferView(
        static_cast<UINT>(RootParam::PassCB),
        currentFrameResource_->passCB_->GetResource()->GetGPUVirtualAddress());
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

    d3dCore_.GetRenderCommandList()->SetGraphicsRootConstantBufferView(
        static_cast<UINT>(RootParam::ObjectCB),
        objCBAddress);
}

void Renderer::Resize(UINT width, UINT height)
{
    if (width == 0 || height == 0)
        return;

    d3dCore_.Resize(width, height);

    if (postProcessRenderer_)
        postProcessRenderer_->Resize(width, height);
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
    Camera* camera = BeginFrame(scene);
    if (!camera)
        return;

    RenderSceneToTexture(scene, camera);
    RenderToBackBuffer(camera);
    EndFrame();
}

Camera* Renderer::BeginFrame(Scene* scene)
{
    if (!scene)
        return nullptr;

    Camera* camera = scene->GetActiveCamera();
    if (!camera)
        return nullptr;

    AdvanceFrameResource();
    WaitForCurrentFrameResource();

    d3dCore_.ResetCommandList(currentFrameResource_->cmdAllocator_.Get());

    return camera;
}

void Renderer::RenderSceneToTexture(Scene* scene, Camera* camera)
{
    if (!scene || !camera)
        return;

    auto* cmdList = d3dCore_.GetRenderCommandList();

    if (postProcessRenderer_)
        postProcessRenderer_->BeginSceneRender(cmdList, camera, d3dCore_.GetDsvHandle());

    cmdList->SetGraphicsRootSignature(rootSignature_.Get());

    BindCameraData(camera);

    if (skyboxRenderer_)
        skyboxRenderer_->Render(cmdList, camera, scene->GetSkybox());

    renderQueueBuilder_.Build(scene, camera);

    RenderItems(renderQueueBuilder_.GetOpaqueQueue(), camera);

    if (postProcessRenderer_)
    {
        postProcessRenderer_->CaptureRefractionScene(cmdList);

        cmdList->SetGraphicsRootDescriptorTable(
            static_cast<UINT>(RootParam::SceneColorTexture),
            postProcessRenderer_->GetRefractionSceneSrv());
    }

    RenderItems(renderQueueBuilder_.GetTransparentQueue(), camera);
    if (postProcessRenderer_)
        postProcessRenderer_->EndSceneRender(cmdList);
}

void Renderer::RenderToBackBuffer(Camera* camera)
{
    if (!camera)
        return;

    d3dCore_.BeginRender();

    if (postProcessRenderer_)
        postProcessRenderer_->Render(d3dCore_.GetRenderCommandList(), camera, rootSignature_.Get(), d3dCore_.GetCurrentRtvHandle());

    d3dCore_.EndRender();
}

void Renderer::EndFrame()
{
    d3dCore_.ExecuteCommandList();
    d3dCore_.Present(0, 0);

    currentFrameResource_->fenceValue_ = d3dCore_.SignalFrameFence();
    d3dCore_.MoveToNextFrame();
}

void Renderer::WaitForGpuComplete()
{
    d3dCore_.WaitForGpuComplete();
}

void Renderer::DrawMeshRenderer(const GameObject* object, const MeshRenderer* meshRenderer, Camera* camera)
{
    if (!object || !meshRenderer || !meshRenderer->IsRenderable())
        return;

    Mesh* mesh = meshRenderer->GetMesh();
    Material* material = meshRenderer->GetMaterial();

    if (!mesh || !material)
        return;

    if (!materialBinder_.Bind(d3dCore_.GetRenderCommandList(), material, camera, currentFrameResource_, skyboxRenderer_.get()))
        return;

    mesh->Render(d3dCore_.GetRenderCommandList());
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

bool Renderer::PrepareSkybox(const SkyboxDesc& skybox, AssetManager& assetManager)
{
    if (!skyboxRenderer_)
        return false;

    return skyboxRenderer_->Prepare(
        skybox,
        assetManager,
        d3dCore_.GetUploadCommandList());
}

void Renderer::ReleaseSkyboxUploadResources()
{
    if (skyboxRenderer_)
        skyboxRenderer_->ReleaseUploadResources();
}
