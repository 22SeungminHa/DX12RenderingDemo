#include "Renderer.h"
#include "Scene.h"
#include "GameObject.h"
#include "Camera.h"
#include "MeshRenderer.h"
#include "Material.h"
#include "Shader.h"
#include "Mesh.h"

void Renderer::Initialize(HWND hwnd, UINT width, UINT height)
{
    d3dCore_.Initialize(hwnd, width, height);

    CreateRootSignature();
    CreateFrameResources();
}

void Renderer::Shutdown()
{
    ReleaseFrameResources();
    ReleaseRootSignature();

    d3dCore_.Shutdown();
}

void Renderer::CreateRootSignature()
{
    D3D12_ROOT_PARAMETER rootParameters[2]{};

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

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.NumParameters = _countof(rootParameters);
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumStaticSamplers = 0;
    rootSignatureDesc.pStaticSamplers = nullptr;
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
    if (d3dCore_.GetCompletedFenceValue() >= currentFrameResource_->fenceValue_) return;

    d3dCore_.WaitForFenceValue(currentFrameResource_->fenceValue_);
}

void Renderer::UpdateCameraData(Camera* camera)
{
    if (!camera || !currentFrameResource_ || !currentFrameResource_->passCB_)
        return;

    auto* cmdList = d3dCore_.GetCommandList();

    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    PassCB passCB = camera->BuildPassCB();
    currentFrameResource_->passCB_->CopyData(0, passCB);

    cmdList->SetGraphicsRootConstantBufferView(1, currentFrameResource_->passCB_->GetResource()->GetGPUVirtualAddress());
}

void Renderer::UpdateObjectData(const GameObject* object)
{
    if (!object || !currentFrameResource_ || !currentFrameResource_->objectCB_)
        return;

    Matrix world = object->GetWorldMatrix();

    ObjectCB objectCB{};
    objectCB.world = world.Transpose();

    Matrix worldInvTranspose = world;
    worldInvTranspose.Translation(Vector3::Zero);
    worldInvTranspose = worldInvTranspose.Invert().Transpose();

    objectCB.worldInvTranspose = worldInvTranspose.Transpose();

    const UINT objectIndex = object->GetObjectCBIndex();
    currentFrameResource_->objectCB_->CopyData(objectIndex, objectCB);

    const UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectCB));
    D3D12_GPU_VIRTUAL_ADDRESS objCBAddress =
        currentFrameResource_->objectCB_->GetResource()->GetGPUVirtualAddress()
        + (static_cast<UINT64>(objectIndex) * objCBByteSize);

    d3dCore_.GetCommandList()->SetGraphicsRootConstantBufferView(0, objCBAddress);
}

void Renderer::SetViewportsAndScissorRects(Camera* camera)
{
    auto* cmdList = d3dCore_.GetCommandList();
    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);
}

void Renderer::Resize(UINT width, UINT height)
{
    if (width == 0 || height == 0)
        return;
    
    d3dCore_.Resize(width, height);
}

void Renderer::BeginSceneLoad()
{
    d3dCore_.ResetUploadCommandList();
}

UINT64 Renderer::EndSceneLoad()
{
    return d3dCore_.ExecuteUploadCommandList();
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

    auto* cmdList = d3dCore_.GetCommandList();
    cmdList->SetGraphicsRootSignature(rootSignature_.Get());

    UpdateCameraData(camera);

    RenderObjects(scene, camera);

    d3dCore_.EndRender();
    d3dCore_.ExecuteCommandList();
    d3dCore_.Present(0, 0);

    currentFrameResource_->fenceValue_ = d3dCore_.Signal();
    d3dCore_.MoveToNextFrame();
}

void Renderer::WaitForGpuComplete()
{
    d3dCore_.WaitForGpuComplete();
}

void Renderer::RenderObjects(Scene* scene, Camera* camera)
{
    if (!scene || !camera) return;

    const auto& objects = scene->GetObjects();

    for (const auto& object : objects)
    {
        if (!object) continue;
        RenderObject(object.get(), camera);
    }
}

void Renderer::RenderObject(GameObject* object, Camera* camera)
{
    if (!object) return;

    object->OnPrepareRender();

    UpdateObjectData(object);

    DrawMeshRenderer(object->GetMeshRenderer(), camera);

    for (const auto& child : object->GetChildren())
    {
        RenderObject(child.get(), camera);
    }
}

void Renderer::DrawMeshRenderer(const MeshRenderer* meshRenderer, Camera* camera)
{
    if (!meshRenderer || !meshRenderer->IsRenderable())
        return;

    auto* cmdList = d3dCore_.GetCommandList();

    Material* material = meshRenderer->GetMaterial();
    Mesh* mesh = meshRenderer->GetMesh();

    if (material && material->GetShader())
        material->GetShader()->Render(cmdList, camera);

    if (mesh)
        mesh->Render(cmdList);
}