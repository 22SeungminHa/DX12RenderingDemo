#include "PostProcessRenderer.h"
#include "Shader.h"
#include "Camera.h"
#include "DescriptorAllocator.h"

void PostProcessRenderer::Initialize(
    ID3D12Device* device,
    ID3D12RootSignature* rootSignature,
    DescriptorAllocator* srvAllocator,
    UINT width,
    UINT height)
{
    device_ = device;
    srvAllocator_ = srvAllocator;

    sceneColorSrv_ = srvAllocator_->Allocate();
    brightColorSrv_ = srvAllocator_->Allocate();
    blurTempSrv_ = srvAllocator_->Allocate();
    refractionSceneColorSrv_ = srvAllocator_->Allocate();
    glassAccumColorSrv_ = srvAllocator_->Allocate();
    glassRevealageSrv_ = srvAllocator_->Allocate();

    CreateRenderTextures(width, height);

    postProcessShader_ = std::make_unique<PostProcessShader>();
    postProcessShader_->CreateShader(device_, rootSignature);

    brightPassShader_ = std::make_unique<BrightPassShader>();
    brightPassShader_->CreateShader(device_, rootSignature);

    horizontalBlurShader_ = std::make_unique<HorizontalBlurShader>();
    horizontalBlurShader_->CreateShader(device_, rootSignature);

    verticalBlurShader_ = std::make_unique<VerticalBlurShader>();
    verticalBlurShader_->CreateShader(device_, rootSignature);
}

void PostProcessRenderer::Shutdown()
{
    postProcessShader_.reset();
    brightPassShader_.reset();
    horizontalBlurShader_.reset();
    verticalBlurShader_.reset();

    sceneColor_.Release();
    brightColor_.Release();
    blurTemp_.Release();
    refractionSceneColor_.Release();
    glassAccumColor_.Release();
    glassRevealage_.Release();

    sceneColorSrv_ = {};
    brightColorSrv_ = {};
    blurTempSrv_ = {};
    refractionSceneColorSrv_ = {};
    glassAccumColorSrv_ = {};
    glassRevealageSrv_ = {};

    device_ = nullptr;
    srvAllocator_ = nullptr;
}

void PostProcessRenderer::Resize(UINT width, UINT height)
{
    CreateRenderTextures(width, height);
}

void PostProcessRenderer::CreateRenderTextures(UINT width, UINT height)
{
    if (!device_ || !srvAllocator_ || width == 0 || height == 0)
        return;

    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const float accumClear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float revealageClear[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    sceneColor_.Create(device_, width, height, kSceneColorFormat, sceneColorSrv_, clearColor);
    brightColor_.Create(device_, width, height, kSceneColorFormat, brightColorSrv_, clearColor);
    blurTemp_.Create(device_, width, height, kSceneColorFormat, blurTempSrv_, clearColor);
    refractionSceneColor_.Create(device_, width, height, kSceneColorFormat, refractionSceneColorSrv_, clearColor);
    glassAccumColor_.Create(device_, width, height, kGlassAccumFormat, glassAccumColorSrv_, accumClear);
    glassRevealage_.Create(device_, width, height, kGlassRevealageFormat, glassRevealageSrv_, revealageClear);
}

void PostProcessRenderer::BeginSceneRender(
    ID3D12GraphicsCommandList* cmdList,
    Camera* camera,
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle)
{
    if (!cmdList || !camera || !sceneColor_.GetResource())
        return;

    sceneColor_.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    const D3D12_CPU_DESCRIPTOR_HANDLE rtv = sceneColor_.GetRtv();
    cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsvHandle);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    cmdList->ClearDepthStencilView(
        dsvHandle,
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f,
        0,
        0,
        nullptr);
}

void PostProcessRenderer::EndSceneRender(ID3D12GraphicsCommandList* cmdList)
{
    sceneColor_.Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void PostProcessRenderer::Render(
    ID3D12GraphicsCommandList* cmdList,
    Camera* camera,
    ID3D12RootSignature* rootSignature,
    D3D12_CPU_DESCRIPTOR_HANDLE finalRtv) 
{
    ID3D12DescriptorHeap* srvDescriptorHeap = srvAllocator_ ? srvAllocator_->GetHeap() : nullptr;

    RenderBrightPass(cmdList, camera, rootSignature, srvDescriptorHeap);
    RenderHorizontalBlur(cmdList, camera, rootSignature, srvDescriptorHeap);
    RenderVerticalBlur(cmdList, camera, rootSignature, srvDescriptorHeap);
    RenderFinalComposite(cmdList, camera, rootSignature, srvDescriptorHeap, finalRtv);
}

void PostProcessRenderer::RenderBrightPass(
    ID3D12GraphicsCommandList* cmdList,
    Camera* camera,
    ID3D12RootSignature* rootSignature,
    ID3D12DescriptorHeap* srvDescriptorHeap)
{
    if (!cmdList || !camera || !brightPassShader_ || !sceneColor_.GetResource() || !brightColor_.GetResource())
        return;

    brightColor_.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    const D3D12_CPU_DESCRIPTOR_HANDLE rtv = brightColor_.GetRtv();
    cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    cmdList->SetGraphicsRootSignature(rootSignature);

    ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootDescriptorTable(
        static_cast<UINT>(RootParam::MaterialTextures),
        sceneColor_.GetSrv());

    brightPassShader_->Render(cmdList, camera, RenderMode::Opaque);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    brightColor_.Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void PostProcessRenderer::RenderHorizontalBlur(
    ID3D12GraphicsCommandList* cmdList,
    Camera* camera,
    ID3D12RootSignature* rootSignature,
    ID3D12DescriptorHeap* srvDescriptorHeap)
{
    if (!cmdList || !camera || !horizontalBlurShader_ || !brightColor_.GetResource() || !blurTemp_.GetResource())
        return;

    blurTemp_.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    const D3D12_CPU_DESCRIPTOR_HANDLE rtv = blurTemp_.GetRtv();
    cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    cmdList->SetGraphicsRootSignature(rootSignature);

    ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootDescriptorTable(
        static_cast<UINT>(RootParam::MaterialTextures),
        brightColor_.GetSrv());

    horizontalBlurShader_->Render(cmdList, camera, RenderMode::Opaque);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    blurTemp_.Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void PostProcessRenderer::RenderVerticalBlur(
    ID3D12GraphicsCommandList* cmdList,
    Camera* camera,
    ID3D12RootSignature* rootSignature,
    ID3D12DescriptorHeap* srvDescriptorHeap)
{
    if (!cmdList || !camera || !verticalBlurShader_ || !blurTemp_.GetResource() || !brightColor_.GetResource())
        return;

    brightColor_.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    const D3D12_CPU_DESCRIPTOR_HANDLE rtv = brightColor_.GetRtv();
    cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    cmdList->SetGraphicsRootSignature(rootSignature);

    ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootDescriptorTable(
        static_cast<UINT>(RootParam::MaterialTextures),
        blurTemp_.GetSrv());

    verticalBlurShader_->Render(cmdList, camera, RenderMode::Opaque);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    brightColor_.Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void PostProcessRenderer::RenderFinalComposite(
    ID3D12GraphicsCommandList* cmdList,
    Camera* camera,
    ID3D12RootSignature* rootSignature,
    ID3D12DescriptorHeap* srvDescriptorHeap,
    D3D12_CPU_DESCRIPTOR_HANDLE finalRtv)
{
    if (!cmdList || !camera || !postProcessShader_ || !sceneColor_.GetResource())
        return;

    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    cmdList->OMSetRenderTargets(1, &finalRtv, FALSE, nullptr);

    cmdList->SetGraphicsRootSignature(rootSignature);

    ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootDescriptorTable(
        static_cast<UINT>(RootParam::MaterialTextures),
        sceneColor_.GetSrv());

    cmdList->SetGraphicsRootDescriptorTable(
        static_cast<UINT>(RootParam::PostProcessTexture),
        brightColor_.GetSrv());

    postProcessShader_->Render(cmdList, camera, RenderMode::Opaque);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);
}

void PostProcessRenderer::CaptureRefractionScene(ID3D12GraphicsCommandList* cmdList)
{
    if (!cmdList || !sceneColor_.GetResource() || !refractionSceneColor_.GetResource())
        return;

    sceneColor_.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
    refractionSceneColor_.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

    cmdList->CopyResource(
        refractionSceneColor_.GetResource(),
        sceneColor_.GetResource());

    refractionSceneColor_.Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    sceneColor_.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void PostProcessRenderer::BeginGlassAccumulation(ID3D12GraphicsCommandList* cmdList, Camera* camera, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle)
{
    if (!cmdList || !camera || !glassAccumColor_.GetResource() || !glassRevealage_.GetResource())
        return;

    glassAccumColor_.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    glassRevealage_.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const auto& viewport = camera->GetViewport();
    const auto& scissor = camera->GetScissorRect();

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2] = { glassAccumColor_.GetRtv(), glassRevealage_.GetRtv() };

    cmdList->OMSetRenderTargets(2, rtvs, FALSE, &dsvHandle);

    const float accumClear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float revealageClear[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    cmdList->ClearRenderTargetView(glassAccumColor_.GetRtv(), accumClear, 0, nullptr);
    cmdList->ClearRenderTargetView(glassRevealage_.GetRtv(), revealageClear, 0, nullptr);
}

void PostProcessRenderer::EndGlassAccumulation(ID3D12GraphicsCommandList* cmdList)
{
    if (!cmdList)
        return;

    glassAccumColor_.Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    glassRevealage_.Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}