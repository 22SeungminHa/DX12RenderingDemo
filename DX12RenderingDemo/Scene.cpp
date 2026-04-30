#include "Scene.h"
#include "Renderer.h"
#include "InputSystem.h"

Scene::Scene(UINT width, UINT height)
{
	clientWidth_ = width;
	clientHeight_ = height;
};

ComPtr<ID3D12RootSignature> Scene::CreateGraphicsRootSignature(ID3D12Device* device)
{
	ComPtr<ID3D12RootSignature> rootSignature;
	D3D12_ROOT_PARAMETER rootParameters[2]{};

	// b0 : ObjectCB
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].Descriptor.ShaderRegister = 0;
	rootParameters[0].Descriptor.RegisterSpace = 0;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// b1 : PassCB
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].Descriptor.ShaderRegister = 1;
	rootParameters[1].Descriptor.RegisterSpace = 0;
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
	rootSignatureDesc.pStaticSamplers = nullptr;
	rootSignatureDesc.Flags = rootSignatureFlags;

	ComPtr<ID3DBlob> signatureBlob;
	ComPtr<ID3DBlob> errorBlob;

	ThrowIfFailedWithBlob(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, signatureBlob.GetAddressOf(), errorBlob.GetAddressOf()), errorBlob.Get());
	ThrowIfFailed(device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(rootSignature.GetAddressOf())));

	return rootSignature;
}

void Scene::Load(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	rootSignature_ = CreateGraphicsRootSignature(device);

	SetupCameraDesc();
	CreateCamera();

	OnLoad(device, cmdList);
}

void Scene::Unload()
{
	OnUnload();

	objects_.clear();
	cameras_.clear();
	activeCamera_ = nullptr;
	rootSignature_.Reset();
}

void Scene::Resize(UINT width, UINT height)
{
	clientWidth_ = width;
	clientHeight_ = height;

	UpdateCameraProjection(width, height);
	OnResize(width, height);
}

void Scene::ReleaseUploadResources()
{
	for (auto& object : objects_) {
		if (object) object->ReleaseUploadResources();
	}

	OnReleaseUploadResources();
}

void Scene::ProcessInput(const InputSystem& input, float deltaTime)
{
	if (!activeCamera_) return;

	constexpr float moveSpeed = 20.0f;
	constexpr float mouseSensitivity = 0.005f;
	constexpr float zoomSpeed = 0.03f;

	const float moveDistance = moveSpeed * deltaTime;

	if (input.IsKeyDown('W'))
		activeCamera_->MoveForward(moveDistance);

	if (input.IsKeyDown('S'))
		activeCamera_->MoveForward(-moveDistance);

	if (input.IsKeyDown('D'))
		activeCamera_->MoveRight(-moveDistance);

	if (input.IsKeyDown('A'))
		activeCamera_->MoveRight(moveDistance);

	if (input.IsLeftMouseDown())
	{
		POINT delta = input.GetMouseDelta();

		activeCamera_->Rotate(
			delta.x * mouseSensitivity,
			delta.y * mouseSensitivity
		);
	}
}

void Scene::Animate(float deltaTime)
{
	for (auto& object : objects_) {
		if (object) object->Animate(deltaTime);
	}
}

void Scene::CreateCamera()
{
	auto camera = std::make_unique<Camera>();
	float aspect = (clientHeight_ == 0) ? 1.0f : static_cast<float>(clientWidth_) / clientHeight_;

	camera->SetViewport(0, 0, static_cast<float>(clientWidth_), static_cast<float>(clientHeight_), 0.0f, 1.0f);
	camera->SetScissorRect(0, 0, clientWidth_, clientHeight_);
	camera->SetProjection(cameraDesc_.nearZ, cameraDesc_.farZ, aspect, cameraDesc_.fovY);
	camera->SetLookAt(cameraDesc_.eye, cameraDesc_.target, cameraDesc_.up);

	activeCamera_ = camera.get();
	cameras_.push_back(std::move(camera));
}

void Scene::UpdateCameraProjection(UINT width, UINT height)
{
	if (!activeCamera_) return;

	float aspect = (height == 0) ? 1.0f : static_cast<float>(width) / height;

	activeCamera_->SetViewport(0, 0, static_cast<float>(width), static_cast<float>(height));
	activeCamera_->SetScissorRect(0, 0, width, height);
	activeCamera_->SetProjection(cameraDesc_.nearZ, cameraDesc_.farZ, aspect, cameraDesc_.fovY);
}