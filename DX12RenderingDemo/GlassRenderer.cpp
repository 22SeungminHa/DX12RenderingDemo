#include "GlassRenderer.h"
#include "Shader.h"
#include "Camera.h"
#include "DescriptorAllocator.h"

void GlassRenderer::Initialize(
    ID3D12Device* device,
    ID3D12RootSignature* rootSignature,
    DescriptorAllocator* srvAllocator,
    RtvDescriptorAllocator* rtvAllocator,
    UINT width,
    UINT height)
{
    device_ = device;
    srvAllocator_ = srvAllocator;
    rtvAllocator_ = rtvAllocator;

    refractionSceneColorSrv_ = srvAllocator_->Allocate();
    glassAccumColorSrv_ = srvAllocator_->Allocate();
    glassRevealageSrv_ = srvAllocator_->Allocate();

    refractionSceneColorRtv_ = rtvAllocator_->Allocate();
    glassAccumColorRtv_ = rtvAllocator_->Allocate();
    glassRevealageRtv_ = rtvAllocator_->Allocate();

    CreateRenderTextures(width, height);

    glassCompositeShader_ = std::make_unique<GlassCompositeShader>();
    glassCompositeShader_->CreateShader(device_, rootSignature);
}

void GlassRenderer::Shutdown()
{
    glassCompositeShader_.reset();

    refractionSceneColor_.Release();
    glassAccumColor_.Release();
    glassRevealage_.Release();

    refractionSceneColorSrv_ = {};
    glassAccumColorSrv_ = {};
    glassRevealageSrv_ = {};

    refractionSceneColorRtv_ = {};
    glassAccumColorRtv_ = {};
    glassRevealageRtv_ = {};

    device_ = nullptr;
    srvAllocator_ = nullptr;
    rtvAllocator_ = nullptr;
}

void GlassRenderer::Resize(UINT width, UINT height)
{
    CreateRenderTextures(width, height);
}

void GlassRenderer::CreateRenderTextures(UINT width, UINT height)
{
    if (!device_ || !srvAllocator_ || !rtvAllocator_ || width == 0 || height == 0)
        return;

    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const float accumClear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float revealageClear[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    refractionSceneColor_.Create(device_, width, height, kRefractionSceneFormat, refractionSceneColorSrv_, refractionSceneColorRtv_, clearColor);
    glassAccumColor_.Create(device_, width, height, kGlassAccumFormat, glassAccumColorSrv_, glassAccumColorRtv_, accumClear);
    glassRevealage_.Create(device_, width, height, kGlassRevealageFormat, glassRevealageSrv_, glassRevealageRtv_, revealageClear);
}

void GlassRenderer::CaptureRefractionScene(ID3D12GraphicsCommandList* cmdList, RenderTexture& sceneColor)
{
    if (!cmdList || !sceneColor.GetResource() || !refractionSceneColor_.GetResource())
        return;

    sceneColor.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
    refractionSceneColor_.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

    cmdList->CopyResource(refractionSceneColor_.GetResource(), sceneColor.GetResource());
    refractionSceneColor_.Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    sceneColor.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void GlassRenderer::BeginAccumulation(ID3D12GraphicsCommandList* cmdList, Camera* camera, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle)
{
    if (!cmdList || !camera || !glassAccumColor_.GetResource() || !glassRevealage_.GetResource())
        return;

    glassAccumColor_.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    glassRevealage_.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] =
    {
        glassAccumColor_.GetRtv(),
        glassRevealage_.GetRtv()
    };

    cmdList->OMSetRenderTargets(_countof(rtvs), rtvs, FALSE, &dsvHandle);

    const float accumClear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float revealageClear[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    cmdList->ClearRenderTargetView(glassAccumColor_.GetRtv(), accumClear, 0, nullptr);
    cmdList->ClearRenderTargetView(glassRevealage_.GetRtv(), revealageClear, 0, nullptr);
}

void GlassRenderer::EndAccumulation(ID3D12GraphicsCommandList* cmdList)
{
    if (!cmdList)
        return;

    glassAccumColor_.Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    glassRevealage_.Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void GlassRenderer::CompositeAccumulation(
    ID3D12GraphicsCommandList* cmdList,
    Camera* camera,
    ID3D12RootSignature* rootSignature,
    RenderTexture& sceneColor)
{
    if (!cmdList ||
        !camera ||
        !rootSignature ||
        !srvAllocator_ ||
        !glassCompositeShader_ ||
        !sceneColor.GetResource() ||
        !refractionSceneColor_.GetResource() ||
        !glassAccumColor_.GetResource() ||
        !glassRevealage_.GetResource())
    {
        return;
    }

    sceneColor.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    const D3D12_CPU_DESCRIPTOR_HANDLE rtv = sceneColor.GetRtv();

    cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    cmdList->SetGraphicsRootSignature(rootSignature);

    ID3D12DescriptorHeap* descriptorHeaps[] = { srvAllocator_->GetHeap() };

    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootParam::SceneColorTexture), refractionSceneColor_.GetSrv());
    cmdList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootParam::GlassAccumTexture), glassAccumColor_.GetSrv());
    cmdList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootParam::GlassRevealageTexture), glassRevealage_.GetSrv());

    glassCompositeShader_->Render(cmdList, camera, RenderMode::Opaque);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->DrawInstanced(3, 1, 0, 0);
}