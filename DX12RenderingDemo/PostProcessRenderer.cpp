#include "PostProcessRenderer.h"
#include "Shader.h"
#include "Camera.h"

void PostProcessRenderer::Initialize(
    ID3D12Device* device,
    ID3D12RootSignature* rootSignature,
    ID3D12DescriptorHeap* srvDescriptorHeap,
    UINT srvDescriptorSize,
    UINT& nextSrvDescriptorIndex,
    UINT width,
    UINT height)
{
    device_ = device;
    srvDescriptorHeap_ = srvDescriptorHeap;
    srvDescriptorSize_ = srvDescriptorSize;

    sceneColorSrvDescriptorIndex_ = nextSrvDescriptorIndex++;
    brightColorSrvDescriptorIndex_ = nextSrvDescriptorIndex++;
    blurTempSrvDescriptorIndex_ = nextSrvDescriptorIndex++;

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

    sceneColorSrvDescriptorIndex_ = UINT_MAX;
    brightColorSrvDescriptorIndex_ = UINT_MAX;
    blurTempSrvDescriptorIndex_ = UINT_MAX;

    device_ = nullptr;
    srvDescriptorHeap_ = nullptr;
    srvDescriptorSize_ = 0;
}

void PostProcessRenderer::Resize(UINT width, UINT height)
{
    CreateRenderTextures(width, height);
}

void PostProcessRenderer::CreateRenderTextures(UINT width, UINT height)
{
    if (!device_ || !srvDescriptorHeap_ || width == 0 || height == 0)
        return;

    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    sceneColor_.Create(
        device_,
        width,
        height,
        kSceneColorFormat,
        srvDescriptorHeap_,
        srvDescriptorSize_,
        sceneColorSrvDescriptorIndex_,
        clearColor);

    brightColor_.Create(
        device_,
        width,
        height,
        kSceneColorFormat,
        srvDescriptorHeap_,
        srvDescriptorSize_,
        brightColorSrvDescriptorIndex_,
        clearColor);

    blurTemp_.Create(
        device_,
        width,
        height,
        kSceneColorFormat,
        srvDescriptorHeap_,
        srvDescriptorSize_,
        blurTempSrvDescriptorIndex_,
        clearColor);
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
    ID3D12DescriptorHeap* srvDescriptorHeap,
    D3D12_CPU_DESCRIPTOR_HANDLE finalRtv)
{
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

    cmdList->SetGraphicsRootDescriptorTable(3, sceneColor_.GetSrv());

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

    cmdList->SetGraphicsRootDescriptorTable(3, brightColor_.GetSrv());

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

    cmdList->SetGraphicsRootDescriptorTable(3, blurTemp_.GetSrv());

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

    cmdList->SetGraphicsRootDescriptorTable(3, sceneColor_.GetSrv());
    cmdList->SetGraphicsRootDescriptorTable(5, brightColor_.GetSrv());

    postProcessShader_->Render(cmdList, camera, RenderMode::Opaque);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);
}