#include "SkyboxRenderer.h"
#include "Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include "AssetManager.h"
#include "Camera.h"

void SkyboxRenderer::Initialize(
    ID3D12Device* device,
    ID3D12RootSignature* rootSignature,
    ID3D12DescriptorHeap* srvDescriptorHeap,
    UINT srvDescriptorSize,
    UINT& nextSrvDescriptorIndex)
{
    device_ = device;
    srvDescriptorHeap_ = srvDescriptorHeap;
    srvDescriptorSize_ = srvDescriptorSize;

    descriptorIndex_ = nextSrvDescriptorIndex++;

    shader_ = std::make_unique<SkyboxShader>();
    shader_->CreateShader(device_, rootSignature);
}

void SkyboxRenderer::Shutdown()
{
    mesh_.reset();
    shader_.reset();
    texture_.reset();

    loadedPath_.clear();
    gpuHandle_ = {};
    descriptorIndex_ = UINT_MAX;

    device_ = nullptr;
    srvDescriptorHeap_ = nullptr;
    srvDescriptorSize_ = 0;
}

bool SkyboxRenderer::Prepare(
    const SkyboxDesc& skybox,
    AssetManager& assetManager,
    ID3D12GraphicsCommandList* uploadCmdList)
{
    if (!skybox.enabled || skybox.cubemapPath.empty())
        return false;

    if (loadedPath_ == skybox.cubemapPath && texture_ && mesh_)
        return true;

    if (!device_ || !uploadCmdList)
        return false;

    texture_ = assetManager.LoadTexture(device_, uploadCmdList, skybox.cubemapPath);

    if (!mesh_)
        mesh_ = std::make_unique<SkyboxMesh>(device_, uploadCmdList);

    if (!CreateSkyboxSrvDescriptor(texture_.get()))
        return false;

    loadedPath_ = skybox.cubemapPath;

    return true;
}

bool SkyboxRenderer::CreateSkyboxSrvDescriptor(Texture* texture)
{
    if (!texture || !texture->GetResource() || !srvDescriptorHeap_)
        return false;

    D3D12_CPU_DESCRIPTOR_HANDLE dstHandle =
        srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    dstHandle.ptr += static_cast<SIZE_T>(descriptorIndex_) * srvDescriptorSize_;

    const D3D12_RESOURCE_DESC resourceDesc = texture->GetResource()->GetDesc();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = resourceDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels = resourceDesc.MipLevels;
    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

    device_->CreateShaderResourceView(
        texture->GetResource(),
        &srvDesc,
        dstHandle);

    gpuHandle_ = srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();
    gpuHandle_.ptr += static_cast<SIZE_T>(descriptorIndex_) * srvDescriptorSize_;

    return true;
}

void SkyboxRenderer::Render(
    ID3D12GraphicsCommandList* cmdList,
    Camera* camera,
    const SkyboxDesc& skybox)
{
    if (!cmdList || !camera)
        return;

    if (!skybox.enabled)
        return;

    if (loadedPath_ != skybox.cubemapPath)
        return;

    if (!shader_ || !mesh_ || !texture_)
        return;

    BindSkyboxTexture(cmdList);

    shader_->Render(cmdList, camera, RenderMode::Opaque);
    mesh_->Render(cmdList);
}

void SkyboxRenderer::BindSkyboxTexture(ID3D12GraphicsCommandList* cmdList)
{
    if (!cmdList || !srvDescriptorHeap_ || descriptorIndex_ == UINT_MAX)
        return;

    ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap_ };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootDescriptorTable(4, gpuHandle_);
}

void SkyboxRenderer::ReleaseUploadResources()
{
    if (mesh_)
        mesh_->ReleaseUploadResources();
}