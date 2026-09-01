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
    CreateFrameResources();
    CreateSrvDescriptorHeap();
    CreateRtvDescriptorHeap();

    materialBinder_.Initialize(d3dCore_.GetDevice(), &srvDescriptorAllocator_);

    skyboxRenderer_ = std::make_unique<SkyboxRenderer>();
    skyboxRenderer_->Initialize(d3dCore_.GetDevice(), rootSignature_.Get(), &srvDescriptorAllocator_);

    postProcessRenderer_ = std::make_unique<PostProcessRenderer>();
    postProcessRenderer_->Initialize(d3dCore_.GetDevice(), rootSignature_.Get(), &srvDescriptorAllocator_, &rtvDescriptorAllocator_, width, height);

    glassRenderer_ = std::make_unique<GlassRenderer>();
    glassRenderer_->Initialize(d3dCore_.GetDevice(), rootSignature_.Get(), &srvDescriptorAllocator_, &rtvDescriptorAllocator_, width, height);
}

void Renderer::Shutdown()
{
    if (skyboxRenderer_) skyboxRenderer_->Shutdown();
    skyboxRenderer_.reset();

    if (glassRenderer_) glassRenderer_->Shutdown();
    glassRenderer_.reset();

    if (postProcessRenderer_) postProcessRenderer_->Shutdown();
    postProcessRenderer_.reset();

    materialBinder_.Shutdown();

    ReleaseFrameResources();

    ReleaseRtvDescriptorHeap();
    ReleaseSrvDescriptorHeap();

    ReleaseRootSignature();

    d3dCore_.Shutdown();
}

void Renderer::CreateRootSignature()
{
    constexpr UINT rootParamCount = static_cast<UINT>(RootParam::End);

    std::array<D3D12_ROOT_PARAMETER, rootParamCount> rootParameters{};

    D3D12_DESCRIPTOR_RANGE materialTextureSrvRange{};
    D3D12_DESCRIPTOR_RANGE skyboxSrvRange{};
    D3D12_DESCRIPTOR_RANGE bloomSrvRange{};
    D3D12_DESCRIPTOR_RANGE sceneColorSrvRange{};
    D3D12_DESCRIPTOR_RANGE glassAccumSrvRange{};
    D3D12_DESCRIPTOR_RANGE glassRevealageSrvRange{};

    D3DUtil::InitCbv(rootParameters[static_cast<UINT>(RootParam::ObjectCB)], 0);
    D3DUtil::InitCbv(rootParameters[static_cast<UINT>(RootParam::PassCB)], 1);
    D3DUtil::InitCbv(rootParameters[static_cast<UINT>(RootParam::MaterialCB)], 2);
    D3DUtil::InitSrvTable(rootParameters[static_cast<UINT>(RootParam::MaterialTextures)], materialTextureSrvRange, 0, static_cast<UINT>(TextureType::End));
    D3DUtil::InitSrvTable(rootParameters[static_cast<UINT>(RootParam::SkyboxTexture)], skyboxSrvRange, 2);
    D3DUtil::InitSrvTable(rootParameters[static_cast<UINT>(RootParam::PostProcessTexture)], bloomSrvRange, 3);
    D3DUtil::InitSrvTable(rootParameters[static_cast<UINT>(RootParam::SceneColorTexture)], sceneColorSrvRange, 4);
    D3DUtil::InitSrvTable(rootParameters[static_cast<UINT>(RootParam::GlassAccumTexture)], glassAccumSrvRange, 5);
    D3DUtil::InitSrvTable(rootParameters[static_cast<UINT>(RootParam::GlassRevealageTexture)], glassRevealageSrvRange, 6);

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
    rootSignatureDesc.NumParameters = static_cast<UINT>(rootParameters.size());
    rootSignatureDesc.pParameters = rootParameters.data();

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

void Renderer::CreateRtvDescriptorHeap()
{
    rtvDescriptorAllocator_.Initialize(d3dCore_.GetDevice(), kMaxRtvDescriptorCount);
}

void Renderer::ReleaseRtvDescriptorHeap()
{
    rtvDescriptorAllocator_.Shutdown();
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

void Renderer::BindCameraData(Scene* scene, Camera* camera)
{
    if (!scene || !camera || !currentFrameResource_ || !currentFrameResource_->passCB_)
        return;

    auto* cmdList = d3dCore_.GetRenderCommandList();

    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    PassCB passCB = camera->BuildPassCB();

    const SceneLightDesc& lightDesc = scene->GetLightDesc();

    passCB.ambientColor = lightDesc.ambientColor;
    passCB.specularPower = lightDesc.specularPower;

    UINT lightCount = 0;

    for (const auto& light : scene->GetDirectionalLights())
    {
        if (!light || !light->IsEnabled())
            continue;

        if (lightCount >= kMaxDirectionalLights)
            break;

        passCB.directionalLights[lightCount] = light->GetLightData();
        ++lightCount;
    }

    passCB.directionalLightCount = lightCount;

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

    if (glassRenderer_)
        glassRenderer_->Resize(width, height);
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

    // 이 FrameResource를 마지막으로 사용했던 GPU 작업은
    // 위 Wait에서 완료되었으므로 이제 안전하게 해제 가능.
    currentFrameResource_->transientUploadResources_.clear();

    d3dCore_.ResetCommandList(currentFrameResource_->cmdAllocator_.Get());

    scene->PrepareRenderResources(
        d3dCore_.GetDevice(),
        d3dCore_.GetRenderCommandList(),
        currentFrameResource_->transientUploadResources_
    );

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

    BindCameraData(scene, camera);

    if (skyboxRenderer_)
        skyboxRenderer_->Render(cmdList, camera, scene->GetSkybox());

    renderQueueBuilder_.Build(scene, camera);
    
    RenderItems(renderQueueBuilder_.GetOpaqueQueue(), camera);
    RenderItems(renderQueueBuilder_.GetTransparentQueue(), camera);
    RenderGlassItems(renderQueueBuilder_.GetGlassQueue(), camera);

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

void Renderer::DrawMeshRenderer(const GameObject* object, const MeshRenderer* meshRenderer, Camera* camera, RenderPass renderPass) 
{
    if (!object || !meshRenderer || !meshRenderer->IsRenderable())
        return;

    Mesh* mesh = meshRenderer->GetMesh();
    Material* material = meshRenderer->GetMaterial();

    if (!mesh || !material)
        return;

    if (!materialBinder_.Bind(
        d3dCore_.GetRenderCommandList(),
        material,
        camera,
        currentFrameResource_,
        skyboxRenderer_.get(),
        renderPass))
    {
        return;
    }

    mesh->Render(d3dCore_.GetRenderCommandList());
}

void Renderer::RenderItem(const RenderItemDesc& item, Camera* camera, RenderPass renderPass)
{
    if (!item.object || !item.meshRenderer)
        return;

    BindObjectData(item.object);
    DrawMeshRenderer(item.object, item.meshRenderer, camera, renderPass);
}

void Renderer::RenderItems(const std::vector<RenderItemDesc>& queue, Camera* camera, RenderPass renderPass)
{
    for (const RenderItemDesc& item : queue)
        RenderItem(item, camera, renderPass);
}

void Renderer::RenderGlassItems(const std::vector<RenderItemDesc>& queue, Camera* camera)
{
    if (queue.empty())
        return;

    switch (refractionMode_)
    {
    case RefractionMode::SingleCapture:
        RenderSingleCapture(queue, camera);
        break;

    case RefractionMode::PerGlassCapture:
        RenderPerGlassCapture(queue, camera);
        break;

    case RefractionMode::AccumulationBuffer:
        RenderAccumulation(queue, camera);
        break;

    default:
        RenderPerGlassCapture(queue, camera);
        break;
    }
}

void Renderer::RenderPerGlassCapture(const std::vector<RenderItemDesc>& queue, Camera* camera)
{
    if (!glassRenderer_ || !postProcessRenderer_)
        return;

    auto* cmdList = d3dCore_.GetRenderCommandList();
    RenderTexture& sceneColor = postProcessRenderer_->GetSceneColorTarget();

    for (const RenderItemDesc& item : queue)
    {
        glassRenderer_->CaptureRefractionScene(cmdList, sceneColor);

        cmdList->SetGraphicsRootDescriptorTable(
            static_cast<UINT>(RootParam::SceneColorTexture),
            glassRenderer_->GetRefractionSceneSrv());

        RenderItem(item, camera);
    }
}

void Renderer::RenderSingleCapture(const std::vector<RenderItemDesc>& queue, Camera* camera)
{
    if (!glassRenderer_ || !postProcessRenderer_)
        return;

    auto* cmdList = d3dCore_.GetRenderCommandList();
    RenderTexture& sceneColor = postProcessRenderer_->GetSceneColorTarget();

    glassRenderer_->CaptureRefractionScene(cmdList, sceneColor);

    cmdList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootParam::SceneColorTexture), glassRenderer_->GetRefractionSceneSrv());

    RenderItems(queue, camera);
}

void Renderer::RenderAccumulation(const std::vector<RenderItemDesc>& queue, Camera* camera)
{
    if (!glassRenderer_ || !postProcessRenderer_ || queue.empty())
        return;

    auto* cmdList = d3dCore_.GetRenderCommandList();
    RenderTexture& sceneColor = postProcessRenderer_->GetSceneColorTarget();

    glassRenderer_->CaptureRefractionScene(cmdList, sceneColor);

    cmdList->SetGraphicsRootDescriptorTable(
        static_cast<UINT>(RootParam::SceneColorTexture),
        glassRenderer_->GetRefractionSceneSrv());

    glassRenderer_->BeginAccumulation(cmdList, camera, d3dCore_.GetDsvHandle());
    RenderItems(queue, camera, RenderPass::GlassAccumulation);
    glassRenderer_->EndAccumulation(cmdList);

    glassRenderer_->CompositeAccumulation(
        cmdList,
        camera,
        rootSignature_.Get(),
        sceneColor);
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
