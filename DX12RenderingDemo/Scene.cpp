#include "Scene.h"

ComPtr<ID3D12RootSignature> CScene::CreateGraphicsRootSignature(ID3D12Device* device)
{
	ComPtr<ID3D12RootSignature> rootSignature;
	D3D12_ROOT_PARAMETER rootParameters[2];
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParameters[0].Constants.Num32BitValues = 16;
	rootParameters[0].Constants.ShaderRegister = 0;
	rootParameters[0].Constants.RegisterSpace = 0;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParameters[1].Constants.Num32BitValues = 32;
	rootParameters[1].Constants.ShaderRegister = 1;
	rootParameters[1].Constants.RegisterSpace = 0;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = NULL;
	rootSignatureDesc.Flags = rootSignatureFlags;

	ComPtr<ID3DBlob> signatureBlob;
	ComPtr<ID3DBlob> errorBlob;

	ThrowIfFailedWithBlob(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, signatureBlob.GetAddressOf(), errorBlob.GetAddressOf()), errorBlob.Get());
	ThrowIfFailed(device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(rootSignature.GetAddressOf())));
	
	return(rootSignature);
}

void CScene::Load(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	mGraphicsRootSignature = CreateGraphicsRootSignature(device);
	OnLoad(device, cmdList);
}

void CScene::Unload()
{
	OnUnload();

	mObjects.clear();
	mGraphicsRootSignature.Reset();
}

void CScene::ReleaseUploadBuffers()
{
	for (auto& object : mObjects) {
		if (object) object->ReleaseUploadBuffers();
	}

	OnReleaseUploadBuffers();
}

bool CScene::OnProcessingMouseMessage(HWND hWnd, UINT messageID, WPARAM wParam, LPARAM lParam)
{
	switch (messageID)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		break;
	case WM_MOUSEMOVE:
		break;
	default:
		break;
	}
	
	return(false);
}

bool CScene::OnProcessingKeyboardMessage(HWND hWnd, UINT messageID, WPARAM wParam, LPARAM lParam)
{
	switch (messageID) {
	case WM_KEYUP:
		switch (wParam) {
		case VK_ESCAPE:
			break;
		case VK_RETURN:
			break;
		case VK_F8:
			break;
		case VK_F9:
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return(false);
}

bool CScene::ProcessInput(const UCHAR* keysBuffer)
{
	return(false);
}

void CScene::Animate(float deltaTime)
{
	for (auto& object : mObjects) {
		if (object) object->Animate(deltaTime);
	}
}

void CScene::Render(ID3D12GraphicsCommandList* cmdList, CCamera* camera)
{
	if (!cmdList || !camera || !mGraphicsRootSignature)
		return;

	camera->SetViewportsAndScissorRects(cmdList);
	cmdList->SetGraphicsRootSignature(mGraphicsRootSignature.Get());
	camera->UpdateShaderVariables(cmdList);

	//씬을 렌더링하는 것은 씬을 구성하는 게임 객체(셰이더를 포함하는 객체)들을 렌더링하는 것이다.
	for (auto& object : mObjects) {
		if (object) object->Render(cmdList, camera);
	}
}