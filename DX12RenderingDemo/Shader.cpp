#include "Shader.h"

D3D12_RASTERIZER_DESC Shader::CreateRasterizerState()
{
	D3D12_RASTERIZER_DESC d3dRasterizerDesc{};
	d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID; // D3D12_FILL_MODE_SOLID, D3D12_FILL_MODE_WIREFRAME
	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;  // D3D12_CULL_MODE_BACK, D3D12_CULL_MODE_NONE, D3D12_CULL_MODE_FRONT.
	d3dRasterizerDesc.FrontCounterClockwise = TRUE;
	d3dRasterizerDesc.DepthBias = 0;
	d3dRasterizerDesc.DepthBiasClamp = 0.0f;
	d3dRasterizerDesc.SlopeScaledDepthBias = 0.0f;
	d3dRasterizerDesc.DepthClipEnable = TRUE;
	d3dRasterizerDesc.MultisampleEnable = FALSE;
	d3dRasterizerDesc.AntialiasedLineEnable = FALSE;
	d3dRasterizerDesc.ForcedSampleCount = 0;
	d3dRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return d3dRasterizerDesc;
}

D3D12_DEPTH_STENCIL_DESC Shader::CreateDepthStencilState(RenderMode renderMode)
{
	D3D12_DEPTH_STENCIL_DESC desc{};
	desc.DepthEnable = TRUE;
	desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	desc.StencilEnable = FALSE;

	if (renderMode == RenderMode::Transparent)
		desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	else
		desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

	return desc;
}

D3D12_BLEND_DESC Shader::CreateBlendState(RenderMode renderMode)
{
	D3D12_BLEND_DESC desc{};
	desc.AlphaToCoverageEnable = FALSE;
	desc.IndependentBlendEnable = FALSE;

	auto& rt = desc.RenderTarget[0];
	rt.LogicOpEnable = FALSE;
	rt.LogicOp = D3D12_LOGIC_OP_NOOP;
	rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	if (renderMode == RenderMode::Transparent)
	{
		rt.BlendEnable = TRUE;

		rt.SrcBlend = D3D12_BLEND_ONE;
		rt.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		rt.BlendOp = D3D12_BLEND_OP_ADD;

		rt.SrcBlendAlpha = D3D12_BLEND_ONE;
		rt.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	}
	else
	{
		rt.BlendEnable = FALSE;

		rt.SrcBlend = D3D12_BLEND_ONE;
		rt.DestBlend = D3D12_BLEND_ZERO;
		rt.BlendOp = D3D12_BLEND_OP_ADD;

		rt.SrcBlendAlpha = D3D12_BLEND_ONE;
		rt.DestBlendAlpha = D3D12_BLEND_ZERO;
		rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	}

	return desc;
}

//입력 조립기에게 정점 버퍼의 구조를 알려주기 위한 구조체를 반환한다. 
D3D12_INPUT_LAYOUT_DESC Shader::CreateInputLayout()
{
	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc{};
	d3dInputLayoutDesc.pInputElementDescs = nullptr;
	d3dInputLayoutDesc.NumElements = 0;
	return d3dInputLayoutDesc;
}

D3D12_SHADER_BYTECODE Shader::CreateVertexShader(ComPtr<ID3DBlob>& pd3dShaderBlob)
{
	pd3dShaderBlob.Reset();

	D3D12_SHADER_BYTECODE d3dShaderByteCode;
	d3dShaderByteCode.BytecodeLength = 0;
	d3dShaderByteCode.pShaderBytecode = NULL;
	return d3dShaderByteCode;
}
D3D12_SHADER_BYTECODE Shader::CreatePixelShader(ComPtr<ID3DBlob>& pd3dShaderBlob)
{
	pd3dShaderBlob.Reset();

	D3D12_SHADER_BYTECODE d3dShaderByteCode;
	d3dShaderByteCode.BytecodeLength = 0;
	d3dShaderByteCode.pShaderBytecode = NULL;
	return d3dShaderByteCode;
}

//셰이더 소스 코드를 컴파일하여 바이트 코드 구조체를 반환한다. 
D3D12_SHADER_BYTECODE Shader::CompileShaderFromFile(
	const WCHAR * pszFileName,
	LPCSTR pszShaderName,
	LPCSTR pszShaderProfile,
	ComPtr<ID3DBlob>&pd3dShaderBlob)
{
	UINT nCompileFlags = 0;
#if defined(_DEBUG)
	nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	
	pd3dShaderBlob.Reset();
	ComPtr<ID3DBlob> errors;

	HRESULT hr = D3DCompileFromFile(pszFileName, NULL, NULL,
		pszShaderName, pszShaderProfile,
		nCompileFlags, 0,
		pd3dShaderBlob.GetAddressOf(),
		errors.GetAddressOf());

	if (errors) OutputDebugStringA((char*)errors->GetBufferPointer());
	ThrowIfFailed(hr);
	
	D3D12_SHADER_BYTECODE d3dShaderByteCode{};
	d3dShaderByteCode.BytecodeLength = pd3dShaderBlob->GetBufferSize();
	d3dShaderByteCode.pShaderBytecode = pd3dShaderBlob->GetBufferPointer();

	return d3dShaderByteCode;
}

//그래픽스 파이프라인 상태 객체를 생성한다. 
void Shader::CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	ComPtr<ID3DBlob> pd3dVertexShaderBlob;
	ComPtr<ID3DBlob> pd3dPixelShaderBlob;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineStateDesc{};
	d3dPipelineStateDesc.pRootSignature = pd3dGraphicsRootSignature;
	d3dPipelineStateDesc.VS = CreateVertexShader(pd3dVertexShaderBlob);
	d3dPipelineStateDesc.PS = CreatePixelShader(pd3dPixelShaderBlob);
	d3dPipelineStateDesc.RasterizerState = CreateRasterizerState();
	d3dPipelineStateDesc.InputLayout = CreateInputLayout();
	d3dPipelineStateDesc.SampleMask = UINT_MAX;
	d3dPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	d3dPipelineStateDesc.NumRenderTargets = 1;
	d3dPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dPipelineStateDesc.SampleDesc.Count = 1;
	d3dPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	
	CreatePipelineStates(pd3dDevice, d3dPipelineStateDesc);
}

void Shader::CreatePipelineStates(
	ID3D12Device* device,
	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
	desc.BlendState = CreateBlendState(RenderMode::Opaque);
	desc.DepthStencilState = CreateDepthStencilState(RenderMode::Opaque);

	ComPtr<ID3D12PipelineState> opaquePso;
	ThrowIfFailed(device->CreateGraphicsPipelineState(
		&desc,
		IID_PPV_ARGS(opaquePso.GetAddressOf())));

	pipelineStates_.push_back(std::move(opaquePso));

	desc.BlendState = CreateBlendState(RenderMode::Transparent);
	desc.DepthStencilState = CreateDepthStencilState(RenderMode::Transparent);

	ComPtr<ID3D12PipelineState> transparentPso;
	ThrowIfFailed(device->CreateGraphicsPipelineState(
		&desc,
		IID_PPV_ARGS(transparentPso.GetAddressOf())));

	pipelineStates_.push_back(std::move(transparentPso));
}

void Shader::OnPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList, RenderMode renderMode)
{
	if (!pd3dCommandList || pipelineStates_.empty())
		return;

	UINT psoIndex = 0;

	if (pipelineStates_.size() >= 2)
		psoIndex = (renderMode == RenderMode::Transparent) ? 1 : 0;

	pd3dCommandList->SetPipelineState(pipelineStates_[psoIndex].Get());
}

void Shader::Render(ID3D12GraphicsCommandList* pd3dCommandList, Camera* pCamera, RenderMode renderMode)
{
	OnPrepareRender(pd3dCommandList, renderMode);
}

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

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc{};
	d3dInputLayoutDesc.pInputElementDescs = inputElementDescs_.data();
	d3dInputLayoutDesc.NumElements = static_cast<UINT>(inputElementDescs_.size());

	return d3dInputLayoutDesc;
}

D3D12_SHADER_BYTECODE LitShader::CreateVertexShader(ComPtr<ID3DBlob>& pd3dShaderBlob)
{
	return Shader::CompileShaderFromFile(L"Shaders.hlsl", "VSLit", "vs_5_1", pd3dShaderBlob);
}

D3D12_SHADER_BYTECODE LitShader::CreatePixelShader(ComPtr<ID3DBlob>& pd3dShaderBlob)
{
	return Shader::CompileShaderFromFile(L"Shaders.hlsl", "PSLit", "ps_5_1", pd3dShaderBlob);
}

void LitShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	pipelineStates_.clear();
	pipelineStates_.reserve(2);
	Shader::CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
}

GlassShader::GlassShader()
{
}

GlassShader::~GlassShader()
{
}

D3D12_SHADER_BYTECODE GlassShader::CreatePixelShader(ComPtr<ID3DBlob>& pd3dShaderBlob)
{
	return Shader::CompileShaderFromFile(
		L"Shaders.hlsl",
		"PSGlass",
		"ps_5_1",
		pd3dShaderBlob
	);
}

D3D12_RASTERIZER_DESC GlassShader::CreateRasterizerState()
{
	D3D12_RASTERIZER_DESC desc = LitShader::CreateRasterizerState();

	// 얇은 유리판은 뒷면도 보여야 자연스러움
	//desc.CullMode = D3D12_CULL_MODE_NONE;

	return desc;
}

void GlassShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	pipelineStates_.clear();
	pipelineStates_.reserve(1);

	Shader::CreateShader(pd3dDevice, pd3dGraphicsRootSignature);
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