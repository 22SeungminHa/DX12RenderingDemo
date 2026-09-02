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

	lightDesc_ = SetupLightDesc();
	fogDesc_ = SetupFogDesc();

	CreateCamera();

	OnLoad(device, cmdList, rootSignature, assetManager);
}

void Scene::Unload()
{
	OnUnload();

	objects_.clear();
	cameras_.clear();
	directionalLights_.clear();

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

void Scene::ProcessInput(
	const InputSystem& input,
	float deltaTime)
{
	OnProcessInput(input, deltaTime);

	if (!activeCamera_)
		return;

	if (!IsFreeCameraControlEnabled())
		return;

	constexpr float moveSpeed = 20.0f;
	constexpr float mouseSensitivity = 0.005f;

	const float moveDistance =
		moveSpeed * deltaTime;

	if (input.IsKeyDown('W'))
		activeCamera_->MoveForward(moveDistance);

	if (input.IsKeyDown('S'))
		activeCamera_->MoveForward(-moveDistance);

	if (input.IsKeyDown('D'))
		activeCamera_->MoveRight(-moveDistance);

	if (input.IsKeyDown('A'))
		activeCamera_->MoveRight(moveDistance);

	if (input.IsRightMouseDown())
	{
		POINT delta =
			input.GetMouseDelta();

		activeCamera_->Rotate(
			delta.x * -mouseSensitivity,
			delta.y * -mouseSensitivity
		);
	}
}

void Scene::Animate(float deltaTime)
{
	for (auto& object : objects_)
	{
		AnimateObject(
			object.get(),
			deltaTime
		);
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

GameObject* Scene::CreateChildObject(
	GameObject* parent,
	const std::shared_ptr<Mesh>& mesh,
	const std::shared_ptr<Material>& material,
	const Vector3& localPosition)
{
	if (!parent)
		return nullptr;

	auto child = std::make_unique<GameObject>();

	child->SetObjectCBIndex(nextObjectCBIndex_++);
	child->SetPosition(localPosition);
	child->SetMesh(mesh);
	child->SetMaterial(material);

	GameObject* ptr = child.get();

	parent->AddChild(std::move(child));

	return ptr;
}

GameObject* Scene::CreateFBXObject(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	ID3D12RootSignature* rootSignature,
	AssetManager& assetManager,
	const std::filesystem::path& filePath,
	const Vector3& position,
	const Vector3& scale)
{
	auto modelData = FBXLoader::LoadLitModel(
		device,
		cmdList,
		rootSignature,
		assetManager,
		filePath
	);

	if (!modelData)
		return nullptr;

	GameObject* root = CreateGameObject();
	root->SetPosition(position);
	root->SetScale(scale);

	CreateFBXChildObject(
		root,
		*modelData
	);

	return root;
}

GameObject* Scene::CreateFBXChildObject(
	GameObject* parent,
	const FBXNodeData& modelData,
	const std::shared_ptr<Material>& materialOverride,
	std::vector<GameObject*>* outMeshObjects)
{
	if (!parent)
		return nullptr;

	auto fbxRoot = std::make_unique<GameObject>();

	fbxRoot->SetObjectCBIndex(nextObjectCBIndex_++);
	fbxRoot->GetTransform()->SetLocalMatrix(modelData.localMatrix);

	GameObject* fbxRootPtr = fbxRoot.get();

	parent->AddChild(std::move(fbxRoot));

	BuildFBXNode(
		fbxRootPtr,
		modelData,
		materialOverride,
		outMeshObjects
	);

	return fbxRootPtr;
}

void Scene::BuildFBXNode(
	GameObject* parent,
	const FBXNodeData& nodeData,
	const std::shared_ptr<Material>& materialOverride,
	std::vector<GameObject*>* outMeshObjects)
{
	for (const auto& meshData : nodeData.meshes)
	{
		auto child = std::make_unique<GameObject>();

		child->SetObjectCBIndex(nextObjectCBIndex_++);
		child->SetMesh(meshData.mesh);
		child->SetMaterial(
			materialOverride
			? materialOverride
			: meshData.material
		);

		GameObject* childPtr = child.get();

		parent->AddChild(std::move(child));

		if (outMeshObjects)
			outMeshObjects->push_back(childPtr);
	}

	for (const auto& childNode : nodeData.children)
	{
		auto child = std::make_unique<GameObject>();

		child->SetObjectCBIndex(nextObjectCBIndex_++);
		child->GetTransform()->SetLocalMatrix(childNode.localMatrix);

		GameObject* childPtr = child.get();

		parent->AddChild(std::move(child));

		BuildFBXNode(
			childPtr,
			childNode,
			materialOverride,
			outMeshObjects
		);
	}
}

void Scene::PrepareRenderResources(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	std::vector<ComPtr<ID3D12Resource>>& transientUploadResources)
{
	OnPrepareRenderResources(
		device,
		cmdList,
		transientUploadResources
	);
}

void Scene::DestroyGameObject(GameObject* object)
{
	if (!object)
		return;

	object->MarkForDestroy();
}

void Scene::FlushDestroyedGameObjects()
{
	for (auto& object : objects_)
		if (object)
			object->RemovePendingDestroyChildren();

	objects_.erase(
		std::remove_if(objects_.begin(), objects_.end(),
			[](const std::unique_ptr<GameObject>& object)
			{
				return !object || object->IsPendingDestroy();
			}),
		objects_.end()
	);

	RebuildObjectCBIndices();
}

void Scene::RebuildObjectCBIndices()
{
	nextObjectCBIndex_ = 0;

	for (auto& object : objects_)
	{
		RebuildObjectCBIndicesRecursive(object.get());
	}
}

void Scene::RebuildObjectCBIndicesRecursive(
	GameObject* object)
{
	if (!object)
		return;

	object->SetObjectCBIndex(nextObjectCBIndex_++);

	for (const auto& child : object->GetChildren())
	{
		RebuildObjectCBIndicesRecursive(child.get());
	}
}