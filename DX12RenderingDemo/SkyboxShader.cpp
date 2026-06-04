#include "Shader.h"

D3D12_INPUT_LAYOUT_DESC SkyboxShader::CreateInputLayout()
{
	inputElementDescs_ =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_INPUT_LAYOUT_DESC desc{};
	desc.pInputElementDescs = inputElementDescs_.data();
	desc.NumElements = static_cast<UINT>(inputElementDescs_.size());

	return desc;
}

D3D12_SHADER_BYTECODE SkyboxShader::CreateVertexShader(ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(AssetPath::ShaderFile(L"Skybox"), "VSSkybox", "vs_5_1", shaderBlob);
}

D3D12_SHADER_BYTECODE SkyboxShader::CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(AssetPath::ShaderFile(L"Skybox"), "PSSkybox", "ps_5_1", shaderBlob);
}

D3D12_RASTERIZER_DESC SkyboxShader::CreateRasterizerState()
{
	D3D12_RASTERIZER_DESC desc = Shader::CreateRasterizerState();
	desc.CullMode = D3D12_CULL_MODE_NONE;
	return desc;
}

D3D12_DEPTH_STENCIL_DESC SkyboxShader::CreateDepthStencilState(RenderMode renderMode)
{
	D3D12_DEPTH_STENCIL_DESC desc = Shader::CreateDepthStencilState(renderMode);
	desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	return desc;
}

void SkyboxShader::CreatePipelineStates(
	ID3D12Device* device,
	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
	desc.BlendState = CreateBlendState(RenderMode::Opaque);
	desc.DepthStencilState = CreateDepthStencilState(RenderMode::Opaque);

	ComPtr<ID3D12PipelineState> pso;
	ThrowIfFailed(device->CreateGraphicsPipelineState(
		&desc,
		IID_PPV_ARGS(pso.GetAddressOf())));

	pipelineStates_.push_back(std::move(pso));
}
