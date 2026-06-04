#pragma once
#include "EngineTypes.h"

class Camera;
class Texture;
class AssetManager;
class SkyboxShader;
class SkyboxMesh;

class SkyboxRenderer
{
public:
    void Initialize(
        ID3D12Device* device,
        ID3D12RootSignature* rootSignature,
        ID3D12DescriptorHeap* srvDescriptorHeap,
        UINT srvDescriptorSize,
        UINT& nextSrvDescriptorIndex);

    void Shutdown();

    bool Prepare(
        const SkyboxDesc& skybox,
        AssetManager& assetManager,
        ID3D12GraphicsCommandList* uploadCmdList);

    void Render(
        ID3D12GraphicsCommandList* cmdList,
        Camera* camera,
        const SkyboxDesc& skybox);

    void BindSkyboxTexture(ID3D12GraphicsCommandList* cmdList);

    void ReleaseUploadResources();

private:
    bool CreateSkyboxSrvDescriptor(Texture* texture);

private:
    ID3D12Device* device_ = nullptr;
    ID3D12DescriptorHeap* srvDescriptorHeap_ = nullptr;
    UINT srvDescriptorSize_ = 0;

    std::unique_ptr<SkyboxShader> shader_;
    std::unique_ptr<SkyboxMesh> mesh_;

    std::shared_ptr<Texture> texture_;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle_{};
    UINT descriptorIndex_ = UINT_MAX;

    std::filesystem::path loadedPath_;
};