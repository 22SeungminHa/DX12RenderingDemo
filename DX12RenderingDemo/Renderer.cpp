#include "Renderer.h"
#include "Scene.h"

CRenderer::CRenderer()
{
}

CRenderer::~CRenderer()
{
}

bool CRenderer::Initialize(HWND hWnd, UINT width, UINT height)
{
    mD3DCore.Initialize(hWnd, width, height);
    CreateCamera(width, height);
    return true;
}

void CRenderer::Shutdown()
{
    mD3DCore.Shutdown();
}

void CRenderer::CreateCamera(UINT width, UINT height)
{
    mCamera = std::make_unique<CCamera>();
    mCamera->SetViewport(0, 0, width, height, 0.0f, 1.0f);
    mCamera->SetScissorRect(0, 0, width, height);
    mCamera->GenerateProjectionMatrix(1.0f, 500.0f, float(width) / float(height), 90.0f);
    mCamera->GenerateViewMatrix(Vector3(0.0f, 15.0f, -25.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3::Up);
}

void CRenderer::Resize(UINT width, UINT height)
{
    if (!mCamera || width == 0 || height == 0) return;

    mD3DCore.Resize(width, height);

    mCamera->SetViewport(0, 0, width, height, 0.0f, 1.0f);
    mCamera->SetScissorRect(0, 0, width, height);
    mCamera->GenerateProjectionMatrix(1.0f, 500.0f, float(width) / float(height), 90.0f);
}

void CRenderer::BeginSceneLoad()
{
    mD3DCore.WaitForGpuComplete();
    mD3DCore.ResetCommandList();
}

void CRenderer::EndSceneLoad()
{
    mD3DCore.ExecuteCommandList();
    mD3DCore.WaitForGpuComplete();
}

void CRenderer::Render(CScene* scene)
{
    float clearColor[4] = { 0.f, 0.f, 0.f, 1.f };

    mD3DCore.ResetCommandList();

    mD3DCore.BeginRender(clearColor);
    if (scene) scene->Render(mD3DCore.GetCommandList(), mCamera.get());
    mD3DCore.EndRender();

    mD3DCore.ExecuteCommandList();
    mD3DCore.Present(0, 0);
    mD3DCore.MoveToNextFrame();
}

void CRenderer::ChangeSwapChainState()
{
    mD3DCore.ChangeSwapChainState();
}

void CRenderer::WaitForGpuComplete()
{
    mD3DCore.WaitForGpuComplete();
}