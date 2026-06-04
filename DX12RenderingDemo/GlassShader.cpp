#include "Shader.h"

GlassShader::GlassShader()
{
}

GlassShader::~GlassShader()
{
}

D3D12_SHADER_BYTECODE GlassShader::CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(
		AssetPath::ShaderFile(L"Glass"),
		"PSGlass",
		"ps_5_1",
		shaderBlob
	);
}

D3D12_RASTERIZER_DESC GlassShader::CreateRasterizerState()
{
	D3D12_RASTERIZER_DESC desc = LitShader::CreateRasterizerState();

	// 얇은 유리판은 뒷면도 보여야 자연스러움
	//desc.CullMode = D3D12_CULL_MODE_NONE;

	return desc;
}

void GlassShader::CreateShader(ID3D12Device* device, ID3D12RootSignature* rootSignature)
{
	pipelineStates_.clear();
	pipelineStates_.reserve(1);

	Shader::CreateShader(device, rootSignature);
}

void GlassShader::CreatePipelineStates(
	ID3D12Device* device,
	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
	desc.BlendState = CreateBlendState(RenderMode::Transparent);
	desc.DepthStencilState = CreateDepthStencilState(RenderMode::Transparent);

	ComPtr<ID3D12PipelineState> transparentPso;
	ThrowIfFailed(device->CreateGraphicsPipelineState(
		&desc,
		IID_PPV_ARGS(transparentPso.GetAddressOf())));

	pipelineStates_.push_back(std::move(transparentPso));
}
