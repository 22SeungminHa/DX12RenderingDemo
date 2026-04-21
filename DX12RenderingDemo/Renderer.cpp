#include "Renderer.h"
#include "Scene.h"

void Renderer::Initialize(HWND hwnd, UINT width, UINT height)
{
    d3dCore_.Initialize(hwnd, width, height);
    CreateShaderVariables();
}

void Renderer::Shutdown()
{
    ReleaseShaderVariables();
    d3dCore_.Shutdown();
}

void Renderer::CreateShaderVariables()
{
    passCB_ = std::make_unique<UploadBuffer<PassCB>>(d3dCore_.GetDevice(), 1, true);
}

void Renderer::UpdateCameraData(Camera* camera)
{
    if (!camera || !passCB_)
        return;

    auto* cmdList = d3dCore_.GetCommandList();

    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    PassCB passCB = camera->BuildPassCB();
    passCB_->CopyData(0, passCB);

    cmdList->SetGraphicsRootConstantBufferView(1, passCB_->GetResource()->GetGPUVirtualAddress());
}

void Renderer::ReleaseShaderVariables()
{
    passCB_.reset();
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

    //if (camera_)
    //{
    //    camera_->SetViewport(0, 0, width, height, 0.0f, 1.0f);
    //    camera_->SetScissorRect(0, 0, width, height);

    //    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    //    camera_->GenerateProjectionMatrix(1.0f, 500.0f, aspect, 90.0f);
    //}
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

    float clearColor[4] = { 0.f, 0.f, 0.f, 1.f };

    d3dCore_.ResetCommandList();
    d3dCore_.BeginRender(clearColor);

    auto* cmdList = d3dCore_.GetCommandList();
    cmdList->SetGraphicsRootSignature(scene->GetRootSignature());
    UpdateCameraData(camera);
    scene->Render(d3dCore_.GetCommandList());

    d3dCore_.EndRender();
    d3dCore_.ExecuteCommandList();
    d3dCore_.Present(0, 0);
    d3dCore_.MoveToNextFrame();
}

void Renderer::WaitForGpuComplete()
{
    d3dCore_.WaitForGpuComplete();
}