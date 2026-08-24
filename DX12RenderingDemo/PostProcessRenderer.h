#pragma once
#include "RenderTexture.h"

class Camera;
class PostProcessShader;
class BrightPassShader;
class HorizontalBlurShader;
class VerticalBlurShader;
class DescriptorAllocator;
class GlassCompositeShader;

class PostProcessRenderer
{
public:
    static constexpr DXGI_FORMAT kSceneColorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    static constexpr DXGI_FORMAT kGlassAccumFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    static constexpr DXGI_FORMAT kGlassRevealageFormat = DXGI_FORMAT_R16_FLOAT;

public:
    PostProcessRenderer() = default;
    ~PostProcessRenderer() = default;

    void Initialize(
        ID3D12Device* device,
        ID3D12RootSignature* rootSignature,
        DescriptorAllocator* srvAllocator,
        UINT width,
        UINT height);

    void Shutdown();
    void Resize(UINT width, UINT height);

    void BeginSceneRender(
        ID3D12GraphicsCommandList* cmdList,
        Camera* camera,
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle);

    void EndSceneRender(ID3D12GraphicsCommandList* cmdList);

    void Render(
        ID3D12GraphicsCommandList* cmdList,
        Camera* camera,
        ID3D12RootSignature* rootSignature,
        D3D12_CPU_DESCRIPTOR_HANDLE finalRtv);

    D3D12_GPU_DESCRIPTOR_HANDLE GetRefractionSceneSrv() const { return refractionSceneColor_.GetSrv(); }
    void CaptureRefractionScene(ID3D12GraphicsCommandList* cmdList);

    void BeginGlassAccumulation(ID3D12GraphicsCommandList* cmdList, Camera* camera, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle);
    void EndGlassAccumulation(ID3D12GraphicsCommandList* cmdList);

    D3D12_GPU_DESCRIPTOR_HANDLE GetGlassAccumSrv() const { return glassAccumColor_.GetSrv(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGlassRevealageSrv() const { return glassRevealage_.GetSrv(); }

    void CompositeGlassAccumulation(
        ID3D12GraphicsCommandList* cmdList,
        Camera* camera,
        ID3D12RootSignature* rootSignature);

private:
    void CreateRenderTextures(UINT width, UINT height);

    void RenderBrightPass(
        ID3D12GraphicsCommandList* cmdList,
        Camera* camera,
        ID3D12RootSignature* rootSignature,
        ID3D12DescriptorHeap* srvDescriptorHeap);

    void RenderHorizontalBlur(
        ID3D12GraphicsCommandList* cmdList,
        Camera* camera,
        ID3D12RootSignature* rootSignature,
        ID3D12DescriptorHeap* srvDescriptorHeap);

    void RenderVerticalBlur(
        ID3D12GraphicsCommandList* cmdList,
        Camera* camera,
        ID3D12RootSignature* rootSignature,
        ID3D12DescriptorHeap* srvDescriptorHeap);

    void RenderFinalComposite(
        ID3D12GraphicsCommandList* cmdList,
        Camera* camera,
        ID3D12RootSignature* rootSignature,
        ID3D12DescriptorHeap* srvDescriptorHeap,
        D3D12_CPU_DESCRIPTOR_HANDLE finalRtv);

private:
    ID3D12Device* device_ = nullptr;
    DescriptorAllocator* srvAllocator_ = nullptr;

    DescriptorAllocation sceneColorSrv_;
    DescriptorAllocation brightColorSrv_;
    DescriptorAllocation blurTempSrv_;
    DescriptorAllocation refractionSceneColorSrv_;
    DescriptorAllocation glassAccumColorSrv_;
    DescriptorAllocation glassRevealageSrv_;

    RenderTexture sceneColor_;
    RenderTexture brightColor_;
    RenderTexture blurTemp_;
    RenderTexture refractionSceneColor_;
    RenderTexture glassAccumColor_;
    RenderTexture glassRevealage_;

    std::unique_ptr<PostProcessShader> postProcessShader_;
    std::unique_ptr<BrightPassShader> brightPassShader_;
    std::unique_ptr<HorizontalBlurShader> horizontalBlurShader_;
    std::unique_ptr<VerticalBlurShader> verticalBlurShader_;
    std::unique_ptr<GlassCompositeShader> glassCompositeShader_;
};