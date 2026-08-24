#pragma once
#include "RenderTexture.h"

class Camera;
class DescriptorAllocator;
class GlassCompositeShader;

class GlassRenderer
{
public:
    static constexpr DXGI_FORMAT kRefractionSceneFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    static constexpr DXGI_FORMAT kGlassAccumFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    static constexpr DXGI_FORMAT kGlassRevealageFormat = DXGI_FORMAT_R16_FLOAT;

public:
    GlassRenderer() = default;
    ~GlassRenderer() = default;

    void Initialize(
        ID3D12Device* device,
        ID3D12RootSignature* rootSignature,
        DescriptorAllocator* srvAllocator,
        RtvDescriptorAllocator* rtvAllocator,
        UINT width,
        UINT height);

    void Shutdown();
    void Resize(UINT width, UINT height);

    void CaptureRefractionScene(ID3D12GraphicsCommandList* cmdList, RenderTexture& sceneColor);

    D3D12_GPU_DESCRIPTOR_HANDLE GetRefractionSceneSrv() const { return refractionSceneColor_.GetSrv(); }

    void BeginAccumulation(ID3D12GraphicsCommandList* cmdList, Camera* camera, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle);
    void EndAccumulation(ID3D12GraphicsCommandList* cmdList);
    void CompositeAccumulation(ID3D12GraphicsCommandList* cmdList, Camera* camera, ID3D12RootSignature* rootSignature, RenderTexture& sceneColor);

private:
    void CreateRenderTextures(UINT width, UINT height);

private:
    ID3D12Device* device_ = nullptr;
    DescriptorAllocator* srvAllocator_ = nullptr;
    RtvDescriptorAllocator* rtvAllocator_ = nullptr;

    DescriptorAllocation refractionSceneColorSrv_;
    DescriptorAllocation glassAccumColorSrv_;
    DescriptorAllocation glassRevealageSrv_;

    RtvDescriptorAllocation refractionSceneColorRtv_;
    RtvDescriptorAllocation glassAccumColorRtv_;
    RtvDescriptorAllocation glassRevealageRtv_;

    RenderTexture refractionSceneColor_;
    RenderTexture glassAccumColor_;
    RenderTexture glassRevealage_;

    std::unique_ptr<GlassCompositeShader> glassCompositeShader_;
};