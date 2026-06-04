#include "Shader.h"

D3D12_RASTERIZER_DESC Shader::CreateRasterizerState()
{
	D3D12_RASTERIZER_DESC desc{};
	desc.FillMode = D3D12_FILL_MODE_SOLID; // D3D12_FILL_MODE_SOLID, D3D12_FILL_MODE_WIREFRAME
	desc.CullMode = D3D12_CULL_MODE_BACK;  // D3D12_CULL_MODE_BACK, D3D12_CULL_MODE_NONE, D3D12_CULL_MODE_FRONT.
	desc.FrontCounterClockwise = TRUE;
	desc.DepthBias = 0;
	desc.DepthBiasClamp = 0.0f;
	desc.SlopeScaledDepthBias = 0.0f;
	desc.DepthClipEnable = TRUE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;
	desc.ForcedSampleCount = 0;
	desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return desc;
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
	D3D12_INPUT_LAYOUT_DESC desc{};
	desc.pInputElementDescs = nullptr;
	desc.NumElements = 0;
	return desc;
}

D3D12_SHADER_BYTECODE Shader::CreateVertexShader(ComPtr<ID3DBlob>& shaderBlob)
{
	shaderBlob.Reset();

	D3D12_SHADER_BYTECODE shaderByteCode;
	shaderByteCode.BytecodeLength = 0;
	shaderByteCode.pShaderBytecode = NULL;
	return shaderByteCode;
}
D3D12_SHADER_BYTECODE Shader::CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob)
{
	shaderBlob.Reset();

	D3D12_SHADER_BYTECODE shaderByteCode;
	shaderByteCode.BytecodeLength = 0;
	shaderByteCode.pShaderBytecode = NULL;
	return shaderByteCode;
}

//셰이더 소스 코드를 컴파일하여 바이트 코드 구조체를 반환한다. 
D3D12_SHADER_BYTECODE Shader::CompileShaderFromFile(
	const WCHAR* fileName,
	LPCSTR shaderName,
	LPCSTR shaderProfile,
	ComPtr<ID3DBlob>& shaderBlob)
{
	shaderBlob = D3DUtil::CompileShader(
		fileName,
		nullptr,
		shaderName,
		shaderProfile
	);

	D3D12_SHADER_BYTECODE shaderByteCode{};
	shaderByteCode.BytecodeLength = shaderBlob->GetBufferSize();
	shaderByteCode.pShaderBytecode = shaderBlob->GetBufferPointer();

	return shaderByteCode;
}

//그래픽스 파이프라인 상태 객체를 생성한다. 
void Shader::CreateShader(ID3D12Device* device, ID3D12RootSignature* rootSignature)
{
	ComPtr<ID3DBlob> vsBlob;
	ComPtr<ID3DBlob> psBlob;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	desc.pRootSignature = rootSignature;
	desc.VS = CreateVertexShader(vsBlob);
	desc.PS = CreatePixelShader(psBlob);
	desc.RasterizerState = CreateRasterizerState();
	desc.InputLayout = CreateInputLayout();
	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = CreateRtvFormat();
	desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.SampleDesc.Count = 1;
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	CreatePipelineStates(device, desc);
}

DXGI_FORMAT Shader::CreateRtvFormat() const
{
	return DXGI_FORMAT_R16G16B16A16_FLOAT;
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

void Shader::OnPrepareRender(ID3D12GraphicsCommandList* cmdList, RenderMode renderMode)
{
	if (!cmdList || pipelineStates_.empty())
		return;

	UINT psoIndex = 0;

	if (pipelineStates_.size() >= 2)
		psoIndex = (renderMode == RenderMode::Transparent) ? 1 : 0;

	cmdList->SetPipelineState(pipelineStates_[psoIndex].Get());
}

void Shader::Render(ID3D12GraphicsCommandList* cmdList, Camera* pCamera, RenderMode renderMode)
{
	OnPrepareRender(cmdList, renderMode);
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

	D3D12_INPUT_LAYOUT_DESC desc{};
	desc.pInputElementDescs = inputElementDescs_.data();
	desc.NumElements = static_cast<UINT>(inputElementDescs_.size());

	return desc;
}

D3D12_SHADER_BYTECODE LitShader::CreateVertexShader(ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(L"Shaders.hlsl", "VSLit", "vs_5_1", shaderBlob);
}

D3D12_SHADER_BYTECODE LitShader::CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(L"Shaders.hlsl", "PSLit", "ps_5_1", shaderBlob);
}

void LitShader::CreateShader(ID3D12Device* device, ID3D12RootSignature* rootSignature)
{
	pipelineStates_.clear();
	pipelineStates_.reserve(2);
	Shader::CreateShader(device, rootSignature);
}

GlassShader::GlassShader()
{
}

GlassShader::~GlassShader()
{
}

D3D12_SHADER_BYTECODE GlassShader::CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(
		L"Shaders.hlsl",
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

D3D12_INPUT_LAYOUT_DESC SkyboxShader::CreateInputLayout()
{
	inputElementDescs_ =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_INPUT_LAYOUT_DESC desc{};
	desc.pInputElementDescs = inputElementDescs_.data();
	desc.NumElements = static_cast<UINT>(inputElementDescs_.size());

	return desc;
}

D3D12_SHADER_BYTECODE SkyboxShader::CreateVertexShader(ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(L"Shaders.hlsl", "VSSkybox", "vs_5_1", shaderBlob);
}

D3D12_SHADER_BYTECODE SkyboxShader::CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(L"Shaders.hlsl", "PSSkybox", "ps_5_1", shaderBlob);
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
		L"Shaders.hlsl",
		"VSFullscreen",
		"vs_5_1",
		shaderBlob
	);
}

D3D12_SHADER_BYTECODE PostProcessShader::CreatePixelShader(ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(
		L"Shaders.hlsl",
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