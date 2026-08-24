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
    pipelineStates_.reserve(2);

    Shader::CreateShader(device, rootSignature);
}

void GlassShader::CreatePipelineStates(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
    // 0 : 기존 Direct Glass
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

    desc.BlendState = CreateBlendState(RenderMode::Transparent);
    desc.DepthStencilState =
        CreateDepthStencilState(RenderMode::Transparent);

    ComPtr<ID3D12PipelineState> transparentPso;

    ThrowIfFailed(device->CreateGraphicsPipelineState(
        &desc,
        IID_PPV_ARGS(transparentPso.GetAddressOf())));

    pipelineStates_.push_back(std::move(transparentPso));

    // 1 : Glass Accumulation MRT
    ComPtr<ID3DBlob> accumulationPsBlob;

    desc.PS = CreateAccumulationPixelShader(accumulationPsBlob);

    desc.NumRenderTargets = 2;
    desc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    desc.RTVFormats[1] = DXGI_FORMAT_R16_FLOAT;

    desc.BlendState = CreateAccumulationBlendState();
    desc.DepthStencilState =
        CreateDepthStencilState(RenderMode::Transparent);

    ComPtr<ID3D12PipelineState> accumulationPso;

    ThrowIfFailed(device->CreateGraphicsPipelineState(
        &desc,
        IID_PPV_ARGS(accumulationPso.GetAddressOf())));

    pipelineStates_.push_back(std::move(accumulationPso));
}

D3D12_SHADER_BYTECODE GlassShader::CreateAccumulationPixelShader(ComPtr<ID3DBlob>& shaderBlob)
{
	return Shader::CompileShaderFromFile(
		AssetPath::ShaderFile(L"Glass"),
		"PSGlassAccumulation",
		"ps_5_1",
		shaderBlob
	);
}

D3D12_BLEND_DESC GlassShader::CreateAccumulationBlendState()
{
    D3D12_BLEND_DESC desc{};
    desc.AlphaToCoverageEnable = FALSE;
    desc.IndependentBlendEnable = TRUE;

    // RT0 : GlassAccumColor
    {
        auto& rt = desc.RenderTarget[0];

        rt.BlendEnable = TRUE;
        rt.LogicOpEnable = FALSE;

        rt.SrcBlend = D3D12_BLEND_ONE;
        rt.DestBlend = D3D12_BLEND_ONE;
        rt.BlendOp = D3D12_BLEND_OP_ADD;

        rt.SrcBlendAlpha = D3D12_BLEND_ONE;
        rt.DestBlendAlpha = D3D12_BLEND_ONE;
        rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;

        rt.LogicOp = D3D12_LOGIC_OP_NOOP;
        rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }

    // RT1 : GlassRevealage
    {
        auto& rt = desc.RenderTarget[1];

        rt.BlendEnable = TRUE;
        rt.LogicOpEnable = FALSE;

        rt.SrcBlend = D3D12_BLEND_ZERO;
        rt.DestBlend = D3D12_BLEND_INV_SRC_COLOR;
        rt.BlendOp = D3D12_BLEND_OP_ADD;

        rt.SrcBlendAlpha = D3D12_BLEND_ZERO;
        rt.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;

        rt.LogicOp = D3D12_LOGIC_OP_NOOP;
        rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_RED;
    }

    return desc;
}

void GlassShader::OnPrepareRender(ID3D12GraphicsCommandList* cmdList, RenderMode renderMode, RenderPass renderPass)
{
    if (!cmdList || pipelineStates_.empty())
        return;

    UINT psoIndex = 0;

    if (renderPass == RenderPass::GlassAccumulation)
        psoIndex = 1;

    if (psoIndex >= pipelineStates_.size())
        return;

    cmdList->SetPipelineState(pipelineStates_[psoIndex].Get());
}