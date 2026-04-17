#include "Renderer.h"
#include "Scene.h"

void Renderer::Initialize(HWND hwnd, UINT width, UINT height)
{
    d3dCore_.Initialize(hwnd, width, height);
    InitializeCamera(width, height);
}

void Renderer::Shutdown()
{
    d3dCore_.Shutdown();
}

void Renderer::InitializeCamera(UINT width, UINT height)
{
    if (!camera_) camera_ = std::make_unique<Camera>();

    camera_->SetViewport(0, 0, width, height, 0.0f, 1.0f);
    camera_->SetScissorRect(0, 0, width, height);

    float aspect = (height == 0) ? 1.0f : static_cast<float>(width) / static_cast<float>(height);
    camera_->GenerateProjectionMatrix(1.0f, 500.0f, aspect, 90.0f);
    camera_->GenerateViewMatrix(Vector3(0.0f, 15.0f, -25.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3::Up);
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
    float clearColor[4] = { 0.f, 0.f, 0.f, 1.f };

    d3dCore_.ResetCommandList();

    d3dCore_.BeginRender(clearColor);
    if (scene) scene->Render(d3dCore_.GetCommandList(), camera_.get());
    d3dCore_.EndRender();

    d3dCore_.ExecuteCommandList();
    d3dCore_.Present(0, 0);
    d3dCore_.MoveToNextFrame();
}

void Renderer::WaitForGpuComplete()
{
    d3dCore_.WaitForGpuComplete();
}