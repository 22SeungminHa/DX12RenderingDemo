#include "Scene.h"
#include "Renderer.h"
#include "InputSystem.h"
#include "AssetManager.h"
#include "Camera.h"
#include "GameObject.h"

Scene::Scene(UINT width, UINT height)
{
	clientWidth_ = width;
	clientHeight_ = height;
};

Scene::~Scene() {}

void Scene::Load(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	ID3D12RootSignature* rootSignature,
	AssetManager& assetManager)
{
	objects_.clear();
	nextObjectCBIndex_ = 0;

	CreateCamera();

	OnLoad(device, cmdList, rootSignature, assetManager);
}

void Scene::Unload()
{
	OnUnload();

	objects_.clear();
	cameras_.clear();
	activeCamera_ = nullptr;
}

void Scene::Resize(UINT width, UINT height)
{
	clientWidth_ = width;
	clientHeight_ = height;

	ResizeCamera(width, height);
	OnResize(width, height);
}

void Scene::ReleaseUploadResources()
{
	OnReleaseUploadResources();
}

void Scene::ProcessInput(const InputSystem& input, float deltaTime)
{
	OnProcessInput(input, deltaTime);

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
		AnimateObject(object.get(), deltaTime);
	}
}

void Scene::AnimateObject(GameObject* object, float deltaTime)
{
	if (!object) return;

	object->Animate(deltaTime);

	for (auto& child : object->GetChildren())
	{
		AnimateObject(child.get(), deltaTime);
	}
}

void Scene::CreateCamera()
{
	auto camera = std::make_unique<Camera>();
	
	camera->SetDesc(SetupCameraDesc());
	camera->Initialize(clientWidth_, clientHeight_);

	activeCamera_ = camera.get();
	cameras_.push_back(std::move(camera));
}

void Scene::ResizeCamera(UINT width, UINT height)
{
	if (!activeCamera_) return;

	activeCamera_->Resize(width, height);
}

GameObject* Scene::CreateGameObject()
{
	auto object = std::make_unique<GameObject>();

	GameObject* ptr = object.get();
	ptr->SetObjectCBIndex(nextObjectCBIndex_++);

	objects_.push_back(std::move(object));

	return ptr;
}

GameObject* Scene::CreateObject(
	const std::shared_ptr<Mesh>& mesh,
	const std::shared_ptr<Material>& material,
	const Vector3& position,
	const Vector3& scale)
{
	GameObject* object = CreateGameObject();

	object->SetPosition(position);
	object->SetScale(scale);
	object->SetMesh(mesh);
	object->SetMaterial(material);

	return object;
}