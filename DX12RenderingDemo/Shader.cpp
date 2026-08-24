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
	const std::filesystem::path& filePath,
	LPCSTR shaderName,
	LPCSTR shaderProfile,
	ComPtr<ID3DBlob>& shaderBlob)
{
	shaderBlob = D3DUtil::CompileShader(
		filePath,
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

void Shader::OnPrepareRender(ID3D12GraphicsCommandList* cmdList, RenderMode renderMode, RenderPass renderPass)
{
	if (!cmdList || pipelineStates_.empty())
		return;

	UINT psoIndex = 0;

	if (pipelineStates_.size() >= 2)
		psoIndex = (renderMode == RenderMode::Transparent) ? 1 : 0;

	cmdList->SetPipelineState(pipelineStates_[psoIndex].Get());
}

void Shader::Render(ID3D12GraphicsCommandList* cmdList, Camera* pCamera, RenderMode renderMode, RenderPass renderPass)
{
	OnPrepareRender(cmdList, renderMode, renderPass);
}