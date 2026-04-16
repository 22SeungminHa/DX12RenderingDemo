#include "Renderer.h"
#include "Scene.h"

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

bool Renderer::Initialize(HWND hwnd, UINT width, UINT height)
{
    d3dCore_.Initialize(hwnd, width, height);
    CreateCamera(width, height);
    return true;
}

void Renderer::Shutdown()
{
    d3dCore_.Shutdown();
}

void Renderer::CreateCamera(UINT width, UINT height)
{
    camera_ = std::make_unique<Camera>();
    camera_->SetViewport(0, 0, width, height, 0.0f, 1.0f);
    camera_->SetScissorRect(0, 0, width, height);
    camera_->GenerateProjectionMatrix(1.0f, 500.0f, float(width) / float(height), 90.0f);
    camera_->GenerateViewMatrix(Vector3(0.0f, 15.0f, -25.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3::Up);
}

void Renderer::Resize(UINT width, UINT height)
{
    if (!camera_ || width == 0 || height == 0) return;

    d3dCore_.Resize(width, height);

    camera_->SetViewport(0, 0, width, height, 0.0f, 1.0f);
    camera_->SetScissorRect(0, 0, width, height);
    camera_->GenerateProjectionMatrix(1.0f, 500.0f, float(width) / float(height), 90.0f);
}

void Renderer::BeginSceneLoad()
{
    d3dCore_.WaitForGpuComplete();
    d3dCore_.ResetCommandList();
}

void Renderer::EndSceneLoad()
{
    d3dCore_.ExecuteCommandList();
    d3dCore_.WaitForGpuComplete();
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

void Renderer::ChangeSwapChainState()
{
}

void Renderer::WaitForGpuComplete()
{
    d3dCore_.WaitForGpuComplete();
}