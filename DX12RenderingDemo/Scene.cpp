#include "Scene.h"

ComPtr<ID3D12RootSignature> Scene::CreateGraphicsRootSignature(ID3D12Device* device)
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

void Scene::Load(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	rootSignature_ = CreateGraphicsRootSignature(device);
	OnLoad(device, cmdList);
}

void Scene::Unload()
{
	OnUnload();

	objects_.clear();
	rootSignature_.Reset();
}

void Scene::ReleaseUploadBuffers()
{
	for (auto& object : objects_) {
		if (object) object->ReleaseUploadBuffers();
	}

	OnReleaseUploadBuffers();
}

void Scene::OnProcessingMouseMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
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
}

void Scene::OnProcessingKeyboardMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_KEYUP:
		switch (wParam) {
		default:
			break;
		}
		break;
	default:
		break;
	}
}

void Scene::ProcessInput(const UCHAR* keysBuffer)
{
}

void Scene::Animate(float deltaTime)
{
	for (auto& object : objects_) {
		if (object) object->Animate(deltaTime);
	}
}

void Scene::Render(ID3D12GraphicsCommandList* cmdList, Camera* camera)
{
	if (!cmdList || !camera || !rootSignature_)
		return;

	camera->SetViewportsAndScissorRects(cmdList);
	cmdList->SetGraphicsRootSignature(rootSignature_.Get());
	camera->UpdateShaderVariables(cmdList);

	//씬을 렌더링하는 것은 씬을 구성하는 게임 객체(셰이더를 포함하는 객체)들을 렌더링하는 것이다.
	for (auto& object : objects_) {
		if (object) object->Render(cmdList, camera);
	}
}