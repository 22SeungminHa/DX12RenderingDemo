#include "Shader.h"

DXGI_FORMAT PostProcessShader::CreateRtvFormat() const
{
	return DXGI_FORMAT_R8G8B8A8_UNORM;
}

D3D12_INPUT_LAYOUT_DESC PostProcessShader::CreateInputLayout()
{
	D3D12_INPUT_LAYOUT_DESC desc{};
	desc.pInputElementDescs = nullptr;
	desc.NumElements = 0;
	return desc;
}

D3D12_SHADER_BYTECODE PostProcessShader::CreateVertexShader(ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(
		AssetPath::ShaderFile(L"PostProcess"),
		"VSFullscreen",
		"vs_5_1",
		shaderBlob
	);
}

D3D12_SHADER_BYTECODE PostProcessShader::CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(
		AssetPath::ShaderFile(L"PostProcess"),
		"PSCopy",
		"ps_5_1",
		shaderBlob
	);
}

D3D12_DEPTH_STENCIL_DESC PostProcessShader::CreateDepthStencilState(RenderMode renderMode)
{
	D3D12_DEPTH_STENCIL_DESC desc{};
	desc.DepthEnable = FALSE;
	desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	desc.StencilEnable = FALSE;
	return desc;
}

void PostProcessShader::CreatePipelineStates(
	ID3D12Device* device,
	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
	desc.BlendState = CreateBlendState(RenderMode::Opaque);
	desc.DepthStencilState = CreateDepthStencilState(RenderMode::Opaque);
	desc.DSVFormat = DXGI_FORMAT_UNKNOWN;

	ComPtr<ID3D12PipelineState> pso;
	ThrowIfFailed(device->CreateGraphicsPipelineState(
		&desc,
		IID_PPV_ARGS(pso.GetAddressOf())));

	pipelineStates_.push_back(std::move(pso));
}

D3D12_RASTERIZER_DESC PostProcessShader::CreateRasterizerState()
{
	D3D12_RASTERIZER_DESC desc = Shader::CreateRasterizerState();

	desc.CullMode = D3D12_CULL_MODE_NONE;

	return desc;
}

DXGI_FORMAT BrightPassShader::CreateRtvFormat() const
{
	return DXGI_FORMAT_R16G16B16A16_FLOAT;
}

D3D12_SHADER_BYTECODE BrightPassShader::CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(
		AssetPath::ShaderFile(L"PostProcess"),
		"PSBrightPass",
		"ps_5_1",
		shaderBlob
	);
}

DXGI_FORMAT HorizontalBlurShader::CreateRtvFormat() const
{
	return DXGI_FORMAT_R16G16B16A16_FLOAT;
}

D3D12_SHADER_BYTECODE HorizontalBlurShader::CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(
		AssetPath::ShaderFile(L"PostProcess"),
		"PSBlurHorizontal",
		"ps_5_1",
		shaderBlob
	);
}

DXGI_FORMAT VerticalBlurShader::CreateRtvFormat() const
{
	return DXGI_FORMAT_R16G16B16A16_FLOAT;
}

D3D12_SHADER_BYTECODE VerticalBlurShader::CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(
		AssetPath::ShaderFile(L"PostProcess"),
		"PSBlurVertical",
		"ps_5_1",
		shaderBlob
	);
}

DXGI_FORMAT GlassCompositeShader::CreateRtvFormat() const
{
	return DXGI_FORMAT_R16G16B16A16_FLOAT;
}

D3D12_SHADER_BYTECODE GlassCompositeShader::CreatePixelShader(
	ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(
		AssetPath::ShaderFile(L"PostProcess"),
		"PSGlassComposite",
		"ps_5_1",
		shaderBlob
	);
}