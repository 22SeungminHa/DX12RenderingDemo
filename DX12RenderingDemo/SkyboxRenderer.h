#pragma once
#include "EngineTypes.h"
#include "DescriptorAllocator.h"

class Camera;
class Texture;
class AssetManager;
class SkyboxShader;
class SkyboxMesh;

class SkyboxRenderer
{
public:
    void Initialize(ID3D12Device* device, ID3D12RootSignature* rootSignature, DescriptorAllocator* srvAllocator);
    void Shutdown();

    bool Prepare(const SkyboxDesc& skybox, AssetManager& assetManager, ID3D12GraphicsCommandList* uploadCmdList);
    void Render(ID3D12GraphicsCommandList* cmdList, Camera* camera, const SkyboxDesc& skybox);

    void BindSkyboxTexture(ID3D12GraphicsCommandList* cmdList);

    void ReleaseUploadResources();

private:
    bool CreateSkyboxSrvDescriptor(Texture* texture);

private:
    ID3D12Device* device_ = nullptr;
    DescriptorAllocator* srvAllocator_ = nullptr;
    DescriptorAllocation srv_;

    std::unique_ptr<SkyboxShader> shader_;
    std::unique_ptr<SkyboxMesh> mesh_;

    std::shared_ptr<Texture> texture_;

    std::filesystem::path loadedPath_;
};