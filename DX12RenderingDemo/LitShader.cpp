#include "Shader.h"

LitShader::LitShader()
{
}

LitShader::~LitShader()
{
}

D3D12_INPUT_LAYOUT_DESC LitShader::CreateInputLayout()
{
	inputElementDescs_ = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 52, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_INPUT_LAYOUT_DESC desc{};
	desc.pInputElementDescs = inputElementDescs_.data();
	desc.NumElements = static_cast<UINT>(inputElementDescs_.size());

	return desc;
}

D3D12_SHADER_BYTECODE LitShader::CreateVertexShader(ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(AssetPath::ShaderFile(L"Lit"), "VSLit", "vs_5_1", shaderBlob);
}

D3D12_SHADER_BYTECODE LitShader::CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(AssetPath::ShaderFile(L"Lit"), "PSLit", "ps_5_1", shaderBlob);
}

void LitShader::CreateShader(ID3D12Device* device, ID3D12RootSignature* rootSignature)
{
	pipelineStates_.clear();
	pipelineStates_.reserve(2);
	Shader::CreateShader(device, rootSignature);
}
