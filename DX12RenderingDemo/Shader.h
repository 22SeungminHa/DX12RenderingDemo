#pragma once
#include "Asset.h"
#include "EngineTypes.h"

class Camera;

class Shader : public Asset
{
public:
    Shader() = default;
    virtual ~Shader() = default;

    virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
    virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
    virtual D3D12_BLEND_DESC CreateBlendState(RenderMode renderMode);
    virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(RenderMode renderMode);

    virtual D3D12_SHADER_BYTECODE CreateVertexShader(ComPtr<ID3DBlob>& shaderBlob);
    virtual D3D12_SHADER_BYTECODE CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob);
    
    virtual DXGI_FORMAT CreateRtvFormat() const;

    D3D12_SHADER_BYTECODE CompileShaderFromFile(
        const std::filesystem::path& filePath,
        LPCSTR shaderName,
        LPCSTR shaderProfile,
        ComPtr<ID3DBlob>& shaderBlob);

    virtual void CreateShader(ID3D12Device* device, ID3D12RootSignature* rootSignature);
    virtual void CreatePipelineStates(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
    
    virtual void OnPrepareRender(ID3D12GraphicsCommandList* cmdList, RenderMode renderMode, RenderPass renderPass = RenderPass::Default);
    virtual void Render(ID3D12GraphicsCommandList* cmdList, Camera* camera, RenderMode renderMode, RenderPass renderPass = RenderPass::Default);

protected:
    std::vector<ComPtr<ID3D12PipelineState>> pipelineStates_;
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs_;
};

class LitShader : public Shader
{
public:
    LitShader();
    virtual ~LitShader();

    virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
    virtual D3D12_SHADER_BYTECODE CreateVertexShader(ComPtr<ID3DBlob>& shaderBlob);
    virtual D3D12_SHADER_BYTECODE CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob);
    virtual void CreateShader(ID3D12Device* device, ID3D12RootSignature* rootSignature);
};

class GlassShader : public LitShader
{
public:
    GlassShader();
    virtual ~GlassShader();

    virtual D3D12_SHADER_BYTECODE CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob) override;
    virtual D3D12_RASTERIZER_DESC CreateRasterizerState() override;

    virtual void CreateShader(ID3D12Device* device, ID3D12RootSignature* rootSignature) override;
    virtual void CreatePipelineStates(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
    
    virtual void OnPrepareRender(ID3D12GraphicsCommandList* cmdList, RenderMode renderMode, RenderPass renderPass = RenderPass::Default) override;

private:
    D3D12_SHADER_BYTECODE CreateAccumulationPixelShader(ComPtr<ID3DBlob>& shaderBlob);
    D3D12_BLEND_DESC CreateAccumulationBlendState();
};

class SkyboxShader : public Shader
{
public:
    SkyboxShader() = default;
    virtual ~SkyboxShader() = default;

    virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() override;
    virtual D3D12_SHADER_BYTECODE CreateVertexShader(ComPtr<ID3DBlob>& shaderBlob) override;
    virtual D3D12_SHADER_BYTECODE CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob) override;
    virtual D3D12_RASTERIZER_DESC CreateRasterizerState() override;
    virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(RenderMode renderMode) override;
    virtual void CreatePipelineStates(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) override;
};

class PostProcessShader : public Shader
{
public:
    PostProcessShader() = default;
    virtual ~PostProcessShader() = default;

    virtual DXGI_FORMAT CreateRtvFormat() const override;
    virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() override;
    virtual D3D12_SHADER_BYTECODE CreateVertexShader(ComPtr<ID3DBlob>& shaderBlob) override;
    virtual D3D12_SHADER_BYTECODE CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob) override;
    virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(RenderMode renderMode) override;
    virtual void CreatePipelineStates(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) override;
    virtual D3D12_RASTERIZER_DESC CreateRasterizerState() override;
};

class BrightPassShader : public PostProcessShader
{
public:
    BrightPassShader() = default;
    virtual ~BrightPassShader() = default;

    virtual DXGI_FORMAT CreateRtvFormat() const override;
    virtual D3D12_SHADER_BYTECODE CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob) override;
};

class HorizontalBlurShader : public PostProcessShader
{
public:
    HorizontalBlurShader() = default;
    virtual ~HorizontalBlurShader() = default;

    virtual DXGI_FORMAT CreateRtvFormat() const override;
    virtual D3D12_SHADER_BYTECODE CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob) override;
};

class VerticalBlurShader : public PostProcessShader
{
public:
    VerticalBlurShader() = default;
    virtual ~VerticalBlurShader() = default;

    virtual DXGI_FORMAT CreateRtvFormat() const override;
    virtual D3D12_SHADER_BYTECODE CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob) override;
};

class GlassCompositeShader : public PostProcessShader
{
public:
    GlassCompositeShader() = default;
    virtual ~GlassCompositeShader() = default;

    virtual DXGI_FORMAT CreateRtvFormat() const override;
    virtual D3D12_SHADER_BYTECODE CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob) override;
};