#pragma once

#include "GameObject.h"
#include "Camera.h"
#include "UploadBuffer.h"

// 게임 객체의 정보를 셰이더에게 넘겨주기 위한 구조체(상수 버퍼)이다.
struct ObjectCB
{
    Matrix world;
};

class Shader
{
public:
    Shader();
    virtual ~Shader();

    virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
    virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
    virtual D3D12_BLEND_DESC CreateBlendState();
    virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
    virtual D3D12_SHADER_BYTECODE CreateVertexShader(ComPtr<ID3DBlob>& pd3dShaderBlob);
    virtual D3D12_SHADER_BYTECODE CreatePixelShader(ComPtr<ID3DBlob>& pd3dShaderBlob);
    D3D12_SHADER_BYTECODE CompileShaderFromFile(
        const WCHAR* pszFileName,
        LPCSTR pszShaderName,
        LPCSTR pszShaderProfile,
        ComPtr<ID3DBlob>& pd3dShaderBlob);
    virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature);
    virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
    virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
    virtual void ReleaseShaderVariables();

    virtual void UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, const Matrix& world);

    virtual void OnPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList);
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, Camera* pCamera);

protected:
    std::vector<ComPtr<ID3D12PipelineState>> pipelineStates_;
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs_;
    std::unique_ptr<UploadBuffer<ObjectCB>> objectCB_;
};

class DiffusedShader : public Shader
{
public:
    DiffusedShader();
    virtual ~DiffusedShader();

    virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
    virtual D3D12_SHADER_BYTECODE CreateVertexShader(ComPtr<ID3DBlob>& pd3dShaderBlob);
    virtual D3D12_SHADER_BYTECODE CreatePixelShader(ComPtr<ID3DBlob>& pd3dShaderBlob);
    virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature);
};