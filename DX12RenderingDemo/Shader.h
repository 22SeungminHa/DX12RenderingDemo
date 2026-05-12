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

    virtual D3D12_SHADER_BYTECODE CreateVertexShader(ComPtr<ID3DBlob>& pd3dShaderBlob);
    virtual D3D12_SHADER_BYTECODE CreatePixelShader(ComPtr<ID3DBlob>& pd3dShaderBlob);
    
    D3D12_SHADER_BYTECODE CompileShaderFromFile(
        const WCHAR* pszFileName,
        LPCSTR pszShaderName,
        LPCSTR pszShaderProfile,
        ComPtr<ID3DBlob>& pd3dShaderBlob);

    virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature);
    
    virtual void OnPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList, RenderMode renderMode);
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, Camera* pCamera, RenderMode renderMode);

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
    virtual D3D12_SHADER_BYTECODE CreateVertexShader(ComPtr<ID3DBlob>& pd3dShaderBlob);
    virtual D3D12_SHADER_BYTECODE CreatePixelShader(ComPtr<ID3DBlob>& pd3dShaderBlob);
    virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature);
};