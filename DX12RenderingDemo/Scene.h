#pragma once
#include "EngineTypes.h"
#include "Mesh.h"
#include "Material.h"
#include "FBXLoader.h"
#include "GameObject.h"
#include "DirectionalLight.h"

class Renderer;
class InputSystem;
class AssetManager;
class Camera;

class Scene
{
public:
	Scene(UINT width, UINT height);
	virtual ~Scene();

	virtual SceneType GetSceneType() const = 0;

	Camera* GetActiveCamera() const { return activeCamera_; }
	const std::vector<std::unique_ptr<GameObject>>& GetObjects() const { return objects_; }
	const SceneLightDesc& GetLightDesc() const { return lightDesc_; }
	const FogDesc& GetFogDesc() const { return fogDesc_; }
	const std::vector<std::unique_ptr<DirectionalLight>>& GetDirectionalLights() const { return directionalLights_; }

	void Load(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		ID3D12RootSignature* rootSignature,
		AssetManager& assetManager);
	void Unload();
	void Resize(UINT width, UINT height);
	void ReleaseUploadResources();

	virtual void ProcessInput(const InputSystem& input, float deltaTime);
	virtual void Animate(float deltaTime);
	virtual void AnimateObject(GameObject* object, float deltaTime);

	GameObject* CreateGameObject();

	GameObject* CreateObject(
		const std::shared_ptr<Mesh>& mesh,
		const std::shared_ptr<Material>& material,
		const Vector3& position = Vector3::Zero,
		const Vector3& scale = Vector3::One
	);
	GameObject* CreateChildObject(
		GameObject* parent,
		const std::shared_ptr<Mesh>& mesh,
		const std::shared_ptr<Material>& material,
		const Vector3& localPosition = Vector3::Zero
	);
	GameObject* CreateFBXObject(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		ID3D12RootSignature* rootSignature,
		AssetManager& assetManager,
		const std::filesystem::path& filePath,
		const Vector3& position,
		const Vector3& scale);

	GameObject* CreateFBXChildObject(
		GameObject* parent,
		const FBXNodeData& modelData,
		const std::shared_ptr<Material>& materialOverride = nullptr,
		std::vector<GameObject*>* outMeshObjects = nullptr,
		bool centerMeshPivot = false
	);

	void BuildFBXNode(
		GameObject* parent,
		const FBXNodeData& nodeData,
		const std::shared_ptr<Material>& materialOverride = nullptr,
		std::vector<GameObject*>* outMeshObjects = nullptr,
		bool centerMeshPivot = false
	);

	void PrepareRenderResources(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		std::vector<ComPtr<ID3D12Resource>>& transientUploadResources);

	void DestroyGameObject(GameObject* object);

	void SetSkybox(const SkyboxDesc& skybox) { skybox_ = skybox; }
	void SetSkybox(const std::wstring& name = L"Skybox4") { skybox_.SetCubemap(name); }
	const SkyboxDesc& GetSkybox() const { return skybox_; }

protected:
	virtual void OnLoad(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		ID3D12RootSignature* rootSignature,
		AssetManager& assetManager) = 0;
	virtual void OnUnload() {}
	virtual void OnResize(UINT width, UINT height) {}

	virtual void OnReleaseUploadResources() {}
	virtual void OnProcessInput(const InputSystem& input, float deltaTime) {};
	virtual void OnPrepareRenderResources(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		std::vector<ComPtr<ID3D12Resource>>& transientUploadResources) {}

	virtual CameraDesc SetupCameraDesc() const { return CameraDesc{}; }
	virtual SceneLightDesc SetupLightDesc() const { return SceneLightDesc{}; }
	virtual FogDesc SetupFogDesc() const { return FogDesc{}; }

	void CreateCamera();
	void ResizeCamera(UINT width, UINT height);

	void FlushDestroyedGameObjects();

	virtual bool IsFreeCameraControlEnabled() const { return true; }

protected:
	std::vector<std::unique_ptr<GameObject>> objects_;
	UINT nextObjectCBIndex_ = 0;

	std::vector<std::unique_ptr<Camera>> cameras_;
	Camera* activeCamera_ = nullptr;

	SkyboxDesc skybox_;

	SceneLightDesc lightDesc_;
	FogDesc fogDesc_;

	std::vector<std::unique_ptr<DirectionalLight>> directionalLights_;

	UINT clientWidth_ = 0;
	UINT clientHeight_ = 0;

public:
	template<typename T, typename Func>
	void ForEachComponent(Func&& func)
	{
		for (auto& object : objects_)
			ForEachComponentRecursive<T>(object.get(), func);
	}

	template<typename Func>
	void ForEachObject(ObjectType type, Func&& func)
	{
		for (auto& object : objects_)
			ForEachObjectRecursive(object.get(), type, func);
	}

	DirectionalLight* AddDirectionalLight()
	{
		auto light = std::make_unique<DirectionalLight>();

		DirectionalLight* ptr = light.get();
		directionalLights_.push_back(std::move(light));

		return ptr;
	}

	template<typename T, typename... Args>
	T* CreateGameObject(Args&&... args)
	{
		static_assert(
			std::is_base_of_v<GameObject, T>,
			"T must derive from GameObject"
			);

		auto object =
			std::make_unique<T>(
				std::forward<Args>(args)...);

		T* ptr = object.get();

		ptr->SetObjectCBIndex(
			nextObjectCBIndex_++);

		objects_.push_back(
			std::move(object));

		return ptr;
	}

private:
	template<typename T, typename Func>
	void ForEachComponentRecursive(GameObject* object, Func& func)
	{
		if (!object)
			return;

		if (auto* component = object->GetComponent<T>())
			func(*component);

		for (const auto& child : object->GetChildren())
			ForEachComponentRecursive<T>(child.get(), func);
	}

	template<typename Func>
	void ForEachObjectRecursive(GameObject* object, ObjectType type, Func& func)
	{
		if (!object)
			return;

		if (object->GetObjectType() == type)
			func(*object);

		for (const auto& child : object->GetChildren())
			ForEachObjectRecursive(child.get(), type, func);
	}

	void RebuildObjectCBIndices();
	void RebuildObjectCBIndicesRecursive(GameObject* object);
};